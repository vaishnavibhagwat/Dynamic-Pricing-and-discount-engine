#include "rules/ConditionNode.hpp"

#include <stdexcept>
#include <utility>

namespace rules {

// ------------------------------------------------------------ ConditionNode

std::string ConditionNode::describeChild(const ConditionNode& child, int parentPrecedence) {
    if (child.precedence() < parentPrecedence) return "(" + child.describe() + ")";
    return child.describe();
}

// --------------------------------------------------------------- BinaryNode

BinaryNode::BinaryNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right)
    : left_(std::move(left)), right_(std::move(right)) {
    if (!left_ || !right_) throw std::invalid_argument("condition node: child must not be null");
}

std::string BinaryNode::describeWith(const std::string& keyword) const {
    return describeChild(*left_, precedence()) + " " + keyword + " " +
           describeChild(*right_, precedence());
}

// ------------------------------------------------------------------ AndNode

AndNode::AndNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right)
    : BinaryNode(std::move(left), std::move(right)) {}

bool AndNode::evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const {
    return left().evaluate(item, ctx) && right().evaluate(item, ctx);
}

bool AndNode::mayMatch(const domain::Product& product) const {
    return left().mayMatch(product) && right().mayMatch(product);
}

std::string AndNode::describe() const {
    return describeWith("AND");
}

// ------------------------------------------------------------------- OrNode

OrNode::OrNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right)
    : BinaryNode(std::move(left), std::move(right)) {}

bool OrNode::evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const {
    return left().evaluate(item, ctx) || right().evaluate(item, ctx);
}

bool OrNode::mayMatch(const domain::Product& product) const {
    return left().mayMatch(product) || right().mayMatch(product);
}

std::string OrNode::describe() const {
    return describeWith("OR");
}

// ------------------------------------------------------------------ NotNode

NotNode::NotNode(std::unique_ptr<ConditionNode> child) : child_(std::move(child)) {
    if (!child_) throw std::invalid_argument("condition node: child must not be null");
}

bool NotNode::evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const {
    return !child_->evaluate(item, ctx);
}

bool NotNode::mayMatch(const domain::Product&) const {
    return true;
}

std::string NotNode::describe() const {
    return "NOT " + describeChild(*child_, precedence());
}

// ----------------------------------------------------------- ComparisonNode

ComparisonNode::ComparisonNode(ConditionField field, CompareOp op, ConditionValue value)
    : field_(std::move(field)), op_(op), value_(std::move(value)) {
    if (value_.type() != field_.type()) {
        throw std::invalid_argument("field '" + field_.name() + "' expects a " +
                                    toString(field_.type()) + ", got a " +
                                    toString(value_.type()));
    }
    if (isOrdering(op_) && !isOrdered(field_.type())) {
        throw std::invalid_argument("operator '" + toString(op_) + "' cannot be used on " +
                                    toString(field_.type()) + " field '" + field_.name() + "'");
    }
}

bool ComparisonNode::evaluate(const domain::CartItem& item,
                              const domain::PricingContext& ctx) const {
    return field_.read(item, ctx).compare(op_, value_);
}

bool ComparisonNode::mayMatch(const domain::Product& product) const {
    const auto actual = field_.readProduct(product);
    if (!actual) return true; // depends on the cart or customer: cannot rule it out
    return actual->compare(op_, value_);
}

std::string ComparisonNode::describe() const {
    return field_.name() + " " + toString(op_) + " " + value_.toString();
}

} // namespace rules
