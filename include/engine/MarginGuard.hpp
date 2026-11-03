#pragma once

#include "engine/PriceGuard.hpp"

namespace engine {

/**
 * @brief Keeps a line's net price at least a markup above cost
 *        (cost + 8% by default).
 *
 * The guard only ever reduces a discount; it never raises the price above
 * the selling price, so a product already sold below the floor (a loss
 * leader) simply gets no discount.
 *
 * OOP ideas: polymorphism through PriceGuard; Single Responsibility.
 */
class MarginGuard final : public PriceGuard {
public:
    /// @throws std::invalid_argument if minMarkupPercent < 0.
    explicit MarginGuard(int minMarkupPercent = 8);

    std::string name() const override { return "MarginGuard"; }
    GuardOutcome limit(const domain::CartItem& item,
                       domain::Money proposedDiscount) const override;

    int minMarkupPercent() const noexcept { return minMarkupPercent_; }
    /// Lowest allowed net price for the line: cost x qty x (100 + markup)%, rounded up.
    domain::Money floorFor(const domain::CartItem& item) const noexcept;

private:
    int minMarkupPercent_;
};

} // namespace engine
