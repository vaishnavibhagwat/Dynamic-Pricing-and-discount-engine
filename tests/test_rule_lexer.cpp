#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "infrastructure/Exceptions.hpp"
#include "language/RuleLexer.hpp"

using infrastructure::RuleParseException;
using language::RuleLexer;
using language::Token;
using language::TokenType;

namespace {
std::vector<TokenType> types(const std::string& line) {
    std::vector<TokenType> out;
    for (const auto& t : RuleLexer().tokenize(line, 1)) out.push_back(t.type);
    return out;
}

/// Lexes @p line expecting an error; returns it.
RuleParseException errorFor(const std::string& line, int lineNumber = 1) {
    try {
        RuleLexer("test.txt").tokenize(line, lineNumber);
    } catch (const RuleParseException& e) {
        return e;
    }
    ADD_FAILURE() << "no error for: " << line;
    return RuleParseException("", 0, 0, "");
}
} // namespace

TEST(RuleLexer, TokenizesUsersExample) {
    const auto tokens = RuleLexer().tokenize(
        "IF category == Electronics AND tier >= GOLD AND qty >= 2 THEN 12% OFF PRIORITY 3", 7);
    const std::vector<TokenType> expected{
        TokenType::If,         TokenType::Identifier,   TokenType::Equal,  TokenType::Identifier,
        TokenType::And,        TokenType::Identifier,   TokenType::GreaterEqual,
        TokenType::Identifier, TokenType::And,          TokenType::Identifier,
        TokenType::GreaterEqual, TokenType::Number,     TokenType::Then,   TokenType::Number,
        TokenType::Percent,    TokenType::Off,          TokenType::Priority, TokenType::Number,
        TokenType::End};
    ASSERT_EQ(tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) EXPECT_EQ(tokens[i].type, expected[i]) << i;
    EXPECT_EQ(tokens[1].text, "category");
    EXPECT_EQ(tokens[1].line, 7);
    EXPECT_EQ(tokens[1].column, 4);
    EXPECT_EQ(tokens[13].text, "12");
}

TEST(RuleLexer, AllOperatorsAndPunctuation) {
    EXPECT_EQ(types("== != < <= > >= ( ) : %"),
              (std::vector<TokenType>{TokenType::Equal, TokenType::NotEqual, TokenType::Less,
                                      TokenType::LessEqual, TokenType::Greater,
                                      TokenType::GreaterEqual, TokenType::LParen,
                                      TokenType::RParen, TokenType::Colon, TokenType::Percent,
                                      TokenType::End}));
    // No spaces needed between tokens.
    EXPECT_EQ(types("qty>=2"), (std::vector<TokenType>{TokenType::Identifier,
                                                       TokenType::GreaterEqual, TokenType::Number,
                                                       TokenType::End}));
}

TEST(RuleLexer, KeywordsAnyCase) {
    EXPECT_EQ(types("if Then and OR not off Priority rule stackable EXCLUSIVE name"),
              (std::vector<TokenType>{TokenType::If, TokenType::Then, TokenType::And,
                                      TokenType::Or, TokenType::Not, TokenType::Off,
                                      TokenType::Priority, TokenType::Rule, TokenType::Stackable,
                                      TokenType::Exclusive, TokenType::Name, TokenType::End}));
}

TEST(RuleLexer, DatesStringsAndIdentifiers) {
    const auto tokens = RuleLexer().tokenize("date >= 2026-10-25 \"Home Decor\" E1 cart_total", 1);
    ASSERT_EQ(tokens.size(), 7u); // six tokens + End
    EXPECT_EQ(tokens[2].type, TokenType::Date);
    EXPECT_EQ(tokens[2].text, "2026-10-25");
    EXPECT_EQ(tokens[3].type, TokenType::String);
    EXPECT_EQ(tokens[3].text, "Home Decor");
    EXPECT_EQ(tokens[4].type, TokenType::Identifier);
    EXPECT_EQ(tokens[5].text, "cart_total");
}

TEST(RuleLexer, CommentsAndBlankLines) {
    EXPECT_EQ(types(""), (std::vector<TokenType>{TokenType::End}));
    EXPECT_EQ(types("   # just a comment"), (std::vector<TokenType>{TokenType::End}));
    EXPECT_EQ(types("qty # trailing"), (std::vector<TokenType>{TokenType::Identifier,
                                                              TokenType::End}));
    EXPECT_EQ(types("qty\r"), (std::vector<TokenType>{TokenType::Identifier, TokenType::End}));
}

TEST(RuleLexer, EndTokenColumnIsAfterLastCharacter) {
    const auto tokens = RuleLexer().tokenize("IF qty", 1);
    EXPECT_EQ(tokens.back().type, TokenType::End);
    EXPECT_EQ(tokens.back().column, 7);
}

TEST(RuleLexer, SingleEqualsSuggestsDoubleEquals) {
    const auto e = errorFor("IF qty = 2", 4);
    EXPECT_EQ(e.line(), 4);
    EXPECT_EQ(e.column(), 8);
    EXPECT_NE(e.detail().find("did you mean '=='"), std::string::npos);
    EXPECT_EQ(std::string(e.what()).rfind("test.txt:4:8: ", 0), 0u) << e.what();
}

TEST(RuleLexer, BangAloneSuggestsNotEqual) {
    const auto e = errorFor("qty ! 2");
    EXPECT_EQ(e.column(), 5);
    EXPECT_NE(e.detail().find("'!='"), std::string::npos);
}

TEST(RuleLexer, UnexpectedCharacter) {
    const auto e = errorFor("IF qty >= 2 @");
    EXPECT_EQ(e.column(), 13);
    EXPECT_NE(e.detail().find("unexpected character '@'"), std::string::npos);
}

TEST(RuleLexer, UnterminatedString) {
    const auto e = errorFor("NAME \"Big basket");
    EXPECT_EQ(e.column(), 6);
    EXPECT_EQ(e.detail(), "unterminated string");
}

TEST(RuleLexer, MalformedDate) {
    const auto e = errorFor("date >= 2026-1-5");
    EXPECT_EQ(e.column(), 9);
    EXPECT_NE(e.detail().find("YYYY-MM-DD"), std::string::npos);
}

TEST(RuleLexer, TokenDisplay) {
    EXPECT_EQ((Token{TokenType::Then, "THEN", 1, 1}).display(), "'THEN'");
    EXPECT_EQ((Token{TokenType::String, "a b", 1, 1}).display(), "\"a b\"");
    EXPECT_EQ((Token{TokenType::End, "", 1, 1}).display(), "end of line");
    EXPECT_EQ(language::toString(TokenType::GreaterEqual), "'>='");
}
