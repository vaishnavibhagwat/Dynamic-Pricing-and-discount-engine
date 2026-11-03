#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "infrastructure/CatalogLoader.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/RuleFactory.hpp"
#include "test_rule_fixtures.hpp"
#include "validation/RuleValidator.hpp"

using namespace fixtures;
using validation::ConflictReport;
using validation::IssueKind;
using validation::RuleValidator;

namespace {

const domain::Grocery& milk() {
    static const auto m = makeMilk("G1", 2);
    return m;
}

std::vector<const domain::Product*> catalog() {
    return {&phone(), &kurta(), &chair(), &milk()};
}

/// The deliberately broken rule file, checked once and shared by the tests below.
const ConflictReport& badReport() {
    static const auto rules = rules::RuleFactory().loadFile(TEST_DATA_DIR "/bad_rules.json");
    static const ConflictReport report = RuleValidator().validate(rules, catalog(), kToday);
    return report;
}

} // namespace

TEST(RuleValidator, RunsTheFourDefaultChecksInOrder) {
    EXPECT_EQ(RuleValidator().checkNames(),
              (std::vector<std::string>{"ExpiredRuleCheck", "UnreachableRuleCheck", "OverlapCheck",
                                        "MarginStackCheck"}));
}

TEST(RuleValidator, RejectsNullCheckRuleOrProduct) {
    std::vector<std::unique_ptr<validation::RuleCheck>> checks;
    checks.push_back(nullptr);
    EXPECT_THROW(RuleValidator(std::move(checks)), std::invalid_argument);

    const RuleValidator validator;
    EXPECT_THROW(validator.validate(std::vector<const rules::DiscountRule*>{nullptr}, {}, kToday),
                 std::invalid_argument);
    EXPECT_THROW(validator.validate(std::vector<const rules::DiscountRule*>{}, {nullptr}, kToday),
                 std::invalid_argument);
}

TEST(RuleValidator, NoChecksMeansEmptyReport) {
    const rules::PercentageDiscount rule("P", "P", 1, true, 99);
    const RuleValidator none{std::vector<std::unique_ptr<validation::RuleCheck>>{}};
    EXPECT_TRUE(none.validate(std::vector<const rules::DiscountRule*>{&rule}, catalog(), kToday)
                    .empty());
}

// ------------------------------------------------- tests/data/bad_rules.json

TEST(RuleValidatorBadFile, CatchesExpiredRule) {
    EXPECT_EQ(badReport().count(IssueKind::Expired), 1u);
    EXPECT_TRUE(badReport().flags(IssueKind::Expired, "EXPIRED_MONSOON"));
}

TEST(RuleValidatorBadFile, CatchesRulesThatCanNeverApply) {
    EXPECT_EQ(badReport().count(IssueKind::NeverApplies), 3u);
    EXPECT_TRUE(badReport().flags(IssueKind::NeverApplies, "TYPO_CATEGORY"));
    EXPECT_TRUE(badReport().flags(IssueKind::NeverApplies, "GHOST_PRODUCT"));
    EXPECT_TRUE(badReport().flags(IssueKind::NeverApplies, "CONTRADICTORY_FILTER"));
}

TEST(RuleValidatorBadFile, CatchesOverlapOnSameCategoryAndDates) {
    const auto overlaps = badReport().ofKind(IssueKind::Overlap);
    ASSERT_EQ(overlaps.size(), 1u);
    EXPECT_EQ(overlaps[0].ruleIds,
              (std::vector<std::string>{"FURNITURE_FEST", "FURNITURE_CLEARANCE"}));
    EXPECT_EQ(overlaps[0].message, "both cover Furniture from 2026-11-10 to 2026-11-20");
    // Same category, different dates: not an overlap.
    EXPECT_FALSE(badReport().flags(IssueKind::Overlap, "FURNITURE_NEWYEAR"));
}

TEST(RuleValidatorBadFile, CatchesStackThatBreaksMarginGuard) {
    const auto breaches = badReport().ofKind(IssueKind::MarginBreach);
    ASSERT_EQ(breaches.size(), 1u);
    EXPECT_EQ(breaches[0].productId, "E1");
    EXPECT_EQ(breaches[0].ruleIds, (std::vector<std::string>{"ELECTRONICS_MEGA", "VIP_TIERS"}));
    // The non-stackable flash deal is never added to the stack.
    EXPECT_FALSE(badReport().flags(IssueKind::MarginBreach, "PHONE_FLASH"));
}

TEST(RuleValidatorBadFile, ReportsNothingElse) {
    EXPECT_EQ(badReport().size(), 6u);
    EXPECT_EQ(badReport().asOf(), kToday);
}

// --------------------------------------------------------- data/rules.json

TEST(RuleValidatorShippedRules, HaveNoExpiredOrDeadRules) {
    // Checked against the shipped catalog, so every shipped rule has a product to match.
    const auto rules = rules::RuleFactory().loadFile(DATA_DIR "/rules.json");
    const auto products = infrastructure::CatalogLoader().loadFile(DATA_DIR "/catalog.json");
    std::vector<const domain::Product*> shipped;
    for (const auto& p : products) shipped.push_back(p.get());
    const auto report = RuleValidator().validate(rules, shipped, kToday);
    EXPECT_EQ(report.count(IssueKind::Expired), 0u);
    EXPECT_EQ(report.count(IssueKind::NeverApplies), 0u);
}
