#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "application/DiscountOptimizer.hpp"
#include "engine/TraceEntry.hpp"
#include "rules/ItemLevelRule.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::DiscountOptimizer;
using application::OfferCombination;
using engine::TraceOutcome;
using rules::PercentageDiscount;

namespace {

using RuleList = std::vector<const rules::DiscountRule*>;
using Ids = std::vector<std::string>;

application::GuardFactory noGuards() {
    return [] { return std::vector<std::unique_ptr<engine::PriceGuard>>{}; };
}

/// One kurta: price ₹1,000 + 12% GST = ₹1,120; cost ₹400, so MarginGuard
/// (cost + 8%) allows at most ₹568 off.
Order oneKurta() {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    return o;
}

/// Applies to every line but takes nothing off.
class ZeroOffRule final : public rules::ItemLevelRule {
public:
    ZeroOffRule() : ItemLevelRule("ZERO", "0% off", 1, true) {}
    rules::RuleStage stage() const override { return rules::RuleStage::Percentage; }
    bool appliesToItem(const domain::CartItem&, const domain::PricingContext&) const override {
        return true;
    }
    Money discountForItem(const domain::CartItem&, const domain::PricingContext&) const override {
        return Money::zero();
    }
};

/// Screening decision recorded for @p ruleId.
const engine::TraceEntry* screened(const application::OptimizationResult& r,
                                   const std::string& ruleId) {
    for (const auto& e : r.screening().entries()) {
        if (e.ruleId == ruleId) return &e;
    }
    return nullptr;
}

} // namespace

TEST(DiscountOptimizer, PicksCombinationWithBiggestSaving) {
    const PercentageDiscount p10("P10", "10% off", 1, true, 10);
    const PercentageDiscount p20("P20", "20% off", 1, true, 20);
    const Order o = oneKurta();

    const auto r = DiscountOptimizer(noGuards()).optimize(o.context(), RuleList{&p10, &p20});
    EXPECT_EQ(r.baselineTotal(), rs(1120));
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"P10", "P20"}));
    EXPECT_EQ(r.bestCombination().ruleNames, (Ids{"10% off", "20% off"}));
    EXPECT_EQ(r.bestSaving(), rs(336)); // ₹300 off + the GST on it
    EXPECT_EQ(r.best().grandTotal(), rs(784));
    EXPECT_EQ(r.combinationsTried(), 3u); // {P10}, {P20}, {P10, P20}
}

TEST(DiscountOptimizer, CombinationCutByGuardLosesToOneThatPasses) {
    // 40% + 30% = ₹700 off: MarginGuard cuts it. Best passing set: 30% + 20% = ₹500.
    const PercentageDiscount p20("P20", "20% off", 1, true, 20);
    const PercentageDiscount p30("P30", "30% off", 1, true, 30);
    const PercentageDiscount p40("P40", "40% off", 1, true, 40);
    const Order o = oneKurta();

    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{&p20, &p30, &p40});
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"P20", "P30"}));
    EXPECT_TRUE(r.bestCombination().passedGuards);
    EXPECT_EQ(r.bestSaving(), rs(560));
    EXPECT_EQ(r.combinationsTried(), 7u);
    EXPECT_EQ(r.failedGuards(), 3u); // {P20,P40}, {P30,P40}, {P20,P30,P40}

    // Ranked: passing sets first, best first; cut sets name the guards that cut them.
    EXPECT_EQ(r.ranked().front().ruleIds, (Ids{"P20", "P30"}));
    for (std::size_t i = 0; i < 4; ++i) EXPECT_TRUE(r.ranked()[i].passedGuards);
    for (const auto& c : r.ranked()) {
        if (c.ruleIds == Ids{"P20", "P40"}) {
            EXPECT_EQ(c.guardsTripped, (Ids{"MarginGuard"})); // ₹600 off: within cap, below floor
        }
        if (c.ruleIds.size() == 3) {
            EXPECT_EQ(c.guardsTripped, (Ids{"DiscountCapPolicy", "MarginGuard"})); // ₹900 off
        }
    }
}

TEST(DiscountOptimizer, NonStackableOfferIsTriedAloneOnly) {
    const PercentageDiscount s10("S10", "10% off", 1, true, 10);
    const PercentageDiscount n15("N15", "15% off, exclusive", 1, false, 15);
    const Order o = oneKurta();

    const auto r = DiscountOptimizer(noGuards()).optimize(o.context(), RuleList{&s10, &n15});
    EXPECT_EQ(r.combinationsTried(), 2u);
    for (const auto& c : r.ranked()) EXPECT_EQ(c.ruleIds.size(), 1u);
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"N15"}));
    EXPECT_NE(screened(r, "N15")->reason.find("not stackable"), std::string::npos);
}

TEST(DiscountOptimizer, TieGoesToFewerOffers) {
    const PercentageDiscount a("A", "10% off", 1, true, 10);
    const PercentageDiscount b("B", "10% off", 1, true, 10);
    const PercentageDiscount c("C", "20% off, exclusive", 1, false, 20);
    const Order o = oneKurta();

    const auto r = DiscountOptimizer(noGuards()).optimize(o.context(), RuleList{&a, &b, &c});
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"C"}));
    EXPECT_EQ(r.ranked()[1].ruleIds, (Ids{"A", "B"})); // same saving, more offers
}

TEST(DiscountOptimizer, TieBetweenSameSizeSetsGoesToSmallerIds) {
    const PercentageDiscount y("Y", "20% off", 1, false, 20);
    const PercentageDiscount x("X", "20% off", 1, false, 20);
    const Order o = oneKurta();
    const auto r = DiscountOptimizer(noGuards()).optimize(o.context(), RuleList{&y, &x});
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"X"}));
}

TEST(DiscountOptimizer, ScreensOutOffersThatCannotHelp) {
    const PercentageDiscount p10("P10", "10% off", 1, true, 10);
    const ZeroOffRule zero;
    const rules::SeasonalDiscount past("PAST", "Old sale", 1, true, domain::Date(2025, 1, 1),
                                       domain::Date(2025, 1, 31), 10);
    const PercentageDiscount huge("HUGE", "70% off", 1, true, 70);
    const Order o = oneKurta();

    const auto r = DiscountOptimizer().optimize(o.context(),
                                                RuleList{&p10, &zero, &past, &huge});
    EXPECT_EQ(r.combinationsTried(), 1u); // only P10 survives
    EXPECT_EQ(r.bestCombination().ruleIds, (Ids{"P10"}));

    EXPECT_EQ(screened(r, "P10")->outcome, TraceOutcome::Passed);
    EXPECT_EQ(screened(r, "ZERO")->reason, "saves nothing on this cart");
    EXPECT_EQ(screened(r, "PAST")->reason.rfind("not applicable: ", 0), 0u);
    // 70% breaks the 60% cap and the margin floor even alone.
    EXPECT_EQ(screened(r, "HUGE")->outcome, TraceOutcome::Rejected);
    EXPECT_NE(screened(r, "HUGE")->reason.find("even on its own"), std::string::npos);
    EXPECT_EQ(r.screening().size(), 4u); // one decision per input rule
}

TEST(DiscountOptimizer, NoUsefulOffersMeansBaseline) {
    const Order o = oneKurta();
    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{});
    EXPECT_TRUE(r.bestCombination().ruleIds.empty());
    EXPECT_EQ(r.bestSaving(), Money::zero());
    EXPECT_EQ(r.best().grandTotal(), r.baselineTotal());
    EXPECT_EQ(r.combinationsTried(), 0u);
}

TEST(DiscountOptimizer, BestBillCarriesFullTrace) {
    const PercentageDiscount p10("P10", "10% off", 1, true, 10);
    const Order o = oneKurta();
    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{&p10});
    const auto entries = r.best().trace().forRule("P10");
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries.front().outcome, TraceOutcome::Applied);
    EXPECT_EQ(r.best().resolverName(), "Stacking");
}

TEST(DiscountOptimizer, CapsNumberOfStackableOffers) {
    const PercentageDiscount a("A", "1%", 1, true, 1);
    const PercentageDiscount b("B", "2%", 1, true, 2);
    const PercentageDiscount c("C", "3%", 1, true, 3);
    const Order o = oneKurta();
    const DiscountOptimizer small(noGuards(), 2);
    EXPECT_EQ(small.maxRules(), 2u);
    EXPECT_THROW(small.optimize(o.context(), RuleList{&a, &b, &c}), std::length_error);
    EXPECT_NO_THROW(small.optimize(o.context(), RuleList{&a, &b}));
}

TEST(DiscountOptimizer, RejectsBadArguments) {
    EXPECT_THROW(DiscountOptimizer{application::GuardFactory()}, std::invalid_argument);
    EXPECT_THROW(DiscountOptimizer(noGuards(), 0), std::invalid_argument);
    EXPECT_THROW(DiscountOptimizer(noGuards(), 21), std::invalid_argument);
    const Order o = oneKurta();
    EXPECT_THROW(DiscountOptimizer().optimize(o.context(), RuleList{nullptr}),
                 std::invalid_argument);
}

TEST(DiscountOptimizer, WorksOnRepository) {
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<PercentageDiscount>("P10", "10% off", 1, true, 10));
    const Order o = oneKurta();
    EXPECT_EQ(DiscountOptimizer().optimize(o.context(), repo).bestCombination().ruleIds,
              (Ids{"P10"}));
}
