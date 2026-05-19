#pragma once

// Testing utilities — deterministic UUID v5 helpers and proto-message
// builders. Mirrors Python's ``angzarr_client.testing`` subpackage and
// Rust's ``testing::*`` module.
//
// Audit findings #50 (uuid_for_default), #99 (testing helpers parity),
// #105 (Destinations export).

#include <google/protobuf/any.pb.h>
#include <google/protobuf/timestamp.pb.h>

#include <initializer_list>
#include <string>
#include <vector>

#include "angzarr/proto_aliases.hpp"

namespace angzarr::testing {

/// Default test namespace UUID — must match the Python and Rust
/// constants byte-for-byte. Audit #99 cross-language parity.
///
/// Python: ``DEFAULT_TEST_NAMESPACE =
/// UUID("a1b2c3d4-e5f6-7890-abcd-ef1234567890")`` Rust:
/// ``Uuid::from_u128(0xa1b2c3d4_e5f6_7890_abcd_ef1234567890u128)``
const std::string& default_test_namespace_bytes();

/// Canonical string form of :func:`default_test_namespace_bytes`. Used
/// by the cross-language test to assert the namespace constant matches
/// Python and Rust.
const std::string& default_test_namespace_str();

/// Generate a deterministic 16-byte UUID v5 (RFC 4122 SHA-1) from the
/// given name + namespace. Returns the raw 16 bytes suitable for
/// aggregate root IDs. Audit #50 / #99 parity.
///
/// The ``namespace_bytes`` overload takes a raw 16-byte namespace; the
/// no-namespace overload uses :func:`default_test_namespace_bytes`.
std::string uuid_for(const std::string& name,
                     const std::string& namespace_bytes);
std::string uuid_for(const std::string& name);

/// Same generator, returns the 8-4-4-4-12 canonical string form.
std::string uuid_str_for(const std::string& name,
                         const std::string& namespace_bytes);
std::string uuid_str_for(const std::string& name);

/// Convenience accessor for ``uuid_for(name, default_test_namespace_bytes())``
/// — explicit cross-language alias of the no-namespace ``uuid_for``.
/// Audit #50: present so language-agnostic docs can reference the
/// ``uuid_for_default`` name without ambiguity.
inline std::string uuid_for_default(const std::string& name) {
  return uuid_for(name);
}
inline std::string uuid_str_for_default(const std::string& name) {
  return uuid_str_for(name);
}

/// Current wall-clock time as a protobuf Timestamp. Same semantics as
/// Python's ``angzarr_client.helpers.now()`` and Rust's
/// ``testing::make_timestamp``.
google::protobuf::Timestamp make_timestamp();

/// Build a Cover proto from domain + 16-byte root.
Cover make_cover(const std::string& domain, const std::string& root_bytes,
                 const std::string& correlation_id = "");

/// Build an EventPage with the given sequence + packed event.
EventPage make_event_page(uint32_t sequence,
                          const google::protobuf::Any& event);

/// Build an EventBook wrapping ``cover`` and ``pages``. ``next_sequence``
/// defaults to ``pages.size()``. Mirrors Python's ``make_event_book``
/// and Rust's ``testing::make_event_book`` (which also auto-fills
/// next_sequence to page count when not provided).
EventBook make_event_book(const Cover& cover, std::vector<EventPage> pages,
                          std::optional<uint32_t> next_sequence = std::nullopt);

/// Convenience initializer-list overload — most call sites construct
/// pages inline.
EventBook make_event_book(const Cover& cover,
                          std::initializer_list<EventPage> pages);

/// Build a CommandPage with the given sequence + packed command.
CommandPage make_command_page(uint32_t sequence,
                              const google::protobuf::Any& command);

/// Build a single-page CommandBook for the given cover + command.
CommandBook make_command_book(const Cover& cover,
                              const google::protobuf::Any& command,
                              uint32_t sequence = 0);

}  // namespace angzarr::testing
