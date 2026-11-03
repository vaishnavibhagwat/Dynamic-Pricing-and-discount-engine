#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "rules/CouponDiscount.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/RuleDecorator.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using rules::CouponDiscount;
using rules::DiscountRule;
using rules::RuleDecorator;

namespace {

/// The plainest decorator: changes nothing, so it shows pure forwarding.
class PassThrough final : public RuleDecorator {
public:
    explicit PassThrough(std::unique_ptr<DiscountRule> inner)
        : RuleDecorator(std::move(inner), "pass") {}
};

/// Blocks every order, to show the extra condition and its reason.
class AlwaysClosed final : public RuleDecorator {
public:
    explicit AlwaysClosed(std::unique_ptr<DiscountRule> inner)
        : RuleDecorator(std::move(inner), "closed") {}

protected:
    bool allows(const domain::PricingContext&) const override { return false; }
    std::string blockedReason(const domain::PricingContext&) const override { return "closed"; }
};

/// Halves each discount, to show the limit() hook.
class Half final : public RuleDecorator {
public:
    explicit Half(std::unique_ptr<DiscountRule> inner) : RuleDecorator(std::move(inner), "half") {}

protected:
    Money limit(Money d) const override { return d.percentOf(50); }
};

std::unique_ptr<DiscountRule> tenPercentElectronics() {
    return std::make_unique<rules::PercentageDiscount>(
        "P10", "Ten off", 7, false, 10, rules::ItemFilter::forCategory("Electronics"));
}

std::unique_ptr<CouponDiscount> flatCoupon(int usageLimit = 1) {
    return std::make_unique<CouponDiscount>("CP", "Flat 500", 3, true, "SAVE", rs(1000),
                                            usageLimit, CouponDiscount::Benefit::flat(rs(500)));
}

} // namespace

TEST(RuleDecorator, KeepsTheWrappedRulesIdentity) {
    const PassThrough d(tenPercentElectronics());
    EXPECT_EQ(d.id(), "P10");
    EXPECT_EQ(d.name(), "Ten off [pass]");
    EXPECT_EQ(d.priority(), 7);
    EXPECT_FALSE(d.stackable());
    EXPECT_EQ(d.stage(), rules::RuleStage::Percentage);
    EXPECT_EQ(d.inner().name(), "Ten off");
    ASSERT_NE(d.scope(), nullptr);
    EXPECT_EQ(d.scope(), d.inner().scope());
    EXPECT_EQ(d.maxPercentOff(phone()), 10);
    EXPECT_FALSE(d.maxPercentOff(kurta()).has_value());
}

TEST(RuleDecorator, ForwardsItemPricing) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    o.cart += domain::CartItem(kurta(), 1);
    const PassThrough d(tenPercentElectronics());
    EXPECT_TRUE(d.isApplicable(o.context()));
    EXPECT_TRUE(d.appliesToItem(o.cart.items()[0], o.context()));
    EXPECT_FALSE(d.appliesToItem(o.cart.items()[1], o.context()));
    EXPECT_EQ(d.discountForItem(o.cart.items()[0], o.context()), rs(1500));
    EXPECT_EQ(d.calculateDiscount(o.context()), rs(1500));
    // An item offer has no order-level pricing.
    EXPECT_EQ(d.discountOnAmount(rs(10000), o.context()), Money::zero());
}

TEST(RuleDecorator, ForwardsOrderPricingAndUsage) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.coupon = "SAVE";
    auto coupon = flatCoupon(1);
    const CouponDiscount* raw = coupon.get();
    PassThrough d(std::move(coupon));
    EXPECT_TRUE(d.isApplicable(o.context()));
    EXPECT_EQ(d.discountOnAmount(rs(2000), o.context()), rs(500));
    EXPECT_EQ(d.calculateDiscount(o.context()), rs(500));

    d.recordUse(); // reaches the coupon inside
    EXPECT_EQ(raw->usesSoFar(), 1);
    EXPECT_FALSE(d.isApplicable(o.context()));
    EXPECT_EQ(d.rejectionReason(o.context()), raw->rejectionReason(o.context()));
}

TEST(RuleDecorator, RecordUseOnPlainOfferDoesNothing) {
    PassThrough d(tenPercentElectronics());
    EXPECT_NO_THROW(d.recordUse());
}

TEST(RuleDecorator, ExtraConditionBlocksWithItsOwnReason) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    const AlwaysClosed d(tenPercentElectronics());
    EXPECT_FALSE(d.isApplicable(o.context()));
    EXPECT_EQ(d.rejectionReason(o.context()), "closed");
    EXPECT_EQ(d.calculateDiscount(o.context()), Money::zero());
}

TEST(RuleDecorator, LimitHookAppliesPerLineForItemOffers) {
    Order o;
    o.cart += domain::CartItem(phone(), 2); // 10% of 30000 = 3000, halved = 1500
    const Half d(tenPercentElectronics());
    EXPECT_EQ(d.discountForItem(o.cart.items()[0], o.context()), rs(1500));
    EXPECT_EQ(d.calculateDiscount(o.context()), rs(1500));
}

TEST(RuleDecorator, LimitHookAppliesToOrderOffers) {
    Order o;
    o.cart += domain::CartItem(kurta(), 2);
    o.coupon = "SAVE";
    const Half d(flatCoupon());
    EXPECT_EQ(d.discountOnAmount(rs(2000), o.context()), rs(250));
    EXPECT_EQ(d.calculateDiscount(o.context()), rs(250));
}

TEST(RuleDecorator, DecoratorsNest) {
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    const Half twice(std::make_unique<Half>(tenPercentElectronics()));
    EXPECT_EQ(twice.name(), "Ten off [half] [half]");
    EXPECT_EQ(twice.calculateDiscount(o.context()), rs(375));
}

TEST(RuleDecorator, WorksForOffersWithBothCapabilities) {
    // MembershipDiscount is order-level by class but prices per item (item stage).
    Order o;
    o.customer = domain::Customer("CU9", "Gold member", domain::MembershipTier::Gold);
    o.cart += domain::CartItem(kurta(), 1);
    auto member = std::make_unique<rules::MembershipDiscount>(
        "M", "Members", 1, true, std::map<domain::MembershipTier, int>{{domain::MembershipTier::Gold, 5}});
    const Money plain = member->calculateDiscount(o.context());
    const PassThrough d(std::move(member));
    EXPECT_EQ(d.calculateDiscount(o.context()), plain);
}

TEST(RuleDecorator, RejectsNullRule) {
    EXPECT_THROW(PassThrough(nullptr), std::invalid_argument);
}
