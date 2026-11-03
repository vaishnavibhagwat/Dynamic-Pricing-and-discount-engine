#include <gtest/gtest.h>

#include <stdexcept>

#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::Date;
using rules::ItemFilter;
using rules::SeasonalDiscount;

namespace {
SeasonalDiscount diwali() {
    return SeasonalDiscount("S1", "Diwali", 1, true, Date(2026, 10, 25), Date(2026, 11, 10), 20,
                            ItemFilter::forCategory("Electronics"));
}

Order phoneOrderOn(const Date& day) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    o.date = day;
    return o;
}
} // namespace

TEST(SeasonalDiscount, AppliesInsideRange) {
    const auto o = phoneOrderOn(Date(2026, 10, 28));
    EXPECT_TRUE(diwali().isApplicable(o.context()));
    EXPECT_EQ(diwali().calculateDiscount(o.context()), rs(3000));
}

TEST(SeasonalDiscount, StartAndEndDaysAreInclusive) {
    EXPECT_TRUE(diwali().isApplicable(phoneOrderOn(Date(2026, 10, 25)).context()));
    EXPECT_TRUE(diwali().isApplicable(phoneOrderOn(Date(2026, 11, 10)).context()));
}

TEST(SeasonalDiscount, NotApplicableOneDayOutside) {
    const auto before = phoneOrderOn(Date(2026, 10, 24));
    const auto after = phoneOrderOn(Date(2026, 11, 11));
    EXPECT_FALSE(diwali().isApplicable(before.context()));
    EXPECT_FALSE(diwali().isApplicable(after.context()));
    EXPECT_EQ(diwali().calculateDiscount(after.context()), Money::zero());
    EXPECT_EQ(diwali().rejectionReason(after.context()),
              "order date 2026-11-11 is outside 2026-10-25 to 2026-11-10");
}

TEST(SeasonalDiscount, InRangeButNoMatchingCategory) {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    EXPECT_FALSE(diwali().isApplicable(o.context()));
    EXPECT_EQ(diwali().rejectionReason(o.context()), "no item from Electronics in cart");
}

TEST(SeasonalDiscount, ItemHookAlsoChecksDate) {
    // discountForItem is public via IItemLevelRule; it must not leak a discount out of season.
    const auto o = phoneOrderOn(Date(2027, 1, 1));
    EXPECT_EQ(diwali().discountForItem(o.cart.items().front(), o.context()), Money::zero());
}

TEST(SeasonalDiscount, SingleDaySale) {
    const SeasonalDiscount flash("S2", "Flash", 1, true, Date(2026, 10, 28), Date(2026, 10, 28),
                                 5);
    EXPECT_TRUE(flash.isApplicable(phoneOrderOn(Date(2026, 10, 28)).context()));
    EXPECT_FALSE(flash.isApplicable(phoneOrderOn(Date(2026, 10, 29)).context()));
}

TEST(SeasonalDiscount, RejectsStartAfterEnd) {
    EXPECT_THROW(SeasonalDiscount("S", "x", 1, true, Date(2026, 11, 2), Date(2026, 11, 1), 10),
                 std::invalid_argument);
}

TEST(SeasonalDiscount, RejectsBadPercent) {
    EXPECT_THROW(SeasonalDiscount("S", "x", 1, true, Date(2026, 1, 1), Date(2026, 1, 2), 0),
                 std::invalid_argument);
}

TEST(SeasonalDiscount, ReportsItsDatesAndScopeForTheValidator) {
    const SeasonalDiscount rule("S1", "Diwali", 1, true, Date(2026, 10, 25), Date(2026, 11, 10), 20,
                                ItemFilter::forCategory("Electronics"));
    ASSERT_TRUE(rule.activePeriod().has_value());
    EXPECT_EQ(*rule.activePeriod(), domain::DateRange(Date(2026, 10, 25), Date(2026, 11, 10)));
    ASSERT_NE(rule.scope(), nullptr);
    EXPECT_EQ(rule.maxPercentOff(phone()), std::optional<int>(20));
    EXPECT_EQ(rule.maxPercentOff(kurta()), std::nullopt);
}
