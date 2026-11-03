#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <vector>

#include "engine/BestDiscountResolver.hpp"
#include "engine/PriorityResolver.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using engine::Candidate;
using engine::Verdict;

namespace {
/// Rules only need id/priority/stackable here; the discount is given directly.
rules::PercentageDiscount rule(const char* id, int priority, bool stackable) {
    return rules::PercentageDiscount(id, id, priority, stackable, 10);
}

std::vector<bool> picks(const std::vector<Verdict>& v) {
    std::vector<bool> out;
    for (const auto& x : v) out.push_back(x.selected);
    return out;
}

void expectReasons(const std::vector<Verdict>& v) {
    for (const auto& x : v) EXPECT_FALSE(x.reason.empty());
}

const auto A = rule("A", 90, true);  // high priority
const auto B = rule("B", 70, true);
const auto C = rule("C", 50, true);
const auto X = rule("X", 60, false); // not stackable
const auto Y = rule("Y", 80, false); // not stackable
} // namespace

TEST(ConflictResolver, AllAreInterchangeableStrategies) {
    EXPECT_TRUE((std::is_abstract_v<engine::ConflictResolver>));
    EXPECT_TRUE((std::has_virtual_destructor_v<engine::ConflictResolver>));
    std::vector<std::unique_ptr<engine::ConflictResolver>> all;
    all.push_back(std::make_unique<engine::BestDiscountResolver>());
    all.push_back(std::make_unique<engine::PriorityResolver>());
    all.push_back(std::make_unique<engine::StackingResolver>());
    EXPECT_EQ(all[0]->name(), "BestDiscount");
    EXPECT_EQ(all[1]->name(), "Priority");
    EXPECT_EQ(all[2]->name(), "Stacking");
    for (const auto& r : all) EXPECT_TRUE(r->resolve({}).empty());
}

// --- BestDiscountResolver --------------------------------------------------

TEST(BestDiscountResolver, PicksLargestSingleDiscount) {
    const engine::BestDiscountResolver r;
    const auto v = r.resolve({{&A, rs(100)}, {&B, rs(300)}, {&C, rs(200)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{false, true, false}));
    expectReasons(v);
    EXPECT_NE(v[0].reason.find("B saves more"), std::string::npos);
}

TEST(BestDiscountResolver, TieGoesToHigherPriorityThenEarlier) {
    const engine::BestDiscountResolver r;
    EXPECT_EQ(picks(r.resolve({{&B, rs(100)}, {&A, rs(100)}})), (std::vector<bool>{false, true}));
    const auto B2 = rule("B2", 70, true);
    EXPECT_EQ(picks(r.resolve({{&B, rs(100)}, {&B2, rs(100)}})), (std::vector<bool>{true, false}));
}

TEST(BestDiscountResolver, IgnoresStackableFlag) {
    const engine::BestDiscountResolver r;
    EXPECT_EQ(picks(r.resolve({{&A, rs(100)}, {&X, rs(150)}})), (std::vector<bool>{false, true}));
}

// --- PriorityResolver ------------------------------------------------------

TEST(PriorityResolver, PicksHighestPriorityEvenIfSmaller) {
    const engine::PriorityResolver r;
    const auto v = r.resolve({{&B, rs(300)}, {&A, rs(100)}, {&C, rs(500)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{false, true, false}));
    expectReasons(v);
    EXPECT_NE(v[0].reason.find("A has higher priority"), std::string::npos);
}

TEST(PriorityResolver, TieGoesToLargerDiscount) {
    const engine::PriorityResolver r;
    const auto B2 = rule("B2", 70, true);
    EXPECT_EQ(picks(r.resolve({{&B, rs(100)}, {&B2, rs(200)}})), (std::vector<bool>{false, true}));
}

// --- StackingResolver ------------------------------------------------------

TEST(StackingResolver, CombinesAllStackable) {
    const engine::StackingResolver r;
    const auto v = r.resolve({{&A, rs(100)}, {&B, rs(300)}, {&C, rs(200)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{true, true, true}));
    expectReasons(v);
}

TEST(StackingResolver, NonStackableLosesToBiggerStack) {
    const engine::StackingResolver r;
    const auto v = r.resolve({{&A, rs(100)}, {&X, rs(250)}, {&B, rs(200)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{true, false, true}));
    EXPECT_NE(v[1].reason.find("not stackable"), std::string::npos);
}

TEST(StackingResolver, NonStackableWinsAloneWhenItBeatsTheStack) {
    const engine::StackingResolver r;
    const auto v = r.resolve({{&A, rs(100)}, {&X, rs(400)}, {&B, rs(200)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{false, true, false}));
    EXPECT_NE(v[0].reason.find("X saves more"), std::string::npos);
}

TEST(StackingResolver, StackWinsATie) {
    const engine::StackingResolver r;
    EXPECT_EQ(picks(r.resolve({{&A, rs(100)}, {&X, rs(300)}, {&B, rs(200)}})),
              (std::vector<bool>{true, false, true}));
}

TEST(StackingResolver, OnlyBestOfSeveralNonStackable) {
    const engine::StackingResolver r;
    const auto v = r.resolve({{&X, rs(100)}, {&Y, rs(300)}});
    EXPECT_EQ(picks(v), (std::vector<bool>{false, true}));
    expectReasons(v);
}
