#include <gtest/gtest.h>

#include <stdexcept>

#include "engine/GstCalculator.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using engine::GstCalculator;

TEST(GstCalculator, PercentOfTaxableAmount) {
    const GstCalculator gst;
    EXPECT_EQ(gst.gstOn(rs(1000), 18), rs(180));
    EXPECT_EQ(gst.gstOn(rs(1000), 0), Money::zero());
    EXPECT_EQ(gst.gstOn(Money::zero(), 18), Money::zero());
}

TEST(GstCalculator, RoundsToNearestPaisa) {
    const GstCalculator gst;
    // 999.99 x 18% = 179.9982 -> 180.00
    EXPECT_EQ(gst.gstOn(Money::fromRupees(999, 99), 18), rs(180));
    // 0.10 x 5% = 0.005 -> 0.01 (half away from zero)
    EXPECT_EQ(gst.gstOn(Money::fromPaise(10), 5), Money::fromPaise(1));
}

TEST(GstCalculator, UsesProductRate) {
    const GstCalculator gst;
    EXPECT_EQ(gst.gstOn(rs(100), phone()), rs(18));
    EXPECT_EQ(gst.gstOn(rs(100), kurta()), rs(12));
    EXPECT_EQ(gst.gstOn(rs(100), makeMilk("G1", 5)), rs(5));
}

TEST(GstCalculator, RejectsBadRate) {
    const GstCalculator gst;
    EXPECT_THROW(gst.gstOn(rs(100), -1), std::invalid_argument);
    EXPECT_THROW(gst.gstOn(rs(100), 101), std::invalid_argument);
}
