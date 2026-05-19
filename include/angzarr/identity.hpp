#pragma once

// Aggregate identity computation for Angzarr domains.
//
// Provides deterministic UUID generation from business keys via UUID
// v5 (SHA-1 name-based). The canonical formula matches Python's
// :file:`angzarr_client/identity.py`, Rust's :file:`src/identity.rs`,
// and C#'s :file:`Identity.cs` byte-for-byte.
//
// Spec HIGH-2.2: cross-language `compute_root(domain, key)` must be
// byte-equal across all languages. Rust pins
// `compute_root("player", "alice@x.com") =
// 8cf1fb5d-45ce-58c2-a7e4-34359eb42d7c`
// (`identity.rs:71-75`); Python and C# match. This module adds C++
// parity.

#include <array>
#include <string>
#include <string_view>

namespace angzarr::identity {

/// 16-byte UUID as raw bytes (big-endian / network-order, the canonical
/// wire representation matching Python/Rust UUID bytes).
using UuidBytes = std::array<unsigned char, 16>;

/// RFC 4122 OID namespace UUID. Used by :func:`compute_root`.
/// Hex: ``6ba7b812-9dad-11d1-80b4-00c04fd430c8``.
inline constexpr UuidBytes NAMESPACE_OID = {
    0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1,
    0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8,
};

/// RFC 4122 DNS namespace UUID. Used by :func:`inventory_product_root`.
/// Matches Python's ``INVENTORY_PRODUCT_NAMESPACE`` and Rust's
/// ``INVENTORY_PRODUCT_NAMESPACE`` (hex
/// ``6ba7b810-9dad-11d1-80b4-00c04fd430c8``).
inline constexpr UuidBytes INVENTORY_PRODUCT_NAMESPACE = {
    0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1,
    0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8,
};

/// Generate a name-based (v5/SHA-1) UUID per RFC 4122 §4.3.
///
/// SHA-1 is mandated by the v5 algorithm; ``OpenSSL::Crypto``
/// provides it (already linked as an explicit dep per audit #50).
UuidBytes new_uuid_v5(const UuidBytes& namespace_id, std::string_view name);

/// Compute a deterministic root UUID from domain and business key.
///
/// Mirrors Python's ``compute_root``:
/// ``uuid5(NAMESPACE_OID, "angzarr" + domain + business_key)``.
///
/// Cross-language byte-equality (spec HIGH-2.2):
/// ``compute_root("player", "alice@x.com")`` →
/// ``8cf1fb5d-45ce-58c2-a7e4-34359eb42d7c``.
UuidBytes compute_root(std::string_view domain, std::string_view business_key);

/// Deterministic UUID for an inventory product aggregate.
/// Uses the DNS namespace (matches Python ``INVENTORY_PRODUCT_NAMESPACE``).
UuidBytes inventory_product_root(std::string_view product_id);

/// Deterministic root UUID for a customer aggregate.
UuidBytes customer_root(std::string_view email);

/// Deterministic root UUID for a product aggregate.
UuidBytes product_root(std::string_view sku);

/// Deterministic root UUID for an order aggregate.
UuidBytes order_root(std::string_view order_id);

/// Deterministic root UUID for an inventory aggregate.
UuidBytes inventory_root(std::string_view product_id);

/// Deterministic root UUID for a cart aggregate.
UuidBytes cart_root(std::string_view customer_id);

/// Deterministic root UUID for a fulfillment aggregate.
UuidBytes fulfillment_root(std::string_view order_id);

/// Convert a UUID byte array to its canonical 8-4-4-4-12 text form
/// (lowercase hex with dashes).
std::string to_text(const UuidBytes& uuid);

/// Convert a UUID byte array to a ``std::string`` of 16 raw bytes
/// (suitable for stamping into proto ``UUID.value``).
inline std::string to_proto_bytes(const UuidBytes& uuid) {
  return std::string(reinterpret_cast<const char*>(uuid.data()), uuid.size());
}

}  // namespace angzarr::identity
