// Spec HIGH-1.3 + MED-1.8 + MED-4.10 — gRPC status translator parity
// with Py/Rs/Ja/Cs and Any-decode wrapping.

#include "angzarr/grpc_translator.hpp"

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

using namespace angzarr;

// ─── HIGH-1.3 — CommandRejectedError → status_code passthrough ───────────────

TEST(GrpcTranslator, PreconditionFailedRejection_MapsToFailedPrecondition) {
    auto e = CommandRejectedError::precondition_failed("CODE", "msg");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::FAILED_PRECONDITION);
    EXPECT_EQ(s.error_message(), "msg");
}

TEST(GrpcTranslator, InvalidArgumentRejection_MapsToInvalidArgument) {
    auto e = CommandRejectedError::invalid_argument("CODE", "msg");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST(GrpcTranslator, NotFoundRejection_MapsToNotFound) {
    auto e = CommandRejectedError::not_found("CODE", "msg");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST(GrpcTranslator, InvalidArgumentError_MapsToInvalidArgument) {
    InvalidArgumentError e("validation failed");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST(GrpcTranslator, InvalidTimestampError_MapsToInvalidArgument) {
    InvalidTimestampError e("bad timestamp");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
}

TEST(GrpcTranslator, ConnectionError_MapsToUnavailable) {
    ConnectionError e("can't reach");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST(GrpcTranslator, TransportError_MapsToUnavailable) {
    TransportError e("transport down");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::UNAVAILABLE);
}

TEST(GrpcTranslator, GenericClientError_MapsToInternal) {
    ClientError e("uncategorized");
    auto s = to_grpc_status(e);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::INTERNAL);
}

TEST(GrpcTranslator, ExceptionPtr_StdException_MapsToInternal) {
    std::exception_ptr eptr;
    try {
        throw std::runtime_error("uh oh");
    } catch (...) {
        eptr = std::current_exception();
    }
    auto s = to_grpc_status(eptr);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::INTERNAL);
    EXPECT_EQ(s.error_message(), "uh oh");
}

TEST(GrpcTranslator, ExceptionPtr_NullReturnsOk) {
    std::exception_ptr eptr;
    EXPECT_TRUE(to_grpc_status(eptr).ok());
}

TEST(GrpcTranslator, ExceptionPtr_PreservesCommandRejected) {
    std::exception_ptr eptr;
    try {
        throw CommandRejectedError::not_found("CODE", "missing");
    } catch (...) {
        eptr = std::current_exception();
    }
    auto s = to_grpc_status(eptr);
    EXPECT_EQ(s.error_code(), grpc::StatusCode::NOT_FOUND);
}

// ─── MED-1.8 / MED-4.10 — unpack_any_or_throw ────────────────────────────────

TEST(UnpackAnyOrThrow, ValidPayload_Succeeds) {
    google::protobuf::Timestamp ts;
    ts.set_seconds(42);
    google::protobuf::Any any;
    any.PackFrom(ts);

    google::protobuf::Timestamp out;
    EXPECT_NO_THROW(unpack_any_or_throw(any, &out));
    EXPECT_EQ(out.seconds(), 42);
}

TEST(UnpackAnyOrThrow, TypeMismatch_StampsAnyDecodeFailedCode) {
    google::protobuf::Any any;
    any.set_type_url("type.googleapis.com/some.Other");
    any.set_value("garbage");

    google::protobuf::Timestamp out;
    try {
        unpack_any_or_throw(any, &out);
        FAIL() << "expected throw";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::ANY_DECODE_FAILED);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::ANY_DECODE_FAILED);
        EXPECT_EQ(e.details().at("type_url"), "type.googleapis.com/some.Other");
    }
}

TEST(UnpackAnyOrThrow, NullOut_StampsAnyDecodeFailedCode) {
    google::protobuf::Any any;
    try {
        unpack_any_or_throw<google::protobuf::Timestamp>(any, nullptr);
        FAIL();
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::ANY_DECODE_FAILED);
    }
}
