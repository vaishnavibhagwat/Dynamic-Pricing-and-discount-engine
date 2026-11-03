#pragma once

#include <map>
#include <optional>
#include <string>

#include "domain/Customer.hpp"
#include "rules/CartLevelRule.hpp"
#include "rules/IItemLevelRule.hpp"

namespace rules {

/**
 * @brief Loyalty discount, percentage set per membership tier.
 *
 * OOP ideas: Interface Segregation - the class offers both small interfaces,
 * ICartLevelRule (tier % of an order amount) and IItemLevelRule (tier % of
 * one cart line), and each client uses only the one it needs. The pricing
 * engine applies it per line because its stage() is Membership, an item
 * stage. Data-driven behaviour: tiers and percentages live in a map.
 */
class MembershipDiscount final : public CartLevelRule, public IItemLevelRule {
public:
    /// @throws std::invalid_argument if the map is empty or a percent is outside 1..100.
    MembershipDiscount(std::string id, std::string name, int priority, bool stackable,
                       std::map<domain::MembershipTier, int> percentByTier);

    /// Highest tier percent, for any product (the customer's tier is not known yet).
    std::optional<int> maxPercentOff(const domain::Product& product) const override;
    RuleStage stage() const override { return RuleStage::Membership; }
    bool isApplicable(const domain::PricingContext& ctx) const override;
    domain::Money discountOnAmount(domain::Money amount,
                                   const domain::PricingContext& ctx) const override;
    std::string rejectionReason(const domain::PricingContext& ctx) const override;

    /// True if the customer's tier has a discount and the line is not free.
    bool appliesToItem(const domain::CartItem& item,
                       const domain::PricingContext& ctx) const override;
    /// Tier % of the line total.
    domain::Money discountForItem(const domain::CartItem& item,
                                  const domain::PricingContext& ctx) const override;

    const std::map<domain::MembershipTier, int>& percentByTier() const noexcept {
        return percentByTier_;
    }

private:
    std::optional<int> percentFor(domain::MembershipTier tier) const;

    std::map<domain::MembershipTier, int> percentByTier_;
};

} // namespace rules
