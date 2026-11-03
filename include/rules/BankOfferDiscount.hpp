#pragma once

#include <string>

#include "domain/Money.hpp"
#include "rules/CartLevelRule.hpp"

namespace rules {

/**
 * @brief Card offer: percentage off when paying with a given bank, capped at
 *        a rupee limit, e.g. "HDFC 10% instant discount up to Rs 1500".
 *
 * OOP ideas: polymorphism through CartLevelRule, and Liskov Substitution -
 * it works anywhere a DiscountRule is expected; the engine never needs to
 * know that it reads the payment bank from the context.
 */
class BankOfferDiscount final : public CartLevelRule {
public:
    /// @throws std::invalid_argument if bank is empty, percent outside 1..100,
    ///         cap <= 0 or minTransaction < 0.
    BankOfferDiscount(std::string id, std::string name, int priority, bool stackable,
                      std::string bank, int percent, domain::Money cap,
                      domain::Money minTransaction = domain::Money::zero());

    RuleStage stage() const override { return RuleStage::BankOffer; }
    bool isApplicable(const domain::PricingContext& ctx) const override;
    domain::Money discountOnAmount(domain::Money amount,
                                   const domain::PricingContext& ctx) const override;
    std::string rejectionReason(const domain::PricingContext& ctx) const override;

    const std::string& bank() const noexcept { return bank_; }
    int percent() const noexcept { return percent_; }
    domain::Money cap() const noexcept { return cap_; }
    domain::Money minTransaction() const noexcept { return minTransaction_; }

private:
    std::string bank_;
    int percent_;
    domain::Money cap_;
    domain::Money minTransaction_;
};

} // namespace rules
