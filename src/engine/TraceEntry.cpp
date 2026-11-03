#include "engine/TraceEntry.hpp"

#include <utility>

namespace engine {

using domain::Money;

std::string toString(TraceOutcome outcome) {
    switch (outcome) {
    case TraceOutcome::Applied: return "Applied";
    case TraceOutcome::Rejected: return "Rejected";
    case TraceOutcome::Adjusted: return "Adjusted";
    case TraceOutcome::Passed: return "Passed";
    }
    return "Unknown";
}

namespace {

TraceEntry make(std::string step, std::string ruleId, std::string ruleName, std::string target,
                TraceOutcome outcome, Money amount, std::string reason) {
    TraceEntry e;
    e.step = std::move(step);
    e.ruleId = std::move(ruleId);
    e.ruleName = std::move(ruleName);
    e.target = std::move(target);
    e.outcome = outcome;
    e.amount = amount;
    e.reason = std::move(reason);
    return e;
}

} // namespace

TraceEntry TraceEntry::applied(std::string step, std::string ruleId, std::string ruleName,
                               std::string target, Money amount, std::string reason) {
    return make(std::move(step), std::move(ruleId), std::move(ruleName), std::move(target),
                TraceOutcome::Applied, amount, std::move(reason));
}

TraceEntry TraceEntry::rejected(std::string step, std::string ruleId, std::string ruleName,
                                std::string target, std::string reason) {
    return make(std::move(step), std::move(ruleId), std::move(ruleName), std::move(target),
                TraceOutcome::Rejected, Money::zero(), std::move(reason));
}

TraceEntry TraceEntry::adjusted(std::string step, std::string ruleId, std::string ruleName,
                                std::string target, Money change, std::string reason) {
    return make(std::move(step), std::move(ruleId), std::move(ruleName), std::move(target),
                TraceOutcome::Adjusted, change, std::move(reason));
}

TraceEntry TraceEntry::passed(std::string step, std::string ruleId, std::string ruleName,
                              std::string target, std::string reason) {
    return make(std::move(step), std::move(ruleId), std::move(ruleName), std::move(target),
                TraceOutcome::Passed, Money::zero(), std::move(reason));
}

std::string TraceEntry::summary() const {
    std::string text = step + " " + ruleId + " ";
    switch (outcome) {
    case TraceOutcome::Applied:
        text += "applied " + amount.toString() + " off";
        break;
    case TraceOutcome::Rejected:
        text += "rejected";
        break;
    case TraceOutcome::Passed:
        text += "passed";
        break;
    case TraceOutcome::Adjusted:
        // Less discount (negative change) means the customer pays more.
        if (amount.isNegative()) {
            text += "raised price by " + (-amount).toString();
        } else {
            text += "lowered price by " + amount.toString();
        }
        break;
    }
    if (!reason.empty()) text += ": " + reason;
    return text;
}

} // namespace engine
