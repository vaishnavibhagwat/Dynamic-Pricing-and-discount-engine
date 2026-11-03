#include "language/RuleScriptLoader.hpp"

#include <utility>

#include "infrastructure/Exceptions.hpp"
#include "language/RuleParser.hpp"

namespace language {

RuleScriptLoader::RuleScriptLoader(rules::FieldCatalog fields) : fields_(std::move(fields)) {}

std::size_t RuleScriptLoader::loadInto(const std::string& path,
                                       rules::Repository<rules::DiscountRule>& repo) const {
    auto parsed = RuleParser::parseFile(path, fields_);
    // Check every id before adding any, so a failure leaves repo unchanged.
    for (const auto& rule : parsed) {
        if (repo.findById(rule->id()) != nullptr) {
            throw infrastructure::ConfigError("rule '" + rule->id() + "' in " + path +
                                              " has the same id as an already loaded rule");
        }
    }
    for (auto& rule : parsed) repo.add(std::move(rule));
    return parsed.size();
}

} // namespace language
