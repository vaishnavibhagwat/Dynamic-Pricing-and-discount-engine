#include "rules/RuleDecorator.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "domain/Cart.hpp"
#include "domain/PricingContext.hpp"
#include "rules/RuleStage.hpp"

namespace rules {

using domain::Money;

namespace {

/// Checks @p inner before the base-class constructor reads from it.
const DiscountRule& checked(const std::unique_ptr<DiscountRule>& inner) {
    if (!inner) throw std::invalid_argument("RuleDecorator: rule to decorate must not be null");
    return *inner;
}

} // namespace

RuleDecorator::RuleDecorator(std::unique_ptr<DiscountRule> inner, const std::string& label)
    : DiscountRule(checked(inner).id(), checked(inner).name() + " [" + label + "]",
                   checked(inner).priority(), checked(inner).stackable()),
      inner_(std::move(inner)) {}

bool RuleDecorator::isApplicable(const domain::PricingContext& ctx) const {
    return allows(ctx) && inner_->isApplicable(ctx);
}

std::string RuleDecorator::rejectionReason(const domain::PricingContext& ctx) const {
    if (!allows(ctx)) return blockedReason(ctx);
    return inner_->rejectionReason(ctx);
}

Money RuleDecorator::calculateDiscount(const domain::PricingContext& ctx) const {
    if (!isApplicable(ctx)) return Money::zero();
    if (!isItemStage(stage())) return limit(inner_->calculateDiscount(ctx));
    // Item offers: the hook works per line, so add up the adjusted lines.
    Money total;
    for (const auto& item : ctx.cart().items()) {
        if (appliesToItem(item, ctx)) total += discountForItem(item, ctx);
    }
    return total;
}

std::optional<domain::DateRange> RuleDecorator::activePeriod() const {
    return inner_->activePeriod();
}

const ItemFilter* RuleDecorator::scope() const noexcept {
    return inner_->scope();
}

std::optional<int> RuleDecorator::maxPercentOff(const domain::Product& product) const {
    return inner_->maxPercentOff(product);
}

bool RuleDecorator::appliesToItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const {
    const auto* perItem = dynamic_cast<const IItemLevelRule*>(inner_.get());
    return perItem != nullptr && perItem->appliesToItem(item, ctx);
}

Money RuleDecorator::discountForItem(const domain::CartItem& item,
                                     const domain::PricingContext& ctx) const {
    const auto* perItem = dynamic_cast<const IItemLevelRule*>(inner_.get());
    if (perItem == nullptr) return Money::zero();
    return std::max(Money::zero(), limit(perItem->discountForItem(item, ctx)));
}

Money RuleDecorator::discountOnAmount(Money amount, const domain::PricingContext& ctx) const {
    const auto* onOrder = dynamic_cast<const ICartLevelRule*>(inner_.get());
    if (onOrder == nullptr) return Money::zero();
    return std::clamp(limit(onOrder->discountOnAmount(amount, ctx)), Money::zero(), amount);
}

void RuleDecorator::recordUse() {
    if (auto* redeemable = dynamic_cast<IRedeemable*>(inner_.get())) redeemable->recordUse();
}

bool RuleDecorator::allows(const domain::PricingContext&) const {
    return true;
}

std::string RuleDecorator::blockedReason(const domain::PricingContext&) const {
    return {};
}

Money RuleDecorator::limit(Money discount) const {
    return discount;
}

} // namespace rules
