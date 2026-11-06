#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "infrastructure/Exceptions.hpp"
#include "rules/CappedRule.hpp"
#include "application/CheckoutService.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::CheckoutService;
using engine::TraceOutcome;
using rules::CouponDiscount;
using Benefit = rules::CouponDiscount::Benefit;

namespace {

const engine::StackingResolver kStacking;

/// A rule set with a one-time coupon and an ordinary 10% offer.
rules::Repository<rules::DiscountRule> oneTimeCouponRules() {
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<CouponDiscount>("ONCE", "One-time Rs 100 off", 50, true, "ONCE",
                                              rs(0), 1, Benefit::flat(rs(100))));
    repo.add(std::make_unique<rules::PercentageDiscount>("PCT10", "10% off", 10, true, 10));
    return repo;
}

const CouponDiscount& coupon(const rules::Repository<rules::DiscountRule>& repo) {
    return dynamic_cast<const CouponDiscount&>(*repo.findById("ONCE"));
}

Order kurtaOrderWithCoupon() {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    o.coupon = "ONCE";
    return o;
}

} // namespace

TEST(CheckoutService, QuoteHasNoSideEffects) {
    auto repo = oneTimeCouponRules();
    const CheckoutService service(repo);
    const Order o = kurtaOrderWithCoupon();

    const auto first = service.quote(o.context(), kStacking);
    const auto second = service.quote(o.context(), kStacking);
    EXPECT_EQ(first.grandTotal(), second.grandTotal());
    EXPECT_EQ(first.cartDiscountTotal(), rs(100));
    EXPECT_EQ(coupon(repo).usesSoFar(), 0);
}

TEST(CheckoutService, CheckoutRecordsCouponUse) {
    auto repo = oneTimeCouponRules();
    CheckoutService service(repo);
    const Order o = kurtaOrderWithCoupon();

    const auto result = service.checkout(o.context(), kStacking);
    EXPECT_EQ(result.cartDiscountTotal(), rs(100));
    EXPECT_EQ(coupon(repo).usesSoFar(), 1);
}

TEST(CheckoutService, OneTimeCouponIsRefusedOnSecondCheckout) {
    auto repo = oneTimeCouponRules();
    CheckoutService service(repo);
    const Order o = kurtaOrderWithCoupon();

    service.checkout(o.context(), kStacking);
    const auto second = service.checkout(o.context(), kStacking);
    EXPECT_EQ(second.cartDiscountTotal(), Money::zero());
    EXPECT_EQ(coupon(repo).usesSoFar(), 1);

    bool explained = false;
    for (const auto& e : second.trace().forRule("ONCE")) {
        if (e.outcome == TraceOutcome::Rejected &&
            e.reason.find("usage limit") != std::string::npos) {
            explained = true;
        }
    }
    EXPECT_TRUE(explained);
}

TEST(CheckoutService, UnusedCouponIsNotCounted) {
    auto repo = oneTimeCouponRules();
    CheckoutService service(repo);
    Order o = kurtaOrderWithCoupon();
    o.coupon.reset();
    service.checkout(o.context(), kStacking);
    EXPECT_EQ(coupon(repo).usesSoFar(), 0);
}

TEST(CheckoutService, SeesRulesAddedAfterConstruction) {
    rules::Repository<rules::DiscountRule> repo;
    const CheckoutService service(repo);
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    EXPECT_EQ(service.quote(o.context(), kStacking).itemDiscountTotal(), Money::zero());

    repo = oneTimeCouponRules(); // e.g. "reload rules"
    EXPECT_EQ(service.quote(o.context(), kStacking).itemDiscountTotal(), rs(100));
}

TEST(CheckoutService, UsesInjectedGuards) {
    auto repo = oneTimeCouponRules();
    const CheckoutService noGuards(repo, [] {
        return std::vector<std::unique_ptr<engine::PriceGuard>>{};
    });
    const Order o = kurtaOrderWithCoupon();
    const auto r = noGuards.quote(o.context(), kStacking);
    EXPECT_EQ(r.trace().count(TraceOutcome::Passed), 0u); // no guard checks at all
}

TEST(CheckoutService, RejectsEmptyGuardFactory) {
    rules::Repository<rules::DiscountRule> repo;
    EXPECT_THROW(CheckoutService(repo, application::GuardFactory()), std::invalid_argument);
}

TEST(CheckoutService, CheckoutWithBuyerCountsTheOrder) {
    auto repo = oneTimeCouponRules();
    CheckoutService service(repo);
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    EXPECT_EQ(o.customer.ordersPlaced(), 0);
    service.checkout(o.context(), kStacking, o.customer);
    EXPECT_EQ(o.customer.ordersPlaced(), 1);

    domain::Customer someoneElse("CU9", "Other");
    EXPECT_THROW(service.checkout(o.context(), kStacking, someoneElse), std::invalid_argument);
    EXPECT_EQ(someoneElse.ordersPlaced(), 0);
}

TEST(CheckoutService, DecoratedCouponStillCountsItsUses) {
    // A one-time coupon wrapped in a decorator is still used up after one checkout.
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<rules::CappedRule>(
        std::make_unique<CouponDiscount>("ONCE", "One-time Rs 100 off", 50, true, "ONCE", rs(0), 1,
                                         Benefit::flat(rs(100))),
        rs(80)));
    CheckoutService service(repo);
    const Order o = kurtaOrderWithCoupon();
    EXPECT_EQ(service.checkout(o.context(), kStacking).cartDiscountTotal(), rs(80));
    EXPECT_EQ(service.quote(o.context(), kStacking).cartDiscountTotal(), Money::zero());
}

TEST(CheckoutService, RefusesToSellExpiredItems) {
    auto repo = oneTimeCouponRules();
    CheckoutService service(repo);
    const auto stale = makeMilk("G9", -1);
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    o.cart += domain::CartItem(stale, 1);
    o.coupon = "ONCE";
    try {
        service.checkout(o.context(), kStacking, o.customer);
        FAIL() << "expected PricingError";
    } catch (const infrastructure::PricingError& e) {
        EXPECT_NE(std::string(e.what()).find("cannot sell Milk [G9]: it expired on"),
                  std::string::npos);
    }
    // Nothing was recorded: the coupon is still unused and the order not counted.
    EXPECT_EQ(coupon(repo).usesSoFar(), 0);
    EXPECT_EQ(o.customer.ordersPlaced(), 0);
}
