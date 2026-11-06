#include "application/UpsellAdvisor.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "domain/Cart.hpp"
#include "domain/CartItem.hpp"
#include "domain/Perishable.hpp"
#include "engine/PricingEngine.hpp"

namespace application {

using domain::Money;

namespace {

/// Ids of every offer the result actually gave, item or order level.
std::set<std::string> appliedIds(const engine::PricingResult& result) {
    std::set<std::string> ids;
    for (const auto& line : result.lines()) {
        for (const auto& d : line.itemDiscounts) ids.insert(d.ruleId);
    }
    for (const auto& d : result.cartDiscounts()) ids.insert(d.ruleId);
    return ids;
}

/// Same customer, date, coupon, bank and time as @p ctx, but a different cart.
domain::PricingContext withCart(const domain::PricingContext& ctx, const domain::Cart& cart) {
    return domain::PricingContext(cart, ctx.customer(), ctx.orderDate(), ctx.couponCode(),
                                  ctx.paymentBank(), ctx.orderTime());
}

} // namespace

UpsellAdvisor::UpsellAdvisor(GuardFactory guards, std::size_t maxSuggestions)
    : guards_(std::move(guards)), maxSuggestions_(maxSuggestions) {
    if (!guards_) throw std::invalid_argument("UpsellAdvisor: guard factory must not be empty");
    if (maxSuggestions_ == 0) throw std::invalid_argument("UpsellAdvisor: maxSuggestions must be > 0");
}

std::vector<UpsellSuggestion> UpsellAdvisor::suggest(
    const domain::PricingContext& ctx, const rules::Repository<rules::DiscountRule>& rules,
    const engine::ConflictResolver& resolver,
    const std::vector<const domain::Product*>& catalog) const {
    const engine::PricingEngine engine(resolver, guards_());
    const engine::PricingResult now = engine.price(ctx, rules);
    const std::set<std::string> appliedNow = appliedIds(now);

    std::vector<UpsellSuggestion> found;
    for (const domain::Product* product : catalog) {
        if (product == nullptr) throw std::invalid_argument("UpsellAdvisor: null product");
        if (domain::isExpiredOn(*product, ctx.orderDate())) continue; // cannot be sold

        domain::Cart bigger = ctx.cart();
        try {
            bigger += domain::CartItem(*product, 1);
        } catch (const std::invalid_argument&) {
            continue; // that line is already at the most units allowed
        }
        domain::Cart alone;
        alone += domain::CartItem(*product, 1);

        const engine::PricingResult withIt = engine.price(withCart(ctx, bigger), rules);
        const engine::PricingResult byItself = engine.price(withCart(ctx, alone), rules);

        UpsellSuggestion s;
        s.product = product;
        s.extraCost = withIt.grandTotal() - now.grandTotal();
        s.standaloneCost = byItself.grandTotal();
        s.bonus = s.standaloneCost - s.extraCost;
        if (s.bonus <= Money::zero()) continue;

        // Offers that need both the cart and the new item.
        const std::set<std::string> alreadyAlone = appliedIds(byItself);
        std::set<std::string> named;
        const auto note = [&](const std::string& id, const std::string& name) {
            if (!appliedNow.count(id) && !alreadyAlone.count(id) && named.insert(id).second) {
                s.unlockedOffers.push_back(name);
            }
        };
        for (const auto& line : withIt.lines()) {
            for (const auto& d : line.itemDiscounts) note(d.ruleId, d.ruleName);
        }
        for (const auto& d : withIt.cartDiscounts()) note(d.ruleId, d.ruleName);
        found.push_back(std::move(s));
    }

    std::sort(found.begin(), found.end(), [](const UpsellSuggestion& a, const UpsellSuggestion& b) {
        return std::make_tuple(b.bonus, a.extraCost, a.product->id()) <
               std::make_tuple(a.bonus, b.extraCost, b.product->id());
    });
    if (found.size() > maxSuggestions_) found.resize(maxSuggestions_);
    return found;
}

} // namespace application
