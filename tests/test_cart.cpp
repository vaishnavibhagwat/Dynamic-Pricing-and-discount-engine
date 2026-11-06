#include <gtest/gtest.h>

#include <stdexcept>

#include "domain/Cart.hpp"
#include "domain/Clothing.hpp"
#include "domain/Electronics.hpp"

using namespace domain;

namespace {
class CartTest : public ::testing::Test {
protected:
    Electronics earbuds{"E1", "Earbuds", Money::fromRupees(900), Money::fromRupees(1499),
                        Money::fromRupees(1999)};
    Clothing tshirt{"C1", "T-Shirt", Money::fromRupees(200), Money::fromRupees(399),
                    Money::fromRupees(499), Size::L, Season::Summer};
};
} // namespace

TEST_F(CartTest, CartItemTotals) {
    const CartItem item(earbuds, 3);
    EXPECT_EQ(&item.product(), &earbuds);
    EXPECT_EQ(item.quantity(), 3);
    EXPECT_EQ(item.lineTotal(), Money::fromRupees(4497));
    EXPECT_EQ(item.mrpTotal(), Money::fromRupees(5997));
}

TEST_F(CartTest, CartItemRejectsBadQuantity) {
    EXPECT_THROW(CartItem(earbuds, 0), std::invalid_argument);
    EXPECT_THROW(CartItem(earbuds, -2), std::invalid_argument);
    CartItem item(earbuds, 1);
    EXPECT_THROW(item.increaseQuantity(0), std::invalid_argument);
    item.increaseQuantity(2);
    EXPECT_EQ(item.quantity(), 3);
}

TEST_F(CartTest, StartsEmpty) {
    const Cart cart;
    EXPECT_TRUE(cart.empty());
    EXPECT_EQ(cart.subtotal(), Money::zero());
    EXPECT_EQ(cart.totalQuantity(), 0);
    EXPECT_FALSE(cart.contains("E1"));
}

TEST_F(CartTest, PlusEqualsAddsLines) {
    Cart cart;
    cart += CartItem(earbuds, 1);
    cart += CartItem(tshirt, 2);
    ASSERT_EQ(cart.items().size(), 2u);
    EXPECT_TRUE(cart.contains("E1"));
    EXPECT_TRUE(cart.contains("C1"));
    EXPECT_FALSE(cart.contains("X9"));
    EXPECT_EQ(cart.totalQuantity(), 3);
    EXPECT_EQ(cart.subtotal(), Money::fromRupees(1499 + 2 * 399));
    EXPECT_EQ(cart.mrpTotal(), Money::fromRupees(1999 + 2 * 499));
}

TEST_F(CartTest, PlusEqualsMergesSameProduct) {
    Cart cart;
    cart += CartItem(earbuds, 1);
    cart += CartItem(earbuds, 2);
    ASSERT_EQ(cart.items().size(), 1u);
    EXPECT_EQ(cart.items().front().quantity(), 3);
    EXPECT_EQ(cart.subtotal(), Money::fromRupees(3 * 1499));
}

TEST_F(CartTest, PlusEqualsIsChainable) {
    Cart cart;
    (cart += CartItem(earbuds, 1)) += CartItem(tshirt, 1);
    EXPECT_EQ(cart.items().size(), 2u);
}

TEST_F(CartTest, RemoveDropsOnlyThatLine) {
    Cart cart;
    cart += CartItem(earbuds, 1);
    cart += CartItem(tshirt, 2);
    EXPECT_TRUE(cart.remove("E1"));
    EXPECT_FALSE(cart.contains("E1"));
    ASSERT_EQ(cart.items().size(), 1u);
    EXPECT_EQ(cart.items()[0].product().id(), "C1");
    EXPECT_EQ(cart.totalQuantity(), 2);
}

TEST_F(CartTest, RemoveUnknownProductReturnsFalse) {
    Cart cart;
    cart += CartItem(earbuds, 1);
    EXPECT_FALSE(cart.remove("NOPE"));
    EXPECT_EQ(cart.items().size(), 1u);
}

TEST_F(CartTest, QuantityIsCappedPerLine) {
    EXPECT_NO_THROW(CartItem(earbuds, CartItem::kMaxQuantity));
    EXPECT_THROW(CartItem(earbuds, CartItem::kMaxQuantity + 1), std::invalid_argument);

    CartItem item(earbuds, 999);
    EXPECT_THROW(item.increaseQuantity(2), std::invalid_argument);
    EXPECT_EQ(item.quantity(), 999); // unchanged after the failed call
    item.increaseQuantity(1);
    EXPECT_EQ(item.quantity(), CartItem::kMaxQuantity);
    // Huge values must be refused, not wrap around to a negative number.
    EXPECT_THROW(item.increaseQuantity(2000000000), std::invalid_argument);
}

TEST_F(CartTest, MergingPastTheCapLeavesCartUnchanged) {
    Cart cart;
    cart += CartItem(earbuds, 600);
    EXPECT_THROW(cart += CartItem(earbuds, 600), std::invalid_argument);
    ASSERT_EQ(cart.items().size(), 1u);
    EXPECT_EQ(cart.items()[0].quantity(), 600);
}
