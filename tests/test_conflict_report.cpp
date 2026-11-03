#include <gtest/gtest.h>

#include "validation/ConflictReport.hpp"

using validation::ConflictReport;
using validation::IssueKind;
using validation::RuleIssue;

TEST(ConflictReport, StartsEmptyWithItsDate) {
    const ConflictReport report(domain::Date(2026, 10, 28));
    EXPECT_TRUE(report.empty());
    EXPECT_EQ(report.size(), 0u);
    EXPECT_EQ(report.asOf(), domain::Date(2026, 10, 28));
}

TEST(ConflictReport, GroupsAndCountsByKind) {
    ConflictReport report(domain::Date(2026, 10, 28));
    report.add({IssueKind::Expired, {"OLD"}, {}, "ended"});
    report.add({IssueKind::Overlap, {"A", "B"}, {}, "both cover X"});
    report.add({IssueKind::Expired, {"OLDER"}, {}, "ended"});

    EXPECT_EQ(report.size(), 3u);
    EXPECT_EQ(report.count(IssueKind::Expired), 2u);
    EXPECT_EQ(report.count(IssueKind::MarginBreach), 0u);
    const auto expired = report.ofKind(IssueKind::Expired);
    ASSERT_EQ(expired.size(), 2u);
    EXPECT_EQ(expired[0].ruleIds.front(), "OLD");
    EXPECT_EQ(expired[1].ruleIds.front(), "OLDER");
}

TEST(ConflictReport, FlagsMatchesKindAndRuleId) {
    ConflictReport report(domain::Date(2026, 10, 28));
    report.add({IssueKind::Overlap, {"A", "B"}, {}, "both cover X"});
    EXPECT_TRUE(report.flags(IssueKind::Overlap, "A"));
    EXPECT_TRUE(report.flags(IssueKind::Overlap, "B"));
    EXPECT_FALSE(report.flags(IssueKind::Expired, "A"));
    EXPECT_FALSE(report.flags(IssueKind::Overlap, "C"));
}

TEST(ConflictReport, IssueKindNames) {
    EXPECT_EQ(validation::toString(IssueKind::Overlap), "Overlap");
    EXPECT_EQ(validation::toString(IssueKind::Expired), "Expired");
    EXPECT_EQ(validation::toString(IssueKind::NeverApplies), "NeverApplies");
    EXPECT_EQ(validation::toString(IssueKind::MarginBreach), "MarginBreach");
}
