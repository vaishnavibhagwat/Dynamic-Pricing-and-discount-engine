#pragma once

#include <memory>
#include <string>

#include "domain/CartItem.hpp"
#include "domain/PricingContext.hpp"
#include "domain/Product.hpp"
#include "rules/ConditionField.hpp"
#include "rules/ConditionValue.hpp"

namespace rules {

/**
 * @brief A node in the condition tree of a text rule (AND, OR, NOT or a comparison).
 *
 * OOP ideas: Composite and Interpreter patterns - AndNode, OrNode and
 * NotNode own child nodes, ComparisonNode is the leaf, and every node
 * answers evaluate() / describe() / mayMatch() through this one abstract
 * interface, so a whole tree is used exactly like a single node
 * (polymorphism). Children are owned by std::unique_ptr (RAII); copying is
 * disabled because the tree has unique ownership.
 */
class ConditionNode {
public:
    virtual ~ConditionNode() = default;

    ConditionNode(const ConditionNode&) = delete;
    ConditionNode& operator=(const ConditionNode&) = delete;
    ConditionNode(ConditionNode&&) = delete;
    ConditionNode& operator=(ConditionNode&&) = delete;

    /// True if the condition holds for this cart line of the order.
    virtual bool evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const = 0;
    /// False only if no cart line holding @p product could ever satisfy the condition,
    /// judged from the product alone (unknown parts count as "may match").
    virtual bool mayMatch(const domain::Product& product) const = 0;
    /// The condition in rule-language syntax, e.g. "(qty >= 2 OR tier >= GOLD) AND price < 5000".
    virtual std::string describe() const = 0;
    /// Binding strength, used by describe() to add only the parentheses needed.
    virtual int precedence() const noexcept = 0;

protected:
    ConditionNode() = default;
    /// describe() of @p child, wrapped in parentheses if it binds looser than @p parentPrecedence.
    static std::string describeChild(const ConditionNode& child, int parentPrecedence);
};

/**
 * @brief Base for AND / OR: two owned children.
 *
 * OOP ideas: inheritance to share code (child ownership and the null check)
 * between the two binary composites.
 */
class BinaryNode : public ConditionNode {
public:
    const ConditionNode& left() const noexcept { return *left_; }
    const ConditionNode& right() const noexcept { return *right_; }

protected:
    /// @throws std::invalid_argument if a child is null.
    BinaryNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right);
    std::string describeWith(const std::string& keyword) const;

private:
    std::unique_ptr<ConditionNode> left_;
    std::unique_ptr<ConditionNode> right_;
};

/**
 * @brief True when both children are true (short-circuits).
 *
 * OOP idea: a Composite node - it treats its children only as ConditionNodes.
 */
class AndNode final : public BinaryNode {
public:
    AndNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right);
    bool evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const override;
    bool mayMatch(const domain::Product& product) const override;
    std::string describe() const override;
    int precedence() const noexcept override { return 2; }
};

/**
 * @brief True when either child is true (short-circuits).
 *
 * OOP idea: a Composite node - it treats its children only as ConditionNodes.
 */
class OrNode final : public BinaryNode {
public:
    OrNode(std::unique_ptr<ConditionNode> left, std::unique_ptr<ConditionNode> right);
    bool evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const override;
    bool mayMatch(const domain::Product& product) const override;
    std::string describe() const override;
    int precedence() const noexcept override { return 1; }
};

/**
 * @brief Negates its single child.
 *
 * OOP idea: a Composite node with one owned child.
 */
class NotNode final : public ConditionNode {
public:
    /// @throws std::invalid_argument if child is null.
    explicit NotNode(std::unique_ptr<ConditionNode> child);
    bool evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const override;
    /// Always true: "may match" cannot be negated safely.
    bool mayMatch(const domain::Product& product) const override;
    std::string describe() const override;
    int precedence() const noexcept override { return 3; }
    const ConditionNode& child() const noexcept { return *child_; }

private:
    std::unique_ptr<ConditionNode> child_;
};

/**
 * @brief The leaf of the tree: `field op value`, e.g. `tier >= GOLD`.
 *
 * OOP ideas: Composite leaf and encapsulation - the constructor checks the
 * literal's type against the field and the operator, so an invalid
 * comparison cannot exist.
 */
class ComparisonNode final : public ConditionNode {
public:
    /// @throws std::invalid_argument if value's type differs from the field's type,
    ///         or op is <, <=, >, >= on a text field.
    ComparisonNode(ConditionField field, CompareOp op, ConditionValue value);
    bool evaluate(const domain::CartItem& item, const domain::PricingContext& ctx) const override;
    bool mayMatch(const domain::Product& product) const override;
    std::string describe() const override;
    int precedence() const noexcept override { return 4; }

    const ConditionField& field() const noexcept { return field_; }
    CompareOp op() const noexcept { return op_; }
    const ConditionValue& value() const noexcept { return value_; }

private:
    ConditionField field_;
    CompareOp op_;
    ConditionValue value_;
};

} // namespace rules
