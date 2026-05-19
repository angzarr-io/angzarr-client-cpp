#include "angzarr/server.hpp"

// Spec MED-5.10 — ``configure_logging`` is meant to be a *uniform*
// entry-point name across languages; the concrete backend choice can
// differ. C++ already ships ``logging.hpp`` with a pluggable
// :class:`Logger`. When the absl build provides
// ``<absl/log/initialize.h>`` we delegate to ``absl::InitializeLog``
// (idempotent under ``std::call_once``); otherwise the helper is a
// no-op and callers continue to drive the default logger directly.
#if __has_include(<absl/log/initialize.h>)
#include <absl/log/initialize.h>
#define ANGZARR_HAS_ABSL_INITIALIZE_LOG 1
#endif

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include "angzarr/logging.hpp"

namespace angzarr::server {

namespace {
const char* env_or_null(const char* name) { return std::getenv(name); }

std::string env_or_default(const char* name, const char* fallback) {
  const char* v = env_or_null(name);
  return (v != nullptr && *v != '\0') ? std::string(v) : std::string(fallback);
}

std::optional<std::string> get_qualifier() {
  static const char* kCandidates[] = {"DOMAIN", "SAGA_NAME", "PROJECTOR_NAME",
                                      "PROCESS_MANAGER_NAME", "UPCASTER_NAME"};
  for (const char* name : kCandidates) {
    const char* v = env_or_null(name);
    if (v != nullptr && *v != '\0') return std::string(v);
  }
  return std::nullopt;
}
}  // namespace

std::string resolve_bind_address(uint16_t default_port) {
  const char* override_addr = env_or_null(ENV_BIND_ADDRESS);
  if (override_addr != nullptr && *override_addr != '\0') return override_addr;
  const char* port_env = env_or_null("PORT");
  std::string port_str = (port_env != nullptr && *port_env != '\0')
                             ? std::string(port_env)
                             : std::to_string(default_port);
  return std::string(DEFAULT_BIND_HOST) + ":" + port_str;
}

ServerConfig config_from_env(uint16_t default_port) {
  ServerConfig cfg;
  cfg.port = default_port;

  const char* uds_base = env_or_null("UDS_BASE_PATH");
  if (uds_base != nullptr && *uds_base != '\0') {
    auto qualifier = get_qualifier();
    std::string service_name = env_or_default("SERVICE_NAME", "business");
    std::string socket_file;
    if (qualifier.has_value()) {
      socket_file = service_name + "-" + *qualifier + ".sock";
    } else {
      socket_file = service_name + ".sock";
    }
    cfg.uds_path = std::string(uds_base) + "/" + socket_file;
    return cfg;
  }
  // TCP path: PORT > GRPC_PORT > default_port (matches C#).
  const char* port_env = env_or_null("PORT");
  if (port_env == nullptr || *port_env == '\0')
    port_env = env_or_null("GRPC_PORT");
  if (port_env != nullptr && *port_env != '\0') {
    try {
      int p = std::stoi(port_env);
      if (p >= 0 && p <= 65535) cfg.port = static_cast<uint16_t>(p);
    } catch (...) {
      // fall through to default_port silently — matches Py/C# tolerance.
    }
  }
  cfg.bind_address = resolve_bind_address(cfg.port);
  return cfg;
}

void cleanup_socket(const std::string& socket_path) {
  if (socket_path.empty()) return;
  std::error_code ec;
  if (std::filesystem::exists(socket_path, ec)) {
    std::filesystem::remove(socket_path, ec);
    // best-effort — matches Py/Go/Ja parity (no throw on missing or
    // permission denied; operators see the eventual bind failure if
    // it persists).
  }
}

std::pair<std::string, std::string> get_transport_config(
    uint16_t default_port) {
  std::string transport = env_or_default("TRANSPORT_TYPE", "tcp");
  for (auto& c : transport)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

  if (transport == "uds") {
    std::string base = env_or_default("UDS_BASE_PATH", "/tmp/angzarr");
    std::string service_name = env_or_default("SERVICE_NAME", "business");
    auto qualifier = get_qualifier();
    std::string socket_path = base + "/" + service_name;
    if (qualifier.has_value()) socket_path += "-" + *qualifier;
    socket_path += ".sock";

    // Side effects: ensure parent dir + clear stale socket (matches
    // Py/Go/Ja semantics). Spec MED-5.12 et al.
    std::error_code ec;
    std::filesystem::create_directories(
        std::filesystem::path(socket_path).parent_path(), ec);
    cleanup_socket(socket_path);

    return {"uds", "unix:" + socket_path};
  }
  return {"tcp", resolve_bind_address(default_port)};
}

namespace {
std::once_flag absl_init_once;
}  // namespace

void configure_logging() {
  // Idempotent — absl::InitializeLog must only be called once per
  // process. Wrap behind ``std::call_once`` so multiple ``main()``
  // entry paths (tests, embedded binaries) can call ``configure_logging()``
  // without crashing on duplicate init.
#ifdef ANGZARR_HAS_ABSL_INITIALIZE_LOG
  std::call_once(absl_init_once, []() { absl::InitializeLog(); });
#else
  // No-op fallback when absl initialization isn't available in the
  // current toolchain (e.g. the mull mutation-test container ships
  // an older absl). The pluggable ``logging.hpp`` backend still
  // works — callers can install their own Logger.
  std::call_once(absl_init_once, []() {});
#endif
}

void run_supervisor(
    const std::vector<std::shared_ptr<readiness::Probe>>& probes,
    const HealthSetter& health_setter,
    const std::vector<std::string>& service_names,
    std::chrono::seconds interval, std::chrono::milliseconds timeout,
    const std::function<bool()>& stop_requested) {
  bool previously_serving = false;
  while (!stop_requested()) {
    bool all_ok = readiness::supervisor_tick(probes, timeout);
    if (all_ok != previously_serving) {
      for (const auto& name : service_names) {
        health_setter(name, all_ok);
      }
      previously_serving = all_ok;
    }
    // Sleep in short slices so stop_requested() is checked
    // frequently — matches Py asyncio.sleep(interval) cancellation
    // semantics.
    auto deadline = std::chrono::steady_clock::now() + interval;
    while (std::chrono::steady_clock::now() < deadline) {
      if (stop_requested()) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void publish_shutdown_status(const HealthSetter& health_setter,
                             const std::vector<std::string>& service_names,
                             const std::string& service_label,
                             const std::string& kind_label) {
  // Audit #83: every registered name flips to NOT_SERVING before the
  // gRPC ``Shutdown()`` call so probers see the transition.
  for (const auto& name : service_names) {
    health_setter(name, false);
  }
  log_server_shutdown(service_label, kind_label);
}

}  // namespace angzarr::server
