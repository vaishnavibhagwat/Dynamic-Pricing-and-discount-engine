#include "presentation/StreamTraceObserver.hpp"

#include <ostream>
#include <utility>

#include "engine/PriceTrace.hpp"

namespace presentation {

StreamTraceObserver::StreamTraceObserver(std::ostream& os, std::string indent)
    : os_(&os), indent_(std::move(indent)) {}

void StreamTraceObserver::onDecision(const engine::TraceEntry& entry) {
    const std::string where =
        entry.target == engine::PriceTrace::kOrder ? std::string() : " [" + entry.target + "]";
    *os_ << indent_ << entry.summary() << where << '\n';
}

} // namespace presentation
