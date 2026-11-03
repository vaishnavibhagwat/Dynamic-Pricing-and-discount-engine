#include "domain/CartItem.hpp"

#include <stdexcept>

namespace domain {

CartItem::CartItem(const Product& product, int quantity) : product_(&product), quantity_(quantity) {
    if (quantity < 1) throw std::invalid_argument("Quantity must be at least 1");
}

void CartItem::increaseQuantity(int extra) {
    if (extra < 1) throw std::invalid_argument("Extra quantity must be at least 1");
    quantity_ += extra;
}

} // namespace domain
