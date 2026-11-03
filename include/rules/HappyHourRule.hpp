#pragma once

#include <memory>
#include <string>

#include "domain/TimeOfDay.hpp"
#include "rules/RuleDecorator.hpp"

namespace rules {

/**
 * @brief Makes any offer valid only during a daily time window, e.g. a
 *        happy hour from 18:00 to 21:00.
 *
 * The window includes @p from and excludes @p to. A window that crosses
 * midnight (22:00 to 02:00) is allowed. Orders with no known time do not
 * qualify.
 *
 * OOP ideas: Decorator pattern - any existing offer becomes a happy-hour
 * offer by wrapping it, with no change to the offer's class (Open/Closed).
 */
class HappyHourRule final : public RuleDecorator {
public:
    /// @throws std::invalid_argument if @p inner is null or from == to.
    HappyHourRule(std::unique_ptr<DiscountRule> inner, domain::TimeOfDay from, domain::TimeOfDay to);

    domain::TimeOfDay from() const noexcept { return from_; }
    domain::TimeOfDay to() const noexcept { return to_; }
    /// True if @p time falls inside the window.
    bool contains(domain::TimeOfDay time) const noexcept;

protected:
    bool allows(const domain::PricingContext& ctx) const override;
    std::string blockedReason(const domain::PricingContext& ctx) const override;

private:
    domain::TimeOfDay from_;
    domain::TimeOfDay to_;
};

} // namespace rules
