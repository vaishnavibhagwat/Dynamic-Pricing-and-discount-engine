#pragma once

#include "domain/Money.hpp"
#include "domain/Product.hpp"

namespace domain {

/**
 * @brief One line in a cart: a product and how many units of it.
 *
 * OOP idea: composition ("has-a" a Product). The item does not own the
 * product - the catalogue (Repository) does, and must outlive the cart.
 * Holding a `const Product&` keeps polymorphism, so lineTotal() and GST
 * work for every product type.
 */
class CartItem {
public:
    /// Most units one cart line may hold. Keeps totals far from integer overflow.
    static constexpr int kMaxQuantity = 1000;

    /// @throws std::invalid_argument unless 1 <= quantity <= kMaxQuantity.
    CartItem(const Product& product, int quantity);

    const Product& product() const noexcept { return *product_; }
    int quantity() const noexcept { return quantity_; }

    /// sellingPrice x quantity
    Money lineTotal() const noexcept { return product_->sellingPrice() * quantity_; }
    /// mrp x quantity
    Money mrpTotal() const noexcept { return product_->mrp() * quantity_; }

    /// Adds more units of the same product. Leaves the item unchanged on error.
    /// @throws std::invalid_argument if extra < 1 or the total would pass kMaxQuantity.
    void increaseQuantity(int extra);

private:
    const Product* product_; // non-owning; never null
    int quantity_;
};

} // namespace domain
