#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "rules/CappedRule.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::CappedRule;

namespace {

std::unique_ptr<rules::DiscountRule> twentyPercent() {
    return std::make_unique<rules::PercentageDiscount>("P20", "Twenty off", 1, true, 20);
}

} // namespace

TEST(CappedRule, CapsEachLineOfAnItemOffer) {
    Order o;
    o.cart += domain::CartItem(phone(), 1); // 20% = 3000 -> capped to 500
    o.cart += domain::CartItem(kurta(), 1); // 20% = 200, under the cap
    const CappedRule rule(twentyPercent(), rs(500));
    EXPECT_EQ(rule.cap(), rs(500));
    EXPECT_EQ(rule.discountForItem(o.cart.items()[0], o.context()), rs(500));
    EXPECT_EQ(rule.discountForItem(o.cart.items()[1], o.context()), rs(200));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(700));
}

TEST(CappedRule, CapsAnOrderOffer) {
    Order o;
    o.cart += domain::CartItem(chair(), 1);
    o.coupon = "BIG";
    auto coupon = std::make_unique<rules::CouponDiscount>(
        "CP", "Big coupon", 1, true, "BIG", rs(0), 10, rules::CouponDiscount::Benefit::percent(50));
    const CappedRule rule(std::move(coupon), rs(300));
    EXPECT_EQ(rule.discountOnAmount(rs(4000), o.context()), rs(300));
    EXPECT_EQ(rule.discountOnAmount(rs(400), o.context()), rs(200)); // 50% is under the cap
}

TEST(CappedRule, NameShowsTheCap) {
    const CappedRule rule(twentyPercent(), rs(500));
    EXPECT_EQ(rule.name(), "Twenty off [max ₹500.00 off]");
}

TEST(CappedRule, RejectsNonPositiveCapAndNullRule) {
    EXPECT_THROW(CappedRule(twentyPercent(), Money::zero()), std::invalid_argument);
    EXPECT_THROW(CappedRule(twentyPercent(), rs(-1)), std::invalid_argument);
    EXPECT_THROW(CappedRule(nullptr, rs(100)), std::invalid_argument);
}
