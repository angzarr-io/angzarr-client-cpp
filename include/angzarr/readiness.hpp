#pragma once

// Readiness probes and health-status supervisor for runner servers.
//
// Cross-language port of Python ``angzarr_client/readiness.py`` and
// Rust ``src/readiness.rs``. Audit findings #68, #74, #82.
//
// A runner exposes its readiness through ``grpc.health.v1.Health``.
// While any probe is failing, the per-kind service name reports
// ``NOT_SERVING``; once every probe is green, it flips to ``SERVING``.
//
// Threading model: unlike Python (asyncio coroutines) or Rust (tokio
// tasks), the C++ port runs probes synchronously on a worker thread.
// Per-probe timeouts are enforced by running each ``check()`` on a
// short-lived ``std::async`` future and ``wait_for`` -ing it; a
// timeout reports as a probe failure but the future is left to drain
// (the supervisor owns no leak — the future detaches when scope
// exits). Probe authors must keep ``check()`` quick (<= timeout).
//
// Aggregation is binary — ``all up`` is ``SERVING``, anything else is
// ``NOT_SERVING``. Audit #82: each cause (panic / timeout / probe-
// returned-false) emits its own WARN log; there is no aggregate
// "failed" log on top.

#include <chrono>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace angzarr::readiness {

/// Default cadence for re-evaluating output-domain probes.
inline constexpr std::chrono::seconds DEFAULT_PROBE_INTERVAL{30};

/// Default per-probe timeout.
inline constexpr std::chrono::seconds DEFAULT_PROBE_TIMEOUT{2};

/// Env-var names — same exact strings as Python and Rust so an
/// operator can set them once for the whole deployment regardless of
/// which language each pod runs.
inline constexpr const char* ENV_INTERVAL = "ANGZARR_READINESS_PROBE_INTERVAL";
inline constexpr const char* ENV_TIMEOUT = "ANGZARR_READINESS_PROBE_TIMEOUT";

/// Audit #74: optional async-bus endpoint (Kafka / RabbitMQ / SQS /
/// SNS / NATS / etc.). When set, a single :class:`BusProbe` covers
/// reachability of the async path for every async-only saga / PM
/// target. When unset, no bus probe is added.
inline constexpr const char* ENV_BUS_ENDPOINT = "ANGZARR_BUS_ENDPOINT";

/// Read the supervisor cadence + per-probe timeout from env, falling
/// back to :data:`DEFAULT_PROBE_INTERVAL` / :data:`DEFAULT_PROBE_TIMEOUT`.
/// Bad values silently fall back, matching Rust's ``.parse::<u64>().ok()``
/// and Python's ``try/except ValueError`` fall-through.
std::pair<std::chrono::seconds, std::chrono::seconds> probe_config_from_env();

/// Endpoint classification — TCP host:port or UDS filesystem path.
/// Public so callers can test the parser surface without going through
/// a full Probe.
struct Endpoint {
  enum class Kind { TCP, UDS };
  Kind kind;
  std::string address;  ///< host:port for TCP; filesystem path for UDS.

  bool is_tcp() const { return kind == Kind::TCP; }
  bool is_uds() const { return kind == Kind::UDS; }

  static Endpoint tcp(std::string addr) { return {Kind::TCP, std::move(addr)}; }
  static Endpoint uds(std::string path) { return {Kind::UDS, std::move(path)}; }
};

/// Classify a resolved endpoint string into TCP or UDS. ``unix:``
/// prefix or leading ``/`` selects UDS; otherwise TCP. Mirrors the
/// Python ``_parse_endpoint`` and Rust ``Endpoint::from_raw`` helpers.
Endpoint parse_endpoint(const std::string& raw);

/// Single readiness probe — evaluated once per supervisor tick.
class Probe {
 public:
  virtual ~Probe() = default;
  /// Stable identifier for log lines.
  virtual const std::string& name() const = 0;
  /// ``true`` when the underlying dependency is currently healthy.
  /// May throw — the supervisor catches and treats as failure.
  virtual bool check() = 0;
};

/// Side of :class:`TransportProbe` used by the runner to mark
/// "bound and serving". Once flipped, never reverts (matches Rust's
/// ``AtomicBool::store(true, SeqCst)`` and Python's ``_bound = True``).
class TransportSignal {
 public:
  void mark_bound() {
    std::lock_guard<std::mutex> lock(mu_);
    bound_ = true;
  }
  bool is_bound() const {
    std::lock_guard<std::mutex> lock(mu_);
    return bound_;
  }

 private:
  mutable std::mutex mu_;
  bool bound_ = false;
};

/// One-shot transport probe — flipped ``true`` once the listener has
/// bound and the server is accepting traffic. From that point its
/// result never changes.
class TransportProbe : public Probe {
 public:
  explicit TransportProbe(std::shared_ptr<TransportSignal> signal)
      : signal_(std::move(signal)) {
    static const std::string kName = "transport";
    name_ = kName;
  }

  /// Factory returning the probe plus the signal the runner uses to
  /// flip it. Mirrors Python's ``TransportProbe.new()`` and Rust's
  /// ``TransportProbe::new()``.
  static std::pair<std::shared_ptr<TransportProbe>,
                   std::shared_ptr<TransportSignal>>
  create() {
    auto signal = std::make_shared<TransportSignal>();
    auto probe = std::make_shared<TransportProbe>(signal);
    return {probe, signal};
  }

  const std::string& name() const override { return name_; }
  bool check() override { return signal_->is_bound(); }

 private:
  std::shared_ptr<TransportSignal> signal_;
  std::string name_;
};

/// Per-output-domain coordinator probe — attempts to open a connection
/// to the downstream domain's command-handler coordinator endpoint.
/// Audit #74: built only for **sync** output domains; async-only
/// targets ride the bus (:class:`BusProbe`).
class OutputDomainProbe : public Probe {
 public:
  OutputDomainProbe(std::string domain, Endpoint endpoint)
      : domain_(std::move(domain)), endpoint_(std::move(endpoint)) {}

  const std::string& name() const override { return domain_; }
  bool check() override;

 private:
  std::string domain_;
  Endpoint endpoint_;
};

/// Async-bus reachability probe (audit #74) — covers the path that
/// async-only saga / PM targets ride. Endpoint operator-supplied via
/// :data:`ENV_BUS_ENDPOINT`.
class BusProbe : public Probe {
 public:
  explicit BusProbe(Endpoint endpoint) : endpoint_(std::move(endpoint)) {
    static const std::string kName = "bus";
    name_ = kName;
  }

  /// Build from :data:`ENV_BUS_ENDPOINT`. Returns ``nullptr`` if the
  /// env var is unset or blank — mirrors Python's ``None`` and
  /// Rust's ``Option::None``.
  static std::shared_ptr<BusProbe> from_env();

  const std::string& name() const override { return name_; }
  bool check() override;

 private:
  Endpoint endpoint_;
  std::string name_;
};

/// One iteration of the supervisor loop: poll every probe with the
/// configured timeout, return ``true`` iff all probes report healthy.
///
/// Audit #82: probes that throw don't kill the supervisor; each
/// failure cause emits its own WARN log via the installed
/// :class:`Logger` backend. Exported for unit-testing the tick logic
/// in isolation (matches Rust's ``supervisor_tick`` and Python's
/// per-iteration loop body).
bool supervisor_tick(const std::vector<std::shared_ptr<Probe>>& probes,
                     std::chrono::milliseconds timeout);

}  // namespace angzarr::readiness
