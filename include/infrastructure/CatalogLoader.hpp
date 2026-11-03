#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "domain/Product.hpp"
#include "infrastructure/JsonRecord.hpp"

namespace infrastructure {

/**
 * @brief Builds the product catalog from JSON (data/catalog.json).
 *
 * OOP ideas: Factory pattern and the Open/Closed Principle - each "type"
 * string maps to a creator in a registry, so a new Product subclass is added
 * with registerType() instead of editing this class. Products are returned
 * as std::unique_ptr<Product>, so ownership of the polymorphic objects is
 * explicit (RAII).
 *
 * Document: { "products": [ { "id", "type", "name", "cost", "price", "mrp",
 * ...type-specific fields } ] }. Prices are whole rupees. Type fields:
 * clothing - "size" (XS..XXL), "season"; furniture - "assembly_required";
 * grocery - "expiry" ("YYYY-MM-DD"). A bare array is also accepted.
 */
class CatalogLoader {
public:
    using Creator = std::function<std::unique_ptr<domain::Product>(const JsonRecord&)>;

    /// Registers electronics, clothing, furniture and grocery.
    CatalogLoader();

    /// @throws std::invalid_argument if type is empty, already registered, or creator is empty.
    void registerType(const std::string& type, Creator creator);
    bool supports(const std::string& type) const;

    /// @throws ConfigError on a bad document, bad product or duplicate id.
    std::vector<std::unique_ptr<domain::Product>> fromJson(const nlohmann::json& document) const;
    /// @throws ConfigError if the file cannot be read or holds a bad product.
    std::vector<std::unique_ptr<domain::Product>> loadFile(const std::string& path) const;

private:
    std::unique_ptr<domain::Product> create(const nlohmann::json& entry) const;

    std::map<std::string, Creator> creators_;
};

} // namespace infrastructure
