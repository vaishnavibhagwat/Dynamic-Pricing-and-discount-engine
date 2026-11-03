#pragma once

#include <string>

#include "domain/Money.hpp"

namespace domain {

/**
 * @brief Abstract base for everything that can be sold.
 *
 * OOP ideas: abstraction (pure virtual gstRate() and category()), and the
 * Open/Closed Principle - a new kind of product is added by writing a new
 * subclass, not by editing existing code. Subclasses must honour the same
 * contract (Liskov Substitution), so any code holding a `const Product&`
 * works for every product type. Copying is disabled to prevent slicing.
 */
class Product {
public:
    /// @throws std::invalid_argument on empty id/name, negative prices,
    ///         or sellingPrice > mrp.
    Product(std::string id, std::string name, Money costPrice, Money sellingPrice, Money mrp);
    virtual ~Product() = default;

    Product(const Product&) = delete;
    Product& operator=(const Product&) = delete;
    Product(Product&&) = delete;
    Product& operator=(Product&&) = delete;

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    Money costPrice() const noexcept { return costPrice_; }
    Money sellingPrice() const noexcept { return sellingPrice_; }
    Money mrp() const noexcept { return mrp_; }

    /// Profit per unit at the selling price (may be negative for loss leaders).
    Money margin() const noexcept { return sellingPrice_ - costPrice_; }

    /// GST rate as a whole percentage, e.g. 18 for 18%.
    virtual int gstRate() const = 0;
    /// Human-readable category name, e.g. "Electronics".
    virtual std::string category() const = 0;

private:
    std::string id_;
    std::string name_;
    Money costPrice_;
    Money sellingPrice_;
    Money mrp_;
};

} // namespace domain
