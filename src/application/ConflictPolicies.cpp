#include "application/ConflictPolicies.hpp"

#include "engine/BestDiscountResolver.hpp"
#include "engine/PriorityResolver.hpp"
#include "engine/StackingResolver.hpp"

namespace application {

ConflictPolicies standardConflictPolicies() {
    ConflictPolicies policies;
    policies.push_back(std::make_unique<engine::StackingResolver>());
    policies.push_back(std::make_unique<engine::BestDiscountResolver>());
    policies.push_back(std::make_unique<engine::PriorityResolver>());
    return policies;
}

} // namespace application
