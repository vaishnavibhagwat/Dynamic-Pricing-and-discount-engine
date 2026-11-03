#pragma once

#include <string>

#include "rules/DiscountRule.hpp"
#include "rules/IItemLevelRule.hpp"

namespace rules {

/**
 * @brief Base for offers that work line by line (sales, quantity deals, markdowns).
 *
 * OOP ideas: Template Method pattern - calculateDiscount() and
 * isApplicable() are written once here in terms of the per-line hooks
 * appliesToItem() / discountForItem(), which subclasses fill in. Multiple
 * inheritance of an implementation base (DiscountRule) and an interface
 * (IItemLevelRule).
 */
class ItemLevelRule : public DiscountRule, public IItemLevelRule {
public:
    using DiscountRule::DiscountRule;

    /// True if order-wide conditions hold and at least one line matches.
    bool isApplicable(const domain::PricingContext& ctx) const final;
    /// Sum of discountForItem() over matching lines (zero if not applicable).
    domain::Money calculateDiscount(const domain::PricingContext& ctx) const final;
    std::string rejectionReason(const domain::PricingContext& ctx) const override;

protected:
    /// Hook for conditions on the whole order (e.g. date range). Default: always true.
    virtual bool orderConditionsMet(const domain::PricingContext& ctx) const;
    /// Reason used when orderConditionsMet() is false.
    virtual std::string orderConditionsReason(const domain::PricingContext& ctx) const;
    /// Reason used when no line matches.
    virtual std::string noMatchingItemReason() const;
};

} // namespace rules
