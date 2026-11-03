#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "rules/FirstOrderRule.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::FirstOrderRule;

namespace {

FirstOrderRule welcome() {
    return FirstOrderRule(std::make_unique<rules::PercentageDiscount>("W", "Welcome", 1, true, 10));
}

} // namespace

TEST(FirstOrderRule, AppliesOnlyBeforeTheFirstCheckout) {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    const auto rule = welcome();
    ASSERT_TRUE(o.customer.isFirstOrder());
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(100));

    o.customer.recordOrder();
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), Money::zero());
    EXPECT_EQ(rule.rejectionReason(o.context()), "first order only; Asha has placed 1 order");

    o.customer.recordOrder();
    EXPECT_EQ(rule.rejectionReason(o.context()), "first order only; Asha has placed 2 orders");
}

TEST(FirstOrderRule, ReturningCustomerFromDataIsRefused) {
    Order o;
    o.customer = domain::Customer("CU2", "Ravi", domain::MembershipTier::Silver, 3);
    o.cart += domain::CartItem(kurta(), 1);
    EXPECT_FALSE(welcome().isApplicable(o.context()));
}

TEST(FirstOrderRule, NameAndNullCheck) {
    EXPECT_EQ(welcome().name(), "Welcome [first order only]");
    EXPECT_THROW(FirstOrderRule(nullptr), std::invalid_argument);
}
