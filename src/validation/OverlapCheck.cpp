#include "validation/OverlapCheck.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>

namespace validation {

namespace {

/// Shared days of two optional windows (nullopt = always). Returns false if they never meet.
bool sharedDays(const std::optional<domain::DateRange>& a,
                const std::optional<domain::DateRange>& b,
                std::optional<domain::DateRange>& shared) {
    if (!a) { shared = b; return true; }
    if (!b) { shared = a; return true; }
    shared = a->intersect(*b);
    return shared.has_value();
}

} // namespace

std::vector<RuleIssue> OverlapCheck::check(const ValidationContext& ctx) const {
    std::vector<const rules::DiscountRule*> scoped;
    for (const auto* rule : ctx.rules) {
        if (rule->scope() && rule->scope()->category() && !isExpired(*rule, ctx.asOf)) {
            scoped.push_back(rule);
        }
    }

    std::vector<RuleIssue> issues;
    for (std::size_t i = 0; i < scoped.size(); ++i) {
        for (std::size_t j = i + 1; j < scoped.size(); ++j) {
            const auto& a = *scoped[i];
            const auto& b = *scoped[j];
            const std::string& category = *a.scope()->category();
            if (category != *b.scope()->category()) continue;

            std::optional<domain::DateRange> shared;
            if (!sharedDays(a.activePeriod(), b.activePeriod(), shared)) continue;

            const bool sameProduct = std::any_of(
                ctx.catalog.begin(), ctx.catalog.end(), [&](const domain::Product* p) {
                    return a.scope()->matches(*p) && b.scope()->matches(*p);
                });
            if (!sameProduct) continue;

            const std::string when = shared ? "from " + shared->toString() : "at all times";
            issues.push_back({IssueKind::Overlap,
                              {a.id(), b.id()},
                              {},
                              "both cover " + category + " " + when});
        }
    }
    return issues;
}

} // namespace validation
