#pragma once

#include "engine/TraceEntry.hpp"

namespace engine {

/**
 * @brief Anything that wants to hear about pricing decisions as they happen.
 *
 * OOP ideas: the Observer pattern (this is the observer interface) and
 * Dependency Inversion - PricingEngine only knows this abstraction, so
 * collectors, live loggers or UI views can listen without the engine ever
 * depending on them. Pure virtual onDecision() = abstraction/polymorphism.
 */
class TraceObserver {
public:
    virtual ~TraceObserver() = default;

    /// Called once for every decision, in the order the engine takes them.
    virtual void onDecision(const TraceEntry& entry) = 0;

protected:
    TraceObserver() = default;
    TraceObserver(const TraceObserver&) = default;
    TraceObserver& operator=(const TraceObserver&) = default;
    TraceObserver(TraceObserver&&) = default;
    TraceObserver& operator=(TraceObserver&&) = default;
};

} // namespace engine
