#pragma once

#include <cstddef>
#include <string>

#include "rules/ConditionField.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace language {

/**
 * @brief Loads a text rule file (data/rules.txt) into the same repository as
 *        the JSON rules, so the engine sees one combined rule set.
 *
 * OOP ideas: Single Responsibility (only "file -> repository"; parsing is
 * RuleParser's job) and the Facade idea - callers need one call instead of
 * wiring a lexer, parser and catalog themselves. It owns its FieldCatalog by
 * value (composition), so it has no lifetime requirements.
 */
class RuleScriptLoader {
public:
    explicit RuleScriptLoader(rules::FieldCatalog fields = rules::FieldCatalog());

    /// Parses @p path and adds every rule to @p repo. All-or-nothing: on any
    /// error, nothing is added.
    /// @return number of rules added.
    /// @throws infrastructure::RuleParseException on a bad rule line,
    ///         infrastructure::ConfigError if the file cannot be read or a rule id
    ///         is already in @p repo.
    std::size_t loadInto(const std::string& path, rules::Repository<rules::DiscountRule>& repo) const;

    const rules::FieldCatalog& fields() const noexcept { return fields_; }

private:
    rules::FieldCatalog fields_;
};

} // namespace language
