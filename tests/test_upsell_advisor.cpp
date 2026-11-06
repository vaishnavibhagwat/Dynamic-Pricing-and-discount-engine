#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "application/UpsellAdvisor.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/Repository.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::UpsellAdvisor;
using rules::CouponDiscount;

namespace {

const engine::StackingResolver kStacking;

/// No guards, so the expected numbers are simple. Clothing GST is 12%.
UpsellAdvisor advisor(std::size_t max = 3) {
    return UpsellAdvisor([] { return std::vector<std::unique_ptr<engine::PriceGuard>>{}; }, max);
}

rules::Repository<rules::DiscountRule> couponOnly() {
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<CouponDiscount>("SAVE500", "Flat 500 on 2999+", 1, true, "SAVE",
                                              rs(2999), 1, CouponDiscount::Benefit::flat(rs(500))));
    return repo;
}

std::vector<const domain::Product*> catalog() {
    return {&phone(), &kurta(), &chair()};
}

} // namespace

TEST(UpsellAdvisor, FindsTheItemThatUnlocksACouponThreshold) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2); // 2000: below the 2999 threshold
    o.coupon = "SAVE";
    const auto repo = couponOnly();
    const auto found = advisor().suggest(o.context(), repo, kStacking, catalog());

    ASSERT_FALSE(found.empty());
    const auto& best = found.front();
    EXPECT_EQ(best.product, &kurta());
    EXPECT_EQ(best.standaloneCost, rs(1120)); // 1000 + 12% GST, no coupon alone
    EXPECT_EQ(best.extraCost, rs(560));       // (3000 - 500 - 2000) + 12% GST
    EXPECT_EQ(best.bonus, rs(560));
    EXPECT_EQ(best.unlockedOffers, std::vector<std::string>{"Flat 500 on 2999+"});
}

TEST(UpsellAdvisor, FindsAQuantityBreak) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<rules::QuantityDiscount>("BUY3", "Buy 3 get 15%", 1, true, 3, 15));
    const auto found = advisor().suggest(o.context(), repo, kStacking, catalog());

    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0].product, &kurta());
    EXPECT_EQ(found[0].extraCost, rs(616)); // (3000 - 450 - 2000) + 12%
    EXPECT_EQ(found[0].bonus, rs(504));     // 1120 - 616
    EXPECT_EQ(found[0].unlockedOffers, std::vector<std::string>{"Buy 3 get 15%"});
}

TEST(UpsellAdvisor, NothingToSuggestWhenNoOfferNeedsTheCombination) {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    const rules::Repository<rules::DiscountRule> none;
    EXPECT_TRUE(advisor().suggest(o.context(), none, kStacking, catalog()).empty());

    Order empty; // an empty cart cannot make anything cheaper
    EXPECT_TRUE(advisor().suggest(empty.context(), couponOnly(), kStacking, catalog()).empty());
}

TEST(UpsellAdvisor, BestFirstAndLimitedToMax) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.coupon = "SAVE";
    rules::Repository<rules::DiscountRule> repo = couponOnly();
    repo.add(std::make_unique<rules::QuantityDiscount>("BUY3", "Buy 3 get 15%", 1, true, 3, 15));
    const auto all = advisor(10).suggest(o.context(), repo, kStacking, catalog());
    for (std::size_t i = 1; i < all.size(); ++i) EXPECT_GE(all[i - 1].bonus, all[i].bonus);

    const auto one = advisor(1).suggest(o.context(), repo, kStacking, catalog());
    ASSERT_EQ(one.size(), 1u);
    EXPECT_EQ(one[0].product, all[0].product);
}

TEST(UpsellAdvisor, DoesNotUseUpCoupons) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.coupon = "SAVE";
    const auto repo = couponOnly();
    advisor().suggest(o.context(), repo, kStacking, catalog());
    const auto* coupon = dynamic_cast<const CouponDiscount*>(repo.findById("SAVE500"));
    ASSERT_NE(coupon, nullptr);
    EXPECT_EQ(coupon->usesSoFar(), 0);
}

TEST(UpsellAdvisor, RejectsBadSetup) {
    EXPECT_THROW(UpsellAdvisor(application::GuardFactory()), std::invalid_argument);
    EXPECT_THROW(advisor(0), std::invalid_argument);
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    EXPECT_THROW(advisor().suggest(o.context(), couponOnly(), kStacking, {nullptr}),
                 std::invalid_argument);
}

TEST(UpsellAdvisor, NeverSuggestsExpiredOrFullItems) {
    Order o;
    o.cart += domain::CartItem(kurta(), domain::CartItem::kMaxQuantity); // line is full
    o.coupon = "SAVE";
    const auto stale = makeMilk("G9", -1);
    const std::vector<const domain::Product*> candidates{&kurta(), &stale};
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<rules::QuantityDiscount>("BUY3", "Buy 3 get 15%", 1, true, 3, 15));
    for (const auto& s : advisor().suggest(o.context(), repo, kStacking, candidates)) {
        EXPECT_NE(s.product, &kurta());
        EXPECT_NE(s.product, &stale);
    }
}
