#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>

#include "domain/TimeOfDay.hpp"

using domain::TimeOfDay;

TEST(TimeOfDay, StoresHourAndMinute) {
    const TimeOfDay t(18, 5);
    EXPECT_EQ(t.hour(), 18);
    EXPECT_EQ(t.minute(), 5);
    EXPECT_EQ(t.minutesSinceMidnight(), 18 * 60 + 5);
    EXPECT_EQ(t.toString(), "18:05");
    EXPECT_EQ(TimeOfDay(0, 0).toString(), "00:00");
}

TEST(TimeOfDay, RejectsOutOfRangeValues) {
    EXPECT_THROW(TimeOfDay(24, 0), std::invalid_argument);
    EXPECT_THROW(TimeOfDay(-1, 0), std::invalid_argument);
    EXPECT_THROW(TimeOfDay(10, 60), std::invalid_argument);
    EXPECT_THROW(TimeOfDay(10, -1), std::invalid_argument);
}

TEST(TimeOfDay, ParsesHourColonMinute) {
    EXPECT_EQ(TimeOfDay::fromString("18:30"), TimeOfDay(18, 30));
    EXPECT_EQ(TimeOfDay::fromString("9:05"), TimeOfDay(9, 5));
    EXPECT_EQ(TimeOfDay::fromString("23:59"), TimeOfDay(23, 59));
}

TEST(TimeOfDay, RejectsBadText) {
    for (const char* bad : {"", "18", "18:", ":30", "18:3", "18:300", "123:00", "ab:cd", "18-30",
                            "24:00", "12:60", " 9:00"}) {
        EXPECT_THROW(TimeOfDay::fromString(bad), std::invalid_argument) << bad;
    }
}

TEST(TimeOfDay, ComparesByClockTime) {
    const TimeOfDay morning(9, 0), evening(18, 0);
    EXPECT_LT(morning, evening);
    EXPECT_GT(evening, morning);
    EXPECT_LE(morning, TimeOfDay(9, 0));
    EXPECT_GE(evening, TimeOfDay(18, 0));
    EXPECT_NE(morning, evening);
}

TEST(TimeOfDay, StreamsAsText) {
    std::ostringstream out;
    out << TimeOfDay(7, 45);
    EXPECT_EQ(out.str(), "07:45");
}
