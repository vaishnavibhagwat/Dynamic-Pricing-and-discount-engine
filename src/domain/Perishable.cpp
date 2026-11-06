#include "domain/Perishable.hpp"

#include "domain/Product.hpp"

namespace domain {

const Perishable* asPerishable(const Product& product) noexcept {
    return dynamic_cast<const Perishable*>(&product);
}

bool isExpiredOn(const Product& product, const Date& day) noexcept {
    const Perishable* perishable = asPerishable(product);
    return perishable != nullptr && perishable->isExpiredOn(day);
}

} // namespace domain
