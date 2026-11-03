#include <gtest/gtest.h>

#include <type_traits>

#include "domain/Electronics.hpp"
#include "domain/PricingContext.hpp"

using namespace domain;

TEST(PricingContext, ExposesCartCustomerAndDate) {
    const Electronics tv("E1", "TV", Money::fromRupees(20000), Money::fromRupees(30000),
                         Money::fromRupees(35000));
    Cart cart;
    cart += CartItem(tv, 1);
    const Customer customer("CU1", "Meera", MembershipTier::Platinum);

    const PricingContext ctx(cart, customer, Date(2026, 10, 28), "DIWALI10");

    EXPECT_EQ(&ctx.cart(), &cart);
    EXPECT_EQ(&ctx.customer(), &customer);
    EXPECT_EQ(ctx.orderDate(), Date(2026, 10, 28));
    ASSERT_TRUE(ctx.couponCode().has_value());
    EXPECT_EQ(*ctx.couponCode(), "DIWALI10");
    EXPECT_EQ(ctx.cart().subtotal(), Money::fromRupees(30000));
    EXPECT_EQ(ctx.customer().tier(), MembershipTier::Platinum);
}

TEST(PricingContext, CouponIsOptional) {
    const Cart cart;
    const Customer customer("CU2", "Kiran");
    const PricingContext ctx(cart, customer, Date(2026, 1, 1));
    EXPECT_FALSE(ctx.couponCode().has_value());
}

TEST(PricingContext, CannotBindToTemporaries) {
    EXPECT_FALSE((std::is_constructible_v<PricingContext, Cart&&, const Customer&, Date>));
    EXPECT_FALSE((std::is_constructible_v<PricingContext, const Cart&, Customer&&, Date>));
    EXPECT_TRUE((std::is_constructible_v<PricingContext, const Cart&, const Customer&, Date>));
}

TEST(PricingContext, PaymentBankIsOptional) {
    const Cart cart;
    const Customer customer("CU3", "Dev");
    const PricingContext without(cart, customer, Date(2026, 1, 1));
    EXPECT_FALSE(without.paymentBank().has_value());
    const PricingContext with(cart, customer, Date(2026, 1, 1), std::nullopt, "HDFC");
    ASSERT_TRUE(with.paymentBank().has_value());
    EXPECT_EQ(*with.paymentBank(), "HDFC");
}

TEST(PricingContext, OrderTimeIsOptional) {
    const Cart cart;
    const Customer customer("CU4", "Isha");
    const PricingContext without(cart, customer, Date(2026, 1, 1));
    EXPECT_FALSE(without.orderTime().has_value());
    const PricingContext with(cart, customer, Date(2026, 1, 1), std::nullopt, std::nullopt,
                              domain::TimeOfDay(18, 30));
    ASSERT_TRUE(with.orderTime().has_value());
    EXPECT_EQ(*with.orderTime(), domain::TimeOfDay(18, 30));
}
