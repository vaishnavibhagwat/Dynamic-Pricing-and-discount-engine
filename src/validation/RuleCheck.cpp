#include "validation/RuleCheck.hpp"

namespace validation {

bool isExpired(const rules::DiscountRule& rule, const domain::Date& asOf) {
    const auto period = rule.activePeriod();
    return period && period->end() < asOf;
}

} // namespace validation
