#pragma once

#include <string>
#include <vector>

#include "domain/Date.hpp"
#include "domain/Product.hpp"
#include "rules/DiscountRule.hpp"
#include "validation/ConflictReport.hpp"

namespace validation {

/// What every check looks at: the loaded rules, the product catalog and today's date.
/// Holds references only; the caller keeps the rules and products alive.
struct ValidationContext {
    const std::vector<const rules::DiscountRule*>& rules;
    const std::vector<const domain::Product*>& catalog;
    domain::Date asOf;
};

/// True if @p rule has a date window that ended before @p asOf.
bool isExpired(const rules::DiscountRule& rule, const domain::Date& asOf);

/**
 * @brief One kind of sanity check over the whole rule set.
 *
 * OOP ideas: Strategy pattern and the Open/Closed Principle - RuleValidator
 * runs a list of checks through this pure virtual interface, so a new kind
 * of check is a new subclass and the validator does not change.
 */
class RuleCheck {
public:
    virtual ~RuleCheck() = default;

    virtual std::string name() const = 0;
    /// Problems found, in a stable order (rule-file order, then catalog order).
    virtual std::vector<RuleIssue> check(const ValidationContext& ctx) const = 0;

protected:
    RuleCheck() = default;
    RuleCheck(const RuleCheck&) = default;
    RuleCheck& operator=(const RuleCheck&) = default;
};

} // namespace validation
