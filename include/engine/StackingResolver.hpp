#pragma once

#include "engine/ConflictResolver.hpp"

namespace engine {

/**
 * @brief Combines every stackable offer; a non-stackable offer is given
 *        alone only if it saves more than the whole stack.
 *
 * Among non-stackable offers the best saving wins (ties: higher priority,
 * then earlier stage). If the stack and the best non-stackable offer save
 * the same, the stack wins.
 *
 * OOP ideas: Strategy pattern (a concrete ConflictResolver) and Liskov
 * Substitution; it is the only resolver that reads DiscountRule::stackable().
 */
class StackingResolver final : public ConflictResolver {
public:
    std::string name() const override { return "Stacking"; }
    std::vector<Verdict> resolve(const std::vector<Candidate>& candidates) const override;
};

} // namespace engine
