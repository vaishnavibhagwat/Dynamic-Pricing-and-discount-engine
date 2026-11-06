#include <gtest/gtest.h>

#include <string>

#include "infrastructure/Exceptions.hpp"
#include "language/RuleScriptLoader.hpp"
#include "rules/CompositeRule.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/RuleFactory.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using infrastructure::ConfigError;
using infrastructure::RuleParseException;
using language::RuleScriptLoader;
using rules::DiscountRule;
using rules::Repository;

namespace {
const std::string kSampleText = std::string(DATA_DIR) + "/rules.txt";
const std::string kSampleJson = std::string(DATA_DIR) + "/rules.json";
} // namespace

TEST(RuleScriptLoader, LoadsSampleRulesTxt) {
    Repository<DiscountRule> repo;
    EXPECT_EQ(RuleScriptLoader().loadInto(kSampleText, repo), 4u);
    EXPECT_EQ(repo.size(), 4u);
    ASSERT_NE(repo.findById("KURTA_BULK"), nullptr);
    ASSERT_NE(repo.findById("BIG_BASKET"), nullptr);
    EXPECT_EQ(repo.findById("BIG_BASKET")->name(), "Big basket bonus");
    EXPECT_NE(dynamic_cast<const rules::CompositeRule*>(repo.findById("FESTIVE_GROCERY")), nullptr);
    // The user's example line has no RULE id, so it is named after its line number.
    const auto autoNamed = repo.filter([](const DiscountRule& r) {
        return r.id().rfind("TXT_L", 0) == 0;
    });
    ASSERT_EQ(autoNamed.size(), 1u);
    EXPECT_EQ(autoNamed[0]->priority(), 3);
}

TEST(RuleScriptLoader, MergesWithRulesJson) {
    auto repo = rules::RuleFactory().loadFile(kSampleJson);
    const std::size_t fromJson = repo.size();
    RuleScriptLoader().loadInto(kSampleText, repo);
    EXPECT_EQ(repo.size(), fromJson + 4);
    EXPECT_NE(repo.findById("BANK_HDFC"), nullptr);
    EXPECT_NE(repo.findById("KURTA_BULK"), nullptr);
}

TEST(RuleScriptLoader, DuplicateIdWithLoadedRuleIsConfigErrorAndAddsNothing) {
    Repository<DiscountRule> repo;
    repo.add(std::make_unique<rules::PercentageDiscount>("KURTA_BULK", "Existing", 1, true, 5));
    EXPECT_THROW(RuleScriptLoader().loadInto(kSampleText, repo), ConfigError);
    EXPECT_EQ(repo.size(), 1u);
}

TEST(RuleScriptLoader, MissingFileIsConfigError) {
    Repository<DiscountRule> repo;
    EXPECT_THROW(RuleScriptLoader().loadInto("does/not/exist.txt", repo), ConfigError);
}

TEST(RuleScriptLoader, BadFileReportsLineAndColumnAndAddsNothing) {
    Repository<DiscountRule> repo;
    try {
        RuleScriptLoader().loadInto(std::string(TEST_DATA_DIR) + "/bad_rules.txt", repo);
        FAIL() << "expected a parse error";
    } catch (const RuleParseException& e) {
        EXPECT_EQ(e.line(), 3);
        EXPECT_EQ(e.column(), 11);
        EXPECT_NE(e.detail().find("expected a value after '>='"), std::string::npos) << e.what();
        EXPECT_NE(e.source().find("bad_rules.txt"), std::string::npos);
    }
    EXPECT_TRUE(repo.empty());
}
