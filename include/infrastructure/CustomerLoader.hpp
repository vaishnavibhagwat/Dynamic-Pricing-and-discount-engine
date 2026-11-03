#pragma once

#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "domain/Customer.hpp"

namespace infrastructure {

/**
 * @brief Reads the customer list from JSON (data/customers.json).
 *
 * OOP ideas: Single Responsibility (file -> Customer objects, nothing
 * else) and layering - the CLI asks for customers without knowing the file
 * format, and Customer stays free of any JSON code.
 *
 * Document: { "customers": [ { "id", "name", "tier"? } ] }. "tier" is
 * Regular / Silver / Gold / Platinum (any case), default Regular.
 */
class CustomerLoader {
public:
    /// @throws ConfigError on a bad document, bad customer or duplicate id.
    std::vector<domain::Customer> fromJson(const nlohmann::json& document) const;
    /// @throws ConfigError if the file cannot be read or holds a bad customer.
    std::vector<domain::Customer> loadFile(const std::string& path) const;
};

} // namespace infrastructure
