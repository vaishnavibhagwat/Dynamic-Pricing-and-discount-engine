#pragma once

#include <cstddef>
#include <vector>

#include "engine/TraceEntry.hpp"
#include "engine/TraceObserver.hpp"

namespace engine {

/**
 * @brief Sends each pricing decision to every subscribed TraceObserver.
 *
 * OOP ideas: the Observer pattern (this is the subject) and Single
 * Responsibility - it only keeps the observer list and fans entries out.
 * Observers are not owned; each must outlive the publisher or unsubscribe.
 */
class TracePublisher {
public:
    /// Adds @p observer. Subscribing the same observer twice has no effect.
    void subscribe(TraceObserver& observer);
    /// Removes @p observer. Does nothing if it was not subscribed.
    void unsubscribe(const TraceObserver& observer);
    std::size_t observerCount() const noexcept { return observers_.size(); }

    /// Calls onDecision() on every observer, in subscription order.
    void publish(const TraceEntry& entry) const;

private:
    std::vector<TraceObserver*> observers_;
};

} // namespace engine
