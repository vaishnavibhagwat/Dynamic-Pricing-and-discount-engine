#include "engine/MarginGuard.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace engine {

MarginGuard::MarginGuard(int minMarkupPercent) : minMarkupPercent_(minMarkupPercent) {
    if (minMarkupPercent_ < 0) {
        throw std::invalid_argument("MarginGuard: minMarkupPercent must not be negative");
    }
}

domain::Money MarginGuard::floorFor(const domain::CartItem& item) const noexcept {
    const std::int64_t cost = item.product().costPrice().paise() * item.quantity();
    const std::int64_t scaled = cost * (100 + minMarkupPercent_);
    // Round up so the margin is never even a paisa short.
    return domain::Money::fromPaise(scaled / 100 + (scaled % 100 > 0 ? 1 : 0));
}

GuardOutcome MarginGuard::limit(const domain::CartItem& item,
                                domain::Money proposedDiscount) const {
    const domain::Money floor = floorFor(item);
    const domain::Money maxDiscount = std::max(item.lineTotal() - floor, domain::Money::zero());
    if (proposedDiscount <= maxDiscount) {
        return {proposedDiscount, "net " + (item.lineTotal() - proposedDiscount).toString() +
                                      " keeps cost + " + std::to_string(minMarkupPercent_) +
                                      "% floor of " + floor.toString()};
    }
    return {maxDiscount, "net price would fall below cost + " +
                             std::to_string(minMarkupPercent_) + "% (" + floor.toString() + ")"};
}

} // namespace engine
