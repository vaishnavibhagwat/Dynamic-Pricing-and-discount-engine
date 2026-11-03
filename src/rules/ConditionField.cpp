#include "rules/ConditionField.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace rules {

namespace {

std::string lower(const std::string& s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

/// Field that depends only on the product: the line reader just forwards to it.
ConditionField productField(std::string name, ValueType type,
                            ConditionField::ProductReader reader) {
    auto onLine = [reader](const domain::CartItem& item, const domain::PricingContext&) {
        return reader(item.product());
    };
    return ConditionField(std::move(name), type, onLine, reader);
}

} // namespace

ConditionField::ConditionField(std::string name, ValueType type, LineReader reader,
                               ProductReader productReader)
    : name_(std::move(name)), type_(type), reader_(std::move(reader)),
      productReader_(std::move(productReader)) {
    if (name_.empty()) throw std::invalid_argument("ConditionField: name must not be empty");
    if (!reader_) throw std::invalid_argument("ConditionField: reader must not be empty");
}

ConditionValue ConditionField::read(const domain::CartItem& item,
                                    const domain::PricingContext& ctx) const {
    return reader_(item, ctx);
}

std::optional<ConditionValue> ConditionField::readProduct(const domain::Product& product) const {
    if (!productReader_) return std::nullopt;
    return productReader_(product);
}

FieldCatalog::FieldCatalog() {
    using domain::CartItem;
    using domain::PricingContext;
    using domain::Product;

    registerField(productField("category", ValueType::Text, [](const Product& p) {
        return ConditionValue::text(p.category());
    }));
    registerField(productField("product", ValueType::Text, [](const Product& p) {
        return ConditionValue::text(p.id());
    }));
    registerField(productField("price", ValueType::Money, [](const Product& p) {
        return ConditionValue::money(p.sellingPrice());
    }));
    registerField(ConditionField("qty", ValueType::Number,
                                 [](const CartItem& item, const PricingContext&) {
                                     return ConditionValue::number(item.quantity());
                                 }));
    registerField(ConditionField("tier", ValueType::Tier,
                                 [](const CartItem&, const PricingContext& ctx) {
                                     return ConditionValue::tier(ctx.customer().tier());
                                 }));
    registerField(ConditionField("cart_total", ValueType::Money,
                                 [](const CartItem&, const PricingContext& ctx) {
                                     return ConditionValue::money(ctx.cart().subtotal());
                                 }));
    registerField(ConditionField("cart_qty", ValueType::Number,
                                 [](const CartItem&, const PricingContext& ctx) {
                                     return ConditionValue::number(ctx.cart().totalQuantity());
                                 }));
    registerField(ConditionField("date", ValueType::Date,
                                 [](const CartItem&, const PricingContext& ctx) {
                                     return ConditionValue::date(ctx.orderDate());
                                 }));
}

void FieldCatalog::registerField(ConditionField field) {
    std::string key = lower(field.name());
    if (fields_.count(key) > 0) {
        throw std::invalid_argument("FieldCatalog: field already registered: " + field.name());
    }
    fields_.emplace(std::move(key), std::move(field));
}

const ConditionField* FieldCatalog::find(const std::string& name) const {
    const auto it = fields_.find(lower(name));
    return it == fields_.end() ? nullptr : &it->second;
}

std::vector<std::string> FieldCatalog::names() const {
    std::vector<std::string> out;
    for (const auto& entry : fields_) out.push_back(entry.second.name());
    return out;
}

} // namespace rules
