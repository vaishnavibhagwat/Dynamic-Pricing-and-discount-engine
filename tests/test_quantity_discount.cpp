#include <gtest/gtest.h>

#include <stdexcept>

#include "rules/QuantityDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::ItemFilter;
using rules::QuantityDiscount;

namespace {
QuantityDiscount buy3Clothing() {
    return QuantityDiscount("Q1", "Buy 3 get 15%", 1, true, 3, 15,
                            ItemFilter::forCategory("Clothing"));
}
} // namespace

TEST(QuantityDiscount, AppliesExactlyAtThreshold) {
    Order o;
    o.cart += domain::CartItem(kurta(), 3);
    const auto rule = buy3Clothing();
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(450));  // 15% of 3000
}

TEST(QuantityDiscount, NotApplicableOneBelowThreshold) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    const auto rule = buy3Clothing();
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), Money::zero());
    EXPECT_EQ(rule.rejectionReason(o.context()),
              "need at least 3 units of one product from Clothing");
}

TEST(QuantityDiscount, QuantitiesOfDifferentProductsAreNotCombined) {
    const domain::Clothing shirt("C2", "Shirt", rs(300), rs(800), rs(900), domain::Size::L,
                                 domain::Season::Summer);
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.cart += domain::CartItem(shirt, 2);
    EXPECT_FALSE(buy3Clothing().isApplicable(o.context()));
}

TEST(QuantityDiscount, MergedCartLinesCountTogether) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.cart += domain::CartItem(kurta(), 1);  // merges into one line of 3
    EXPECT_TRUE(buy3Clothing().isApplicable(o.context()));
}

TEST(QuantityDiscount, IgnoresNonMatchingCategoryEvenWithHighQuantity) {
    Order o;
    o.cart += domain::CartItem(phone(), 5);
    o.cart += domain::CartItem(kurta(), 4);
    EXPECT_EQ(buy3Clothing().calculateDiscount(o.context()), rs(600));  // only kurtas
}

TEST(QuantityDiscount, RejectsInvalidSettings) {
    EXPECT_THROW(QuantityDiscount("Q", "x", 1, true, 0, 10), std::invalid_argument);
    EXPECT_THROW(QuantityDiscount("Q", "x", 1, true, 2, 0), std::invalid_argument);
    EXPECT_THROW(QuantityDiscount("Q", "x", 1, true, 2, 101), std::invalid_argument);
}

TEST(QuantityDiscount, DescribesItsScopeForTheValidator) {
    const QuantityDiscount rule("Q1", "Buy 3 kurtas", 1, true, 3, 15, ItemFilter(std::nullopt, {"C1"}));
    ASSERT_NE(rule.scope(), nullptr);
    EXPECT_FALSE(rule.activePeriod().has_value());
    EXPECT_EQ(rule.maxPercentOff(kurta()), std::optional<int>(15));
    EXPECT_EQ(rule.maxPercentOff(chair()), std::nullopt);
}
