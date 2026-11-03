#pragma once

#include <memory>
#include <string>
#include <vector>

#include "domain/PricingContext.hpp"
#include "engine/ConflictResolver.hpp"
#include "engine/GstCalculator.hpp"
#include "engine/PriceGuard.hpp"
#include "engine/PricingResult.hpp"
#include "engine/TraceObserver.hpp"
#include "engine/TracePublisher.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace engine {

/**
 * @brief Turns a cart and a set of rules into a fully priced, traced bill.
 *
 * Pipeline: item offers by stage (Expiry, Seasonal, Percentage, Quantity,
 * Membership) per line -> guards per line -> order offers (Coupon,
 * BankOffer) -> split order offers over lines -> GST per line. Every
 * decision (rule applied/rejected, guard passed/adjusted) is published to
 * TraceObservers; a PriceTrace observer collects them into the result.
 *
 * OOP ideas: Dependency Injection and Dependency Inversion - the conflict
 * policy is passed in by reference and the guards as owned objects, and the
 * engine only talks to abstractions (DiscountRule, IItemLevelRule,
 * ICartLevelRule, ConflictResolver, PriceGuard, TraceObserver). Observer
 * pattern - listeners such as a live log subscribe without the engine
 * knowing their concrete type, so it never depends on presentation code.
 * Open/Closed - a new offer,
 * guard or resolver is a new class; this one does not change.
 */
class PricingEngine {
public:
    /// The standard guards: DiscountCapPolicy(60), MarginGuard(8), MrpCeilingPolicy.
    static std::vector<std::unique_ptr<PriceGuard>> defaultGuards();

    /// @p resolver must outlive the engine.
    /// @throws std::invalid_argument if a guard is null.
    explicit PricingEngine(const ConflictResolver& resolver,
                           std::vector<std::unique_ptr<PriceGuard>> guards = defaultGuards());
    // Binding to a temporary resolver would leave a dangling reference.
    explicit PricingEngine(ConflictResolver&&,
                           std::vector<std::unique_ptr<PriceGuard>> = {}) = delete;

    /// Prices the order in @p ctx using @p rules (non-owning, any order).
    /// @throws std::invalid_argument if a rule is null.
    PricingResult price(const domain::PricingContext& ctx,
                        const std::vector<const rules::DiscountRule*>& rules) const;
    PricingResult price(const domain::PricingContext& ctx,
                        const rules::Repository<rules::DiscountRule>& rules) const;

    /// Every later price() call also sends its decisions to @p observer.
    /// Not owned: it must outlive the engine or be unsubscribed first.
    void subscribe(TraceObserver& observer) { publisher_.subscribe(observer); }
    void unsubscribe(const TraceObserver& observer) { publisher_.unsubscribe(observer); }

    const ConflictResolver& resolver() const noexcept { return resolver_; }
    std::vector<std::string> guardNames() const;

private:
    struct LineWork; // per-line scratch data, defined in the .cpp

    void priceItemOffers(const domain::PricingContext& ctx,
                         const std::vector<const rules::DiscountRule*>& itemRules,
                         std::vector<LineWork>& lines, const TracePublisher& trace) const;
    void applyGuards(std::vector<LineWork>& lines, const TracePublisher& trace) const;
    std::vector<AppliedDiscount> priceCartOffers(
        const domain::PricingContext& ctx, const std::vector<const rules::DiscountRule*>& cartRules,
        domain::Money amount, const TracePublisher& trace) const;

    const ConflictResolver& resolver_;
    std::vector<std::unique_ptr<PriceGuard>> guards_;
    GstCalculator gst_;
    TracePublisher publisher_;
};

} // namespace engine
