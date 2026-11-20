#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

#include "angzarr/types.pb.h"
#include "errors.hpp"

namespace angzarr {

/**
 * Provides access to destination sequences for command stamping.
 *
 * Sagas and process managers receive destination sequences from the framework
 * based on output_domains configured in the component config. Destinations
 * wraps the domain -> next_sequence map from the gRPC request.
 *
 * Design Philosophy:
 * - Sagas/PMs are translators, NOT decision makers
 * - They should NOT rebuild destination state to make business decisions
 * - Business logic belongs in aggregates
 * - Destinations provide only sequences for command stamping
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
    /**
     * Create Destinations from a sequences map.
     *
     * The sequences map comes from the gRPC request's destination_sequences field.
     *
     * @param sequences Map of domain name to next sequence number
     */
    explicit Destinations(std::unordered_map<std::string, uint32_t> sequences = {})
        : sequences_(std::move(sequences)) {}

    /**
     * Get the next sequence number for a domain.
     *
     * @param domain The target domain
     * @return The sequence number, or std::nullopt if domain not found
     */
    std::optional<uint32_t> sequence_for(const std::string& domain) const {
        auto it = sequences_.find(domain);
        if (it == sequences_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    /**
     * Stamp all pages in a command book with the sequence for the given domain.
     *
     * Sets each page's header sequence to the value from the sequences map.
     *
     * @param cmd The command book to stamp
     * @param domain The domain to look up the sequence for
     * @throws InvalidArgumentError if domain is not in the sequences map
     */
    void stamp_command(CommandBook& cmd, const std::string& domain) const {
        auto it = sequences_.find(domain);
        if (it == sequences_.end()) {
            throw InvalidArgumentError(
                "No sequence for domain '" + domain + "' - check output_domains config");
        }
        uint32_t seq = it->second;
        for (int i = 0; i < cmd.pages_size(); ++i) {
            cmd.mutable_pages(i)->mutable_header()->set_sequence(seq);
        }
    }

    /**
     * Check if a sequence exists for the given domain.
     *
     * @param domain The domain to check
     * @return true if the domain has a sequence
     */
    bool has(const std::string& domain) const {
        return sequences_.find(domain) != sequences_.end();
    }

    /**
     * Get all domain names that have sequences.
     *
     * @return Vector of domain names
     */
    std::vector<std::string> domains() const {
        std::vector<std::string> result;
        result.reserve(sequences_.size());
        for (const auto& [domain, _] : sequences_) {
            result.push_back(domain);
        }
        return result;
    }

   private:
    std::unordered_map<std::string, uint32_t> sequences_;
};

}  // namespace angzarr
