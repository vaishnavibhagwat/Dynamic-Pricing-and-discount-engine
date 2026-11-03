#include "rules/CouponDiscount.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rules {

using domain::Money;

CouponDiscount::Benefit CouponDiscount::Benefit::percent(int percent, std::optional<Money> cap) {
    requireValidPercent(percent, "CouponDiscount");
    if (cap && *cap <= Money::zero()) {
        throw std::invalid_argument("CouponDiscount: cap must be positive");
    }
    return Benefit(percent, cap, std::nullopt);
}

CouponDiscount::Benefit CouponDiscount::Benefit::flat(Money amount) {
    if (amount <= Money::zero()) {
        throw std::invalid_argument("CouponDiscount: flat amount must be positive");
    }
    return Benefit(std::nullopt, std::nullopt, amount);
}

Money CouponDiscount::Benefit::apply(Money amount) const noexcept {
    if (amount <= Money::zero()) return Money::zero();
    Money discount = flat_ ? *flat_ : amount.percentOf(*percent_);
    if (cap_) discount = std::min(discount, *cap_);
    return std::min(discount, amount);
}

std::string CouponDiscount::Benefit::describe() const {
    if (flat_) return flat_->toString() + " off";
    std::string text = std::to_string(*percent_) + "% off";
    if (cap_) text += " up to " + cap_->toString();
    return text;
}

CouponDiscount::CouponDiscount(std::string id, std::string name, int priority, bool stackable,
                               std::string code, Money minCartValue, int usageLimit,
                               Benefit benefit)
    : CartLevelRule(std::move(id), std::move(name), priority, stackable),
      code_(std::move(code)), minCartValue_(minCartValue), usageLimit_(usageLimit),
      benefit_(std::move(benefit)) {
    if (code_.empty()) throw std::invalid_argument("CouponDiscount: code must not be empty");
    if (minCartValue_.isNegative()) {
        throw std::invalid_argument("CouponDiscount: minCartValue must not be negative");
    }
    if (usageLimit_ < 1) throw std::invalid_argument("CouponDiscount: usageLimit must be >= 1");
}

bool CouponDiscount::isApplicable(const domain::PricingContext& ctx) const {
    return rejectionReason(ctx).empty();
}

std::string CouponDiscount::rejectionReason(const domain::PricingContext& ctx) const {
    const auto& entered = ctx.couponCode();
    if (!entered) return "no coupon code entered";
    if (!equalsIgnoreCase(*entered, code_)) return "coupon code '" + *entered + "' does not match";
    if (ctx.cart().empty()) return "cart is empty";
    const Money subtotal = ctx.cart().subtotal();
    if (subtotal < minCartValue_) {
        return "cart value " + subtotal.toString() + " is below minimum " + minCartValue_.toString();
    }
    if (usesSoFar_ >= usageLimit_) return "usage limit of " + std::to_string(usageLimit_) + " reached";
    return {};
}

Money CouponDiscount::discountOnAmount(Money amount, const domain::PricingContext& ctx) const {
    if (!isApplicable(ctx)) return Money::zero();
    return benefit_.apply(amount);
}

void CouponDiscount::recordUse() {
    if (usesSoFar_ >= usageLimit_) {
        throw std::logic_error("CouponDiscount: usage limit reached for " + code_);
    }
    ++usesSoFar_;
}

} // namespace rules
