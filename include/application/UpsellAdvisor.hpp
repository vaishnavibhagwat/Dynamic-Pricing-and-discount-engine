#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "application/GuardFactory.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"
#include "domain/Product.hpp"
#include "engine/ConflictResolver.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace application {

/// One product worth adding to the cart, and why.
struct UpsellSuggestion {
    const domain::Product* product = nullptr; ///< non-owning, from the catalog
    domain::Money extraCost;      ///< how much the bill goes up if it is added
    domain::Money standaloneCost; ///< what it costs bought on its own
    domain::Money bonus;          ///< standaloneCost - extraCost: savings unlocked by the cart
    /// Names of offers that apply only because of the combination.
    std::vector<std::string> unlockedOffers;
};

/**
 * @brief Suggests products that become cheaper because of what is already
 *        in the cart - e.g. "add a Kurta: it unlocks FESTIVE500, so it really
 *        costs you ₹680, not ₹1,180".
 *
 * For each candidate product it prices three carts through the real engine:
 * the current cart, the cart plus one unit, and that unit on its own. The
 * gap (alone - extra cost) is savings that exist only because of the
 * combination: coupon thresholds, cart-total rules, quantity breaks and so
 * on. Nothing is recorded; coupon usage is untouched.
 *
 * OOP ideas: Open/Closed and Dependency Inversion - it never inspects rule
 * types or thresholds; it only asks the engine for prices, so it already
 * understands every future offer type, decorator or text rule. Dependency
 * Injection of the guards and conflict policy keeps it testable.
 */
class UpsellAdvisor {
public:
    /// @throws std::invalid_argument if @p guards is empty or maxSuggestions is 0.
    explicit UpsellAdvisor(GuardFactory guards = defaultGuardFactory(),
                           std::size_t maxSuggestions = 3);

    /// Best suggestions first (largest bonus, then smallest extra cost).
    /// Only products with a positive bonus are returned.
    std::vector<UpsellSuggestion> suggest(const domain::PricingContext& ctx,
                                          const rules::Repository<rules::DiscountRule>& rules,
                                          const engine::ConflictResolver& resolver,
                                          const std::vector<const domain::Product*>& catalog) const;

private:
    GuardFactory guards_;
    std::size_t maxSuggestions_;
};

} // namespace application
