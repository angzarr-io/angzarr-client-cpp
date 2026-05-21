#pragma once

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "angzarr/proto_aliases.hpp"

namespace angzarr {

// Constants matching Rust proto_ext::constants and Python helpers.
//
// 2026-04-30 (audit #88 follow-up): DEFAULT_EDITION sentinel realigned
// to the empty string, matching Python's ``DEFAULT_EDITION = ""`` and
// Rust's main-timeline sentinel post-b6bd920.
namespace constants {
constexpr const char* UNKNOWN_DOMAIN = "unknown";
constexpr const char* WILDCARD_DOMAIN = "*";
constexpr const char* DEFAULT_EDITION = "";
constexpr const char* META_ANGZARR_DOMAIN = "_angzarr";
constexpr const char* PROJECTION_DOMAIN_PREFIX = "_projection";
constexpr const char* PROJECTION_TYPE_URL =
    "angzarr_client.proto.angzarr.v1.Projection";
constexpr const char* CORRELATION_ID_HEADER = "x-correlation-id";
}  // namespace constants

/**
 * Helper functions for working with Angzarr types.
 */
namespace helpers {

/**
 * Get the domain from an EventBook.
 *
 * Audit #54 (Rust e6d6ee2 / Python helpers.py::domain): falls back
 * to ``UNKNOWN_DOMAIN`` for an absent cover *or* an empty-domain
 * cover. Empty-domain covers arise from partially-constructed test
 * fixtures and malformed wire input; treating them as missing
 * (Postel's Law) normalizes routing-key / cache-key derivations so
 * cross-language calls produce the same string.
 */
inline std::string domain(const EventBook& book) {
  if (!book.has_cover()) return constants::UNKNOWN_DOMAIN;
  const auto& d = book.cover().domain();
  if (d.empty()) return constants::UNKNOWN_DOMAIN;
  return d;
}

/**
 * Get the correlation ID from an EventBook.
 */
inline std::string correlation_id(const EventBook& book) {
  return book.has_cover() ? book.cover().correlation_id() : "";
}

/**
 * Check if an EventBook has a correlation ID.
 */
inline bool has_correlation_id(const EventBook& book) {
  return book.has_cover() && !book.cover().correlation_id().empty();
}

/**
 * Get the root UUID from an EventBook.
 */
inline const UUID* root_uuid(const EventBook& book) {
  return book.has_cover() && book.cover().has_root() ? &book.cover().root()
                                                     : nullptr;
}

/**
 * Get the root UUID as hex string from an EventBook.
 */
std::string root_id_hex(const EventBook& book);

/**
 * Spec MED-2.6 — get the edition name from an EventBook, returning
 * ``nullopt`` when the cover is missing or carries no (or an empty)
 * edition. Mirrors Py's ``edition()`` (returns ``Optional[str]``) and
 * Rs's ``edition()`` (returns ``&str`` defaulting to
 * :data:`constants::DEFAULT_EDITION`).
 */
inline std::optional<std::string> edition(const EventBook& book) {
  if (!book.has_cover() || !book.cover().has_edition()) return std::nullopt;
  const auto& name = book.cover().edition().name();
  if (name.empty()) return std::nullopt;
  return name;
}

/**
 * Spec MED-2.4 — routing key for an EventBook. Format:
 * ``{domain}:{root_hex}``. ``root_hex`` is the canonical lowercase
 * hex form (no dashes — short form for routing). Mirrors Py/Rs/Go
 * ``routing_key`` shape so the same key is computed in every lang.
 */
inline std::string routing_key(const EventBook& book) {
  std::string root_hex;
  const auto* r = root_uuid(book);
  if (r != nullptr) {
    root_hex.reserve(r->value().size() * 2);
    char buf[3];
    for (unsigned char c : r->value()) {
      std::snprintf(buf, sizeof(buf), "%02x", c);
      root_hex.append(buf, 2);
    }
  }
  return domain(book) + ":" + root_hex;
}

/**
 * Spec MED-2.3 — cache key for an EventBook. Format:
 * ``{edition}:{domain}:{root_hex}``. ``edition`` falls through to
 * :data:`constants::DEFAULT_EDITION` (empty string) when missing,
 * matching the canonical empty-edition main-timeline sentinel
 * established by spec HIGH-2.1.
 */
inline std::string cache_key(const EventBook& book) {
  auto ed = edition(book);
  return (ed.has_value() ? *ed : std::string(constants::DEFAULT_EDITION)) +
         ":" + routing_key(book);
}

/**
 * Calculate the next sequence number from an EventBook.
 *
 * Uses the framework-precomputed next_sequence field rather than counting
 * pages, because snapshots may cause the EventBook to contain only
 * post-snapshot events — counting pages would give the wrong sequence.
 */
inline int next_sequence(const EventBook* book) {
  if (!book) return 0;
  return static_cast<int>(book->next_sequence());
}

/**
 * Extract the type name from a type URL.
 */
inline std::string type_name_from_url(const std::string& type_url) {
  auto pos = type_url.rfind('/');
  return pos != std::string::npos ? type_url.substr(pos + 1) : type_url;
}

constexpr const char* TYPE_URL_PREFIX = "type.googleapis.com/";

/**
 * Check if a type URL matches the given fully qualified type name.
 * @param type_url Full type URL (e.g.,
 * "type.googleapis.com/examples.CardsDealt")
 * @param type_name Fully qualified type name (e.g., "examples.CardsDealt")
 * @return true if type_url equals TYPE_URL_PREFIX + type_name
 */
inline bool type_url_matches(const std::string& type_url,
                             const std::string& type_name) {
  return type_url == std::string(TYPE_URL_PREFIX) + type_name;
}

/**
 * Get the current timestamp as a protobuf Timestamp.
 */
google::protobuf::Timestamp now();

/**
 * Pack a protobuf message into an Any.
 */
template <typename T>
google::protobuf::Any pack_any(const T& message) {
  google::protobuf::Any any;
  any.PackFrom(message, "type.googleapis.com/");
  return any;
}

/**
 * Pack an event into an EventPage.
 */
template <typename T>
EventPage pack_event(const T& event_message) {
  EventPage page;
  page.mutable_event()->PackFrom(event_message, "type.googleapis.com/");
  return page;
}

/**
 * Create a new EventBook with the given events.
 */
template <typename... Events>
EventBook new_event_book(const Events&... events) {
  EventBook book;
  (book.add_pages()->CopyFrom(pack_event(events)), ...);
  return book;
}

/**
 * Create a new EventBook with a single event and sequence number.
 * Used by CommandHandlerBase for handler dispatch.
 */
template <typename Event>
EventBook new_event_book(const Event& event, int seq) {
  EventBook book;
  auto* page = book.add_pages();
  page->mutable_header()->set_sequence(seq);
  page->mutable_event()->PackFrom(event, "type.googleapis.com/");
  *page->mutable_created_at() = now();
  return book;
}

// ============================================================================
// Audit-ported helpers (cross-language parity with Python helpers.py
// / Rust proto_ext/uuid.rs / pages.rs / type_url.rs).
// ============================================================================

/**
 * Convert raw bytes to a standard UUID text format.
 *
 * 16-byte input formats as the canonical 8-4-4-4-12 UUID. Other
 * lengths fall back to lowercase hex. Mirrors Python's
 * ``bytes_to_uuid_text`` and Rust's ``proto_ext::uuid::to_uuid_text``
 * (audit #48).
 */
inline std::string bytes_to_uuid_text(const std::string& bytes) {
  auto to_hex = [](unsigned char c) -> std::string {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02x", c);
    return std::string(buf, 2);
  };
  std::string hex;
  hex.reserve(bytes.size() * 2);
  for (unsigned char c : bytes) hex += to_hex(c);
  if (bytes.size() != 16) return hex;
  // 8-4-4-4-12 form: positions 8, 12, 16, 20 get a dash.
  return hex.substr(0, 8) + "-" + hex.substr(8, 4) + "-" + hex.substr(12, 4) +
         "-" + hex.substr(16, 4) + "-" + hex.substr(20, 12);
}

/**
 * Convert a proto UUID to its canonical text form. Returns ``""`` for
 * a null pointer. Audit #48.
 */
inline std::string to_uuid_text(const UUID* u) {
  if (u == nullptr) return "";
  return bytes_to_uuid_text(u->value());
}

/**
 * Return the divergence sequence for ``domain_name`` on the given
 * edition, or ``std::nullopt`` when the edition is missing the
 * domain (or the edition itself is null).
 *
 * Audit #49: the legacy ``-1`` sentinel was replaced with
 * ``std::optional<uint32_t>`` to match Rust's ``Option<u32>`` and
 * Python's ``Optional[int]``. Stops conflating "missing" with the
 * impossible "negative-sequence" case.
 */
inline std::optional<uint32_t> divergence_for(const Edition* edition,
                                              std::string_view domain_name) {
  if (edition == nullptr) return std::nullopt;
  for (int i = 0; i < edition->divergences_size(); ++i) {
    const auto& d = edition->divergences(i);
    if (d.domain() == domain_name) return d.sequence();
  }
  return std::nullopt;
}

/**
 * Build the composite idempotency key for a saga-produced deferred
 * sequence.
 *
 * Format: ``{source.edition.name}:{source.domain}:{root_hex}:{source_seq}``.
 *
 * Returns ``std::nullopt`` when the deferred sequence has no source
 * cover — a malformed wire input is a missing-key signal, not an
 * exception. Audit #55: parity with Python returning ``None`` and
 * Rust returning ``Result`` (Err).
 */
inline std::optional<std::string> idempotency_key(
    const AngzarrDeferredSequence* deferred) {
  if (deferred == nullptr || !deferred->has_source()) return std::nullopt;
  const auto& source = deferred->source();
  std::string edition_name =
      source.has_edition() ? source.edition().name() : "";
  std::string root_hex;
  if (source.has_root()) {
    const auto& bytes = source.root().value();
    root_hex.reserve(bytes.size() * 2);
    char buf[3];
    for (unsigned char c : bytes) {
      snprintf(buf, sizeof(buf), "%02x", c);
      root_hex.append(buf, 2);
    }
  }
  return edition_name + ":" + source.domain() + ":" + root_hex + ":" +
         std::to_string(deferred->source_seq());
}

/**
 * Build a map from root UUID hex string to EventBook for destination
 * lookup. Used in multi-destination sagas/PMs to find the correct
 * EventBook by aggregate root when stamping commands.
 *
 * Entries whose cover has no root are silently skipped — they're
 * unaddressable in the map's key space. Mirrors Python's
 * ``helpers.destination_map`` and Rust's
 * ``proto_ext::destination_map`` (audit #86 mechanical).
 */
inline std::unordered_map<std::string, EventBook> destination_map(
    const std::vector<EventBook>& destinations) {
  std::unordered_map<std::string, EventBook> result;
  for (const auto& dest : destinations) {
    if (!dest.has_cover() || !dest.cover().has_root()) continue;
    const auto& bytes = dest.cover().root().value();
    std::string hex;
    hex.reserve(bytes.size() * 2);
    char buf[3];
    for (unsigned char c : bytes) {
      snprintf(buf, sizeof(buf), "%02x", c);
      hex.append(buf, 2);
    }
    result.emplace(std::move(hex), dest);
  }
  return result;
}

/**
 * Extract the wire-format type name from a fully-qualified type name.
 *
 * Cross-language alias — identity in C++ (and Python). Rust's
 * ``convert::wire_name`` strips the prost-emitted package prefix; in
 * C++ the wire form matches the descriptor's ``full_name`` directly
 * so this is a no-op. Function exists for cross-language API
 * symmetry. Audit finding #32.
 */
inline std::string wire_name(std::string_view full_name) {
  return std::string(full_name);
}

/**
 * Construct a full type URL from a fully-qualified message type
 * name. Mirrors Python's ``helpers.type_url`` and Rust's
 * ``convert::full_type_url``. Audit #32.
 */
inline std::string type_url(std::string_view type_name) {
  return std::string(TYPE_URL_PREFIX) + wire_name(type_name);
}

/**
 * Alias for cross-language API parity with Rust's
 * ``type_url_matches_exact``. Same semantics as ``type_url_matches``.
 */
inline bool type_url_matches_exact(const std::string& url,
                                   const std::string& type_name) {
  return type_url_matches(url, type_name);
}

/**
 * Build gRPC metadata carrying the ``x-correlation-id`` header.
 *
 * Audit #69: an empty correlation_id produces an empty list — the
 * header is skipped, never sent as a blank value. Mirrors Python
 * (``correlated_metadata``) and Rust.
 */
inline std::vector<std::pair<std::string, std::string>> correlated_metadata(
    const std::string& correlation_id) {
  if (correlation_id.empty()) return {};
  return {{constants::CORRELATION_ID_HEADER, correlation_id}};
}

/**
 * Spec MED-2.x parity — :func:`full_type_name<T>` returns the
 * fully-qualified protobuf message name. ``T`` must be a generated
 * proto message type (has ``descriptor()``).
 */
template <typename T>
std::string full_type_name() {
  return T::descriptor()->full_name();
}

/**
 * Spec MED-2.x parity — :func:`full_type_url<T>` returns the
 * ``type.googleapis.com/``-prefixed fully-qualified URL.
 */
template <typename T>
std::string full_type_url() {
  return type_url(full_type_name<T>());
}

/**
 * Spec MED-2.x parity — return ``true`` if ``any.type_url()`` matches
 * the wire-format URL of ``T``.
 */
template <typename T>
bool type_matches(const google::protobuf::Any& any) {
  return type_url_matches_exact(any.type_url(), full_type_name<T>());
}

/**
 * Spec MED-2.x parity — try to unpack ``any`` into ``T``. Returns
 * ``nullopt`` on type mismatch or decode failure. Non-throwing
 * counterpart to ``unpack_any_or_throw`` (see
 * :file:`grpc_translator.hpp` for the loud-fail variant).
 */
template <typename T>
std::optional<T> try_unpack(const google::protobuf::Any& any) {
  if (!type_matches<T>(any)) return std::nullopt;
  T out;
  if (!any.UnpackTo(&out)) return std::nullopt;
  return out;
}

/**
 * Spec MED-2.x parity — parse an RFC3339 timestamp into a protobuf
 * ``Timestamp``. Throws :class:`InvalidTimestampError` on bad grammar
 * (matches Py/Cs ``parse_timestamp`` audit-#34 raise-at-call-site
 * semantics).
 *
 * Delegates to the same parser used by ``QueryBuilder::as_of_time``
 * (spec MED-3.6) so call sites see identical grammar acceptance.
 */
google::protobuf::Timestamp parse_timestamp(const std::string& rfc3339);

/**
 * Generate a fresh 16-byte UUID v4 as raw bytes.
 *
 * Used by ``command_new`` to materialize a client-side aggregate
 * root (audit #20). The implementation uses ``std::random_device``
 * seeding plus ``mt19937_64`` for reasonable independence between
 * calls without pulling in libuuid as a dependency. RFC 4122
 * version/variant bits are set per the spec (version=4, variant=10b).
 */
inline std::string new_uuid_v4_bytes() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::string bytes(16, '\0');
  uint64_t hi = rng();
  uint64_t lo = rng();
  for (int i = 0; i < 8; ++i) {
    bytes[i] = static_cast<char>((hi >> (8 * (7 - i))) & 0xff);
    bytes[8 + i] = static_cast<char>((lo >> (8 * (7 - i))) & 0xff);
  }
  // RFC 4122 v4: octet 6 high nibble = 0100, octet 8 high two bits = 10.
  bytes[6] = static_cast<char>((bytes[6] & 0x0f) | 0x40);
  bytes[8] = static_cast<char>((bytes[8] & 0x3f) | 0x80);
  return bytes;
}

}  // namespace helpers
}  // namespace angzarr
