#include "language/RuleParser.hpp"

#include <cctype>
#include <cstdio>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

#include "domain/Customer.hpp"
#include "domain/Date.hpp"
#include "domain/Money.hpp"
#include "infrastructure/Exceptions.hpp"

namespace language {

using rules::CompareOp;
using rules::ConditionNode;
using rules::ConditionValue;
using rules::ValueType;

// ------------------------------------------------------------------ Cursor

/// Position in the token list of one line. The list always ends with End.
class RuleParser::Cursor {
public:
    explicit Cursor(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    const Token& peek() const { return tokens_[pos_]; }
    bool check(TokenType type) const { return peek().type == type; }
    bool atEnd() const { return check(TokenType::End); }

    const Token& next() {
        const Token& t = tokens_[pos_];
        if (t.type != TokenType::End) ++pos_;
        return t;
    }

    bool match(TokenType type) {
        if (!check(type)) return false;
        next();
        return true;
    }

private:
    std::vector<Token> tokens_;
    std::size_t pos_ = 0;
};

// ----------------------------------------------------------------- helpers

namespace {

std::string upper(const std::string& s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

std::optional<CompareOp> compareOpFor(TokenType type) {
    switch (type) {
        case TokenType::Equal: return CompareOp::Equal;
        case TokenType::NotEqual: return CompareOp::NotEqual;
        case TokenType::Less: return CompareOp::Less;
        case TokenType::LessEqual: return CompareOp::LessEqual;
        case TokenType::Greater: return CompareOp::Greater;
        case TokenType::GreaterEqual: return CompareOp::GreaterEqual;
        default: return std::nullopt;
    }
}

bool isLiteral(TokenType type) {
    return type == TokenType::Number || type == TokenType::Date ||
           type == TokenType::Identifier || type == TokenType::String;
}

std::string joined(const std::vector<std::string>& items) {
    std::string out;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out += ", ";
        out += items[i];
    }
    return out;
}

} // namespace

// -------------------------------------------------------------- RuleParser

RuleParser::RuleParser(const rules::FieldCatalog& fields, std::string source)
    : fields_(fields), source_(std::move(source)), lexer_(source_) {}

void RuleParser::fail(const Token& at, const std::string& message) const {
    throw infrastructure::RuleParseException(source_, at.line, at.column, message);
}

int RuleParser::toInt(const Token& number) const {
    if (number.text.size() > 9) fail(number, "number '" + number.text + "' is too large");
    return std::stoi(number.text);
}

std::unique_ptr<rules::CompositeRule> RuleParser::parseLine(std::string_view text,
                                                             int lineNumber) const {
    Cursor c(lexer_.tokenize(text, lineNumber));
    if (c.atEnd()) return nullptr; // blank or comment-only
    return parseRule(c);
}

std::vector<std::unique_ptr<rules::DiscountRule>> RuleParser::parseText(
    std::string_view text) const {
    std::vector<std::unique_ptr<rules::DiscountRule>> result;
    std::map<std::string, int> firstLineOf;
    int lineNumber = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t nl = text.find('\n', start);
        const std::size_t end = nl == std::string_view::npos ? text.size() : nl;
        ++lineNumber;
        if (auto rule = parseLine(text.substr(start, end - start), lineNumber)) {
            const auto [it, inserted] = firstLineOf.emplace(rule->id(), lineNumber);
            if (!inserted) {
                throw infrastructure::RuleParseException(
                    source_, lineNumber, 1,
                    "duplicate rule id '" + rule->id() + "' (first used on line " +
                        std::to_string(it->second) + ")");
            }
            result.push_back(std::move(rule));
        }
        if (nl == std::string_view::npos) break;
        start = nl + 1;
    }
    return result;
}

std::vector<std::unique_ptr<rules::DiscountRule>> RuleParser::parseFile(
    const std::string& path, const rules::FieldCatalog& fields) {
    // C stdio, owned by unique_ptr so it is closed on every path (RAII). It reports a
    // missing file reliably on every toolchain we use.
    const std::unique_ptr<std::FILE, int (*)(std::FILE*)> file(std::fopen(path.c_str(), "rb"),
                                                               &std::fclose);
    if (!file) throw infrastructure::ConfigError("cannot open rules file: " + path);
    std::string content;
    char buffer[4096];
    std::size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof buffer, file.get())) > 0) content.append(buffer, n);
    if (std::ferror(file.get()) != 0) {
        throw infrastructure::ConfigError("cannot read rules file: " + path);
    }
    return RuleParser(fields, path).parseText(content);
}

// rule = [ "RULE" IDENT ":" ] "IF" condition "THEN" action { option }
std::unique_ptr<rules::CompositeRule> RuleParser::parseRule(Cursor& c) const {
    const int lineNumber = c.peek().line;
    std::string id = "TXT_L" + std::to_string(lineNumber);
    if (c.match(TokenType::Rule)) {
        if (!c.check(TokenType::Identifier)) {
            fail(c.peek(), "expected a rule id after 'RULE' but found " + c.peek().display());
        }
        id = c.next().text;
        if (!c.match(TokenType::Colon)) {
            fail(c.peek(), "expected ':' after rule id '" + id + "' but found " +
                               c.peek().display());
        }
    }
    if (!c.match(TokenType::If)) {
        fail(c.peek(), "expected 'IF' at the start of the rule but found " + c.peek().display());
    }

    auto condition = parseOr(c);

    if (!c.match(TokenType::Then)) {
        fail(c.peek(), "expected AND, OR or THEN after the condition but found " +
                           c.peek().display());
    }
    const int percent = parseAction(c);

    // option = "PRIORITY" NUMBER | "STACKABLE" | "EXCLUSIVE" | "NAME" STRING
    std::optional<int> priority;
    std::optional<bool> stackable;
    std::optional<std::string> name;
    while (!c.atEnd()) {
        const Token& option = c.next();
        switch (option.type) {
            case TokenType::Priority: {
                if (priority) fail(option, "PRIORITY is given more than once");
                if (!c.check(TokenType::Number)) {
                    fail(c.peek(), "expected a whole number after PRIORITY but found " +
                                       c.peek().display());
                }
                priority = toInt(c.next());
                break;
            }
            case TokenType::Stackable:
            case TokenType::Exclusive:
                if (stackable) fail(option, "STACKABLE / EXCLUSIVE is given more than once");
                stackable = option.type == TokenType::Stackable;
                break;
            case TokenType::Name:
                if (name) fail(option, "NAME is given more than once");
                if (!c.check(TokenType::String) || c.peek().text.empty()) {
                    fail(c.peek(), "expected a quoted name after NAME but found " +
                                       c.peek().display());
                }
                name = c.next().text;
                break;
            default:
                fail(option, "unexpected " + option.display() +
                                 " after the rule; expected PRIORITY, STACKABLE, EXCLUSIVE or NAME");
        }
    }

    std::string ruleName = name ? *name : rules::CompositeRule::defaultName(percent, *condition);
    return std::make_unique<rules::CompositeRule>(std::move(id), std::move(ruleName),
                                                  priority.value_or(0), stackable.value_or(true),
                                                  std::move(condition), percent);
}

// or_expr = and_expr { "OR" and_expr }
std::unique_ptr<ConditionNode> RuleParser::parseOr(Cursor& c) const {
    auto left = parseAnd(c);
    while (c.match(TokenType::Or)) {
        left = std::make_unique<rules::OrNode>(std::move(left), parseAnd(c));
    }
    return left;
}

// and_expr = not_expr { "AND" not_expr }
std::unique_ptr<ConditionNode> RuleParser::parseAnd(Cursor& c) const {
    auto left = parseNot(c);
    while (c.match(TokenType::And)) {
        left = std::make_unique<rules::AndNode>(std::move(left), parseNot(c));
    }
    return left;
}

// not_expr = "NOT" not_expr | primary
std::unique_ptr<ConditionNode> RuleParser::parseNot(Cursor& c) const {
    if (c.match(TokenType::Not)) return std::make_unique<rules::NotNode>(parseNot(c));
    return parsePrimary(c);
}

// primary = "(" or_expr ")" | comparison
std::unique_ptr<ConditionNode> RuleParser::parsePrimary(Cursor& c) const {
    if (c.check(TokenType::LParen)) {
        const Token open = c.next();
        auto inner = parseOr(c);
        if (!c.match(TokenType::RParen)) {
            fail(c.peek(), "expected ')' to close '(' at column " + std::to_string(open.column) +
                               " but found " + c.peek().display());
        }
        return inner;
    }
    return parseComparison(c);
}

// comparison = FIELD comp_op literal
std::unique_ptr<ConditionNode> RuleParser::parseComparison(Cursor& c) const {
    if (!c.check(TokenType::Identifier)) {
        fail(c.peek(), "expected a field name or '(' but found " + c.peek().display());
    }
    const Token fieldToken = c.next();
    const rules::ConditionField* field = fields_.find(fieldToken.text);
    if (field == nullptr) {
        fail(fieldToken, "unknown field '" + fieldToken.text + "' (known fields: " +
                             joined(fields_.names()) + ")");
    }

    const Token opToken = c.peek();
    const auto op = compareOpFor(opToken.type);
    if (!op) {
        fail(opToken, "expected a comparison operator (==, !=, <, <=, >, >=) after '" +
                          fieldToken.text + "' but found " + opToken.display());
    }
    c.next();
    if (rules::isOrdering(*op) && !rules::isOrdered(field->type())) {
        fail(opToken, "operator '" + opToken.text + "' cannot be used on " +
                          rules::toString(field->type()) + " field '" + field->name() +
                          "'; use == or !=");
    }

    ConditionValue value = parseLiteral(c, *field, opToken);
    return std::make_unique<rules::ComparisonNode>(*field, *op, std::move(value));
}

// literal = NUMBER | DATE | IDENT | STRING, checked against the field's type
ConditionValue RuleParser::parseLiteral(Cursor& c, const rules::ConditionField& field,
                                        const Token& opToken) const {
    const Token& t = c.peek();
    if (!isLiteral(t.type)) {
        fail(t, "expected a value after '" + opToken.text + "' but found " + t.display());
    }
    const std::string& fieldName = field.name();
    switch (field.type()) {
        case ValueType::Number:
            if (t.type != TokenType::Number) {
                fail(t, "field '" + fieldName + "' expects a whole number, got " + t.display());
            }
            return ConditionValue::number(toInt(c.next()));
        case ValueType::Money:
            if (t.type != TokenType::Number) {
                fail(t, "field '" + fieldName + "' expects whole rupees, got " + t.display());
            }
            return ConditionValue::money(domain::Money::fromRupees(toInt(c.next())));
        case ValueType::Text:
            if (t.type == TokenType::Date) {
                fail(t, "field '" + fieldName + "' expects text, got " + t.display());
            }
            return ConditionValue::text(c.next().text);
        case ValueType::Tier: {
            if (t.type == TokenType::Identifier || t.type == TokenType::String) {
                for (const auto tier : domain::allMembershipTiers()) {
                    if (upper(domain::toString(tier)) == upper(t.text)) {
                        c.next();
                        return ConditionValue::tier(tier);
                    }
                }
            }
            fail(t, "field '" + fieldName + "' expects REGULAR, SILVER, GOLD or PLATINUM, got " +
                        t.display());
        }
        case ValueType::Date:
            if (t.type != TokenType::Date) {
                fail(t, "field '" + fieldName + "' expects a date like 2026-10-25, got " +
                            t.display());
            }
            try {
                const domain::Date d = domain::Date::fromString(t.text);
                c.next();
                return ConditionValue::date(d);
            } catch (const std::invalid_argument&) {
                fail(t, "invalid date '" + t.text + "'");
            }
    }
    fail(t, "unsupported field type");
}

// action = NUMBER "%" "OFF"
int RuleParser::parseAction(Cursor& c) const {
    if (!c.check(TokenType::Number)) {
        fail(c.peek(), "expected a discount like '12% OFF' after THEN but found " +
                           c.peek().display());
    }
    const Token number = c.next();
    const int percent = toInt(number);
    if (percent < 1 || percent > 100) {
        fail(number, "discount must be between 1% and 100%, got " + number.text + "%");
    }
    if (!c.match(TokenType::Percent)) {
        fail(c.peek(), "expected '%' after " + number.text + " but found " + c.peek().display());
    }
    if (!c.match(TokenType::Off)) {
        fail(c.peek(), "expected 'OFF' after '" + number.text + "%' but found " +
                           c.peek().display());
    }
    return percent;
}

} // namespace language
