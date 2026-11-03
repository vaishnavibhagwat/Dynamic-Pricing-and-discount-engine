#include <gtest/gtest.h>

#include "engine/PriceTrace.hpp"

using domain::Money;
using engine::PriceTrace;
using engine::TraceEntry;
using engine::TraceOutcome;

TEST(PriceTrace, StartsEmpty) {
    const PriceTrace t;
    EXPECT_TRUE(t.empty());
    EXPECT_EQ(t.size(), 0u);
}

TEST(PriceTrace, RecordsDecisionsInOrder) {
    PriceTrace t;
    t.record(TraceEntry::applied("Seasonal", "DIWALI", "Diwali sale", "E1", Money::fromRupees(300),
                                 "stacked"));
    t.record(TraceEntry::rejected("Coupon", "NEW15", "New user", PriceTrace::kOrder,
                                  "no coupon code entered"));
    t.record(TraceEntry::adjusted("Guard", "MarginGuard", "MarginGuard", "E1",
                                  -Money::fromRupees(50), "floor"));

    ASSERT_EQ(t.size(), 3u);
    const auto& e = t.entries();
    EXPECT_EQ(e[0].outcome, TraceOutcome::Applied);
    EXPECT_EQ(e[0].amount, Money::fromRupees(300));
    EXPECT_EQ(e[0].reason, "stacked");
    EXPECT_EQ(e[1].outcome, TraceOutcome::Rejected);
    EXPECT_EQ(e[1].amount, Money::zero());
    EXPECT_EQ(e[1].target, "order");
    EXPECT_EQ(e[2].outcome, TraceOutcome::Adjusted);
    EXPECT_EQ(e[2].amount, -Money::fromRupees(50));
}

TEST(PriceTrace, OnDecisionRecordsLikeRecord) {
    PriceTrace t;
    engine::TraceObserver& asObserver = t;
    asObserver.onDecision(TraceEntry::passed("Guard", "MarginGuard", "MarginGuard", "E1", "ok"));
    ASSERT_EQ(t.size(), 1u);
    EXPECT_EQ(t.entries()[0].outcome, TraceOutcome::Passed);
}

TEST(PriceTrace, FiltersByTargetRuleAndOutcome) {
    PriceTrace t;
    t.record(TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E1", Money::fromRupees(1)));
    t.record(TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E2", Money::fromRupees(2)));
    t.record(TraceEntry::rejected("Percentage", "P10", "10%", "E1", "does not cover Phone"));
    t.record(TraceEntry::passed("Guard", "MarginGuard", "MarginGuard", "E1", "ok"));

    EXPECT_EQ(t.forTarget("E1").size(), 3u);
    EXPECT_EQ(t.forTarget("E2").size(), 1u);
    EXPECT_TRUE(t.forTarget("nope").empty());
    EXPECT_EQ(t.forRule("DIWALI").size(), 2u);
    EXPECT_EQ(t.count(TraceOutcome::Applied), 2u);
    EXPECT_EQ(t.count(TraceOutcome::Rejected), 1u);
    EXPECT_EQ(t.count(TraceOutcome::Passed), 1u);
    EXPECT_EQ(t.count(TraceOutcome::Adjusted), 0u);
}
