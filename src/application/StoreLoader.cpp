#include "application/StoreLoader.hpp"

#include <utility>

#include "infrastructure/CatalogLoader.hpp"
#include "infrastructure/CustomerLoader.hpp"
#include "infrastructure/Exceptions.hpp"
#include "language/RuleScriptLoader.hpp"
#include "rules/RuleFactory.hpp"

namespace application {

StoreLoader::StoreLoader(std::string dataDir) : dataDir_(std::move(dataDir)) {}

std::string StoreLoader::path(const char* file) const {
    return dataDir_ + "/" + file;
}

rules::Repository<domain::Product> StoreLoader::loadCatalog() const {
    rules::Repository<domain::Product> catalog;
    for (auto& product : infrastructure::CatalogLoader().loadFile(path("catalog.json"))) {
        catalog.add(std::move(product));
    }
    return catalog;
}

std::vector<domain::Customer> StoreLoader::loadCustomers() const {
    auto customers = infrastructure::CustomerLoader().loadFile(path("customers.json"));
    if (customers.empty()) throw infrastructure::ConfigError("customers.json has no customers");
    return customers;
}

rules::Repository<rules::DiscountRule> StoreLoader::loadRules() const {
    auto repo = rules::RuleFactory().loadFile(path("rules.json"));
    language::RuleScriptLoader().loadInto(path("rules.txt"), repo);
    return repo;
}

} // namespace application
