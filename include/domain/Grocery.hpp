#pragma once

#include <utility>

#include "domain/Date.hpp"
#include "domain/Perishable.hpp"
#include "domain/Product.hpp"

namespace domain {

/**
 * @brief A grocery product (5% GST) with an expiry date.
 *
 * OOP ideas: inheritance with extension - adds state (expiryDate) that only
 * groceries have, without touching the Product base class (Open/Closed).
 * Multiple inheritance of an interface - it is-a Product and also
 * implements the Perishable capability that ExpiryMarkdown relies on.
 */
class Grocery final : public Product, public Perishable {
public:
    Grocery(std::string id, std::string name, Money costPrice, Money sellingPrice, Money mrp,
            Date expiryDate)
        : Product(std::move(id), std::move(name), costPrice, sellingPrice, mrp),
          expiryDate_(expiryDate) {}

    int gstRate() const override { return 5; }
    std::string category() const override { return "Grocery"; }

    const Date& expiryDate() const noexcept override { return expiryDate_; }

private:
    Date expiryDate_;
};

} // namespace domain
