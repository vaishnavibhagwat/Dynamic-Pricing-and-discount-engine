#pragma once

#include "domain/CartItem.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"

namespace rules {

/**
 * @brief Interface for offers that discount individual cart lines.
 *
 * OOP idea: Interface Segregation Principle - item rules only promise
 * per-line operations; they are not forced to implement cart-level ones.
 */
class IItemLevelRule {
public:
    virtual ~IItemLevelRule() = default;

    /// True if this offer gives a discount on this line.
    virtual bool appliesToItem(const domain::CartItem& item,
                               const domain::PricingContext& ctx) const = 0;
    /// Discount on this line (zero if it does not apply).
    virtual domain::Money discountForItem(const domain::CartItem& item,
                                          const domain::PricingContext& ctx) const = 0;

protected:
    IItemLevelRule() = default;
    IItemLevelRule(const IItemLevelRule&) = default;
    IItemLevelRule& operator=(const IItemLevelRule&) = default;
};

} // namespace rules
