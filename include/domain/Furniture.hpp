#pragma once

#include <utility>

#include "domain/Product.hpp"

namespace domain {

/**
 * @brief A furniture product (18% GST) that may need assembly.
 *
 * OOP idea: inheritance with extension - adds assemblyRequired while
 * reusing all common product behaviour from the base class.
 */
class Furniture final : public Product {
public:
    Furniture(std::string id, std::string name, Money costPrice, Money sellingPrice, Money mrp,
              bool assemblyRequired)
        : Product(std::move(id), std::move(name), costPrice, sellingPrice, mrp),
          assemblyRequired_(assemblyRequired) {}

    int gstRate() const override { return 18; }
    std::string category() const override { return "Furniture"; }

    bool assemblyRequired() const noexcept { return assemblyRequired_; }

private:
    bool assemblyRequired_;
};

} // namespace domain
