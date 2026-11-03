#include <gtest/gtest.h>

#include <stdexcept>

#include "infrastructure/Exceptions.hpp"

using namespace infrastructure;

TEST(Exceptions, HierarchyLetsCallersCatchTheBase) {
    // RuleParseException is-a ConfigError is-a PricingError is-a std::runtime_error.
    EXPECT_THROW(throw RuleParseException("r.txt", 1, 1, "x"), ConfigError);
    EXPECT_THROW(throw RuleParseException("r.txt", 1, 1, "x"), PricingError);
    EXPECT_THROW(throw ConfigError("bad"), PricingError);
    EXPECT_THROW(throw PricingError("bad"), std::runtime_error);
}

TEST(Exceptions, MessagesAreKept) {
    EXPECT_STREQ(PricingError("boom").what(), "boom");
    EXPECT_STREQ(ConfigError("bad file").what(), "bad file");
}

TEST(Exceptions, RuleParseExceptionCarriesPosition) {
    const RuleParseException e("rules.txt", 3, 14, "expected THEN");
    EXPECT_STREQ(e.what(), "rules.txt:3:14: expected THEN");
    EXPECT_EQ(e.source(), "rules.txt");
    EXPECT_EQ(e.line(), 3);
    EXPECT_EQ(e.column(), 14);
    EXPECT_EQ(e.detail(), "expected THEN");
}
