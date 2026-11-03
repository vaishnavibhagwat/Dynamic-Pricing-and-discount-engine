#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "rules/ICartLevelRule.hpp"
#include "rules/IItemLevelRule.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::ItemFilter;
using rules::PercentageDiscount;

TEST(PercentageDiscount, ExposesCommonRuleApi) {
    const PercentageDiscount rule("P1", "10% off", 70, false, 10);
    EXPECT_EQ(rule.id(), "P1");
    EXPECT_EQ(rule.name(), "10% off");
    EXPECT_EQ(rule.priority(), 70);
    EXPECT_FALSE(rule.stackable());
    EXPECT_EQ(rule.percent(), 10);
}

TEST(PercentageDiscount, IsAnItemLevelRuleNotACartLevelRule) {
    EXPECT_TRUE((std::is_base_of_v<rules::IItemLevelRule, PercentageDiscount>));
    EXPECT_FALSE((std::is_base_of_v<rules::ICartLevelRule, PercentageDiscount>));
    EXPECT_TRUE((std::is_abstract_v<rules::DiscountRule>));
}

TEST(PercentageDiscount, DiscountsOnlyMatchingCategory) {
    Order o;
    o.cart += domain::CartItem(chair(), 2);  // 8000
    o.cart += domain::CartItem(kurta(), 1);  // 1000, not furniture
    const PercentageDiscount rule("P1", "Furniture 10%", 1, true, 10,
                                  ItemFilter::forCategory("Furniture"));
    const auto ctx = o.context();
    EXPECT_TRUE(rule.isApplicable(ctx));
    EXPECT_EQ(rule.calculateDiscount(ctx), rs(800));
    EXPECT_TRUE(rule.rejectionReason(ctx).empty());
}

TEST(PercentageDiscount, NotApplicableWhenNoItemMatches) {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    const PercentageDiscount rule("P1", "Furniture 10%", 1, true, 10,
                                  ItemFilter::forCategory("Furniture"));
    const auto ctx = o.context();
    EXPECT_FALSE(rule.isApplicable(ctx));
    EXPECT_EQ(rule.calculateDiscount(ctx), Money::zero());
    EXPECT_EQ(rule.rejectionReason(ctx), "no item from Furniture in cart");
}

TEST(PercentageDiscount, EmptyCartIsNotApplicable) {
    const Order o;
    const PercentageDiscount rule("P1", "All 10%", 1, true, 10);
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), Money::zero());
}

TEST(PercentageDiscount, RoundsToNearestPaisa) {
    Order o;
    o.cart += domain::CartItem(oddPricedCharger(), 1);  // 999.99
    const PercentageDiscount rule("P1", "All 15%", 1, true, 15);
    // 15% of 99999 paise = 14999.85 -> 15000 paise
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(150));
}

TEST(PercentageDiscount, HundredPercentMakesItemFree) {
    Order o;
    o.cart += domain::CartItem(kurta(), 3);
    const PercentageDiscount rule("P1", "Free", 1, true, 100);
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(3000));
}

TEST(PercentageDiscount, RejectsOutOfRangePercent) {
    EXPECT_THROW(PercentageDiscount("P", "x", 1, true, 0), std::invalid_argument);
    EXPECT_THROW(PercentageDiscount("P", "x", 1, true, 101), std::invalid_argument);
    EXPECT_THROW(PercentageDiscount("P", "x", 1, true, -5), std::invalid_argument);
}

TEST(PercentageDiscount, RejectsEmptyIdOrName) {
    EXPECT_THROW(PercentageDiscount("", "x", 1, true, 10), std::invalid_argument);
    EXPECT_THROW(PercentageDiscount("P", "", 1, true, 10), std::invalid_argument);
}

TEST(PercentageDiscount, WorksThroughBasePointer) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    const std::unique_ptr<rules::DiscountRule> rule =
        std::make_unique<PercentageDiscount>("P1", "5%", 1, true, 5);
    EXPECT_EQ(rule->calculateDiscount(o.context()), rs(750));
}

TEST(PercentageDiscount, DescribesItsScopeForTheValidator) {
    const PercentageDiscount rule("P1", "10% off Furniture", 1, true, 10,
                                  ItemFilter::forCategory("Furniture"));
    ASSERT_NE(rule.scope(), nullptr);
    EXPECT_EQ(rule.scope()->category(), std::optional<std::string>("Furniture"));
    EXPECT_FALSE(rule.activePeriod().has_value());
    EXPECT_EQ(rule.maxPercentOff(chair()), std::optional<int>(10));
    EXPECT_EQ(rule.maxPercentOff(phone()), std::nullopt);
}
