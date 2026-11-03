#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "rules/ICartLevelRule.hpp"
#include "rules/IItemLevelRule.hpp"
#include "rules/MembershipDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::MembershipTier;
using rules::MembershipDiscount;

namespace {
MembershipDiscount loyalty() {
    return MembershipDiscount("M1", "Loyalty", 1, true,
                              {{MembershipTier::Silver, 2},
                               {MembershipTier::Gold, 5},
                               {MembershipTier::Platinum, 10}});
}

Order orderFor(MembershipTier tier) {
    Order o;
    o.customer = domain::Customer("CU9", "Ravi", tier);
    o.cart += domain::CartItem(phone(), 1);  // 15000
    return o;
}
} // namespace

TEST(MembershipDiscount, OffersBothCartAndItemInterfaces) {
    EXPECT_TRUE((std::is_base_of_v<rules::ICartLevelRule, MembershipDiscount>));
    EXPECT_TRUE((std::is_base_of_v<rules::IItemLevelRule, MembershipDiscount>));
    EXPECT_EQ(loyalty().stage(), rules::RuleStage::Membership);
}

TEST(MembershipDiscount, PerLineDiscountIsTierPercentOfLine) {
    Order o;
    o.customer = domain::Customer("CU9", "Ravi", MembershipTier::Gold);
    o.cart += domain::CartItem(kurta(), 3);            // 3000
    o.cart += domain::CartItem(oddPricedCharger(), 1); // 999.99
    const auto ctx = o.context();
    const auto& lines = o.cart.items();
    EXPECT_TRUE(loyalty().appliesToItem(lines[0], ctx));
    EXPECT_EQ(loyalty().discountForItem(lines[0], ctx), rs(150));
    EXPECT_EQ(loyalty().discountForItem(lines[1], ctx), Money::fromPaise(5000)); // 49.9995 -> 50.00
}

TEST(MembershipDiscount, PerLineNotForTierWithoutDiscount) {
    const auto o = orderFor(MembershipTier::Regular);
    const auto ctx = o.context();
    EXPECT_FALSE(loyalty().appliesToItem(o.cart.items()[0], ctx));
    EXPECT_EQ(loyalty().discountForItem(o.cart.items()[0], ctx), Money::zero());
}

TEST(MembershipDiscount, PercentDependsOnTier) {
    EXPECT_EQ(loyalty().calculateDiscount(orderFor(MembershipTier::Silver).context()), rs(300));
    EXPECT_EQ(loyalty().calculateDiscount(orderFor(MembershipTier::Gold).context()), rs(750));
    EXPECT_EQ(loyalty().calculateDiscount(orderFor(MembershipTier::Platinum).context()), rs(1500));
}

TEST(MembershipDiscount, RegularCustomerNotApplicable) {
    const auto o = orderFor(MembershipTier::Regular);
    EXPECT_FALSE(loyalty().isApplicable(o.context()));
    EXPECT_EQ(loyalty().calculateDiscount(o.context()), Money::zero());
    EXPECT_EQ(loyalty().rejectionReason(o.context()), "no membership discount for Regular tier");
}

TEST(MembershipDiscount, EmptyCartNotApplicable) {
    Order o;
    o.customer = domain::Customer("CU9", "Ravi", MembershipTier::Gold);
    EXPECT_FALSE(loyalty().isApplicable(o.context()));
    EXPECT_EQ(loyalty().rejectionReason(o.context()), "cart is empty");
}

TEST(MembershipDiscount, DiscountOnAmountUsesGivenAmount) {
    // The engine may pass a total that item discounts already reduced.
    const auto o = orderFor(MembershipTier::Gold);
    EXPECT_EQ(loyalty().discountOnAmount(rs(10000), o.context()), rs(500));
    EXPECT_EQ(loyalty().discountOnAmount(Money::zero(), o.context()), Money::zero());
    EXPECT_EQ(loyalty().discountOnAmount(-rs(10), o.context()), Money::zero());
}

TEST(MembershipDiscount, RejectsInvalidConfiguration) {
    EXPECT_THROW(MembershipDiscount("M", "x", 1, true, {}), std::invalid_argument);
    EXPECT_THROW(MembershipDiscount("M", "x", 1, true, {{MembershipTier::Gold, 0}}),
                 std::invalid_argument);
}

TEST(MembershipDiscount, MaxPercentOffIsTopTierForAnyProduct) {
    const MembershipDiscount rule("M1", "Loyalty", 1, true,
                                  {{MembershipTier::Silver, 2}, {MembershipTier::Platinum, 10},
                                   {MembershipTier::Gold, 5}});
    EXPECT_EQ(rule.maxPercentOff(phone()), std::optional<int>(10));
    EXPECT_EQ(rule.maxPercentOff(kurta()), std::optional<int>(10));
    EXPECT_EQ(rule.scope(), nullptr);
}
