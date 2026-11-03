#include "application/DiscountOptimizer.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>

#include "engine/PricingEngine.hpp"
#include "engine/StackingResolver.hpp"
#include "engine/TraceEntry.hpp"

namespace application {

using domain::Money;
using engine::PricingResult;
using engine::TraceEntry;
using rules::DiscountRule;

namespace {

constexpr const char* kStep = "Optimizer";

/// True if @p a is a better choice than @p b.
bool better(const OfferCombination& a, const OfferCombination& b) {
    if (a.passedGuards != b.passedGuards) return a.passedGuards;
    if (a.saving != b.saving) return a.saving > b.saving;
    if (a.ruleIds.size() != b.ruleIds.size()) return a.ruleIds.size() < b.ruleIds.size();
    return a.ruleIds < b.ruleIds;
}

/// Describes how @p result came out for the offers in @p chosen.
OfferCombination describe(std::vector<const DiscountRule*> chosen, const PricingResult& result,
                          Money baselineTotal) {
    std::sort(chosen.begin(), chosen.end(),
              [](const DiscountRule* a, const DiscountRule* b) { return a->id() < b->id(); });
    OfferCombination combo;
    for (const auto* r : chosen) {
        combo.ruleIds.push_back(r->id());
        combo.ruleNames.push_back(r->name());
    }
    combo.grandTotal = result.grandTotal();
    combo.saving = baselineTotal - combo.grandTotal;
    for (const auto& e : result.trace().entries()) {
        // Only a cut counts as a failure; a guard may also raise a discount
        // (MrpCeilingPolicy keeps the price within MRP), which is not a rejection.
        if (e.outcome != engine::TraceOutcome::Adjusted || !e.amount.isNegative()) continue;
        if (std::find(combo.guardsTripped.begin(), combo.guardsTripped.end(), e.ruleId) ==
            combo.guardsTripped.end()) {
            combo.guardsTripped.push_back(e.ruleId);
        }
    }
    combo.passedGuards = combo.guardsTripped.empty();
    return combo;
}

} // namespace

// ------------------------------------------------------- OptimizationResult

OptimizationResult::OptimizationResult(PricingResult best, OfferCombination bestCombination,
                                       Money baselineTotal, std::vector<OfferCombination> ranked,
                                       engine::PriceTrace screening)
    : best_(std::move(best)), bestCombination_(std::move(bestCombination)),
      baselineTotal_(baselineTotal), ranked_(std::move(ranked)),
      screening_(std::move(screening)) {}

std::size_t OptimizationResult::failedGuards() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        ranked_.begin(), ranked_.end(), [](const OfferCombination& c) { return !c.passedGuards; }));
}

// -------------------------------------------------------- DiscountOptimizer

DiscountOptimizer::DiscountOptimizer(GuardFactory guards, std::size_t maxRules)
    : guards_(std::move(guards)), maxRules_(maxRules) {
    if (!guards_) throw std::invalid_argument("DiscountOptimizer: guard factory must not be empty");
    if (maxRules_ == 0 || maxRules_ > 20) {
        throw std::invalid_argument("DiscountOptimizer: maxRules must be 1..20");
    }
}

OptimizationResult DiscountOptimizer::optimize(
    const domain::PricingContext& ctx, const rules::Repository<DiscountRule>& rules) const {
    return optimize(ctx, rules.all());
}

OptimizationResult DiscountOptimizer::optimize(
    const domain::PricingContext& ctx, const std::vector<const DiscountRule*>& rules) const {
    for (const auto* r : rules) {
        if (r == nullptr) throw std::invalid_argument("DiscountOptimizer::optimize: null rule");
    }

    // StackingResolver gives every stackable offer it is handed, so the set we
    // price is exactly the set applied.
    const engine::StackingResolver stacking;
    const engine::PricingEngine engine(stacking, guards_());

    const PricingResult baseline = engine.price(ctx, std::vector<const DiscountRule*>{});
    const Money baselineTotal = baseline.grandTotal();

    // 1. Screening.
    engine::PriceTrace screening;
    std::vector<const DiscountRule*> stackable;
    std::vector<const DiscountRule*> soloOnly;
    for (const auto* rule : rules) {
        const std::string target = engine::PriceTrace::kOrder;
        if (!rule->isApplicable(ctx)) {
            screening.record(TraceEntry::rejected(kStep, rule->id(), rule->name(), target,
                                                  "not applicable: " + rule->rejectionReason(ctx)));
            continue;
        }
        const PricingResult aloneBill = engine.price(ctx, {rule});
        const OfferCombination alone = describe({rule}, aloneBill, baselineTotal);
        if (!alone.passedGuards) {
            // Item discounts on a line only add up, so a cut on its own is a cut in any combination.
            screening.record(TraceEntry::rejected(kStep, rule->id(), rule->name(), target,
                                                  "cut by " + alone.guardsTripped.front() +
                                                      " even on its own"));
            continue;
        }
        if (alone.saving <= Money::zero()) {
            screening.record(TraceEntry::rejected(kStep, rule->id(), rule->name(), target,
                                                  "saves nothing on this cart"));
            continue;
        }
        if (rule->stackable()) {
            stackable.push_back(rule);
            screening.record(TraceEntry::passed(kStep, rule->id(), rule->name(), target,
                                                "kept; saves " + alone.saving.toString() +
                                                    " alone"));
        } else {
            soloOnly.push_back(rule);
            screening.record(TraceEntry::passed(
                kStep, rule->id(), rule->name(), target,
                "kept; not stackable, so tried alone only (saves " + alone.saving.toString() + ")"));
        }
    }
    if (stackable.size() > maxRules_) {
        throw std::length_error("DiscountOptimizer: " + std::to_string(stackable.size()) +
                                " stackable offers apply; at most " + std::to_string(maxRules_) +
                                " can be combined");
    }

    // 2. Price every candidate set, keeping the best bill seen so far.
    std::vector<OfferCombination> ranked;
    std::optional<PricingResult> bestBill;
    std::optional<OfferCombination> bestCombo;
    const auto tryCombination = [&](const std::vector<const DiscountRule*>& chosen) {
        PricingResult bill = engine.price(ctx, chosen);
        OfferCombination combo = describe(chosen, bill, baselineTotal);
        if (combo.passedGuards && (!bestCombo || better(combo, *bestCombo))) {
            bestCombo = combo;
            bestBill = std::move(bill);
        }
        ranked.push_back(std::move(combo));
    };

    const std::size_t n = stackable.size();
    for (std::size_t mask = 1; mask < (std::size_t{1} << n); ++mask) {
        std::vector<const DiscountRule*> chosen;
        for (std::size_t i = 0; i < n; ++i) {
            if (mask & (std::size_t{1} << i)) chosen.push_back(stackable[i]);
        }
        tryCombination(chosen);
    }
    for (const auto* rule : soloOnly) tryCombination({rule});

    std::stable_sort(ranked.begin(), ranked.end(), better);

    // 3. Nothing passed the guards (or nothing to try): the best choice is no offers.
    if (!bestCombo) {
        bestCombo = describe({}, baseline, baselineTotal);
        bestBill = baseline;
    }
    return OptimizationResult(std::move(*bestBill), std::move(*bestCombo), baselineTotal,
                              std::move(ranked), std::move(screening));
}

} // namespace application
