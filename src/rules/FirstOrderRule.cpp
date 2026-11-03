#include "rules/FirstOrderRule.hpp"

#include <utility>

#include "domain/Customer.hpp"
#include "domain/PricingContext.hpp"

namespace rules {

FirstOrderRule::FirstOrderRule(std::unique_ptr<DiscountRule> inner)
    : RuleDecorator(std::move(inner), "first order only") {}

bool FirstOrderRule::allows(const domain::PricingContext& ctx) const {
    return ctx.customer().isFirstOrder();
}

std::string FirstOrderRule::blockedReason(const domain::PricingContext& ctx) const {
    const int n = ctx.customer().ordersPlaced();
    return "first order only; " + ctx.customer().name() + " has placed " + std::to_string(n) +
           (n == 1 ? " order" : " orders");
}

} // namespace rules
