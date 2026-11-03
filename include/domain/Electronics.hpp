#pragma once

#include "domain/Product.hpp"

namespace domain {

/**
 * @brief An electronics product (18% GST).
 *
 * OOP idea: inheritance and runtime polymorphism - overrides the pure
 * virtual gstRate() and category() from Product.
 */
class Electronics final : public Product {
public:
    using Product::Product;

    int gstRate() const override { return 18; }
    std::string category() const override { return "Electronics"; }
};

} // namespace domain
