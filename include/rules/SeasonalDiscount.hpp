#pragma once

#include <optional>
#include <string>

#include "domain/Date.hpp"
#include "domain/DateRange.hpp"
#include "rules/ItemFilter.hpp"
#include "rules/ItemLevelRule.hpp"

namespace rules {

/**
 * @brief A sale valid only between two dates (inclusive), e.g. a Diwali sale.
 *
 * OOP idea: Template Method - overrides the orderConditionsMet() hook of
 * ItemLevelRule to add a date check, reusing all the per-line logic.
 */
class SeasonalDiscount final : public ItemLevelRule {
public:
    /// @throws std::invalid_argument if start > end or percent outside 1..100.
    SeasonalDiscount(std::string id, std::string name, int priority, bool stackable,
                     domain::Date start, domain::Date end, int percent,
                     ItemFilter filter = ItemFilter::all());

    std::optional<int> maxPercentOff(const domain::Product& product) const override;
    const ItemFilter* scope() const noexcept override { return &filter_; }
    std::optional<domain::DateRange> activePeriod() const override {
        return domain::DateRange(start_, end_);
    }
    RuleStage stage() const override { return RuleStage::Seasonal; }
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;

    const domain::Date& start() const noexcept { return start_; }
    const domain::Date& end() const noexcept { return end_; }
    int percent() const noexcept { return percent_; }
    const ItemFilter& filter() const noexcept { return filter_; }
    /// True if @p day is within [start, end].
    bool isActiveOn(const domain::Date& day) const noexcept { return start_ <= day && day <= end_; }

protected:
    bool orderConditionsMet(const domain::PricingContext& ctx) const override;
    std::string orderConditionsReason(const domain::PricingContext& ctx) const override;
    std::string noMatchingItemReason() const override;

private:
    domain::Date start_;
    domain::Date end_;
    int percent_;
    ItemFilter filter_;
};

} // namespace rules
