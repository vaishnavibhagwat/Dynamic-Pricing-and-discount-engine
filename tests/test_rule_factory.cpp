#include <gtest/gtest.h>

#include <set>
#include <string>

#include <nlohmann/json.hpp>

#include "rules/HappyHourRule.hpp"
#include "rules/FirstOrderRule.hpp"
#include "rules/CappedRule.hpp"
#include "infrastructure/Exceptions.hpp"
#include "rules/BankOfferDiscount.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/RuleFactory.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using infrastructure::ConfigError;
using nlohmann::json;
using rules::RuleFactory;

namespace {
template <typename T>
const T& as(const std::unique_ptr<rules::DiscountRule>& rule) {
    const auto* typed = dynamic_cast<const T*>(rule.get());
    if (typed == nullptr) throw std::runtime_error("wrong rule type");
    return *typed;
}

json base(const std::string& type) {
    return {{"id", "R1"}, {"type", type}, {"name", "Test rule"}, {"priority", 10}};
}

/// Runs create() and returns the ConfigError message (or "" if nothing was thrown).
std::string errorFor(const json& spec) {
    try {
        RuleFactory().create(spec);
    } catch (const ConfigError& e) {
        return e.what();
    }
    return "";
}
} // namespace

TEST(RuleFactory, RegistersAllBuiltInTypes) {
    const RuleFactory f;
    for (const char* type :
         {"percentage", "quantity", "seasonal", "expiry", "membership", "coupon", "bank"}) {
        EXPECT_TRUE(f.supports(type)) << type;
    }
    EXPECT_FALSE(f.supports("bogo"));
}

TEST(RuleFactory, CreatesPercentageWithCommonFields) {
    json j = base("percentage");
    j["percent"] = 10;
    j["category"] = "Furniture";
    j["stackable"] = false;
    const auto rule = RuleFactory().create(j);
    const auto& p = as<rules::PercentageDiscount>(rule);
    EXPECT_EQ(p.id(), "R1");
    EXPECT_EQ(p.name(), "Test rule");
    EXPECT_EQ(p.priority(), 10);
    EXPECT_FALSE(p.stackable());
    EXPECT_EQ(p.percent(), 10);
    EXPECT_EQ(p.filter().category(), std::optional<std::string>("Furniture"));
}

TEST(RuleFactory, StackableDefaultsToTrue) {
    json j = base("percentage");
    j["percent"] = 10;
    EXPECT_TRUE(RuleFactory().create(j)->stackable());
}

TEST(RuleFactory, CreatesQuantityWithProductIds) {
    json j = base("quantity");
    j["min_quantity"] = 3;
    j["percent"] = 15;
    j["product_ids"] = {"C1", "C2"};
    const auto rule = RuleFactory().create(j);
    const auto& q = as<rules::QuantityDiscount>(rule);
    EXPECT_EQ(q.minQuantity(), 3);
    EXPECT_EQ(q.filter().productIds().size(), 2u);
}

TEST(RuleFactory, CreatesSeasonal) {
    json j = base("seasonal");
    j["percent"] = 20;
    j["start"] = "2026-10-25";
    j["end"] = "2026-11-10";
    const auto rule = RuleFactory().create(j);
    const auto& s = as<rules::SeasonalDiscount>(rule);
    EXPECT_EQ(s.start(), domain::Date(2026, 10, 25));
    EXPECT_EQ(s.end(), domain::Date(2026, 11, 10));
}

TEST(RuleFactory, CreatesExpiry) {
    json j = base("expiry");
    j["tiers"] = json::array({{{"max_days_left", 3}, {"percent", 30}},
                              {{"max_days_left", 1}, {"percent", 50}}});
    const auto rule = RuleFactory().create(j);
    EXPECT_EQ(as<rules::ExpiryMarkdown>(rule).percentFor(0), 50);
}

TEST(RuleFactory, CreatesMembership) {
    json j = base("membership");
    j["tiers"] = {{"Gold", 5}, {"Platinum", 10}};
    const auto rule = RuleFactory().create(j);
    const auto& m = as<rules::MembershipDiscount>(rule);
    EXPECT_EQ(m.percentByTier().at(domain::MembershipTier::Platinum), 10);
}

TEST(RuleFactory, CreatesFlatAndPercentCoupons) {
    json flat = base("coupon");
    flat["code"] = "FESTIVE500";
    flat["min_cart_value"] = 2999;
    flat["usage_limit"] = 100;
    flat["flat"] = 500;
    const auto f = RuleFactory().create(flat);
    EXPECT_EQ(as<rules::CouponDiscount>(f).minCartValue(), rs(2999));
    EXPECT_EQ(as<rules::CouponDiscount>(f).benefit().describe(), "\xE2\x82\xB9" "500.00 off");

    json pct = base("coupon");
    pct["code"] = "NEW";
    pct["usage_limit"] = 1;
    pct["percent"] = 15;
    pct["cap"] = 300;
    const auto p = RuleFactory().create(pct);
    EXPECT_EQ(as<rules::CouponDiscount>(p).minCartValue(), Money::zero());
}

TEST(RuleFactory, CouponNeedsExactlyOneBenefit) {
    json both = base("coupon");
    both["code"] = "X";
    both["usage_limit"] = 1;
    both["flat"] = 100;
    both["percent"] = 10;
    EXPECT_NE(errorFor(both).find("exactly one"), std::string::npos);

    json neither = base("coupon");
    neither["code"] = "X";
    neither["usage_limit"] = 1;
    EXPECT_NE(errorFor(neither).find("exactly one"), std::string::npos);
}

TEST(RuleFactory, CreatesBank) {
    json j = base("bank");
    j["bank"] = "HDFC";
    j["percent"] = 10;
    j["cap"] = 1500;
    j["min_transaction"] = 5000;
    const auto rule = RuleFactory().create(j);
    const auto& b = as<rules::BankOfferDiscount>(rule);
    EXPECT_EQ(b.cap(), rs(1500));
    EXPECT_EQ(b.minTransaction(), rs(5000));
}

TEST(RuleFactory, UnknownTypeIsConfigError) {
    EXPECT_NE(errorFor(base("bogo")).find("unknown rule type \"bogo\""), std::string::npos);
}

TEST(RuleFactory, MissingFieldNamesRuleAndField) {
    const std::string msg = errorFor(base("percentage"));
    EXPECT_NE(msg.find("rule 'R1'"), std::string::npos) << msg;
    EXPECT_NE(msg.find("\"percent\""), std::string::npos) << msg;
}

TEST(RuleFactory, MissingIdIsConfigError) {
    EXPECT_THROW(RuleFactory().create(json{{"type", "percentage"}}), ConfigError);
    EXPECT_THROW(RuleFactory().create(json::array()), ConfigError);
}

TEST(RuleFactory, WrongFieldTypeIsConfigError) {
    json j = base("percentage");
    j["percent"] = "ten";
    EXPECT_NE(errorFor(j).find("whole number"), std::string::npos);
    j["percent"] = 10.5;
    EXPECT_NE(errorFor(j).find("whole number"), std::string::npos);
}

TEST(RuleFactory, BadDateIsConfigError) {
    json j = base("seasonal");
    j["percent"] = 20;
    j["start"] = "25-10-2026";
    j["end"] = "2026-11-10";
    EXPECT_NE(errorFor(j).find("\"start\""), std::string::npos);
    j["start"] = "2026-02-30";
    EXPECT_THROW(RuleFactory().create(j), ConfigError);
}

TEST(RuleFactory, ConstructorValidationBecomesConfigError) {
    json j = base("seasonal");
    j["percent"] = 20;
    j["start"] = "2026-11-10";
    j["end"] = "2026-10-25";
    EXPECT_NE(errorFor(j).find("rule 'R1'"), std::string::npos);

    json bad = base("bank");
    bad["bank"] = "HDFC";
    bad["percent"] = 150;
    bad["cap"] = 100;
    EXPECT_THROW(RuleFactory().create(bad), ConfigError);
}

TEST(RuleFactory, UnknownMembershipTierIsConfigError) {
    json j = base("membership");
    j["tiers"] = {{"Diamond", 5}};
    EXPECT_NE(errorFor(j).find("Diamond"), std::string::npos);
}

TEST(RuleFactory, CustomTypeCanBeRegisteredWithoutEditingFactory) {
    RuleFactory f;
    f.registerType("staff", [](const rules::RuleSpec& s) {
        return std::make_unique<rules::PercentageDiscount>(s.id(), s.name(), s.priority(),
                                                           s.stackable(), 25);
    });
    EXPECT_TRUE(f.supports("staff"));
    const auto rule = f.create(base("staff"));
    EXPECT_EQ(as<rules::PercentageDiscount>(rule).percent(), 25);
}

TEST(RuleFactory, RegisterTypeRejectsDuplicatesAndEmpties) {
    RuleFactory f;
    const RuleFactory::Creator dummy = [](const rules::RuleSpec&) {
        return std::unique_ptr<rules::DiscountRule>();
    };
    EXPECT_THROW(f.registerType("bank", dummy), std::invalid_argument);
    EXPECT_THROW(f.registerType("", dummy), std::invalid_argument);
    EXPECT_THROW(f.registerType("x", RuleFactory::Creator()), std::invalid_argument);
}

TEST(RuleFactory, CreateAllRejectsDuplicateIds) {
    json r = base("percentage");
    r["percent"] = 5;
    const json doc = {{"rules", {r, r}}};
    EXPECT_THROW(RuleFactory().createAll(doc), ConfigError);
}

TEST(RuleFactory, CreateAllAcceptsBareArrayAndRejectsBadShape) {
    json r = base("percentage");
    r["percent"] = 5;
    EXPECT_EQ(RuleFactory().createAll(json::array({r})).size(), 1u);
    EXPECT_THROW(RuleFactory().createAll(json{{"offers", json::array()}}), ConfigError);
    EXPECT_THROW(RuleFactory().createAll(json{{"rules", 5}}), ConfigError);
}

TEST(RuleFactory, LoadFileMissingIsConfigError) {
    EXPECT_THROW(RuleFactory().loadFile("does/not/exist.json"), ConfigError);
}

TEST(RuleFactory, LoadsSampleRulesJson) {
    const auto repo = RuleFactory().loadFile(std::string(DATA_DIR) + "/rules.json");
    EXPECT_EQ(repo.size(), 13u);
    ASSERT_NE(repo.findById("BANK_HDFC"), nullptr);
    ASSERT_NE(repo.findById("DIWALI_ELECTRONICS"), nullptr);

    // HDFC 10% up to Rs 1500 behaves as advertised on a big order.
    Order o;
    o.cart += domain::CartItem(phone(), 2);  // 30000
    o.bank = "HDFC";
    EXPECT_EQ(repo.findById("BANK_HDFC")->calculateDiscount(o.context()), rs(1500));
    // Diwali sale is live on 2026-10-28: 20% of 30000.
    EXPECT_EQ(repo.findById("DIWALI_ELECTRONICS")->calculateDiscount(o.context()), rs(6000));
}

TEST(RuleFactory, RegistersBuiltInDecorators) {
    const RuleFactory f;
    for (const char* type : {"cap", "happy_hour", "first_order"}) {
        EXPECT_TRUE(f.supportsDecorator(type)) << type;
    }
    EXPECT_FALSE(f.supportsDecorator("percentage"));
}

TEST(RuleFactory, WrapsRulesInDecoratorsInnermostFirst) {
    json j = base("percentage");
    j["percent"] = 20;
    j["decorators"] = json::parse(R"([
        {"type": "happy_hour", "from": "18:00", "to": "21:00"},
        {"type": "cap", "max": 500}])");
    const auto rule = RuleFactory().create(j);
    EXPECT_EQ(rule->id(), "R1");
    EXPECT_EQ(rule->name(), "Test rule [happy hour 18:00-21:00] [max ₹500.00 off]");
    const auto& cap = as<rules::CappedRule>(rule);
    EXPECT_EQ(cap.cap(), rs(500));
    EXPECT_NE(dynamic_cast<const rules::HappyHourRule*>(&cap.inner()), nullptr);

    Order o;
    o.cart += domain::CartItem(phone(), 1); // 20% of 15000 = 3000, capped at 500
    o.time = domain::TimeOfDay(19, 0);
    EXPECT_EQ(rule->calculateDiscount(o.context()), rs(500));
    o.time = domain::TimeOfDay(10, 0);
    EXPECT_EQ(rule->calculateDiscount(o.context()), Money::zero());
}

TEST(RuleFactory, FirstOrderDecoratorNeedsNoFields) {
    json j = base("percentage");
    j["percent"] = 10;
    j["decorators"] = json::parse(R"([{"type": "first_order"}])");
    const auto rule = RuleFactory().create(j);
    EXPECT_NO_THROW(as<rules::FirstOrderRule>(rule));
}

TEST(RuleFactory, ReportsBadDecorators) {
    json j = base("percentage");
    j["percent"] = 10;
    j["decorators"] = json::parse(R"([{"type": "bogo"}])");
    EXPECT_EQ(errorFor(j), "rule 'R1': unknown decorator type \"bogo\"");
    j["decorators"] = json::parse(R"({"type": "cap"})");
    EXPECT_EQ(errorFor(j), "rule 'R1': field \"decorators\" must be an array");
    j["decorators"] = json::parse(R"([42])");
    EXPECT_EQ(errorFor(j), "rule 'R1': each decorator must be a JSON object");
    j["decorators"] = json::parse(R"([{"type": "cap"}])");
    EXPECT_EQ(errorFor(j), "rule 'R1': missing field \"max\"");
    j["decorators"] = json::parse(R"([{"type": "cap", "max": 0}])");
    EXPECT_EQ(errorFor(j), "rule 'R1': cap must be positive");
    j["decorators"] = json::parse(R"([{"type": "happy_hour", "from": "6pm", "to": "21:00"}])");
    EXPECT_EQ(errorFor(j), "rule 'R1': time must be HH:MM, got \"6pm\"");
}

TEST(RuleFactory, CustomDecoratorCanBeRegistered) {
    RuleFactory f;
    f.registerDecorator("first_order_too", [](std::unique_ptr<rules::DiscountRule> r, const rules::RuleSpec&) {
        return std::make_unique<rules::FirstOrderRule>(std::move(r));
    });
    EXPECT_TRUE(f.supportsDecorator("first_order_too"));
    EXPECT_THROW(f.registerDecorator("cap", [](std::unique_ptr<rules::DiscountRule> r,
                                               const rules::RuleSpec&) { return r; }),
                 std::invalid_argument);
    EXPECT_THROW(f.registerDecorator("x", RuleFactory::DecoratorCreator()), std::invalid_argument);
}
