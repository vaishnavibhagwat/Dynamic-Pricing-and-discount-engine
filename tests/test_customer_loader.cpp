#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include "infrastructure/CustomerLoader.hpp"
#include "infrastructure/Exceptions.hpp"

using domain::MembershipTier;
using infrastructure::ConfigError;
using infrastructure::CustomerLoader;
using nlohmann::json;

namespace {

std::string errorOf(const std::string& text) {
    try {
        CustomerLoader().fromJson(json::parse(text));
    } catch (const ConfigError& e) {
        return e.what();
    }
    return "";
}

} // namespace

TEST(CustomerLoader, ReadsCustomersInOrder) {
    const auto customers = CustomerLoader().fromJson(json::parse(R"({"customers":[
        {"id":"CU1","name":"Asha","tier":"Gold"},
        {"id":"CU2","name":"Ravi","tier":"platinum"},
        {"id":"CU3","name":"Meera"}
    ]})"));
    ASSERT_EQ(customers.size(), 3u);
    EXPECT_EQ(customers[0].id(), "CU1");
    EXPECT_EQ(customers[0].name(), "Asha");
    EXPECT_EQ(customers[0].tier(), MembershipTier::Gold);
    EXPECT_EQ(customers[1].tier(), MembershipTier::Platinum); // any case
    EXPECT_EQ(customers[2].tier(), MembershipTier::Regular);  // default
}

TEST(CustomerLoader, AcceptsBareArray) {
    EXPECT_EQ(CustomerLoader().fromJson(json::parse(R"([{"id":"A","name":"B"}])")).size(), 1u);
}

TEST(CustomerLoader, ReportsBadInput) {
    EXPECT_EQ(errorOf(R"({"people":[]})"), "customer file has no \"customers\" array");
    EXPECT_EQ(errorOf(R"({"customers":{}})"), "\"customers\" must be an array");
    EXPECT_EQ(errorOf(R"([{"id":"CU1","name":"Asha","tier":"Diamond"}])"),
              "customer 'CU1': unknown membership tier \"Diamond\"");
    EXPECT_EQ(errorOf(R"([{"id":"CU1"}])"), "customer 'CU1': missing field \"name\"");
    EXPECT_EQ(errorOf(R"([{"id":"CU1","name":""}])"),
              "customer 'CU1': field \"name\" must not be empty");
    EXPECT_EQ(errorOf(R"([{"name":"Asha"}])"),
              "customer entry is missing a non-empty string \"id\"");
}

TEST(CustomerLoader, RejectsDuplicateIds) {
    EXPECT_EQ(errorOf(R"([{"id":"CU1","name":"A"},{"id":"CU1","name":"B"}])"),
              "duplicate customer id 'CU1'");
}

TEST(CustomerLoader, LoadsProjectCustomerFile) {
    const auto customers = CustomerLoader().loadFile(std::string(DATA_DIR) + "/customers.json");
    ASSERT_EQ(customers.size(), 4u);
    // One customer per tier, lowest first.
    for (std::size_t i = 0; i < customers.size(); ++i) {
        EXPECT_EQ(customers[i].tier(), domain::allMembershipTiers()[i]);
    }
}

TEST(CustomerLoader, MissingFileIsConfigError) {
    EXPECT_THROW(CustomerLoader().loadFile("no/such/customers.json"), ConfigError);
}

TEST(CustomerLoader, ReadsOptionalOrderHistory) {
    const auto customers = CustomerLoader().fromJson(json::parse(R"([
        {"id":"CU1","name":"Asha"},
        {"id":"CU2","name":"Ravi","orders_placed":4}])"));
    ASSERT_EQ(customers.size(), 2u);
    EXPECT_EQ(customers[0].ordersPlaced(), 0);
    EXPECT_EQ(customers[1].ordersPlaced(), 4);
    EXPECT_EQ(errorOf(R"([{"id":"C","name":"X","orders_placed":-2}])"),
              "customer 'C': field \"orders_placed\" must not be negative");
    EXPECT_EQ(errorOf(R"([{"id":"C","name":"X","orders_placed":"many"}])"),
              "customer 'C': field \"orders_placed\" must be a whole number");
}
