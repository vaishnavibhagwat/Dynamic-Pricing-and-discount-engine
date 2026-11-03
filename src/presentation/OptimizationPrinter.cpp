#include "presentation/OptimizationPrinter.hpp"

#include <algorithm>
#include <ostream>
#include <string>

#include "presentation/InvoicePrinter.hpp"

namespace presentation {

namespace {

std::string joinIds(const application::OfferCombination& combo) {
    if (combo.ruleIds.empty()) return "(no offers)";
    std::string text;
    for (std::size_t i = 0; i < combo.ruleIds.size(); ++i) {
        if (i > 0) text += " + ";
        text += combo.ruleIds[i];
    }
    return text;
}

std::string padLeft(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : std::string(width - shown, ' ') + text;
}

} // namespace

OptimizationPrinter::OptimizationPrinter(std::size_t topN) : topN_(std::max<std::size_t>(topN, 1)) {}

void OptimizationPrinter::print(std::ostream& os,
                                const application::OptimizationResult& result) const {
    const auto& best = result.bestCombination();
    os << "Best offer combination - tried " << result.combinationsTried() << " combination"
       << (result.combinationsTried() == 1 ? "" : "s") << ", " << result.failedGuards()
       << " cut by price guards\n\n";
    os << "  Without offers: " << result.baselineTotal() << '\n';
    os << "  Best total:     " << best.grandTotal << "  (saves " << best.saving << ")\n";
    if (best.ruleIds.empty()) {
        os << "  No combination saves money on this cart without a guard cutting it.\n";
    } else {
        os << "  Offers:\n";
        for (std::size_t i = 0; i < best.ruleIds.size(); ++i) {
            os << "    " << best.ruleIds[i] << " - " << best.ruleNames[i] << '\n';
        }
    }

    if (!result.ranked().empty()) {
        const std::size_t shown = std::min(topN_, result.ranked().size());
        os << "\n  Top " << shown << ":\n";
        for (std::size_t i = 0; i < shown; ++i) {
            const auto& c = result.ranked()[i];
            os << "  " << padLeft(std::to_string(i + 1), 3) << ". "
               << padLeft(c.grandTotal.toString(), 12) << "  saves "
               << padLeft(c.saving.toString(), 10) << "  " << joinIds(c);
            if (!c.passedGuards) {
                os << "  [cut by ";
                for (std::size_t g = 0; g < c.guardsTripped.size(); ++g) {
                    os << (g > 0 ? ", " : "") << c.guardsTripped[g];
                }
                os << ']';
            }
            os << '\n';
        }
    }

    if (!result.screening().empty()) {
        os << "\n  Offers considered:\n";
        for (const auto& entry : result.screening().entries()) {
            os << "    " << entry.summary() << '\n';
        }
    }
}

} // namespace presentation
