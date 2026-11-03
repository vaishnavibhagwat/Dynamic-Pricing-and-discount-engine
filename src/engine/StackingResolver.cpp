#include "engine/StackingResolver.hpp"

#include <cstddef>
#include <optional>

namespace engine {

std::vector<Verdict> StackingResolver::resolve(const std::vector<Candidate>& candidates) const {
    std::vector<Verdict> verdicts(candidates.size());

    domain::Money stackTotal;
    bool anyStackable = false;
    std::optional<std::size_t> bestSolo; // best non-stackable candidate
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const Candidate& c = candidates[i];
        if (c.rule->stackable()) {
            stackTotal += c.discount;
            anyStackable = true;
        } else if (!bestSolo) {
            bestSolo = i;
        } else {
            const Candidate& b = candidates[*bestSolo];
            if (c.discount > b.discount ||
                (c.discount == b.discount && c.rule->priority() > b.rule->priority())) {
                bestSolo = i;
            }
        }
    }

    const bool soloWins = bestSolo && (!anyStackable || candidates[*bestSolo].discount > stackTotal);

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const Candidate& c = candidates[i];
        if (soloWins) {
            const Candidate& solo = candidates[*bestSolo];
            if (i == *bestSolo) {
                verdicts[i] = {true, anyStackable ? "not stackable, but saves more than the stack (" +
                                                        stackTotal.toString() + ")"
                                                  : "best non-stackable offer"};
            } else if (c.rule->stackable()) {
                verdicts[i] = {false, "non-stackable " + solo.rule->id() + " saves more (" +
                                          solo.discount.toString() + " vs stack " +
                                          stackTotal.toString() + ")"};
            } else {
                verdicts[i] = {false, "not stackable; " + solo.rule->id() + " saves more"};
            }
        } else if (c.rule->stackable()) {
            verdicts[i] = {true, "stacked with other stackable offers"};
        } else {
            verdicts[i] = {false, "not stackable; stacked offers save " + stackTotal.toString() +
                                      " together"};
        }
    }
    return verdicts;
}

} // namespace engine
