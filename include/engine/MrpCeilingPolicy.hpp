#pragma once

#include "engine/GstCalculator.hpp"
#include "engine/PriceGuard.hpp"

namespace engine {

/**
 * @brief Makes sure no line costs more than its MRP once GST is added.
 *
 * Selling prices are before GST, while MRP includes all taxes (as the law
 * requires). If net + GST would exceed MRP x quantity, the discount is
 * raised just enough to bring the price down to the ceiling. This is the
 * last guard, so the legal limit always wins.
 *
 * OOP ideas: polymorphism through PriceGuard, and composition - it reuses
 * GstCalculator instead of repeating tax arithmetic.
 */
class MrpCeilingPolicy final : public PriceGuard {
public:
    std::string name() const override { return "MrpCeilingPolicy"; }
    GuardOutcome limit(const domain::CartItem& item,
                       domain::Money proposedDiscount) const override;

    /// Highest net price (before GST) whose GST-inclusive price is within MRP x qty.
    domain::Money ceilingFor(const domain::CartItem& item) const noexcept;

private:
    GstCalculator gst_;
};

} // namespace engine
