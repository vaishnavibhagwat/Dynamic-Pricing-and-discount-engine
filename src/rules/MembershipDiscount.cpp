#include "rules/MembershipDiscount.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rules {

MembershipDiscount::MembershipDiscount(std::string id, std::string name, int priority,
                                       bool stackable,
                                       std::map<domain::MembershipTier, int> percentByTier)
    : CartLevelRule(std::move(id), std::move(name), priority, stackable),
      percentByTier_(std::move(percentByTier)) {
    if (percentByTier_.empty()) {
        throw std::invalid_argument("MembershipDiscount: needs at least one tier");
    }
    for (const auto& entry : percentByTier_) requireValidPercent(entry.second, "MembershipDiscount");
}

std::optional<int> MembershipDiscount::maxPercentOff(const domain::Product&) const {
    int best = 0;
    for (const auto& entry : percentByTier_) best = std::max(best, entry.second);
    return best;
}

std::optional<int> MembershipDiscount::percentFor(domain::MembershipTier tier) const {
    const auto it = percentByTier_.find(tier);
    if (it == percentByTier_.end()) return std::nullopt;
    return it->second;
}

bool MembershipDiscount::isApplicable(const domain::PricingContext& ctx) const {
    return !ctx.cart().empty() && percentFor(ctx.customer().tier()).has_value();
}

domain::Money MembershipDiscount::discountOnAmount(domain::Money amount,
                                                   const domain::PricingContext& ctx) const {
    const auto percent = percentFor(ctx.customer().tier());
    if (!percent || amount <= domain::Money::zero()) return domain::Money::zero();
    return amount.percentOf(*percent);
}

std::string MembershipDiscount::rejectionReason(const domain::PricingContext& ctx) const {
    if (ctx.cart().empty()) return "cart is empty";
    if (!isApplicable(ctx)) {
        return "no membership discount for " + domain::toString(ctx.customer().tier()) + " tier";
    }
    return {};
}

bool MembershipDiscount::appliesToItem(const domain::CartItem& item,
                                       const domain::PricingContext& ctx) const {
    return percentFor(ctx.customer().tier()).has_value() &&
           item.lineTotal() > domain::Money::zero();
}

domain::Money MembershipDiscount::discountForItem(const domain::CartItem& item,
                                                  const domain::PricingContext& ctx) const {
    return discountOnAmount(item.lineTotal(), ctx);
}

} // namespace rules
