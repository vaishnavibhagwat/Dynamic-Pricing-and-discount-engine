#include "rules/CappedRule.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rules {

namespace {

domain::Money checkedCap(domain::Money cap) {
    if (cap <= domain::Money::zero()) throw std::invalid_argument("cap must be positive");
    return cap;
}

} // namespace

CappedRule::CappedRule(std::unique_ptr<DiscountRule> inner, domain::Money cap)
    : RuleDecorator(std::move(inner), "max " + checkedCap(cap).toString() + " off"), cap_(cap) {}

domain::Money CappedRule::limit(domain::Money discount) const {
    return std::min(discount, cap_);
}

} // namespace rules
