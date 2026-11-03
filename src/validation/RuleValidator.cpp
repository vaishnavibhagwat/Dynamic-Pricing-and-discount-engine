#include "validation/RuleValidator.hpp"

#include <stdexcept>
#include <utility>

#include "validation/ExpiredRuleCheck.hpp"
#include "validation/MarginStackCheck.hpp"
#include "validation/OverlapCheck.hpp"
#include "validation/UnreachableRuleCheck.hpp"

namespace validation {

std::vector<std::unique_ptr<RuleCheck>> RuleValidator::defaultChecks() {
    std::vector<std::unique_ptr<RuleCheck>> checks;
    checks.push_back(std::make_unique<ExpiredRuleCheck>());
    checks.push_back(std::make_unique<UnreachableRuleCheck>());
    checks.push_back(std::make_unique<OverlapCheck>());
    checks.push_back(std::make_unique<MarginStackCheck>());
    return checks;
}

RuleValidator::RuleValidator(std::vector<std::unique_ptr<RuleCheck>> checks)
    : checks_(std::move(checks)) {
    for (const auto& c : checks_) {
        if (!c) throw std::invalid_argument("RuleValidator: null check");
    }
}

ConflictReport RuleValidator::validate(const std::vector<const rules::DiscountRule*>& rules,
                                       const std::vector<const domain::Product*>& catalog,
                                       const domain::Date& asOf) const {
    for (const auto* r : rules) {
        if (r == nullptr) throw std::invalid_argument("RuleValidator::validate: null rule");
    }
    for (const auto* p : catalog) {
        if (p == nullptr) throw std::invalid_argument("RuleValidator::validate: null product");
    }

    const ValidationContext ctx{rules, catalog, asOf};
    ConflictReport report(asOf);
    for (const auto& c : checks_) {
        for (auto& issue : c->check(ctx)) report.add(std::move(issue));
    }
    return report;
}

ConflictReport RuleValidator::validate(const rules::Repository<rules::DiscountRule>& rules,
                                       const std::vector<const domain::Product*>& catalog,
                                       const domain::Date& asOf) const {
    return validate(rules.all(), catalog, asOf);
}

std::vector<std::string> RuleValidator::checkNames() const {
    std::vector<std::string> names;
    for (const auto& c : checks_) names.push_back(c->name());
    return names;
}

} // namespace validation
