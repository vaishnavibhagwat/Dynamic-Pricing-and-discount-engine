#pragma once

#include <string>

namespace language {

/// Every kind of token in the rule language.
enum class TokenType {
    Identifier, ///< field names and bare words: category, Electronics, GOLD
    Number,     ///< whole number: 12
    Date,       ///< YYYY-MM-DD: 2026-10-25
    String,     ///< "quoted text" (text holds the contents without quotes)
    Percent,    ///< %
    LParen,     ///< (
    RParen,     ///< )
    Colon,      ///< :
    Equal,      ///< ==
    NotEqual,   ///< !=
    Less,       ///< <
    LessEqual,  ///< <=
    Greater,    ///< >
    GreaterEqual, ///< >=
    If, Then, And, Or, Not, Off, Priority, Rule, Stackable, Exclusive, Name, ///< keywords
    End         ///< end of the line
};

/// e.g. "identifier", "'=='", "'THEN'", "end of line"
std::string toString(TokenType type);

/**
 * @brief One lexical unit of a rule line, with its position for error messages.
 *
 * OOP idea: a small value object - type, text and 1-based line/column travel
 * together, so every later error can point at the exact place in the file.
 */
struct Token {
    TokenType type = TokenType::End;
    std::string text;
    int line = 1;
    int column = 1;

    /// How to show the token in an error: "'THEN'", "\"Home Decor\"" or "end of line".
    std::string display() const;
};

} // namespace language
