#pragma once

#include <optional>
#include <string>

#include "rules/ItemFilter.hpp"
#include "rules/ItemLevelRule.hpp"

namespace rules {

/**
 * @brief A flat percentage off every matching product, e.g. "10% off Furniture".
 *
 * OOP ideas: inheritance and polymorphism - the simplest ItemLevelRule; it
 * only supplies the two per-line hooks. Uses composition (ItemFilter) to
 * decide which products are covered.
 */
class PercentageDiscount final : public ItemLevelRule {
public:
    /// @throws std::invalid_argument unless 1 <= percent <= 100.
    PercentageDiscount(std::string id, std::string name, int priority, bool stackable,
                       int percent, ItemFilter filter = ItemFilter::all());

    std::optional<int> maxPercentOff(const domain::Product& product) const override;
    const ItemFilter* scope() const noexcept override { return &filter_; }
    RuleStage stage() const override { return RuleStage::Percentage; }
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;

    int percent() const noexcept { return percent_; }
    const ItemFilter& filter() const noexcept { return filter_; }

protected:
    std::string noMatchingItemReason() const override;

private:
    int percent_;
    ItemFilter filter_;
};

} // namespace rules
