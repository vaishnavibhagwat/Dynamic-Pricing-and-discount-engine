#include "validation/ConflictReport.hpp"

#include <algorithm>

namespace validation {

std::string toString(IssueKind kind) {
    switch (kind) {
        case IssueKind::Overlap: return "Overlap";
        case IssueKind::Expired: return "Expired";
        case IssueKind::NeverApplies: return "NeverApplies";
        case IssueKind::MarginBreach: return "MarginBreach";
    }
    return "Unknown";
}

std::vector<RuleIssue> ConflictReport::ofKind(IssueKind kind) const {
    std::vector<RuleIssue> result;
    for (const auto& issue : issues_) {
        if (issue.kind == kind) result.push_back(issue);
    }
    return result;
}

std::size_t ConflictReport::count(IssueKind kind) const {
    return static_cast<std::size_t>(std::count_if(
        issues_.begin(), issues_.end(), [kind](const RuleIssue& i) { return i.kind == kind; }));
}

bool ConflictReport::flags(IssueKind kind, const std::string& ruleId) const {
    for (const auto& issue : issues_) {
        if (issue.kind != kind) continue;
        if (std::find(issue.ruleIds.begin(), issue.ruleIds.end(), ruleId) != issue.ruleIds.end()) {
            return true;
        }
    }
    return false;
}

} // namespace validation
