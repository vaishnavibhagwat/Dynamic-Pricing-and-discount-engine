#pragma once

#include <iosfwd>

#include "application/WhatIfSimulator.hpp"

namespace presentation {

/**
 * @brief Prints the what-if grid as a table: one row per membership tier,
 *        one column per conflict policy, grand totals in the cells.
 *
 * "*" marks the customer's own tier and "<" the cheapest policy in each row.
 *
 * OOP ideas: Single Responsibility and layering - it only formats an
 * application::WhatIfReport; the numbers come ready-made from
 * WhatIfSimulator. Writes to any std::ostream, so tests print into a string.
 */
class WhatIfPrinter {
public:
    void print(std::ostream& os, const application::WhatIfReport& report) const;
};

} // namespace presentation
