#include "rules/HappyHourRule.hpp"

#include <stdexcept>
#include <utility>

#include "domain/PricingContext.hpp"

namespace rules {

namespace {

std::string window(domain::TimeOfDay from, domain::TimeOfDay to) {
    if (from == to) throw std::invalid_argument("happy hour start and end must differ");
    return from.toString() + "-" + to.toString();
}

} // namespace

HappyHourRule::HappyHourRule(std::unique_ptr<DiscountRule> inner, domain::TimeOfDay from,
                             domain::TimeOfDay to)
    : RuleDecorator(std::move(inner), "happy hour " + window(from, to)), from_(from), to_(to) {}

bool HappyHourRule::contains(domain::TimeOfDay time) const noexcept {
    if (from_ < to_) return from_ <= time && time < to_;
    return time >= from_ || time < to_; // crosses midnight
}

bool HappyHourRule::allows(const domain::PricingContext& ctx) const {
    return ctx.orderTime().has_value() && contains(*ctx.orderTime());
}

std::string HappyHourRule::blockedReason(const domain::PricingContext& ctx) const {
    const std::string hours = from_.toString() + " and " + to_.toString();
    if (!ctx.orderTime()) return "only between " + hours + "; order time unknown";
    return "only between " + hours + " (ordered at " + ctx.orderTime()->toString() + ")";
}

} // namespace rules
