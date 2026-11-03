#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "rules/ConditionField.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;
using domain::MembershipTier;
using rules::CompareOp;
using rules::ConditionField;
using rules::ConditionValue;
using rules::FieldCatalog;
using rules::ValueType;

namespace {
/// Reads field @p name for the first line of @p order.
ConditionValue readFirstLine(const std::string& name, const Order& order) {
    const ConditionField* field = FieldCatalog().find(name);
    if (field == nullptr) throw std::runtime_error("no field " + name);
    return field->read(order.cart.items().front(), order.context());
}

bool equals(const ConditionValue& a, const ConditionValue& b) {
    return a.compare(CompareOp::Equal, b);
}
} // namespace

TEST(FieldCatalog, HasAllBuiltIns) {
    const FieldCatalog catalog;
    for (const char* name :
         {"category", "product", "qty", "price", "tier", "cart_total", "cart_qty", "date"}) {
        EXPECT_NE(catalog.find(name), nullptr) << name;
    }
    EXPECT_EQ(catalog.find("brand"), nullptr);
    EXPECT_EQ(catalog.names().size(), 8u);
}

TEST(FieldCatalog, LookupIgnoresCase) {
    const FieldCatalog catalog;
    ASSERT_NE(catalog.find("QTY"), nullptr);
    EXPECT_EQ(catalog.find("QTY")->name(), "qty");
}

TEST(ConditionField, BuiltInReadersLookAtTheRightData) {
    Order o;
    o.customer = domain::Customer("CU2", "Ravi", MembershipTier::Gold);
    o.cart += CartItem(phone(), 2);  // 30000
    o.cart += CartItem(kurta(), 3);  // 3000

    EXPECT_TRUE(equals(readFirstLine("category", o), ConditionValue::text("Electronics")));
    EXPECT_TRUE(equals(readFirstLine("product", o), ConditionValue::text("E1")));
    EXPECT_TRUE(equals(readFirstLine("qty", o), ConditionValue::number(2)));
    EXPECT_TRUE(equals(readFirstLine("price", o), ConditionValue::money(rs(15000))));
    EXPECT_TRUE(equals(readFirstLine("tier", o), ConditionValue::tier(MembershipTier::Gold)));
    EXPECT_TRUE(equals(readFirstLine("cart_total", o), ConditionValue::money(rs(33000))));
    EXPECT_TRUE(equals(readFirstLine("cart_qty", o), ConditionValue::number(5)));
    EXPECT_TRUE(equals(readFirstLine("date", o), ConditionValue::date(kToday)));
}

TEST(ConditionField, ProductFieldsCanBeReadFromProductAlone) {
    const FieldCatalog catalog;
    const auto category = catalog.find("category")->readProduct(chair());
    ASSERT_TRUE(category.has_value());
    EXPECT_TRUE(equals(*category, ConditionValue::text("Furniture")));
    EXPECT_FALSE(catalog.find("qty")->readProduct(chair()).has_value());
    EXPECT_FALSE(catalog.find("tier")->readProduct(chair()).has_value());
}

TEST(FieldCatalog, CustomFieldCanBeRegisteredWithoutEditingParser) {
    FieldCatalog catalog;
    catalog.registerField(ConditionField(
        "gst", ValueType::Number,
        [](const CartItem& item, const domain::PricingContext&) {
            return ConditionValue::number(item.product().gstRate());
        }));
    ASSERT_NE(catalog.find("gst"), nullptr);
    EXPECT_EQ(catalog.find("gst")->type(), ValueType::Number);
}

TEST(FieldCatalog, RejectsDuplicateAndBadFields) {
    FieldCatalog catalog;
    const auto reader = [](const CartItem&, const domain::PricingContext&) {
        return ConditionValue::number(0);
    };
    EXPECT_THROW(catalog.registerField(ConditionField("QTY", ValueType::Number, reader)),
                 std::invalid_argument);
    EXPECT_THROW(ConditionField("", ValueType::Number, reader), std::invalid_argument);
    EXPECT_THROW(ConditionField("x", ValueType::Number, ConditionField::LineReader()),
                 std::invalid_argument);
}
