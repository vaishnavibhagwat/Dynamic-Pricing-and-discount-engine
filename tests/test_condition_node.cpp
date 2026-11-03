#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "rules/ConditionNode.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;
using domain::MembershipTier;
using rules::AndNode;
using rules::CompareOp;
using rules::ComparisonNode;
using rules::ConditionNode;
using rules::ConditionValue;
using rules::FieldCatalog;
using rules::NotNode;
using rules::OrNode;

namespace {

const FieldCatalog& catalog() {
    static const FieldCatalog c;
    return c;
}

std::unique_ptr<ConditionNode> cmp(const char* field, CompareOp op, ConditionValue v) {
    return std::make_unique<ComparisonNode>(*catalog().find(field), op, std::move(v));
}

std::unique_ptr<ConditionNode> isElectronics() {
    return cmp("category", CompareOp::Equal, ConditionValue::text("Electronics"));
}
std::unique_ptr<ConditionNode> qtyAtLeast(int n) {
    return cmp("qty", CompareOp::GreaterEqual, ConditionValue::number(n));
}
std::unique_ptr<ConditionNode> goldOrAbove() {
    return cmp("tier", CompareOp::GreaterEqual, ConditionValue::tier(MembershipTier::Gold));
}

/// A node with a fixed answer, to test AND/OR/NOT truth tables.
class Fixed final : public ConditionNode {
public:
    explicit Fixed(bool value) : value_(value) {}
    bool evaluate(const CartItem&, const domain::PricingContext&) const override { return value_; }
    bool mayMatch(const domain::Product&) const override { return value_; }
    std::string describe() const override { return value_ ? "T" : "F"; }
    int precedence() const noexcept override { return 4; }

private:
    bool value_;
};

std::unique_ptr<ConditionNode> fixed(bool v) { return std::make_unique<Fixed>(v); }

struct OneLine {
    Order order;
    OneLine() { order.cart += CartItem(phone(), 2); }
    bool eval(const ConditionNode& n) const {
        return n.evaluate(order.cart.items().front(), order.context());
    }
};

} // namespace

TEST(ConditionNode, AndOrNotTruthTables) {
    const OneLine l;
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            EXPECT_EQ(l.eval(AndNode(fixed(a), fixed(b))), a && b);
            EXPECT_EQ(l.eval(OrNode(fixed(a), fixed(b))), a || b);
        }
        EXPECT_EQ(l.eval(NotNode(fixed(a))), !a);
    }
}

TEST(ConditionNode, ComparisonReadsTheCartLine) {
    OneLine l;
    EXPECT_TRUE(l.eval(*isElectronics()));
    EXPECT_TRUE(l.eval(*qtyAtLeast(2)));
    EXPECT_FALSE(l.eval(*qtyAtLeast(3)));
    EXPECT_FALSE(l.eval(*goldOrAbove())); // fixture customer is Regular
    l.order.customer = domain::Customer("CU2", "Ravi", MembershipTier::Platinum);
    EXPECT_TRUE(l.eval(*goldOrAbove()));
}

TEST(ConditionNode, UsersExampleTree) {
    OneLine l;
    l.order.customer = domain::Customer("CU2", "Ravi", MembershipTier::Gold);
    const AndNode tree(std::make_unique<AndNode>(isElectronics(), goldOrAbove()), qtyAtLeast(2));
    EXPECT_TRUE(l.eval(tree));
    EXPECT_EQ(tree.describe(), "category == Electronics AND tier >= GOLD AND qty >= 2");
}

TEST(ConditionNode, DescribeAddsOnlyNeededParentheses) {
    // (a OR b) AND c
    const AndNode orInsideAnd(std::make_unique<OrNode>(isElectronics(), goldOrAbove()),
                              qtyAtLeast(2));
    EXPECT_EQ(orInsideAnd.describe(),
              "(category == Electronics OR tier >= GOLD) AND qty >= 2");
    // a OR b AND c
    const OrNode andInsideOr(isElectronics(), std::make_unique<AndNode>(goldOrAbove(), qtyAtLeast(2)));
    EXPECT_EQ(andInsideOr.describe(), "category == Electronics OR tier >= GOLD AND qty >= 2");
    // NOT on a leaf and on a group
    EXPECT_EQ(NotNode(goldOrAbove()).describe(), "NOT tier >= GOLD");
    EXPECT_EQ(NotNode(std::make_unique<AndNode>(isElectronics(), qtyAtLeast(2))).describe(),
              "NOT (category == Electronics AND qty >= 2)");
}

TEST(ConditionNode, MayMatchJudgesProductFieldsOnly) {
    EXPECT_TRUE(isElectronics()->mayMatch(phone()));
    EXPECT_FALSE(isElectronics()->mayMatch(chair()));
    EXPECT_TRUE(qtyAtLeast(99)->mayMatch(chair())); // depends on the cart
    EXPECT_FALSE(AndNode(isElectronics(), qtyAtLeast(2)).mayMatch(chair()));
    EXPECT_TRUE(OrNode(isElectronics(), goldOrAbove()).mayMatch(chair()));
    EXPECT_TRUE(NotNode(isElectronics()).mayMatch(phone())); // conservative
}

TEST(ConditionNode, ComparisonRejectsTypeMismatches) {
    const auto& tier = *catalog().find("tier");
    const auto& category = *catalog().find("category");
    EXPECT_THROW(ComparisonNode(tier, CompareOp::Equal, ConditionValue::number(5)),
                 std::invalid_argument);
    EXPECT_THROW(ComparisonNode(category, CompareOp::Less, ConditionValue::text("A")),
                 std::invalid_argument);
}

TEST(ConditionNode, CompositesRejectNullChildren) {
    EXPECT_THROW(AndNode(nullptr, fixed(true)), std::invalid_argument);
    EXPECT_THROW(OrNode(fixed(true), nullptr), std::invalid_argument);
    EXPECT_THROW(NotNode(nullptr), std::invalid_argument);
}
