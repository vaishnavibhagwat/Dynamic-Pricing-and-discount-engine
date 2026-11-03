#include "presentation/WhatIfPrinter.hpp"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

#include "presentation/InvoicePrinter.hpp"

namespace presentation {

namespace {

constexpr std::size_t kTierWidth = 10;

std::string padRight(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : text + std::string(width - shown, ' ');
}

std::string padLeft(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : std::string(width - shown, ' ') + text;
}

} // namespace

void WhatIfPrinter::print(std::ostream& os, const application::WhatIfReport& report) const {
    const auto& names = report.resolverNames();
    const auto& tiers = report.tiers();

    // Each column is wide enough for its name and its widest amount.
    std::vector<std::size_t> widths(names.size());
    for (std::size_t c = 0; c < names.size(); ++c) {
        widths[c] = InvoicePrinter::displayWidth(names[c]);
        for (std::size_t t = 0; t < tiers.size(); ++t) {
            widths[c] = std::max(widths[c], InvoicePrinter::displayWidth(
                                                report.cell(t, c).grandTotal.toString()));
        }
    }

    os << "What-if: grand total (incl. GST) for this cart under each policy and tier\n\n";
    os << "  " << padRight("Tier", kTierWidth);
    for (std::size_t c = 0; c < names.size(); ++c) os << "  " << padLeft(names[c], widths[c]) << "  ";
    os << '\n';

    std::size_t bestTier = 0;
    std::size_t bestCol = 0;
    for (std::size_t t = 0; t < tiers.size(); ++t) {
        const std::size_t cheapest = report.cheapestResolver(t);
        if (report.cell(t, cheapest).grandTotal < report.cell(bestTier, bestCol).grandTotal) {
            bestTier = t;
            bestCol = cheapest;
        }
        os << (tiers[t] == report.currentTier() ? "* " : "  ")
           << padRight(domain::toString(tiers[t]), kTierWidth);
        for (std::size_t c = 0; c < names.size(); ++c) {
            os << "  " << padLeft(report.cell(t, c).grandTotal.toString(), widths[c])
               << (c == cheapest ? " <" : "  ");
        }
        os << '\n';
    }

    os << "\n  * = this customer's tier   < = cheapest policy for that tier\n";
    if (!tiers.empty() && !names.empty()) {
        os << "  Lowest overall: " << domain::toString(tiers[bestTier]) << " with "
           << names[bestCol] << ", " << report.cell(bestTier, bestCol).grandTotal << '\n';
    }
}

} // namespace presentation
