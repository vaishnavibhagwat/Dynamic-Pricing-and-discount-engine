#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "engine/BestDiscountResolver.hpp"
#include "engine/PricingEngine.hpp"
#include "engine/PriorityResolver.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/BankOfferDiscount.hpp"
#include "rules/CartLevelRule.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "rules/ItemLevelRule.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/RuleFactory.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;
using domain::MembershipTier;
using engine::PriceTrace;
using engine::PricingEngine;
using engine::TraceOutcome;
using rules::ItemFilter;
using Benefit = rules::CouponDiscount::Benefit;

namespace {

using RuleList = std::vector<const rules::DiscountRule*>;

const engine::StackingResolver kStacking;
const engine::BestDiscountResolver kBest;
const engine::PriorityResolver kPriority;

std::vector<std::string> appliedSteps(const engine::PricingResult& r, const std::string& target) {
    std::vector<std::string> steps;
    for (const auto& e : r.trace().forTarget(target)) {
        if (e.outcome == TraceOutcome::Applied) steps.push_back(e.step);
    }
    return steps;
}

const engine::TraceEntry* findEntry(const engine::PricingResult& r, const std::string& ruleId,
                                    const std::string& target) {
    for (const auto& e : r.trace().entries()) {
        if (e.ruleId == ruleId && e.target == target) return &e;
    }
    return nullptr;
}

/// A brand-new offer type the engine has never heard of: flat Rs 100 off every line.
class FlatHundredPerLine final : public rules::ItemLevelRule {
public:
    FlatHundredPerLine() : ItemLevelRule("FLAT100", "Rs 100 off every line", 10, true) {}
    rules::RuleStage stage() const override { return rules::RuleStage::Quantity; }
    bool appliesToItem(const CartItem&, const domain::PricingContext&) const override {
        return true;
    }
    Money discountForItem(const CartItem&, const domain::PricingContext&) const override {
        return rs(100);
    }
};

/// A misconfigured rule: claims an item stage but only prices whole orders.
class OrderOnlyRule final : public rules::CartLevelRule {
public:
    OrderOnlyRule() : CartLevelRule("ODD", "Order-only rule", 10, true) {}
    rules::RuleStage stage() const override { return rules::RuleStage::Seasonal; }
    bool isApplicable(const domain::PricingContext&) const override { return true; }
    Money discountOnAmount(Money amount, const domain::PricingContext&) const override {
        return amount.percentOf(1);
    }
};

/// A custom guard injected into the engine: no item discounts at all.
class NoItemDiscountGuard final : public engine::PriceGuard {
public:
    std::string name() const override { return "NoItemDiscount"; }
    engine::GuardOutcome limit(const CartItem&, Money proposed) const override {
        if (proposed.isZero()) return {proposed, {}};
        return {Money::zero(), "item discounts switched off"};
    }
};

} // namespace

// --- Construction ----------------------------------------------------------

TEST(PricingEngine, TakesResolverByReferenceNotTemporary) {
    EXPECT_TRUE((std::is_constructible_v<PricingEngine, const engine::StackingResolver&>));
    EXPECT_FALSE((std::is_constructible_v<PricingEngine, engine::StackingResolver&&>));
    const PricingEngine e(kBest);
    EXPECT_EQ(&e.resolver(), &kBest);
}

TEST(PricingEngine, DefaultGuardsRunInOrder) {
    const PricingEngine e(kStacking);
    EXPECT_EQ(e.guardNames(),
              (std::vector<std::string>{"DiscountCapPolicy", "MarginGuard", "MrpCeilingPolicy"}));
}

TEST(PricingEngine, RejectsNullGuardAndNullRule) {
    std::vector<std::unique_ptr<engine::PriceGuard>> guards;
    guards.push_back(nullptr);
    EXPECT_THROW(PricingEngine(kStacking, std::move(guards)), std::invalid_argument);

    Order o;
    o.cart += CartItem(phone(), 1);
    EXPECT_THROW(PricingEngine(kStacking).price(o.context(), RuleList{nullptr}),
                 std::invalid_argument);
}

// --- The three resolvers on the same cart ----------------------------------

TEST(PricingEngine, ThreeResolversGiveDifferentTotalsForSameCart) {
    const rules::SeasonalDiscount festive("FESTIVE10", "Festive 10% off Furniture", 90, true,
                                          kToday, kToday, 10, ItemFilter::forCategory("Furniture"));
    const rules::PercentageDiscount clearance("FURN25", "25% off Furniture", 70, true, 25,
                                              ItemFilter::forCategory("Furniture"));
    const rules::CouponDiscount coupon("FLAT300", "Flat Rs 300 off", 50, true, "FLAT300", rs(2000),
                                       100, Benefit::flat(rs(300)));
    const rules::BankOfferDiscount hdfc("HDFC15", "HDFC 15% off", 40, true, "HDFC", 15, rs(1500));
    const RuleList rules{&hdfc, &coupon, &clearance, &festive}; // deliberately out of order

    Order o;
    o.cart += CartItem(chair(), 1); // cost 2000, price 4000, MRP 5000, GST 18%
    o.coupon = "FLAT300";
    o.bank = "HDFC";
    const auto ctx = o.context();

    const auto byPriority = PricingEngine(kPriority).price(ctx, rules);
    const auto byBest = PricingEngine(kBest).price(ctx, rules);
    const auto byStacking = PricingEngine(kStacking).price(ctx, rules);

    // Priority: festive 10% (400) beats 25% on priority; coupon (300) beats bank on priority.
    EXPECT_EQ(byPriority.itemDiscountTotal(), rs(400));
    EXPECT_EQ(byPriority.cartDiscountTotal(), rs(300));
    EXPECT_EQ(byPriority.taxableTotal(), rs(3300));
    EXPECT_EQ(byPriority.gstTotal(), rs(594));
    EXPECT_EQ(byPriority.grandTotal(), rs(3894));

    // Best: 25% (1000) is the biggest item offer; on 3000 the bank's 450 beats the coupon's 300.
    EXPECT_EQ(byBest.itemDiscountTotal(), rs(1000));
    EXPECT_EQ(byBest.cartDiscountTotal(), rs(450));
    EXPECT_EQ(byBest.taxableTotal(), rs(2550));
    EXPECT_EQ(byBest.gstTotal(), rs(459));
    EXPECT_EQ(byBest.grandTotal(), rs(3009));

    // Stacking: 10% + 25% (1400); coupon 300, then 15% of the remaining 2300 = 345.
    EXPECT_EQ(byStacking.itemDiscountTotal(), rs(1400));
    EXPECT_EQ(byStacking.cartDiscountTotal(), rs(645));
    EXPECT_EQ(byStacking.taxableTotal(), rs(1955));
    EXPECT_EQ(byStacking.gstTotal(), Money::fromRupees(351, 90));
    EXPECT_EQ(byStacking.grandTotal(), Money::fromRupees(2306, 90));

    const std::set<std::int64_t> totals{byPriority.grandTotal().paise(),
                                        byBest.grandTotal().paise(),
                                        byStacking.grandTotal().paise()};
    EXPECT_EQ(totals.size(), 3u);

    EXPECT_EQ(byPriority.resolverName(), "Priority");
    EXPECT_EQ(byBest.resolverName(), "BestDiscount");
    EXPECT_EQ(byStacking.resolverName(), "Stacking");

    // The losing offers are traced with the resolver's reason.
    const auto* dropped = findEntry(byPriority, "FURN25", "F1");
    ASSERT_NE(dropped, nullptr);
    EXPECT_EQ(dropped->outcome, TraceOutcome::Rejected);
    EXPECT_NE(dropped->reason.find("Priority"), std::string::npos);
}

// --- Pipeline order --------------------------------------------------------

TEST(PricingEngine, ItemOffersApplyInStageOrderThenGuardsInOrder) {
    const rules::MembershipDiscount loyalty("LOYAL", "Loyalty", 10, true,
                                            {{MembershipTier::Gold, 5}});
    const rules::QuantityDiscount bulk("BULK", "Buy 3 get 10%", 99, true, 3, 10);
    const rules::PercentageDiscount pct("PCT5", "5% off", 1, true, 5);
    const rules::SeasonalDiscount sale("SALE", "Sale 10%", 50, true, kToday, kToday, 10);
    const rules::ExpiryMarkdown expiry("EXP", "Near expiry", 5, true, {{3, 50}});
    const RuleList rules{&loyalty, &bulk, &pct, &sale, &expiry};

    const auto milk = makeMilk("G1", 2); // cost 40, price 60, MRP 65, GST 5%
    Order o;
    o.customer = domain::Customer("CU2", "Meera", MembershipTier::Gold);
    o.cart += CartItem(milk, 3); // 180
    const auto r = PricingEngine(kStacking).price(o.context(), rules);

    EXPECT_EQ(appliedSteps(r, "G1"), (std::vector<std::string>{"Expiry", "Seasonal", "Percentage",
                                                               "Quantity", "Membership"}));
    const auto& line = r.lines().at(0);
    ASSERT_EQ(line.itemDiscounts.size(), 5u);
    EXPECT_EQ(line.itemDiscounts[0].amount, rs(90)); // 50% of 180
    // 90 + 18 + 9 + 18 + 9 = 144 offered; cap 60% -> 108; margin floor 129.60 -> 50.40.
    std::vector<std::pair<std::string, Money>> adjustments;
    for (const auto& e : r.trace().forTarget("G1")) {
        if (e.outcome == TraceOutcome::Adjusted) adjustments.emplace_back(e.ruleId, e.amount);
    }
    ASSERT_EQ(adjustments.size(), 2u);
    EXPECT_EQ(adjustments[0], std::make_pair(std::string("DiscountCapPolicy"), -rs(36)));
    EXPECT_EQ(adjustments[1], std::make_pair(std::string("MarginGuard"), -Money::fromRupees(57, 60)));
    EXPECT_EQ(line.itemDiscount, Money::fromRupees(50, 40));
    EXPECT_EQ(line.guardAdjustment, -Money::fromRupees(93, 60));
    EXPECT_EQ(line.net(), Money::fromRupees(129, 60));
}

TEST(PricingEngine, OrderOffersUsePostGuardAmount) {
    const rules::SeasonalDiscount diwali("DIWALI", "Diwali 20%", 90, true, kToday, kToday, 20,
                                         ItemFilter::forCategory("Electronics"));
    const rules::MembershipDiscount loyalty("LOYAL", "Loyalty", 10, true,
                                            {{MembershipTier::Gold, 5}});
    const rules::BankOfferDiscount hdfc("HDFC", "HDFC 10%", 40, true, "HDFC", 10, rs(1500));

    Order o;
    o.customer = domain::Customer("CU2", "Meera", MembershipTier::Gold);
    o.cart += CartItem(phone(), 1); // 15000, cost 12000 -> floor 12960
    o.bank = "HDFC";
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&hdfc, &loyalty, &diwali});

    EXPECT_EQ(r.itemDiscountTotal(), rs(2040)); // 3750 offered, margin keeps 2040
    ASSERT_EQ(r.cartDiscounts().size(), 1u);
    EXPECT_EQ(r.cartDiscounts()[0].amount, rs(1296)); // 10% of 12960, not 1500 of 15000
    EXPECT_EQ(r.taxableTotal(), rs(11664));
}

TEST(PricingEngine, MrpCeilingRaisesDiscountWhenTaxWouldExceedMrp) {
    const domain::Electronics speaker("E7", "Speaker", rs(500), rs(1000), rs(1050));
    Order o;
    o.cart += CartItem(speaker, 1);
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{});
    EXPECT_EQ(r.itemDiscountTotal(), Money::fromRupees(110, 17));
    EXPECT_EQ(r.grandTotal(), rs(1050));
    EXPECT_EQ(r.savingsVsMrp(), Money::zero());
}

// --- Splitting order offers and GST ----------------------------------------

TEST(PricingEngine, OrderOfferSplitExactlyAndGstPerLine) {
    const rules::CouponDiscount coupon("FLAT500", "Flat 500", 50, true, "FLAT500", Money::zero(),
                                       10, Benefit::flat(rs(500)));
    Order o;
    o.cart += CartItem(phone(), 1);            // 15000, 18%
    o.cart += CartItem(kurta(), 1);            // 1000, 12%
    o.cart += CartItem(oddPricedCharger(), 1); // 999.99, 18%
    o.coupon = "FLAT500";
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&coupon});

    ASSERT_EQ(r.lines().size(), 3u);
    Money shares;
    for (const auto& l : r.lines()) {
        shares += l.cartDiscountShare;
        EXPECT_EQ(l.taxable, l.gross - l.itemDiscount - l.cartDiscountShare);
        EXPECT_EQ(l.gst, l.taxable.percentOf(l.gstRate)) << l.productId;
        EXPECT_EQ(l.total, l.taxable + l.gst);
    }
    EXPECT_EQ(shares, rs(500));
    EXPECT_EQ(r.lines()[0].cartDiscountShare, Money::fromRupees(441, 18));
    EXPECT_EQ(r.lines()[1].cartDiscountShare, Money::fromRupees(29, 41));
    EXPECT_EQ(r.lines()[2].cartDiscountShare, Money::fromRupees(29, 41));
    EXPECT_EQ(r.lines()[1].gstRate, 12);
    EXPECT_EQ(r.grandTotal(), r.taxableTotal() + r.gstTotal());
    EXPECT_EQ(r.subtotal(), Money::fromRupees(16999, 99));
}

TEST(PricingEngine, EmptyCartCostsNothing) {
    const rules::MembershipDiscount loyalty("LOYAL", "Loyalty", 10, true,
                                            {{MembershipTier::Gold, 5}});
    Order o;
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&loyalty});
    EXPECT_TRUE(r.lines().empty());
    EXPECT_EQ(r.grandTotal(), Money::zero());
    ASSERT_EQ(r.trace().size(), 1u);
    EXPECT_EQ(r.trace().entries()[0].outcome, TraceOutcome::Rejected);
}

// --- Tracing rejected rules ------------------------------------------------

TEST(PricingEngine, EveryRejectedRuleIsTracedWithItsReason) {
    const rules::SeasonalDiscount eoss("EOSS", "EOSS", 85, true, domain::Date(2027, 1, 15),
                                       domain::Date(2027, 1, 31), 30);
    const rules::PercentageDiscount furniture("FURN10", "10% Furniture", 70, true, 10,
                                              ItemFilter::forCategory("Furniture"));
    const rules::PercentageDiscount electronics("ELEC5", "5% Electronics", 70, true, 5,
                                                ItemFilter::forCategory("Electronics"));
    const rules::CouponDiscount coupon("SAVE_CPN", "Coupon", 50, true, "SAVE", Money::zero(), 5,
                                       Benefit::flat(rs(50)));
    const rules::BankOfferDiscount sbi("SBI", "SBI 5%", 40, true, "SBI", 5, rs(1000));

    Order o;
    o.cart += CartItem(phone(), 1);
    o.cart += CartItem(kurta(), 1);
    o.coupon = "WRONG";
    const auto r = PricingEngine(kStacking).price(
        o.context(), RuleList{&eoss, &furniture, &electronics, &coupon, &sbi});

    const auto* e = findEntry(r, "EOSS", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_NE(e->reason.find("outside"), std::string::npos);

    e = findEntry(r, "FURN10", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->reason, "no item from Furniture in cart");

    e = findEntry(r, "ELEC5", "C1"); // the kurta line
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->outcome, TraceOutcome::Rejected);
    EXPECT_EQ(e->reason, "does not cover Kurta");
    EXPECT_EQ(findEntry(r, "ELEC5", "E1")->outcome, TraceOutcome::Applied);

    e = findEntry(r, "SAVE_CPN", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_NE(e->reason.find("does not match"), std::string::npos);

    e = findEntry(r, "SBI", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->reason, "no bank card used for payment");
}

// --- Open/Closed and dependency injection ---------------------------------

TEST(PricingEngine, NewOfferTypeWorksWithoutChangingEngine) {
    const FlatHundredPerLine flat;
    Order o;
    o.cart += CartItem(phone(), 1);
    o.cart += CartItem(kurta(), 1);
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&flat});
    EXPECT_EQ(r.itemDiscountTotal(), rs(200));
    EXPECT_EQ(appliedSteps(r, "E1"), (std::vector<std::string>{"Quantity"}));
}

TEST(PricingEngine, ItemStageRuleWithoutPerItemPricingIsRejected) {
    const OrderOnlyRule odd;
    Order o;
    o.cart += CartItem(phone(), 1);
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&odd});
    EXPECT_EQ(r.itemDiscountTotal(), Money::zero());
    const auto* e = findEntry(r, "ODD", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->outcome, TraceOutcome::Rejected);
}

TEST(PricingEngine, InjectedGuardsReplaceDefaults) {
    const rules::PercentageDiscount pct("PCT10", "10% off", 1, true, 10);
    std::vector<std::unique_ptr<engine::PriceGuard>> guards;
    guards.push_back(std::make_unique<NoItemDiscountGuard>());
    const PricingEngine e(kStacking, std::move(guards));
    EXPECT_EQ(e.guardNames(), (std::vector<std::string>{"NoItemDiscount"}));

    Order o;
    o.cart += CartItem(kurta(), 1);
    const auto r = e.price(o.context(), RuleList{&pct});
    EXPECT_EQ(r.itemDiscountTotal(), Money::zero());
    EXPECT_EQ(r.trace().count(TraceOutcome::Adjusted), 1u);
}

// --- Trace observers and guard checks --------------------------------------

namespace {

/// Test spy: remembers every decision it is told about.
class SpyObserver final : public engine::TraceObserver {
public:
    void onDecision(const engine::TraceEntry& e) override { seen.push_back(e); }
    std::vector<engine::TraceEntry> seen;
};

} // namespace

TEST(PricingEngine, SubscribedObserverHearsEveryDecisionInOrder) {
    const rules::SeasonalDiscount diwali("DIWALI", "Diwali 20%", 90, true, kToday, kToday, 20,
                                         ItemFilter::forCategory("Electronics"));
    const rules::BankOfferDiscount sbi("SBI", "SBI 5%", 40, true, "SBI", 5, rs(1000));
    Order o;
    o.cart += CartItem(phone(), 1);
    o.cart += CartItem(kurta(), 1);

    PricingEngine engine(kStacking);
    SpyObserver spy;
    engine.subscribe(spy);
    const auto r = engine.price(o.context(), RuleList{&diwali, &sbi});

    ASSERT_EQ(spy.seen.size(), r.trace().size());
    ASSERT_FALSE(spy.seen.empty());
    for (std::size_t i = 0; i < spy.seen.size(); ++i) {
        EXPECT_EQ(spy.seen[i].summary(), r.trace().entries()[i].summary()) << i;
        EXPECT_EQ(spy.seen[i].target, r.trace().entries()[i].target) << i;
    }

    engine.unsubscribe(spy);
    spy.seen.clear();
    engine.price(o.context(), RuleList{&diwali, &sbi});
    EXPECT_TRUE(spy.seen.empty());
}

TEST(PricingEngine, EveryGuardRecordsItsCheckOnEveryLine) {
    const rules::PercentageDiscount pct("PCT10", "10% off", 1, true, 10);
    Order o;
    o.cart += CartItem(phone(), 1);
    o.cart += CartItem(kurta(), 2);
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&pct});

    for (const std::string target : {"E1", "C1"}) {
        std::vector<std::string> guards;
        for (const auto& e : r.trace().forTarget(target)) {
            if (e.step != "Guard") continue;
            EXPECT_EQ(e.outcome, TraceOutcome::Passed) << e.summary();
            EXPECT_FALSE(e.reason.empty());
            guards.push_back(e.ruleId);
        }
        EXPECT_EQ(guards, PricingEngine(kStacking).guardNames()) << target;
    }
    const auto* cap = findEntry(r, "DiscountCapPolicy", "E1");
    ASSERT_NE(cap, nullptr);
    EXPECT_EQ(cap->summary(), "Guard DiscountCapPolicy passed: discount " + rs(1500).toString() +
                                  " is within the 60% cap of " + rs(9000).toString());
}

TEST(PricingEngine, GuardWithoutReasonStillGetsOne) {
    std::vector<std::unique_ptr<engine::PriceGuard>> guards;
    guards.push_back(std::make_unique<NoItemDiscountGuard>()); // passes with an empty reason
    const PricingEngine e(kStacking, std::move(guards));
    Order o;
    o.cart += CartItem(kurta(), 1);
    const auto r = e.price(o.context(), RuleList{});
    const auto* entry = findEntry(r, "NoItemDiscount", "C1");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->outcome, TraceOutcome::Passed);
    EXPECT_EQ(entry->reason, "check passed");
}

TEST(PricingEngine, CouponBelowMinimumIsExplainedInOneLine) {
    const rules::CouponDiscount save10("SAVE10", "Save 10%", 50, true, "SAVE10", rs(75000), 5,
                                       Benefit::percent(10));
    Order o;
    o.cart += CartItem(phone(), 1); // 15000
    o.coupon = "SAVE10";
    const auto r = PricingEngine(kStacking).price(o.context(), RuleList{&save10});

    const auto* e = findEntry(r, "SAVE10", PriceTrace::kOrder);
    ASSERT_NE(e, nullptr);
    EXPECT_EQ(e->summary(), "Coupon SAVE10 rejected: cart value " + rs(15000).toString() +
                                " is below minimum " + rs(75000).toString());
}

// --- End to end with the shipped rules file --------------------------------

TEST(PricingEngine, PricesOrderWithRulesFromJson) {
    const auto repo = rules::RuleFactory().loadFile(std::string(DATA_DIR) + "/rules.json");
    const auto milk = makeMilk("G1", 2);

    Order o;
    o.customer = domain::Customer("CU7", "Priya", MembershipTier::Gold);
    o.cart += CartItem(phone(), 1);
    o.cart += CartItem(kurta(), 3);
    o.cart += CartItem(chair(), 1);
    o.cart += CartItem(milk, 2);
    o.coupon = "FESTIVE500";
    o.bank = "HDFC";
    const auto ctx = o.context();

    const auto fromRepo = PricingEngine(kStacking).price(ctx, repo);
    const auto fromList = PricingEngine(kStacking).price(ctx, repo.all());
    EXPECT_EQ(fromRepo.grandTotal(), fromList.grandTotal());

    // Every rule in the file was considered and its decision recorded.
    for (const auto* rule : repo.all()) {
        EXPECT_FALSE(fromRepo.trace().forRule(rule->id()).empty()) << rule->id();
    }
    ASSERT_EQ(fromRepo.cartDiscounts().size(), 2u);
    EXPECT_EQ(fromRepo.cartDiscounts()[0].ruleId, "COUPON_FESTIVE500");
    EXPECT_EQ(fromRepo.cartDiscounts()[0].amount, rs(500));
    EXPECT_EQ(fromRepo.cartDiscounts()[1].ruleId, "BANK_HDFC");
    EXPECT_EQ(fromRepo.grandTotal(), fromRepo.taxableTotal() + fromRepo.gstTotal());
    EXPECT_LT(fromRepo.grandTotal(), fromRepo.mrpTotal());

    // The three policies disagree on this cart too.
    const auto best = PricingEngine(kBest).price(ctx, repo).grandTotal();
    const auto prio = PricingEngine(kPriority).price(ctx, repo).grandTotal();
    EXPECT_LT(fromRepo.grandTotal(), best);
    EXPECT_NE(best, prio);
}
