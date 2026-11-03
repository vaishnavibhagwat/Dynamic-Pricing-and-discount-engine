#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "domain/Customer.hpp"
#include "domain/Date.hpp"
#include "domain/Money.hpp"

namespace rules {

/// Kind of value a rule condition compares.
enum class ValueType { Number, Money, Text, Tier, Date };

/// e.g. "number", "money", "text", "tier", "date"
std::string toString(ValueType type);

/// True if <, <=, > and >= make sense for this type (everything except Text).
bool isOrdered(ValueType type) noexcept;

/// A comparison operator in a rule condition.
enum class CompareOp { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };

/// e.g. "==", "<="
std::string toString(CompareOp op);

/// True for <, <=, > and >= (they need an ordered type).
bool isOrdering(CompareOp op) noexcept;

/**
 * @brief One typed value inside a rule condition: a literal such as `GOLD`
 *        or `2026-10-25`, or what a field reads from the cart.
 *
 * OOP ideas: encapsulation and value objects - the type tag and the stored
 * data can only be built together through the named constructors, so a
 * value is never in an inconsistent state. Money is kept as paise, so no
 * floating point is ever used. Text comparisons ignore case.
 */
class ConditionValue {
public:
    static ConditionValue number(std::int64_t n);
    static ConditionValue money(domain::Money amount);
    static ConditionValue text(std::string s);
    static ConditionValue tier(domain::MembershipTier t);
    static ConditionValue date(domain::Date d);

    ValueType type() const noexcept { return type_; }

    /// `*this op rhs`, e.g. tier GOLD >= SILVER is true.
    /// @throws std::invalid_argument if the types differ or op is an ordering on text.
    bool compare(CompareOp op, const ConditionValue& rhs) const;

    /// As it would be written in a rule file, e.g. "2", "5000", "GOLD", "\"Home Decor\"".
    std::string toString() const;

private:
    ConditionValue(ValueType type, std::variant<std::int64_t, std::string, domain::Date> data);

    /// -1, 0 or +1. Types must already match.
    int threeWay(const ConditionValue& rhs) const;

    ValueType type_;
    std::variant<std::int64_t, std::string, domain::Date> data_; // int64: Number, Money paise, Tier ordinal
};

} // namespace rules
