#pragma once

#include <grpcpp/grpcpp.h>

#include <map>
#include <stdexcept>
#include <string>
#include <utility>

namespace angzarr {

/**
 * Base exception for all Angzarr client errors.
 *
 * Audit finding #59 (structural error model). Every error carries:
 *
 *  - a static ``message`` — the same exact string for the same
 *    predicate failure across all call sites, suitable for log
 *    greppability, cross-language equality checks, and i18n keys.
 *  - a stable ``code`` identifier (SCREAMING_SNAKE) — programmatic
 *    dispatch and cucumber assertions key off this.
 *  - structured ``details`` — runtime context (field name, type URL,
 *    domain, status code) that varies per call site.
 *
 * Callers MUST NOT interpolate runtime values into ``message``; that
 * defeats the static-string contract. Put runtime values in
 * ``details``.
 *
 * Mirrors Python's ``ClientError`` and Rust's ``ClientError`` with
 * the same ``code`` values for cross-language symmetry.
 */
class ClientError : public std::runtime_error {
 public:
  using DetailsMap = std::map<std::string, std::string>;

  explicit ClientError(const std::string& message)
      : std::runtime_error(message), code_{}, details_{} {}

  ClientError(const std::string& message, const std::string& code,
              DetailsMap details = {})
      : std::runtime_error(message),
        code_(code),
        details_(std::move(details)) {}

  /// Stable SCREAMING_SNAKE identifier (e.g. ``MISSING_DESTINATION_SEQUENCE``).
  const std::string& code() const { return code_; }

  /// Structured runtime context (field/domain/type_url/...).
  const DetailsMap& details() const { return details_; }

  /// Returns true if this is a "not found" error.
  virtual bool is_not_found() const { return false; }

  /// Returns true if this is a "precondition failed" error.
  virtual bool is_precondition_failed() const { return false; }

  /// Returns true if this is an "invalid argument" error.
  virtual bool is_invalid_argument() const { return false; }

  /// Returns true if this is a connection or transport error.
  virtual bool is_connection_error() const { return false; }

 private:
  std::string code_;
  DetailsMap details_;
};

/**
 * Thrown when a command is rejected by business logic.
 * Maps to gRPC FAILED_PRECONDITION status by default.
 */
class CommandRejectedError : public ClientError {
 public:
  explicit CommandRejectedError(const std::string& message)
      : ClientError(message),
        status_code(grpc::StatusCode::FAILED_PRECONDITION) {}

  CommandRejectedError(const std::string& message, grpc::StatusCode status)
      : ClientError(message), status_code(status) {}

  CommandRejectedError(const std::string& message, grpc::StatusCode status,
                       const std::string& code, DetailsMap details = {})
      : ClientError(message, code, std::move(details)), status_code(status) {}

  bool is_precondition_failed() const override {
    return status_code == grpc::StatusCode::FAILED_PRECONDITION;
  }

  bool is_invalid_argument() const override {
    return status_code == grpc::StatusCode::INVALID_ARGUMENT;
  }

  bool is_not_found() const override {
    return status_code == grpc::StatusCode::NOT_FOUND;
  }

  /// gRPC status code for this rejection.
  grpc::StatusCode status_code;

  /// Factory methods for common rejection types.
  static CommandRejectedError precondition_failed(const std::string& code,
                                                  const std::string& message,
                                                  DetailsMap details = {}) {
    return CommandRejectedError(message, grpc::StatusCode::FAILED_PRECONDITION,
                                code, std::move(details));
  }

  static CommandRejectedError invalid_argument(const std::string& code,
                                               const std::string& message,
                                               DetailsMap details = {}) {
    return CommandRejectedError(message, grpc::StatusCode::INVALID_ARGUMENT,
                                code, std::move(details));
  }

  static CommandRejectedError not_found(const std::string& code,
                                        const std::string& message,
                                        DetailsMap details = {}) {
    return CommandRejectedError(message, grpc::StatusCode::NOT_FOUND, code,
                                std::move(details));
  }

  // Legacy single-arg factories — keep building so existing tests
  // and downstream code continue to compile.  New call sites should
  // prefer the structured (code, message, details) form above.
  static CommandRejectedError precondition_failed(const std::string& message) {
    return CommandRejectedError(message, grpc::StatusCode::FAILED_PRECONDITION);
  }

  static CommandRejectedError invalid_argument(const std::string& message) {
    return CommandRejectedError(message, grpc::StatusCode::INVALID_ARGUMENT);
  }

  static CommandRejectedError not_found(const std::string& message) {
    return CommandRejectedError(message, grpc::StatusCode::NOT_FOUND);
  }

  // Spec MED-1.9 — the ``already_exists`` factory was Cpp-only and had
  // no ``ALREADY_EXISTS`` code in the cross-language inventory.
  // Removed in D-C++; the ``ENTITY_ALREADY_EXISTS`` code in the
  // inventory still exists for explicit construction via the
  // ``(message, status, code, details)`` ctor when needed.
};

/**
 * Thrown when a gRPC call fails.
 */
class GrpcError : public ClientError {
 public:
  GrpcError(const std::string& message, grpc::StatusCode status_code)
      : ClientError(message), status_code_(status_code) {}

  explicit GrpcError(const grpc::Status& status)
      : ClientError(status.error_message()),
        status_code_(status.error_code()),
        status_(status) {}

  grpc::StatusCode status_code() const { return status_code_; }

  /// Get the original gRPC Status (if available).
  const grpc::Status& status() const { return status_; }

  bool is_not_found() const override {
    return status_code_ == grpc::StatusCode::NOT_FOUND;
  }

  bool is_precondition_failed() const override {
    return status_code_ == grpc::StatusCode::FAILED_PRECONDITION;
  }

  bool is_invalid_argument() const override {
    return status_code_ == grpc::StatusCode::INVALID_ARGUMENT;
  }

  bool is_connection_error() const override {
    return status_code_ == grpc::StatusCode::UNAVAILABLE;
  }

 private:
  grpc::StatusCode status_code_;
  grpc::Status status_;
};

/**
 * Thrown when connection to the server fails.
 */
class ConnectionError : public ClientError {
 public:
  explicit ConnectionError(const std::string& message) : ClientError(message) {}
  ConnectionError(const std::string& message, const std::string& code,
                  DetailsMap details = {})
      : ClientError(message, code, std::move(details)) {}

  bool is_connection_error() const override { return true; }
};

/**
 * Thrown when transport-level errors occur.
 */
class TransportError : public ClientError {
 public:
  explicit TransportError(const std::string& message) : ClientError(message) {}
  TransportError(const std::string& message, const std::string& code,
                 DetailsMap details = {})
      : ClientError(message, code, std::move(details)) {}

  bool is_connection_error() const override { return true; }
};

/**
 * Thrown when an invalid argument is provided.
 *
 * Audit #59 structural error: prefer the (message, code, details)
 * form for new call sites so cross-language assertions can key on
 * the code rather than a fragile substring match.
 */
class InvalidArgumentError : public ClientError {
 public:
  explicit InvalidArgumentError(const std::string& message)
      : ClientError(message) {}
  InvalidArgumentError(const std::string& message, const std::string& code,
                       DetailsMap details = {})
      : ClientError(message, code, std::move(details)) {}

  bool is_invalid_argument() const override { return true; }
};

/**
 * Thrown when a timestamp cannot be parsed.
 */
class InvalidTimestampError : public ClientError {
 public:
  explicit InvalidTimestampError(const std::string& message)
      : ClientError(message) {}
  InvalidTimestampError(const std::string& message, const std::string& code,
                        DetailsMap details = {})
      : ClientError(message, code, std::move(details)) {}
};

}  // namespace angzarr
