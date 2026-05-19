#pragma once

// Unified Router aggregator for the C++ client.
//
// Spec HIGH-4.2 / HIGH-4.3 / HIGH-4.4: prior to D-C++, C++ had no
// class-level kind decoration, no ``BuildError`` type, no ``Built``
// discriminated return, and no Router aggregator on top of the
// existing per-kind typed routers. This module adds all four, in C++
// idiom (templates over reflection), mirroring C#'s ``UnifiedRouter``
// and Java's ``Router``.
//
// Cross-language convergence pattern: the typed routers
// (``CommandHandlerRouter<S,H>``, ``SagaRouter<H>``,
// ``ProcessManagerRouter<S>``, ``ProjectorRouter``, ``UpcasterRouter``)
// stay as the ergonomic API; :class:`UnifiedRouter` performs the
// cross-cutting build-time validation (``MIXED_HANDLER_KINDS``,
// ``DUPLICATE_COMMAND_HANDLER``, ``ROUTER_NO_HANDLERS``) and returns a
// :struct:`Built` carrying the metadata the per-kind ``run_*_server``
// runners and readiness/sync wiring (audit #74) need.

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "angzarr/component_router.hpp"
#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"
#include "angzarr/router.hpp"
#include "angzarr/upcaster.hpp"

namespace angzarr {

/**
 * Structured build-time error raised by :class:`UnifiedRouter` when a
 * registration or validation invariant is violated. Spec HIGH-4.3 —
 * audit #72 cross-language shape (parallels Py ``BuildError``, Rs
 * ``BuildError``, Ja ``BuildException``, C# ``BuildException``).
 *
 * Carries a stable :func:`ClientError::code` from
 * :namespace:`error_codes::codes` and structured
 * :func:`ClientError::details` keyed by :namespace:`error_codes::keys`.
 */
class BuildError : public ClientError {
 public:
  BuildError(const std::string& message, const std::string& code,
             DetailsMap details = {})
      : ClientError(message, code, std::move(details)) {}
};

/// Spec HIGH-4.4 — five-variant kind label parity with Py/Rs/Ja/Cs/Go.
enum class RouterKind {
  CommandHandler,
  Saga,
  ProcessManager,
  Projector,
  Upcaster,
};

inline const char* router_kind_name(RouterKind k) {
  switch (k) {
    case RouterKind::CommandHandler:
      return "CommandHandler";
    case RouterKind::Saga:
      return "Saga";
    case RouterKind::ProcessManager:
      return "ProcessManager";
    case RouterKind::Projector:
      return "Projector";
    case RouterKind::Upcaster:
      return "Upcaster";
  }
  return "Unknown";
}

/**
 * Cross-language convergence shape returned by :func:`UnifiedRouter::build`.
 * Spec HIGH-4.4 + audit #74.
 *
 * ``handler_count`` and ``output_domains`` are the audit-#74 readiness
 * wiring inputs — the per-kind runner uses them to build the
 * :class:`OutputDomainProbe` set.
 */
struct Built {
  std::string router_name;
  RouterKind kind;
  int handler_count = 0;
  std::vector<std::string> output_domains;
  std::vector<std::string> sync_output_domains;
  bool has_async_outputs = false;
};

/**
 * Aggregator on top of the existing typed per-kind routers. Spec
 * HIGH-4.2 / HIGH-4.4.
 *
 * Usage:
 * @code
 *   CommandHandlerRouter<MyState, MyHandler> ch_router{...};
 *   UnifiedRouter("order")
 *       .add_command_handler(ch_router)
 *       .build();
 * @endcode
 *
 * The aggregator owns no handler state — it merely tracks (a) which
 * kind has been registered, (b) duplicate command-handler keys, and
 * (c) output-domain metadata. The typed routers remain the ergonomic
 * dispatch surface.
 */
class UnifiedRouter {
 public:
  explicit UnifiedRouter(std::string name) : name_(std::move(name)) {}

  const std::string& name() const { return name_; }

  /**
   * Register a typed command handler router. Tracks the
   * ``(domain, type_url)`` key set so :func:`build` can stamp
   * ``DUPLICATE_COMMAND_HANDLER`` when two handlers claim the same
   * pair (audit #62).
   */
  template <typename S, typename H>
  UnifiedRouter& add_command_handler(const CommandHandlerRouter<S, H>& router) {
    set_or_assert_kind(RouterKind::CommandHandler);
    // The C++ template signature carries the command type as ``H``;
    // since each router owns one (domain, handler) pair, the
    // duplicate guard reduces to ``(domain, handler-type-id)``
    // ↔ unique. ``typeid().name()`` is unspecified but stable per
    // translation unit, which is enough for build-time validation.
    auto key = std::make_pair(router.domain(), std::string(typeid(H).name()));
    if (!command_keys_.insert(key).second) {
      throw BuildError(
          error_codes::messages::DUPLICATE_COMMAND_HANDLER,
          error_codes::codes::DUPLICATE_COMMAND_HANDLER,
          ClientError::DetailsMap{
              {error_codes::keys::ROUTER_NAME, name_},
              {error_codes::keys::DOMAIN, router.domain()},
              {error_codes::keys::TYPE_URL, std::string(typeid(H).name())},
          });
    }
    ++handler_count_;
    return *this;
  }

  /// Register a saga router. Spec HIGH-4.4.
  template <typename H>
  UnifiedRouter& add_saga(const SagaRouter<H>& router) {
    set_or_assert_kind(RouterKind::Saga);
    if (!router.target_domain().empty()) {
      output_domains_.push_back(router.target_domain());
      // Sagas may be sync or async; without per-router metadata
      // here we conservatively treat them as sync (audit #74 sync
      // probe wiring) — async ones will be caught at runtime by
      // their own configuration.
      sync_output_domains_.push_back(router.target_domain());
    }
    ++handler_count_;
    return *this;
  }

  /// Register a process manager router. Spec HIGH-4.4.
  template <typename S>
  UnifiedRouter& add_process_manager(
      const ProcessManagerRouter<S>& /*router*/) {
    set_or_assert_kind(RouterKind::ProcessManager);
    has_async_outputs_ = true;
    ++handler_count_;
    return *this;
  }

  /// Register a projector router. Spec HIGH-4.4.
  UnifiedRouter& add_projector(const ProjectorRouter& /*router*/) {
    set_or_assert_kind(RouterKind::Projector);
    ++handler_count_;
    return *this;
  }

  /// Register an upcaster router. Spec HIGH-4.4 — adds the 5th
  /// variant that C++ previously couldn't recognize.
  UnifiedRouter& add_upcaster(const UpcasterRouter& /*router*/) {
    set_or_assert_kind(RouterKind::Upcaster);
    ++handler_count_;
    return *this;
  }

  /**
   * Build + validate. Raises :class:`BuildError` with
   * ``ROUTER_NO_HANDLERS`` when no router has been registered.
   */
  Built build() const {
    if (!kind_.has_value() || handler_count_ == 0) {
      throw BuildError(error_codes::messages::ROUTER_NO_HANDLERS,
                       error_codes::codes::ROUTER_NO_HANDLERS,
                       ClientError::DetailsMap{
                           {error_codes::keys::ROUTER_NAME, name_},
                       });
    }

    Built b;
    b.router_name = name_;
    b.kind = *kind_;
    b.handler_count = handler_count_;
    // Dedupe output domains while preserving insertion order so
    // tests can assert on a stable shape.
    std::set<std::string> seen_outs;
    for (const auto& d : output_domains_) {
      if (seen_outs.insert(d).second) b.output_domains.push_back(d);
    }
    std::set<std::string> seen_syncs;
    for (const auto& d : sync_output_domains_) {
      if (seen_syncs.insert(d).second) b.sync_output_domains.push_back(d);
    }
    b.has_async_outputs = has_async_outputs_;
    return b;
  }

 private:
  void set_or_assert_kind(RouterKind k) {
    if (!kind_.has_value()) {
      kind_ = k;
      return;
    }
    if (*kind_ != k) {
      // Audit #72 — MIXED_HANDLER_KINDS.
      throw BuildError(
          error_codes::messages::MIXED_HANDLER_KINDS,
          error_codes::codes::MIXED_HANDLER_KINDS,
          ClientError::DetailsMap{
              {error_codes::keys::ROUTER_NAME, name_},
              {error_codes::keys::HANDLER_KIND, router_kind_name(*kind_)},
              {error_codes::keys::OTHER_KIND, router_kind_name(k)},
          });
    }
  }

  std::string name_;
  std::optional<RouterKind> kind_;
  int handler_count_ = 0;
  std::vector<std::string> output_domains_;
  std::vector<std::string> sync_output_domains_;
  bool has_async_outputs_ = false;
  std::set<std::pair<std::string, std::string>> command_keys_;
};

}  // namespace angzarr
