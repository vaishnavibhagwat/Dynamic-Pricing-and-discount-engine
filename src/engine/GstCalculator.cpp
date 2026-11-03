#include "engine/GstCalculator.hpp"

#include <stdexcept>
#include <string>

namespace engine {

domain::Money GstCalculator::gstOn(domain::Money taxable, int ratePercent) const {
    if (ratePercent < 0 || ratePercent > 100) {
        throw std::invalid_argument("GstCalculator: rate must be 0..100, got " +
                                    std::to_string(ratePercent));
    }
    return taxable.percentOf(ratePercent);
}

domain::Money GstCalculator::gstOn(domain::Money taxable, const domain::Product& product) const {
    return gstOn(taxable, product.gstRate());
}

} // namespace engine
