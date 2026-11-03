#include <gtest/gtest.h>

#include <utility>

#include "engine/PricingResult.hpp"

using domain::Money;
using engine::AppliedDiscount;
using engine::PricedLine;
using engine::PricingResult;

namespace {

Money rs(std::int64_t rupees) {
    return Money::fromRupees(rupees);
}

PricedLine line(int qty, std::int64_t unit, std::int64_t mrp, std::int64_t itemOff,
                std::int64_t cartShare, int gstRate) {
    PricedLine l;
    l.quantity = qty;
    l.unitPrice = rs(unit);
    l.unitMrp = rs(mrp);
    l.gross = rs(unit) * qty;
    l.itemDiscount = rs(itemOff);
    l.cartDiscountShare = rs(cartShare);
    l.taxable = l.gross - l.itemDiscount - l.cartDiscountShare;
    l.gstRate = gstRate;
    l.gst = l.taxable.percentOf(gstRate);
    l.total = l.taxable + l.gst;
    return l;
}

PricingResult sample() {
    std::vector<PricedLine> lines{line(2, 100, 130, 20, 10, 18),  // taxable 170, gst 30.60
                                  line(1, 500, 600, 50, 40, 5)};  // taxable 410, gst 20.50
    std::vector<AppliedDiscount> cart{{"C1", "Coupon", rules::RuleStage::Coupon, rs(50)}};
    return PricingResult("Stacking", std::move(lines), std::move(cart), engine::PriceTrace{});
}

} // namespace

TEST(PricedLine, NetAndMrpTotal) {
    const PricedLine l = line(3, 100, 120, 30, 0, 18);
    EXPECT_EQ(l.net(), rs(270));
    EXPECT_EQ(l.mrpTotal(), rs(360));
}

TEST(PricingResult, TotalsAreSummedFromLines) {
    const PricingResult r = sample();
    EXPECT_EQ(r.resolverName(), "Stacking");
    EXPECT_EQ(r.lines().size(), 2u);
    EXPECT_EQ(r.subtotal(), rs(700));
    EXPECT_EQ(r.itemDiscountTotal(), rs(70));
    EXPECT_EQ(r.cartDiscountTotal(), rs(50));
    EXPECT_EQ(r.taxableTotal(), rs(580));
    EXPECT_EQ(r.gstTotal(), Money::fromRupees(51, 10));
    EXPECT_EQ(r.grandTotal(), Money::fromRupees(631, 10));
    EXPECT_EQ(r.mrpTotal(), rs(860));
    EXPECT_EQ(r.savingsVsMrp(), Money::fromRupees(228, 90));
}

TEST(PricingResult, EmptyOrderIsAllZero) {
    const PricingResult r("Priority", {}, {}, engine::PriceTrace{});
    EXPECT_EQ(r.subtotal(), Money::zero());
    EXPECT_EQ(r.grandTotal(), Money::zero());
    EXPECT_EQ(r.cartDiscountTotal(), Money::zero());
    EXPECT_TRUE(r.trace().empty());
}
