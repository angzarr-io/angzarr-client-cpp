#pragma once

// Structured logging for the angzarr C++ client.
//
// Audit findings #87..#91 — pre-port, C++ had no integrated logging
// story; structured fields, panic level, and lifecycle event shape
// were all left "TBD". This module picks ``absl::log`` as the default
// backend (already a transitive dep through gRPC's link-set) and wraps
// it behind a pluggable :class:`Logger` interface. Callers who want a
// different sink (test capture, spdlog, JSON-to-stdout for k8s log
// shipping, ...) install their own backend via ``set_default_logger``.
//
// Parity targets:
//   - Python: ``logging`` + ``structlog`` — ``logger.info(event,
//     key=value)`` shape.
//   - Rust:   ``tracing::warn!(field = value, "event")`` — same field
//     set, same event names.
//
// The wire-level contract pinned by the tests:
//   - ``LogLevel`` names are lowercase strings (``"info"``, ``"warn"``,
//     ...) so log routers can filter on the level field consistently
//     across pods of different languages.
//   - ``log_server_started`` / ``log_server_shutdown`` emit the exact
//     field set listed in Python ``server.py`` (audit #89).

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace angzarr {

/// Severity levels — match Python's ``logging`` module and Rust's
/// ``tracing`` ordering. Reported as lowercase strings on the wire so
/// log routers can filter without per-language casing translation.
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

/// Lower-case string name for a level, suitable for emission as a
/// structured field or in absl::log routing. Audit #87..#90.
inline const char* level_name(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG:
      return "debug";
    case LogLevel::INFO:
      return "info";
    case LogLevel::WARN:
      return "warn";
    case LogLevel::ERROR:
      return "error";
  }
  return "info";
}

/// Structured field map — string-to-string. Matches Python structlog's
/// ``**kwargs`` and Rust tracing's ``key = value`` pairs.
using LogFields = std::map<std::string, std::string>;

/// Pluggable backend. Default implementation routes through
/// ``absl::log``; tests install ``CaptureLogger`` to assert event
/// shape; production callers can install a JSON-to-stdout sink for
/// k8s log shipping without touching call sites.
class Logger {
 public:
  virtual ~Logger() = default;
  virtual void log(LogLevel level, const std::string& event,
                   const LogFields& fields) = 0;
};

/// Test capture backend — records every log call in memory so tests
/// can assert the exact event name + field set produced. Mirrors
/// Python's ``caplog`` fixture and Rust's ``tracing-test`` capture
/// pattern.
class CaptureLogger : public Logger {
 public:
  struct Record {
    LogLevel level;
    std::string event;
    LogFields fields;
  };

  void log(LogLevel level, const std::string& event,
           const LogFields& fields) override {
    std::lock_guard<std::mutex> lock(mu_);
    records_.push_back({level, event, fields});
  }

  /// Snapshot of records captured so far. Returned by value so callers
  /// can iterate without holding the internal mutex.
  std::vector<Record> records() const {
    std::lock_guard<std::mutex> lock(mu_);
    return records_;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mu_);
    records_.clear();
  }

 private:
  mutable std::mutex mu_;
  std::vector<Record> records_;
};

namespace detail {

/// Default backend — routes through absl::log. Defined out-of-line in
/// ``src/logging.cpp`` so absl headers don't leak into every include
/// site (absl/log/log.h pulls a lot in).
class AbslLogger : public Logger {
 public:
  void log(LogLevel level, const std::string& event,
           const LogFields& fields) override;
};

/// Process-wide installed logger. Accessor + mutator below are the
/// public surface; the storage itself is detail.
std::shared_ptr<Logger>& default_logger_storage();
std::mutex& default_logger_mutex();

}  // namespace detail

/// Get the process-wide default logger as a shared pointer. Always
/// non-null — falls back to an :class:`AbslLogger` if no caller has
/// installed an alternative. Audit #91: this guarantees structured
/// fields are never silently dropped by a missing logger.
inline std::shared_ptr<Logger> default_logger_shared() {
  std::lock_guard<std::mutex> lock(detail::default_logger_mutex());
  auto& storage = detail::default_logger_storage();
  if (!storage) {
    storage = std::make_shared<detail::AbslLogger>();
  }
  return storage;
}

/// Raw pointer accessor — equivalent to ``default_logger_shared().get()``
/// for call sites that don't need to extend the lifetime.
inline Logger* default_logger() { return default_logger_shared().get(); }

/// Install a new default logger. Returns the previously-installed one
/// so callers can restore it (e.g. test teardown).
inline std::shared_ptr<Logger> set_default_logger(
    std::shared_ptr<Logger> logger) {
  std::lock_guard<std::mutex> lock(detail::default_logger_mutex());
  auto& storage = detail::default_logger_storage();
  auto previous = std::move(storage);
  storage = std::move(logger);
  return previous;
}

/// One-line convenience helpers. Mirror Python's ``logger.info(event,
/// **fields)`` and Rust's ``tracing::info!(field=value, "event")``.
inline void log_debug(const std::string& event, LogFields fields = {}) {
  default_logger_shared()->log(LogLevel::DEBUG, event, fields);
}
inline void log_info(const std::string& event, LogFields fields = {}) {
  default_logger_shared()->log(LogLevel::INFO, event, fields);
}
inline void log_warn(const std::string& event, LogFields fields = {}) {
  default_logger_shared()->log(LogLevel::WARN, event, fields);
}
inline void log_error(const std::string& event, LogFields fields = {}) {
  default_logger_shared()->log(LogLevel::ERROR, event, fields);
}

/// Audit #89: ``server_started`` / ``server_shutdown`` log shape
/// parity. Same event name + field set as Python's ``server.py`` and
/// Rust's ``server.rs``, so operators querying logs by ``service`` /
/// ``name`` / ``transport`` / ``address`` see equivalent records from
/// pods of either language.
inline void log_server_started(const std::string& service,
                               const std::string& name,
                               const std::string& transport,
                               const std::string& address) {
  log_info("server_started", {{"service", service},
                              {"name", name},
                              {"transport", transport},
                              {"address", address}});
}

inline void log_server_shutdown(const std::string& service,
                                const std::string& name) {
  log_info("server_shutdown", {{"service", service}, {"name", name}});
}

}  // namespace angzarr
