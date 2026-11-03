#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "domain/Clothing.hpp"
#include "domain/Electronics.hpp"
#include "domain/Furniture.hpp"
#include "domain/Grocery.hpp"

using namespace domain;

namespace {
Money rs(std::int64_t r) { return Money::fromRupees(r); }
}

TEST(Product, IsAbstractAndNotCopyable) {
    EXPECT_TRUE(std::is_abstract_v<Product>);
    EXPECT_TRUE(std::has_virtual_destructor_v<Product>);
    EXPECT_FALSE(std::is_copy_constructible_v<Product>);
}

TEST(Product, CommonFieldsAndMargin) {
    const Electronics phone("E1", "Phone", rs(15000), rs(19999), rs(24999));
    EXPECT_EQ(phone.id(), "E1");
    EXPECT_EQ(phone.name(), "Phone");
    EXPECT_EQ(phone.costPrice(), rs(15000));
    EXPECT_EQ(phone.sellingPrice(), rs(19999));
    EXPECT_EQ(phone.mrp(), rs(24999));
    EXPECT_EQ(phone.margin(), rs(4999));
}

TEST(Product, ConstructorValidates) {
    EXPECT_THROW(Electronics("", "X", rs(1), rs(2), rs(3)), std::invalid_argument);
    EXPECT_THROW(Electronics("E", "", rs(1), rs(2), rs(3)), std::invalid_argument);
    EXPECT_THROW(Electronics("E", "X", rs(-1), rs(2), rs(3)), std::invalid_argument);
    EXPECT_THROW(Electronics("E", "X", rs(1), rs(4), rs(3)), std::invalid_argument); // SP > MRP
    EXPECT_NO_THROW(Electronics("E", "X", rs(1), rs(3), rs(3)));                    // SP == MRP ok
}

TEST(Product, PolymorphicGstAndCategory) {
    std::vector<std::unique_ptr<Product>> products;
    products.push_back(std::make_unique<Electronics>("E1", "TV", rs(1), rs(2), rs(3)));
    products.push_back(
        std::make_unique<Grocery>("G1", "Rice", rs(1), rs(2), rs(3), Date(2026, 12, 31)));
    products.push_back(std::make_unique<Clothing>("C1", "Shirt", rs(1), rs(2), rs(3), Size::M,
                                                  Season::Summer));
    products.push_back(std::make_unique<Furniture>("F1", "Chair", rs(1), rs(2), rs(3), true));

    const int expectedGst[] = {18, 5, 12, 18};
    const char* expectedCategory[] = {"Electronics", "Grocery", "Clothing", "Furniture"};
    for (std::size_t i = 0; i < products.size(); ++i) {
        const Product& p = *products[i];
        EXPECT_EQ(p.gstRate(), expectedGst[i]);
        EXPECT_EQ(p.category(), expectedCategory[i]);
    }
}

TEST(Grocery, ExpiryDate) {
    const Grocery milk("G2", "Milk", rs(40), rs(50), rs(56), Date(2026, 11, 1));
    EXPECT_EQ(milk.expiryDate(), Date(2026, 11, 1));
    EXPECT_FALSE(milk.isExpiredOn(Date(2026, 10, 31)));
    EXPECT_FALSE(milk.isExpiredOn(Date(2026, 11, 1))); // still good on the day
    EXPECT_TRUE(milk.isExpiredOn(Date(2026, 11, 2)));
}

TEST(Clothing, SizeAndSeason) {
    const Clothing jacket("C2", "Jacket", rs(800), rs(1500), rs(2000), Size::XL, Season::Winter);
    EXPECT_EQ(jacket.size(), Size::XL);
    EXPECT_EQ(jacket.season(), Season::Winter);
    EXPECT_EQ(toString(Size::XXL), "XXL");
    EXPECT_EQ(toString(Season::Monsoon), "Monsoon");
    EXPECT_EQ(toString(Season::AllSeason), "AllSeason");
}

TEST(Furniture, AssemblyRequired) {
    const Furniture table("F2", "Table", rs(3000), rs(5000), rs(6000), false);
    EXPECT_FALSE(table.assemblyRequired());
    const Furniture wardrobe("F3", "Wardrobe", rs(9000), rs(12000), rs(15000), true);
    EXPECT_TRUE(wardrobe.assemblyRequired());
}
