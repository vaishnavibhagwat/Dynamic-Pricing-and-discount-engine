#pragma once

#include <memory>
#include <string>
#include <vector>

#include "domain/Date.hpp"
#include "domain/Product.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"
#include "validation/ConflictReport.hpp"
#include "validation/RuleCheck.hpp"

namespace validation {

/**
 * @brief Looks at a freshly loaded rule set as a whole and reports problems:
 *        overlapping offers, expired offers, offers that can never apply and
 *        stacks that would break the margin guard.
 *
 * OOP ideas: Strategy and Dependency Injection - the checks are injected
 * as owned RuleCheck objects and run through their interface, so a new
 * check is a new class and this one does not change (Open/Closed). It only
 * coordinates; each check does one job (Single Responsibility).
 */
class RuleValidator {
public:
    /// ExpiredRuleCheck, UnreachableRuleCheck, OverlapCheck, MarginStackCheck.
    static std::vector<std::unique_ptr<RuleCheck>> defaultChecks();

    /// @throws std::invalid_argument if a check is null.
    explicit RuleValidator(std::vector<std::unique_ptr<RuleCheck>> checks = defaultChecks());

    /// Runs every check in order. @throws std::invalid_argument on a null rule or product.
    ConflictReport validate(const std::vector<const rules::DiscountRule*>& rules,
                            const std::vector<const domain::Product*>& catalog,
                            const domain::Date& asOf) const;
    ConflictReport validate(const rules::Repository<rules::DiscountRule>& rules,
                            const std::vector<const domain::Product*>& catalog,
                            const domain::Date& asOf) const;

    std::vector<std::string> checkNames() const;

private:
    std::vector<std::unique_ptr<RuleCheck>> checks_;
};

} // namespace validation
