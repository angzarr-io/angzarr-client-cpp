#include "angzarr/grpc_translator.hpp"

#include "angzarr/errors.hpp"

namespace angzarr {

grpc::Status to_grpc_status(const ClientError& e) {
  // CommandRejectedError carries an explicit ``status_code`` field
  // populated by the factory. Mirrors Py ``CommandRejectedError`` →
  // status mapping at ``server.py:49-77``.
  if (auto rej = dynamic_cast<const CommandRejectedError*>(&e)) {
    return grpc::Status(rej->status_code, e.what());
  }
  if (auto grpc_err = dynamic_cast<const GrpcError*>(&e)) {
    // Pass-through the original status — preserves the dynamic
    // server message in ``.status().message()`` while the public
    // ``.what()`` (and therefore Display in other langs) stays
    // static. Spec PY-RS-MISMATCH-1.2 semantics.
    const auto& s = grpc_err->status();
    if (s.ok() && s.error_message().empty()) {
      return grpc::Status(grpc_err->status_code(), e.what());
    }
    return s;
  }
  if (dynamic_cast<const InvalidArgumentError*>(&e) ||
      dynamic_cast<const InvalidTimestampError*>(&e)) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
  }
  if (dynamic_cast<const ConnectionError*>(&e) ||
      dynamic_cast<const TransportError*>(&e)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, e.what());
  }
  // Fall-through: any other ClientError → INTERNAL.
  return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
}

grpc::Status to_grpc_status(std::exception_ptr eptr) {
  if (!eptr) return grpc::Status::OK;
  try {
    std::rethrow_exception(eptr);
  } catch (const ClientError& ce) {
    return to_grpc_status(ce);
  } catch (const std::exception& e) {
    return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
  } catch (...) {
    return grpc::Status(grpc::StatusCode::INTERNAL, "unknown error");
  }
}

}  // namespace angzarr
