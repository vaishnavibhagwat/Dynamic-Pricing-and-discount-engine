#pragma once

#include <optional>
#include <string>

#include "rules/ItemFilter.hpp"
#include "rules/ItemLevelRule.hpp"

namespace rules {

/**
 * @brief Bulk deal: percentage off a line once its quantity reaches a minimum,
 *        e.g. "Buy 3 or more shirts, get 15% off".
 *
 * OOP ideas: polymorphism (same ItemLevelRule interface, different
 * per-line condition) and Open/Closed - added without touching other rules.
 */
class QuantityDiscount final : public ItemLevelRule {
public:
    /// @throws std::invalid_argument if minQuantity < 1 or percent outside 1..100.
    QuantityDiscount(std::string id, std::string name, int priority, bool stackable,
                     int minQuantity, int percent, ItemFilter filter = ItemFilter::all());

    std::optional<int> maxPercentOff(const domain::Product& product) const override;
    const ItemFilter* scope() const noexcept override { return &filter_; }
    RuleStage stage() const override { return RuleStage::Quantity; }
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;

    int minQuantity() const noexcept { return minQuantity_; }
    int percent() const noexcept { return percent_; }
    const ItemFilter& filter() const noexcept { return filter_; }

protected:
    std::string noMatchingItemReason() const override;

private:
    int minQuantity_;
    int percent_;
    ItemFilter filter_;
};

} // namespace rules
