#pragma once

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "angzarr/error_codes.hpp"
#include "angzarr/proto_aliases.hpp"
#include "client.hpp"
#include "errors.hpp"
#include "helpers.hpp"

namespace angzarr {

/**
 * Fluent builder for constructing and executing commands.
 *
 * CommandBuilder reduces boilerplate when creating commands:
 *
 * - Chain method calls instead of nested object construction
 * - Type-safe methods prevent invalid field combinations
 * - Auto-generates correlation IDs when not provided
 * - Build incrementally, execute when ready
 *
 * Example:
 *   auto response = CommandBuilder(client.aggregate(), "orders")
 *       .with_root(order_id)
 *       .with_correlation_id("corr-123")
 *       .with_sequence(5)
 *       .with_command("type.googleapis.com/orders.CreateOrder", create_cmd)
 *       .execute();
 */
class CommandBuilder {
 public:
  /**
   * Tag for the auto-generate-root constructor / command_new factory.
   *
   * Spec HIGH-3.1: audit #67 forbids any path that produces a
   * CommandBook without a stamped root. The previous rootless ctors
   * (builder.hpp:43 + :54 pre-D-C++) let callers accidentally skip
   * the stamp. We retain the public no-explicit-root entry point but
   * gate it behind an :struct:`AutoGenerateRoot` tag, mirroring C#'s
   * ``autoGenerateRoot: true`` named parameter. Callers MUST either
   * pass an explicit root via :func:`CommandBuilder::with_root` or
   * pick the tagged ctor / :func:`command_new` factory — both of
   * which stamp a fresh UUID v4 immediately. There is no longer a
   * way to ``build()`` a CommandBook with no root.
   */
  struct AutoGenerateRoot {};

  /**
   * Create a command builder for an existing aggregate.
   *
   * Spec HIGH-3.1 — the rootless ctor that previously lived at
   * ``builder.hpp:43`` (no explicit root, no auto-gen) was removed;
   * this ctor now requires the caller to supply the root bytes
   * up-front so audit #67 cannot be violated.
   *
   * @param client      The aggregate client to use for execution (nullptr
   *                    for build-only test scenarios).
   * @param domain      The aggregate domain.
   * @param root_bytes  Existing aggregate root UUID as raw bytes
   *                    (16 bytes, network order).
   */
  CommandBuilder(AggregateClient* client, const std::string& domain,
                 const std::string& root_bytes)
      : client_(client), domain_(domain), root_(root_bytes), sequence_(0) {}

  /**
   * Create a command builder for a *new* aggregate, auto-generating
   * the root UUID v4 client-side (audit #20 / P2.4a).
   *
   * Spec HIGH-3.1 — the rootless ctor that previously lived at
   * ``builder.hpp:54`` (``explicit CommandBuilder(const std::string&)``)
   * was removed. New aggregates now go through this tagged ctor (or
   * :func:`command_new`) so the root is stamped before any
   * :func:`build` call — there is no longer a path that produces a
   * cover without a root.
   *
   * @param client The aggregate client to use for execution (nullptr
   *               for build-only test scenarios).
   * @param domain The aggregate domain.
   */
  CommandBuilder(AggregateClient* client, const std::string& domain,
                 AutoGenerateRoot)
      : client_(client),
        domain_(domain),
        root_(helpers::new_uuid_v4_bytes()),
        sequence_(0) {}

  /**
   * Factory: build a CommandBuilder for a *new* aggregate.
   *
   * Materializes a fresh UUID v4 client-side and assigns it as the
   * aggregate root. Audit #20 / P2.4a: ``root`` is always
   * client-assigned, no path exists to skip it.  Mirrors Python's
   * ``CommandHandlerClient.command_new`` and Rust's
   * ``CommandBuilderExt::command_new``.
   */
  static CommandBuilder command_new(AggregateClient* client,
                                    const std::string& domain) {
    return CommandBuilder(client, domain, AutoGenerateRoot{});
  }

  /**
   * Set the aggregate root UUID.
   *
   * For existing aggregates, this identifies which instance to target.
   * For new aggregates, omit this to let the coordinator generate one.
   *
   * @param root_bytes UUID as 16-byte array
   * @return Reference to this builder for chaining
   */
  CommandBuilder& with_root(const std::string& root_bytes) {
    root_ = root_bytes;
    return *this;
  }

  /**
   * Set the correlation ID for request tracing.
   *
   * Correlation IDs link related operations across services.
   * If not set, a UUID will be auto-generated on build.
   *
   * @param id The correlation ID
   * @return Reference to this builder for chaining
   */
  CommandBuilder& with_correlation_id(const std::string& id) {
    correlation_id_ = id;
    return *this;
  }

  /**
   * Set the expected sequence number for optimistic locking.
   *
   * The aggregate will reject commands with mismatched sequences,
   * preventing concurrent modification conflicts.
   *
   * @param seq The sequence number (0 for new aggregates)
   * @return Reference to this builder for chaining
   */
  CommandBuilder& with_sequence(uint32_t seq) {
    sequence_ = seq;
    sequence_set_ = true;
    return *this;
  }

  /**
   * Set the command type URL and message.
   *
   * The message is serialized to bytes and wrapped in protobuf Any.
   *
   * @param type_url Fully-qualified type URL (e.g.,
   * "type.googleapis.com/orders.CreateOrder")
   * @param message The protobuf command message
   * @return Reference to this builder for chaining
   */
  /**
   * Set the merge strategy for conflict resolution.
   */
  CommandBuilder& with_merge_strategy(MergeStrategy strategy) {
    merge_strategy_ = strategy;
    return *this;
  }

  /**
   * Set the sync mode for command execution. Spec MED-3.3 — cross-
   * language parity (Rust ``with_sync_mode``). Default is ASYNC
   * matching Py/Rs/Ja/Cs/Cpp ``execute()`` semantics; callers can
   * configure on the builder, then ``execute()`` honors the stored
   * mode.
   */
  CommandBuilder& with_sync_mode(SyncMode mode) {
    sync_mode_ = mode;
    return *this;
  }

  template <typename T>
  CommandBuilder& with_command(const std::string& type_url, const T& message) {
    type_url_ = type_url;
    payload_ = message.SerializeAsString();
    return *this;
  }

  /**
   * Build the CommandBook without executing.
   *
   * @return The constructed CommandBook
   * @throws InvalidArgumentError if required fields are missing
   */
  CommandBook build() const {
    // Spec HIGH-3.2 — stamp the cross-language SCREAMING_SNAKE code
    // on every builder validation error. Previously the throw was a
    // bare ``InvalidArgumentError(message)`` with no code — parity
    // tests asserting on ``error.code == COMMAND_TYPE_URL_MISSING``
    // only passed in Rust.
    if (!type_url_.has_value()) {
      throw InvalidArgumentError(
          error_codes::messages::COMMAND_TYPE_URL_MISSING,
          error_codes::codes::COMMAND_TYPE_URL_MISSING,
          ClientError::DetailsMap{
              {error_codes::keys::FIELD, "type_url"},
              {error_codes::keys::DOMAIN, domain_},
          });
    }
    if (!payload_.has_value()) {
      throw InvalidArgumentError(error_codes::messages::COMMAND_PAYLOAD_MISSING,
                                 error_codes::codes::COMMAND_PAYLOAD_MISSING,
                                 ClientError::DetailsMap{
                                     {error_codes::keys::FIELD, "payload"},
                                     {error_codes::keys::DOMAIN, domain_},
                                 });
    }
    if (!sequence_set_) {
      throw InvalidArgumentError(
          error_codes::messages::COMMAND_SEQUENCE_MISSING,
          error_codes::codes::COMMAND_SEQUENCE_MISSING,
          ClientError::DetailsMap{
              {error_codes::keys::FIELD, "sequence"},
              {error_codes::keys::DOMAIN, domain_},
          });
    }

    std::string corr_id = correlation_id_.value_or(generate_uuid());

    CommandBook book;
    auto* cover = book.mutable_cover();
    cover->set_domain(domain_);
    cover->set_correlation_id(corr_id);

    if (root_.has_value()) {
      cover->mutable_root()->set_value(root_.value());
    }

    auto* page = book.add_pages();
    page->mutable_header()->set_sequence(sequence_);
    page->set_merge_strategy(merge_strategy_);
    auto* cmd = page->mutable_command();
    cmd->set_type_url(type_url_.value());
    cmd->set_value(payload_.value());

    return book;
  }

  /**
   * Build and execute the command.
   *
   * @return The command response
   * @throws InvalidArgumentError if required fields are missing
   * @throws GrpcError if the gRPC call fails
   */
  CommandResponse execute() {
    auto command = build();
    // Spec MED-3.3: honor builder-configured sync mode. Default is
    // ASYNC which matches CommandHandlerClient::handle() semantics.
    if (sync_mode_.has_value() && *sync_mode_ != SYNC_MODE_ASYNC) {
      return client_->handle_sync(command, *sync_mode_);
    }
    return client_->handle(command);
  }

 private:
  AggregateClient* client_;
  std::string domain_;
  std::optional<std::string> root_;
  std::optional<std::string> correlation_id_;
  uint32_t sequence_;
  bool sequence_set_ = false;
  MergeStrategy merge_strategy_ = MERGE_COMMUTATIVE;
  std::optional<std::string> type_url_;
  std::optional<std::string> payload_;
  std::optional<SyncMode> sync_mode_;

  // Spec MED-3.7 — replace the old ``rand()``-based generator (which
  // had no seeding and no RFC 4122 grade bit-stamping) with the
  // ``helpers::new_uuid_v4_bytes()`` cryptographically-seeded
  // generator already used by ``command_new``. Formats the raw bytes
  // through the canonical 8-4-4-4-12 hex form.
  static std::string generate_uuid() {
    auto bytes = helpers::new_uuid_v4_bytes();
    return helpers::bytes_to_uuid_text(bytes);
  }
};

/**
 * Fluent builder for constructing and executing event queries.
 *
 * QueryBuilder supports multiple access patterns:
 *
 * - By root: Fetch all events for a specific aggregate instance
 * - By correlation ID: Fetch events across aggregates in a workflow
 * - By sequence range: Fetch specific event windows for pagination
 * - By temporal point: Reconstruct historical state (as-of queries)
 * - By edition: Query from specific schema versions after upcasting
 *
 * Example:
 *   auto events = QueryBuilder(client.query(), "orders")
 *       .with_root(order_id)
 *       .range(10)
 *       .get_event_book();
 */
class QueryBuilder {
 public:
  /**
   * Create a query builder for a domain.
   *
   * @param client The query client to use for execution
   * @param domain The aggregate domain
   */
  QueryBuilder(QueryClient* client, const std::string& domain)
      : client_(client),
        domain_(domain),
        has_range_(false),
        has_temporal_(false) {}

  /**
   * Set the aggregate root UUID.
   *
   * Spec MED-3.9 — this setter is Cpp-only (Py/Rs/Go/Ja/Cs all set
   * root only via ctor). Kept for back-compat with the existing
   * call-site corpus (used heavily in the C++ test suite and the
   * shared ``QueryClient::query(domain, root_bytes)`` convenience
   * route which delegates here). Spec recommends removal in a
   * future iteration; flagged as a cosmetic MED, not a parity bug.
   *
   * @param root_bytes UUID as 16-byte array
   * @return Reference to this builder for chaining
   */
  QueryBuilder& with_root(const std::string& root_bytes) {
    root_ = root_bytes;
    correlation_id_.reset();  // Clear correlation ID when root is set
    return *this;
  }

  /**
   * Query by correlation ID instead of root.
   *
   * Correlation IDs link events across aggregates in a distributed workflow.
   * This enables queries like "show me all events for order workflow corr-456".
   *
   * @param id The correlation ID
   * @return Reference to this builder for chaining
   */
  QueryBuilder& by_correlation_id(const std::string& id) {
    correlation_id_ = id;
    root_.reset();  // Clear root when correlation ID is set
    return *this;
  }

  /**
   * Query events from a specific edition.
   *
   * After upcasting (event schema migration), events exist in multiple
   * editions.
   *
   * @param edition The edition name
   * @return Reference to this builder for chaining
   */
  QueryBuilder& with_edition(const std::string& edition) {
    edition_ = edition;
    return *this;
  }

  /**
   * Query a range of sequences from lower (inclusive).
   *
   * Use for incremental sync: "give me events since sequence N"
   *
   * @param lower The lower bound (inclusive)
   * @return Reference to this builder for chaining
   */
  QueryBuilder& range(uint32_t lower) {
    range_lower_ = lower;
    range_upper_.reset();
    has_range_ = true;
    has_temporal_ = false;
    return *this;
  }

  /**
   * Query a range of sequences with upper bound (inclusive).
   *
   * Use for pagination: fetch events 100-200, then 200-300.
   *
   * @param lower The lower bound (inclusive)
   * @param upper The upper bound (inclusive)
   * @return Reference to this builder for chaining
   */
  QueryBuilder& range_to(uint32_t lower, uint32_t upper) {
    range_lower_ = lower;
    range_upper_ = upper;
    has_range_ = true;
    has_temporal_ = false;
    return *this;
  }

  /**
   * Query state as of a specific sequence number.
   *
   * Essential for debugging: "What was the state when this bug occurred?"
   *
   * @param seq The sequence number
   * @return Reference to this builder for chaining
   */
  QueryBuilder& as_of_sequence(uint32_t seq) {
    temporal_sequence_ = seq;
    has_temporal_ = true;
    has_range_ = false;
    return *this;
  }

  /**
   * Query state as of a specific timestamp (RFC3339 format).
   *
   * Example: "2024-01-15T10:30:00Z"
   *
   * @param rfc3339 The timestamp in RFC3339 format
   * @return Reference to this builder for chaining
   * @throws InvalidTimestampError if timestamp parsing fails
   */
  QueryBuilder& as_of_time(const std::string& rfc3339) {
    // Simple RFC3339 parsing (YYYY-MM-DDTHH:MM:SSZ)
    // Production code should use a proper date library
    temporal_time_ = parse_rfc3339(rfc3339);
    has_temporal_ = true;
    has_range_ = false;
    return *this;
  }

  /**
   * Build the Query without executing.
   *
   * @return The constructed Query
   */
  Query build() const {
    Query query;
    auto* cover = query.mutable_cover();
    cover->set_domain(domain_);

    if (root_.has_value()) {
      cover->mutable_root()->set_value(root_.value());
    }
    if (correlation_id_.has_value()) {
      cover->set_correlation_id(correlation_id_.value());
    }
    if (edition_.has_value()) {
      cover->mutable_edition()->set_name(edition_.value());
    }

    if (has_range_) {
      auto* range = query.mutable_range();
      range->set_lower(range_lower_);
      if (range_upper_.has_value()) {
        range->set_upper(range_upper_.value());
      }
    } else if (has_temporal_) {
      auto* temporal = query.mutable_temporal();
      if (temporal_sequence_.has_value()) {
        temporal->set_as_of_sequence(temporal_sequence_.value());
      } else if (temporal_time_.has_value()) {
        *temporal->mutable_as_of_time() = temporal_time_.value();
      }
    }

    return query;
  }

  /**
   * Execute the query and return a single EventBook.
   *
   * @return The EventBook containing matching events
   * @throws GrpcError if the gRPC call fails
   */
  EventBook get_event_book() {
    auto query = build();
    return client_->get_event_book(query);
  }

  /**
   * Execute the query and return all matching EventBooks.
   *
   * @return Vector of EventBooks
   * @throws GrpcError if the gRPC call fails
   */
  std::vector<EventBook> get_events() {
    auto query = build();
    return client_->get_events(query);
  }

  /**
   * Execute the query and return just the event pages.
   *
   * Convenience method when you only need events, not metadata.
   *
   * @return Vector of EventPages
   * @throws GrpcError if the gRPC call fails
   */
  std::vector<EventPage> get_pages() {
    auto book = get_event_book();
    std::vector<EventPage> pages;
    pages.reserve(book.pages_size());
    for (const auto& page : book.pages()) {
      pages.push_back(page);
    }
    return pages;
  }

 private:
  QueryClient* client_;
  std::string domain_;
  std::optional<std::string> root_;
  std::optional<std::string> correlation_id_;
  std::optional<std::string> edition_;

  bool has_range_;
  uint32_t range_lower_ = 0;
  std::optional<uint32_t> range_upper_;

  bool has_temporal_;
  std::optional<uint32_t> temporal_sequence_;
  std::optional<google::protobuf::Timestamp> temporal_time_;

  static google::protobuf::Timestamp parse_rfc3339(const std::string& rfc3339) {
    // Spec MED-3.6 — full RFC 3339 grammar including fractional
    // seconds (up to nanosecond precision) and timezone designator
    // (``Z`` or ``±HH:MM``). The previous parser only accepted bare
    // ``YYYY-MM-DDTHH:MM:SS`` and silently failed on any of:
    //   - ``2024-01-15T10:30:00Z``           (zulu suffix)
    //   - ``2024-01-15T10:30:00.123Z``       (millisecond fraction)
    //   - ``2024-01-15T10:30:00+02:00``      (offset)
    // All five other languages use ecosystem parsers that accept
    // these forms; C++ now matches.
    google::protobuf::Timestamp ts;

    auto throw_invalid = [&]() {
      throw InvalidTimestampError(error_codes::messages::TIMESTAMP_PARSE_FAILED,
                                  error_codes::codes::TIMESTAMP_PARSE_FAILED,
                                  ClientError::DetailsMap{
                                      {error_codes::keys::INPUT, rfc3339},
                                  });
    };

    if (rfc3339.size() < 19 || rfc3339[10] != 'T') throw_invalid();

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (std::sscanf(rfc3339.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day,
                    &hour, &minute, &second) != 6) {
      throw_invalid();
    }
    if (month < 1 || month > 12 || day < 1 || day > 31) throw_invalid();

    // Parse the tail (fractional seconds + timezone offset).
    size_t pos = 19;  // index of next char after ``YYYY-MM-DDTHH:MM:SS``.
    int32_t nanos = 0;
    if (pos < rfc3339.size() && rfc3339[pos] == '.') {
      ++pos;
      std::string digits;
      while (pos < rfc3339.size() &&
             std::isdigit(static_cast<unsigned char>(rfc3339[pos]))) {
        if (digits.size() < 9) digits.push_back(rfc3339[pos]);
        ++pos;
      }
      if (digits.empty()) throw_invalid();
      while (digits.size() < 9) digits.push_back('0');
      try {
        nanos = std::stoi(digits);
      } catch (...) {
        throw_invalid();
      }
    }
    int tz_offset_seconds = 0;
    if (pos >= rfc3339.size()) throw_invalid();  // require designator
    char tz = rfc3339[pos];
    if (tz == 'Z' || tz == 'z') {
      ++pos;
    } else if (tz == '+' || tz == '-') {
      ++pos;
      if (pos + 5 > rfc3339.size() || rfc3339[pos + 2] != ':') throw_invalid();
      int oh = 0, om = 0;
      if (std::sscanf(rfc3339.c_str() + pos, "%d:%d", &oh, &om) != 2)
        throw_invalid();
      if (oh < 0 || oh > 23 || om < 0 || om > 59) throw_invalid();
      tz_offset_seconds = (oh * 3600 + om * 60) * (tz == '+' ? 1 : -1);
      pos += 5;
    } else {
      throw_invalid();
    }
    if (pos != rfc3339.size()) throw_invalid();

    // Day computation (Howard Hinnant's days-from-civil algorithm —
    // exact for the proleptic Gregorian calendar, handles leap
    // years and centuries without manual branching).
    auto y = static_cast<int64_t>(year) - (month <= 2 ? 1 : 0);
    auto era = (y >= 0 ? y : y - 399) / 400;
    auto yoe = static_cast<uint32_t>(y - era * 400);
    auto doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    auto doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days_since_1970 = era * 146097 + static_cast<int64_t>(doe) - 719468;
    int64_t seconds = days_since_1970 * 86400 + hour * 3600 + minute * 60 +
                      second - tz_offset_seconds;

    ts.set_seconds(seconds);
    ts.set_nanos(nanos);
    return ts;
  }
};

// --- Convenience builder methods on client classes ---
// Defined here (not in client.hpp) to avoid circular include.

/**
 * Start building a command for an existing aggregate.
 *
 * @param domain The aggregate domain
 * @param root_bytes UUID as raw bytes
 * @return A CommandBuilder ready for chaining
 */
inline CommandBuilder CommandHandlerClient::command(
    const std::string& domain, const std::string& root_bytes) {
  // Spec HIGH-3.1: route through the existing-root ctor so the cover
  // is never built without a stamped root.
  return CommandBuilder(this, domain, root_bytes);
}

/**
 * Start building a command for a new aggregate.
 *
 * Audit #20 / P2.4a: materializes a fresh UUID v4 client-side and
 * assigns it as the aggregate root. The server-side never sees a
 * missing-root command for a new aggregate. Mirrors Python's
 * ``CommandHandlerClient.command_new`` and Rust's
 * ``CommandBuilderExt::command_new``.
 *
 * @param domain The aggregate domain
 * @return A CommandBuilder ready for chaining
 */
inline CommandBuilder CommandHandlerClient::command_new(
    const std::string& domain) {
  return CommandBuilder::command_new(this, domain);
}

/**
 * Start building a query for a specific aggregate.
 *
 * @param domain The aggregate domain
 * @param root_bytes UUID as raw bytes
 * @return A QueryBuilder ready for chaining
 */
inline QueryBuilder QueryClient::query(const std::string& domain,
                                       const std::string& root_bytes) {
  return QueryBuilder(this, domain).with_root(root_bytes);
}

/**
 * Start building a query by domain only (use with by_correlation_id).
 *
 * @param domain The aggregate domain
 * @return A QueryBuilder ready for chaining
 */
inline QueryBuilder QueryClient::query_domain(const std::string& domain) {
  return QueryBuilder(this, domain);
}

// --- DomainClient convenience methods ---

inline CommandBuilder DomainClient::command(const std::string& domain,
                                            const std::string& root_bytes) {
  return command_handler_->command(domain, root_bytes);
}

inline CommandBuilder DomainClient::command_new(const std::string& domain) {
  return command_handler_->command_new(domain);
}

inline QueryBuilder DomainClient::query_events(const std::string& domain,
                                               const std::string& root_bytes) {
  return query_->query(domain, root_bytes);
}

inline QueryBuilder DomainClient::query_domain(const std::string& domain) {
  return query_->query_domain(domain);
}

}  // namespace angzarr
