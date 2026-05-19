// Implementation for the angzarr::testing namespace.
//
// uuid_for follows RFC 4122 v5 (SHA-1) — same algorithm Python's
// ``uuid5`` and Rust's ``Uuid::new_v5`` implement. The SHA-1 digest
// comes from OpenSSL (explicit link added in CMakeLists). Audit #50.

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

#include "angzarr/helpers.hpp"
#include "angzarr/testing.hpp"

namespace angzarr::testing {

namespace {

// "a1b2c3d4-e5f6-7890-abcd-ef1234567890" -> 16 raw bytes.
constexpr std::array<unsigned char, 16> kDefaultNamespace = {
    0xa1, 0xb2, 0xc3, 0xd4, 0xe5, 0xf6, 0x78, 0x90,
    0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90,
};

const std::string& default_namespace_bytes_storage() {
  static const std::string bytes(
      reinterpret_cast<const char*>(kDefaultNamespace.data()),
      kDefaultNamespace.size());
  return bytes;
}

const std::string& default_namespace_str_storage() {
  static const std::string str = "a1b2c3d4-e5f6-7890-abcd-ef1234567890";
  return str;
}

// RFC 4122 v5: ``SHA1(namespace_bytes || name).truncate(16)`` with the
// version (bits 48..51) clamped to 0101b and the variant (bits 64..65)
// clamped to 10b.
std::string uuid_v5(const std::string& namespace_bytes,
                    const std::string& name) {
  if (namespace_bytes.size() != 16) {
    throw std::invalid_argument("uuid_v5: namespace must be 16 bytes");
  }
  unsigned char digest[SHA_DIGEST_LENGTH];
  unsigned int digest_len = 0;
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) throw std::runtime_error("EVP_MD_CTX_new failed");
  EVP_DigestInit_ex(ctx, EVP_sha1(), nullptr);
  EVP_DigestUpdate(ctx, namespace_bytes.data(), namespace_bytes.size());
  EVP_DigestUpdate(ctx, name.data(), name.size());
  EVP_DigestFinal_ex(ctx, digest, &digest_len);
  EVP_MD_CTX_free(ctx);

  std::string out(reinterpret_cast<const char*>(digest), 16);
  // Set version to 5: clear high nibble of byte 6, then OR 0x50.
  out[6] =
      static_cast<char>((static_cast<unsigned char>(out[6]) & 0x0f) | 0x50);
  // Set variant to RFC 4122: clear top two bits of byte 8, OR 0x80.
  out[8] =
      static_cast<char>((static_cast<unsigned char>(out[8]) & 0x3f) | 0x80);
  return out;
}

std::string bytes_to_uuid_string(const std::string& bytes) {
  if (bytes.size() != 16)
    throw std::invalid_argument("uuid string requires 16 bytes");
  char buf[37];
  std::snprintf(
      buf, sizeof(buf),
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      static_cast<unsigned char>(bytes[0]),
      static_cast<unsigned char>(bytes[1]),
      static_cast<unsigned char>(bytes[2]),
      static_cast<unsigned char>(bytes[3]),
      static_cast<unsigned char>(bytes[4]),
      static_cast<unsigned char>(bytes[5]),
      static_cast<unsigned char>(bytes[6]),
      static_cast<unsigned char>(bytes[7]),
      static_cast<unsigned char>(bytes[8]),
      static_cast<unsigned char>(bytes[9]),
      static_cast<unsigned char>(bytes[10]),
      static_cast<unsigned char>(bytes[11]),
      static_cast<unsigned char>(bytes[12]),
      static_cast<unsigned char>(bytes[13]),
      static_cast<unsigned char>(bytes[14]),
      static_cast<unsigned char>(bytes[15]));
  return std::string(buf, 36);
}

}  // namespace

const std::string& default_test_namespace_bytes() {
  return default_namespace_bytes_storage();
}

const std::string& default_test_namespace_str() {
  return default_namespace_str_storage();
}

std::string uuid_for(const std::string& name,
                     const std::string& namespace_bytes) {
  return uuid_v5(namespace_bytes, name);
}

std::string uuid_for(const std::string& name) {
  return uuid_v5(default_test_namespace_bytes(), name);
}

std::string uuid_str_for(const std::string& name,
                         const std::string& namespace_bytes) {
  return bytes_to_uuid_string(uuid_v5(namespace_bytes, name));
}

std::string uuid_str_for(const std::string& name) {
  return bytes_to_uuid_string(uuid_v5(default_test_namespace_bytes(), name));
}

google::protobuf::Timestamp make_timestamp() { return helpers::now(); }

Cover make_cover(const std::string& domain, const std::string& root_bytes,
                 const std::string& correlation_id) {
  Cover cover;
  cover.set_domain(domain);
  cover.mutable_root()->set_value(root_bytes);
  if (!correlation_id.empty()) {
    cover.set_correlation_id(correlation_id);
  }
  return cover;
}

EventPage make_event_page(uint32_t sequence,
                          const google::protobuf::Any& event) {
  EventPage page;
  page.mutable_header()->set_sequence(sequence);
  *page.mutable_event() = event;
  *page.mutable_created_at() = make_timestamp();
  return page;
}

EventBook make_event_book(const Cover& cover, std::vector<EventPage> pages,
                          std::optional<uint32_t> next_sequence) {
  EventBook book;
  *book.mutable_cover() = cover;
  uint32_t fallback = static_cast<uint32_t>(pages.size());
  for (auto& page : pages) {
    *book.add_pages() = std::move(page);
  }
  book.set_next_sequence(next_sequence.value_or(fallback));
  return book;
}

EventBook make_event_book(const Cover& cover,
                          std::initializer_list<EventPage> pages) {
  return make_event_book(
      cover, std::vector<EventPage>(pages.begin(), pages.end()), std::nullopt);
}

CommandPage make_command_page(uint32_t sequence,
                              const google::protobuf::Any& command) {
  CommandPage page;
  page.mutable_header()->set_sequence(sequence);
  *page.mutable_command() = command;
  return page;
}

CommandBook make_command_book(const Cover& cover,
                              const google::protobuf::Any& command,
                              uint32_t sequence) {
  CommandBook book;
  *book.mutable_cover() = cover;
  *book.add_pages() = make_command_page(sequence, command);
  return book;
}

}  // namespace angzarr::testing
