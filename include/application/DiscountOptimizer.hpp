#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "application/GuardFactory.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"
#include "engine/PriceTrace.hpp"
#include "engine/PricingResult.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace application {

/// One set of offers the optimizer priced, and how it came out.
struct OfferCombination {
    std::vector<std::string> ruleIds;   ///< sorted by id
    std::vector<std::string> ruleNames; ///< same order as ruleIds
    domain::Money grandTotal;           ///< what the customer would pay
    domain::Money saving;               ///< no-offer total minus grandTotal
    bool passedGuards = false;          ///< true if no guard had to cut a discount
    std::vector<std::string> guardsTripped; ///< guards that cut a discount, in order
};

/**
 * @brief What DiscountOptimizer found: the winning offer set with its full
 *        priced bill, plus every combination it tried.
 *
 * OOP ideas: encapsulation (read-only after construction) and Single
 * Responsibility - it only holds results; OptimizationPrinter displays them.
 */
class OptimizationResult {
public:
    OptimizationResult(engine::PricingResult best, OfferCombination bestCombination,
                       domain::Money baselineTotal, std::vector<OfferCombination> ranked,
                       engine::PriceTrace screening);

    /// Bill for the winning combination (the no-offer bill if nothing passed the guards).
    const engine::PricingResult& best() const noexcept { return best_; }
    /// The winning combination. Empty ruleIds means "no offers".
    const OfferCombination& bestCombination() const noexcept { return bestCombination_; }
    /// Grand total with no offers at all.
    domain::Money baselineTotal() const noexcept { return baselineTotal_; }
    domain::Money bestSaving() const noexcept { return bestCombination_.saving; }

    /// Every non-empty combination tried, best first (guard-passing ones before the rest).
    const std::vector<OfferCombination>& ranked() const noexcept { return ranked_; }
    std::size_t combinationsTried() const noexcept { return ranked_.size(); }
    std::size_t failedGuards() const noexcept;

    /// Optimizer decisions about each input rule: kept (Passed) or screened out
    /// (Rejected, with the reason).
    const engine::PriceTrace& screening() const noexcept { return screening_; }

private:
    engine::PricingResult best_;
    OfferCombination bestCombination_;
    domain::Money baselineTotal_;
    std::vector<OfferCombination> ranked_;
    engine::PriceTrace screening_;
};

/**
 * @brief Finds the set of offers that saves the customer the most while
 *        passing every price guard.
 *
 * Steps: (1) price the order with no offers (the baseline); (2) screen out
 * offers that do not apply, save nothing on their own, or are cut by a guard
 * even on their own - safe, because adding offers only lowers the amounts
 * later offers see and only raises each line's total discount; (3) price every
 * combination of the remaining stackable offers, and each remaining
 * non-stackable offer on its own (it cannot be combined); (4) keep the
 * combination with the largest saving among those where no guard had to
 * cut a discount (a guard raising one, like the MRP ceiling, is fine). Ties go to fewer offers, then to the smaller id list.
 *
 * OOP ideas: Single Responsibility (search only - pricing is delegated to
 * PricingEngine) and Dependency Injection of the guards through a
 * GuardFactory. It reuses StackingResolver (Strategy) so every offer in a
 * combination is actually given.
 */
class DiscountOptimizer {
public:
    /// Combinations double with each stackable offer, so the search is capped.
    static constexpr std::size_t kDefaultMaxRules = 14;

    /// @throws std::invalid_argument if @p guards is empty or maxRules is 0 or above 20.
    explicit DiscountOptimizer(GuardFactory guards = defaultGuardFactory(),
                               std::size_t maxRules = kDefaultMaxRules);

    /// @throws std::invalid_argument on a null rule.
    /// @throws std::length_error if more than maxRules stackable offers survive screening.
    OptimizationResult optimize(const domain::PricingContext& ctx,
                                const std::vector<const rules::DiscountRule*>& rules) const;
    OptimizationResult optimize(const domain::PricingContext& ctx,
                                const rules::Repository<rules::DiscountRule>& rules) const;

    std::size_t maxRules() const noexcept { return maxRules_; }

private:
    GuardFactory guards_;
    std::size_t maxRules_;
};

} // namespace application
