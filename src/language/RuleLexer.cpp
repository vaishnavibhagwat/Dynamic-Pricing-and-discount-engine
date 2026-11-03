#include "language/RuleLexer.hpp"

#include <cctype>
#include <map>
#include <utility>

#include "infrastructure/Exceptions.hpp"

namespace language {

namespace {

bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }
bool isWordStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }

std::string upper(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

const std::map<std::string, TokenType>& keywords() {
    static const std::map<std::string, TokenType> table{
        {"IF", TokenType::If},           {"THEN", TokenType::Then},
        {"AND", TokenType::And},         {"OR", TokenType::Or},
        {"NOT", TokenType::Not},         {"OFF", TokenType::Off},
        {"PRIORITY", TokenType::Priority}, {"RULE", TokenType::Rule},
        {"STACKABLE", TokenType::Stackable}, {"EXCLUSIVE", TokenType::Exclusive},
        {"NAME", TokenType::Name}};
    return table;
}

/// True if text[pos..] is exactly `count` digits.
bool digitsAt(std::string_view text, std::size_t pos, std::size_t count) {
    if (pos + count > text.size()) return false;
    for (std::size_t i = 0; i < count; ++i) {
        if (!isDigit(text[pos + i])) return false;
    }
    return true;
}

} // namespace

RuleLexer::RuleLexer(std::string source) : source_(std::move(source)) {}

void RuleLexer::fail(int line, int column, const std::string& message) const {
    throw infrastructure::RuleParseException(source_, line, column, message);
}

std::vector<Token> RuleLexer::tokenize(std::string_view text, int lineNumber) const {
    std::vector<Token> tokens;
    std::size_t i = 0;
    const auto add = [&](TokenType type, std::string tokenText, std::size_t start) {
        tokens.push_back({type, std::move(tokenText), lineNumber, static_cast<int>(start) + 1});
    };

    while (i < text.size()) {
        const char c = text[i];
        const std::size_t start = i;
        const int column = static_cast<int>(start) + 1;

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++i; continue; }
        if (c == '#') break; // comment to end of line

        if (isWordStart(c)) {
            while (i < text.size() && isWordChar(text[i])) ++i;
            const std::string_view word = text.substr(start, i - start);
            const auto kw = keywords().find(upper(word));
            add(kw == keywords().end() ? TokenType::Identifier : kw->second, std::string(word),
                start);
            continue;
        }

        if (isDigit(c)) {
            while (i < text.size() && isDigit(text[i])) ++i;
            if (i < text.size() && text[i] == '-') {
                // Looks like a date: must be exactly YYYY-MM-DD.
                std::size_t end = i;
                while (end < text.size() && (isDigit(text[end]) || text[end] == '-')) ++end;
                const std::string_view candidate = text.substr(start, end - start);
                const bool ok = candidate.size() == 10 && digitsAt(candidate, 0, 4) &&
                                candidate[4] == '-' && digitsAt(candidate, 5, 2) &&
                                candidate[7] == '-' && digitsAt(candidate, 8, 2);
                if (!ok) {
                    fail(lineNumber, column,
                         "malformed date '" + std::string(candidate) + "'; expected YYYY-MM-DD");
                }
                i = end;
                add(TokenType::Date, std::string(candidate), start);
            } else {
                add(TokenType::Number, std::string(text.substr(start, i - start)), start);
            }
            continue;
        }

        if (c == '"') {
            const std::size_t close = text.find('"', i + 1);
            if (close == std::string_view::npos) fail(lineNumber, column, "unterminated string");
            add(TokenType::String, std::string(text.substr(i + 1, close - i - 1)), start);
            i = close + 1;
            continue;
        }

        const char next = i + 1 < text.size() ? text[i + 1] : '\0';
        switch (c) {
            case '%': add(TokenType::Percent, "%", start); ++i; continue;
            case '(': add(TokenType::LParen, "(", start); ++i; continue;
            case ')': add(TokenType::RParen, ")", start); ++i; continue;
            case ':': add(TokenType::Colon, ":", start); ++i; continue;
            case '=':
                if (next != '=') fail(lineNumber, column, "unexpected character '='; did you mean '=='?");
                add(TokenType::Equal, "==", start); i += 2; continue;
            case '!':
                if (next != '=') fail(lineNumber, column, "unexpected character '!'; did you mean '!='?");
                add(TokenType::NotEqual, "!=", start); i += 2; continue;
            case '<':
                if (next == '=') { add(TokenType::LessEqual, "<=", start); i += 2; }
                else { add(TokenType::Less, "<", start); ++i; }
                continue;
            case '>':
                if (next == '=') { add(TokenType::GreaterEqual, ">=", start); i += 2; }
                else { add(TokenType::Greater, ">", start); ++i; }
                continue;
            default: break;
        }

        if (std::isprint(static_cast<unsigned char>(c)) != 0) {
            fail(lineNumber, column, std::string("unexpected character '") + c + "'");
        }
        fail(lineNumber, column, "unexpected non-ASCII or control character");
    }

    // i is at the end of the text or at the '#' of a comment.
    tokens.push_back({TokenType::End, "", lineNumber, static_cast<int>(i) + 1});
    return tokens;
}

} // namespace language
