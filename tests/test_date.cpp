#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

#include "domain/Date.hpp"

using domain::Date;

TEST(Date, StoresFields) {
    const Date d(2026, 10, 28);
    EXPECT_EQ(d.year(), 2026);
    EXPECT_EQ(d.month(), 10);
    EXPECT_EQ(d.day(), 28);
}

TEST(Date, RejectsImpossibleDates) {
    EXPECT_THROW(Date(2026, 2, 30), std::invalid_argument);
    EXPECT_THROW(Date(2026, 13, 1), std::invalid_argument);
    EXPECT_THROW(Date(2026, 0, 1), std::invalid_argument);
    EXPECT_THROW(Date(2026, 4, 31), std::invalid_argument);
    EXPECT_THROW(Date(2026, 1, 0), std::invalid_argument);
    EXPECT_THROW(Date(2025, 2, 29), std::invalid_argument);
}

TEST(Date, HandlesLeapYears) {
    EXPECT_NO_THROW(Date(2024, 2, 29));
    EXPECT_NO_THROW(Date(2000, 2, 29));
    EXPECT_THROW(Date(1900, 2, 29), std::invalid_argument);
    EXPECT_TRUE(Date::isLeapYear(2024));
    EXPECT_FALSE(Date::isLeapYear(2100));
}

TEST(Date, Ordering) {
    EXPECT_TRUE(Date(2026, 1, 1) < Date(2026, 1, 2));
    EXPECT_TRUE(Date(2025, 12, 31) < Date(2026, 1, 1));
    EXPECT_TRUE(Date(2026, 3, 1) > Date(2026, 2, 28));
    EXPECT_TRUE(Date(2026, 5, 5) == Date(2026, 5, 5));
    EXPECT_TRUE(Date(2026, 5, 5) != Date(2026, 5, 6));
    EXPECT_TRUE(Date(2026, 5, 5) <= Date(2026, 5, 5));
    EXPECT_TRUE(Date(2026, 5, 5) >= Date(2026, 5, 4));
}

TEST(Date, DaysUntil) {
    EXPECT_EQ(Date(2026, 1, 1).daysUntil(Date(2026, 1, 1)), 0);
    EXPECT_EQ(Date(2026, 1, 1).daysUntil(Date(2026, 1, 31)), 30);
    EXPECT_EQ(Date(2024, 2, 28).daysUntil(Date(2024, 3, 1)), 2); // leap year
    EXPECT_EQ(Date(2025, 1, 1).daysUntil(Date(2026, 1, 1)), 365);
    EXPECT_EQ(Date(2026, 1, 10).daysUntil(Date(2026, 1, 1)), -9);
}

TEST(Date, Formatting) {
    EXPECT_EQ(Date(2026, 3, 7).toString(), "2026-03-07");
    std::ostringstream out;
    out << Date(2026, 12, 25);
    EXPECT_EQ(out.str(), "2026-12-25");
}

TEST(Date, FromStringParsesIsoDates) {
    EXPECT_EQ(Date::fromString("2026-10-28"), Date(2026, 10, 28));
    EXPECT_EQ(Date::fromString("2024-02-29"), Date(2024, 2, 29));
}

TEST(Date, FromStringRejectsBadInput) {
    EXPECT_THROW(Date::fromString(""), std::invalid_argument);
    EXPECT_THROW(Date::fromString("2026-1-5"), std::invalid_argument);
    EXPECT_THROW(Date::fromString("28-10-2026"), std::invalid_argument);
    EXPECT_THROW(Date::fromString("2026/10/28"), std::invalid_argument);
    EXPECT_THROW(Date::fromString("2026-10-2x"), std::invalid_argument);
    EXPECT_THROW(Date::fromString("2026-02-30"), std::invalid_argument);
}
