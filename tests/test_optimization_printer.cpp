#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "application/DiscountOptimizer.hpp"
#include "presentation/OptimizationPrinter.hpp"
#include "rules/PercentageDiscount.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::DiscountOptimizer;
using presentation::OptimizationPrinter;
using rules::ItemFilter;
using rules::PercentageDiscount;

namespace {

using RuleList = std::vector<const rules::DiscountRule*>;

std::string printed(const application::OptimizationResult& r, std::size_t topN = 5) {
    std::ostringstream os;
    OptimizationPrinter(topN).print(os, r);
    return os.str();
}

Order oneKurta() {
    Order o;
    o.cart += domain::CartItem(kurta(), 1);
    return o;
}

std::size_t count(const std::string& text, const std::string& what) {
    std::size_t n = 0;
    for (auto pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + 1)) ++n;
    return n;
}

} // namespace

TEST(OptimizationPrinter, ShowsWinnerSavingAndCounts) {
    const PercentageDiscount p20("P20", "20% off", 1, true, 20);
    const PercentageDiscount p30("P30", "30% off", 1, true, 30);
    const PercentageDiscount p40("P40", "40% off", 1, true, 40);
    const Order o = oneKurta();
    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{&p20, &p30, &p40});
    const std::string out = printed(r);

    EXPECT_NE(out.find("tried 7 combinations, 3 cut by price guards"), std::string::npos) << out;
    EXPECT_NE(out.find("Without offers: ₹1,120.00"), std::string::npos) << out;
    EXPECT_NE(out.find("Best total:     ₹560.00  (saves ₹560.00)"), std::string::npos) << out;
    EXPECT_NE(out.find("P20 - 20% off"), std::string::npos);
    EXPECT_NE(out.find("P30 - 30% off"), std::string::npos);
    EXPECT_NE(out.find("1.      ₹560.00  saves    ₹560.00  P20 + P30"), std::string::npos) << out;
}

TEST(OptimizationPrinter, ListsOnlyTopNAndNamesGuards) {
    const PercentageDiscount p20("P20", "20% off", 1, true, 20);
    const PercentageDiscount p30("P30", "30% off", 1, true, 30);
    const PercentageDiscount p40("P40", "40% off", 1, true, 40);
    const Order o = oneKurta();
    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{&p20, &p30, &p40});

    EXPECT_NE(printed(r, 2).find("Top 2:"), std::string::npos);
    const std::string all = printed(r, 10);
    EXPECT_NE(all.find("Top 7:"), std::string::npos);
    EXPECT_EQ(count(all, "[cut by "), 3u);
    EXPECT_EQ(count(all, "[cut by MarginGuard]"), 1u);                    // P20 + P40
    EXPECT_EQ(count(all, "[cut by DiscountCapPolicy, MarginGuard]"), 2u); // over the 60% cap
}

TEST(OptimizationPrinter, ExplainsScreenedOffers) {
    const PercentageDiscount phones("PHONES", "5% off Electronics", 1, true, 5,
                                    ItemFilter::forCategory("Electronics"));
    const Order o = oneKurta();
    const auto r = DiscountOptimizer().optimize(o.context(), RuleList{&phones});
    const std::string out = printed(r);
    EXPECT_NE(out.find("No combination saves money"), std::string::npos) << out;
    EXPECT_NE(out.find("Optimizer PHONES rejected: not applicable"), std::string::npos) << out;
    EXPECT_EQ(out.find("Top"), std::string::npos);
}
