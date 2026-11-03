#include <gtest/gtest.h>

#include "application/ConflictPolicies.hpp"

TEST(ConflictPolicies, StandardListIsInMenuOrder) {
    const auto policies = application::standardConflictPolicies();
    ASSERT_EQ(policies.size(), 3u);
    for (const auto& p : policies) ASSERT_NE(p, nullptr);
    EXPECT_EQ(policies[0]->name(), "Stacking");
    EXPECT_EQ(policies[1]->name(), "BestDiscount");
    EXPECT_EQ(policies[2]->name(), "Priority");
}

TEST(ConflictPolicies, EachCallMakesNewObjects) {
    const auto a = application::standardConflictPolicies();
    const auto b = application::standardConflictPolicies();
    EXPECT_NE(a[0].get(), b[0].get());
}
