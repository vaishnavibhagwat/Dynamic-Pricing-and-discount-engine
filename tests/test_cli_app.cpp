#include <gtest/gtest.h>

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "infrastructure/Exceptions.hpp"
#include "infrastructure/StreamLogSink.hpp"
#include "presentation/CliApp.hpp"

using presentation::AppConfig;
using presentation::CliApp;

namespace {

const domain::Date kDay(2026, 10, 30);

AppConfig projectData() {
    return AppConfig{DATA_DIR, kDay};
}

/// Runs a scripted session against the project's data files and returns everything printed.
std::string session(const std::string& input, int* exitCode = nullptr) {
    std::istringstream in(input);
    std::ostringstream out;
    CliApp app(in, out, projectData());
    const int code = app.run();
    if (exitCode != nullptr) *exitCode = code;
    return out.str();
}

bool has(const std::string& text, const std::string& what) {
    return text.find(what) != std::string::npos;
}

std::size_t count(const std::string& text, const std::string& what) {
    std::size_t n = 0;
    for (auto pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + 1)) ++n;
    return n;
}

/// Phone x1, kurta x3, chair x1; Gold customer; FESTIVE500 + HDFC.
const std::string kFestiveOrder =
    "2\na\nE1\n1\na\nc1\n3\na\nF1\n\nb\n"
    "3\nCU42\n"
    "4\nfestive500\nhdfc\n";

/// A private copy of data/ (in the build's scratch folder) that a test may break.
class DataCopy {
public:
    DataCopy() {
        for (const char* f : {"catalog.json", "customers.json", "rules.json", "rules.txt"}) {
            std::ifstream in(std::string(DATA_DIR) + "/" + f, std::ios::binary);
            std::ofstream(dir() + "/" + f, std::ios::binary) << in.rdbuf();
        }
    }
    DataCopy(const DataCopy&) = delete;
    DataCopy& operator=(const DataCopy&) = delete;

    std::string dir() const { return std::string(TEST_SCRATCH_DIR) + "/cli_data"; }
    void overwrite(const std::string& file, const std::string& text) const {
        std::ofstream(dir() + "/" + file) << text;
    }
};

} // namespace

TEST(CliApp, LoadsDataAndExitsOnZero) {
    int code = -1;
    const std::string out = session("0\n", &code);
    EXPECT_EQ(code, 0);
    EXPECT_TRUE(has(out, "Loaded 8 products, 4 customers and 17 rules")) << out;
    EXPECT_TRUE(has(out, "Goodbye."));
}

TEST(CliApp, EndOfInputExitsCleanly) {
    int code = -1;
    const std::string out = session("1\n", &code); // no "0"
    EXPECT_EQ(code, 0);
    EXPECT_TRUE(has(out, "Goodbye."));
}

TEST(CliApp, EndOfInputInsideSubMenuExitsCleanly) {
    int code = -1;
    const std::string out = session("2\na\nE1\n", &code); // stops at the quantity prompt
    EXPECT_EQ(code, 0);
    EXPECT_TRUE(has(out, "Goodbye."));
}

TEST(CliApp, ViewCatalogListsProducts) {
    const std::string out = session("1\n0\n");
    for (const char* name : {"Smartphone", "Cotton Kurta", "Study Chair", "Toned Milk 1L"}) {
        EXPECT_TRUE(has(out, name)) << name;
    }
    EXPECT_TRUE(has(out, "₹15,000.00"));
}

TEST(CliApp, IgnoresByteOrderMarkFromPipedInput) {
    const std::string out = session("\xEF\xBB\xBF" "1\n0\n");
    EXPECT_FALSE(has(out, "Unknown option"));
    EXPECT_TRUE(has(out, "Smartphone"));
}

TEST(CliApp, UnknownOptionReprompts) {
    const std::string out = session("42\n0\n");
    EXPECT_TRUE(has(out, "Unknown option \"42\""));
    EXPECT_EQ(count(out, "Choose an option:"), 2u);
}

TEST(CliApp, BuildCartAddsRemovesAndValidates) {
    std::istringstream in("2\na\ne1\n2\na\nC1\n0\na\nNOPE\nr\nE1\nb\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData());
    app.run();
    const std::string text = out.str();
    EXPECT_TRUE(has(text, "Added 2 x Smartphone."));
    EXPECT_TRUE(has(text, "Quantity must be a whole number from 1 to 1000."));
    EXPECT_TRUE(has(text, "No product with id \"NOPE\""));
    EXPECT_TRUE(has(text, "Removed."));
    EXPECT_TRUE(app.cart().empty());
}

TEST(CliApp, ClearEmptiesCart) {
    std::istringstream in("2\na\nE1\n\nc\nb\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData());
    app.run();
    EXPECT_TRUE(app.cart().empty());
}

TEST(CliApp, PickCustomerByNumberOrId) {
    std::istringstream in("3\n4\n3\ncu2\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData());
    app.run();
    EXPECT_TRUE(has(out.str(), "Customer is now Meera Iyer (Platinum)."));
    EXPECT_EQ(app.customer().id(), "CU2");
}

TEST(CliApp, ActionsNeedACart) {
    const std::string out = session("5\n7\n10\n0\n");
    EXPECT_EQ(count(out, "Your cart is empty"), 3u);
}

TEST(CliApp, TraceBeforeCheckoutSaysSo) {
    EXPECT_TRUE(has(session("6\n0\n"), "No checkout yet"));
}

TEST(CliApp, CheckoutPrintsInvoiceAndTrace) {
    const std::string out = session(kFestiveOrder + "5\n2\n6\n0\n");
    EXPECT_TRUE(has(out, "TAX INVOICE")) << out;
    EXPECT_TRUE(has(out, "Priya Sharma"));
    EXPECT_TRUE(has(out, "FESTIVE500"));
    EXPECT_TRUE(has(out, "Policy BestDiscount")); // the chosen policy is remembered
    EXPECT_TRUE(has(out, "Order placed."));
}

TEST(CliApp, OneTimeCouponIsRefusedOnSecondCheckout) {
    const std::string out =
        session("2\na\nC1\n1\nb\n4\nNEWUSER15\n-\n5\n\n5\n\n6\n0\n");
    EXPECT_TRUE(has(out, "usage limit of 1 reached")) << out;
}

TEST(CliApp, CouponAndBankCanBeKeptOrRemoved) {
    const std::string out = session("4\nSAVE\nSBI\n4\n\n-\n0\n");
    EXPECT_TRUE(has(out, "Coupon: SAVE, bank: SBI."));
    EXPECT_TRUE(has(out, "Coupon: SAVE, bank: none."));
}

TEST(CliApp, WhatIfOptimizerAndConflictReportRun) {
    const std::string out = session(kFestiveOrder + "7\n10\n8\n0\n");
    EXPECT_TRUE(has(out, "What-if: grand total")) << out;
    EXPECT_TRUE(has(out, "* Gold"));
    EXPECT_TRUE(has(out, "Best offer combination - tried"));
    EXPECT_TRUE(has(out, "Your current policy (Stacking) gives"));
    EXPECT_TRUE(has(out, "Rule check (as of 2026-10-30)"));
}

TEST(CliApp, ReloadPicksUpChangedRules) {
    const DataCopy data;
    std::istringstream in("9\n0\n");
    std::ostringstream out;
    CliApp app(in, out, AppConfig{data.dir(), kDay});
    EXPECT_EQ(app.ruleCount(), 17u);

    data.overwrite("rules.txt", "RULE ONLY: IF category == Grocery THEN 5% OFF\n");
    app.run();
    EXPECT_EQ(app.ruleCount(), 14u); // 13 from JSON + 1 from text
    EXPECT_TRUE(has(out.str(), "Reloaded 14 rules."));
}

TEST(CliApp, FailedReloadKeepsOldRules) {
    const DataCopy data;
    std::istringstream in("9\n0\n");
    std::ostringstream out;
    CliApp app(in, out, AppConfig{data.dir(), kDay});

    data.overwrite("rules.json", "{ broken");
    app.run();
    EXPECT_EQ(app.ruleCount(), 17u);
    EXPECT_TRUE(has(out.str(), "Could not reload rules, keeping the 17 rules already loaded."));
}

TEST(CliApp, BadDataFolderFailsAtStartup) {
    std::istringstream in;
    std::ostringstream out;
    EXPECT_THROW(CliApp(in, out, AppConfig{"no/such/folder", kDay}), infrastructure::ConfigError);
}

namespace {
/// A policy the CLI has never heard of: it refuses every offer.
class NoOffersPolicy final : public engine::ConflictResolver {
public:
    std::string name() const override { return "NoOffers"; }
    std::vector<engine::Verdict> resolve(const std::vector<engine::Candidate>& c) const override {
        return std::vector<engine::Verdict>(c.size(), engine::Verdict{false, "policy allows none"});
    }
};
} // namespace

TEST(CliApp, PolicyMenuIsBuiltFromInjectedPolicies) {
    // Open/Closed: a new policy shows up in the menus with no change to CliApp.
    auto policies = application::standardConflictPolicies();
    policies.push_back(std::make_unique<NoOffersPolicy>());
    std::istringstream in(kFestiveOrder + "5\n4\n7\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData(), std::move(policies));
    app.run();
    const std::string text = out.str();
    EXPECT_TRUE(has(text, "1 Stacking, 2 BestDiscount, 3 Priority, 4 NoOffers")) << text;
    EXPECT_EQ(app.policy().name(), "NoOffers");
    EXPECT_TRUE(has(text, "Policy NoOffers"));
    EXPECT_TRUE(has(text, "NoOffers")); // what-if table has a column for it
}

TEST(CliApp, RejectsEmptyOrNullPolicyList) {
    std::istringstream in;
    std::ostringstream out;
    EXPECT_THROW(CliApp(in, out, projectData(), {}), std::invalid_argument);
    application::ConflictPolicies withNull;
    withNull.push_back(nullptr);
    EXPECT_THROW(CliApp(in, out, projectData(), std::move(withNull)), std::invalid_argument);
}

namespace {
/// Like session(), but with an order time so happy-hour offers can apply.
std::string sessionAt(const std::string& input, domain::TimeOfDay time) {
    std::istringstream in(input);
    std::ostringstream out;
    CliApp app(in, out, AppConfig{DATA_DIR, kDay, time});
    app.run();
    return out.str();
}
} // namespace

TEST(CliApp, SmartSuggestionsShowWhatACartUnlocks) {
    // Two kurtas + FESTIVE500: a third kurta unlocks buy-3 and the coupon threshold.
    const std::string out = session("2\na\nC1\n2\nb\n4\nFESTIVE500\n\n11\n0\n");
    EXPECT_TRUE(has(out, "Smart suggestions (policy Stacking)")) << out;
    EXPECT_TRUE(has(out, "1. Add 1 x Cotton Kurta (C1)"));
    EXPECT_TRUE(has(out, "FESTIVE500"));
    EXPECT_TRUE(has(out, "11  Smart upsell suggestions"));
}

TEST(CliApp, SmartSuggestionsNeedACart) {
    EXPECT_TRUE(has(session("11\n0\n"), "Your cart is empty"));
}

TEST(CliApp, HappyHourOfferDependsOnOrderTime) {
    const std::string order = "2\na\nE2\n1\nb\n5\n1\n6\n0\n";
    const std::string evening = sessionAt(order, domain::TimeOfDay(19, 30));
    EXPECT_TRUE(has(evening, "Date 2026-10-30 19:30")) << evening;
    EXPECT_TRUE(has(evening, "HAPPY_HOUR_EARBUDS applied"));
    const std::string afternoon = sessionAt(order, domain::TimeOfDay(14, 0));
    EXPECT_TRUE(has(afternoon, "only between 18:00 and 21:00 (ordered at 14:00)")) << afternoon;
}

TEST(CliApp, WelcomeCouponOnlyOnFirstOrder) {
    // Asha (CU1) has no orders yet: the first checkout gets WELCOME200, the second does not.
    const std::string out = session("2\na\nC1\n1\nb\n4\nWELCOME200\n\n5\n1\n5\n1\n6\n0\n");
    EXPECT_TRUE(has(out, "COUPON_WELCOME200 applied")) << out;
    EXPECT_TRUE(has(out, "first order only; Asha Rao has placed 1 order"));
}

namespace {
/// A logger that writes into @p log, for checking what the CLI records.
infrastructure::Logger loggerInto(std::ostringstream& log) {
    infrastructure::Logger logger;
    logger.addSink(std::make_unique<infrastructure::StreamLogSink>(log));
    return logger;
}
} // namespace

TEST(CliApp, LogsStartupOrdersAndSessionEnd) {
    std::ostringstream log;
    std::istringstream in(kFestiveOrder + "5\n1\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData(), application::standardConflictPolicies(), loggerInto(log));
    app.run();
    const std::string text = log.str();
    EXPECT_TRUE(has(text, "[INFO] Loaded 8 products, 4 customers and 17 rules from")) << text;
    EXPECT_TRUE(has(text, "[INFO] Order placed by CU42 (Priya Sharma), policy Stacking: 3 lines"));
    EXPECT_TRUE(has(text, "COUPON_FESTIVE500"));
    EXPECT_TRUE(has(text, "[INFO] Session ended"));
}

TEST(CliApp, LogsAFailedReloadAsAWarning) {
    const DataCopy data;
    std::ostringstream log;
    std::istringstream in("9\n0\n");
    std::ostringstream out;
    CliApp app(in, out, AppConfig{data.dir(), kDay}, application::standardConflictPolicies(),
               loggerInto(log));
    data.overwrite("rules.json", "{ broken");
    app.run();
    EXPECT_TRUE(has(log.str(), "[WARN] Rule reload failed, kept 17 rules: invalid JSON")) << log.str();
}

TEST(CliApp, IsSilentWithoutALogger) {
    // The default logger has no sinks; the session must still run normally.
    EXPECT_TRUE(has(session("0\n"), "Goodbye."));
}

TEST(CliApp, RejectsHugeQuantitiesWithoutOverflow) {
    // Two big adds used to overflow int and give a negative bill.
    std::istringstream in("2\na\nE1\n2000000000\na\nE1\n600\na\nE1\n600\nb\n0\n");
    std::ostringstream out;
    CliApp app(in, out, projectData());
    app.run();
    const std::string text = out.str();
    EXPECT_TRUE(has(text, "Quantity must be a whole number from 1 to 1000.")) << text;
    EXPECT_TRUE(has(text, "Not added: A cart line can hold at most 1000 units."));
    ASSERT_EQ(app.cart().items().size(), 1u);
    EXPECT_EQ(app.cart().items()[0].quantity(), 600);
}

TEST(CliApp, ExpiredProductsAreMarkedAndCannotBeSold) {
    // Toned Milk (G1) expires on 2026-11-01; the shop is open on 2026-11-03.
    std::istringstream in("1\n2\na\nG1\n1\nb\n0\n");
    std::ostringstream out;
    CliApp app(in, out, AppConfig{DATA_DIR, domain::Date(2026, 11, 3)});
    app.run();
    const std::string text = out.str();
    EXPECT_TRUE(has(text, "EXPIRED")) << text;
    EXPECT_TRUE(has(text, "Toned Milk 1L expired on 2026-11-01 and cannot be sold."));
    EXPECT_TRUE(app.cart().empty());
}
