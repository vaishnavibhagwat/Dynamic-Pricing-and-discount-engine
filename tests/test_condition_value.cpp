#include <gtest/gtest.h>

#include <stdexcept>

#include "rules/ConditionValue.hpp"

using domain::MembershipTier;
using rules::CompareOp;
using rules::ConditionValue;
using rules::ValueType;

TEST(ConditionValue, NamedConstructorsSetType) {
    EXPECT_EQ(ConditionValue::number(2).type(), ValueType::Number);
    EXPECT_EQ(ConditionValue::money(domain::Money::fromRupees(5)).type(), ValueType::Money);
    EXPECT_EQ(ConditionValue::text("x").type(), ValueType::Text);
    EXPECT_EQ(ConditionValue::tier(MembershipTier::Gold).type(), ValueType::Tier);
    EXPECT_EQ(ConditionValue::date(domain::Date(2026, 1, 1)).type(), ValueType::Date);
}

TEST(ConditionValue, NumberComparisons) {
    const auto two = ConditionValue::number(2);
    const auto three = ConditionValue::number(3);
    EXPECT_TRUE(three.compare(CompareOp::GreaterEqual, two));
    EXPECT_TRUE(two.compare(CompareOp::GreaterEqual, two));
    EXPECT_FALSE(two.compare(CompareOp::Greater, two));
    EXPECT_TRUE(two.compare(CompareOp::Less, three));
    EXPECT_TRUE(two.compare(CompareOp::LessEqual, three));
    EXPECT_TRUE(two.compare(CompareOp::Equal, ConditionValue::number(2)));
    EXPECT_TRUE(two.compare(CompareOp::NotEqual, three));
}

TEST(ConditionValue, TiersAreOrdered) {
    const auto silver = ConditionValue::tier(MembershipTier::Silver);
    const auto gold = ConditionValue::tier(MembershipTier::Gold);
    const auto platinum = ConditionValue::tier(MembershipTier::Platinum);
    EXPECT_TRUE(platinum.compare(CompareOp::GreaterEqual, gold));
    EXPECT_TRUE(gold.compare(CompareOp::GreaterEqual, gold));
    EXPECT_FALSE(silver.compare(CompareOp::GreaterEqual, gold));
}

TEST(ConditionValue, MoneyComparesInPaise) {
    const auto a = ConditionValue::money(domain::Money::fromRupees(4999, 99));
    const auto b = ConditionValue::money(domain::Money::fromRupees(5000));
    EXPECT_TRUE(a.compare(CompareOp::Less, b));
}

TEST(ConditionValue, DatesCompare) {
    const auto start = ConditionValue::date(domain::Date(2026, 10, 25));
    const auto today = ConditionValue::date(domain::Date(2026, 10, 28));
    EXPECT_TRUE(today.compare(CompareOp::GreaterEqual, start));
    EXPECT_FALSE(today.compare(CompareOp::Equal, start));
}

TEST(ConditionValue, TextIgnoresCase) {
    EXPECT_TRUE(ConditionValue::text("Electronics")
                    .compare(CompareOp::Equal, ConditionValue::text("ELECTRONICS")));
    EXPECT_TRUE(ConditionValue::text("Clothing")
                    .compare(CompareOp::NotEqual, ConditionValue::text("Electronics")));
}

TEST(ConditionValue, RejectsMixedTypesAndOrderingOnText) {
    EXPECT_THROW(ConditionValue::number(1).compare(CompareOp::Equal, ConditionValue::text("1")),
                 std::invalid_argument);
    EXPECT_THROW(ConditionValue::text("a").compare(CompareOp::Less, ConditionValue::text("b")),
                 std::invalid_argument);
}

TEST(ConditionValue, ToStringMatchesRuleSyntax) {
    EXPECT_EQ(ConditionValue::number(2).toString(), "2");
    EXPECT_EQ(ConditionValue::money(domain::Money::fromRupees(5000)).toString(), "5000");
    EXPECT_EQ(ConditionValue::money(domain::Money::fromRupees(99, 5)).toString(), "99.05");
    EXPECT_EQ(ConditionValue::text("Electronics").toString(), "Electronics");
    EXPECT_EQ(ConditionValue::text("Home Decor").toString(), "\"Home Decor\"");
    EXPECT_EQ(ConditionValue::tier(MembershipTier::Gold).toString(), "GOLD");
    EXPECT_EQ(ConditionValue::date(domain::Date(2026, 10, 25)).toString(), "2026-10-25");
}

TEST(ConditionValue, OperatorHelpers) {
    EXPECT_EQ(rules::toString(CompareOp::GreaterEqual), ">=");
    EXPECT_TRUE(rules::isOrdering(CompareOp::Less));
    EXPECT_FALSE(rules::isOrdering(CompareOp::NotEqual));
    EXPECT_FALSE(rules::isOrdered(ValueType::Text));
    EXPECT_TRUE(rules::isOrdered(ValueType::Tier));
    EXPECT_EQ(rules::toString(ValueType::Money), "money");
}
