#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "engine/PricingEngine.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/CompositeRule.hpp"
#include "rules/ConditionNode.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;
using domain::MembershipTier;
using rules::CompareOp;
using rules::ComparisonNode;
using rules::CompositeRule;
using rules::ConditionNode;
using rules::ConditionValue;
using rules::FieldCatalog;

namespace {

const FieldCatalog& catalog() {
    static const FieldCatalog c;
    return c;
}

/// category == Electronics AND qty >= 2
std::unique_ptr<ConditionNode> electronicsTwoOrMore() {
    return std::make_unique<rules::AndNode>(
        std::make_unique<ComparisonNode>(*catalog().find("category"), CompareOp::Equal,
                                         ConditionValue::text("Electronics")),
        std::make_unique<ComparisonNode>(*catalog().find("qty"), CompareOp::GreaterEqual,
                                         ConditionValue::number(2)));
}

CompositeRule makeRule(int percent = 12) {
    return CompositeRule("R1", "Test", 3, true, electronicsTwoOrMore(), percent);
}

} // namespace

TEST(CompositeRule, DiscountsOnlyMatchingLines) {
    const auto rule = makeRule();
    Order o;
    o.cart += CartItem(phone(), 2);  // 30000 -> matches
    o.cart += CartItem(oddPricedCharger(), 1);  // qty 1 -> no
    o.cart += CartItem(kurta(), 5);  // not electronics -> no
    EXPECT_TRUE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.calculateDiscount(o.context()), rs(3600));
    EXPECT_EQ(rule.discountForItem(o.cart.items()[1], o.context()), Money::zero());
}

TEST(CompositeRule, RejectionReasonShowsTheCondition) {
    const auto rule = makeRule();
    Order o;
    o.cart += CartItem(phone(), 1);
    EXPECT_FALSE(rule.isApplicable(o.context()));
    EXPECT_EQ(rule.rejectionReason(o.context()),
              "no cart line satisfies: category == Electronics AND qty >= 2");
}

TEST(CompositeRule, DescribesItselfForTheValidator) {
    const auto rule = makeRule();
    EXPECT_EQ(rule.stage(), rules::RuleStage::Percentage);
    EXPECT_EQ(rule.maxPercentOff(phone()), std::optional<int>(12));
    EXPECT_EQ(rule.maxPercentOff(chair()), std::nullopt);
    EXPECT_EQ(rule.scope(), nullptr);
    EXPECT_EQ(rule.percent(), 12);
}

TEST(CompositeRule, DefaultName) {
    EXPECT_EQ(CompositeRule::defaultName(12, *electronicsTwoOrMore()),
              "12% off when category == Electronics AND qty >= 2");
}

TEST(CompositeRule, ValidatesArguments) {
    EXPECT_THROW(makeRule(0), std::invalid_argument);
    EXPECT_THROW(makeRule(101), std::invalid_argument);
    EXPECT_THROW(CompositeRule("R1", "Test", 3, true, nullptr, 10), std::invalid_argument);
}

TEST(CompositeRule, EngineTracesAppliedAndRejectedDecisions) {
    const auto rule = makeRule();
    const engine::StackingResolver stacking;
    const engine::PricingEngine engine(stacking);

    Order hit;
    hit.cart += CartItem(phone(), 2);
    hit.cart += CartItem(kurta(), 1);
    const auto result = engine.price(hit.context(), {&rule});
    bool appliedOnPhone = false;
    bool rejectedOnKurta = false;
    for (const auto& e : result.trace().forRule("R1")) {
        if (e.target == "E1" && e.outcome == engine::TraceOutcome::Applied) {
            appliedOnPhone = e.amount == rs(3600);
        }
        if (e.target == "C1" && e.outcome == engine::TraceOutcome::Rejected) rejectedOnKurta = true;
    }
    EXPECT_TRUE(appliedOnPhone);
    EXPECT_TRUE(rejectedOnKurta);

    Order miss;
    miss.cart += CartItem(phone(), 1);
    const auto missed = engine.price(miss.context(), {&rule});
    const auto entries = missed.trace().forRule("R1");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].outcome, engine::TraceOutcome::Rejected);
    EXPECT_NE(entries[0].reason.find("no cart line satisfies"), std::string::npos);
}
