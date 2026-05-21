#pragma once

#include <grpcpp/grpcpp.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "angzarr/proto_aliases.hpp"
#include "errors.hpp"
#include "retry.hpp"

namespace angzarr {

// Forward declarations for builder convenience methods (defined in
// builder.hpp).
class CommandBuilder;
class QueryBuilder;

namespace detail {
/**
 * Format endpoint for gRPC channel creation.
 *
 * Audit finding #39 (Rust 7d54510 / Python parity): lenient UDS
 * prefix detection matching Python's ``client.py::_create_channel``.
 * Recognized UDS forms:
 *
 *   - ``/abs/path``               — absolute path, no scheme
 *   - ``./rel/path``              — relative path, no scheme
 *   - ``unix:relative/path``      — gRPC URI, relative
 *   - ``unix:/abs/path``          — gRPC URI, absolute (single slash)
 *   - ``unix:///abs/path``        — gRPC URI, absolute, empty authority
 *
 * All UDS forms normalize to the canonical ``unix://`` prefix that
 * gRPC's URI parser accepts. TCP endpoints (``host:port``,
 * ``http://...``, ``https://...``) drop any ``scheme://`` prefix.
 *
 * Postel's Law: be lenient in what we accept.
 */
inline std::string format_endpoint(const std::string& endpoint) {
  // UDS: absolute path, no scheme
  if (!endpoint.empty() && endpoint[0] == '/') {
    return "unix://" + endpoint;
  }
  // UDS: relative path, no scheme
  if (endpoint.size() >= 2 && endpoint[0] == '.' && endpoint[1] == '/') {
    return "unix://" + endpoint;
  }
  // UDS: triple-slash URI form (canonical)
  if (endpoint.rfind("unix:///", 0) == 0) {
    return endpoint;
  }
  // UDS: double-slash URI form
  if (endpoint.rfind("unix://", 0) == 0) {
    return endpoint;
  }
  // UDS: single-slash URI form (unix:/abs or unix:relative). Strip
  // the ``unix:`` prefix and re-prepend the canonical ``unix://``
  // so gRPC's URI parser accepts the resulting endpoint.
  if (endpoint.rfind("unix:", 0) == 0) {
    return "unix://" + endpoint.substr(5);
  }
  // TCP: plain host:port (no scheme)
  if (endpoint.find("://") == std::string::npos) {
    return endpoint;
  }
  // TCP: strip http:// or https:// prefix for gRPC
  auto pos = endpoint.find("://");
  return endpoint.substr(pos + 3);
}
}  // namespace detail

/**
 * Client for querying aggregate event streams.
 *
 * QueryClient provides read access to aggregate event streams. In event-sourced
 * systems, all state is derived from events. QueryClient enables:
 *
 * - State reconstruction: Fetch events to rebuild aggregate state locally
 * - Audit trails: Read complete history for debugging and compliance
 * - Projections: Feed events to read-model projectors
 * - Testing: Verify events were persisted correctly after commands
 *
 * Example:
 *   auto client = QueryClient::connect("localhost:1310");
 *   Query query;
 *   query.mutable_cover()->set_domain("orders");
 *   auto events = client->get_event_book(query);
 */
class QueryClient {
 public:
  /**
   * Connect to an event query service at the given endpoint.
   *
   * @param endpoint Server endpoint (e.g., "localhost:1310")
   * @return Unique pointer to QueryClient
   * @throws ConnectionError if connection fails
   */
  static std::unique_ptr<QueryClient> connect(const std::string& endpoint) {
    return connect(endpoint, default_retry_policy());
  }

  static std::unique_ptr<QueryClient> connect(
      const std::string& endpoint, std::unique_ptr<RetryPolicy> retry) {
    std::shared_ptr<grpc::Channel> channel;
    retry->execute([&]() {
      channel = grpc::CreateChannel(format_endpoint(endpoint),
                                    grpc::InsecureChannelCredentials());
    });
    return std::make_unique<QueryClient>(channel);
  }

  /**
   * Connect using an endpoint from environment variable with fallback.
   *
   * Production deployments use environment variables for configuration.
   * This enables the same binary to run in different environments
   * without code changes.
   *
   * @param env_var Environment variable name
   * @param default_endpoint Fallback endpoint if env var is not set
   * @return Unique pointer to QueryClient
   */
  static std::unique_ptr<QueryClient> from_env(
      const std::string& env_var, const std::string& default_endpoint) {
    const char* endpoint = std::getenv(env_var.c_str());
    return connect(endpoint ? endpoint : default_endpoint);
  }

  /**
   * Create a client from an existing channel.
   *
   * Channel lifetime is managed via shared_ptr reference counting — the
   * caller may share ownership with this client.
   *
   * @param channel Shared gRPC channel
   */
  explicit QueryClient(std::shared_ptr<grpc::Channel> channel)
      : channel_(std::move(channel)),
        stub_(EventQueryService::NewStub(channel_)) {}

  /**
   * Create a client from a caller-managed channel.
   *
   * Named factory equivalent to the shared-channel constructor; provided for
   * naming parity with Python ``QueryClient.from_channel``, Go
   * ``QueryClientFromConn``, Java/C# ``FromChannel``, Rust
   * ``QueryClient::from_channel``.
   */
  static std::unique_ptr<QueryClient> from_channel(
      std::shared_ptr<grpc::Channel> channel) {
    return std::make_unique<QueryClient>(std::move(channel));
  }

  /**
   * Release the stub and drop this client's reference to the channel.
   * Idempotent and safe to call multiple times. After ``close()`` further
   * RPCs are undefined; mirror of Python ``close()`` / Rust ``close``.
   */
  void close() {
    stub_.reset();
    channel_.reset();
  }

  /**
   * Query events for an aggregate and return a single EventBook.
   *
   * @param query The query specifying domain, root, and optional filters
   * @return EventBook containing matching events
   * @throws GrpcError if the gRPC call fails
   */
  EventBook get_event_book(const Query& query) {
    EventBook response;
    grpc::ClientContext context;
    auto status = stub_->GetEventBook(&context, query, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  /**
   * Query events and return all matching EventBooks.
   *
   * Uses streaming RPC for bulk retrieval.
   *
   * @param query The query specifying domain, root, and optional filters
   * @return Vector of EventBooks
   * @throws GrpcError if the gRPC call fails
   */
  std::vector<EventBook> get_events(const Query& query) {
    std::vector<EventBook> results;
    grpc::ClientContext context;
    auto reader = stub_->GetEvents(&context, query);
    EventBook book;
    while (reader->Read(&book)) {
      results.push_back(std::move(book));
    }
    auto status = reader->Finish();
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return results;
  }

  /// Start building a query for a specific aggregate (defined in builder.hpp).
  inline QueryBuilder query(const std::string& domain,
                            const std::string& root_bytes);

  /// Start building a query by domain only (defined in builder.hpp).
  inline QueryBuilder query_domain(const std::string& domain);

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<EventQueryService::Stub> stub_;

  static std::string format_endpoint(const std::string& endpoint) {
    return detail::format_endpoint(endpoint);
  }
};

/**
 * Client for sending commands to command handlers through the coordinator.
 *
 * CommandHandlerClient handles command routing, response parsing, and provides
 * multiple execution modes:
 *
 * - Async (fire-and-forget): For high-throughput scenarios
 * - Sync: Wait for persistence, receive resulting events
 * - Speculative: What-if execution without persistence
 *
 * Example:
 *   auto client = CommandHandlerClient::connect("localhost:1310");
 *   CommandBook cmd;
 *   // ... build command ...
 *   auto response = client->handle(cmd);
 */
class CommandHandlerClient {
 public:
  /**
   * Connect to a command handler coordinator at the given endpoint.
   *
   * @param endpoint Server endpoint (e.g., "localhost:1310")
   * @return Unique pointer to CommandHandlerClient
   * @throws ConnectionError if connection fails
   */
  static std::unique_ptr<CommandHandlerClient> connect(
      const std::string& endpoint) {
    return connect(endpoint, default_retry_policy());
  }

  static std::unique_ptr<CommandHandlerClient> connect(
      const std::string& endpoint, std::unique_ptr<RetryPolicy> retry) {
    std::shared_ptr<grpc::Channel> channel;
    retry->execute([&]() {
      channel = grpc::CreateChannel(format_endpoint(endpoint),
                                    grpc::InsecureChannelCredentials());
    });
    return std::make_unique<CommandHandlerClient>(channel);
  }

  /**
   * Connect using an endpoint from environment variable with fallback.
   *
   * @param env_var Environment variable name
   * @param default_endpoint Fallback endpoint if env var is not set
   * @return Unique pointer to CommandHandlerClient
   */
  static std::unique_ptr<CommandHandlerClient> from_env(
      const std::string& env_var, const std::string& default_endpoint) {
    const char* endpoint = std::getenv(env_var.c_str());
    return connect(endpoint ? endpoint : default_endpoint);
  }

  /**
   * Create a client from an existing channel.
   *
   * @param channel Shared gRPC channel
   */
  explicit CommandHandlerClient(std::shared_ptr<grpc::Channel> channel)
      : channel_(std::move(channel)),
        stub_(CommandHandlerCoordinatorService::NewStub(channel_)) {}

  /**
   * Create a client from a caller-managed channel. Named factory mirroring
   * Python ``CommandHandlerClient.from_channel``, Rust
   * ``CommandHandlerClient::from_channel``.
   */
  static std::unique_ptr<CommandHandlerClient> from_channel(
      std::shared_ptr<grpc::Channel> channel) {
    return std::make_unique<CommandHandlerClient>(std::move(channel));
  }

  /**
   * Release the stub and drop this client's reference to the channel.
   * Idempotent and safe to call multiple times.
   */
  void close() {
    stub_.reset();
    channel_.reset();
  }

  /**
   * Execute a command asynchronously (fire-and-forget).
   *
   * Returns immediately after the coordinator accepts the command.
   * The command is guaranteed to be processed, but the client doesn't wait.
   *
   * @param command The command to execute
   * @return CommandResponse indicating acceptance
   * @throws GrpcError if the gRPC call fails
   */
  CommandResponse handle(const CommandBook& command) {
    CommandRequest request;
    *request.mutable_command() = command;
    request.set_sync_mode(SYNC_MODE_ASYNC);
    request.set_cascade_error_mode(CASCADE_ERROR_FAIL_FAST);

    CommandResponse response;
    grpc::ClientContext context;
    auto status = stub_->HandleCommand(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  /**
   * Execute a command synchronously.
   *
   * Blocks until the aggregate processes the command and events are persisted.
   * The response includes the resulting events.
   *
   * @param command The command to execute
   * @param sync_mode The sync mode (default: SYNC_MODE_SIMPLE)
   * @return CommandResponse with resulting events
   * @throws GrpcError if the gRPC call fails
   */
  CommandResponse handle_sync(const CommandBook& command,
                              SyncMode sync_mode = SYNC_MODE_SIMPLE) {
    CommandRequest request;
    *request.mutable_command() = command;
    request.set_sync_mode(sync_mode);

    CommandResponse response;
    grpc::ClientContext context;
    auto status = stub_->HandleCommand(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  /**
   * Execute a command speculatively against temporal state (no persistence).
   *
   * Use for form validation, preview, or testing without polluting event store.
   * The aggregate state remains unchanged after speculative execution.
   *
   * @param request The speculative execution request
   * @return CommandResponse with projected events
   * @throws GrpcError if the gRPC call fails
   */
  CommandResponse handle_sync_speculative(
      const SpeculateCommandHandlerRequest& request) {
    CommandResponse response;
    grpc::ClientContext context;
    auto status = stub_->HandleSyncSpeculative(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  /**
   * Execute a command with the specified CommandRequest.
   *
   * Full-control method accepting a pre-built CommandRequest with custom
   * sync mode and cascade error mode settings.
   *
   * @param request The command request with sync mode and options
   * @return CommandResponse with resulting events
   * @throws GrpcError if the gRPC call fails
   */
  CommandResponse handle_command(const CommandRequest& request) {
    CommandResponse response;
    grpc::ClientContext context;
    auto status = stub_->HandleCommand(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  /// Start building a command for an existing aggregate (defined in
  /// builder.hpp).
  inline CommandBuilder command(const std::string& domain,
                                const std::string& root_bytes);

  /// Start building a command for a new aggregate (defined in builder.hpp).
  inline CommandBuilder command_new(const std::string& domain);

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<CommandHandlerCoordinatorService::Stub> stub_;

  static std::string format_endpoint(const std::string& endpoint) {
    return detail::format_endpoint(endpoint);
  }
};

/**
 * Client for speculative (what-if) execution across coordinator services.
 *
 * Speculative execution runs commands/events against temporal state without
 * persistence. Use for form validation, preview, or testing.
 */
class SpeculativeClient {
 public:
  static std::unique_ptr<SpeculativeClient> connect(
      const std::string& endpoint) {
    auto formatted = detail::format_endpoint(endpoint);
    auto channel =
        grpc::CreateChannel(formatted, grpc::InsecureChannelCredentials());
    return std::make_unique<SpeculativeClient>(channel);
  }

  static std::unique_ptr<SpeculativeClient> from_env(
      const std::string& env_var, const std::string& default_endpoint) {
    const char* endpoint = std::getenv(env_var.c_str());
    return connect(endpoint ? endpoint : default_endpoint);
  }

  explicit SpeculativeClient(std::shared_ptr<grpc::Channel> channel)
      : channel_(std::move(channel)),
        ch_stub_(CommandHandlerCoordinatorService::NewStub(channel_)),
        projector_stub_(ProjectorCoordinatorService::NewStub(channel_)),
        saga_stub_(SagaCoordinatorService::NewStub(channel_)),
        pm_stub_(ProcessManagerCoordinatorService::NewStub(channel_)) {}

  /**
   * Create a client from a caller-managed channel. Named factory mirroring
   * Python ``SpeculativeClient.from_channel``, Rust
   * ``SpeculativeClient::from_channel``.
   */
  static std::unique_ptr<SpeculativeClient> from_channel(
      std::shared_ptr<grpc::Channel> channel) {
    return std::make_unique<SpeculativeClient>(std::move(channel));
  }

  /**
   * Release all stubs and drop this client's reference to the channel.
   * Idempotent and safe to call multiple times.
   */
  void close() {
    ch_stub_.reset();
    projector_stub_.reset();
    saga_stub_.reset();
    pm_stub_.reset();
    channel_.reset();
  }

  CommandResponse command_handler(
      const SpeculateCommandHandlerRequest& request) {
    CommandResponse response;
    grpc::ClientContext context;
    auto status = ch_stub_->HandleSyncSpeculative(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  // Returns the proto Projection (cover/projector/sequence/payload),
  // not the in-memory key/value helper struct of the same simple
  // name in :file:`handler_traits.hpp`. Fully qualified to disambiguate.
  ::angzarr_client::proto::angzarr::v1::Projection projector(
      const SpeculateProjectorRequest& request) {
    ::angzarr_client::proto::angzarr::v1::Projection response;
    grpc::ClientContext context;
    auto status =
        projector_stub_->HandleSpeculative(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  SagaResponse saga(const SpeculateSagaRequest& request) {
    SagaResponse response;
    grpc::ClientContext context;
    auto status = saga_stub_->ExecuteSpeculative(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

  ProcessManagerHandleResponse process_manager(
      const SpeculatePmRequest& request) {
    ProcessManagerHandleResponse response;
    grpc::ClientContext context;
    auto status = pm_stub_->HandleSpeculative(&context, request, &response);
    if (!status.ok()) {
      throw GrpcError(status);
    }
    return response;
  }

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<CommandHandlerCoordinatorService::Stub> ch_stub_;
  std::unique_ptr<ProjectorCoordinatorService::Stub> projector_stub_;
  std::unique_ptr<SagaCoordinatorService::Stub> saga_stub_;
  std::unique_ptr<ProcessManagerCoordinatorService::Stub> pm_stub_;
};

/**
 * Combined client for command handler commands and event queries.
 *
 * DomainClient combines QueryClient and CommandHandlerClient into a single
 * unified interface. This is the recommended entry point for most applications
 * because:
 *
 * - Single connection: One endpoint, one channel, reduced resource usage
 * - Unified API: Both queries and commands through one object
 * - Simpler DI: Inject one client instead of two
 *
 * For advanced use cases (separate scaling, different endpoints), use
 * QueryClient and CommandHandlerClient directly.
 *
 * Example:
 *   auto client = DomainClient::connect("localhost:1310");
 *   // Send a command
 *   auto response = client->command_handler()->handle(cmd);
 *   // Query events
 *   auto events = client->query()->get_event_book(query);
 */
class DomainClient {
 public:
  /**
   * Connect to a domain's coordinator at the given endpoint.
   *
   * @param endpoint Server endpoint (e.g., "localhost:1310")
   * @return Unique pointer to DomainClient
   * @throws ConnectionError if connection fails
   */
  static std::unique_ptr<DomainClient> connect(const std::string& endpoint) {
    auto formatted = format_endpoint(endpoint);
    auto channel =
        grpc::CreateChannel(formatted, grpc::InsecureChannelCredentials());
    return std::make_unique<DomainClient>(channel);
  }

  /**
   * Connect using an endpoint from environment variable with fallback.
   *
   * @param env_var Environment variable name
   * @param default_endpoint Fallback endpoint if env var is not set
   * @return Unique pointer to DomainClient
   */
  static std::unique_ptr<DomainClient> from_env(
      const std::string& env_var, const std::string& default_endpoint) {
    const char* endpoint = std::getenv(env_var.c_str());
    return connect(endpoint ? endpoint : default_endpoint);
  }

  /**
   * Create a client from an existing channel.
   *
   * @param channel Shared gRPC channel
   */
  explicit DomainClient(std::shared_ptr<grpc::Channel> channel)
      : channel_(std::move(channel)),
        command_handler_(std::make_unique<CommandHandlerClient>(channel_)),
        query_(std::make_unique<QueryClient>(channel_)),
        speculative_(std::make_unique<SpeculativeClient>(channel_)) {}

  /**
   * Create a client from a caller-managed channel. Named factory mirroring
   * Python ``DomainClient.from_channel``, Rust ``DomainClient::from_channel``.
   */
  static std::unique_ptr<DomainClient> from_channel(
      std::shared_ptr<grpc::Channel> channel) {
    return std::make_unique<DomainClient>(std::move(channel));
  }

  /**
   * Release all sub-clients and drop this client's reference to the channel.
   * Idempotent and safe to call multiple times.
   */
  void close() {
    if (command_handler_) command_handler_->close();
    if (query_) query_->close();
    if (speculative_) speculative_->close();
    channel_.reset();
  }

  /**
   * Get the command handler client for command execution.
   */
  CommandHandlerClient* command_handler() { return command_handler_.get(); }

  /**
   * Get the query client for event retrieval.
   */
  QueryClient* query() { return query_.get(); }

  /**
   * Get the speculative client for what-if scenarios.
   */
  SpeculativeClient* speculative() { return speculative_.get(); }

  /**
   * Execute a command (convenience method delegating to command handler).
   */
  CommandResponse execute(const CommandBook& command) {
    return command_handler_->handle(command);
  }

  /// Start building a command for an existing aggregate (defined in
  /// builder.hpp).
  inline CommandBuilder command(const std::string& domain,
                                const std::string& root_bytes);

  /// Start building a command for a new aggregate (defined in builder.hpp).
  inline CommandBuilder command_new(const std::string& domain);

  /// Start building a query for a specific aggregate (defined in builder.hpp).
  inline QueryBuilder query_events(const std::string& domain,
                                   const std::string& root_bytes);

  /// Start building a query by domain only (defined in builder.hpp).
  inline QueryBuilder query_domain(const std::string& domain);

 private:
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<CommandHandlerClient> command_handler_;
  std::unique_ptr<QueryClient> query_;
  std::unique_ptr<SpeculativeClient> speculative_;

  static std::string format_endpoint(const std::string& endpoint) {
    return detail::format_endpoint(endpoint);
  }
};

// Alias for backward compatibility with builder.hpp
using AggregateClient = CommandHandlerClient;

}  // namespace angzarr
