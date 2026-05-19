#include "angzarr/transport.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <string>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

namespace angzarr::transport {

namespace {
const char* env_or_null(const char* name) { return std::getenv(name); }
}  // namespace

TransportMode transport_mode_from_string(std::string_view s) {
  if (s == "standalone") return TransportMode::Standalone;
  if (s == "distributed") return TransportMode::Distributed;
  throw InvalidArgumentError(error_codes::messages::INVALID_TRANSPORT_MODE,
                             error_codes::codes::INVALID_TRANSPORT_MODE,
                             ClientError::DetailsMap{
                                 {error_codes::keys::INPUT, std::string(s)},
                                 {error_codes::keys::ENV_VAR, ENV_MODE},
                             });
}

TransportMode transport_mode_from_env() {
  const char* v = env_or_null(ENV_MODE);
  if (v == nullptr || *v == '\0') return DEFAULT_TRANSPORT_MODE;
  return transport_mode_from_string(v);
}

uint16_t parse_port(std::string_view s) {
  // Audit #40 — strict parse mirroring Python ``int("1310 ")`` (raises)
  // and Rust ``s.parse::<u16>()`` (rejects whitespace).
  auto bad = [&]() {
    throw InvalidArgumentError(error_codes::messages::INVALID_PORT,
                               error_codes::codes::INVALID_PORT,
                               ClientError::DetailsMap{
                                   {error_codes::keys::INPUT, std::string(s)},
                                   {error_codes::keys::ENV_VAR, ENV_CH_PORT},
                               });
  };
  if (s.empty()) bad();
  for (char c : s) {
    if (!std::isdigit(static_cast<unsigned char>(c))) bad();
  }
  uint32_t value = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec != std::errc() || ptr != s.data() + s.size() || value > 65535) bad();
  return static_cast<uint16_t>(value);
}

std::optional<std::string> detect_uds_path(std::string_view endpoint) {
  if (endpoint.empty()) return std::nullopt;
  constexpr std::string_view triple = "unix:///";
  constexpr std::string_view dbl = "unix://";
  constexpr std::string_view single = "unix:";
  if (endpoint.substr(0, triple.size()) == triple) {
    return std::string(endpoint.substr(dbl.size()));
  }
  if (endpoint.substr(0, dbl.size()) == dbl) {
    return std::string(endpoint.substr(dbl.size()));
  }
  if (endpoint.substr(0, single.size()) == single) {
    return std::string(endpoint.substr(single.size()));
  }
  if (endpoint[0] == '/' ||
      (endpoint.size() >= 2 && endpoint[0] == '.' && endpoint[1] == '/')) {
    return std::string(endpoint);
  }
  return std::nullopt;
}

std::string resolve_ch_endpoint(std::string_view domain,
                                std::optional<TransportMode> mode,
                                std::optional<std::string> uds_base,
                                std::optional<std::string> ns,
                                std::optional<uint16_t> port) {
  TransportMode resolved = mode.has_value() ? *mode : transport_mode_from_env();

  if (resolved == TransportMode::Standalone) {
    // env > explicit > default.
    const char* env_base = env_or_null(ENV_UDS_BASE);
    std::string base =
        (env_base && *env_base)
            ? env_base
            : (uds_base.has_value() ? *uds_base : DEFAULT_UDS_BASE);
    return base + "/ch-" + std::string(domain) + ".sock";
  }
  // Distributed.
  const char* env_ns = env_or_null(ENV_NAMESPACE);
  std::string nspc =
      (env_ns && *env_ns) ? env_ns : (ns.has_value() ? *ns : DEFAULT_NAMESPACE);

  const char* env_port = env_or_null(ENV_CH_PORT);
  uint16_t p;
  if (env_port && *env_port) {
    p = parse_port(env_port);
  } else if (port.has_value()) {
    p = *port;
  } else {
    p = DEFAULT_CH_PORT;
  }
  return "ch-" + std::string(domain) + "." + nspc + ".svc:" + std::to_string(p);
}

}  // namespace angzarr::transport
