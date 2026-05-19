// Mirrors Python's tests/test_command_rejected_error.py,
// tests/test_command_rejected_error_cover.py, and
// tests/test_error_defaults_mutation_kill.py — pins the cross-language
// CommandRejectedError envelope (status_code defaults, factory bindings,
// code/details preservation) and the default code of each error class.
//
// The cover-attachment tests (PY-RS-MISMATCH-1.1) are disabled because
// C++ CommandRejectedError lacks cover()/with_cover(). They are kept as
// `DISABLED_*` so they show up in test output for tracking.

#include <gtest/gtest.h>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

namespace {

using namespace angzarr;

// ============================================================================
// CommandRejectedError defaults
// ============================================================================

// Mirrors test_command_rejected_error_default_status_code_is_failed_precondition.
TEST(CommandRejectedErrorDefaults, DefaultStatusCode_IsFailedPrecondition) {
    CommandRejectedError err("rejected");
    EXPECT_EQ(err.status_code, grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_TRUE(err.is_precondition_failed());
    EXPECT_FALSE(err.is_invalid_argument());
    EXPECT_FALSE(err.is_not_found());
}

// Mirrors test_command_rejected_error_default_code_is_empty_string.
TEST(CommandRejectedErrorDefaults, DefaultCode_IsEmptyString) {
    CommandRejectedError err("rejected");
    EXPECT_EQ(err.code(), "");
}

// Mirrors test_command_rejected_error_preserves_message.
TEST(CommandRejectedErrorDefaults, PreservesMessage) {
    CommandRejectedError err("the precise message");
    EXPECT_STREQ(err.what(), "the precise message");
}

// ============================================================================
// CommandRejectedError factory bindings
// ============================================================================

// Mirrors test_precondition_failed_factory_status_code.
TEST(CommandRejectedErrorFactories, PreconditionFailed_BindsFailedPreconditionStatus) {
    auto err = CommandRejectedError::precondition_failed("X", "m");
    EXPECT_EQ(err.status_code, grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_TRUE(err.is_precondition_failed());
    EXPECT_EQ(err.code(), "X");
    EXPECT_STREQ(err.what(), "m");
}

// Mirrors test_invalid_argument_factory_status_code.
TEST(CommandRejectedErrorFactories, InvalidArgument_BindsInvalidArgumentStatus) {
    auto err = CommandRejectedError::invalid_argument("X", "m");
    EXPECT_EQ(err.status_code, grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_TRUE(err.is_invalid_argument());
    EXPECT_FALSE(err.is_precondition_failed());
    EXPECT_EQ(err.code(), "X");
}

// Mirrors test_not_found_factory_status_code.
TEST(CommandRejectedErrorFactories, NotFound_BindsNotFoundStatus) {
    auto err = CommandRejectedError::not_found("X", "m");
    EXPECT_EQ(err.status_code, grpc::StatusCode::NOT_FOUND);
    EXPECT_TRUE(err.is_not_found());
    EXPECT_FALSE(err.is_precondition_failed());
    EXPECT_FALSE(err.is_invalid_argument());
}

// Mirrors test_command_rejected_error_with_cover_preserves_code_and_details
// (details portion only — cover bit is skipped per PY-RS-MISMATCH-1.1).
TEST(CommandRejectedErrorFactories, DetailsPreservedVerbatim) {
    auto err = CommandRejectedError::precondition_failed(
        "STATUS_MISMATCH", "bad state", {{"field", "phase"}});
    EXPECT_EQ(err.details().at("field"), "phase");
    EXPECT_EQ(err.code(), "STATUS_MISMATCH");
    EXPECT_EQ(err.status_code, grpc::StatusCode::FAILED_PRECONDITION);
}

// ============================================================================
// ClientError defaults
// ============================================================================

// Mirrors test_client_error_default_code_is_empty_string.
TEST(ClientErrorDefaults, DefaultCode_IsEmpty) {
    ClientError err("msg");
    EXPECT_EQ(err.code(), "");
}

// Mirrors test_client_error_default_details_is_empty_dict.
TEST(ClientErrorDefaults, DefaultDetails_IsEmpty) {
    ClientError err("msg");
    EXPECT_TRUE(err.details().empty());
}

// ============================================================================
// ConnectionError defaults
// ============================================================================

// Mirrors test_connection_error_predicate_is_connection_error.
TEST(ConnectionErrorDefaults, Predicate_IsConnectionError) {
    ConnectionError err("oops");
    EXPECT_TRUE(err.is_connection_error());
}

// Mirrors test_connection_error_custom_message_honored.
TEST(ConnectionErrorDefaults, CustomMessage_Honored) {
    ConnectionError err("my custom message");
    EXPECT_STREQ(err.what(), "my custom message");
}

// ============================================================================
// TransportError defaults
// ============================================================================

// Mirrors test_transport_error_predicate_is_connection_error
// (transport errors are connection errors per the inheritance contract).
TEST(TransportErrorDefaults, Predicate_IsConnectionError) {
    TransportError err("transport failed");
    EXPECT_TRUE(err.is_connection_error());
}

// ============================================================================
// InvalidArgumentError + InvalidTimestampError
// ============================================================================

// Mirrors test_invalid_argument_error_predicate.
TEST(InvalidArgumentErrorDefaults, Predicate_IsInvalidArgument) {
    InvalidArgumentError err("bad arg");
    EXPECT_TRUE(err.is_invalid_argument());
}

// Mirrors test_invalid_timestamp_error_default_message — message arg honored.
TEST(InvalidTimestampErrorDefaults, CustomMessage_Honored) {
    InvalidTimestampError err("very bad timestamp");
    EXPECT_STREQ(err.what(), "very bad timestamp");
}

// ============================================================================
// PY-RS-MISMATCH-1.1: Cover attachment surface (DISABLED — C++ source lacks
// CommandRejectedError::cover() / with_cover()). Per the porting hard rule
// "DO NOT fix the source", these are kept as DISABLED_* so they appear in the
// inventory without flipping CI red. Re-enable by removing the DISABLED_
// prefix once C++ adds the surface.
// ============================================================================

TEST(CommandRejectedErrorCoverGap, DISABLED_Default_Cover_IsNullopt) {
    // CommandRejectedError err("rejected");
    // EXPECT_FALSE(err.cover().has_value());
}

TEST(CommandRejectedErrorCoverGap, DISABLED_WithCover_StampsCoverAndReturnsSelf) {
    // angzarr::Cover c; c.set_domain("player");
    // CommandRejectedError err("rejected");
    // auto& returned = err.with_cover(c);
    // EXPECT_EQ(&returned, &err);
    // EXPECT_TRUE(err.cover().has_value());
    // EXPECT_EQ(err.cover()->domain(), "player");
}

}  // namespace
