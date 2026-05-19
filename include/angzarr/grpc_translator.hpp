#pragma once

// Cross-language gRPC status translator.
//
// Spec HIGH-1.3: prior to D-C++, the C++ client had no dedicated
// translator from ``ClientError`` / ``CommandRejectedError`` subclasses
// to ``grpc::Status``. A ``precondition_failed`` rejection from a
// business handler propagated as a default-INTERNAL error rather than
// FAILED_PRECONDITION — a cross-language wire-protocol divergence
// (Py: ``_translate_and_abort``, Rs: ``client_error_to_status``,
// Ja: ``GrpcAdapters.toStatus``, C#: ``GrpcStatusTranslator.ToRpcException``).
//
// This module supplies that translator. It also exposes a wrapper for
// :class:`google::protobuf::Any` decode failures so callers can stamp
// ``ANY_DECODE_FAILED`` per audit #87 (spec MED-1.8 / MED-4.10).

#include <grpcpp/grpcpp.h>

#include <exception>
#include <string>

#include "angzarr/errors.hpp"

namespace google::protobuf {
class Any;
class Message;
}  // namespace google::protobuf

namespace angzarr {

/// Translate any caught exception into a ``grpc::Status``. Spec HIGH-1.3
/// — cross-language mapping:
///
///   - ``CommandRejectedError`` → its embedded ``status_code``
///     (FAILED_PRECONDITION / INVALID_ARGUMENT / NOT_FOUND / ...).
///   - ``InvalidArgumentError`` / ``InvalidTimestampError`` → INVALID_ARGUMENT.
///   - ``ConnectionError`` / ``TransportError`` → UNAVAILABLE.
///   - ``GrpcError`` → its embedded status (pass-through).
///   - ``ClientError`` (other) → INTERNAL.
///   - anything else → INTERNAL with ``e.what()``.
grpc::Status to_grpc_status(std::exception_ptr eptr);

/// Convenience: translate a live exception object directly without an
/// ``exception_ptr`` round-trip.
grpc::Status to_grpc_status(const ClientError& e);

/// Spec MED-1.8 / MED-4.10: wrap a ``google::protobuf::Any::UnpackTo``
/// call so a decode failure raises a structured
/// :class:`InvalidArgumentError` with code ``ANY_DECODE_FAILED`` (audit
/// #87 — same shape as Py ``_unpack_any``, Rs macro-emitted dispatch,
/// Go ``UnpackAnyE``, Ja ``Dispatch.decodeMessage``).
///
/// ``T`` must be a concrete protobuf message type with ``ParseFromString``.
template <typename T>
void unpack_any_or_throw(const google::protobuf::Any& any, T* out);

}  // namespace angzarr

// Template definition — header-only so call sites get implicit
// instantiation without a build-system explicit-instantiation list.
#include "angzarr/error_codes.hpp"
#include "angzarr/proto_aliases.hpp"  // for google::protobuf::Any visibility

namespace angzarr {

template <typename T>
void unpack_any_or_throw(const google::protobuf::Any& any, T* out) {
  if (out == nullptr) {
    throw InvalidArgumentError(
        error_codes::messages::ANY_DECODE_FAILED,
        error_codes::codes::ANY_DECODE_FAILED,
        ClientError::DetailsMap{
            {error_codes::keys::CAUSE, "out pointer is null"},
            {error_codes::keys::TYPE_URL, any.type_url()},
        });
  }
  if (!any.UnpackTo(out)) {
    throw InvalidArgumentError(
        error_codes::messages::ANY_DECODE_FAILED,
        error_codes::codes::ANY_DECODE_FAILED,
        ClientError::DetailsMap{
            {error_codes::keys::TYPE_URL, any.type_url()},
        });
  }
}

}  // namespace angzarr
