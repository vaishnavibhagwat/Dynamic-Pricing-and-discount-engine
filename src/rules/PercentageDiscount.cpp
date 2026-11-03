#include "rules/PercentageDiscount.hpp"

#include <utility>

namespace rules {

PercentageDiscount::PercentageDiscount(std::string id, std::string name, int priority,
                                       bool stackable, int percent, ItemFilter filter)
    : ItemLevelRule(std::move(id), std::move(name), priority, stackable),
      percent_(percent), filter_(std::move(filter)) {
    requireValidPercent(percent, "PercentageDiscount");
}

bool PercentageDiscount::appliesToItem(const domain::CartItem& item,
                                       const domain::PricingContext&) const {
    return filter_.matches(item.product());
}

domain::Money PercentageDiscount::discountForItem(const domain::CartItem& item,
                                                  const domain::PricingContext& ctx) const {
    if (!appliesToItem(item, ctx)) return domain::Money::zero();
    return item.lineTotal().percentOf(percent_);
}

std::optional<int> PercentageDiscount::maxPercentOff(const domain::Product& product) const {
    if (!filter_.matches(product)) return std::nullopt;
    return percent_;
}

std::string PercentageDiscount::noMatchingItemReason() const {
    return "no item from " + filter_.describe() + " in cart";
}

} // namespace rules
