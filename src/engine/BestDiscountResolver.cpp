#include "engine/BestDiscountResolver.hpp"

#include <cstddef>

namespace engine {

std::vector<Verdict> BestDiscountResolver::resolve(const std::vector<Candidate>& candidates) const {
    std::vector<Verdict> verdicts(candidates.size());
    if (candidates.empty()) return verdicts;

    std::size_t best = 0;
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        const Candidate& c = candidates[i];
        const Candidate& b = candidates[best];
        // Strictly better only, so earlier candidates win full ties.
        if (c.discount > b.discount ||
            (c.discount == b.discount && c.rule->priority() > b.rule->priority())) {
            best = i;
        }
    }

    const Candidate& winner = candidates[best];
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (i == best) {
            verdicts[i] = {true, "largest single saving"};
        } else if (candidates[i].discount == winner.discount) {
            verdicts[i] = {false, "same saving as " + winner.rule->id() +
                                      ", which has priority or comes first"};
        } else {
            verdicts[i] = {false, winner.rule->id() + " saves more (" +
                                      winner.discount.toString() + ")"};
        }
    }
    return verdicts;
}

} // namespace engine
