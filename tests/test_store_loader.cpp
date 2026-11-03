#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "application/StoreLoader.hpp"
#include "infrastructure/Exceptions.hpp"

using application::StoreLoader;
using infrastructure::ConfigError;

TEST(StoreLoader, LoadsTheProjectData) {
    const StoreLoader store(DATA_DIR);
    EXPECT_EQ(store.dataDir(), DATA_DIR);
    EXPECT_EQ(store.loadCatalog().size(), 8u);
    EXPECT_EQ(store.loadCustomers().size(), 4u);
    // 13 rules from rules.json plus 4 from rules.txt.
    EXPECT_EQ(store.loadRules().size(), 17u);
}

TEST(StoreLoader, EachLoadReturnsAFreshRepository) {
    const StoreLoader store(DATA_DIR);
    const auto first = store.loadRules();
    const auto second = store.loadRules();
    ASSERT_FALSE(first.empty());
    EXPECT_NE(first.all().front(), second.all().front()); // separate objects, not shared
}

TEST(StoreLoader, MissingFolderThrowsConfigError) {
    const StoreLoader store("no/such/folder");
    EXPECT_THROW(store.loadCatalog(), ConfigError);
    EXPECT_THROW(store.loadCustomers(), ConfigError);
    EXPECT_THROW(store.loadRules(), ConfigError);
}

TEST(StoreLoader, EmptyCustomerListIsAnError) {
    const std::string dir = std::string(TEST_SCRATCH_DIR) + "/store_no_customers";
    std::filesystem::create_directories(dir);
    {
        std::ofstream out(dir + "/customers.json");
        out << R"({ "customers": [] })";
    }
    try {
        StoreLoader(dir).loadCustomers();
        FAIL() << "expected ConfigError";
    } catch (const ConfigError& e) {
        EXPECT_EQ(std::string(e.what()), "customers.json has no customers");
    }
    std::filesystem::remove_all(dir);
}
