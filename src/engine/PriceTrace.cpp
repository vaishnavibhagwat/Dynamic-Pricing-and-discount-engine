#include "engine/PriceTrace.hpp"

#include <algorithm>
#include <utility>

namespace engine {

void PriceTrace::record(TraceEntry entry) {
    entries_.push_back(std::move(entry));
}

std::vector<TraceEntry> PriceTrace::forTarget(const std::string& target) const {
    std::vector<TraceEntry> result;
    for (const auto& e : entries_) {
        if (e.target == target) result.push_back(e);
    }
    return result;
}

std::vector<TraceEntry> PriceTrace::forRule(const std::string& ruleId) const {
    std::vector<TraceEntry> result;
    for (const auto& e : entries_) {
        if (e.ruleId == ruleId) result.push_back(e);
    }
    return result;
}

std::size_t PriceTrace::count(TraceOutcome outcome) const noexcept {
    return static_cast<std::size_t>(std::count_if(
        entries_.begin(), entries_.end(),
        [outcome](const TraceEntry& e) { return e.outcome == outcome; }));
}

} // namespace engine
