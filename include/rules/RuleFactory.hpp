#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "domain/Date.hpp"
#include "domain/Money.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/ItemFilter.hpp"
#include "rules/Repository.hpp"

namespace rules {

/**
 * @brief Typed, error-checked reader for one rule's JSON object.
 *
 * OOP ideas: Single Responsibility (only field access + validation) and
 * reuse - built-in and custom rule creators share it. Every error is a
 * infrastructure::ConfigError naming the rule id and field.
 */
class RuleSpec {
public:
    /// @throws infrastructure::ConfigError if @p json is not an object.
    explicit RuleSpec(const nlohmann::json& json);
    explicit RuleSpec(nlohmann::json&&) = delete; // would dangle

    const std::string& id() const noexcept { return id_; }
    std::string name() const;
    int priority() const;
    /// Optional, default true.
    bool stackable() const;

    bool has(const std::string& key) const;
    std::string requireString(const std::string& key) const;
    int requireInt(const std::string& key) const;
    std::optional<int> optionalInt(const std::string& key) const;
    /// Whole rupees in JSON, e.g. "cap": 1500.
    domain::Money requireRupees(const std::string& key) const;
    std::optional<domain::Money> optionalRupees(const std::string& key) const;
    /// "YYYY-MM-DD"
    domain::Date requireDate(const std::string& key) const;
    /// Optional "category" and "product_ids" fields.
    ItemFilter itemFilter() const;
    const nlohmann::json& raw() const noexcept { return json_; }

    /// Builds a ConfigError whose message names this rule.
    [[noreturn]] void fail(const std::string& message) const;

private:
    const nlohmann::json& field(const std::string& key) const;

    const nlohmann::json& json_;
    std::string id_;
};

/**
 * @brief Builds DiscountRule objects from JSON (Factory pattern).
 *
 * OOP ideas: Factory pattern and the Open/Closed Principle - each "type"
 * string maps to a creator function in a registry, so a new offer type is
 * added with registerType() instead of editing this class. Returns
 * std::unique_ptr so ownership is explicit (RAII).
 *
 * JSON document: { "rules": [ { "id", "type", "name", "priority",
 * "stackable"?, ...type-specific fields, "decorators"? } ] }. A bare array is
 * also accepted.
 *
 * Optional "decorators" wrap the offer, innermost first, using a second
 * registry (Decorator pattern), e.g.
 * `"decorators": [ {"type": "happy_hour", "from": "18:00", "to": "21:00"},
 *                  {"type": "cap", "max": 500}, {"type": "first_order"} ]`.
 */
class RuleFactory {
public:
    using Creator = std::function<std::unique_ptr<DiscountRule>(const RuleSpec&)>;
    /// Wraps an already built rule; the spec holds the decorator's own fields.
    using DecoratorCreator =
        std::function<std::unique_ptr<DiscountRule>(std::unique_ptr<DiscountRule>, const RuleSpec&)>;

    /// Registers the built-in types: percentage, quantity, seasonal, expiry,
    /// membership, coupon, bank; and decorators: cap, happy_hour, first_order.
    RuleFactory();

    /// @throws std::invalid_argument if type is empty, already registered, or creator is empty.
    void registerType(const std::string& type, Creator creator);
    bool supports(const std::string& type) const;
    std::vector<std::string> supportedTypes() const;

    /// @throws std::invalid_argument if type is empty, already registered, or creator is empty.
    void registerDecorator(const std::string& type, DecoratorCreator creator);
    bool supportsDecorator(const std::string& type) const;

    /// @throws infrastructure::ConfigError on unknown type or bad fields.
    std::unique_ptr<DiscountRule> create(const nlohmann::json& spec) const;
    /// @throws infrastructure::ConfigError on bad document, bad rule or duplicate id.
    Repository<DiscountRule> createAll(const nlohmann::json& document) const;
    /// @throws infrastructure::ConfigError if the file cannot be read or parsed.
    Repository<DiscountRule> loadFile(const std::string& path) const;

private:
    std::unique_ptr<DiscountRule> decorate(std::unique_ptr<DiscountRule> rule,
                                           const RuleSpec& spec) const;

    std::map<std::string, Creator> creators_;
    std::map<std::string, DecoratorCreator> decorators_;
};

} // namespace rules
