#include "validation/MarginStackCheck.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "domain/CartItem.hpp"
#include "engine/MarginGuard.hpp"

namespace validation {

using domain::Money;

namespace {

/// One rule's part in a stack on one product.
struct Part {
    const rules::DiscountRule* rule;
    int percent;
};

/// The rules running together on one day.
struct Stack {
    domain::Date day;
    std::vector<Part> parts;
    Money discount;
    int percent = 0;
};

std::string describe(const domain::Product& product, const Stack& stack) {
    std::string text = product.name() + " on " + stack.day.toString() + ": ";
    for (std::size_t i = 0; i < stack.parts.size(); ++i) {
        if (i > 0) text += " + ";
        text += stack.parts[i].rule->id() + " " + std::to_string(stack.parts[i].percent) + "%";
    }
    text += " = " + std::to_string(stack.percent) + "% off " + product.sellingPrice().toString();
    return text;
}

} // namespace

MarginStackCheck::MarginStackCheck() : MarginStackCheck(std::make_unique<engine::MarginGuard>()) {}

MarginStackCheck::MarginStackCheck(std::unique_ptr<engine::PriceGuard> guard)
    : guard_(std::move(guard)) {
    if (!guard_) throw std::invalid_argument("MarginStackCheck: guard must not be null");
}

std::vector<RuleIssue> MarginStackCheck::check(const ValidationContext& ctx) const {
    // Stackable rules that can still run. The most date windows are open at once
    // on the first day of some window (or today), so only those days are checked.
    std::vector<const rules::DiscountRule*> live;
    std::vector<domain::Date> days{ctx.asOf};
    for (const auto* rule : ctx.rules) {
        if (!rule->stackable() || isExpired(*rule, ctx.asOf)) continue;
        live.push_back(rule);
        if (const auto period = rule->activePeriod()) {
            days.push_back(std::max(period->start(), ctx.asOf));
        }
    }

    std::vector<RuleIssue> issues;
    for (const auto* product : ctx.catalog) {
        const domain::CartItem oneUnit(*product, 1);
        std::optional<Stack> worst;
        std::string worstReason;

        for (const auto& day : days) {
            Stack stack{day, {}, Money::zero(), 0};
            for (const auto* rule : live) {
                const auto period = rule->activePeriod();
                if (period && !period->contains(day)) continue;
                const auto percent = rule->maxPercentOff(*product);
                if (!percent) continue;
                stack.parts.push_back({rule, *percent});
                stack.percent += *percent;
                // Like the engine: each offer is worked out on the line total, then summed.
                stack.discount += oneUnit.lineTotal().percentOf(*percent);
            }
            if (stack.parts.empty()) continue;

            const engine::GuardOutcome out = guard_->limit(oneUnit, stack.discount);
            if (out.discount >= stack.discount) continue; // the guard lets it through
            if (!worst || stack.discount > worst->discount) {
                worst = std::move(stack);
                worstReason = out.reason;
            }
        }

        if (!worst) continue;
        std::vector<std::string> ids;
        for (const auto& part : worst->parts) ids.push_back(part.rule->id());
        issues.push_back({IssueKind::MarginBreach, std::move(ids), product->id(),
                          describe(*product, *worst) + "; " + worstReason});
    }
    return issues;
}

} // namespace validation
