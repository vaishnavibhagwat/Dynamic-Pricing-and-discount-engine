#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "application/GuardFactory.hpp"
#include "domain/Customer.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"
#include "engine/ConflictResolver.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace application {

/// One cell of the what-if grid: one tier under one conflict policy.
struct WhatIfCell {
    domain::Money grandTotal; ///< what the customer would pay
    domain::Money discount;   ///< item + order discounts given
};

/**
 * @brief The what-if grid: rows are membership tiers, columns are conflict
 *        policies.
 *
 * OOP ideas: encapsulation (read-only, bounds-checked access) and Single
 * Responsibility - it holds the numbers; WhatIfPrinter lays them out.
 */
class WhatIfReport {
public:
    /// @throws std::invalid_argument unless cells is tiers x resolverNames.
    WhatIfReport(std::vector<std::string> resolverNames, std::vector<domain::MembershipTier> tiers,
                 std::vector<std::vector<WhatIfCell>> cells, domain::MembershipTier currentTier);

    const std::vector<std::string>& resolverNames() const noexcept { return resolverNames_; }
    const std::vector<domain::MembershipTier>& tiers() const noexcept { return tiers_; }
    /// The customer's real tier.
    domain::MembershipTier currentTier() const noexcept { return currentTier_; }

    /// @throws std::out_of_range on a bad index.
    const WhatIfCell& cell(std::size_t tierIndex, std::size_t resolverIndex) const;
    /// Column of the lowest grand total in a row (first one on a tie).
    std::size_t cheapestResolver(std::size_t tierIndex) const;

private:
    std::vector<std::string> resolverNames_;
    std::vector<domain::MembershipTier> tiers_;
    std::vector<std::vector<WhatIfCell>> cells_;
    domain::MembershipTier currentTier_;
};

/**
 * @brief Re-prices the same cart under every conflict policy and every
 *        membership tier, so the shopper (or examiner) can see what the
 *        policy and the tier are each worth.
 *
 * Nothing is changed: each run uses a copy of the customer with a different
 * tier and the same cart, date, coupon and bank.
 *
 * OOP ideas: Strategy pattern and Liskov Substitution - it holds a list of
 * ConflictResolver pointers and treats them all alike; a new policy shows
 * up as a new column with no change here. Dependency Injection of the
 * resolvers and guards.
 */
class WhatIfSimulator {
public:
    /// Resolvers are not owned and must outlive the simulator.
    /// @throws std::invalid_argument if the list is empty, holds a null, or guards is empty.
    explicit WhatIfSimulator(std::vector<const engine::ConflictResolver*> resolvers,
                             GuardFactory guards = defaultGuardFactory());

    WhatIfReport run(const domain::PricingContext& ctx,
                     const std::vector<const rules::DiscountRule*>& rules) const;
    WhatIfReport run(const domain::PricingContext& ctx,
                     const rules::Repository<rules::DiscountRule>& rules) const;

private:
    std::vector<const engine::ConflictResolver*> resolvers_;
    GuardFactory guards_;
};

} // namespace application
