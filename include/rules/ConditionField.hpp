#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "domain/CartItem.hpp"
#include "domain/PricingContext.hpp"
#include "domain/Product.hpp"
#include "rules/ConditionValue.hpp"

namespace rules {

/**
 * @brief Something a rule condition can look at, such as `qty` or `tier`.
 *
 * OOP ideas: encapsulation and the Strategy idea - each field carries its
 * own reader function, so a ComparisonNode asks "what is your value for
 * this cart line?" without knowing which field it holds.
 *
 * A field may also be a pure product property (category, price); then it
 * can be read from a Product alone, which lets the rule validator check
 * statically whether a rule could ever match a catalogue product.
 */
class ConditionField {
public:
    using LineReader =
        std::function<ConditionValue(const domain::CartItem&, const domain::PricingContext&)>;
    using ProductReader = std::function<ConditionValue(const domain::Product&)>;

    /// @throws std::invalid_argument if name is empty or reader is empty.
    ConditionField(std::string name, ValueType type, LineReader reader,
                   ProductReader productReader = {});

    const std::string& name() const noexcept { return name_; }
    ValueType type() const noexcept { return type_; }

    /// Value of this field for one cart line of the order.
    ConditionValue read(const domain::CartItem& item, const domain::PricingContext& ctx) const;
    /// Value from the product alone, or nullopt if the field needs the cart/customer.
    std::optional<ConditionValue> readProduct(const domain::Product& product) const;

private:
    std::string name_;
    ValueType type_;
    LineReader reader_;
    ProductReader productReader_;
};

/**
 * @brief The set of fields the rule language knows, looked up by name.
 *
 * OOP ideas: Open/Closed Principle (registry) - a new field such as
 * `brand` is added with registerField(); the parser and the condition
 * nodes do not change. Same idea as RuleFactory::registerType().
 *
 * Built-ins: category, product, qty, price, tier, cart_total, cart_qty, date.
 */
class FieldCatalog {
public:
    /// Registers the built-in fields.
    FieldCatalog();

    /// @throws std::invalid_argument if a field with this name already exists.
    void registerField(ConditionField field);
    /// Case-insensitive lookup. @return nullptr if unknown.
    const ConditionField* find(const std::string& name) const;
    /// Field names in alphabetical order.
    std::vector<std::string> names() const;

private:
    std::map<std::string, ConditionField> fields_; // key: lower-case name
};

} // namespace rules
