#pragma once

#include <memory>
#include <optional>
#include <string>

#include "domain/Money.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/ICartLevelRule.hpp"
#include "rules/IItemLevelRule.hpp"
#include "rules/IRedeemable.hpp"

namespace rules {

/**
 * @brief Wraps any offer and adds one extra condition or limit to it, such
 *        as "happy hour only", "max ₹500 off" or "first order only".
 *
 * The decorator has the same id, priority, stackability and stage as the
 * offer it wraps, and passes every call through to it. Subclasses change
 * only what they need through three hooks:
 *  - allows() / blockedReason(): an extra condition on the whole order;
 *  - limit(): a change to each discount the wrapped offer computes.
 *
 * It implements every capability interface (IItemLevelRule, ICartLevelRule,
 * IRedeemable) and forwards each to the wrapped offer, so the engine and
 * CheckoutService treat a decorated coupon exactly like a plain one. A
 * capability the wrapped offer lacks behaves as "no discount" / "nothing
 * to record". Decorators can wrap other decorators.
 *
 * OOP ideas: Decorator pattern - behaviour is added by wrapping at run time
 * instead of writing a subclass for every combination (a happy-hour capped
 * coupon is two wrappers, not a HappyHourCappedCoupon class). Open/Closed -
 * no existing offer class changes. Liskov - a decorated rule can be used
 * anywhere a DiscountRule is expected. Template Method - the public calls
 * are fixed (final) and subclasses fill in the protected hooks.
 */
class RuleDecorator : public DiscountRule,
                      public IItemLevelRule,
                      public ICartLevelRule,
                      public IRedeemable {
public:
    /// The decorated rule is named "<inner name> [<label>]".
    /// @throws std::invalid_argument if @p inner is null.
    RuleDecorator(std::unique_ptr<DiscountRule> inner, const std::string& label);

    /// The offer being decorated.
    const DiscountRule& inner() const noexcept { return *inner_; }

    // DiscountRule: the wrapped answer combined with this decorator's hooks.
    bool isApplicable(const domain::PricingContext& ctx) const final;
    domain::Money calculateDiscount(const domain::PricingContext& ctx) const final;
    std::string rejectionReason(const domain::PricingContext& ctx) const final;
    RuleStage stage() const final { return inner_->stage(); }
    std::optional<domain::DateRange> activePeriod() const override;
    const ItemFilter* scope() const noexcept override;
    /// The wrapped offer's answer (an upper bound: decorators only narrow offers).
    std::optional<int> maxPercentOff(const domain::Product& product) const override;

    // IItemLevelRule
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const final;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const final;
    // ICartLevelRule
    domain::Money discountOnAmount(domain::Money amount,
                                   const domain::PricingContext& ctx) const final;
    // IRedeemable: passes the use on if the wrapped offer keeps a count.
    void recordUse() final;

protected:
    /// Extra condition on the order. Default: always allowed.
    virtual bool allows(const domain::PricingContext& ctx) const;
    /// Why allows() is false, for the price trace.
    virtual std::string blockedReason(const domain::PricingContext& ctx) const;
    /// Adjusts one discount computed by the wrapped offer (one line for item
    /// offers, the order for order offers). Default: unchanged.
    virtual domain::Money limit(domain::Money discount) const;

private:
    std::unique_ptr<DiscountRule> inner_;
};

} // namespace rules
