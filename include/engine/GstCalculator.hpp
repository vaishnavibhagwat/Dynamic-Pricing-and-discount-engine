#pragma once

#include "domain/Money.hpp"
#include "domain/Product.hpp"

namespace engine {

/**
 * @brief Works out GST on a taxable amount.
 *
 * OOP ideas: Single Responsibility - tax arithmetic lives in one place, used
 * by both the engine and MrpCeilingPolicy. The rate itself comes from the
 * product through the virtual Product::gstRate() (polymorphism).
 */
class GstCalculator {
public:
    /// ratePercent% of @p taxable, rounded to the nearest paisa.
    /// @throws std::invalid_argument if ratePercent is outside 0..100.
    domain::Money gstOn(domain::Money taxable, int ratePercent) const;
    /// GST on @p taxable at the product's own rate.
    domain::Money gstOn(domain::Money taxable, const domain::Product& product) const;
};

} // namespace engine
