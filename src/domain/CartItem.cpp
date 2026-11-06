#include "domain/CartItem.hpp"

#include <stdexcept>
#include <string>

namespace domain {

CartItem::CartItem(const Product& product, int quantity) : product_(&product), quantity_(quantity) {
    if (quantity < 1) throw std::invalid_argument("Quantity must be at least 1");
    if (quantity > kMaxQuantity) {
        throw std::invalid_argument("Quantity must be at most " + std::to_string(kMaxQuantity));
    }
}

void CartItem::increaseQuantity(int extra) {
    if (extra < 1) throw std::invalid_argument("Extra quantity must be at least 1");
    // Compared this way round so the check itself cannot overflow.
    if (extra > kMaxQuantity - quantity_) {
        throw std::invalid_argument("A cart line can hold at most " +
                                    std::to_string(kMaxQuantity) + " units");
    }
    quantity_ += extra;
}

} // namespace domain
