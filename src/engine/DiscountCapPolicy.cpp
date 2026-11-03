#include "engine/DiscountCapPolicy.hpp"

#include <stdexcept>
#include <string>

namespace engine {

DiscountCapPolicy::DiscountCapPolicy(int maxPercent) : maxPercent_(maxPercent) {
    if (maxPercent_ < 0 || maxPercent_ > 100) {
        throw std::invalid_argument("DiscountCapPolicy: maxPercent must be 0..100, got " +
                                    std::to_string(maxPercent_));
    }
}

GuardOutcome DiscountCapPolicy::limit(const domain::CartItem& item,
                                      domain::Money proposedDiscount) const {
    const domain::Money cap = item.lineTotal().percentOf(maxPercent_);
    if (proposedDiscount <= cap) {
        return {proposedDiscount, "discount " + proposedDiscount.toString() + " is within the " +
                                      std::to_string(maxPercent_) + "% cap of " + cap.toString()};
    }
    return {cap, "discount " + proposedDiscount.toString() + " is over the " +
                     std::to_string(maxPercent_) + "% cap of " + cap.toString()};
}

} // namespace engine
