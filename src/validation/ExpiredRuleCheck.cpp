#include "validation/ExpiredRuleCheck.hpp"

namespace validation {

std::vector<RuleIssue> ExpiredRuleCheck::check(const ValidationContext& ctx) const {
    std::vector<RuleIssue> issues;
    for (const auto* rule : ctx.rules) {
        if (!isExpired(*rule, ctx.asOf)) continue;
        issues.push_back({IssueKind::Expired,
                          {rule->id()},
                          {},
                          "ended on " + rule->activePeriod()->end().toString() + ", before " +
                              ctx.asOf.toString()});
    }
    return issues;
}

} // namespace validation
