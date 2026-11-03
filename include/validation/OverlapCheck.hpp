#pragma once

#include "validation/RuleCheck.hpp"

namespace validation {

/**
 * @brief Flags pairs of offers that cover the same category on the same days.
 *
 * Two rules overlap when both are limited to the same category, their date
 * windows meet (no window means "always"), neither has expired, and at
 * least one catalog product is covered by both (so two lists of different
 * product ids in one category do not count).
 *
 * OOP ideas: Strategy (one concrete RuleCheck) and polymorphism through
 * DiscountRule::scope() and activePeriod().
 */
class OverlapCheck final : public RuleCheck {
public:
    std::string name() const override { return "OverlapCheck"; }
    std::vector<RuleIssue> check(const ValidationContext& ctx) const override;
};

} // namespace validation
