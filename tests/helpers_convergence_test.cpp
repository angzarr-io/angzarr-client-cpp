// Spec MED-2.3 / MED-2.4 / MED-2.6 / MED-2.x convergence — helper
// surface parity with Py/Rs/Go.

#include "angzarr/helpers.hpp"

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <gtest/gtest.h>

#include <string>

#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"

using namespace angzarr;

namespace {
EventBook book_with(const std::string& dom = "orders",
                    const std::string& root_bytes = std::string(16, 'r'),
                    const std::string& ed = "") {
    EventBook b;
    auto* cover = b.mutable_cover();
    cover->set_domain(dom);
    if (!root_bytes.empty()) cover->mutable_root()->set_value(root_bytes);
    if (!ed.empty()) cover->mutable_edition()->set_name(ed);
    return b;
}
}  // namespace

// ─── MED-2.3 cache_key ────────────────────────────────────────────────────────

TEST(Helpers_Med23, CacheKey_Format_EditionDomainRoot) {
    auto b = book_with("orders", std::string(16, '\xab'), "v2");
    // 16 bytes of 0xab → 32-char lowercase hex.
    EXPECT_EQ(helpers::cache_key(b),
              "v2:orders:abababababababababababababababab");
}

TEST(Helpers_Med23, CacheKey_EmptyEdition_LeadsWithColon) {
    auto b = book_with("orders", std::string(16, '\x00'));
    EXPECT_EQ(helpers::cache_key(b), ":orders:00000000000000000000000000000000");
}

TEST(Helpers_Med23, CacheKey_UnknownDomain_WhenMissing) {
    EventBook b;  // no cover at all
    EXPECT_EQ(helpers::cache_key(b), ":unknown:");
}

// ─── MED-2.4 routing_key ──────────────────────────────────────────────────────

TEST(Helpers_Med24, RoutingKey_Format_DomainColonRoot) {
    auto b = book_with("orders", std::string(16, '\x01'));
    EXPECT_EQ(helpers::routing_key(b), "orders:01010101010101010101010101010101");
}

TEST(Helpers_Med24, RoutingKey_NoRoot_EmptyAfterColon) {
    EventBook b;
    b.mutable_cover()->set_domain("orders");
    EXPECT_EQ(helpers::routing_key(b), "orders:");
}

// ─── MED-2.6 edition ──────────────────────────────────────────────────────────

TEST(Helpers_Med26, Edition_PresentNonEmpty_ReturnsValue) {
    auto b = book_with("o", std::string(16, 'r'), "v3");
    auto e = helpers::edition(b);
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(*e, "v3");
}

TEST(Helpers_Med26, Edition_EmptyString_ReturnsNullopt) {
    EventBook b;
    b.mutable_cover()->mutable_edition()->set_name("");
    EXPECT_FALSE(helpers::edition(b).has_value());
}

TEST(Helpers_Med26, Edition_Missing_ReturnsNullopt) {
    EventBook b;
    b.mutable_cover()->set_domain("d");
    EXPECT_FALSE(helpers::edition(b).has_value());
}

TEST(Helpers_Med26, Edition_NoCover_ReturnsNullopt) {
    EventBook b;
    EXPECT_FALSE(helpers::edition(b).has_value());
}

// ─── full_type_name + full_type_url + type_matches + try_unpack ───────────────

TEST(Helpers_MedTypes, FullTypeNameOfTimestamp_IsCanonical) {
    EXPECT_EQ(helpers::full_type_name<google::protobuf::Timestamp>(),
              "google.protobuf.Timestamp");
}

TEST(Helpers_MedTypes, FullTypeUrlOfTimestamp_IsPrefixed) {
    EXPECT_EQ(helpers::full_type_url<google::protobuf::Timestamp>(),
              "type.googleapis.com/google.protobuf.Timestamp");
}

TEST(Helpers_MedTypes, TypeMatches_PositiveCase) {
    google::protobuf::Any any;
    google::protobuf::Timestamp ts;
    any.PackFrom(ts);
    EXPECT_TRUE(helpers::type_matches<google::protobuf::Timestamp>(any));
}

TEST(Helpers_MedTypes, TypeMatches_Negative) {
    google::protobuf::Any any;
    any.set_type_url("type.googleapis.com/something.Else");
    EXPECT_FALSE(helpers::type_matches<google::protobuf::Timestamp>(any));
}

TEST(Helpers_MedTypes, TryUnpack_PositiveReturnsValue) {
    google::protobuf::Timestamp ts;
    ts.set_seconds(99);
    google::protobuf::Any any;
    any.PackFrom(ts);
    auto opt = helpers::try_unpack<google::protobuf::Timestamp>(any);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->seconds(), 99);
}

TEST(Helpers_MedTypes, TryUnpack_NegativeReturnsNullopt) {
    google::protobuf::Any any;
    any.set_type_url("type.googleapis.com/something.Else");
    EXPECT_FALSE(helpers::try_unpack<google::protobuf::Timestamp>(any).has_value());
}

// ─── parse_timestamp ──────────────────────────────────────────────────────────

TEST(Helpers_ParseTimestamp, FullGrammarAccepted) {
    EXPECT_NO_THROW(helpers::parse_timestamp("2024-01-15T10:30:00.123Z"));
    EXPECT_NO_THROW(helpers::parse_timestamp("2024-01-15T10:30:00+02:00"));
    EXPECT_NO_THROW(helpers::parse_timestamp("2024-01-15T10:30:00Z"));
}

TEST(Helpers_ParseTimestamp, BadGrammar_StampsParseFailedCode) {
    try {
        helpers::parse_timestamp("not-a-timestamp");
        FAIL();
    } catch (const InvalidTimestampError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::TIMESTAMP_PARSE_FAILED);
        EXPECT_EQ(std::string(e.what()), error_codes::messages::TIMESTAMP_PARSE_FAILED);
    }
}
