#pragma once

// Transport-mode resolver for command handler coordinator endpoints.
//
// Spec HIGH-6.1: C++ has historically lacked this module. Cross-language
// callers can set ``ANGZARR_MODE=standalone`` for local UDS or
// ``ANGZARR_MODE=distributed`` for K8s DNS resolution — the same env
// vars produce the same endpoints in Py/Rs/Go/Ja/C#. C++ now joins.
//
// Mirrors :file:`angzarr_client/client.py` (Python),
// :file:`src/transport.rs` (Rust), :file:`client.go` (Go),
// :file:`Transport.java` (Java), and :file:`Transport.cs` (C#) so the
// surface and behavior are byte-equivalent.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace angzarr::transport {

/// Default UDS base directory for ``STANDALONE`` mode.
inline constexpr const char* DEFAULT_UDS_BASE = "/tmp/angzarr";

/// Default Kubernetes namespace for ``DISTRIBUTED`` mode.
inline constexpr const char* DEFAULT_NAMESPACE = "angzarr";

/// Default command handler coordinator port for ``DISTRIBUTED`` mode.
inline constexpr uint16_t DEFAULT_CH_PORT = 1310;

/// Env-var names — same exact strings as Py/Rs/Go/Ja/C# so an operator
/// can set them once for the whole deployment regardless of which
/// language each pod runs.
inline constexpr const char* ENV_MODE = "ANGZARR_MODE";
inline constexpr const char* ENV_UDS_BASE = "ANGZARR_UDS_BASE";
inline constexpr const char* ENV_NAMESPACE = "ANGZARR_NAMESPACE";
inline constexpr const char* ENV_CH_PORT = "ANGZARR_CH_PORT";

/// Transport mode for gRPC connections.
enum class TransportMode {
  /// Unix Domain Sockets for local-process communication.
  Standalone,
  /// TCP via Kubernetes DNS for cluster communication.
  Distributed,
};

/// Default transport mode when neither arg nor env var is set.
inline constexpr TransportMode DEFAULT_TRANSPORT_MODE =
    TransportMode::Distributed;

/// Parse a mode-name string. Audit #40: loud-fail on operator typos.
///
/// Throws ``InvalidArgumentError`` with code ``INVALID_TRANSPORT_MODE``
/// when ``s`` is not exactly ``"standalone"`` or ``"distributed"``.
TransportMode transport_mode_from_string(std::string_view s);

/// Resolve mode from :data:`ENV_MODE`.
///
/// Unset → :data:`DEFAULT_TRANSPORT_MODE`. Recognized value → that
/// variant. Anything else → throws (audit #40).
TransportMode transport_mode_from_env();

/// Parse a port number string, throwing on non-numeric / out-of-range.
///
/// Audit #40: loud-fail. ``"1310 "`` (trailing-space typo) raises
/// ``InvalidArgumentError`` with code ``INVALID_PORT``.
uint16_t parse_port(std::string_view s);

/// Detect a UDS endpoint and return the socket path, or ``nullopt`` for
/// TCP. Audit #39: lenient prefix detection.
///
/// Recognized UDS forms:
///   - ``/abs/path``               — absolute path, no scheme
///   - ``./rel/path``              — relative path, no scheme
///   - ``unix:relative/path``      — gRPC URI, relative
///   - ``unix:/abs/path``          — gRPC URI, absolute (single slash)
///   - ``unix:///abs/path``        — gRPC URI, absolute, empty authority
std::optional<std::string> detect_uds_path(std::string_view endpoint);

/// Resolve a domain name to a command handler coordinator endpoint.
///
/// - :enumerator:`TransportMode::Standalone` → ``{uds_base}/ch-{domain}.sock``
/// - :enumerator:`TransportMode::Distributed` → ``ch-{domain}.{ns}.svc:{port}``
///
/// Resolution precedence (matches Py/Rs/Go/Ja/C#): **env var > explicit
/// arg > default**.
///
/// @param domain   Domain name (required).
/// @param mode     Transport mode; ``nullopt`` auto-detects from
/// :data:`ENV_MODE`.
/// @param uds_base Override UDS base path; ``nullopt`` falls through
/// env+default.
/// @param ns       Override K8s namespace; ``nullopt`` falls through
/// env+default.
/// @param port     Override TCP port; ``nullopt`` falls through env+default.
std::string resolve_ch_endpoint(
    std::string_view domain, std::optional<TransportMode> mode = std::nullopt,
    std::optional<std::string> uds_base = std::nullopt,
    std::optional<std::string> ns = std::nullopt,
    std::optional<uint16_t> port = std::nullopt);

}  // namespace angzarr::transport
