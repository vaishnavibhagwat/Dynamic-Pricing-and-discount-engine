#pragma once

#include <memory>

#include "engine/PriceGuard.hpp"
#include "validation/RuleCheck.hpp"

namespace validation {

/**
 * @brief Flags products where stackable offers, all running on the same day,
 *        would add up to more discount than the margin guard allows.
 *
 * For each catalog product it takes every stackable, non-expired rule that
 * can cover the product, finds the day on which the most of them run
 * together, and asks the guard whether it would cut that combined discount.
 * Each rule counts at its best case for the product (e.g. the top
 * membership tier). Non-stackable offers are left out (they never combine),
 * and so are order-level offers (coupons, bank offers), which the engine
 * applies after the guards.
 *
 * OOP ideas: Strategy (one concrete RuleCheck), composition and Dependency
 * Inversion - it owns a PriceGuard and reuses its limit() instead of
 * repeating the margin arithmetic, so the check always agrees with the engine.
 */
class MarginStackCheck final : public RuleCheck {
public:
    /// Uses engine::MarginGuard with its default markup (the engine's own setting).
    MarginStackCheck();
    /// @throws std::invalid_argument if guard is null.
    explicit MarginStackCheck(std::unique_ptr<engine::PriceGuard> guard);

    std::string name() const override { return "MarginStackCheck"; }
    std::vector<RuleIssue> check(const ValidationContext& ctx) const override;

private:
    std::unique_ptr<engine::PriceGuard> guard_;
};

} // namespace validation
