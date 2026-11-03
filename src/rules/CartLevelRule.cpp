#include "rules/CartLevelRule.hpp"

namespace rules {

domain::Money CartLevelRule::calculateDiscount(const domain::PricingContext& ctx) const {
    if (!isApplicable(ctx)) return domain::Money::zero();
    return discountOnAmount(ctx.cart().subtotal(), ctx);
}

} // namespace rules
