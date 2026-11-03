#include <gtest/gtest.h>

#include "rules/ItemFilter.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::ItemFilter;

TEST(ItemFilter, EmptyFilterMatchesEverything) {
    const ItemFilter f = ItemFilter::all();
    EXPECT_TRUE(f.matches(phone()));
    EXPECT_TRUE(f.matches(kurta()));
    EXPECT_EQ(f.describe(), "all products");
}

TEST(ItemFilter, CategoryFilter) {
    const ItemFilter f = ItemFilter::forCategory("Electronics");
    EXPECT_TRUE(f.matches(phone()));
    EXPECT_FALSE(f.matches(kurta()));
    EXPECT_EQ(f.describe(), "Electronics");
}

TEST(ItemFilter, CategoryIsCaseSensitive) {
    EXPECT_FALSE(ItemFilter::forCategory("electronics").matches(phone()));
}

TEST(ItemFilter, ProductIdFilter) {
    const ItemFilter f(std::nullopt, {"E2", "C1"});
    EXPECT_FALSE(f.matches(phone()));
    EXPECT_TRUE(f.matches(oddPricedCharger()));
    EXPECT_TRUE(f.matches(kurta()));
}

TEST(ItemFilter, CategoryAndIdsMustBothMatch) {
    const ItemFilter f(std::string("Electronics"), {"C1", "E2"});
    EXPECT_TRUE(f.matches(oddPricedCharger()));
    EXPECT_FALSE(f.matches(kurta()));  // id ok, category wrong
    EXPECT_FALSE(f.matches(phone()));  // category ok, id wrong
    EXPECT_EQ(f.describe(), "Electronics products [C1, E2]");
}
