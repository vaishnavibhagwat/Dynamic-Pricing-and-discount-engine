#include <gtest/gtest.h>

#include <stdexcept>

#include "rules/BankOfferDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::BankOfferDiscount;

namespace {
BankOfferDiscount hdfc() {
    return BankOfferDiscount("B1", "HDFC 10% up to 1500", 1, true, "HDFC", 10, rs(1500), rs(5000));
}

Order paidWith(const domain::Product& p, int qty, std::optional<std::string> bank) {
    Order o;
    o.cart += domain::CartItem(p, qty);
    o.bank = std::move(bank);
    return o;
}
} // namespace

TEST(BankOfferDiscount, BelowCapGivesFullPercent) {
    const auto o = paidWith(phone(), 1, "HDFC");  // 15000 -> 1500 (exactly the cap)
    EXPECT_TRUE(hdfc().isApplicable(o.context()));
    EXPECT_EQ(hdfc().calculateDiscount(o.context()), rs(1500));
    const auto small = paidWith(chair(), 2, "HDFC");  // 8000 -> 800
    EXPECT_EQ(hdfc().calculateDiscount(small.context()), rs(800));
}

TEST(BankOfferDiscount, CapLimitsLargeOrders) {
    const auto o = paidWith(phone(), 4, "HDFC");  // 60000 -> 6000, capped
    EXPECT_EQ(hdfc().calculateDiscount(o.context()), rs(1500));
}

TEST(BankOfferDiscount, BankNameIsCaseInsensitive) {
    EXPECT_TRUE(hdfc().isApplicable(paidWith(phone(), 1, "hdfc").context()));
}

TEST(BankOfferDiscount, WrongOrNoBankRejected) {
    const auto sbi = paidWith(phone(), 1, "SBI");
    const auto cash = paidWith(phone(), 1, std::nullopt);
    EXPECT_FALSE(hdfc().isApplicable(sbi.context()));
    EXPECT_EQ(hdfc().rejectionReason(sbi.context()), "offer is for HDFC, paid with SBI");
    EXPECT_FALSE(hdfc().isApplicable(cash.context()));
    EXPECT_EQ(hdfc().rejectionReason(cash.context()), "no bank card used for payment");
    EXPECT_EQ(hdfc().calculateDiscount(cash.context()), Money::zero());
}

TEST(BankOfferDiscount, MinimumTransactionBoundary) {
    const auto atMin = paidWith(kurta(), 5, "HDFC");    // 5000
    const auto below = paidWith(kurta(), 4, "HDFC");    // 4000
    EXPECT_TRUE(hdfc().isApplicable(atMin.context()));
    EXPECT_EQ(hdfc().calculateDiscount(atMin.context()), rs(500));
    EXPECT_FALSE(hdfc().isApplicable(below.context()));
    EXPECT_NE(hdfc().rejectionReason(below.context()).find("below minimum"), std::string::npos);
}

TEST(BankOfferDiscount, NoMinimumByDefault) {
    const BankOfferDiscount icici("B2", "ICICI", 1, true, "ICICI", 5, rs(750));
    EXPECT_EQ(icici.calculateDiscount(paidWith(kurta(), 1, "ICICI").context()), rs(50));
}

TEST(BankOfferDiscount, CapAppliesToGivenAmount) {
    const auto o = paidWith(phone(), 1, "HDFC");
    EXPECT_EQ(hdfc().discountOnAmount(rs(100000), o.context()), rs(1500));
    EXPECT_EQ(hdfc().discountOnAmount(rs(2000), o.context()), rs(200));
    EXPECT_EQ(hdfc().discountOnAmount(Money::zero(), o.context()), Money::zero());
}

TEST(BankOfferDiscount, RejectsInvalidConfiguration) {
    EXPECT_THROW(BankOfferDiscount("B", "x", 1, true, "", 10, rs(100)), std::invalid_argument);
    EXPECT_THROW(BankOfferDiscount("B", "x", 1, true, "HDFC", 0, rs(100)), std::invalid_argument);
    EXPECT_THROW(BankOfferDiscount("B", "x", 1, true, "HDFC", 10, Money::zero()),
                 std::invalid_argument);
    EXPECT_THROW(BankOfferDiscount("B", "x", 1, true, "HDFC", 10, rs(100), -rs(1)),
                 std::invalid_argument);
}

TEST(BankOfferDiscount, IsOrderLevelSoHasNoItemScope) {
    const BankOfferDiscount rule("B9", "HDFC 10%", 1, true, "HDFC", 10, rs(1500));
    EXPECT_EQ(rule.scope(), nullptr);
    EXPECT_FALSE(rule.activePeriod().has_value());
    EXPECT_EQ(rule.maxPercentOff(phone()), std::nullopt);
}
