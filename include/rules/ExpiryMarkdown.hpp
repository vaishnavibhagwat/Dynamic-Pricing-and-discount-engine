#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/Date.hpp"
#include "rules/ItemLevelRule.hpp"

namespace rules {

/**
 * @brief Markdown on perishable items that grows as the expiry date gets closer,
 *        e.g. 15% with a week left, 30% with 3 days, 50% on the last day.
 *
 * OOP ideas: Dependency Inversion - it asks a product for the
 * domain::Perishable capability instead of naming the concrete Grocery
 * class, so any new perishable product is marked down with no change here
 * (Open/Closed). Encapsulation of the tier table behind a single
 * percentFor() query. Expired items are never discounted (they must not be
 * sold).
 */
class ExpiryMarkdown final : public ItemLevelRule {
public:
    /// "If at most maxDaysLeft days remain, take percent off."
    struct Tier {
        int maxDaysLeft;
        int percent;
    };

    /// @throws std::invalid_argument if tiers is empty, a maxDaysLeft is negative
    ///         or repeated, or a percent is outside 1..100.
    ExpiryMarkdown(std::string id, std::string name, int priority, bool stackable,
                   std::vector<Tier> tiers);

    /// Highest tier percent for perishable products; nullopt for every other product.
    std::optional<int> maxPercentOff(const domain::Product& product) const override;
    RuleStage stage() const override { return RuleStage::Expiry; }
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;

    /// Markdown percent for a grocery with @p daysLeft days to expiry, if any tier matches.
    std::optional<int> percentFor(long daysLeft) const noexcept;
    /// Tiers sorted by maxDaysLeft, tightest first.
    const std::vector<Tier>& tiers() const noexcept { return tiers_; }

protected:
    std::string noMatchingItemReason() const override;

private:
    std::optional<int> percentForItem(const domain::CartItem& item, const domain::Date& today) const;

    std::vector<Tier> tiers_;
};

} // namespace rules
