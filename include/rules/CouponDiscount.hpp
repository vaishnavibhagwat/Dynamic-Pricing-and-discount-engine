#pragma once

#include <optional>
#include <string>

#include "domain/Money.hpp"
#include "rules/CartLevelRule.hpp"
#include "rules/IRedeemable.hpp"

namespace rules {

/**
 * @brief A coupon code with a minimum cart value and a usage limit,
 *        giving either a percentage (optionally capped) or a flat amount off.
 *
 * OOP ideas: encapsulation of mutable state - the usage count only changes
 * through recordUse(), which checkout calls after an order is placed, so
 * evaluating the rule (const methods) never has side effects. Implements
 * IRedeemable, so checkout records the use without knowing this class. Benefit is a
 * small value object built with named constructors.
 */
class CouponDiscount final : public CartLevelRule, public IRedeemable {
public:
    /// What the coupon gives: either a percent (with optional cap) or a flat amount.
    class Benefit {
    public:
        /// @throws std::invalid_argument if percent outside 1..100 or cap <= 0.
        static Benefit percent(int percent, std::optional<domain::Money> cap = std::nullopt);
        /// @throws std::invalid_argument if amount <= 0.
        static Benefit flat(domain::Money amount);

        /// Discount on @p amount, never more than the amount itself.
        domain::Money apply(domain::Money amount) const noexcept;
        std::string describe() const;

    private:
        Benefit(std::optional<int> percent, std::optional<domain::Money> cap,
                std::optional<domain::Money> flat)
            : percent_(percent), cap_(cap), flat_(flat) {}

        std::optional<int> percent_;
        std::optional<domain::Money> cap_;
        std::optional<domain::Money> flat_;
    };

    /// @throws std::invalid_argument if code is empty, minCartValue < 0 or usageLimit < 1.
    CouponDiscount(std::string id, std::string name, int priority, bool stackable,
                   std::string code, domain::Money minCartValue, int usageLimit, Benefit benefit);

    RuleStage stage() const override { return RuleStage::Coupon; }
    bool isApplicable(const domain::PricingContext& ctx) const override;
    domain::Money discountOnAmount(domain::Money amount,
                                   const domain::PricingContext& ctx) const override;
    std::string rejectionReason(const domain::PricingContext& ctx) const override;

    /// Counts one redemption. @throws std::logic_error if the limit is already reached.
    void recordUse() override;

    const std::string& code() const noexcept { return code_; }
    domain::Money minCartValue() const noexcept { return minCartValue_; }
    int usageLimit() const noexcept { return usageLimit_; }
    int usesSoFar() const noexcept { return usesSoFar_; }
    int usesRemaining() const noexcept { return usageLimit_ - usesSoFar_; }
    const Benefit& benefit() const noexcept { return benefit_; }

private:
    std::string code_;
    domain::Money minCartValue_;
    int usageLimit_;
    int usesSoFar_ = 0;
    Benefit benefit_;
};

} // namespace rules
