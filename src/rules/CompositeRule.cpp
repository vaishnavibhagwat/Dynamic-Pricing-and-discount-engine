#include "rules/CompositeRule.hpp"

#include <stdexcept>
#include <utility>

namespace rules {

CompositeRule::CompositeRule(std::string id, std::string name, int priority, bool stackable,
                             std::unique_ptr<ConditionNode> condition, int percent)
    : ItemLevelRule(std::move(id), std::move(name), priority, stackable),
      condition_(std::move(condition)), percent_(percent) {
    if (!condition_) throw std::invalid_argument("CompositeRule: condition must not be null");
    requireValidPercent(percent, "CompositeRule");
}

std::string CompositeRule::defaultName(int percent, const ConditionNode& condition) {
    return std::to_string(percent) + "% off when " + condition.describe();
}

bool CompositeRule::appliesToItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const {
    return condition_->evaluate(item, ctx);
}

domain::Money CompositeRule::discountForItem(const domain::CartItem& item,
                                             const domain::PricingContext& ctx) const {
    if (!appliesToItem(item, ctx)) return domain::Money::zero();
    return item.lineTotal().percentOf(percent_);
}

std::optional<int> CompositeRule::maxPercentOff(const domain::Product& product) const {
    if (!condition_->mayMatch(product)) return std::nullopt;
    return percent_;
}

std::string CompositeRule::noMatchingItemReason() const {
    return "no cart line satisfies: " + condition_->describe();
}

} // namespace rules
