#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <sstream>

#include "domain/Money.hpp"

using domain::Money;

namespace {
const std::string R = "\xE2\x82\xB9"; // ₹
}

TEST(Money, DefaultIsZero) {
    EXPECT_EQ(Money().paise(), 0);
    EXPECT_TRUE(Money::zero().isZero());
}

TEST(Money, FactoriesStorePaise) {
    EXPECT_EQ(Money::fromPaise(1234).paise(), 1234);
    EXPECT_EQ(Money::fromRupees(499, 50).paise(), 49950);
    EXPECT_EQ(Money::fromRupees(10).paise(), 1000);
}

TEST(Money, AdditionAndSubtraction) {
    const Money a = Money::fromRupees(100);
    const Money b = Money::fromRupees(30, 25);
    EXPECT_EQ(a + b, Money::fromPaise(13025));
    EXPECT_EQ(a - b, Money::fromPaise(6975));
    EXPECT_EQ(b - a, Money::fromPaise(-6975));
    EXPECT_TRUE((b - a).isNegative());
    EXPECT_EQ(-a, Money::fromRupees(-100));

    Money c = a;
    c += b;
    EXPECT_EQ(c, a + b);
    c -= b;
    EXPECT_EQ(c, a);
}

TEST(Money, MultiplicationByQuantity) {
    const Money price = Money::fromRupees(19, 99);
    EXPECT_EQ(price * 3, Money::fromPaise(5997));
    EXPECT_EQ(3 * price, price * 3);
    Money m = price;
    m *= 2;
    EXPECT_EQ(m, Money::fromPaise(3998));
}

TEST(Money, Comparisons) {
    const Money small = Money::fromRupees(1);
    const Money big = Money::fromRupees(2);
    EXPECT_TRUE(small < big);
    EXPECT_TRUE(big > small);
    EXPECT_TRUE(small <= small);
    EXPECT_TRUE(big >= small);
    EXPECT_TRUE(small == Money::fromPaise(100));
    EXPECT_TRUE(small != big);
    EXPECT_FALSE(big < small);
}

TEST(Money, PercentOfRoundsHalfAwayFromZero) {
    EXPECT_EQ(Money::fromRupees(1000).percentOf(18), Money::fromRupees(180));
    EXPECT_EQ(Money::fromPaise(250).percentOf(5), Money::fromPaise(13));   // 12.5 -> 13
    EXPECT_EQ(Money::fromPaise(249).percentOf(5), Money::fromPaise(12));   // 12.45 -> 12
    EXPECT_EQ(Money::fromPaise(-250).percentOf(5), Money::fromPaise(-13)); // -12.5 -> -13
    EXPECT_EQ(Money::fromRupees(50).percentOf(0), Money::zero());
}

TEST(Money, FormatsWithIndianGrouping) {
    EXPECT_EQ(Money::zero().toString(), R + "0.00");
    EXPECT_EQ(Money::fromPaise(5).toString(), R + "0.05");
    EXPECT_EQ(Money::fromPaise(99999).toString(), R + "999.99");
    EXPECT_EQ(Money::fromRupees(1000).toString(), R + "1,000.00");
    EXPECT_EQ(Money::fromRupees(99999, 50).toString(), R + "99,999.50");
    EXPECT_EQ(Money::fromPaise(12345678).toString(), R + "1,23,456.78");
    EXPECT_EQ(Money::fromPaise(1234567890).toString(), R + "1,23,45,678.90");
    EXPECT_EQ(Money::fromRupees(10000000).toString(), R + "1,00,00,000.00");
}

TEST(Money, FormatsNegativeAmounts) {
    EXPECT_EQ(Money::fromPaise(-123450).toString(), "-" + R + "1,234.50");
    EXPECT_EQ(Money::fromPaise(-7).toString(), "-" + R + "0.07");
}

TEST(Money, FormatsExtremeValuesWithoutOverflow) {
    const Money min = Money::fromPaise(std::numeric_limits<std::int64_t>::min());
    EXPECT_EQ(min.toString(), "-" + R + "92,23,37,20,36,85,47,758.08");
}

TEST(Money, StreamOperatorUsesToString) {
    std::ostringstream out;
    out << Money::fromPaise(12345678);
    EXPECT_EQ(out.str(), R + "1,23,456.78");
}
