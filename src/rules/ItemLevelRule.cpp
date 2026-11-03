#include "rules/ItemLevelRule.hpp"

namespace rules {

bool ItemLevelRule::isApplicable(const domain::PricingContext& ctx) const {
    if (!orderConditionsMet(ctx)) return false;
    for (const auto& item : ctx.cart().items()) {
        if (appliesToItem(item, ctx)) return true;
    }
    return false;
}

domain::Money ItemLevelRule::calculateDiscount(const domain::PricingContext& ctx) const {
    domain::Money total;
    if (!orderConditionsMet(ctx)) return total;
    for (const auto& item : ctx.cart().items()) {
        if (appliesToItem(item, ctx)) total += discountForItem(item, ctx);
    }
    return total;
}

std::string ItemLevelRule::rejectionReason(const domain::PricingContext& ctx) const {
    if (!orderConditionsMet(ctx)) return orderConditionsReason(ctx);
    if (!isApplicable(ctx)) return noMatchingItemReason();
    return {};
}

bool ItemLevelRule::orderConditionsMet(const domain::PricingContext&) const {
    return true;
}

std::string ItemLevelRule::orderConditionsReason(const domain::PricingContext&) const {
    return "order conditions not met";
}

std::string ItemLevelRule::noMatchingItemReason() const {
    return "no matching item in cart";
}

} // namespace rules
