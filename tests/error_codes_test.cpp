// Tests for cross-language error-code parity (Python: error_codes/{codes,messages,keys}.py
// / Rust: error_codes.rs). Audit findings #59, #40, #64, #75-#86.
//
// Asserts the value of each constant matches the Python/Rust value
// character-for-character — that's the parity contract.

#include <gtest/gtest.h>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

using namespace angzarr;
using namespace angzarr::error_codes;

TEST(ErrorCodes, AuditCodesMatchPythonAndRust) {
    // Audit #40 — symmetric INVALID_TRANSPORT_MODE / INVALID_PORT
    EXPECT_STREQ(codes::INVALID_TRANSPORT_MODE, "INVALID_TRANSPORT_MODE");
    EXPECT_STREQ(codes::INVALID_PORT, "INVALID_PORT");

    // Audit #64 — destination stamping missing sequence
    EXPECT_STREQ(codes::MISSING_DESTINATION_SEQUENCE, "MISSING_DESTINATION_SEQUENCE");

    // Builder
    EXPECT_STREQ(codes::COMMAND_TYPE_URL_MISSING, "COMMAND_TYPE_URL_MISSING");
    EXPECT_STREQ(codes::COMMAND_PAYLOAD_MISSING, "COMMAND_PAYLOAD_MISSING");
    EXPECT_STREQ(codes::COMMAND_SEQUENCE_MISSING, "COMMAND_SEQUENCE_MISSING");

    // Validation
    EXPECT_STREQ(codes::VALUE_NOT_POSITIVE, "VALUE_NOT_POSITIVE");
    EXPECT_STREQ(codes::VALUE_NOT_NON_NEGATIVE, "VALUE_NOT_NON_NEGATIVE");
    EXPECT_STREQ(codes::ENTITY_NOT_FOUND, "ENTITY_NOT_FOUND");
}

TEST(ErrorCodes, MessagesMatchPythonAndRust) {
    EXPECT_STREQ(messages::INVALID_TRANSPORT_MODE, "invalid transport mode env value");
    EXPECT_STREQ(messages::INVALID_PORT, "invalid port env value");
    EXPECT_STREQ(messages::MISSING_DESTINATION_SEQUENCE, "no sequence for destination domain");
    EXPECT_STREQ(messages::COMMAND_SEQUENCE_MISSING, "sequence not set (call with_sequence)");
}

TEST(ErrorCodes, KeysMatchPythonAndRust) {
    EXPECT_STREQ(keys::FIELD, "field");
    EXPECT_STREQ(keys::DOMAIN, "domain");
    EXPECT_STREQ(keys::TYPE_URL, "type_url");
    EXPECT_STREQ(keys::ENV_VAR, "env_var");
    EXPECT_STREQ(keys::OTHER_KIND, "other_kind");
}

TEST(StructuredClientError, CarriesCodeAndDetails) {
    // Audit #59: every ClientError carries a static message + a
    // SCREAMING_SNAKE code + a details map. Construction supplies
    // each; the runtime preserves all three for cross-language
    // assertions and i18n keys.
    InvalidArgumentError err(messages::MISSING_DESTINATION_SEQUENCE,
                             codes::MISSING_DESTINATION_SEQUENCE,
                             {{keys::DOMAIN, "fulfillment"}});

    EXPECT_STREQ(err.what(), messages::MISSING_DESTINATION_SEQUENCE);
    EXPECT_EQ(err.code(), codes::MISSING_DESTINATION_SEQUENCE);
    ASSERT_TRUE(err.details().count(keys::DOMAIN));
    EXPECT_EQ(err.details().at(keys::DOMAIN), "fulfillment");
}

TEST(StructuredClientError, EmptyDetailsByDefault) {
    InvalidArgumentError err("a message");
    EXPECT_TRUE(err.details().empty());
    EXPECT_EQ(err.code(), "");
}

TEST(StructuredClientError, CommandRejectedFactoryBindsCodeAndStatus) {
    // Audit #59: the factory methods bind both the gRPC status and
    // the structured (code, message, details). Mirrors Python's
    // ``CommandRejectedError.precondition_failed`` and Rust's
    // ``CommandRejectedError::precondition_failed``.
    auto err = CommandRejectedError::precondition_failed(
        codes::ENTITY_NOT_FOUND, messages::ENTITY_NOT_FOUND,
        {{keys::FIELD, "player.id"}});

    EXPECT_EQ(err.code(), codes::ENTITY_NOT_FOUND);
    EXPECT_EQ(err.status_code, grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_TRUE(err.is_precondition_failed());
    EXPECT_FALSE(err.is_invalid_argument());
    ASSERT_TRUE(err.details().count(keys::FIELD));
    EXPECT_EQ(err.details().at(keys::FIELD), "player.id");
}

TEST(StructuredClientError, CommandRejectedFactoryInvalidArgument) {
    auto err = CommandRejectedError::invalid_argument(
        codes::VALUE_NOT_POSITIVE, messages::VALUE_NOT_POSITIVE,
        {{keys::FIELD, "amount"}});

    EXPECT_EQ(err.code(), codes::VALUE_NOT_POSITIVE);
    EXPECT_EQ(err.status_code, grpc::StatusCode::INVALID_ARGUMENT);
    EXPECT_TRUE(err.is_invalid_argument());
    EXPECT_FALSE(err.is_precondition_failed());
}

TEST(StructuredClientError, CommandRejectedFactoryNotFound) {
    auto err = CommandRejectedError::not_found(
        codes::ENTITY_NOT_FOUND, messages::ENTITY_NOT_FOUND);

    EXPECT_EQ(err.code(), codes::ENTITY_NOT_FOUND);
    EXPECT_EQ(err.status_code, grpc::StatusCode::NOT_FOUND);
    EXPECT_TRUE(err.is_not_found());
}
