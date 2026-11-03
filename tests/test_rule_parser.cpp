#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "infrastructure/Exceptions.hpp"
#include "language/RuleParser.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;
using domain::MembershipTier;
using infrastructure::RuleParseException;
using language::RuleParser;

namespace {

const rules::FieldCatalog& catalog() {
    static const rules::FieldCatalog c;
    return c;
}

std::unique_ptr<rules::CompositeRule> parse(const std::string& line, int lineNumber = 1) {
    return RuleParser(catalog(), "test.txt").parseLine(line, lineNumber);
}

/// Parses @p line expecting a RuleParseException; returns it.
RuleParseException errorFor(const std::string& line) {
    try {
        parse(line);
    } catch (const RuleParseException& e) {
        return e;
    }
    ADD_FAILURE() << "no error for: " << line;
    return RuleParseException("", 0, 0, "");
}

/// Expects @p text to fail at line 1, column @p column, with @p fragment in the message.
void expectParseError(const std::string& text, int column, const std::string& fragment) {
    SCOPED_TRACE(text);
    const auto e = errorFor(text);
    EXPECT_EQ(e.line(), 1) << e.what();
    EXPECT_EQ(e.column(), column) << e.what();
    EXPECT_NE(e.detail().find(fragment), std::string::npos) << e.what();
}

const char* kExample =
    "IF category == Electronics AND tier >= GOLD AND qty >= 2 THEN 12% OFF PRIORITY 3";

} // namespace

TEST(RuleParser, ParsesUsersExample) {
    const auto rule = parse(kExample, 4);
    ASSERT_NE(rule, nullptr);
    EXPECT_EQ(rule->id(), "TXT_L4");
    EXPECT_EQ(rule->name(),
              "12% off when category == Electronics AND tier >= GOLD AND qty >= 2");
    EXPECT_EQ(rule->priority(), 3);
    EXPECT_TRUE(rule->stackable());
    EXPECT_EQ(rule->percent(), 12);
    EXPECT_EQ(rule->stage(), rules::RuleStage::Percentage);
}

TEST(RuleParser, UsersExamplePricesCorrectly) {
    const auto rule = parse(kExample);
    Order o;
    o.customer = domain::Customer("CU2", "Ravi", MembershipTier::Gold);
    o.cart += CartItem(phone(), 2);  // 30000
    o.cart += CartItem(kurta(), 3);
    EXPECT_EQ(rule->calculateDiscount(o.context()), rs(3600));

    o.customer = domain::Customer("CU3", "Meera", MembershipTier::Silver);
    EXPECT_FALSE(rule->isApplicable(o.context()));
}

TEST(RuleParser, AndBindsTighterThanOr) {
    const auto rule = parse("IF qty >= 5 OR category == Electronics AND tier >= GOLD THEN 5% OFF");
    const auto* top = dynamic_cast<const rules::OrNode*>(&rule->condition());
    ASSERT_NE(top, nullptr);
    EXPECT_NE(dynamic_cast<const rules::AndNode*>(&top->right()), nullptr);
}

TEST(RuleParser, ParenthesesOverridePrecedence) {
    const auto rule = parse("IF (qty >= 5 OR category == Electronics) AND tier >= GOLD THEN 5% OFF");
    const auto* top = dynamic_cast<const rules::AndNode*>(&rule->condition());
    ASSERT_NE(top, nullptr);
    EXPECT_NE(dynamic_cast<const rules::OrNode*>(&top->left()), nullptr);
    EXPECT_EQ(rule->condition().describe(),
              "(qty >= 5 OR category == Electronics) AND tier >= GOLD");
}

TEST(RuleParser, NotBindsTightest) {
    const auto rule = parse("IF NOT tier == REGULAR AND qty >= 2 THEN 5% OFF");
    const auto* top = dynamic_cast<const rules::AndNode*>(&rule->condition());
    ASSERT_NE(top, nullptr);
    EXPECT_NE(dynamic_cast<const rules::NotNode*>(&top->left()), nullptr);
    // NOT NOT is allowed.
    EXPECT_NE(parse("IF NOT NOT qty >= 2 THEN 5% OFF"), nullptr);
}

TEST(RuleParser, OptionsAndExplicitId) {
    const auto rule = parse(
        "rule BIG_BASKET: if cart_total >= 20000 then 3% off exclusive priority 10 "
        "name \"Big basket bonus\"");
    EXPECT_EQ(rule->id(), "BIG_BASKET");
    EXPECT_EQ(rule->name(), "Big basket bonus");
    EXPECT_EQ(rule->priority(), 10);
    EXPECT_FALSE(rule->stackable());
}

TEST(RuleParser, DefaultsWhenNoOptions) {
    const auto rule = parse("IF qty >= 2 THEN 5% OFF");
    EXPECT_EQ(rule->priority(), 0);
    EXPECT_TRUE(rule->stackable());
}

TEST(RuleParser, AllFieldTypesAndLiterals) {
    const auto rule = parse(
        "IF product == E1 AND category != \"Home Decor\" AND price < 20000 AND cart_qty <= 10 "
        "AND date >= 2026-10-25 AND date <= 2026-11-10 AND tier > silver THEN 7% OFF");
    Order o;
    o.customer = domain::Customer("CU2", "Ravi", MembershipTier::Gold);
    o.cart += CartItem(phone(), 1);
    EXPECT_EQ(rule->calculateDiscount(o.context()), rs(1050));
}

TEST(RuleParser, BlankAndCommentLinesGiveNothing) {
    EXPECT_EQ(parse(""), nullptr);
    EXPECT_EQ(parse("   "), nullptr);
    EXPECT_EQ(parse("# a comment"), nullptr);
    EXPECT_NE(parse("IF qty >= 2 THEN 5% OFF # trailing comment"), nullptr);
}

TEST(RuleParser, ParseTextSkipsBlanksAndNumbersLines) {
    const auto list = RuleParser(catalog()).parseText(
        "# header\n\nIF qty >= 2 THEN 5% OFF\r\nRULE X: IF qty >= 3 THEN 6% OFF\n");
    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0]->id(), "TXT_L3");
    EXPECT_EQ(list[1]->id(), "X");
}

TEST(RuleParser, ParseTextRejectsDuplicateIds) {
    try {
        RuleParser(catalog(), "dup.txt")
            .parseText("RULE A: IF qty >= 2 THEN 5% OFF\nRULE A: IF qty >= 3 THEN 6% OFF");
        FAIL() << "expected an error";
    } catch (const RuleParseException& e) {
        EXPECT_EQ(e.line(), 2);
        EXPECT_NE(e.detail().find("duplicate rule id 'A'"), std::string::npos);
        EXPECT_NE(e.detail().find("line 1"), std::string::npos);
    }
}

TEST(RuleParser, ErrorsInTheText) {
    // column numbers are 1-based
    expectParseError("IF qty >= THEN 5% OFF", 11, "expected a value after '>=' but found 'THEN'");
    expectParseError("IF catgory == Electronics THEN 5% OFF", 4, "unknown field 'catgory'");
    expectParseError("IF category < Electronics THEN 5% OFF", 13,
                       "operator '<' cannot be used on text field 'category'");
    expectParseError("IF tier >= 5 THEN 5% OFF", 12,
                       "field 'tier' expects REGULAR, SILVER, GOLD or PLATINUM, got '5'");
    expectParseError("IF tier >= DIAMOND THEN 5% OFF", 12, "got 'DIAMOND'");
    expectParseError("IF qty >= two THEN 5% OFF", 11, "expects a whole number");
    expectParseError("IF cart_total >= 2026-01-01 THEN 5% OFF", 18, "expects whole rupees");
    expectParseError("IF date >= 2026-02-30 THEN 5% OFF", 12, "invalid date '2026-02-30'");
    expectParseError("IF date >= 5 THEN 5% OFF", 12, "expects a date");
    expectParseError("IF qty 2 THEN 5% OFF", 8, "expected a comparison operator");
}

TEST(RuleParser, ErrorsInTheStructure) {
    expectParseError("qty >= 2 THEN 5% OFF", 1, "expected 'IF'");
    expectParseError("IF qty >= 2 5% OFF", 13, "expected AND, OR or THEN");
    expectParseError("IF qty >= 2", 12, "but found end of line");
    expectParseError("IF (qty >= 2 THEN 5% OFF", 14, "expected ')' to close '(' at column 4");
    expectParseError("IF qty >= 2 AND THEN 5% OFF", 17, "expected a field name or '('");
    expectParseError("RULE : IF qty >= 2 THEN 5% OFF", 6, "expected a rule id");
    expectParseError("RULE X IF qty >= 2 THEN 5% OFF", 8, "expected ':'");
}

TEST(RuleParser, ErrorsInTheAction) {
    expectParseError("IF qty >= 2 THEN OFF", 18, "expected a discount like '12% OFF'");
    expectParseError("IF qty >= 2 THEN 150% OFF", 18, "between 1% and 100%, got 150%");
    expectParseError("IF qty >= 2 THEN 0% OFF", 18, "between 1% and 100%");
    expectParseError("IF qty >= 2 THEN 12 OFF", 21, "expected '%' after 12");
    expectParseError("IF qty >= 2 THEN 12%", 21, "expected 'OFF' after '12%'");
}

TEST(RuleParser, ErrorsInTheOptions) {
    expectParseError("IF qty >= 2 THEN 5% OFF PRIORITY", 33, "expected a whole number after PRIORITY");
    expectParseError("IF qty >= 2 THEN 5% OFF PRIORITY 1 PRIORITY 2", 36, "more than once");
    expectParseError("IF qty >= 2 THEN 5% OFF STACKABLE EXCLUSIVE", 35, "more than once");
    expectParseError("IF qty >= 2 THEN 5% OFF NAME Big", 30, "expected a quoted name");
    expectParseError("IF qty >= 2 THEN 5% OFF qty", 25, "unexpected 'qty' after the rule");
    expectParseError("IF qty >= 2 THEN 5% OFF PRIORITY 9999999999", 34, "too large");
}

TEST(RuleParser, ParseExceptionIsAConfigError) {
    EXPECT_THROW(parse("IF"), infrastructure::ConfigError);
    try {
        parse("IF qty >= THEN 5% OFF");
    } catch (const RuleParseException& e) {
        EXPECT_EQ(e.source(), "test.txt");
        EXPECT_EQ(std::string(e.what()),
                  "test.txt:1:11: expected a value after '>=' but found 'THEN'");
    }
}

TEST(RuleParser, ParseFileMissingIsConfigError) {
    EXPECT_THROW(RuleParser::parseFile("does/not/exist.txt", catalog()),
                 infrastructure::ConfigError);
}
