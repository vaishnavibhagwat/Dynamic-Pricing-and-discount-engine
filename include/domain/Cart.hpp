#pragma once

#include <string>
#include <vector>

#include "domain/CartItem.hpp"
#include "domain/Money.hpp"

namespace domain {

/**
 * @brief A shopping cart: an ordered list of CartItems.
 *
 * OOP ideas: composition (a Cart has CartItems) and operator overloading
 * (`cart += item`). Adding a product that is already in the cart merges the
 * quantities instead of creating a duplicate line, so the cart's invariant
 * "one line per product" is always kept.
 */
class Cart {
public:
    Cart& operator+=(const CartItem& item);
    /// Removes the line for @p productId. @return false if there was no such line.
    bool remove(const std::string& productId);

    const std::vector<CartItem>& items() const noexcept { return items_; }
    bool empty() const noexcept { return items_.empty(); }
    bool contains(const std::string& productId) const noexcept;

    /// Sum of selling price x quantity over all lines.
    Money subtotal() const noexcept;
    /// Sum of MRP x quantity over all lines.
    Money mrpTotal() const noexcept;
    /// Total number of units across all lines.
    int totalQuantity() const noexcept;

private:
    std::vector<CartItem> items_;
};

} // namespace domain
