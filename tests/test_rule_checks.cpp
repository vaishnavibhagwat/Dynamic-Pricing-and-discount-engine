// Each RuleCheck on its own, with rules built in code.

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/MarginGuard.hpp"
#include "rules/BankOfferDiscount.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"
#include "validation/ExpiredRuleCheck.hpp"
#include "validation/MarginStackCheck.hpp"
#include "validation/OverlapCheck.hpp"
#include "validation/UnreachableRuleCheck.hpp"

using namespace fixtures;
using domain::Date;
using rules::ItemFilter;
using rules::PercentageDiscount;
using rules::SeasonalDiscount;
using validation::IssueKind;
using validation::RuleIssue;
using validation::ValidationContext;

namespace {

using Rules = std::vector<const rules::DiscountRule*>;
using Catalog = std::vector<const domain::Product*>;

std::vector<RuleIssue> run(const validation::RuleCheck& check, const Rules& rules,
                           const Catalog& catalog, Date asOf = kToday) {
    return check.check(ValidationContext{rules, catalog, asOf});
}

SeasonalDiscount seasonal(const std::string& id, Date start, Date end, int percent,
                          const std::string& category, bool stackable = true) {
    return SeasonalDiscount(id, id, 1, stackable, start, end, percent,
                            ItemFilter::forCategory(category));
}

/// A guard that allows at most a fixed amount, to test MarginStackCheck without margin maths.
class FixedLimitGuard final : public engine::PriceGuard {
public:
    explicit FixedLimitGuard(domain::Money max) : max_(max) {}
    std::string name() const override { return "FixedLimit"; }
    engine::GuardOutcome limit(const domain::CartItem&, domain::Money proposed) const override {
        if (proposed <= max_) return {proposed, "ok"};
        return {max_, "over the fixed limit"};
    }

private:
    domain::Money max_;
};

} // namespace

// ------------------------------------------------------------ ExpiredRuleCheck

TEST(ExpiredRuleCheck, FlagsRuleThatEndedBeforeToday) {
    const auto old = seasonal("OLD", Date(2026, 7, 1), Date(2026, 7, 31), 10, "Clothing");
    const auto issues = run(validation::ExpiredRuleCheck(), {&old}, {&kurta()});
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].kind, IssueKind::Expired);
    EXPECT_EQ(issues[0].ruleIds, std::vector<std::string>{"OLD"});
    EXPECT_NE(issues[0].message.find("2026-07-31"), std::string::npos);
}

TEST(ExpiredRuleCheck, RuleEndingTodayOrLaterOrUndatedIsFine) {
    const auto endsToday = seasonal("TODAY", Date(2026, 10, 1), kToday, 10, "Clothing");
    const auto future = seasonal("LATER", Date(2027, 1, 1), Date(2027, 1, 31), 10, "Clothing");
    const PercentageDiscount undated("ALWAYS", "Always", 1, true, 10);
    EXPECT_TRUE(run(validation::ExpiredRuleCheck(), {&endsToday, &future, &undated}, {}).empty());
}

// -------------------------------------------------------- UnreachableRuleCheck

TEST(UnreachableRuleCheck, FlagsItemRuleMatchingNoCatalogProduct) {
    const PercentageDiscount typo("TYPO", "Typo", 1, true, 10, ItemFilter::forCategory("Electornics"));
    const rules::QuantityDiscount ghost("GHOST", "Ghost", 1, true, 2, 5,
                                        ItemFilter(std::nullopt, {"X999"}));
    const PercentageDiscount clash("CLASH", "Clash", 1, true, 5,
                                   ItemFilter(std::string("Furniture"), {"E1"}));
    const rules::ExpiryMarkdown expiry("EXP", "Expiry", 1, true, {{3, 30}});
    const auto issues =
        run(validation::UnreachableRuleCheck(), {&typo, &ghost, &clash, &expiry},
            {&phone(), &kurta(), &chair()});
    ASSERT_EQ(issues.size(), 4u); // no grocery in this catalog, so the expiry rule too
    EXPECT_EQ(issues[0].ruleIds.front(), "TYPO");
    EXPECT_NE(issues[0].message.find("Electornics"), std::string::npos);
    EXPECT_EQ(issues[1].ruleIds.front(), "GHOST");
    EXPECT_EQ(issues[2].ruleIds.front(), "CLASH");
    EXPECT_EQ(issues[3].ruleIds.front(), "EXP");
    for (const auto& i : issues) EXPECT_EQ(i.kind, IssueKind::NeverApplies);
}

TEST(UnreachableRuleCheck, IgnoresReachableAndOrderLevelRules) {
    const PercentageDiscount furniture("FURN", "Furniture", 1, true, 10,
                                       ItemFilter::forCategory("Furniture"));
    const rules::BankOfferDiscount bank("BANK", "Bank", 1, true, "HDFC", 10, rs(1000));
    EXPECT_TRUE(run(validation::UnreachableRuleCheck(), {&furniture, &bank}, {&chair()}).empty());
}

TEST(UnreachableRuleCheck, SkipsWhenCatalogIsEmpty) {
    const PercentageDiscount typo("TYPO", "Typo", 1, true, 10, ItemFilter::forCategory("Nope"));
    EXPECT_TRUE(run(validation::UnreachableRuleCheck(), {&typo}, {}).empty());
}

// ---------------------------------------------------------------- OverlapCheck

TEST(OverlapCheck, FlagsSameCategoryWithSharedDays) {
    const auto fest = seasonal("FEST", Date(2026, 10, 20), Date(2026, 11, 20), 15, "Furniture");
    const auto clear = seasonal("CLEAR", Date(2026, 11, 10), Date(2026, 12, 10), 12, "Furniture");
    const auto issues = run(validation::OverlapCheck(), {&fest, &clear}, {&chair()});
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].kind, IssueKind::Overlap);
    EXPECT_EQ(issues[0].ruleIds, (std::vector<std::string>{"FEST", "CLEAR"}));
    EXPECT_EQ(issues[0].message, "both cover Furniture from 2026-11-10 to 2026-11-20");
}

TEST(OverlapCheck, UndatedRulesOverlapAtAllTimesAndWithDatedOnes) {
    const PercentageDiscount a("A", "A", 1, true, 10, ItemFilter::forCategory("Clothing"));
    const rules::QuantityDiscount b("B", "B", 1, true, 3, 15, ItemFilter::forCategory("Clothing"));
    const auto c = seasonal("C", Date(2027, 1, 15), Date(2027, 1, 31), 30, "Clothing");
    const auto issues = run(validation::OverlapCheck(), {&a, &b, &c}, {&kurta()});
    ASSERT_EQ(issues.size(), 3u);
    EXPECT_EQ(issues[0].message, "both cover Clothing at all times");
    EXPECT_EQ(issues[1].message, "both cover Clothing from 2027-01-15 to 2027-01-31");
}

TEST(OverlapCheck, NoOverlapForOtherDatesCategoriesOrExpiredRules) {
    const auto fest = seasonal("FEST", Date(2026, 10, 20), Date(2026, 11, 20), 15, "Furniture");
    const auto newYear = seasonal("NY", Date(2027, 1, 1), Date(2027, 1, 10), 5, "Furniture");
    const auto phones = seasonal("PH", Date(2026, 10, 20), Date(2026, 11, 20), 5, "Electronics");
    EXPECT_TRUE(run(validation::OverlapCheck(), {&fest, &newYear, &phones}, {&chair(), &phone()})
                    .empty());

    // An undated Furniture rule would overlap every window, but expired ones are skipped.
    const PercentageDiscount always("ALL", "All", 1, true, 5, ItemFilter::forCategory("Furniture"));
    const auto expired = seasonal("OLD", Date(2026, 7, 1), Date(2026, 7, 31), 5, "Furniture");
    EXPECT_TRUE(run(validation::OverlapCheck(), {&always, &expired}, {&chair()}).empty());
}

TEST(OverlapCheck, SameCategoryButNoSharedProductIsNotAnOverlap) {
    const PercentageDiscount onlyChair("A", "A", 1, true, 10,
                                       ItemFilter(std::string("Furniture"), {"F1"}));
    const PercentageDiscount onlySofa("B", "B", 1, true, 10,
                                      ItemFilter(std::string("Furniture"), {"F2"}));
    EXPECT_TRUE(run(validation::OverlapCheck(), {&onlyChair, &onlySofa}, {&chair()}).empty());
}

// ------------------------------------------------------------ MarginStackCheck

TEST(MarginStackCheck, RejectsNullGuard) {
    EXPECT_THROW(validation::MarginStackCheck(nullptr), std::invalid_argument);
}

TEST(MarginStackCheck, FlagsStackableRulesThatBreakTheMarginGuard) {
    // Phone: cost 12,000, price 15,000 -> cost + 8% floor 12,960 leaves 2,040 (13.6%).
    const PercentageDiscount mega("MEGA", "Mega", 1, true, 10, ItemFilter::forCategory("Electronics"));
    const rules::MembershipDiscount vip("VIP", "VIP", 1, true,
                                        {{domain::MembershipTier::Platinum, 5}});
    const auto issues = run(validation::MarginStackCheck(), {&mega, &vip}, {&phone(), &kurta()});
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_EQ(issues[0].kind, IssueKind::MarginBreach);
    EXPECT_EQ(issues[0].productId, "E1");
    EXPECT_EQ(issues[0].ruleIds, (std::vector<std::string>{"MEGA", "VIP"}));
    EXPECT_NE(issues[0].message.find("MEGA 10% + VIP 5% = 15%"), std::string::npos);
    EXPECT_NE(issues[0].message.find("below cost + 8%"), std::string::npos);
}

TEST(MarginStackCheck, StackWithinMarginIsFine) {
    const PercentageDiscount mega("MEGA", "Mega", 1, true, 10, ItemFilter::forCategory("Electronics"));
    EXPECT_TRUE(run(validation::MarginStackCheck(), {&mega}, {&phone()}).empty());
}

TEST(MarginStackCheck, IgnoresNonStackableExpiredAndOrderLevelRules) {
    const PercentageDiscount mega("MEGA", "Mega", 1, true, 10, ItemFilter::forCategory("Electronics"));
    const PercentageDiscount flash("FLASH", "Flash", 1, false, 13, ItemFilter(std::nullopt, {"E1"}));
    const auto old = seasonal("OLD", Date(2026, 1, 1), Date(2026, 1, 31), 20, "Electronics");
    const rules::BankOfferDiscount bank("BANK", "Bank", 1, true, "HDFC", 10, rs(1500));
    EXPECT_TRUE(run(validation::MarginStackCheck(), {&mega, &flash, &old, &bank}, {&phone()}).empty());
}

TEST(MarginStackCheck, OnlyStacksRulesWhoseDatesMeet) {
    // 8% + 8% would break the phone's 13.6%, but the windows never meet.
    const auto oct = seasonal("OCT", Date(2026, 10, 25), Date(2026, 10, 31), 8, "Electronics");
    const auto dec = seasonal("DEC", Date(2026, 12, 1), Date(2026, 12, 31), 8, "Electronics");
    EXPECT_TRUE(run(validation::MarginStackCheck(), {&oct, &dec}, {&phone()}).empty());

    // A future window that meets an always-on rule is found on its first day.
    const PercentageDiscount always("ALL", "All", 1, true, 8, ItemFilter::forCategory("Electronics"));
    const auto issues = run(validation::MarginStackCheck(), {&always, &dec}, {&phone()});
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_NE(issues[0].message.find("on 2026-12-01"), std::string::npos);
}

TEST(MarginStackCheck, UsesTheInjectedGuard) {
    const PercentageDiscount ten("TEN", "Ten", 1, true, 10);
    // Kurta at 1,000: 10% = 100. A guard allowing only 50 must flag it.
    const validation::MarginStackCheck strict(std::make_unique<FixedLimitGuard>(rs(50)));
    const auto issues = run(strict, {&ten}, {&kurta()});
    ASSERT_EQ(issues.size(), 1u);
    EXPECT_NE(issues[0].message.find("over the fixed limit"), std::string::npos);

    const validation::MarginStackCheck loose(std::make_unique<FixedLimitGuard>(rs(100)));
    EXPECT_TRUE(run(loose, {&ten}, {&kurta()}).empty());
}
