#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "presentation/InvoicePrinter.hpp"
#include "presentation/WhatIfPrinter.hpp"
#include "test_rule_fixtures.hpp"

using namespace fixtures;
using application::WhatIfReport;
using domain::MembershipTier;

namespace {

std::string printed(const WhatIfReport& r) {
    std::ostringstream os;
    presentation::WhatIfPrinter().print(os, r);
    return os.str();
}

/// Line of @p text that starts with @p prefix (empty if none).
std::string lineStartingWith(const std::string& text, const std::string& prefix) {
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(prefix, 0) == 0) return line;
    }
    return "";
}

WhatIfReport sample() {
    return WhatIfReport({"Stacking", "BestDiscount"},
                        {MembershipTier::Regular, MembershipTier::Gold},
                        {{{rs(900), rs(100)}, {rs(950), rs(50)}},
                         {{rs(880), rs(120)}, {rs(850), rs(150)}}},
                        MembershipTier::Gold);
}

} // namespace

TEST(WhatIfPrinter, HeaderListsEveryPolicy) {
    const std::string header = lineStartingWith(printed(sample()), "  Tier");
    EXPECT_NE(header.find("Stacking"), std::string::npos);
    EXPECT_NE(header.find("BestDiscount"), std::string::npos);
}

TEST(WhatIfPrinter, MarksCurrentTierAndCheapestPolicyPerRow) {
    const std::string out = printed(sample());
    const std::string regular = lineStartingWith(out, "  Regular");
    const std::string gold = lineStartingWith(out, "* Gold");
    ASSERT_FALSE(regular.empty()) << out;
    ASSERT_FALSE(gold.empty()) << out;

    // Regular: Stacking (₹900) is cheapest. Gold: BestDiscount (₹850).
    EXPECT_NE(regular.find("₹900.00 <"), std::string::npos) << regular;
    EXPECT_EQ(regular.find("₹950.00 <"), std::string::npos) << regular;
    EXPECT_NE(gold.find("₹850.00 <"), std::string::npos) << gold;
    EXPECT_EQ(gold.find("₹880.00 <"), std::string::npos) << gold;
}

TEST(WhatIfPrinter, ColumnsLineUp) {
    const std::string out = printed(sample());
    const std::string regular = lineStartingWith(out, "  Regular");
    const std::string gold = lineStartingWith(out, "* Gold");
    using presentation::InvoicePrinter;
    EXPECT_EQ(InvoicePrinter::displayWidth(regular), InvoicePrinter::displayWidth(gold));
}

TEST(WhatIfPrinter, NamesLowestOverall) {
    EXPECT_NE(printed(sample()).find("Lowest overall: Gold with BestDiscount, ₹850.00"),
              std::string::npos);
}
