#include "engine/PriorityResolver.hpp"

#include <cstddef>

namespace engine {

std::vector<Verdict> PriorityResolver::resolve(const std::vector<Candidate>& candidates) const {
    std::vector<Verdict> verdicts(candidates.size());
    if (candidates.empty()) return verdicts;

    std::size_t top = 0;
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        const Candidate& c = candidates[i];
        const Candidate& t = candidates[top];
        if (c.rule->priority() > t.rule->priority() ||
            (c.rule->priority() == t.rule->priority() && c.discount > t.discount)) {
            top = i;
        }
    }

    const Candidate& winner = candidates[top];
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (i == top) {
            verdicts[i] = {true, "highest priority (" + std::to_string(winner.rule->priority()) + ")"};
        } else {
            verdicts[i] = {false, winner.rule->id() + " has higher priority (" +
                                      std::to_string(winner.rule->priority()) + " vs " +
                                      std::to_string(candidates[i].rule->priority()) + ")"};
        }
    }
    return verdicts;
}

} // namespace engine
