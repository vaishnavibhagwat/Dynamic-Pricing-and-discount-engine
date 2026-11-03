#pragma once

#include "validation/RuleCheck.hpp"

namespace validation {

/**
 * @brief Flags offers whose end date is already in the past.
 *
 * OOP ideas: Strategy (one concrete RuleCheck) and polymorphism - it asks
 * each rule for its activePeriod() instead of checking concrete rule types.
 */
class ExpiredRuleCheck final : public RuleCheck {
public:
    std::string name() const override { return "ExpiredRuleCheck"; }
    std::vector<RuleIssue> check(const ValidationContext& ctx) const override;
};

} // namespace validation
