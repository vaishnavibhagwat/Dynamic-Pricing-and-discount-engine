#pragma once

#include "rules/DiscountRule.hpp"
#include "rules/ICartLevelRule.hpp"

namespace rules {

/**
 * @brief Base for offers on the whole order (coupons, bank offers, membership).
 *
 * OOP ideas: Template Method pattern - calculateDiscount() is defined once
 * as discountOnAmount(cart subtotal); subclasses supply discountOnAmount()
 * and isApplicable(). Combines DiscountRule with the ICartLevelRule interface.
 */
class CartLevelRule : public DiscountRule, public ICartLevelRule {
public:
    using DiscountRule::DiscountRule;

    /// discountOnAmount(cart subtotal) when applicable, else zero.
    domain::Money calculateDiscount(const domain::PricingContext& ctx) const final;
};

} // namespace rules
