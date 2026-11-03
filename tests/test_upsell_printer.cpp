#include <gtest/gtest.h>

#include <sstream>

#include "presentation/UpsellPrinter.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::UpsellSuggestion;

namespace {

std::string printed(const std::vector<UpsellSuggestion>& s) {
    std::ostringstream out;
    presentation::UpsellPrinter().print(out, s, "Stacking");
    return out.str();
}

bool has(const std::string& text, const std::string& what) {
    return text.find(what) != std::string::npos;
}

} // namespace

TEST(UpsellPrinter, SaysSoWhenThereIsNothingToSuggest) {
    const std::string out = printed({});
    EXPECT_TRUE(has(out, "Smart suggestions (policy Stacking)"));
    EXPECT_TRUE(has(out, "No single item unlocks extra savings"));
}

TEST(UpsellPrinter, PrintsEachSuggestionWithItsNumbers) {
    UpsellSuggestion s;
    s.product = &kurta();
    s.standaloneCost = rs(1120);
    s.extraCost = rs(560);
    s.bonus = rs(560);
    s.unlockedOffers = {"Flat 500", "Buy 3"};
    const std::string out = printed({s});
    EXPECT_TRUE(has(out, "1. Add 1 x Kurta (C1)")) << out;
    EXPECT_TRUE(has(out, "Bought alone it costs ₹1,120.00"));
    EXPECT_TRUE(has(out, "goes up by only ₹560.00 - you save ₹560.00 extra."));
    EXPECT_TRUE(has(out, "Unlocks: Flat 500, Buy 3"));
}

TEST(UpsellPrinter, OmitsUnlocksLineWhenNoOfferIsNamed) {
    UpsellSuggestion s;
    s.product = &chair();
    s.standaloneCost = rs(100);
    s.extraCost = rs(90);
    s.bonus = rs(10);
    EXPECT_FALSE(has(printed({s}), "Unlocks:"));
}
