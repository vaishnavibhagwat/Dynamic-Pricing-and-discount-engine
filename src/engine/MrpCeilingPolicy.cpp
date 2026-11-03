#include "engine/MrpCeilingPolicy.hpp"

#include <cstdint>
#include <string>

namespace engine {

domain::Money MrpCeilingPolicy::ceilingFor(const domain::CartItem& item) const noexcept {
    // net x (100 + gst) / 100 <= mrp  =>  net <= mrp x 100 / (100 + gst), rounded down.
    const std::int64_t mrp = item.mrpTotal().paise();
    const int rate = item.product().gstRate();
    return domain::Money::fromPaise(mrp * 100 / (100 + rate));
}

GuardOutcome MrpCeilingPolicy::limit(const domain::CartItem& item,
                                     domain::Money proposedDiscount) const {
    const domain::Money net = item.lineTotal() - proposedDiscount;
    const int rate = item.product().gstRate();
    const domain::Money withGst = net + gst_.gstOn(net, rate);
    if (withGst <= item.mrpTotal()) {
        return {proposedDiscount, "price incl. " + std::to_string(rate) + "% GST " +
                                      withGst.toString() + " is within MRP " +
                                      item.mrpTotal().toString()};
    }
    const domain::Money ceiling = ceilingFor(item);
    return {item.lineTotal() - ceiling,
            "price incl. " + std::to_string(rate) + "% GST would exceed MRP " +
                item.mrpTotal().toString() + "; net lowered to " + ceiling.toString()};
}

} // namespace engine
