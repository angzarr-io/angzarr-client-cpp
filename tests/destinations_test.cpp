// Tests for Destinations — Python parity covers Destinations behaviour
// in tests/test_destinations.py.  C++ inherits the audit-#64 contract:
//   - stamp_command throws InvalidArgumentError with
//     code=MISSING_DESTINATION_SEQUENCE when the domain is missing.
// And the cross-language additions:
//   - deferred_header static helper (Rust 1f4c8b3, Python cf480ee).
//   - insertion-order preserving iteration of domains() (audit #66).
//   - has_domain canonicalised over `has` (Rust 0693486 / Python 6cd60b9).

#include <gtest/gtest.h>

#include <string>

#include "angzarr/destinations.hpp"
#include "angzarr/error_codes.hpp"
#include "angzarr/errors.hpp"
#include "angzarr/proto_aliases.hpp"

using namespace angzarr;

class DestinationsTest : public ::testing::Test {};

TEST_F(DestinationsTest, SequenceForReturnsValueWhenPresent) {
    Destinations d({{"inventory", 5u}});
    auto s = d.sequence_for("inventory");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(*s, 5u);
}

TEST_F(DestinationsTest, SequenceForReturnsNulloptWhenMissing) {
    Destinations d({{"inventory", 5u}});
    EXPECT_FALSE(d.sequence_for("missing").has_value());
}

TEST_F(DestinationsTest, StampCommandSetsSequenceOnEveryPage) {
    Destinations d({{"inventory", 17u}});
    CommandBook cmd;
    cmd.mutable_cover()->set_domain("inventory");
    cmd.add_pages();
    cmd.add_pages();

    d.stamp_command(cmd, "inventory");
    EXPECT_EQ(cmd.pages(0).header().sequence(), 17u);
    EXPECT_EQ(cmd.pages(1).header().sequence(), 17u);
}

TEST_F(DestinationsTest, StampCommandThrowsInvalidArgumentWithMissingCode) {
    // Audit #64: InvalidArgumentError carries
    // code=MISSING_DESTINATION_SEQUENCE and details["domain"]=<name>.
    Destinations d({{"inventory", 5u}});
    CommandBook cmd;
    cmd.add_pages();

    try {
        d.stamp_command(cmd, "fulfillment");
        FAIL() << "expected InvalidArgumentError";
    } catch (const InvalidArgumentError& e) {
        EXPECT_EQ(e.code(), error_codes::codes::MISSING_DESTINATION_SEQUENCE);
        EXPECT_EQ(e.details().at(error_codes::keys::DOMAIN), "fulfillment");
    }
}

TEST_F(DestinationsTest, HasDomainCanonicalAccessor) {
    Destinations d({{"inventory", 5u}});
    EXPECT_TRUE(d.has_domain("inventory"));
    EXPECT_FALSE(d.has_domain("missing"));
}

TEST_F(DestinationsTest, DomainsPreservesInsertionOrder) {
    // Audit cross-language insertion-order parity (Python preserves
    // dict iteration order; Rust uses IndexMap; C++ now mirrors).
    Destinations d({{"alpha", 1u}, {"beta", 2u}, {"gamma", 3u}});
    auto domains = d.domains();
    ASSERT_EQ(domains.size(), 3u);
    EXPECT_EQ(domains[0], "alpha");
    EXPECT_EQ(domains[1], "beta");
    EXPECT_EQ(domains[2], "gamma");
}

TEST_F(DestinationsTest, DeferredHeaderEmbedsAngzarrDeferredSequence) {
    // Static helper — produces a PageHeader carrying an
    // AngzarrDeferredSequence so the framework can dedupe on
    // (source.root, source_seq, target.root). Mirrors Python's
    // Destinations.deferred_header.
    Cover source;
    source.set_domain("orders");
    source.mutable_root()->set_value(std::string(16, '\x42'));

    auto header = Destinations::deferred_header(source, 9);
    ASSERT_TRUE(header.has_angzarr_deferred());
    EXPECT_EQ(header.angzarr_deferred().source_seq(), 9u);
    ASSERT_TRUE(header.angzarr_deferred().has_source());
    EXPECT_EQ(header.angzarr_deferred().source().domain(), "orders");
}
