#include "application/CheckoutService.hpp"

#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "engine/PricingEngine.hpp"
#include "rules/IRedeemable.hpp"

namespace application {

CheckoutService::CheckoutService(rules::Repository<rules::DiscountRule>& rules,
                                 GuardFactory guards)
    : rules_(rules), guards_(std::move(guards)) {
    if (!guards_) throw std::invalid_argument("CheckoutService: guard factory must not be empty");
}

engine::PricingResult CheckoutService::quote(const domain::PricingContext& ctx,
                                             const engine::ConflictResolver& resolver) const {
    const engine::PricingEngine engine(resolver, guards_());
    return engine.price(ctx, rules_);
}

engine::PricingResult CheckoutService::checkout(const domain::PricingContext& ctx,
                                                const engine::ConflictResolver& resolver) {
    engine::PricingResult result = quote(ctx, resolver);

    // Each offer that was given counts once per order, even if it covered several lines.
    std::vector<std::string> used;
    std::set<std::string> seen;
    const auto note = [&](const std::string& id) {
        if (seen.insert(id).second) used.push_back(id);
    };
    for (const auto& line : result.lines()) {
        for (const auto& d : line.itemDiscounts) note(d.ruleId);
    }
    for (const auto& d : result.cartDiscounts()) note(d.ruleId);

    for (const auto& id : used) {
        if (auto* redeemable = dynamic_cast<rules::IRedeemable*>(rules_.findById(id))) {
            redeemable->recordUse();
        }
    }
    return result;
}

engine::PricingResult CheckoutService::checkout(const domain::PricingContext& ctx,
                                                const engine::ConflictResolver& resolver,
                                                domain::Customer& buyer) {
    if (&buyer != &ctx.customer()) {
        throw std::invalid_argument("CheckoutService: buyer must be the customer being priced");
    }
    engine::PricingResult result = checkout(ctx, resolver);
    buyer.recordOrder();
    return result;
}

} // namespace application
