#include "domain/Product.hpp"

#include <stdexcept>
#include <utility>

namespace domain {

Product::Product(std::string id, std::string name, Money costPrice, Money sellingPrice, Money mrp)
    : id_(std::move(id)),
      name_(std::move(name)),
      costPrice_(costPrice),
      sellingPrice_(sellingPrice),
      mrp_(mrp) {
    if (id_.empty()) throw std::invalid_argument("Product id must not be empty");
    if (name_.empty()) throw std::invalid_argument("Product name must not be empty");
    if (costPrice_.isNegative() || sellingPrice_.isNegative() || mrp_.isNegative()) {
        throw std::invalid_argument("Product prices must not be negative: " + id_);
    }
    if (sellingPrice_ > mrp_) {
        throw std::invalid_argument("Selling price exceeds MRP for product " + id_);
    }
}

} // namespace domain
