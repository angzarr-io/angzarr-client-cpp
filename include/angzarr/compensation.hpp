#pragma once

#include <optional>
#include <string>

#include "angzarr/proto_aliases.hpp"
#include "helpers.hpp"

namespace angzarr {

/**
 * Extracted context from a rejection Notification.
 *
 * When a saga or process manager issues a command that gets rejected, the
 * framework sends a Notification containing rejection details.
 * CompensationContext extracts this information into a developer-friendly
 * structure.
 *
 * **Why this matters:**
 * - Debugging: Understand which component issued the failing command
 * - Compensation logic: Decide whether to retry, rollback, or escalate
 * - Observability: Log structured rejection data for monitoring
 * - Business rules: Different compensation for different rejection reasons
 *
 * Without CompensationContext, developers must manually unpack nested protobuf
 * messages (Notification -> Any -> RejectionNotification -> fields), which is
 * error-prone and obscures the business logic in boilerplate.
 *
 * Example:
 *   auto ctx = CompensationContext::from_notification(notification);
 *   if (ctx.rejected_command_type() == "ReserveStock") {
 *       // Emit compensation events
 *       StockReleased release;
 *       release.set_reason(ctx.rejection_reason());
 *       return emit_compensation_events(new_event_book(release));
 *   }
 */
class CompensationContext {
 public:
  /**
   * Extract compensation context from a Notification.
   *
   * If the notification payload is not a RejectionNotification or cannot
   * be unpacked, returns a context with default/empty values.
   *
   * @param notification The notification containing rejection details
   * @return A new CompensationContext
   */
  static CompensationContext from_notification(
      const Notification& notification) {
    CompensationContext ctx;

    if (notification.has_payload()) {
      RejectionNotification rejection;
      if (notification.payload().UnpackTo(&rejection)) {
        ctx.rejection_reason_ = rejection.rejection_reason();

        if (rejection.has_rejected_command()) {
          ctx.rejected_command_ = rejection.rejected_command();

          // Extract source info from angzarr_deferred header
          if (rejection.rejected_command().pages_size() > 0) {
            const auto& page = rejection.rejected_command().pages(0);
            if (page.has_header() && page.header().has_angzarr_deferred()) {
              const auto& deferred = page.header().angzarr_deferred();
              ctx.source_event_sequence_ = deferred.source_seq();
              if (deferred.has_source()) {
                ctx.source_aggregate_ = deferred.source();
              }
            }
          }
        }
      }
    }

    return ctx;
  }

  /**
   * Sequence of the event that triggered the saga/PM flow.
   */
  uint32_t source_event_sequence() const { return source_event_sequence_; }

  /**
   * Why the command was rejected.
   */
  const std::string& rejection_reason() const { return rejection_reason_; }

  /**
   * The command that was rejected (if available).
   */
  const std::optional<CommandBook>& rejected_command() const {
    return rejected_command_;
  }

  /**
   * Cover of the aggregate that triggered the flow (if available).
   */
  const std::optional<Cover>& source_aggregate() const {
    return source_aggregate_;
  }

  /**
   * Get the type URL of the rejected command, if available.
   *
   * Compensation handlers are often keyed by command type:
   * "If ReserveStock was rejected, release the hold."
   *
   * @return The type name suffix (e.g., "ReserveStock") or empty string
   */
  std::string rejected_command_type() const {
    if (!rejected_command_.has_value() ||
        rejected_command_->pages_size() == 0) {
      return "";
    }

    const auto& page = rejected_command_->pages(0);
    if (!page.has_command()) {
      return "";
    }

    return helpers::type_name_from_url(page.command().type_url());
  }

  /**
   * Build a dispatch key for routing rejection handlers.
   *
   * @return A key in format "domain/command" or empty string
   */
  std::string dispatch_key() const {
    if (!rejected_command_.has_value()) {
      return "";
    }

    std::string domain = rejected_command_->has_cover()
                             ? rejected_command_->cover().domain()
                             : "";
    std::string cmd_type = rejected_command_type();

    if (domain.empty() || cmd_type.empty()) {
      return "";
    }

    return domain + "/" + cmd_type;
  }

 private:
  uint32_t source_event_sequence_ = 0;
  std::string rejection_reason_;
  std::optional<CommandBook> rejected_command_;
  std::optional<Cover> source_aggregate_;
};

// --- Aggregate compensation helpers ---

/**
 * Notification type name constant for type URL matching.
 */
constexpr const char* kNotificationTypeName =
    "angzarr_client.proto.angzarr.Notification";

/**
 * Delegate compensation to the framework with emit_system_revocation=true.
 *
 * Use when the aggregate doesn't have custom compensation logic.
 * The framework will emit a SagaCompensationFailed event to the fallback
 * domain.
 *
 * @param reason Why the command was rejected / context for the delegation
 * @return BusinessResponse with revocation flags
 */
inline BusinessResponse delegate_to_framework(const std::string& reason) {
  BusinessResponse response;
  auto* revocation = response.mutable_revocation();
  revocation->set_emit_system_revocation(true);
  revocation->set_reason(reason);
  return response;
}

/**
 * Delegate compensation to the framework with custom revocation flags.
 *
 * @param reason Context for the delegation
 * @param emit_system_event Whether to emit SagaCompensationFailed
 * @param send_to_dlq Whether to route to dead letter queue
 * @param escalate Whether to trigger webhook notification
 * @param abort Whether to stop saga chain and propagate error
 * @return BusinessResponse with revocation flags
 */
inline BusinessResponse delegate_to_framework(const std::string& reason,
                                              bool emit_system_event,
                                              bool send_to_dlq, bool escalate,
                                              bool abort) {
  BusinessResponse response;
  auto* revocation = response.mutable_revocation();
  revocation->set_emit_system_revocation(emit_system_event);
  revocation->set_send_to_dead_letter_queue(send_to_dlq);
  revocation->set_escalate(escalate);
  revocation->set_abort(abort);
  revocation->set_reason(reason);
  return response;
}

/**
 * Create a response containing compensation events.
 *
 * Use when the aggregate emits events to record compensation.
 * The framework will persist these events and NOT emit a system event.
 *
 * @param events The compensation events to persist
 * @return BusinessResponse with events
 */
inline BusinessResponse emit_compensation_events(const EventBook& events) {
  BusinessResponse response;
  *response.mutable_events() = events;
  return response;
}

// --- Process Manager compensation helpers ---

/**
 * Holds PM compensation results: optional process events plus framework action
 * flags.
 */
struct PMRevocationResponse {
  /** PM events to persist (may be empty/nullopt). */
  std::optional<EventBook> process_events;

  /** Framework action flags. */
  RevocationResponse revocation;
};

/**
 * Create a PM response that delegates compensation to the framework.
 *
 * Use when the PM doesn't have custom compensation logic.
 *
 * @param reason Context for the delegation
 * @return PMRevocationResponse with emit_system_revocation=true
 */
inline PMRevocationResponse pm_delegate_to_framework(
    const std::string& reason) {
  RevocationResponse rev;
  rev.set_emit_system_revocation(true);
  rev.set_reason(reason);
  return PMRevocationResponse{std::nullopt, std::move(rev)};
}

/**
 * Create a PM response that delegates compensation with custom emit flag.
 *
 * @param reason Context for the delegation
 * @param emit_system_event Whether to emit SagaCompensationFailed
 * @return PMRevocationResponse with specified flags
 */
inline PMRevocationResponse pm_delegate_to_framework(const std::string& reason,
                                                     bool emit_system_event) {
  RevocationResponse rev;
  rev.set_emit_system_revocation(emit_system_event);
  rev.set_reason(reason);
  return PMRevocationResponse{std::nullopt, std::move(rev)};
}

/**
 * Create a PM response with compensation events and revocation flags.
 *
 * Use when the PM emits events to record the failure in its state.
 *
 * @param events Compensation events to persist
 * @param also_emit Whether to also emit SagaCompensationFailed
 * @param reason Context for the revocation
 * @return PMRevocationResponse with events and revocation flags
 */
inline PMRevocationResponse pm_emit_compensation_events(
    const EventBook& events, bool also_emit, const std::string& reason) {
  RevocationResponse rev;
  rev.set_emit_system_revocation(also_emit);
  rev.set_reason(reason);
  return PMRevocationResponse{events, std::move(rev)};
}

// --- Helper functions ---

/**
 * Check if a type URL represents a rejection Notification.
 *
 * @param type_url The type URL to check
 * @return true if the type URL matches the Notification type
 */
inline bool is_notification(const std::string& type_url) {
  return type_url ==
         std::string(helpers::TYPE_URL_PREFIX) + kNotificationTypeName;
}

}  // namespace angzarr
