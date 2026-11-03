#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "application/UpsellAdvisor.hpp"

namespace presentation {

/**
 * @brief Prints the upsell advisor's suggestions as short, plain sentences
 *        a shopper can act on.
 *
 * OOP ideas: Single Responsibility and layering - it only formats
 * application::UpsellSuggestion values and writes to any std::ostream.
 */
class UpsellPrinter {
public:
    void print(std::ostream& os, const std::vector<application::UpsellSuggestion>& suggestions,
               const std::string& policyName) const;
};

} // namespace presentation
