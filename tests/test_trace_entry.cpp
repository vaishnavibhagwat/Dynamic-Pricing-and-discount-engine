#include <gtest/gtest.h>

#include "engine/TraceEntry.hpp"

using domain::Money;
using engine::TraceEntry;
using engine::TraceOutcome;

TEST(TraceEntry, NamedConstructorsSetOutcomeAndAmount) {
    const auto a = TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E1", Money::fromRupees(300),
                                       "stacked");
    EXPECT_EQ(a.outcome, TraceOutcome::Applied);
    EXPECT_EQ(a.amount, Money::fromRupees(300));
    EXPECT_EQ(a.step, "Seasonal");
    EXPECT_EQ(a.ruleId, "DIWALI");
    EXPECT_EQ(a.ruleName, "Diwali");
    EXPECT_EQ(a.target, "E1");
    EXPECT_EQ(a.reason, "stacked");

    const auto r = TraceEntry::rejected("Coupon", "SAVE10", "Save 10", "order", "no code");
    EXPECT_EQ(r.outcome, TraceOutcome::Rejected);
    EXPECT_EQ(r.amount, Money::zero());

    const auto adj = TraceEntry::adjusted("Guard", "MarginGuard", "MarginGuard", "E1",
                                          -Money::fromRupees(50), "floor");
    EXPECT_EQ(adj.outcome, TraceOutcome::Adjusted);
    EXPECT_EQ(adj.amount, -Money::fromRupees(50));

    const auto p = TraceEntry::passed("Guard", "MarginGuard", "MarginGuard", "E1", "ok");
    EXPECT_EQ(p.outcome, TraceOutcome::Passed);
    EXPECT_EQ(p.amount, Money::zero());
}

TEST(TraceEntry, SummaryForRejectedRule) {
    const auto e = TraceEntry::rejected("Coupon", "SAVE10", "Save 10%", "order",
                                        "cart value ₹50,000.00 is below minimum ₹75,000.00");
    EXPECT_EQ(e.summary(),
              "Coupon SAVE10 rejected: cart value ₹50,000.00 is below minimum ₹75,000.00");
}

TEST(TraceEntry, SummaryForAppliedRuleShowsAmount) {
    const auto e = TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E1",
                                       Money::fromRupees(3000), "stackable");
    EXPECT_EQ(e.summary(), "Seasonal DIWALI applied " + Money::fromRupees(3000).toString() +
                               " off: stackable");
}

TEST(TraceEntry, SummaryOmitsColonWhenReasonIsEmpty) {
    const auto e = TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E1", Money::fromRupees(1));
    EXPECT_EQ(e.summary(), "Seasonal DIWALI applied " + Money::fromRupees(1).toString() + " off");
}

TEST(TraceEntry, SummaryForGuards) {
    const auto passed = TraceEntry::passed("Guard", "DiscountCapPolicy", "DiscountCapPolicy", "E1",
                                           "within cap");
    EXPECT_EQ(passed.summary(), "Guard DiscountCapPolicy passed: within cap");

    const auto cut = TraceEntry::adjusted("Guard", "MarginGuard", "MarginGuard", "E1",
                                          -Money::fromRupees(40), "floor");
    EXPECT_EQ(cut.summary(),
              "Guard MarginGuard raised price by " + Money::fromRupees(40).toString() + ": floor");

    const auto raise = TraceEntry::adjusted("Guard", "MrpCeilingPolicy", "MrpCeilingPolicy", "E1",
                                            Money::fromRupees(10), "MRP");
    EXPECT_EQ(raise.summary(), "Guard MrpCeilingPolicy lowered price by " +
                                   Money::fromRupees(10).toString() + ": MRP");
}

TEST(TraceEntry, OutcomeToString) {
    EXPECT_EQ(engine::toString(TraceOutcome::Applied), "Applied");
    EXPECT_EQ(engine::toString(TraceOutcome::Rejected), "Rejected");
    EXPECT_EQ(engine::toString(TraceOutcome::Adjusted), "Adjusted");
    EXPECT_EQ(engine::toString(TraceOutcome::Passed), "Passed");
}
