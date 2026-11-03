#pragma once

#include <string>

#include "domain/CartItem.hpp"
#include "domain/Money.hpp"

namespace engine {

/// What a guard allows. reason explains the check whether or not the discount changed.
struct GuardOutcome {
    domain::Money discount;
    std::string reason;
};

/**
 * @brief A business safety check on one line's total item discount.
 *
 * OOP ideas: abstraction (pure virtual interface) and the Open/Closed
 * Principle - PricingEngine runs an injected list of guards in order, so a
 * new check is a new subclass and the engine does not change. Each guard has
 * one job (Single Responsibility).
 */
class PriceGuard {
public:
    virtual ~PriceGuard() = default;

    virtual std::string name() const = 0;
    /// Returns the discount this guard allows on @p item, given the one proposed.
    virtual GuardOutcome limit(const domain::CartItem& item,
                               domain::Money proposedDiscount) const = 0;

protected:
    PriceGuard() = default;
    PriceGuard(const PriceGuard&) = default;
    PriceGuard& operator=(const PriceGuard&) = default;
};

} // namespace engine
