#include "rules/BankOfferDiscount.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rules {

using domain::Money;

BankOfferDiscount::BankOfferDiscount(std::string id, std::string name, int priority,
                                     bool stackable, std::string bank, int percent, Money cap,
                                     Money minTransaction)
    : CartLevelRule(std::move(id), std::move(name), priority, stackable),
      bank_(std::move(bank)), percent_(percent), cap_(cap), minTransaction_(minTransaction) {
    if (bank_.empty()) throw std::invalid_argument("BankOfferDiscount: bank must not be empty");
    requireValidPercent(percent, "BankOfferDiscount");
    if (cap_ <= Money::zero()) throw std::invalid_argument("BankOfferDiscount: cap must be positive");
    if (minTransaction_.isNegative()) {
        throw std::invalid_argument("BankOfferDiscount: minTransaction must not be negative");
    }
}

bool BankOfferDiscount::isApplicable(const domain::PricingContext& ctx) const {
    return rejectionReason(ctx).empty();
}

std::string BankOfferDiscount::rejectionReason(const domain::PricingContext& ctx) const {
    const auto& paidWith = ctx.paymentBank();
    if (!paidWith) return "no bank card used for payment";
    if (!equalsIgnoreCase(*paidWith, bank_)) return "offer is for " + bank_ + ", paid with " + *paidWith;
    if (ctx.cart().empty()) return "cart is empty";
    const Money subtotal = ctx.cart().subtotal();
    if (subtotal < minTransaction_) {
        return "cart value " + subtotal.toString() + " is below minimum " +
               minTransaction_.toString();
    }
    return {};
}

Money BankOfferDiscount::discountOnAmount(Money amount, const domain::PricingContext& ctx) const {
    if (!isApplicable(ctx) || amount <= Money::zero()) return Money::zero();
    return std::min(amount.percentOf(percent_), cap_);
}

} // namespace rules
