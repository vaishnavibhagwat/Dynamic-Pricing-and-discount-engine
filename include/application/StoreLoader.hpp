#pragma once

#include <string>
#include <vector>

#include "domain/Customer.hpp"
#include "domain/Product.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace application {

/**
 * @brief Loads the shop's data - catalog, customers and rules - from one folder.
 *
 * Files read: catalog.json, customers.json, rules.json and rules.txt (text
 * rules are added after the JSON ones).
 *
 * OOP ideas: Single Responsibility - this class knows the file names and the
 * loading order, so the CLI only deals with menus. Facade - it hides the
 * four infrastructure/rule loaders behind three calls.
 */
class StoreLoader {
public:
    explicit StoreLoader(std::string dataDir);

    const std::string& dataDir() const noexcept { return dataDir_; }

    /// @throws infrastructure::ConfigError if catalog.json is missing or bad.
    rules::Repository<domain::Product> loadCatalog() const;
    /// @throws infrastructure::ConfigError if customers.json is missing, bad or empty.
    std::vector<domain::Customer> loadCustomers() const;
    /// Reads rules.json, then rules.txt, into one repository.
    /// @throws infrastructure::ConfigError if either file is missing or bad.
    rules::Repository<rules::DiscountRule> loadRules() const;

private:
    std::string path(const char* file) const;

    std::string dataDir_;
};

} // namespace application
