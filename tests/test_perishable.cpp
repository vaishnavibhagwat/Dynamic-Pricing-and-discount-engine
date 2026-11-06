#include <gtest/gtest.h>

#include "domain/Perishable.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;

TEST(Perishable, ExpiresTheDayAfterItsExpiryDate) {
    const auto milk = makeMilk("G1", 0); // expires on kToday
    EXPECT_FALSE(milk.isExpiredOn(kToday));
    EXPECT_TRUE(makeMilk("G2", -1).isExpiredOn(kToday));
}

TEST(Perishable, AsPerishableFindsTheCapability) {
    const auto milk = makeMilk("G1", 3);
    const domain::Product& asProduct = milk;
    EXPECT_EQ(domain::asPerishable(asProduct), static_cast<const domain::Perishable*>(&milk));
    EXPECT_EQ(domain::asPerishable(phone()), nullptr);
}

TEST(Perishable, IsExpiredOnForAnyProduct) {
    EXPECT_TRUE(domain::isExpiredOn(makeMilk("G1", -2), kToday));
    EXPECT_FALSE(domain::isExpiredOn(makeMilk("G1", 2), kToday));
    EXPECT_FALSE(domain::isExpiredOn(phone(), kToday)); // never expires
}
