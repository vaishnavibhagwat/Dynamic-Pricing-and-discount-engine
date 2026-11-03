#pragma once

#include <string>

#include "domain/Money.hpp"

namespace engine {

/// What happened to a rule or guard when the engine considered it.
enum class TraceOutcome {
    Applied,  ///< offer was given
    Rejected, ///< offer was not given
    Adjusted, ///< a guard changed the line discount
    Passed    ///< a guard checked the line and left it unchanged
};

std::string toString(TraceOutcome outcome);

/**
 * @brief One decision taken while pricing an order.
 *
 * OOP ideas: a value object built through named constructors
 * (applied(), rejected(), adjusted(), passed()), so every entry has a
 * consistent outcome/amount pair. summary() keeps the wording of a decision
 * in one place for every reader (invoice, live log, tests).
 */
struct TraceEntry {
    std::string step;     ///< e.g. "Seasonal", "Coupon", "Guard"
    std::string ruleId;   ///< rule id, or guard name
    std::string ruleName; ///< human-readable rule or guard name
    std::string target;   ///< product id, or PriceTrace::kOrder for the whole order
    TraceOutcome outcome = TraceOutcome::Rejected;
    /// Applied: discount given. Adjusted: change to the line discount (negative = less
    /// discount). Rejected and Passed: zero.
    domain::Money amount;
    std::string reason;

    static TraceEntry applied(std::string step, std::string ruleId, std::string ruleName,
                              std::string target, domain::Money amount, std::string reason = {});
    static TraceEntry rejected(std::string step, std::string ruleId, std::string ruleName,
                               std::string target, std::string reason);
    static TraceEntry adjusted(std::string step, std::string ruleId, std::string ruleName,
                               std::string target, domain::Money change, std::string reason);
    static TraceEntry passed(std::string step, std::string ruleId, std::string ruleName,
                             std::string target, std::string reason);

    /// One readable line, e.g. "Coupon SAVE10 rejected: cart value ₹500.00 is below minimum
    /// ₹750.00" or "Guard MarginGuard raised price by ₹40.00: ...".
    std::string summary() const;
};

} // namespace engine
