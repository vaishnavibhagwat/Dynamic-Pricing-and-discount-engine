#include "engine/TracePublisher.hpp"

#include <algorithm>

namespace engine {

void TracePublisher::subscribe(TraceObserver& observer) {
    if (std::find(observers_.begin(), observers_.end(), &observer) == observers_.end()) {
        observers_.push_back(&observer);
    }
}

void TracePublisher::unsubscribe(const TraceObserver& observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), &observer),
                     observers_.end());
}

void TracePublisher::publish(const TraceEntry& entry) const {
    for (TraceObserver* o : observers_) o->onDecision(entry);
}

} // namespace engine
