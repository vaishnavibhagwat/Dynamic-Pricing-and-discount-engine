#include "presentation/ConflictReportPrinter.hpp"

#include <cstddef>
#include <ostream>
#include <string>

namespace presentation {

namespace {

constexpr std::size_t kTagWidth = 15; // "[MarginBreach]" + one space

std::string subjectOf(const validation::RuleIssue& issue) {
    if (!issue.productId.empty()) return issue.productId;
    std::string ids;
    for (std::size_t i = 0; i < issue.ruleIds.size(); ++i) {
        if (i > 0) ids += ", ";
        ids += issue.ruleIds[i];
    }
    return ids;
}

} // namespace

void ConflictReportPrinter::print(std::ostream& os, const validation::ConflictReport& report) const {
    os << "Rule check (as of " << report.asOf().toString() << "): ";
    if (report.empty()) {
        os << "no problems found\n";
        return;
    }
    os << report.size() << (report.size() == 1 ? " problem\n" : " problems\n");
    for (const auto& issue : report.issues()) {
        std::string tag = "[" + validation::toString(issue.kind) + "]";
        if (tag.size() < kTagWidth) tag.append(kTagWidth - tag.size(), ' ');
        os << "  " << tag << subjectOf(issue) << ": " << issue.message << '\n';
    }
    if (report.count(validation::IssueKind::MarginBreach) > 0) {
        os << "  Note: a MarginBreach is the worst case if every listed offer stacks.\n"
              "        At checkout MarginGuard still keeps each line above cost + 8%.\n";
    }
}

} // namespace presentation
