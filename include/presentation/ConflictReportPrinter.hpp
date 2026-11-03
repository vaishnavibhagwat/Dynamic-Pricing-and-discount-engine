#pragma once

#include <iosfwd>

#include "validation/ConflictReport.hpp"

namespace presentation {

/**
 * @brief Prints the rule validator's findings as a short startup report,
 *        one line per problem.
 *
 * OOP ideas: Single Responsibility and layering - it only formats a
 * validation::ConflictReport; finding problems is RuleValidator's job.
 * Writes to any std::ostream, so tests print into a string stream.
 */
class ConflictReportPrinter {
public:
    void print(std::ostream& os, const validation::ConflictReport& report) const;
};

} // namespace presentation
