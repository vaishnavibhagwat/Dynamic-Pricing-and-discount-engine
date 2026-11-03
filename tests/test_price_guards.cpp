#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "domain/Electronics.hpp"
#include "engine/DiscountCapPolicy.hpp"
#include "engine/MarginGuard.hpp"
#include "engine/MrpCeilingPolicy.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using domain::CartItem;

TEST(PriceGuard, IsAnAbstractInterface) {
    EXPECT_TRUE((std::is_abstract_v<engine::PriceGuard>));
    EXPECT_TRUE((std::has_virtual_destructor_v<engine::PriceGuard>));
    EXPECT_TRUE((std::is_base_of_v<engine::PriceGuard, engine::DiscountCapPolicy>));
    EXPECT_TRUE((std::is_base_of_v<engine::PriceGuard, engine::MarginGuard>));
    EXPECT_TRUE((std::is_base_of_v<engine::PriceGuard, engine::MrpCeilingPolicy>));
}

// --- DiscountCapPolicy -----------------------------------------------------

TEST(DiscountCapPolicy, LeavesSmallDiscountAlone) {
    const engine::DiscountCapPolicy cap;
    const auto out = cap.limit(CartItem(phone(), 1), rs(9000)); // exactly 60% of 15000
    EXPECT_EQ(out.discount, rs(9000));
    // A passing check still explains itself, for the price trace.
    EXPECT_EQ(out.reason, "discount " + rs(9000).toString() + " is within the 60% cap of " +
                              rs(9000).toString());
}

TEST(DiscountCapPolicy, CutsDiscountToSixtyPercent) {
    const engine::DiscountCapPolicy cap;
    const auto out = cap.limit(CartItem(phone(), 2), rs(20000)); // line 30000, cap 18000
    EXPECT_EQ(out.discount, rs(18000));
    EXPECT_NE(out.reason.find("60%"), std::string::npos);
}

TEST(DiscountCapPolicy, ConfigurableAndValidated) {
    EXPECT_EQ(engine::DiscountCapPolicy().maxPercent(), 60);
    EXPECT_EQ(engine::DiscountCapPolicy(30).limit(CartItem(kurta(), 1), rs(500)).discount, rs(300));
    EXPECT_THROW(engine::DiscountCapPolicy(-1), std::invalid_argument);
    EXPECT_THROW(engine::DiscountCapPolicy(101), std::invalid_argument);
}

// --- MarginGuard -----------------------------------------------------------

TEST(MarginGuard, FloorIsCostPlusEightPercent) {
    const engine::MarginGuard guard;
    EXPECT_EQ(guard.floorFor(CartItem(phone(), 1)), rs(12960));
    EXPECT_EQ(guard.floorFor(CartItem(phone(), 2)), rs(25920));
}

TEST(MarginGuard, FloorRoundsUp) {
    const domain::Electronics cable("E9", "Cable", Money::fromPaise(33333), rs(500), rs(700));
    const engine::MarginGuard guard;
    // 333.33 x 1.08 = 359.9964 -> 360.00
    EXPECT_EQ(guard.floorFor(CartItem(cable, 1)), rs(360));
    EXPECT_EQ(guard.limit(CartItem(cable, 1), rs(200)).discount, rs(140));
}

TEST(MarginGuard, CutsDiscountThatBreaksMargin) {
    const engine::MarginGuard guard;
    const auto ok = guard.limit(CartItem(phone(), 1), rs(2000));
    EXPECT_EQ(ok.discount, rs(2000));
    EXPECT_EQ(ok.reason, "net " + rs(13000).toString() + " keeps cost + 8% floor of " +
                             rs(12960).toString());

    const auto cut = guard.limit(CartItem(phone(), 1), rs(3750));
    EXPECT_EQ(cut.discount, rs(2040));
    EXPECT_NE(cut.reason.find("cost + 8%"), std::string::npos);
}

TEST(MarginGuard, LossLeaderGetsNoDiscountButPriceIsNotRaised) {
    const domain::Electronics lossLeader("E8", "Earbuds", rs(100), rs(105), rs(150)); // floor 108
    const engine::MarginGuard guard;
    EXPECT_EQ(guard.limit(CartItem(lossLeader, 1), rs(10)).discount, Money::zero());
    EXPECT_EQ(guard.limit(CartItem(lossLeader, 1), Money::zero()).discount, Money::zero());
}

TEST(MarginGuard, RejectsNegativeMarkup) {
    EXPECT_THROW(engine::MarginGuard(-1), std::invalid_argument);
    EXPECT_EQ(engine::MarginGuard(0).floorFor(CartItem(phone(), 1)), rs(12000));
}

// --- MrpCeilingPolicy ------------------------------------------------------

TEST(MrpCeilingPolicy, NoChangeWhenPriceWithGstIsWithinMrp) {
    const engine::MrpCeilingPolicy mrp;
    // 15000 + 18% = 17700 <= 18000
    const auto out = mrp.limit(CartItem(phone(), 1), Money::zero());
    EXPECT_EQ(out.discount, Money::zero());
    EXPECT_EQ(out.reason, "price incl. 18% GST " + rs(17700).toString() + " is within MRP " +
                              rs(18000).toString());
}

TEST(MrpCeilingPolicy, RaisesDiscountSoPriceWithGstMeetsMrp) {
    // 1000 + 18% = 1180 > MRP 1050
    const domain::Electronics speaker("E7", "Speaker", rs(500), rs(1000), rs(1050));
    const engine::MrpCeilingPolicy mrp;
    EXPECT_EQ(mrp.ceilingFor(CartItem(speaker, 1)), Money::fromRupees(889, 83));

    const auto out = mrp.limit(CartItem(speaker, 1), Money::zero());
    EXPECT_EQ(out.discount, Money::fromRupees(110, 17));
    EXPECT_NE(out.reason.find("exceed MRP"), std::string::npos);

    // Net 889.83 + GST 160.17 = 1050.00 exactly.
    const Money net = rs(1000) - out.discount;
    EXPECT_LE(net + net.percentOf(18), rs(1050));
}

TEST(MrpCeilingPolicy, ScalesWithQuantityAndKeepsBiggerDiscounts) {
    const domain::Electronics speaker("E7", "Speaker", rs(500), rs(1000), rs(1050));
    const engine::MrpCeilingPolicy mrp;
    EXPECT_EQ(mrp.limit(CartItem(speaker, 2), Money::zero()).discount, Money::fromRupees(220, 34));
    EXPECT_EQ(mrp.limit(CartItem(speaker, 1), rs(200)).discount, rs(200));
}
