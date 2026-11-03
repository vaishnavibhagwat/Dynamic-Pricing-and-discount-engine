#pragma once

#include "engine/ConflictResolver.hpp"

namespace engine {

/**
 * @brief Gives only the offer with the highest priority, whatever it saves.
 *
 * Models a business that prefers some offers (e.g. its own sale over a bank
 * offer). Ties go to the larger saving, then to the earlier stage.
 *
 * OOP ideas: Strategy pattern (a concrete ConflictResolver) and Liskov
 * Substitution.
 */
class PriorityResolver final : public ConflictResolver {
public:
    std::string name() const override { return "Priority"; }
    std::vector<Verdict> resolve(const std::vector<Candidate>& candidates) const override;
};

} // namespace engine
