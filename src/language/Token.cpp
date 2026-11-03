#include "language/Token.hpp"

namespace language {

std::string toString(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "identifier";
        case TokenType::Number: return "number";
        case TokenType::Date: return "date";
        case TokenType::String: return "string";
        case TokenType::Percent: return "'%'";
        case TokenType::LParen: return "'('";
        case TokenType::RParen: return "')'";
        case TokenType::Colon: return "':'";
        case TokenType::Equal: return "'=='";
        case TokenType::NotEqual: return "'!='";
        case TokenType::Less: return "'<'";
        case TokenType::LessEqual: return "'<='";
        case TokenType::Greater: return "'>'";
        case TokenType::GreaterEqual: return "'>='";
        case TokenType::If: return "'IF'";
        case TokenType::Then: return "'THEN'";
        case TokenType::And: return "'AND'";
        case TokenType::Or: return "'OR'";
        case TokenType::Not: return "'NOT'";
        case TokenType::Off: return "'OFF'";
        case TokenType::Priority: return "'PRIORITY'";
        case TokenType::Rule: return "'RULE'";
        case TokenType::Stackable: return "'STACKABLE'";
        case TokenType::Exclusive: return "'EXCLUSIVE'";
        case TokenType::Name: return "'NAME'";
        case TokenType::End: return "end of line";
    }
    return "token";
}

std::string Token::display() const {
    if (type == TokenType::End) return "end of line";
    if (type == TokenType::String) return "\"" + text + "\"";
    return "'" + text + "'";
}

} // namespace language
