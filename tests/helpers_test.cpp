// Tests for the helpers free functions ported from Python/Rust.
//
// Mirrors the Python suite in tests/test_helpers.py for the audit
// findings the C++ client is catching up on: #48 (to_uuid_text), #49
// (divergence_for returns optional), #55 (idempotency_key returns
// optional), and the destination_map / type_url helpers introduced
// during the cross-language parity sweep.

#include <google/protobuf/any.pb.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "angzarr/helpers.hpp"
#include "angzarr/proto_aliases.hpp"

using namespace angzarr;

// =============================================================================
// divergence_for — audit #49
// =============================================================================

class DivergenceForTest : public ::testing::Test {};

TEST_F(DivergenceForTest, ReturnsSequenceWhenFound) {
    Edition ed;
    ed.set_name("test");
    auto* d = ed.add_divergences();
    d->set_domain("orders");
    d->set_sequence(42);

    auto result = helpers::divergence_for(&ed, "orders");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42u);
}

TEST_F(DivergenceForTest, ReturnsNulloptWhenNotFound) {
    // Audit #49: shifted from a -1 sentinel to std::optional<uint32_t>
    // to match Rust's Option<u32> and Python's Optional[int]. Stops
    // conflating "missing" with the impossible "negative-sequence" case.
    Edition ed;
    ed.set_name("test");
    auto* d = ed.add_divergences();
    d->set_domain("orders");
    d->set_sequence(42);

    auto result = helpers::divergence_for(&ed, "inventory");
    EXPECT_FALSE(result.has_value());
}

TEST_F(DivergenceForTest, ReturnsNulloptForNullEdition) {
    auto result = helpers::divergence_for(nullptr, "orders");
    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// idempotency_key — audit #55
// =============================================================================

class IdempotencyKeyTest : public ::testing::Test {};

TEST_F(IdempotencyKeyTest, BuildsCompositeKeyFromSourceCover) {
    // Format: "{edition}:{domain}:{root_hex}:{source_seq}"
    AngzarrDeferredSequence deferred;
    auto* source = deferred.mutable_source();
    source->set_domain("orders");
    source->mutable_edition()->set_name("angzarr");
    std::string root_bytes(16, '\x01');  // 16 0x01 bytes
    source->mutable_root()->set_value(root_bytes);
    deferred.set_source_seq(7);

    auto key = helpers::idempotency_key(&deferred);
    ASSERT_TRUE(key.has_value());
    // 16 0x01 bytes -> 32 char hex "01" * 16
    std::string expected_hex(32, '0');
    for (size_t i = 1; i < 32; i += 2) expected_hex[i] = '1';
    EXPECT_EQ(*key, std::string("angzarr:orders:") + expected_hex + ":7");
}

TEST_F(IdempotencyKeyTest, ReturnsNulloptWithoutSourceCover) {
    // Audit #55: a malformed wire input is a missing-key signal, not
    // a panic. Mirrors Python returning None and Rust returning Err.
    AngzarrDeferredSequence deferred;
    deferred.set_source_seq(42);  // source field intentionally absent

    auto key = helpers::idempotency_key(&deferred);
    EXPECT_FALSE(key.has_value());
}

TEST_F(IdempotencyKeyTest, ReturnsNulloptForNull) {
    auto key = helpers::idempotency_key(nullptr);
    EXPECT_FALSE(key.has_value());
}

TEST_F(IdempotencyKeyTest, EmptyEditionNameProducesEmptySegment) {
    // Mirrors Python's test_idempotency_key_no_edition.
    AngzarrDeferredSequence deferred;
    auto* source = deferred.mutable_source();
    source->set_domain("players");
    // No edition set
    std::string root_bytes(16, '\xab');
    source->mutable_root()->set_value(root_bytes);
    deferred.set_source_seq(42);

    auto key = helpers::idempotency_key(&deferred);
    ASSERT_TRUE(key.has_value());
    std::string expected_hex;
    for (int i = 0; i < 16; ++i) expected_hex += "ab";
    EXPECT_EQ(*key, std::string(":players:") + expected_hex + ":42");
}

// =============================================================================
// to_uuid_text / bytes_to_uuid_text — audit #48
// =============================================================================

class BytesToUuidTextTest : public ::testing::Test {};

TEST_F(BytesToUuidTextTest, SixteenByteInputFormatsAsCanonicalUuid) {
    // 16-byte input formats as 8-4-4-4-12 canonical UUID.
    // Bytes: 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 00
    std::string bytes;
    bytes += static_cast<char>(0x11);
    bytes += static_cast<char>(0x22);
    bytes += static_cast<char>(0x33);
    bytes += static_cast<char>(0x44);
    bytes += static_cast<char>(0x55);
    bytes += static_cast<char>(0x66);
    bytes += static_cast<char>(0x77);
    bytes += static_cast<char>(0x88);
    bytes += static_cast<char>(0x99);
    bytes += static_cast<char>(0xaa);
    bytes += static_cast<char>(0xbb);
    bytes += static_cast<char>(0xcc);
    bytes += static_cast<char>(0xdd);
    bytes += static_cast<char>(0xee);
    bytes += static_cast<char>(0xff);
    bytes += static_cast<char>(0x00);

    EXPECT_EQ(helpers::bytes_to_uuid_text(bytes),
              "11223344-5566-7788-99aa-bbccddeeff00");
}

TEST_F(BytesToUuidTextTest, NonSixteenByteInputFallsBackToHex) {
    std::string bytes;
    bytes += static_cast<char>(0xab);
    bytes += static_cast<char>(0xcd);
    bytes += static_cast<char>(0xef);
    EXPECT_EQ(helpers::bytes_to_uuid_text(bytes), "abcdef");
}

TEST_F(BytesToUuidTextTest, EmptyBytesReturnsEmpty) {
    EXPECT_EQ(helpers::bytes_to_uuid_text(""), "");
}

// =============================================================================
// helpers::domain — audit #54 (fall back when cover is missing or domain is empty)
// =============================================================================

class DomainAccessorTest : public ::testing::Test {};

TEST_F(DomainAccessorTest, FallsBackWhenCoverMissing) {
    EventBook book;
    EXPECT_EQ(helpers::domain(book), constants::UNKNOWN_DOMAIN);
}

TEST_F(DomainAccessorTest, FallsBackWhenCoverDomainEmpty) {
    // Audit #54 (Rust e6d6ee2 / Python parity): empty-domain covers
    // arise from partially-constructed test fixtures and malformed
    // wire input. Treat them as missing (Postel's Law).
    EventBook book;
    book.mutable_cover();  // present but empty domain
    EXPECT_EQ(helpers::domain(book), constants::UNKNOWN_DOMAIN);
}

TEST_F(DomainAccessorTest, ReturnsCoverDomainWhenSet) {
    EventBook book;
    book.mutable_cover()->set_domain("orders");
    EXPECT_EQ(helpers::domain(book), "orders");
}

// =============================================================================
// destination_map — Python helpers.destination_map / Rust proto_ext::destination_map
// =============================================================================

class DestinationMapTest : public ::testing::Test {};

TEST_F(DestinationMapTest, BuildsMapKeyedByRootHex) {
    EventBook book1;
    book1.set_next_sequence(5);
    book1.mutable_cover()->set_domain("player");
    std::string root1(16, '\x11');
    book1.mutable_cover()->mutable_root()->set_value(root1);

    EventBook book2;
    book2.set_next_sequence(10);
    book2.mutable_cover()->set_domain("player");
    std::string root2(16, '\x22');
    book2.mutable_cover()->mutable_root()->set_value(root2);

    auto m = helpers::destination_map({book1, book2});
    EXPECT_EQ(m.size(), 2u);

    std::string hex1, hex2;
    for (char c : root1) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned char>(c));
        hex1 += buf;
    }
    for (char c : root2) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned char>(c));
        hex2 += buf;
    }
    EXPECT_NE(m.find(hex1), m.end());
    EXPECT_NE(m.find(hex2), m.end());
}

TEST_F(DestinationMapTest, EmptyListProducesEmptyMap) {
    EXPECT_TRUE(helpers::destination_map({}).empty());
}

TEST_F(DestinationMapTest, SkipsEntriesWithoutRoot) {
    EventBook with_root;
    with_root.mutable_cover()->set_domain("player");
    std::string root(16, '\x11');
    with_root.mutable_cover()->mutable_root()->set_value(root);

    EventBook without_root;
    without_root.mutable_cover()->set_domain("player");  // no root

    auto m = helpers::destination_map({with_root, without_root});
    EXPECT_EQ(m.size(), 1u);
}
