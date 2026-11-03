#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "engine/PriceGuard.hpp"
#include "engine/PricingEngine.hpp"

namespace application {

/// Makes a fresh set of price guards. Application services build their own
/// PricingEngine for each run, and an engine owns its guards, so services are
/// given this factory instead of guard objects (Dependency Injection: tests
/// can pass no guards or a strict guard without touching the services).
using GuardFactory = std::function<std::vector<std::unique_ptr<engine::PriceGuard>>()>;

/// The engine's standard guards: discount cap, margin floor, MRP ceiling.
inline GuardFactory defaultGuardFactory() {
    return &engine::PricingEngine::defaultGuards;
}

} // namespace application
