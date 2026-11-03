#pragma once

#include <memory>
#include <vector>

#include "engine/ConflictResolver.hpp"

namespace application {

/// A list of conflict policies the user can choose between.
using ConflictPolicies = std::vector<std::unique_ptr<engine::ConflictResolver>>;

/// The standard policies, in menu order: Stacking, BestDiscount, Priority.
/// Front ends build their policy menu and what-if columns from this list,
/// so a new policy is one new line here and no menu code changes
/// (Open/Closed).
ConflictPolicies standardConflictPolicies();

} // namespace application
