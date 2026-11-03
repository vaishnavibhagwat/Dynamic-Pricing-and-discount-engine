#pragma once

#include <string>
#include <vector>

#include "domain/Money.hpp"
#include "rules/DiscountRule.hpp"

namespace engine {

/// An offer that could be used, and what it would save on its own.
struct Candidate {
    const rules::DiscountRule* rule; ///< non-owning, never null
    domain::Money discount;
};

/// The resolver's decision for one candidate.
struct Verdict {
    bool selected = false;
    std::string reason; ///< why it was chosen or dropped (shown in the price trace)
};

/**
 * @brief Decides which of several competing offers are actually given.
 *
 * OOP ideas: Strategy pattern and Dependency Inversion - PricingEngine is
 * given a ConflictResolver by reference and never knows which policy it is,
 * so a new policy is a new subclass (Open/Closed). Pure virtual interface
 * with a virtual destructor.
 */
class ConflictResolver {
public:
    virtual ~ConflictResolver() = default;

    /// Short policy name for invoices and traces, e.g. "BestDiscount".
    virtual std::string name() const = 0;

    /// Returns one Verdict per candidate, in the same order. Candidates arrive in
    /// stage order and all have a positive discount.
    virtual std::vector<Verdict> resolve(const std::vector<Candidate>& candidates) const = 0;

protected:
    ConflictResolver() = default;
    ConflictResolver(const ConflictResolver&) = default;
    ConflictResolver& operator=(const ConflictResolver&) = default;
};

} // namespace engine
