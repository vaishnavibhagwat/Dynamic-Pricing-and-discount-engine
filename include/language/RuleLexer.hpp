#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "language/Token.hpp"

namespace language {

/**
 * @brief Splits one line of a rule file into Tokens.
 *
 * OOP ideas: Single Responsibility - it only knows characters and token
 * shapes; what the tokens mean is the parser's job. Keywords are matched
 * case-insensitively, `#` starts a comment, and the result always ends with
 * an End token.
 */
class RuleLexer {
public:
    /// @p source names the file in error messages, e.g. "rules.txt".
    explicit RuleLexer(std::string source = "rules.txt");

    /// @p lineNumber is 1-based and only used for token positions.
    /// @throws infrastructure::RuleParseException on a bad character, bad date
    ///         or unterminated string.
    std::vector<Token> tokenize(std::string_view line, int lineNumber) const;

    const std::string& source() const noexcept { return source_; }

private:
    [[noreturn]] void fail(int line, int column, const std::string& message) const;

    std::string source_;
};

} // namespace language
