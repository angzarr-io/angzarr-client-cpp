// Tests for the testing namespace — uuid_for, make_timestamp,
// make_cover, etc.
//
// Audit findings #50, #99, #105: these helpers were previously
// unimplemented in C++, so cross-language tests couldn't materialize
// the same fixtures from a string seed. Bringing them in lets C++
// scenarios reuse the deterministic UUID v5 byte strings produced by
// the Python and Rust tests.

#include <gtest/gtest.h>

#include <string>

#include "angzarr/testing.hpp"

using namespace angzarr;
using namespace angzarr::testing;

// ---------------------------------------------------------------------------
// uuid_for / uuid_for_default — audit #50, parity with Python and
// Rust (RFC 4122 v5 over SHA-1, same DEFAULT_TEST_NAMESPACE).
// ---------------------------------------------------------------------------

TEST(UuidForDefault, DefaultNamespaceMatchesPythonAndRust) {
    // Python: DEFAULT_TEST_NAMESPACE = "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
    // Rust:   Uuid::from_u128(0xa1b2c3d4_e5f6_7890_abcd_ef1234567890u128)
    EXPECT_EQ(default_test_namespace_str(), "a1b2c3d4-e5f6-7890-abcd-ef1234567890");
}

TEST(UuidForDefault, ProducesCanonicalRustAndPythonByteStringForTestRoot) {
    // Pinned in Rust src/testing/uuid.rs:53: uuid_for("test-root",
    // DEFAULT_TEST_NAMESPACE).hex == "1596515c79c357278c0e26e3ccf145c6"
    auto bytes = uuid_for("test-root");
    ASSERT_EQ(bytes.size(), 16u);

    auto to_hex = [](const std::string& s) {
        std::string out;
        out.reserve(s.size() * 2);
        const char* hex = "0123456789abcdef";
        for (unsigned char c : s) {
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0f]);
        }
        return out;
    };
    EXPECT_EQ(to_hex(bytes), "1596515c79c357278c0e26e3ccf145c6");
}

TEST(UuidForDefault, IsDeterministic) {
    EXPECT_EQ(uuid_for("player-alice"), uuid_for("player-alice"));
}

TEST(UuidForDefault, DistinctNamesYieldDistinctBytes) {
    EXPECT_NE(uuid_for("alpha"), uuid_for("beta"));
}

TEST(UuidStrForDefault, FormatsAsCanonical8_4_4_4_12) {
    auto s = uuid_str_for("player-alice");
    EXPECT_EQ(s.size(), 36u);
    EXPECT_EQ(s[8], '-');
    EXPECT_EQ(s[13], '-');
    EXPECT_EQ(s[18], '-');
    EXPECT_EQ(s[23], '-');
}

TEST(UuidStrForDefault, MatchesUuidForByteString) {
    auto bytes = uuid_for("player-alice");
    auto str = uuid_str_for("player-alice");

    // First 8 hex digits of the string must equal the hex of the
    // first 4 bytes of the byte string.
    char first[9];
    std::snprintf(first, sizeof(first), "%02x%02x%02x%02x",
                  static_cast<unsigned char>(bytes[0]), static_cast<unsigned char>(bytes[1]),
                  static_cast<unsigned char>(bytes[2]), static_cast<unsigned char>(bytes[3]));
    EXPECT_EQ(std::string(first, 8), str.substr(0, 8));
}

TEST(UuidForExplicitNamespace, DifferentNamespaceYieldsDifferentBytes) {
    std::string ns1_bytes(16, 'A');
    std::string ns2_bytes(16, 'B');
    EXPECT_NE(uuid_for("name", ns1_bytes), uuid_for("name", ns2_bytes));
}

// ---------------------------------------------------------------------------
// make_timestamp / make_cover / make_event_book / make_command_book
// (audit #99 — testing helpers parity).
// ---------------------------------------------------------------------------

TEST(MakeTimestamp, ReturnsNonZeroProtoTimestamp) {
    auto ts = make_timestamp();
    EXPECT_GT(ts.seconds(), 0);
}

TEST(MakeCover, SetsDomainAndRoot) {
    auto root = uuid_for("player-1");
    auto cover = make_cover("inventory", root, "corr-id-99");
    EXPECT_EQ(cover.domain(), "inventory");
    EXPECT_EQ(cover.root().value(), root);
    EXPECT_EQ(cover.correlation_id(), "corr-id-99");
}

TEST(MakeEventBook, WrapsCoverAndPagesAndAutoFillsNextSequence) {
    auto root = uuid_for("order-1");
    auto cover = make_cover("orders", root);

    EventPage page1;
    page1.mutable_header()->set_sequence(0);
    EventPage page2;
    page2.mutable_header()->set_sequence(1);

    auto book = make_event_book(cover, {page1, page2});
    EXPECT_EQ(book.cover().domain(), "orders");
    EXPECT_EQ(book.pages_size(), 2);
    EXPECT_EQ(book.next_sequence(), 2u);
}

TEST(MakeCommandBook, WrapsCoverAndCommandIntoSinglePage) {
    auto root = uuid_for("order-1");
    auto cover = make_cover("orders", root);
    google::protobuf::Any cmd;
    cmd.set_type_url("type.googleapis.com/test.Create");

    auto book = make_command_book(cover, cmd, 5);
    EXPECT_EQ(book.cover().domain(), "orders");
    ASSERT_EQ(book.pages_size(), 1);
    EXPECT_EQ(book.pages(0).header().sequence(), 5u);
    EXPECT_EQ(book.pages(0).command().type_url(), "type.googleapis.com/test.Create");
}
