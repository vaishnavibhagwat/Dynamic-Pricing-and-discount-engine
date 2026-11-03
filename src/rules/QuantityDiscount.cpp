#include "rules/QuantityDiscount.hpp"

#include <stdexcept>
#include <utility>

namespace rules {

QuantityDiscount::QuantityDiscount(std::string id, std::string name, int priority, bool stackable,
                                   int minQuantity, int percent, ItemFilter filter)
    : ItemLevelRule(std::move(id), std::move(name), priority, stackable),
      minQuantity_(minQuantity), percent_(percent), filter_(std::move(filter)) {
    if (minQuantity < 1) {
        throw std::invalid_argument("QuantityDiscount: minQuantity must be at least 1");
    }
    requireValidPercent(percent, "QuantityDiscount");
}

bool QuantityDiscount::appliesToItem(const domain::CartItem& item,
                                     const domain::PricingContext&) const {
    return filter_.matches(item.product()) && item.quantity() >= minQuantity_;
}

domain::Money QuantityDiscount::discountForItem(const domain::CartItem& item,
                                                const domain::PricingContext& ctx) const {
    if (!appliesToItem(item, ctx)) return domain::Money::zero();
    return item.lineTotal().percentOf(percent_);
}

std::optional<int> QuantityDiscount::maxPercentOff(const domain::Product& product) const {
    if (!filter_.matches(product)) return std::nullopt;
    return percent_;
}

std::string QuantityDiscount::noMatchingItemReason() const {
    return "need at least " + std::to_string(minQuantity_) + " units of one product from " +
           filter_.describe();
}

} // namespace rules
