#include <gtest/gtest.h>

#include <stdexcept>

#include "domain/DateRange.hpp"

using domain::Date;
using domain::DateRange;

TEST(DateRange, RejectsEndBeforeStart) {
    EXPECT_THROW(DateRange(Date(2026, 11, 2), Date(2026, 11, 1)), std::invalid_argument);
    EXPECT_NO_THROW(DateRange(Date(2026, 11, 1), Date(2026, 11, 1)));
}

TEST(DateRange, ContainsIsInclusiveAtBothEnds) {
    const DateRange r(Date(2026, 10, 25), Date(2026, 11, 10));
    EXPECT_TRUE(r.contains(Date(2026, 10, 25)));
    EXPECT_TRUE(r.contains(Date(2026, 11, 10)));
    EXPECT_FALSE(r.contains(Date(2026, 10, 24)));
    EXPECT_FALSE(r.contains(Date(2026, 11, 11)));
}

TEST(DateRange, IntersectReturnsSharedDays) {
    const DateRange a(Date(2026, 10, 20), Date(2026, 11, 20));
    const DateRange b(Date(2026, 11, 10), Date(2026, 12, 10));
    const auto shared = a.intersect(b);
    ASSERT_TRUE(shared.has_value());
    EXPECT_EQ(*shared, DateRange(Date(2026, 11, 10), Date(2026, 11, 20)));
    EXPECT_EQ(b.intersect(a), shared);
}

TEST(DateRange, RangesTouchingOnOneDayShareThatDay) {
    const DateRange a(Date(2026, 1, 1), Date(2026, 1, 10));
    const DateRange b(Date(2026, 1, 10), Date(2026, 1, 20));
    EXPECT_EQ(a.intersect(b), DateRange(Date(2026, 1, 10), Date(2026, 1, 10)));
}

TEST(DateRange, DisjointRangesDoNotIntersect) {
    const DateRange a(Date(2026, 1, 1), Date(2026, 1, 10));
    const DateRange b(Date(2026, 1, 11), Date(2026, 1, 20));
    EXPECT_FALSE(a.intersect(b).has_value());
}

TEST(DateRange, ToStringShowsBothEnds) {
    EXPECT_EQ(DateRange(Date(2027, 1, 15), Date(2027, 1, 31)).toString(),
              "2027-01-15 to 2027-01-31");
}
