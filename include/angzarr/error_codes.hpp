// Cross-language error-code parity (Python:
// error_codes/{codes,messages,keys}.py / Rust: error_codes.rs).
//
// Every constant here matches the Python/Rust value
// character-for-character — that's the contract programs and
// cucumber assertions key off. Audit findings #59 (structural error
// model) and #40 (INVALID_TRANSPORT_MODE/INVALID_PORT symmetry) drive
// the bulk of the surface; the remainder is mechanical fill-in from
// the deep-scan sweep (#60-#86).

#pragma once

namespace angzarr::error_codes {

/// SCREAMING_SNAKE event-type identifiers — the value of ``code`` on
/// every error this client constructs.
namespace codes {

// Validation
inline constexpr const char* VALUE_NOT_POSITIVE = "VALUE_NOT_POSITIVE";
inline constexpr const char* VALUE_NOT_NON_NEGATIVE = "VALUE_NOT_NON_NEGATIVE";
inline constexpr const char* VALUE_EMPTY = "VALUE_EMPTY";
inline constexpr const char* COLLECTION_EMPTY = "COLLECTION_EMPTY";
inline constexpr const char* ENTITY_NOT_FOUND = "ENTITY_NOT_FOUND";
inline constexpr const char* ENTITY_ALREADY_EXISTS = "ENTITY_ALREADY_EXISTS";
inline constexpr const char* STATUS_MISMATCH = "STATUS_MISMATCH";
inline constexpr const char* STATUS_FORBIDDEN = "STATUS_FORBIDDEN";
// PR #12 — ChangeSeats rejected when current_seat == requested_seat. The
// 3-decision design (BlindLevel int64, SEATS_IDENTICAL, rng_seed variable
// bytes) pins this name; cross-language error_codes carry the same value.
inline constexpr const char* SEATS_IDENTICAL = "SEATS_IDENTICAL";

// Builder
inline constexpr const char* COMMAND_TYPE_URL_MISSING =
    "COMMAND_TYPE_URL_MISSING";
inline constexpr const char* COMMAND_PAYLOAD_MISSING =
    "COMMAND_PAYLOAD_MISSING";
inline constexpr const char* COMMAND_SEQUENCE_MISSING =
    "COMMAND_SEQUENCE_MISSING";

// Conversion
inline constexpr const char* ANY_TYPE_MISMATCH = "ANY_TYPE_MISMATCH";
inline constexpr const char* ANY_DECODE_FAILED = "ANY_DECODE_FAILED";
inline constexpr const char* PROTO_UUID_INVALID = "PROTO_UUID_INVALID";
inline constexpr const char* TIMESTAMP_PARSE_FAILED = "TIMESTAMP_PARSE_FAILED";

// Transport / connection
inline constexpr const char* ENDPOINT_PARSE_FAILED = "ENDPOINT_PARSE_FAILED";
inline constexpr const char* ENDPOINT_INVALID_URI = "ENDPOINT_INVALID_URI";
inline constexpr const char* CONNECTION_FAILED = "CONNECTION_FAILED";
inline constexpr const char* CONNECTION_FAILED_MAX_RETRIES =
    "CONNECTION_FAILED_MAX_RETRIES";
inline constexpr const char* TRANSPORT_ERROR = "TRANSPORT_ERROR";
inline constexpr const char* GRPC_ERROR = "GRPC_ERROR";
// Audit #40: cross-language symmetry for transport-env validation.
inline constexpr const char* INVALID_TRANSPORT_MODE = "INVALID_TRANSPORT_MODE";
inline constexpr const char* INVALID_PORT = "INVALID_PORT";

// Dispatch — common
inline constexpr const char* HANDLER_WRONG_RESPONSE_KIND =
    "HANDLER_WRONG_RESPONSE_KIND";
inline constexpr const char* HANDLER_WRONG_REQUEST_KIND =
    "HANDLER_WRONG_REQUEST_KIND";
inline constexpr const char* NO_HANDLER_REGISTERED = "NO_HANDLER_REGISTERED";
inline constexpr const char* MISSING_COMMAND_BOOK = "MISSING_COMMAND_BOOK";
inline constexpr const char* MISSING_COMMAND_PAGE = "MISSING_COMMAND_PAGE";
inline constexpr const char* MISSING_COMMAND_PAYLOAD =
    "MISSING_COMMAND_PAYLOAD";
inline constexpr const char* NOTIFICATION_DECODE_FAILED =
    "NOTIFICATION_DECODE_FAILED";
inline constexpr const char* REJECTION_NOTIFICATION_DECODE_FAILED =
    "REJECTION_NOTIFICATION_DECODE_FAILED";

// Dispatch — saga
inline constexpr const char* MISSING_SAGA_SOURCE = "MISSING_SAGA_SOURCE";
inline constexpr const char* EMPTY_SAGA_SOURCE = "EMPTY_SAGA_SOURCE";
inline constexpr const char* MISSING_SAGA_EVENT_PAYLOAD =
    "MISSING_SAGA_EVENT_PAYLOAD";
inline constexpr const char* SAGA_INVALID_TYPE_URL = "SAGA_INVALID_TYPE_URL";
inline constexpr const char* SAGA_HANDLER_UNSUPPORTED_RETURN_TYPE =
    "SAGA_HANDLER_UNSUPPORTED_RETURN_TYPE";

// Dispatch — process manager
inline constexpr const char* MISSING_PM_TRIGGER = "MISSING_PM_TRIGGER";
inline constexpr const char* EMPTY_PM_TRIGGER = "EMPTY_PM_TRIGGER";
inline constexpr const char* MISSING_PM_EVENT_PAYLOAD =
    "MISSING_PM_EVENT_PAYLOAD";
inline constexpr const char* PM_INVALID_TYPE_URL = "PM_INVALID_TYPE_URL";
inline constexpr const char* PM_HANDLER_WRONG_RETURN_TYPE =
    "PM_HANDLER_WRONG_RETURN_TYPE";

// Dispatch — upcaster
inline constexpr const char* UPCASTER_WRONG_RESPONSE_KIND =
    "UPCASTER_WRONG_RESPONSE_KIND";

// Build-time validation
inline constexpr const char* HANDLER_FIELD_EMPTY_STRING =
    "HANDLER_FIELD_EMPTY_STRING";
inline constexpr const char* HANDLER_FIELD_EMPTY_LIST =
    "HANDLER_FIELD_EMPTY_LIST";
inline constexpr const char* HANDLER_STATE_NOT_TYPE = "HANDLER_STATE_NOT_TYPE";
inline constexpr const char* HANDLER_UNKNOWN_KIND = "HANDLER_UNKNOWN_KIND";
inline constexpr const char* ROUTER_NO_HANDLERS = "ROUTER_NO_HANDLERS";
// Audit #62: a Router was built with two `command_handler`s claiming
// the same ``(domain, type_url)`` pair.
inline constexpr const char* DUPLICATE_COMMAND_HANDLER =
    "DUPLICATE_COMMAND_HANDLER";
// Audit #72: a Router was built with handlers of different kinds.
inline constexpr const char* MIXED_HANDLER_KINDS = "MIXED_HANDLER_KINDS";

// Saga / PM destinations — audit #64
inline constexpr const char* MISSING_DESTINATION_SEQUENCE =
    "MISSING_DESTINATION_SEQUENCE";

}  // namespace codes

/// Static human-readable messages — the value of ``message`` on every
/// error this client constructs.
namespace messages {

// Validation
inline constexpr const char* VALUE_NOT_POSITIVE = "value must be positive";
inline constexpr const char* VALUE_NOT_NON_NEGATIVE =
    "value must be non-negative";
inline constexpr const char* VALUE_EMPTY = "value must not be empty";
inline constexpr const char* COLLECTION_EMPTY = "collection must not be empty";
inline constexpr const char* ENTITY_NOT_FOUND = "entity not found";
inline constexpr const char* ENTITY_ALREADY_EXISTS = "entity already exists";
inline constexpr const char* STATUS_MISMATCH = "status does not match expected";
inline constexpr const char* STATUS_FORBIDDEN = "status is the forbidden value";
// PR #12 SEATS_IDENTICAL; canonical text from Py/Rs.
inline constexpr const char* SEATS_IDENTICAL =
    "current_seat and requested_seat are identical";

// Builder
inline constexpr const char* COMMAND_TYPE_URL_MISSING =
    "command type_url not set";
inline constexpr const char* COMMAND_PAYLOAD_MISSING =
    "command payload not set";
inline constexpr const char* COMMAND_SEQUENCE_MISSING =
    "sequence not set (call with_sequence)";

// Conversion
inline constexpr const char* ANY_TYPE_MISMATCH =
    "Any type_url does not match expected message type";
inline constexpr const char* ANY_DECODE_FAILED =
    "failed to decode Any payload into expected message type";
inline constexpr const char* PROTO_UUID_INVALID =
    "proto UUID bytes are not a valid 16-byte UUID";
inline constexpr const char* TIMESTAMP_PARSE_FAILED =
    "failed to parse RFC3339 timestamp";

// Transport / connection
inline constexpr const char* ENDPOINT_PARSE_FAILED =
    "failed to parse fallback endpoint";
inline constexpr const char* ENDPOINT_INVALID_URI = "endpoint URI is invalid";
inline constexpr const char* CONNECTION_FAILED = "connection failed";
inline constexpr const char* CONNECTION_FAILED_MAX_RETRIES =
    "connection failed after max retries";
inline constexpr const char* TRANSPORT_ERROR = "transport error";
inline constexpr const char* GRPC_ERROR = "grpc error";
inline constexpr const char* INVALID_TRANSPORT_MODE =
    "invalid transport mode env value";
inline constexpr const char* INVALID_PORT = "invalid port env value";

// Dispatch — common
inline constexpr const char* HANDLER_WRONG_RESPONSE_KIND =
    "handler returned wrong response kind";
inline constexpr const char* HANDLER_WRONG_REQUEST_KIND =
    "handler dispatched with wrong request kind";
inline constexpr const char* NO_HANDLER_REGISTERED =
    "no handler registered for the given (domain, type_url)";
inline constexpr const char* MISSING_COMMAND_BOOK = "missing command book";
inline constexpr const char* MISSING_COMMAND_PAGE = "missing command page";
inline constexpr const char* MISSING_COMMAND_PAYLOAD =
    "missing command payload";
inline constexpr const char* NOTIFICATION_DECODE_FAILED =
    "failed to decode Notification payload";
inline constexpr const char* REJECTION_NOTIFICATION_DECODE_FAILED =
    "failed to decode RejectionNotification payload";

// Dispatch — saga
inline constexpr const char* MISSING_SAGA_SOURCE = "missing saga source";
inline constexpr const char* EMPTY_SAGA_SOURCE = "empty saga source";
inline constexpr const char* MISSING_SAGA_EVENT_PAYLOAD =
    "missing event payload";
inline constexpr const char* SAGA_INVALID_TYPE_URL =
    "saga trigger has invalid type_url";
inline constexpr const char* SAGA_HANDLER_UNSUPPORTED_RETURN_TYPE =
    "saga handler returned unsupported type";

// Dispatch — process manager
inline constexpr const char* MISSING_PM_TRIGGER = "missing PM trigger";
inline constexpr const char* EMPTY_PM_TRIGGER = "empty PM trigger";
inline constexpr const char* MISSING_PM_EVENT_PAYLOAD =
    "missing event payload on PM trigger";
inline constexpr const char* PM_INVALID_TYPE_URL =
    "PM trigger has invalid type_url";
inline constexpr const char* PM_HANDLER_WRONG_RETURN_TYPE =
    "PM handler must return ProcessManagerResponse";

// Dispatch — upcaster
inline constexpr const char* UPCASTER_WRONG_RESPONSE_KIND =
    "upcaster handler returned non-Upcaster response";

// Build-time validation
inline constexpr const char* HANDLER_FIELD_EMPTY_STRING =
    "handler field must be a non-empty string";
inline constexpr const char* HANDLER_FIELD_EMPTY_LIST =
    "handler field must be a non-empty list";
inline constexpr const char* HANDLER_STATE_NOT_TYPE =
    "handler 'state' must be a type";
inline constexpr const char* HANDLER_UNKNOWN_KIND = "unknown handler kind";
inline constexpr const char* ROUTER_NO_HANDLERS =
    "no handlers registered on Router";
inline constexpr const char* DUPLICATE_COMMAND_HANDLER =
    "duplicate command handler registration for (domain, type_url)";
inline constexpr const char* MIXED_HANDLER_KINDS =
    "cannot mix handler kinds in one Router — all handlers must share a kind";

// Saga / PM destinations
inline constexpr const char* MISSING_DESTINATION_SEQUENCE =
    "no sequence for destination domain";

}  // namespace messages

/// Detail-map keys used in the ``details`` mapping on every error.
namespace keys {

inline constexpr const char* FIELD = "field";
inline constexpr const char* CONTEXT = "context";
inline constexpr const char* CAUSE = "cause";
inline constexpr const char* ENDPOINT = "endpoint";
inline constexpr const char* INPUT = "input";
inline constexpr const char* EXPECTED = "expected";
inline constexpr const char* ACTUAL = "actual";
inline constexpr const char* EXPECTED_KIND = "expected_kind";
inline constexpr const char* DOMAIN = "domain";
inline constexpr const char* TYPE_URL = "type_url";
inline constexpr const char* ROUTER_NAME = "router_name";
inline constexpr const char* HANDLER_CLASS = "handler_class";
inline constexpr const char* HANDLER_KIND = "handler_kind";
inline constexpr const char* ACTUAL_RETURN_TYPE = "actual_return_type";
inline constexpr const char* ENV_VAR = "env_var";
// Audit #72: paired with HANDLER_KIND for MIXED_HANDLER_KINDS errors.
inline constexpr const char* OTHER_KIND = "other_kind";

}  // namespace keys
}  // namespace angzarr::error_codes
