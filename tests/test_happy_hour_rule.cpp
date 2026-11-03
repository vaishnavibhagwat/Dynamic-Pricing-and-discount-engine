#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "rules/HappyHourRule.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::TimeOfDay;
using rules::HappyHourRule;

namespace {

std::unique_ptr<rules::DiscountRule> tenPercent() {
    return std::make_unique<rules::PercentageDiscount>("P10", "Ten off", 1, true, 10);
}

HappyHourRule evening() {
    return HappyHourRule(tenPercent(), TimeOfDay(18, 0), TimeOfDay(21, 0));
}

Order kurtaAt(std::optional<TimeOfDay> time) {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    o.time = time;
    return o;
}

} // namespace

TEST(HappyHourRule, AppliesInsideTheWindowOnly) {
    const auto rule = evening();
    EXPECT_TRUE(rule.isApplicable(kurtaAt(TimeOfDay(18, 0)).context()));  // start included
    EXPECT_TRUE(rule.isApplicable(kurtaAt(TimeOfDay(20, 59)).context()));
    EXPECT_FALSE(rule.isApplicable(kurtaAt(TimeOfDay(21, 0)).context())); // end excluded
    EXPECT_FALSE(rule.isApplicable(kurtaAt(TimeOfDay(17, 59)).context()));
    EXPECT_EQ(rule.calculateDiscount(kurtaAt(TimeOfDay(19, 0)).context()), rs(100));
    EXPECT_EQ(rule.calculateDiscount(kurtaAt(TimeOfDay(9, 0)).context()), Money::zero());
}

TEST(HappyHourRule, ExplainsWhyItWasRejected) {
    const auto rule = evening();
    EXPECT_EQ(rule.rejectionReason(kurtaAt(TimeOfDay(14, 5)).context()),
              "only between 18:00 and 21:00 (ordered at 14:05)");
    EXPECT_EQ(rule.rejectionReason(kurtaAt(std::nullopt).context()),
              "only between 18:00 and 21:00; order time unknown");
    EXPECT_EQ(rule.rejectionReason(kurtaAt(TimeOfDay(19, 0)).context()), "");
}

TEST(HappyHourRule, WindowCanCrossMidnight) {
    const HappyHourRule late(tenPercent(), TimeOfDay(22, 0), TimeOfDay(2, 0));
    EXPECT_TRUE(late.contains(TimeOfDay(23, 30)));
    EXPECT_TRUE(late.contains(TimeOfDay(0, 0)));
    EXPECT_TRUE(late.contains(TimeOfDay(1, 59)));
    EXPECT_FALSE(late.contains(TimeOfDay(2, 0)));
    EXPECT_FALSE(late.contains(TimeOfDay(12, 0)));
}

TEST(HappyHourRule, NameAndAccessors) {
    const auto rule = evening();
    EXPECT_EQ(rule.name(), "Ten off [happy hour 18:00-21:00]");
    EXPECT_EQ(rule.from(), TimeOfDay(18, 0));
    EXPECT_EQ(rule.to(), TimeOfDay(21, 0));
}

TEST(HappyHourRule, RejectsEmptyWindow) {
    EXPECT_THROW(HappyHourRule(tenPercent(), TimeOfDay(9, 0), TimeOfDay(9, 0)),
                 std::invalid_argument);
}
