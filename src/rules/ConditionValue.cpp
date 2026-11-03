#include "rules/ConditionValue.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace rules {

namespace {

std::string lower(const std::string& s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string upper(const std::string& s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

bool isPlainWord(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}

} // namespace

std::string toString(ValueType type) {
    switch (type) {
        case ValueType::Number: return "number";
        case ValueType::Money: return "money";
        case ValueType::Text: return "text";
        case ValueType::Tier: return "tier";
        case ValueType::Date: return "date";
    }
    return "unknown";
}

bool isOrdered(ValueType type) noexcept {
    return type != ValueType::Text;
}

std::string toString(CompareOp op) {
    switch (op) {
        case CompareOp::Equal: return "==";
        case CompareOp::NotEqual: return "!=";
        case CompareOp::Less: return "<";
        case CompareOp::LessEqual: return "<=";
        case CompareOp::Greater: return ">";
        case CompareOp::GreaterEqual: return ">=";
    }
    return "?";
}

bool isOrdering(CompareOp op) noexcept {
    return op != CompareOp::Equal && op != CompareOp::NotEqual;
}

ConditionValue::ConditionValue(ValueType type,
                               std::variant<std::int64_t, std::string, domain::Date> data)
    : type_(type), data_(std::move(data)) {}

ConditionValue ConditionValue::number(std::int64_t n) {
    return ConditionValue(ValueType::Number, n);
}

ConditionValue ConditionValue::money(domain::Money amount) {
    return ConditionValue(ValueType::Money, amount.paise());
}

ConditionValue ConditionValue::text(std::string s) {
    return ConditionValue(ValueType::Text, std::move(s));
}

ConditionValue ConditionValue::tier(domain::MembershipTier t) {
    return ConditionValue(ValueType::Tier, static_cast<std::int64_t>(t));
}

ConditionValue ConditionValue::date(domain::Date d) {
    return ConditionValue(ValueType::Date, d);
}

int ConditionValue::threeWay(const ConditionValue& rhs) const {
    switch (type_) {
        case ValueType::Text: {
            const std::string a = lower(std::get<std::string>(data_));
            const std::string b = lower(std::get<std::string>(rhs.data_));
            return a < b ? -1 : (b < a ? 1 : 0);
        }
        case ValueType::Date: {
            const auto& a = std::get<domain::Date>(data_);
            const auto& b = std::get<domain::Date>(rhs.data_);
            return a < b ? -1 : (b < a ? 1 : 0);
        }
        case ValueType::Number:
        case ValueType::Money:
        case ValueType::Tier: {
            const std::int64_t a = std::get<std::int64_t>(data_);
            const std::int64_t b = std::get<std::int64_t>(rhs.data_);
            return a < b ? -1 : (b < a ? 1 : 0);
        }
    }
    return 0;
}

bool ConditionValue::compare(CompareOp op, const ConditionValue& rhs) const {
    if (type_ != rhs.type_) {
        throw std::invalid_argument("cannot compare " + rules::toString(type_) + " with " +
                                    rules::toString(rhs.type_));
    }
    if (isOrdering(op) && !isOrdered(type_)) {
        throw std::invalid_argument("operator " + rules::toString(op) + " cannot be used on " +
                                    rules::toString(type_));
    }
    const int c = threeWay(rhs);
    switch (op) {
        case CompareOp::Equal: return c == 0;
        case CompareOp::NotEqual: return c != 0;
        case CompareOp::Less: return c < 0;
        case CompareOp::LessEqual: return c <= 0;
        case CompareOp::Greater: return c > 0;
        case CompareOp::GreaterEqual: return c >= 0;
    }
    return false;
}

std::string ConditionValue::toString() const {
    switch (type_) {
        case ValueType::Number: return std::to_string(std::get<std::int64_t>(data_));
        case ValueType::Money: {
            const std::int64_t paise = std::get<std::int64_t>(data_);
            std::string text = std::to_string(paise / 100);
            const std::int64_t rest = paise % 100;
            if (rest != 0) {
                const std::int64_t p = rest < 0 ? -rest : rest;
                text += (p < 10 ? ".0" : ".") + std::to_string(p);
            }
            return text;
        }
        case ValueType::Text: {
            const auto& s = std::get<std::string>(data_);
            return isPlainWord(s) ? s : "\"" + s + "\"";
        }
        case ValueType::Tier:
            return upper(domain::toString(
                static_cast<domain::MembershipTier>(std::get<std::int64_t>(data_))));
        case ValueType::Date: return std::get<domain::Date>(data_).toString();
    }
    return {};
}

} // namespace rules
