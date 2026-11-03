#include "rules/SeasonalDiscount.hpp"

#include <stdexcept>
#include <utility>

namespace rules {

SeasonalDiscount::SeasonalDiscount(std::string id, std::string name, int priority, bool stackable,
                                   domain::Date start, domain::Date end, int percent,
                                   ItemFilter filter)
    : ItemLevelRule(std::move(id), std::move(name), priority, stackable),
      start_(start), end_(end), percent_(percent), filter_(std::move(filter)) {
    if (end_ < start_) {
        throw std::invalid_argument("SeasonalDiscount: start " + start_.toString() +
                                    " is after end " + end_.toString());
    }
    requireValidPercent(percent, "SeasonalDiscount");
}

bool SeasonalDiscount::appliesToItem(const domain::CartItem& item,
                                     const domain::PricingContext&) const {
    return filter_.matches(item.product());
}

domain::Money SeasonalDiscount::discountForItem(const domain::CartItem& item,
                                                const domain::PricingContext& ctx) const {
    if (!orderConditionsMet(ctx) || !appliesToItem(item, ctx)) return domain::Money::zero();
    return item.lineTotal().percentOf(percent_);
}

bool SeasonalDiscount::orderConditionsMet(const domain::PricingContext& ctx) const {
    return isActiveOn(ctx.orderDate());
}

std::string SeasonalDiscount::orderConditionsReason(const domain::PricingContext& ctx) const {
    return "order date " + ctx.orderDate().toString() + " is outside " + start_.toString() +
           " to " + end_.toString();
}

std::optional<int> SeasonalDiscount::maxPercentOff(const domain::Product& product) const {
    if (!filter_.matches(product)) return std::nullopt;
    return percent_;
}

std::string SeasonalDiscount::noMatchingItemReason() const {
    return "no item from " + filter_.describe() + " in cart";
}

} // namespace rules
