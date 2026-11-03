#pragma once

#include <memory>
#include <optional>
#include <string>

#include "rules/ConditionNode.hpp"
#include "rules/ItemLevelRule.hpp"

namespace rules {

/**
 * @brief An offer written in the text rule language: "N% off every cart line
 *        for which this condition tree is true".
 *
 * OOP ideas: composition over inheritance - instead of a new subclass for
 * every combination of conditions, one class owns a ConditionNode tree
 * (Composite pattern) and asks it about each line. Template Method - it only
 * fills in ItemLevelRule's per-line hooks. Liskov Substitution - the engine
 * and validator use it as any other DiscountRule, so PricingEngine never
 * changes (Open/Closed).
 */
class CompositeRule final : public ItemLevelRule {
public:
    /// @throws std::invalid_argument if condition is null or percent is not 1..100.
    CompositeRule(std::string id, std::string name, int priority, bool stackable,
                  std::unique_ptr<ConditionNode> condition, int percent);

    /// e.g. "12% off when category == Electronics AND qty >= 2"
    static std::string defaultName(int percent, const ConditionNode& condition);

    RuleStage stage() const override { return RuleStage::Percentage; }
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;
    /// percent if the condition could match a line of @p product, else nullopt.
    std::optional<int> maxPercentOff(const domain::Product& product) const override;

    const ConditionNode& condition() const noexcept { return *condition_; }
    int percent() const noexcept { return percent_; }

protected:
    std::string noMatchingItemReason() const override;

private:
    std::unique_ptr<ConditionNode> condition_;
    int percent_;
};

} // namespace rules
