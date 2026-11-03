#pragma once

#include "engine/PriceGuard.hpp"

namespace engine {

/**
 * @brief Caps a line's item discount at a percentage of its selling price
 *        (60% by default), so stacked offers cannot give goods away.
 *
 * OOP ideas: polymorphism through PriceGuard; encapsulation of the cap as
 * a validated, read-only setting.
 */
class DiscountCapPolicy final : public PriceGuard {
public:
    /// @throws std::invalid_argument unless 0 <= maxPercent <= 100.
    explicit DiscountCapPolicy(int maxPercent = 60);

    std::string name() const override { return "DiscountCapPolicy"; }
    GuardOutcome limit(const domain::CartItem& item,
                       domain::Money proposedDiscount) const override;

    int maxPercent() const noexcept { return maxPercent_; }

private:
    int maxPercent_;
};

} // namespace engine
