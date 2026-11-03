#pragma once

#include <memory>
#include <string>

#include "rules/RuleDecorator.hpp"

namespace rules {

/**
 * @brief Makes any offer valid only on a customer's first order.
 *
 * Reads domain::Customer::ordersPlaced(), so a welcome offer is refused once
 * the customer has checked out before.
 *
 * OOP ideas: Decorator pattern - "first order only" is a reusable wrapper,
 * not a flag added to every offer class (Open/Closed, Single Responsibility).
 */
class FirstOrderRule final : public RuleDecorator {
public:
    /// @throws std::invalid_argument if @p inner is null.
    explicit FirstOrderRule(std::unique_ptr<DiscountRule> inner);

protected:
    bool allows(const domain::PricingContext& ctx) const override;
    std::string blockedReason(const domain::PricingContext& ctx) const override;
};

} // namespace rules
