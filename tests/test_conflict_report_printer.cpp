#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "presentation/ConflictReportPrinter.hpp"

using presentation::ConflictReportPrinter;
using validation::ConflictReport;
using validation::IssueKind;

TEST(ConflictReportPrinter, EmptyReportSaysNoProblems) {
    std::ostringstream out;
    ConflictReportPrinter().print(out, ConflictReport(domain::Date(2026, 10, 28)));
    EXPECT_EQ(out.str(), "Rule check (as of 2026-10-28): no problems found\n");
}

TEST(ConflictReportPrinter, PrintsOneLinePerIssue) {
    ConflictReport report(domain::Date(2026, 10, 28));
    report.add({IssueKind::Overlap, {"A", "B"}, {}, "both cover Furniture at all times"});
    report.add({IssueKind::MarginBreach, {"A", "M"}, "E1", "too much"});

    std::ostringstream out;
    ConflictReportPrinter().print(out, report);
    EXPECT_EQ(out.str(),
              "Rule check (as of 2026-10-28): 2 problems\n"
              "  [Overlap]      A, B: both cover Furniture at all times\n"
              "  [MarginBreach] E1: too much\n"
              "  Note: a MarginBreach is the worst case if every listed offer stacks.\n"
              "        At checkout MarginGuard still keeps each line above cost + 8%.\n");
}

TEST(ConflictReportPrinter, SingularForOneProblem) {
    ConflictReport report(domain::Date(2026, 10, 28));
    report.add({IssueKind::Expired, {"OLD"}, {}, "ended"});
    std::ostringstream out;
    ConflictReportPrinter().print(out, report);
    EXPECT_NE(out.str().find("1 problem\n"), std::string::npos);
}
