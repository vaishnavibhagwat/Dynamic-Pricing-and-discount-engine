#pragma once

#include <memory>

#include "domain/Money.hpp"
#include "rules/RuleDecorator.hpp"

namespace rules {

/**
 * @brief Limits any offer to at most a fixed rupee amount, e.g. "max ₹500 off".
 *
 * The cap applies to each discount the wrapped offer gives: per cart line
 * for item offers, and on the order for order offers (coupon, bank).
 *
 * OOP ideas: Decorator pattern - one class caps every kind of offer, so no
 * offer needs its own "max" field. Single Responsibility - it only caps.
 */
class CappedRule final : public RuleDecorator {
public:
    /// @throws std::invalid_argument if @p inner is null or @p cap is not positive.
    CappedRule(std::unique_ptr<DiscountRule> inner, domain::Money cap);

    domain::Money cap() const noexcept { return cap_; }

protected:
    domain::Money limit(domain::Money discount) const override;

private:
    domain::Money cap_;
};

} // namespace rules
