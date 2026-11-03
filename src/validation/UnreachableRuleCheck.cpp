#include "validation/UnreachableRuleCheck.hpp"

#include <algorithm>

namespace validation {

std::vector<RuleIssue> UnreachableRuleCheck::check(const ValidationContext& ctx) const {
    std::vector<RuleIssue> issues;
    if (ctx.catalog.empty()) return issues;
    for (const auto* rule : ctx.rules) {
        if (!rules::isItemStage(rule->stage())) continue;
        const bool reachable =
            std::any_of(ctx.catalog.begin(), ctx.catalog.end(), [rule](const domain::Product* p) {
                return rule->maxPercentOff(*p).has_value();
            });
        if (reachable) continue;
        const std::string what = rule->scope() ? rule->scope()->describe() : "its conditions";
        issues.push_back({IssueKind::NeverApplies,
                          {rule->id()},
                          {},
                          "no product in the catalog matches " + what});
    }
    return issues;
}

} // namespace validation
