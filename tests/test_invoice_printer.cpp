#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/PricingEngine.hpp"
#include "engine/StackingResolver.hpp"
#include "presentation/InvoicePrinter.hpp"
#include "rules/BankOfferDiscount.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/SeasonalDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using presentation::InvoicePrinter;

namespace {

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

struct PricedOrder {
    Order order;
    rules::SeasonalDiscount diwali{"DIWALI", "Diwali 20% off Electronics", 90, true, kToday,
                                   kToday, 20, rules::ItemFilter::forCategory("Electronics")};
    rules::PercentageDiscount furniture{"FURN10", "10% off Furniture", 70, true, 10,
                                        rules::ItemFilter::forCategory("Furniture")};
    rules::CouponDiscount coupon{"FLAT300", "Flat Rs 300 off", 50, true, "FLAT300", rs(0), 5,
                                 rules::CouponDiscount::Benefit::flat(rs(300))};
    rules::BankOfferDiscount sbi{"SBI5", "SBI 5% off", 40, true, "SBI", 5, rs(1000)};
    engine::StackingResolver resolver;

    PricedOrder() {
        order.cart += domain::CartItem(phone(), 1);
        order.cart += domain::CartItem(kurta(), 2);
        order.coupon = "FLAT300";
    }

    engine::PricingResult price() const {
        return engine::PricingEngine(resolver).price(
            order.context(), {&diwali, &furniture, &coupon, &sbi});
    }
};

} // namespace

TEST(InvoicePrinter, DisplayWidthCountsRupeeSignAsOneCharacter) {
    EXPECT_EQ(InvoicePrinter::displayWidth("abc"), 3u);
    EXPECT_EQ(InvoicePrinter::displayWidth(rs(1000).toString()), 9u); // ₹1,000.00
    EXPECT_EQ(InvoicePrinter::displayWidth(""), 0u);
}

TEST(InvoicePrinter, RejectsTooNarrowWidth) {
    EXPECT_THROW(InvoicePrinter(20), std::invalid_argument);
}

TEST(InvoicePrinter, PrintsItemisedBill) {
    const PricedOrder p;
    const auto result = p.price();
    std::ostringstream out;
    InvoicePrinter().print(out, p.order.context(), result);
    const std::string bill = out.str();

    EXPECT_TRUE(contains(bill, "TAX INVOICE"));
    EXPECT_TRUE(contains(bill, "Asha (CU1)"));
    EXPECT_TRUE(contains(bill, "Offers resolved by : Stacking"));
    EXPECT_TRUE(contains(bill, "1. Phone [E1]"));
    EXPECT_TRUE(contains(bill, "2. Kurta [C1]"));
    EXPECT_TRUE(contains(bill, "2 x " + rs(1000).toString()));
    EXPECT_TRUE(contains(bill, "Diwali 20% off Electronics"));
    EXPECT_TRUE(contains(bill, (-rs(3000)).toString()));
    EXPECT_TRUE(contains(bill, "Share of order offers"));
    EXPECT_TRUE(contains(bill, "GST @ 18%"));
    EXPECT_TRUE(contains(bill, "GST @ 12%"));
    EXPECT_TRUE(contains(bill, "Flat Rs 300 off"));
    EXPECT_FALSE(contains(bill, "SBI 5% off")); // not applied, so no money row for it
    EXPECT_TRUE(contains(bill, "GRAND TOTAL"));
    EXPECT_TRUE(contains(bill, result.grandTotal().toString()));
    EXPECT_TRUE(contains(bill, "You saved " + result.savingsVsMrp().toString()));
}

TEST(InvoicePrinter, EveryLineFitsTheWidth) {
    const PricedOrder p;
    std::ostringstream out;
    InvoicePrinter(60).print(out, p.order.context(), p.price());
    std::istringstream lines(out.str());
    std::string line;
    while (std::getline(lines, line)) {
        EXPECT_LE(InvoicePrinter::displayWidth(line), 60u) << line;
    }
}

TEST(InvoicePrinter, ShowsGuardAdjustments) {
    // Phone: 20% (3000) + 25% (3750) = 6750 offered; margin floor 12960 keeps 2040.
    const rules::SeasonalDiscount diwali("DIWALI", "Diwali 20%", 90, true, kToday, kToday, 20);
    const rules::PercentageDiscount extra("EXTRA25", "Extra 25%", 70, true, 25);
    const engine::StackingResolver stacking;
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    const auto result = engine::PricingEngine(stacking).price(o.context(), {&diwali, &extra});

    std::ostringstream out;
    InvoicePrinter().print(out, o.context(), result);
    EXPECT_TRUE(contains(out.str(), "Guard adjustment"));
    EXPECT_TRUE(contains(out.str(), "+" + rs(4710).toString())); // price goes back up by 4710
    EXPECT_TRUE(contains(out.str(), "Guard MarginGuard raised price by " + rs(4710).toString()));
}

TEST(InvoicePrinter, ShowsTraceUnderEachItem) {
    const PricedOrder p;
    std::ostringstream out;
    InvoicePrinter(100).print(out, p.order.context(), p.price());
    const std::string bill = out.str();

    // Each decision sits under the item it is about, before the next item starts.
    const auto phoneAt = bill.find("1. Phone [E1]");
    const auto kurtaAt = bill.find("2. Kurta [C1]");
    const auto orderAt = bill.find("Order offers\n");
    ASSERT_NE(phoneAt, std::string::npos);
    ASSERT_NE(kurtaAt, std::string::npos);
    ASSERT_NE(orderAt, std::string::npos);

    const std::string phoneBlock = bill.substr(phoneAt, kurtaAt - phoneAt);
    const std::string kurtaBlock = bill.substr(kurtaAt, orderAt - kurtaAt);
    const std::string orderBlock = bill.substr(orderAt);

    EXPECT_TRUE(contains(phoneBlock, "Why:"));
    EXPECT_TRUE(contains(phoneBlock, "Seasonal DIWALI applied " + rs(3000).toString() + " off"));
    EXPECT_TRUE(contains(phoneBlock, "Guard DiscountCapPolicy passed"));
    // 3000 off leaves 12000, under the 12960 margin floor, so the guard takes 960 back.
    EXPECT_TRUE(contains(phoneBlock, "Guard MarginGuard raised price by " + rs(960).toString()));
    EXPECT_TRUE(contains(phoneBlock, "Guard MrpCeilingPolicy passed"));
    EXPECT_FALSE(contains(phoneBlock, "Kurta"));

    EXPECT_TRUE(contains(kurtaBlock, "Seasonal DIWALI rejected: does not cover Kurta"));
    EXPECT_FALSE(contains(kurtaBlock, "[E1]"));

    // Order-wide decisions: the coupon given, the bank offer and furniture offer refused.
    EXPECT_TRUE(contains(orderBlock, "Coupon FLAT300 applied"));
    EXPECT_TRUE(contains(orderBlock, "BankOffer SBI5 rejected: no bank card used for payment"));
    EXPECT_TRUE(contains(orderBlock, "Percentage FURN10 rejected"));
}

TEST(InvoicePrinter, OrderSectionShownWhenOnlyRejectionsHappened) {
    const rules::CouponDiscount save10("SAVE10", "Save 10%", 50, true, "SAVE10", rs(75000), 5,
                                       rules::CouponDiscount::Benefit::percent(10));
    const engine::StackingResolver stacking;
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    o.coupon = "SAVE10";
    const auto result = engine::PricingEngine(stacking).price(o.context(), {&save10});
    ASSERT_TRUE(result.cartDiscounts().empty());

    std::ostringstream out;
    InvoicePrinter(100).print(out, o.context(), result);
    EXPECT_TRUE(contains(out.str(), "Order offers\n"));
    EXPECT_TRUE(contains(out.str(), "Coupon SAVE10 rejected: cart value " + rs(1000).toString() +
                                        " is below minimum " + rs(75000).toString()));
}

TEST(InvoicePrinter, TraceListsAppliedAndRejectedDecisions) {
    const PricedOrder p;
    const auto result = p.price();
    std::ostringstream out;
    InvoicePrinter().printTrace(out, result.trace());
    const std::string trace = out.str();

    EXPECT_TRUE(contains(trace, "PRICE TRACE (" + std::to_string(result.trace().size())));
    EXPECT_TRUE(contains(trace, "Applied"));
    EXPECT_TRUE(contains(trace, "Rejected"));
    EXPECT_TRUE(contains(trace, "DIWALI"));
    EXPECT_TRUE(contains(trace, "no bank card used for payment"));
    EXPECT_TRUE(contains(trace, "does not cover Kurta"));
}

TEST(InvoicePrinter, WrappedReasonsNeverLeaveBlankLines) {
    // Reasons of every length: when a wrapped line exactly filled the width, the
    // printer used to add a line of spaces after it.
    Order o;
    o.cart += domain::CartItem(phone(), 1);
    // Words up to 50 characters, so every word fits on one 72-character line.
    for (std::size_t extra = 0; extra < 50; ++extra) {
        engine::PriceTrace trace;
        trace.record(engine::TraceEntry::rejected("Seasonal", "R1", "Rule", "E1",
                                                  "word " + std::string(extra, 'x') +
                                                      " more words to wrap over the line end"));
        engine::PricedLine line;
        line.productId = "E1";
        line.productName = "Phone";
        line.quantity = 1;
        const engine::PricingResult result("Stacking", {line}, {}, trace);
        std::ostringstream out;
        InvoicePrinter().print(out, o.context(), result);
        std::istringstream lines(out.str());
        std::string text;
        while (std::getline(lines, text)) {
            const bool onlySpaces = !text.empty() && text.find_first_not_of(' ') == std::string::npos;
            EXPECT_FALSE(onlySpaces) << "blank line for extra=" << extra << "\n" << out.str();
            EXPECT_LE(InvoicePrinter::displayWidth(text), 72u) << text;
        }
    }
}
