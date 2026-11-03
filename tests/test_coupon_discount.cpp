#include <gtest/gtest.h>

#include <stdexcept>

#include "rules/CouponDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::CouponDiscount;
using Benefit = rules::CouponDiscount::Benefit;

namespace {
CouponDiscount festive500(int limit = 1000) {
    return CouponDiscount("CP1", "Festive", 1, true, "FESTIVE500", rs(2999), limit,
                          Benefit::flat(rs(500)));
}

Order orderWorth(const domain::Product& p, int qty, std::optional<std::string> code) {
    Order o;
    o.cart += domain::CartItem(p, qty);
    o.coupon = std::move(code);
    return o;
}
} // namespace

TEST(CouponDiscount, FlatCouponApplies) {
    const auto o = orderWorth(kurta(), 3, "FESTIVE500");  // 3000
    const auto rule = festive500();
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(500));
}

TEST(CouponDiscount, CodeIsCaseInsensitive) {
    EXPECT_TRUE(festive500().isApplicable(orderWorth(kurta(), 3, "festive500").context()));
}

TEST(CouponDiscount, MissingOrWrongCodeRejected) {
    const auto none = orderWorth(kurta(), 3, std::nullopt);
    const auto wrong = orderWorth(kurta(), 3, "DIWALI");
    EXPECT_FALSE(festive500().isApplicable(none.context()));
    EXPECT_EQ(festive500().rejectionReason(none.context()), "no coupon code entered");
    EXPECT_FALSE(festive500().isApplicable(wrong.context()));
    EXPECT_EQ(festive500().rejectionReason(wrong.context()), "coupon code 'DIWALI' does not match");
}

TEST(CouponDiscount, MinimumCartValueBoundary) {
    const domain::Electronics exact("E9", "Speaker", rs(2000), rs(2999), rs(3500));
    const domain::Electronics justBelow("E8", "Speaker", rs(2000), rs(2998, 99), rs(3500));
    EXPECT_TRUE(festive500().isApplicable(orderWorth(exact, 1, "FESTIVE500").context()));
    const auto below = orderWorth(justBelow, 1, "FESTIVE500");
    EXPECT_FALSE(festive500().isApplicable(below.context()));
    EXPECT_NE(festive500().rejectionReason(below.context()).find("below minimum"),
              std::string::npos);
}

TEST(CouponDiscount, UsageLimitEnforcedAfterRecordUse) {
    auto rule = festive500(2);
    const auto o = orderWorth(kurta(), 3, "FESTIVE500");
    rule.recordUse();
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.usesRemaining(), 1);
    rule.recordUse();
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), Money::zero());
    EXPECT_EQ(rule.rejectionReason(o.context()), "usage limit of 2 reached");
    EXPECT_THROW(rule.recordUse(), std::logic_error);
    EXPECT_EQ(rule.usesSoFar(), 2);
}

TEST(CouponDiscount, EvaluatingDoesNotConsumeUses) {
    const auto rule = festive500(1);
    const auto o = orderWorth(kurta(), 3, "FESTIVE500");
    for (int i = 0; i < 5; ++i) EXPECT_EQ(rule.calculateDiscount(o.context()), rs(500));
    EXPECT_EQ(rule.usesSoFar(), 0);
}

TEST(CouponDiscount, FlatAmountNeverExceedsCartValue) {
    const CouponDiscount big("CP2", "Big", 1, true, "BIG", Money::zero(), 5,
                             Benefit::flat(rs(5000)));
    const auto o = orderWorth(kurta(), 1, "BIG");  // 1000
    EXPECT_EQ(big.calculateDiscount(o.context()), rs(1000));
}

TEST(CouponDiscount, PercentCouponWithCap) {
    const CouponDiscount newUser("CP3", "New user", 1, false, "NEWUSER15", rs(499), 1,
                                 Benefit::percent(15, rs(300)));
    EXPECT_EQ(newUser.calculateDiscount(orderWorth(kurta(), 1, "NEWUSER15").context()),
              rs(150));  // 15% of 1000, under cap
    EXPECT_EQ(newUser.calculateDiscount(orderWorth(kurta(), 3, "NEWUSER15").context()),
              rs(300));  // 450 capped to 300
}

TEST(CouponDiscount, PercentCouponWithoutCap) {
    const CouponDiscount c("CP4", "x", 1, true, "TEN", Money::zero(), 1, Benefit::percent(10));
    EXPECT_EQ(c.calculateDiscount(orderWorth(phone(), 1, "TEN").context()), rs(1500));
}

TEST(CouponDiscount, EmptyCartRejected) {
    const CouponDiscount c("CP4", "x", 1, true, "TEN", Money::zero(), 1, Benefit::percent(10));
    Order o;
    o.coupon = "TEN";
    EXPECT_FALSE(c.isApplicable(o.context()));
    EXPECT_EQ(c.rejectionReason(o.context()), "cart is empty");
}

TEST(CouponDiscount, BenefitDescriptions) {
    EXPECT_EQ(Benefit::percent(15, rs(300)).describe(), "15% off up to \xE2\x82\xB9" "300.00");
    EXPECT_EQ(Benefit::flat(rs(500)).describe(), "\xE2\x82\xB9" "500.00 off");
}

TEST(CouponDiscount, RejectsInvalidConfiguration) {
    EXPECT_THROW(Benefit::percent(0), std::invalid_argument);
    EXPECT_THROW(Benefit::percent(10, Money::zero()), std::invalid_argument);
    EXPECT_THROW(Benefit::flat(Money::zero()), std::invalid_argument);
    EXPECT_THROW(CouponDiscount("C", "x", 1, true, "", Money::zero(), 1, Benefit::flat(rs(1))),
                 std::invalid_argument);
    EXPECT_THROW(CouponDiscount("C", "x", 1, true, "X", -rs(1), 1, Benefit::flat(rs(1))),
                 std::invalid_argument);
    EXPECT_THROW(CouponDiscount("C", "x", 1, true, "X", Money::zero(), 0, Benefit::flat(rs(1))),
                 std::invalid_argument);
}

TEST(CouponDiscount, IsOrderLevelSoHasNoItemScope) {
    const CouponDiscount rule("C9", "Flat 100", 1, true, "FLAT100", rs(0), 5, Benefit::flat(rs(100)));
    EXPECT_EQ(rule.scope(), nullptr);
    EXPECT_FALSE(rule.activePeriod().has_value());
    EXPECT_EQ(rule.maxPercentOff(phone()), std::nullopt);
}

TEST(CouponDiscount, RecordsUseThroughRedeemableInterface) {
    CouponDiscount coupon = festive500(1);
    rules::IRedeemable& redeemable = coupon;
    redeemable.recordUse();
    EXPECT_EQ(coupon.usesSoFar(), 1);
    EXPECT_EQ(coupon.usesRemaining(), 0);
    EXPECT_THROW(redeemable.recordUse(), std::logic_error);
}
