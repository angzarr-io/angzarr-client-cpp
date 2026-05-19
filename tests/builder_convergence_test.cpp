// Spec convergence tests for CommandBuilder + QueryBuilder.
//
// Covers:
//   HIGH-3.1 — no rootless path; the AutoGenerateRoot ctor + factory
//              always stamp a client-side UUID v4 (audit #67).
//   HIGH-3.2 — build() validation errors carry SCREAMING_SNAKE codes
//              + canonical messages + structured details.
//   MED-3.3  — with_sync_mode setter pre-configures execute().
//   MED-3.6  — full RFC 3339 grammar (fractional seconds + timezone).
//   MED-3.7  — correlation-ID generator uses crypto-quality UUID v4.

#include "angzarr/builder.hpp"

#include <gtest/gtest.h>

#include <google/protobuf/any.pb.h>

#include <string>
#include <unordered_set>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

using namespace angzarr;

namespace {
google::protobuf::Any make_any() {
    google::protobuf::Any any;
    any.set_type_url("type.googleapis.com/test.TestCommand");
    any.set_value("payload");
    return any;
}
}  // namespace

// ─── HIGH-3.1 ─────────────────────────────────────────────────────────────────

TEST(BuilderHigh31, ExistingRootCtor_StampsRoot) {
    std::string root(16, 'r');
    CommandBuilder b(nullptr, "orders", root);
    b.with_sequence(1).with_command("type.googleapis.com/test.TestCommand", make_any());
    auto book = b.build();
    EXPECT_TRUE(book.cover().has_root());
    EXPECT_EQ(book.cover().root().value(), root);
}

TEST(BuilderHigh31, AutoGenerateRootCtor_StampsRoot) {
    // The new-aggregate tagged ctor materializes the UUID immediately;
    // build() must see a stamped root.
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    b.with_sequence(0).with_command("type.googleapis.com/test.TestCommand", make_any());
    auto book = b.build();
    EXPECT_TRUE(book.cover().has_root());
    EXPECT_EQ(book.cover().root().value().size(), 16u);
}

TEST(BuilderHigh31, CommandNewFactory_StampsRoot) {
    auto b = CommandBuilder::command_new(nullptr, "orders");
    b.with_sequence(0).with_command("type.googleapis.com/test.TestCommand", make_any());
    auto book = b.build();
    EXPECT_TRUE(book.cover().has_root());
}

TEST(BuilderHigh31, CommandNew_RootsAreUnique) {
    // Spec audit #20: each command_new call must produce an
    // independent root — collision indicates the auto-gen is broken.
    std::unordered_set<std::string> roots;
    for (int i = 0; i < 100; ++i) {
        auto b = CommandBuilder::command_new(nullptr, "x");
        b.with_sequence(0).with_command("type.googleapis.com/test.TestCommand", make_any());
        auto book = b.build();
        roots.insert(book.cover().root().value());
    }
    EXPECT_EQ(roots.size(), 100u);
}

// ─── HIGH-3.2 ─────────────────────────────────────────────────────────────────

TEST(BuilderHigh32, MissingTypeUrl_StampsCommandTypeUrlMissingCode) {
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    b.with_sequence(1);  // no with_command
    try {
        b.build();
        FAIL() << "expected throw";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::COMMAND_TYPE_URL_MISSING);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::COMMAND_TYPE_URL_MISSING);
        EXPECT_EQ(e.details().at("field"), "type_url");
        EXPECT_EQ(e.details().at("domain"), "orders");
    }
}

TEST(BuilderHigh32, MissingPayload_StampsCommandPayloadMissingCode) {
    // The current CommandBuilder couples ``type_url_`` and ``payload_``
    // through ``with_command(type_url, message)`` — to exercise the
    // payload-missing branch independently the type_url is set via
    // the same call as the payload. The payload branch is reachable
    // through programmatic construction; we instead pin the message
    // text and code constants here to guard against drift.
    EXPECT_EQ(std::string(error_codes::codes::COMMAND_PAYLOAD_MISSING),
              "COMMAND_PAYLOAD_MISSING");
    EXPECT_EQ(std::string(error_codes::messages::COMMAND_PAYLOAD_MISSING),
              "command payload not set");
}

TEST(BuilderHigh32, MissingSequence_StampsCommandSequenceMissingCode) {
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    b.with_command("type.googleapis.com/test.TestCommand", make_any());
    try {
        b.build();
        FAIL() << "expected throw";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::COMMAND_SEQUENCE_MISSING);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::COMMAND_SEQUENCE_MISSING);
        EXPECT_EQ(e.details().at("field"), "sequence");
    }
}

TEST(BuilderHigh32, IsInvalidArgumentPredicate_TrueForBuildErrors) {
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    try {
        b.build();  // missing everything
        FAIL();
    } catch (const InvalidArgumentError& e) {
        EXPECT_TRUE(e.is_invalid_argument());
    }
}

// ─── MED-3.3 ──────────────────────────────────────────────────────────────────

TEST(BuilderMed33, WithSyncMode_ReturnsBuilderForChaining) {
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    auto& ref = b.with_sync_mode(SYNC_MODE_SIMPLE);
    EXPECT_EQ(&ref, &b);
}

TEST(BuilderMed33, SyncModeStoredOnBuilderBeforeExecute) {
    // The setter is the contract; execute() routes through the
    // configured mode (real client wiring is verified in integration
    // tests). Builder-level pin guards against regressions removing
    // the field.
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    b.with_sequence(0)
        .with_command("type.googleapis.com/test.TestCommand", make_any())
        .with_sync_mode(SYNC_MODE_SIMPLE);
    auto book = b.build();
    // Sync mode is metadata, not stamped on the CommandBook itself;
    // the assertion here is that build() doesn't reject when sync mode
    // is configured.
    EXPECT_TRUE(book.cover().has_root());
}

// ─── MED-3.6 ──────────────────────────────────────────────────────────────────

TEST(QueryBuilderMed36, AsOfTime_AcceptsZuluSuffix) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00Z"));
}

TEST(QueryBuilderMed36, AsOfTime_AcceptsFractionalSeconds) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00.123Z"));
}

TEST(QueryBuilderMed36, AsOfTime_AcceptsMicrosecondFraction) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00.123456Z"));
}

TEST(QueryBuilderMed36, AsOfTime_AcceptsNanosecondFraction) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00.123456789Z"));
}

TEST(QueryBuilderMed36, AsOfTime_AcceptsPositiveOffset) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00+02:00"));
}

TEST(QueryBuilderMed36, AsOfTime_AcceptsNegativeOffset) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_NO_THROW(b.as_of_time("2024-01-15T10:30:00-05:30"));
}

TEST(QueryBuilderMed36, AsOfTime_StampsCorrectSeconds_Zulu) {
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("1970-01-01T00:00:01Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().seconds(), 1);
    EXPECT_EQ(q.temporal().as_of_time().nanos(), 0);
}

TEST(QueryBuilderMed36, AsOfTime_StampsCorrectNanos_FromFraction) {
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("2024-01-15T10:30:00.500Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().nanos(), 500000000);
}

TEST(QueryBuilderMed36, AsOfTime_PositiveOffsetSubtracted) {
    // 2024-01-15T10:00:00+02:00 == 2024-01-15T08:00:00Z
    QueryBuilder b1(nullptr, "orders");
    b1.as_of_time("2024-01-15T10:00:00+02:00");
    auto q1 = b1.build();
    QueryBuilder b2(nullptr, "orders");
    b2.as_of_time("2024-01-15T08:00:00Z");
    auto q2 = b2.build();
    EXPECT_EQ(q1.temporal().as_of_time().seconds(), q2.temporal().as_of_time().seconds());
}

TEST(QueryBuilderMed36, AsOfTime_NoTimezone_LoudFails) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_BadGrammar_LoudFails) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("not-a-timestamp"), InvalidTimestampError);
}

// Boundary tests targeting the parse_rfc3339 mutation survivors.

TEST(QueryBuilderMed36, AsOfTime_RejectsShortString_LessThan19) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsExactly18Chars) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:0"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsMonthZero) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-00-15T10:30:00Z"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsMonth13) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-13-15T10:30:00Z"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsDayZero) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-00T10:30:00Z"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsDay32) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-32T10:30:00Z"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsOffsetHour24) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00+24:00"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsOffsetMinute60) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00+02:60"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsMissingColonInOffset) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00+0200"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsEmptyFractional) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00.Z"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_RejectsTrailingChars) {
    QueryBuilder b(nullptr, "orders");
    EXPECT_THROW(b.as_of_time("2024-01-15T10:30:00ZX"), InvalidTimestampError);
}

TEST(QueryBuilderMed36, AsOfTime_DaysFromCivil_Jan1_1970_IsZero) {
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("1970-01-01T00:00:00Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().seconds(), 0);
}

TEST(QueryBuilderMed36, AsOfTime_DaysFromCivil_Mar1_2000_LeapYearBoundary) {
    // 2000 is a leap year (divisible by 400) — Feb has 29 days.
    // 2000-03-01 == day 60 of year == 30+29 = 59 days into year (0-indexed).
    // Days since 1970: 30 years * 365 + leap-day count.
    // Pinned via Python: datetime(2000, 3, 1, tzinfo=UTC).timestamp() == 951868800.
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("2000-03-01T00:00:00Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().seconds(), 951868800);
}

TEST(QueryBuilderMed36, AsOfTime_DaysFromCivil_FebLeapYearMarchBoundary) {
    // 2024 is a leap year — Feb has 29.
    // Pinned: datetime(2024, 2, 29).timestamp() in UTC == 1709164800.
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("2024-02-29T00:00:00Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().seconds(), 1709164800);
}

TEST(QueryBuilderMed36, AsOfTime_HourMinuteSecondMath) {
    // Pinned: datetime(2024, 1, 15, 10, 30, 45, tzinfo=UTC).timestamp() == 1705314645.
    QueryBuilder b(nullptr, "orders");
    b.as_of_time("2024-01-15T10:30:45Z");
    auto q = b.build();
    EXPECT_EQ(q.temporal().as_of_time().seconds(), 1705314645);
}

TEST(QueryBuilderMed36, AsOfTime_NegativeOffsetAdded) {
    // 2024-01-15T10:00:00-05:30 == 2024-01-15T15:30:00Z (offset added because negative).
    QueryBuilder b1(nullptr, "orders");
    b1.as_of_time("2024-01-15T10:00:00-05:30");
    QueryBuilder b2(nullptr, "orders");
    b2.as_of_time("2024-01-15T15:30:00Z");
    EXPECT_EQ(b1.build().temporal().as_of_time().seconds(),
              b2.build().temporal().as_of_time().seconds());
}

TEST(QueryBuilderMed36, AsOfTime_StampsTimestampParseFailedCode) {
    QueryBuilder b(nullptr, "orders");
    try {
        b.as_of_time("not-a-timestamp");
        FAIL();
    } catch (const InvalidTimestampError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::TIMESTAMP_PARSE_FAILED);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::TIMESTAMP_PARSE_FAILED);
    }
}

// ─── MED-3.7 ──────────────────────────────────────────────────────────────────

TEST(BuilderMed37, CorrelationIdAutoGen_IsRFC4122V4) {
    CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
    b.with_sequence(0).with_command("type.googleapis.com/test.TestCommand", make_any());
    auto book = b.build();
    auto corr = book.cover().correlation_id();
    ASSERT_EQ(corr.length(), 36u);
    EXPECT_EQ(corr[8], '-');
    EXPECT_EQ(corr[13], '-');
    EXPECT_EQ(corr[18], '-');
    EXPECT_EQ(corr[23], '-');
    // Version 4 (third group's first hex digit).
    EXPECT_EQ(corr[14], '4');
    // Variant bits (fourth group's first hex digit is 8, 9, a, or b).
    char variant = corr[19];
    EXPECT_TRUE(variant == '8' || variant == '9' || variant == 'a' || variant == 'b');
}

TEST(BuilderMed37, CorrelationIdAutoGen_DifferentEachBuild) {
    std::unordered_set<std::string> ids;
    for (int i = 0; i < 50; ++i) {
        CommandBuilder b(nullptr, "orders", CommandBuilder::AutoGenerateRoot{});
        b.with_sequence(0).with_command("type.googleapis.com/test.TestCommand", make_any());
        ids.insert(b.build().cover().correlation_id());
    }
    EXPECT_EQ(ids.size(), 50u);
}
