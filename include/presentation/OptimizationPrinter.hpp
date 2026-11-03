#pragma once

#include <cstddef>
#include <iosfwd>

#include "application/DiscountOptimizer.hpp"

namespace presentation {

/**
 * @brief Prints what DiscountOptimizer found: the winning offers and their
 *        saving, the top few combinations tried, and why each offer was kept
 *        or screened out.
 *
 * OOP ideas: Single Responsibility and layering - it only formats an
 * application::OptimizationResult. Writes to any std::ostream.
 */
class OptimizationPrinter {
public:
    /// @p topN combinations are listed (at least 1).
    explicit OptimizationPrinter(std::size_t topN = 5);

    void print(std::ostream& os, const application::OptimizationResult& result) const;

private:
    std::size_t topN_;
};

} // namespace presentation
