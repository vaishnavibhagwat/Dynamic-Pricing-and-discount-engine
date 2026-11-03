#pragma once

#include "validation/RuleCheck.hpp"

namespace validation {

/**
 * @brief Flags item offers whose conditions can never be true: no product in
 *        the catalog matches them (a misspelt category, unknown product ids,
 *        a category and product list that contradict each other, an expiry
 *        markdown with no groceries...).
 *
 * Skipped when the catalog is empty, since nothing can be judged. A start
 * date after the end date never gets this far: the rule constructor rejects
 * it when the file is loaded.
 *
 * OOP ideas: Strategy (one concrete RuleCheck) and polymorphism - it relies
 * on each rule's own maxPercentOff() answer, so new rule types work unchanged.
 */
class UnreachableRuleCheck final : public RuleCheck {
public:
    std::string name() const override { return "UnreachableRuleCheck"; }
    std::vector<RuleIssue> check(const ValidationContext& ctx) const override;
};

} // namespace validation
