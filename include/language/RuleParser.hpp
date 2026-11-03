#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "language/RuleLexer.hpp"
#include "language/Token.hpp"
#include "rules/CompositeRule.hpp"
#include "rules/ConditionField.hpp"
#include "rules/ConditionNode.hpp"
#include "rules/DiscountRule.hpp"

namespace language {

/**
 * @brief Recursive-descent parser for the text rule language. Builds one
 *        CompositeRule (owning a ConditionNode tree) per rule line.
 *
 * Grammar (EBNF), one rule per line, `#` starts a comment, keywords any case:
 * @code
 *   rule        = [ "RULE" IDENT ":" ] "IF" condition "THEN" action { option } ;
 *   condition   = or_expr ;
 *   or_expr     = and_expr { "OR" and_expr } ;          (lowest precedence)
 *   and_expr    = not_expr { "AND" not_expr } ;
 *   not_expr    = "NOT" not_expr | primary ;            (highest precedence)
 *   primary     = "(" or_expr ")" | comparison ;
 *   comparison  = FIELD comp_op literal ;
 *   comp_op     = "==" | "!=" | "<" | "<=" | ">" | ">=" ;
 *   literal     = NUMBER | DATE | IDENT | STRING ;
 *   action      = NUMBER "%" "OFF" ;                    (1..100)
 *   option      = "PRIORITY" NUMBER | "STACKABLE" | "EXCLUSIVE" | "NAME" STRING ;
 * @endcode
 * Fields: category, product (text); qty, cart_qty (number); price, cart_total
 * (whole rupees); tier (REGULAR < SILVER < GOLD < PLATINUM); date (YYYY-MM-DD).
 * Defaults: id TXT_L<line>, name generated from the condition, PRIORITY 0,
 * STACKABLE.
 *
 * OOP ideas: Single Responsibility (lexing is delegated to RuleLexer; the
 * parser only checks structure and types) and Dependency Injection - the
 * known fields come from an injected FieldCatalog, so new fields need no
 * change here (Open/Closed). Each grammar rule is one private method.
 * Every error is an infrastructure::RuleParseException with line and column.
 */
class RuleParser {
public:
    /// @p fields must outlive the parser. @p source names the file in error messages.
    explicit RuleParser(const rules::FieldCatalog& fields, std::string source = "rules.txt");
    RuleParser(rules::FieldCatalog&&, std::string = {}) = delete; // would dangle

    /// Parses one line. @return nullptr for a blank or comment-only line.
    /// @throws infrastructure::RuleParseException on any syntax or type error.
    std::unique_ptr<rules::CompositeRule> parseLine(std::string_view text, int lineNumber) const;

    /// Parses every line of a whole file's text.
    /// @throws infrastructure::RuleParseException on a bad line or a repeated rule id.
    std::vector<std::unique_ptr<rules::DiscountRule>> parseText(std::string_view text) const;

    /// Reads and parses a file; errors are reported against @p path.
    /// @throws infrastructure::ConfigError if the file cannot be read,
    ///         infrastructure::RuleParseException on a bad rule.
    static std::vector<std::unique_ptr<rules::DiscountRule>> parseFile(
        const std::string& path, const rules::FieldCatalog& fields);

    const std::string& source() const noexcept { return source_; }

private:
    class Cursor; // walks the token list of one line; defined in the .cpp

    std::unique_ptr<rules::CompositeRule> parseRule(Cursor& c) const;
    std::unique_ptr<rules::ConditionNode> parseOr(Cursor& c) const;
    std::unique_ptr<rules::ConditionNode> parseAnd(Cursor& c) const;
    std::unique_ptr<rules::ConditionNode> parseNot(Cursor& c) const;
    std::unique_ptr<rules::ConditionNode> parsePrimary(Cursor& c) const;
    std::unique_ptr<rules::ConditionNode> parseComparison(Cursor& c) const;
    rules::ConditionValue parseLiteral(Cursor& c, const rules::ConditionField& field,
                                       const Token& opToken) const;
    int parseAction(Cursor& c) const;

    [[noreturn]] void fail(const Token& at, const std::string& message) const;
    int toInt(const Token& number) const;

    const rules::FieldCatalog& fields_;
    std::string source_;
    RuleLexer lexer_;
};

} // namespace language
