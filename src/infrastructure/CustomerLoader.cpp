#include "infrastructure/CustomerLoader.hpp"

#include <set>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "infrastructure/Exceptions.hpp"
#include "infrastructure/JsonRecord.hpp"

namespace infrastructure {

using nlohmann::json;

std::vector<domain::Customer> CustomerLoader::fromJson(const json& document) const {
    const json* list = &document;
    if (document.is_object()) {
        const auto it = document.find("customers");
        if (it == document.end()) throw ConfigError("customer file has no \"customers\" array");
        list = &*it;
    }
    if (!list->is_array()) throw ConfigError("\"customers\" must be an array");

    std::vector<domain::Customer> customers;
    std::set<std::string> seen;
    for (const auto& entry : *list) {
        const JsonRecord record(entry, "customer");
        domain::MembershipTier tier = domain::MembershipTier::Regular;
        if (record.has("tier")) {
            try {
                tier = domain::membershipTierFromString(record.requireString("tier"));
            } catch (const std::invalid_argument& e) {
                record.fail(e.what());
            }
        }
        const std::string name = record.requireString("name");
        if (name.empty()) record.fail("field \"name\" must not be empty");
        if (!seen.insert(record.id()).second) {
            throw ConfigError("duplicate customer id '" + record.id() + "'");
        }
        int orders = 0;
        if (record.has("orders_placed")) {
            orders = record.requireInt("orders_placed");
            if (orders < 0) record.fail("field \"orders_placed\" must not be negative");
        }
        customers.emplace_back(record.id(), name, tier, orders);
    }
    return customers;
}

std::vector<domain::Customer> CustomerLoader::loadFile(const std::string& path) const {
    return fromJson(JsonRecord::readFile(path));
}

} // namespace infrastructure
