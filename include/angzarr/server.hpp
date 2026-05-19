#pragma once

// Server module — wiring for the angzarr C++ runtime.
//
// Spec HIGH-5.1: C++ historically lacked a server module entirely; the
// per-kind ``run_*_server`` runners (sibling of Py/Rs/Go/Ja/C#) didn't
// exist, ``ServerConfig`` / ``resolve_bind_address`` / ``cleanup_socket``
// were missing, and there was no place to wire the two-phase shutdown
// described in audit #83. This module fills that gap.
//
// Audit #77: bind host defaults to ``[::]`` (IPv6 wildcard with IPV6_V6ONLY=0
// dual-stack on Linux), overridable via ``ANGZARR_BIND_ADDRESS``.
//
// Audit #68: server begins life at NOT_SERVING and is only flipped to
// SERVING once every readiness probe reports healthy. Callers wire
// :class:`readiness::TransportProbe` plus per-kind output-domain probes
// and call :func:`run_supervisor` to drive the loop.

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "angzarr/component_router.hpp"
#include "angzarr/readiness.hpp"
#include "angzarr/router.hpp"
#include "angzarr/upcaster.hpp"

namespace angzarr::server {

/// Audit #77 — overridable from operator-supplied
/// ``ANGZARR_BIND_ADDRESS=[::1]:50052`` (forces localhost-only) or
/// ``0.0.0.0:50052`` (forces IPv4-only).
inline constexpr const char* ENV_BIND_ADDRESS = "ANGZARR_BIND_ADDRESS";

/// Audit #77 — IPv6 wildcard, dual-stack via IPV6_V6ONLY=0 on Linux.
inline constexpr const char* DEFAULT_BIND_HOST = "[::]";

/// Default coordinator port matching Py/Rs/Ja/C# runners.
inline constexpr uint16_t DEFAULT_PORT = 50052;

/// Per-kind health-check service names. Aligns with audit #74 / #68
/// where each kind exposes both ``""`` (gRPC overall) and a per-kind
/// name to enable rolling-deploy targeting.
inline constexpr const char* HEALTH_NAME_COMMAND_HANDLER =
    "angzarr_client.proto.angzarr.CommandHandlerCoordinatorService";
inline constexpr const char* HEALTH_NAME_SAGA =
    "angzarr_client.proto.angzarr.SagaCoordinatorService";
inline constexpr const char* HEALTH_NAME_PROCESS_MANAGER =
    "angzarr_client.proto.angzarr.ProcessManagerCoordinatorService";
inline constexpr const char* HEALTH_NAME_PROJECTOR =
    "angzarr_client.proto.angzarr.ProjectorCoordinatorService";
inline constexpr const char* HEALTH_NAME_UPCASTER =
    "angzarr_client.proto.angzarr.UpcasterCoordinatorService";

/// Pure-data transport configuration; produced by :func:`config_from_env`
/// and consumed by :func:`create_server`. Mirrors Py/Rs/Ja/C# shape.
///
/// When :data:`uds_path` is populated, the runner binds Unix Domain
/// Sockets (creating the parent dir + clearing stale sockets as a side
/// effect — see :func:`get_transport_config`). When ``nullopt`` the
/// runner binds TCP at :data:`bind_address`.
struct ServerConfig {
  uint16_t port = DEFAULT_PORT;
  std::string bind_address;  ///< ``"[::]:50052"`` etc. — TCP only.
  std::optional<std::string>
      uds_path;  ///< When present, supersedes TCP binding.
};

/// Compute the TCP bind address.
///
/// Audit #77: returns ``ANGZARR_BIND_ADDRESS`` verbatim when set;
/// otherwise composes ``[::]:{port}`` where ``port`` comes from
/// the ``PORT`` env var or ``default_port``. IPv6 hosts must include
/// brackets (``[::1]:50052``); IPv4 hosts are written bare
/// (``0.0.0.0:50052``).
std::string resolve_bind_address(uint16_t default_port = DEFAULT_PORT);

/// Build a :struct:`ServerConfig` from the environment.
///
/// UDS mode is selected when ``UDS_BASE_PATH`` is set (and a qualifier
/// — DOMAIN / SAGA_NAME / PROJECTOR_NAME / PROCESS_MANAGER_NAME /
/// UPCASTER_NAME — is available). Otherwise TCP, with port resolved
/// from ``PORT`` / ``GRPC_PORT`` / ``default_port`` precedence.
///
/// Pure compute: no filesystem side effects. The runner is responsible
/// for ``create_dir_all`` + ``cleanup_socket`` at the chosen path —
/// performed by :func:`get_transport_config`.
ServerConfig config_from_env(uint16_t default_port = DEFAULT_PORT);

/// Get transport configuration as ``("tcp", address)`` or
/// ``("uds", "unix:{path}")``. Has filesystem side effects in UDS mode
/// (matches Py/Go/Ja semantics): ensures parent directory exists and
/// removes any stale socket file at the chosen path.
std::pair<std::string, std::string> get_transport_config(
    uint16_t default_port = DEFAULT_PORT);

/// Clean up a UDS socket file. No-op if missing or empty. Mirrors
/// Python's ``cleanup_socket``.
void cleanup_socket(const std::string& socket_path);

/// Audit #89 idiom convenience: install absl::log default initialization
/// for users who haven't installed their own backend. Idempotent.
///
/// Other languages (Py: structlog, Rs: tracing-subscriber, Go: slog,
/// Ja: slf4j, C#: Microsoft.Extensions.Logging) each ship a
/// ``configure_logging`` initializer. C++ already had a pluggable
/// :class:`Logger` (see :file:`logging.hpp`); ``configure_logging``
/// formalizes the entry point so cross-language docs can point at a
/// uniform symbol per spec MED-5.10.
void configure_logging();

/// Drive the readiness supervisor loop until ``stop_requested()``
/// returns true. Audit #68 / #74:
///
/// - while any probe is failing, ``health_setter(name, false)`` keeps
///   ``""`` and the per-kind health name at NOT_SERVING.
/// - once every probe reports healthy, ``health_setter(name, true)``
///   flips both to SERVING. Subsequent dips back to unhealthy flip the
///   per-kind name (but never ``""``, which only flips back to
///   NOT_SERVING during two-phase shutdown — audit #83).
///
/// Cross-language parity: this matches Py ``run_supervisor``, Rs
/// ``run_supervisor``, Go ``(*ReadinessSupervisor).Run``, Ja
/// ``Readiness.runSupervisor``. Spec MED-5.9 was the gap.
using HealthSetter =
    std::function<void(const std::string& service_name, bool serving)>;

void run_supervisor(
    const std::vector<std::shared_ptr<readiness::Probe>>& probes,
    const HealthSetter& health_setter,
    const std::vector<std::string>& service_names,
    std::chrono::seconds interval, std::chrono::milliseconds timeout,
    const std::function<bool()>& stop_requested);

/// Two-phase shutdown helper (audit #83): flip every registered health
/// name (including ``""``) to NOT_SERVING, then return so the caller
/// can ``server->Shutdown()``. Emits the ``server_shutdown`` log line
/// (audit #89) with the supplied service / kind labels.
void publish_shutdown_status(const HealthSetter& health_setter,
                             const std::vector<std::string>& service_names,
                             const std::string& service_label,
                             const std::string& kind_label);

// ---------------------------------------------------------------------------
// Per-kind runner factories — spec MED-5.iter2.A.
//
// Cross-language parity with Py ``angzarr_client.server.run_*_server``,
// Rs ``angzarr_client::server::run_*_server``, Ja ``Server.run*Server``,
// C# ``Server.Run*Server``. Each factory:
//   1. Builds the appropriate gRPC service from the router.
//   2. Resolves transport via :func:`get_transport_config`.
//   3. Wires output-domain probes (saga/PM only) + the transport probe.
//   4. Drives :func:`run_supervisor` until SIGINT/SIGTERM.
//   5. Calls :func:`publish_shutdown_status` for two-phase shutdown
//      (audit #83), then tears down the server.
//
// Domain / name metadata is read from the router (e.g. the
// ``ANGZARR_DOMAIN`` / ``ANGZARR_SAGA_NAME`` env var declared at the
// component-router level, or compiled-in defaults). ``default_port``
// follows :data:`DEFAULT_PORT` when overridden by ``PORT`` / ``GRPC_PORT``
// is absent.
// ---------------------------------------------------------------------------

/// Run a command handler service. Mirrors Py ``run_command_handler_server``.
/// Templated on the router's state/handler types —
/// :class:`CommandHandlerRouter` is ``template <typename S, typename H>``.
template <typename S, typename H>
void run_command_handler_server(::angzarr::CommandHandlerRouter<S, H>& router,
                                uint16_t default_port = DEFAULT_PORT);

/// Run a saga service. Audit #74: only ``sync = true`` targets get an
/// :class:`OutputDomainProbe`; async-only sagas rely on the
/// :class:`BusProbe` (configured via ``ANGZARR_BUS_ENDPOINT``).
/// Templated on the handler type — :class:`SagaRouter` is
/// ``template <typename H>``.
template <typename H>
void run_saga_server(::angzarr::SagaRouter<H>& router,
                     uint16_t default_port = DEFAULT_PORT);

/// Run a process-manager service. Audit #74: only ``sync_targets`` get an
/// :class:`OutputDomainProbe`; async-only targets ride the
/// :class:`BusProbe`. Templated on the state type —
/// :class:`ProcessManagerRouter` is ``template <typename S>``.
template <typename S>
void run_process_manager_server(::angzarr::ProcessManagerRouter<S>& router,
                                uint16_t default_port = DEFAULT_PORT);

/// Run a projector service. Projectors are read-side and have no
/// output-domain probes. :class:`ProjectorRouter` is NOT templated.
void run_projector_server(::angzarr::ProjectorRouter& router,
                          uint16_t default_port = DEFAULT_PORT);

/// Run an upcaster service. Upcasters have no output-domain probes.
/// :class:`UpcasterRouter` is NOT templated.
void run_upcaster_server(::angzarr::UpcasterRouter& router,
                         uint16_t default_port = DEFAULT_PORT);

}  // namespace angzarr::server
