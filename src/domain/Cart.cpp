#include "domain/Cart.hpp"

#include <algorithm>

namespace domain {

Cart& Cart::operator+=(const CartItem& item) {
    const auto existing = std::find_if(items_.begin(), items_.end(), [&](const CartItem& line) {
        return line.product().id() == item.product().id();
    });
    if (existing != items_.end()) {
        existing->increaseQuantity(item.quantity());
    } else {
        items_.push_back(item);
    }
    return *this;
}

bool Cart::remove(const std::string& productId) {
    const auto line = std::find_if(items_.begin(), items_.end(), [&](const CartItem& item) {
        return item.product().id() == productId;
    });
    if (line == items_.end()) return false;
    items_.erase(line);
    return true;
}

bool Cart::contains(const std::string& productId) const noexcept {
    return std::any_of(items_.begin(), items_.end(),
                       [&](const CartItem& line) { return line.product().id() == productId; });
}

Money Cart::subtotal() const noexcept {
    Money total;
    for (const auto& line : items_) total += line.lineTotal();
    return total;
}

Money Cart::mrpTotal() const noexcept {
    Money total;
    for (const auto& line : items_) total += line.mrpTotal();
    return total;
}

int Cart::totalQuantity() const noexcept {
    int total = 0;
    for (const auto& line : items_) total += line.quantity();
    return total;
}

} // namespace domain
