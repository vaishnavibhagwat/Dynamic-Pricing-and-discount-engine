#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "domain/Date.hpp"

namespace validation {

/// The kinds of problem the rule validator looks for.
enum class IssueKind {
    Overlap,      ///< two offers cover the same category on the same days
    Expired,      ///< the offer's end date has passed
    NeverApplies, ///< no product in the catalog can ever match the offer
    MarginBreach  ///< stackable offers together cut below the margin floor
};

/// e.g. "Overlap", "MarginBreach"
std::string toString(IssueKind kind);

/// One problem found in the rule set.
struct RuleIssue {
    IssueKind kind;
    std::vector<std::string> ruleIds; ///< rules involved, in rule-file order
    std::string productId;            ///< product concerned, empty if none
    std::string message;              ///< human-readable explanation
};

/**
 * @brief Everything the rule validator found, as of one date.
 *
 * OOP ideas: encapsulation (issues are only added through add() and read
 * through const queries) and Single Responsibility - it only stores
 * results; ConflictReportPrinter formats them and RuleValidator finds them.
 */
class ConflictReport {
public:
    explicit ConflictReport(domain::Date asOf) : asOf_(asOf) {}

    void add(RuleIssue issue) { issues_.push_back(std::move(issue)); }

    const domain::Date& asOf() const noexcept { return asOf_; }
    const std::vector<RuleIssue>& issues() const noexcept { return issues_; }
    /// Issues of one kind, in the order they were found.
    std::vector<RuleIssue> ofKind(IssueKind kind) const;
    std::size_t count(IssueKind kind) const;
    /// True if some issue of @p kind involves the rule @p ruleId.
    bool flags(IssueKind kind, const std::string& ruleId) const;

    bool empty() const noexcept { return issues_.empty(); }
    std::size_t size() const noexcept { return issues_.size(); }

private:
    domain::Date asOf_;
    std::vector<RuleIssue> issues_;
};

} // namespace validation
