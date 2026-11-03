#pragma once

#include <iosfwd>
#include <string>

#include "engine/TraceObserver.hpp"

namespace presentation {

/**
 * @brief Live decision log: writes each pricing decision to a stream the
 *        moment the engine takes it.
 *
 * OOP ideas: a concrete Observer. It subscribes to PricingEngine through the
 * engine::TraceObserver interface, so the engine never includes or knows
 * about this presentation class (Dependency Inversion). Liskov - it can be
 * used anywhere a TraceObserver is expected.
 */
class StreamTraceObserver : public engine::TraceObserver {
public:
    /// @p os must outlive this observer. Each line starts with @p indent.
    explicit StreamTraceObserver(std::ostream& os, std::string indent = "  ");

    void onDecision(const engine::TraceEntry& entry) override;

private:
    std::ostream* os_;
    std::string indent_;
};

} // namespace presentation
