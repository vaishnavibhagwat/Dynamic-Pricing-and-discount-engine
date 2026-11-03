#pragma once

#include "engine/ConflictResolver.hpp"

namespace engine {

/**
 * @brief Gives only the single offer that saves the customer the most.
 *
 * Ties go to the higher priority, then to the earlier stage.
 *
 * OOP ideas: Strategy pattern (a concrete ConflictResolver) and Liskov
 * Substitution - PricingEngine works with it exactly as with any resolver.
 */
class BestDiscountResolver final : public ConflictResolver {
public:
    std::string name() const override { return "BestDiscount"; }
    std::vector<Verdict> resolve(const std::vector<Candidate>& candidates) const override;
};

} // namespace engine
