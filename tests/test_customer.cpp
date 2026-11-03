#include <gtest/gtest.h>

#include <stdexcept>

#include "domain/Customer.hpp"

using domain::Customer;
using domain::MembershipTier;

TEST(Customer, StoresFields) {
    const Customer c("CU1", "Asha", MembershipTier::Gold);
    EXPECT_EQ(c.id(), "CU1");
    EXPECT_EQ(c.name(), "Asha");
    EXPECT_EQ(c.tier(), MembershipTier::Gold);
}

TEST(Customer, DefaultsToRegularTier) {
    const Customer c("CU2", "Ravi");
    EXPECT_EQ(c.tier(), MembershipTier::Regular);
}

TEST(Customer, RejectsEmptyIdOrName) {
    EXPECT_THROW(Customer("", "Ravi"), std::invalid_argument);
    EXPECT_THROW(Customer("CU3", ""), std::invalid_argument);
}

TEST(Customer, TierToString) {
    EXPECT_EQ(domain::toString(MembershipTier::Regular), "Regular");
    EXPECT_EQ(domain::toString(MembershipTier::Silver), "Silver");
    EXPECT_EQ(domain::toString(MembershipTier::Gold), "Gold");
    EXPECT_EQ(domain::toString(MembershipTier::Platinum), "Platinum");
}

TEST(Customer, AllTiersLowestFirst) {
    const auto& tiers = domain::allMembershipTiers();
    ASSERT_EQ(tiers.size(), 4u);
    EXPECT_EQ(tiers[0], MembershipTier::Regular);
    EXPECT_EQ(tiers[1], MembershipTier::Silver);
    EXPECT_EQ(tiers[2], MembershipTier::Gold);
    EXPECT_EQ(tiers[3], MembershipTier::Platinum);
}

TEST(Customer, TierFromStringIgnoresCase) {
    EXPECT_EQ(domain::membershipTierFromString("Gold"), MembershipTier::Gold);
    EXPECT_EQ(domain::membershipTierFromString("PLATINUM"), MembershipTier::Platinum);
    EXPECT_EQ(domain::membershipTierFromString("silver"), MembershipTier::Silver);
    for (const auto tier : domain::allMembershipTiers()) {
        EXPECT_EQ(domain::membershipTierFromString(domain::toString(tier)), tier);
    }
}

TEST(Customer, TierFromStringRejectsUnknown) {
    EXPECT_THROW(domain::membershipTierFromString("Diamond"), std::invalid_argument);
    EXPECT_THROW(domain::membershipTierFromString(""), std::invalid_argument);
}

TEST(Customer, CountsOrdersPlaced) {
    Customer c("CU1", "Asha");
    EXPECT_EQ(c.ordersPlaced(), 0);
    EXPECT_TRUE(c.isFirstOrder());
    c.recordOrder();
    c.recordOrder();
    EXPECT_EQ(c.ordersPlaced(), 2);
    EXPECT_FALSE(c.isFirstOrder());
    EXPECT_EQ(Customer("CU2", "Ravi", MembershipTier::Silver, 3).ordersPlaced(), 3);
    EXPECT_THROW(Customer("CU3", "X", MembershipTier::Regular, -1), std::invalid_argument);
}
