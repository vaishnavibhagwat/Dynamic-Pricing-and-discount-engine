#include "presentation/UpsellPrinter.hpp"

#include <ostream>

namespace presentation {

void UpsellPrinter::print(std::ostream& os,
                          const std::vector<application::UpsellSuggestion>& suggestions,
                          const std::string& policyName) const {
    os << "Smart suggestions (policy " << policyName << ")\n\n";
    if (suggestions.empty()) {
        os << "  No single item unlocks extra savings for this cart right now.\n";
        return;
    }
    for (std::size_t i = 0; i < suggestions.size(); ++i) {
        const auto& s = suggestions[i];
        os << "  " << (i + 1) << ". Add 1 x " << s.product->name() << " (" << s.product->id()
           << ")\n"
           << "     Bought alone it costs " << s.standaloneCost << "; added to this cart your bill\n"
           << "     goes up by only " << s.extraCost << " - you save " << s.bonus << " extra.\n";
        if (!s.unlockedOffers.empty()) {
            os << "     Unlocks: ";
            for (std::size_t k = 0; k < s.unlockedOffers.size(); ++k) {
                os << (k > 0 ? ", " : "") << s.unlockedOffers[k];
            }
            os << '\n';
        }
    }
    os << "\n  Prices include GST and every offer and guard, as at checkout.\n";
}

} // namespace presentation
