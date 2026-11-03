#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "application/WhatIfSimulator.hpp"
#include "engine/BestDiscountResolver.hpp"
#include "engine/PriorityResolver.hpp"
#include "engine/StackingResolver.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::WhatIfCell;
using application::WhatIfReport;
using application::WhatIfSimulator;
using domain::MembershipTier;

namespace {

using RuleList = std::vector<const rules::DiscountRule*>;

const engine::StackingResolver kStacking;
const engine::BestDiscountResolver kBest;
const engine::PriorityResolver kPriority;

const rules::MembershipDiscount kLoyalty("LOYAL", "Loyalty", 10, true,
                                         {{MembershipTier::Silver, 2},
                                          {MembershipTier::Gold, 5},
                                          {MembershipTier::Platinum, 10}});
const rules::PercentageDiscount kSale("SALE", "20% off", 50, true, 20);

Order goldKurtaOrder() {
    Order o;
    o.customer = domain::Customer("CU9", "Neha", MembershipTier::Gold);
    o.cart += domain::CartItem(kurta(), 1);
    return o;
}

} // namespace

TEST(WhatIfSimulator, OneRowPerTierOneColumnPerResolver) {
    const WhatIfSimulator sim({&kStacking, &kBest, &kPriority});
    const Order o = goldKurtaOrder();
    const WhatIfReport r = sim.run(o.context(), RuleList{&kLoyalty, &kSale});

    EXPECT_EQ(r.resolverNames(), (std::vector<std::string>{"Stacking", "BestDiscount", "Priority"}));
    ASSERT_EQ(r.tiers().size(), 4u);
    EXPECT_EQ(r.tiers().front(), MembershipTier::Regular);
    EXPECT_EQ(r.tiers().back(), MembershipTier::Platinum);
    EXPECT_EQ(r.currentTier(), MembershipTier::Gold);
}

TEST(WhatIfSimulator, HigherTierPaysLessWhenOffersStack) {
    const WhatIfSimulator sim({&kStacking, &kBest});
    const Order o = goldKurtaOrder();
    const WhatIfReport r = sim.run(o.context(), RuleList{&kLoyalty, &kSale});

    // Stacking: 20% sale + tier percent. ₹1,000 kurta, 12% GST.
    EXPECT_EQ(r.cell(0, 0).grandTotal, rs(896));      // Regular: 20% off
    EXPECT_EQ(r.cell(1, 0).grandTotal, rs(873, 60));  // Silver: 22%
    EXPECT_EQ(r.cell(2, 0).grandTotal, rs(840));      // Gold: 25%
    EXPECT_EQ(r.cell(3, 0).grandTotal, rs(784));      // Platinum: 30%
    EXPECT_EQ(r.cell(3, 0).discount, rs(300));

    // BestDiscount gives only the 20% sale, whatever the tier.
    for (std::size_t t = 0; t < 4; ++t) EXPECT_EQ(r.cell(t, 1).grandTotal, rs(896));
    EXPECT_EQ(r.cheapestResolver(3), 0u);
    EXPECT_EQ(r.cheapestResolver(0), 0u); // tie goes to the first column
}

TEST(WhatIfSimulator, DoesNotChangeTheRealCustomer) {
    const WhatIfSimulator sim({&kStacking});
    const Order o = goldKurtaOrder();
    sim.run(o.context(), RuleList{&kLoyalty});
    EXPECT_EQ(o.customer.tier(), MembershipTier::Gold);
}

TEST(WhatIfSimulator, KeepsCouponAndBankOfOrder) {
    rules::Repository<rules::DiscountRule> repo;
    repo.add(std::make_unique<rules::PercentageDiscount>("SALE", "20% off", 50, true, 20));
    const WhatIfSimulator sim({&kStacking});
    Order o = goldKurtaOrder();
    o.coupon = "ANY";
    o.bank = "HDFC";
    const WhatIfReport r = sim.run(o.context(), repo); // Repository overload
    EXPECT_EQ(r.cell(0, 0).grandTotal, rs(896));
}

TEST(WhatIfSimulator, RejectsBadArguments) {
    EXPECT_THROW(WhatIfSimulator({}), std::invalid_argument);
    EXPECT_THROW(WhatIfSimulator({&kStacking, nullptr}), std::invalid_argument);
    EXPECT_THROW(WhatIfSimulator({&kStacking}, application::GuardFactory()),
                 std::invalid_argument);
}

TEST(WhatIfReport, ChecksShapeAndBounds) {
    const WhatIfCell c{rs(1), rs(0)};
    EXPECT_THROW(WhatIfReport({"A"}, {MembershipTier::Regular}, {}, MembershipTier::Regular),
                 std::invalid_argument);
    EXPECT_THROW(WhatIfReport({"A", "B"}, {MembershipTier::Regular}, {{c}},
                              MembershipTier::Regular),
                 std::invalid_argument);
    const WhatIfReport ok({"A"}, {MembershipTier::Regular}, {{c}}, MembershipTier::Regular);
    EXPECT_EQ(ok.cell(0, 0).grandTotal, rs(1));
    EXPECT_THROW(ok.cell(1, 0), std::out_of_range);
    EXPECT_THROW(ok.cell(0, 1), std::out_of_range);
}
