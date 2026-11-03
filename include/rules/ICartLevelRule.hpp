#pragma once

#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"

namespace rules {

/**
 * @brief Interface for offers that discount the order total as a whole.
 *
 * OOP idea: Interface Segregation Principle - cart rules only promise a
 * whole-amount operation. Taking the amount as a parameter lets the engine
 * apply the offer to a total that item discounts have already reduced.
 */
class ICartLevelRule {
public:
    virtual ~ICartLevelRule() = default;

    /// Discount on @p amount for this order (zero if not applicable, never above amount).
    virtual domain::Money discountOnAmount(domain::Money amount,
                                           const domain::PricingContext& ctx) const = 0;

protected:
    ICartLevelRule() = default;
    ICartLevelRule(const ICartLevelRule&) = default;
    ICartLevelRule& operator=(const ICartLevelRule&) = default;
};

} // namespace rules
