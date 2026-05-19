#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "angzarr/proto_aliases.hpp"
#include "error_codes.hpp"
#include "errors.hpp"

namespace angzarr {

/**
 * Provides access to destination sequences for command stamping.
 *
 * Sagas and process managers receive destination sequences from the
 * framework based on output_domains configured in the component
 * config. Destinations wraps the domain -> next_sequence map from the
 * gRPC request.
 *
 * Design Philosophy
 *  - Sagas/PMs are translators, NOT decision makers
 *  - They should NOT rebuild destination state to make business decisions
 *  - Business logic belongs in aggregates
 *  - Destinations provide only sequences for command stamping
 *
 * Insertion-order parity: the underlying storage preserves the order
 * in which entries were inserted (Python ``dict`` / Rust ``IndexMap``
 * parity). ``domains()`` returns them in that order.
 *
 * Example:
 * @code
 *   SagaHandlerResponse execute(const EventBook& source,
 *                                const Any& event,
 *                                const Destinations& destinations) override {
 *       CommandBook cmd;
 *       cmd.mutable_cover()->set_domain("fulfillment");
 *       destinations.stamp_command(cmd, "fulfillment");
 *       return SagaHandlerResponse::with_commands({cmd});
 *   }
 * @endcode
 */
class Destinations {
 public:
  Destinations() = default;

  /**
   * Create Destinations from an initializer list of (domain,
   * sequence) pairs. Pairs retain their order; later pairs with
   * the same key replace earlier ones.
   */
  Destinations(
      std::initializer_list<std::pair<std::string, uint32_t>> entries) {
    for (const auto& [d, s] : entries) insert(d, s);
  }

  /**
   * Create Destinations from a flat domain->sequence map. The
   * unordered_map iteration order is captured at construction.
   */
  explicit Destinations(
      const std::unordered_map<std::string, uint32_t>& sequences) {
    for (const auto& [d, s] : sequences) insert(d, s);
  }

  /**
   * Create Destinations from a vector of ordered (domain, seq)
   * pairs — the canonical insertion-order form. Mirrors Rust's
   * ``IndexMap::from_iter``.
   */
  explicit Destinations(std::vector<std::pair<std::string, uint32_t>> entries) {
    for (auto& e : entries) insert(e.first, e.second);
  }

  /**
   * Get the next sequence number for a destination domain.
   *
   * @return The next sequence number, or std::nullopt if domain not found.
   */
  std::optional<uint32_t> sequence_for(const std::string& domain) const {
    auto it = index_.find(domain);
    if (it == index_.end()) return std::nullopt;
    return order_[it->second].second;
  }

  /**
   * Stamp all pages in a command book with the sequence for the
   * given domain.
   *
   * Audit #64: throws ``InvalidArgumentError`` carrying
   * ``code=MISSING_DESTINATION_SEQUENCE`` and
   * ``details["domain"]=<domain>`` when the domain is absent.
   * Mirrors Python's ``Destinations.stamp_command`` and Rust's
   * equivalent.
   */
  void stamp_command(CommandBook& cmd, const std::string& domain) const {
    auto it = index_.find(domain);
    if (it == index_.end()) {
      throw InvalidArgumentError(
          error_codes::messages::MISSING_DESTINATION_SEQUENCE,
          error_codes::codes::MISSING_DESTINATION_SEQUENCE,
          {{error_codes::keys::DOMAIN, domain}});
    }
    uint32_t seq = order_[it->second].second;
    for (int i = 0; i < cmd.pages_size(); ++i) {
      cmd.mutable_pages(i)->mutable_header()->set_sequence(seq);
    }
  }

  /// Audit canonical accessor for "does this destination exist?".
  /// Mirrors Python's ``has_domain`` / Rust's ``has_domain``.
  bool has_domain(const std::string& domain) const {
    return index_.find(domain) != index_.end();
  }

  /// Legacy alias retained for backwards compat with earlier C++ callers.
  bool has(const std::string& domain) const { return has_domain(domain); }

  /**
   * Get destination domains in insertion order.
   *
   * Audit P2.2 / 4b78e97 (Rust) / 326fec5 (Python): the order in
   * which the framework supplied destinations is observable here;
   * sagas can rely on it for deterministic test fixtures.
   */
  std::vector<std::string> domains() const {
    std::vector<std::string> out;
    out.reserve(order_.size());
    for (const auto& [d, _] : order_) out.push_back(d);
    return out;
  }

  /**
   * Build a PageHeader carrying an AngzarrDeferredSequence.
   *
   * Use this on saga-produced commands so the framework can dedupe
   * on ``(source.root, source_seq, target.root)``. AMQP
   * at-least-once redelivery of the trigger event then becomes a
   * no-op at the destination aggregate.
   *
   * Mirrors Python's ``Destinations.deferred_header`` (commit
   * cf480ee) and Rust's ``Destinations::deferred_header`` (commit
   * e7d9603).
   */
  static PageHeader deferred_header(const Cover& source_cover,
                                    uint32_t source_seq) {
    PageHeader header;
    auto* deferred = header.mutable_angzarr_deferred();
    *deferred->mutable_source() = source_cover;
    deferred->set_source_seq(source_seq);
    return header;
  }

 private:
  void insert(const std::string& domain, uint32_t seq) {
    auto it = index_.find(domain);
    if (it == index_.end()) {
      index_[domain] = order_.size();
      order_.emplace_back(domain, seq);
    } else {
      order_[it->second].second = seq;  // last-write-wins
    }
  }

  std::vector<std::pair<std::string, uint32_t>> order_;
  std::unordered_map<std::string, std::size_t> index_;
};

}  // namespace angzarr
