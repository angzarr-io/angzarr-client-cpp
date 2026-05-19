// Spec HIGH-2.2 — cross-language `compute_root` byte-equality with
// Python/Rust/C#. The pinned values mirror Rust's
// `identity.rs:71-75` test for `compute_root("player", "alice@x.com")`.

#include "angzarr/identity.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace angzarr::identity;

TEST(IdentityModule, ComputeRoot_PlayerAliceAtX_IsByteEqualWithOtherLangs) {
    // Cross-language pinned canonical value. If this ever drifts, the
    // C++ client diverges from Py/Rs/C#/Ja and the cross-language
    // identity contract breaks.
    auto u = compute_root("player", "alice@x.com");
    EXPECT_EQ(to_text(u), "8cf1fb5d-45ce-58c2-a7e4-34359eb42d7c");
}

TEST(IdentityModule, NamespaceOid_IsRFC4122Constant) {
    // 6ba7b812-9dad-11d1-80b4-00c04fd430c8
    EXPECT_EQ(NAMESPACE_OID[0], 0x6b);
    EXPECT_EQ(NAMESPACE_OID[1], 0xa7);
    EXPECT_EQ(NAMESPACE_OID[15], 0xc8);
}

TEST(IdentityModule, InventoryProductNamespace_IsDNSNamespace) {
    // 6ba7b810-9dad-11d1-80b4-00c04fd430c8 (RFC 4122 DNS namespace).
    EXPECT_EQ(INVENTORY_PRODUCT_NAMESPACE[3], 0x10);
    EXPECT_EQ(INVENTORY_PRODUCT_NAMESPACE[15], 0xc8);
}

TEST(IdentityModule, NewUuidV5_StampsVersionAndVariantBits) {
    auto u = new_uuid_v5(NAMESPACE_OID, "anything");
    // Version 5 (high nibble of byte 6).
    EXPECT_EQ(u[6] & 0xf0, 0x50);
    // RFC 4122 variant (top two bits of byte 8).
    EXPECT_EQ(u[8] & 0xc0, 0x80);
}

TEST(IdentityModule, ComputeRoot_DifferentDomain_DifferentRoot) {
    // Same key, different domain → different UUID.
    auto a = compute_root("player", "alice@x.com");
    auto b = compute_root("customer", "alice@x.com");
    EXPECT_NE(to_text(a), to_text(b));
}

TEST(IdentityModule, ComputeRoot_DifferentKey_DifferentRoot) {
    auto a = compute_root("player", "alice@x.com");
    auto b = compute_root("player", "bob@x.com");
    EXPECT_NE(to_text(a), to_text(b));
}

TEST(IdentityModule, ComputeRoot_SameInputs_Deterministic) {
    auto a = compute_root("inventory", "sku-42");
    auto b = compute_root("inventory", "sku-42");
    EXPECT_EQ(to_text(a), to_text(b));
}

TEST(IdentityModule, DomainSpecificHelpers_DispatchToComputeRoot) {
    EXPECT_EQ(to_text(customer_root("alice@x.com")), to_text(compute_root("customer", "alice@x.com")));
    EXPECT_EQ(to_text(product_root("sku")), to_text(compute_root("product", "sku")));
    EXPECT_EQ(to_text(order_root("ord-1")), to_text(compute_root("order", "ord-1")));
    EXPECT_EQ(to_text(inventory_root("p")), to_text(compute_root("inventory", "p")));
    EXPECT_EQ(to_text(cart_root("c")), to_text(compute_root("cart", "c")));
    EXPECT_EQ(to_text(fulfillment_root("o")), to_text(compute_root("fulfillment", "o")));
}

TEST(IdentityModule, InventoryProductRoot_UsesDNSNamespaceNotOID) {
    // Inventory product uses the DNS namespace per Python parity.
    auto via_helper = inventory_product_root("sku-1");
    auto via_oid = new_uuid_v5(NAMESPACE_OID, "sku-1");
    EXPECT_NE(to_text(via_helper), to_text(via_oid));
}

TEST(IdentityModule, ToProtoBytes_ReturnsExactly16Bytes) {
    auto u = compute_root("player", "alice@x.com");
    auto s = to_proto_bytes(u);
    EXPECT_EQ(s.size(), 16u);
    // First byte must match (no endian swap on raw byte view).
    EXPECT_EQ(static_cast<unsigned char>(s[0]), u[0]);
    EXPECT_EQ(static_cast<unsigned char>(s[15]), u[15]);
}

TEST(IdentityModule, ToText_FormatsCanonical8_4_4_4_12) {
    UuidBytes b = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    };
    EXPECT_EQ(to_text(b), "00112233-4455-6677-8899-aabbccddeeff");
}
