#include "angzarr/identity.hpp"

#include <openssl/evp.h>

#include <cstdio>
#include <stdexcept>
#include <string>

namespace angzarr::identity {

UuidBytes new_uuid_v5(const UuidBytes& namespace_id, std::string_view name) {
  // RFC 4122 §4.3: hash = SHA1(namespace_id_bytes || name_bytes).
  // Take the first 16 bytes; stamp version 5 (high nibble of byte 6 = 0101)
  // and the RFC 4122 variant (top two bits of byte 8 = 10).
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) {
    throw std::runtime_error("EVP_MD_CTX_new failed");
  }
  if (EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr) != 1 ||
      EVP_DigestUpdate(ctx, namespace_id.data(), namespace_id.size()) != 1 ||
      EVP_DigestUpdate(ctx, name.data(), name.size()) != 1 ||
      EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("SHA-1 digest computation failed");
  }
  EVP_MD_CTX_free(ctx);

  UuidBytes out{};
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = hash[i];
  }
  // Version 5 (name-based, SHA-1).
  out[6] = static_cast<unsigned char>((out[6] & 0x0f) | 0x50);
  // RFC 4122 variant.
  out[8] = static_cast<unsigned char>((out[8] & 0x3f) | 0x80);
  return out;
}

UuidBytes compute_root(std::string_view domain, std::string_view business_key) {
  // Mirror Python: name = "angzarr" + domain + business_key (UTF-8 bytes).
  std::string name;
  name.reserve(7 + domain.size() + business_key.size());
  name.append("angzarr");
  name.append(domain);
  name.append(business_key);
  return new_uuid_v5(NAMESPACE_OID, name);
}

UuidBytes inventory_product_root(std::string_view product_id) {
  return new_uuid_v5(INVENTORY_PRODUCT_NAMESPACE, product_id);
}

UuidBytes customer_root(std::string_view email) {
  return compute_root("customer", email);
}
UuidBytes product_root(std::string_view sku) {
  return compute_root("product", sku);
}
UuidBytes order_root(std::string_view order_id) {
  return compute_root("order", order_id);
}
UuidBytes inventory_root(std::string_view product_id) {
  return compute_root("inventory", product_id);
}
UuidBytes cart_root(std::string_view customer_id) {
  return compute_root("cart", customer_id);
}
UuidBytes fulfillment_root(std::string_view order_id) {
  return compute_root("fulfillment", order_id);
}

std::string to_text(const UuidBytes& uuid) {
  char buf[37];
  std::snprintf(
      buf, sizeof(buf),
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
      uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
      uuid[15]);
  return std::string(buf, 36);
}

}  // namespace angzarr::identity
