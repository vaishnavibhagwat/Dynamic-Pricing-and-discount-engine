#include <gtest/gtest.h>

#include "rules/BankOfferDiscount.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/RuleStage.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::RuleStage;

TEST(RuleStage, DeclaredInPipelineOrder) {
    EXPECT_LT(RuleStage::Expiry, RuleStage::Seasonal);
    EXPECT_LT(RuleStage::Seasonal, RuleStage::Percentage);
    EXPECT_LT(RuleStage::Percentage, RuleStage::Quantity);
    EXPECT_LT(RuleStage::Quantity, RuleStage::Membership);
    EXPECT_LT(RuleStage::Membership, RuleStage::Coupon);
    EXPECT_LT(RuleStage::Coupon, RuleStage::BankOffer);
}

TEST(RuleStage, ItemStagesComeBeforeCartStages) {
    for (auto s : {RuleStage::Expiry, RuleStage::Seasonal, RuleStage::Percentage,
                   RuleStage::Quantity, RuleStage::Membership}) {
        EXPECT_TRUE(rules::isItemStage(s)) << rules::toString(s);
    }
    EXPECT_FALSE(rules::isItemStage(RuleStage::Coupon));
    EXPECT_FALSE(rules::isItemStage(RuleStage::BankOffer));
}

TEST(RuleStage, ToString) {
    EXPECT_EQ(rules::toString(RuleStage::Expiry), "Expiry");
    EXPECT_EQ(rules::toString(RuleStage::BankOffer), "BankOffer");
}

TEST(RuleStage, EachRuleReportsItsStage) {
    EXPECT_EQ(rules::ExpiryMarkdown("X", "x", 1, true, {{3, 30}}).stage(), RuleStage::Expiry);
    EXPECT_EQ(rules::SeasonalDiscount("S", "s", 1, true, kToday, kToday, 10).stage(),
              RuleStage::Seasonal);
    EXPECT_EQ(rules::PercentageDiscount("P", "p", 1, true, 10).stage(), RuleStage::Percentage);
    EXPECT_EQ(rules::QuantityDiscount("Q", "q", 1, true, 2, 10).stage(), RuleStage::Quantity);
    EXPECT_EQ(rules::MembershipDiscount("M", "m", 1, true, {{domain::MembershipTier::Gold, 5}})
                  .stage(),
              RuleStage::Membership);
    EXPECT_EQ(rules::CouponDiscount("C", "c", 1, true, "CODE", rs(0), 1,
                                    rules::CouponDiscount::Benefit::flat(rs(100)))
                  .stage(),
              RuleStage::Coupon);
    EXPECT_EQ(rules::BankOfferDiscount("B", "b", 1, true, "HDFC", 10, rs(500)).stage(),
              RuleStage::BankOffer);
}
