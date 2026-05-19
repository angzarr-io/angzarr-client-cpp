// Readiness supervisor implementation. See readiness.hpp for the
// architectural sketch. Audit findings #68, #74, #82.

#include "angzarr/readiness.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include "angzarr/logging.hpp"

namespace angzarr::readiness {

namespace {

std::optional<long> parse_seconds(const char* raw) {
  if (raw == nullptr || raw[0] == '\0') return std::nullopt;
  char* end = nullptr;
  long v = std::strtol(raw, &end, 10);
  if (end == raw || *end != '\0') return std::nullopt;
  if (v < 0) return std::nullopt;
  return v;
}

// Connect to ``host:port`` using a brief timeout. Returns ``true`` on
// successful TCP handshake. Mirrors Python's ``asyncio.open_connection``
// and Rust's ``TcpStream::connect`` with the supervisor-controlled
// timeout enforced at the supervisor layer.
bool tcp_connect_check(const std::string& addr) {
  auto sep = addr.rfind(':');
  if (sep == std::string::npos) return false;
  std::string host = addr.substr(0, sep);
  std::string port_s = addr.substr(sep + 1);

  if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
    host = host.substr(1, host.size() - 2);
  }
  if (host.empty()) host = "localhost";

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* res = nullptr;
  int err = ::getaddrinfo(host.c_str(), port_s.c_str(), &hints, &res);
  if (err != 0 || res == nullptr) {
    if (res) ::freeaddrinfo(res);
    return false;
  }
  bool ok = false;
  for (addrinfo* ai = res; ai != nullptr; ai = ai->ai_next) {
    int fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) continue;
    // Best-effort short timeout via SO_RCVTIMEO/SO_SNDTIMEO so a
    // dead host doesn't hang the supervisor tick. The supervisor
    // wraps this in its own wait_for as a hard ceiling.
    timeval tv{};
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
      ok = true;
    }
    ::close(fd);
    if (ok) break;
  }
  ::freeaddrinfo(res);
  return ok;
}

bool uds_connect_check(const std::string& path) {
  if (path.size() >= sizeof(sockaddr_un::sun_path)) return false;
  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return false;
  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
  bool ok =
      ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
  ::close(fd);
  return ok;
}

bool endpoint_check(const Endpoint& ep) {
  return ep.is_tcp() ? tcp_connect_check(ep.address)
                     : uds_connect_check(ep.address);
}

}  // namespace

std::pair<std::chrono::seconds, std::chrono::seconds> probe_config_from_env() {
  auto interval = parse_seconds(std::getenv(ENV_INTERVAL))
                      .value_or(DEFAULT_PROBE_INTERVAL.count());
  auto timeout = parse_seconds(std::getenv(ENV_TIMEOUT))
                     .value_or(DEFAULT_PROBE_TIMEOUT.count());
  return {std::chrono::seconds(interval), std::chrono::seconds(timeout)};
}

Endpoint parse_endpoint(const std::string& raw) {
  static const std::string kUnixPrefix = "unix:";
  if (raw.rfind(kUnixPrefix, 0) == 0) {
    return Endpoint::uds(raw.substr(kUnixPrefix.size()));
  }
  if (!raw.empty() && raw.front() == '/') {
    return Endpoint::uds(raw);
  }
  return Endpoint::tcp(raw);
}

bool OutputDomainProbe::check() { return endpoint_check(endpoint_); }

bool BusProbe::check() { return endpoint_check(endpoint_); }

std::shared_ptr<BusProbe> BusProbe::from_env() {
  const char* raw = std::getenv(ENV_BUS_ENDPOINT);
  if (raw == nullptr || raw[0] == '\0') return nullptr;
  return std::make_shared<BusProbe>(parse_endpoint(raw));
}

bool supervisor_tick(const std::vector<std::shared_ptr<Probe>>& probes,
                     std::chrono::milliseconds timeout) {
  bool all_ok = true;
  for (const auto& probe : probes) {
    // Run on a detachable thread + promise so a slow probe doesn't
    // block the supervisor past ``timeout``. The future stores the
    // result if it completes; otherwise the supervisor logs the
    // timeout and the thread is left to drain (probe authors
    // contract to keep ``check()`` bounded).
    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();
    auto exception_box = std::make_shared<std::exception_ptr>();
    std::thread([probe, promise, exception_box]() {
      try {
        bool result = probe->check();
        promise->set_value(result);
      } catch (...) {
        *exception_box = std::current_exception();
        try {
          promise->set_value(false);
        } catch (const std::future_error&) {
          // promise already satisfied — ignore.
        }
      }
    }).detach();

    bool ok = false;
    if (future.wait_for(timeout) == std::future_status::ready) {
      if (*exception_box) {
        std::string err_msg;
        try {
          std::rethrow_exception(*exception_box);
        } catch (const std::exception& e) {
          err_msg = e.what();
        } catch (...) {
          err_msg = "<non-std::exception panic>";
        }
        log_warn("readiness_probe_panicked",
                 {{"probe", probe->name()}, {"error", err_msg}});
        ok = false;
      } else {
        ok = future.get();
        if (!ok) {
          log_warn("readiness_probe_failed", {{"probe", probe->name()}});
        }
      }
    } else {
      log_warn("readiness_probe_timed_out", {{"probe", probe->name()}});
      ok = false;
    }
    if (!ok) all_ok = false;
  }
  return all_ok;
}

}  // namespace angzarr::readiness
