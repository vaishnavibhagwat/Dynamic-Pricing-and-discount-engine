#pragma once

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "domain/Date.hpp"
#include "domain/Money.hpp"

namespace infrastructure {

/**
 * @brief Typed, error-checked reader for one JSON object in a data file
 *        (a product, a customer...).
 *
 * OOP ideas: Single Responsibility (field access and validation only) and
 * reuse through composition - CatalogLoader and CustomerLoader both use it,
 * so every data file reports errors the same way. Every error is a
 * ConfigError naming the record, e.g. "product 'E1': missing field \"mrp\"".
 */
class JsonRecord {
public:
    /// @p kind is the record type used in messages, e.g. "product".
    /// @throws ConfigError if @p json is not an object or has no non-empty string "id".
    JsonRecord(const nlohmann::json& json, std::string kind);
    JsonRecord(nlohmann::json&&, std::string) = delete; // would dangle

    /// Reads and parses a whole JSON file. @throws ConfigError if unreadable or invalid.
    static nlohmann::json readFile(const std::string& path);

    const std::string& id() const noexcept { return id_; }

    bool has(const std::string& key) const;
    std::string requireString(const std::string& key) const;
    bool requireBool(const std::string& key) const;
    int requireInt(const std::string& key) const;
    /// Whole rupees in JSON, e.g. "price": 1499.
    domain::Money requireRupees(const std::string& key) const;
    /// "YYYY-MM-DD"
    domain::Date requireDate(const std::string& key) const;

    /// @throws ConfigError whose message names this record.
    [[noreturn]] void fail(const std::string& message) const;

private:
    const nlohmann::json& field(const std::string& key) const;

    const nlohmann::json& json_;
    std::string kind_;
    std::string id_;
};

} // namespace infrastructure
