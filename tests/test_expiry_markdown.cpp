#include <gtest/gtest.h>

#include <stdexcept>

#include "domain/Perishable.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::ExpiryMarkdown;

namespace {
// Deliberately unsorted to check that the constructor sorts tiers.
ExpiryMarkdown freshSaver() {
    return ExpiryMarkdown("X1", "Fresh Saver", 1, true, {{7, 15}, {1, 50}, {3, 30}});
}
} // namespace

TEST(ExpiryMarkdown, PercentGrowsAsExpiryNears) {
    const auto rule = freshSaver();
    EXPECT_EQ(rule.percentFor(0), 50);  // expires today
    EXPECT_EQ(rule.percentFor(1), 50);  // tier boundary
    EXPECT_EQ(rule.percentFor(2), 30);
    EXPECT_EQ(rule.percentFor(3), 30);  // tier boundary
    EXPECT_EQ(rule.percentFor(4), 15);
    EXPECT_EQ(rule.percentFor(7), 15);  // tier boundary
    EXPECT_FALSE(rule.percentFor(8).has_value());
    EXPECT_FALSE(rule.percentFor(-1).has_value());  // expired
}

TEST(ExpiryMarkdown, SortsTiersTightestFirst) {
    const auto rule = freshSaver();
    ASSERT_EQ(rule.tiers().size(), 3u);
    EXPECT_EQ(rule.tiers()[0].maxDaysLeft, 1);
    EXPECT_EQ(rule.tiers()[2].maxDaysLeft, 7);
}

TEST(ExpiryMarkdown, DiscountsEachGroceryByItsOwnTier) {
    const auto milkToday = makeMilk("G1", 0);  // 50%
    const auto milkIn5 = makeMilk("G2", 5);    // 15%
    const auto milkIn30 = makeMilk("G3", 30);  // none
    Order o;
    o.cart += domain::CartItem(milkToday, 2);  // 120 -> 60
    o.cart += domain::CartItem(milkIn5, 1);    // 60 -> 9
    o.cart += domain::CartItem(milkIn30, 4);
    const auto rule = freshSaver();
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(69));
}

TEST(ExpiryMarkdown, ExpiredGroceryIsNeverDiscounted) {
    const auto stale = makeMilk("G1", -1);
    Order o;
    o.cart += domain::CartItem(stale, 1);
    const auto rule = freshSaver();
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), Money::zero());
    EXPECT_NE(rule.rejectionReason(o.context()).find("expired"), std::string::npos);
}

TEST(ExpiryMarkdown, IgnoresNonGroceryProducts) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    EXPECT_FALSE(freshSaver().isApplicable(o.context()));
}

namespace {
/// A perishable product type the rule has never heard of (not a Grocery).
class Medicine final : public domain::Product, public domain::Perishable {
public:
    Medicine(std::string id, domain::Date expiry)
        : Product(std::move(id), "Syrup", rs(80), rs(100), rs(112)), expiry_(expiry) {}
    int gstRate() const override { return 12; }
    std::string category() const override { return "Medicine"; }
    const domain::Date& expiryDate() const noexcept override { return expiry_; }

private:
    domain::Date expiry_;
};
} // namespace

TEST(ExpiryMarkdown, WorksForAnyPerishableProduct) {
    // Open/Closed: a new Perishable product is marked down with no change to the rule.
    Order o;
    const Medicine syrup("M1", domain::Date(2026, 10, 30)); // kToday + 2 days: 30% tier
    o.cart += domain::CartItem(syrup, 1);
    const auto rule = freshSaver();
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(30));
    EXPECT_EQ(rule.maxPercentOff(syrup), 50);
    EXPECT_FALSE(rule.maxPercentOff(phone()).has_value());
}

TEST(ExpiryMarkdown, FarFromExpiryNotApplicable) {
    const auto milk = makeMilk("G1", 8);
    Order o;
    o.cart += domain::CartItem(milk, 1);
    EXPECT_FALSE(freshSaver().isApplicable(o.context()));
}

TEST(ExpiryMarkdown, RejectsInvalidTiers) {
    EXPECT_THROW(ExpiryMarkdown("X", "x", 1, true, {}), std::invalid_argument);
    EXPECT_THROW(ExpiryMarkdown("X", "x", 1, true, {{-1, 10}}), std::invalid_argument);
    EXPECT_THROW(ExpiryMarkdown("X", "x", 1, true, {{3, 10}, {3, 20}}), std::invalid_argument);
    EXPECT_THROW(ExpiryMarkdown("X", "x", 1, true, {{3, 0}}), std::invalid_argument);
    EXPECT_THROW(ExpiryMarkdown("X", "x", 1, true, {{3, 101}}), std::invalid_argument);
}

TEST(ExpiryMarkdown, MaxPercentOffIsTopTierForGroceriesOnly) {
    const ExpiryMarkdown rule("X1", "Fresh saver", 1, true, {{7, 15}, {1, 50}, {3, 30}});
    const auto milk = makeMilk("G9", 2);
    EXPECT_EQ(rule.maxPercentOff(milk), std::optional<int>(50));
    EXPECT_EQ(rule.maxPercentOff(phone()), std::nullopt);
    EXPECT_EQ(rule.scope(), nullptr);
    EXPECT_FALSE(rule.activePeriod().has_value());
}
