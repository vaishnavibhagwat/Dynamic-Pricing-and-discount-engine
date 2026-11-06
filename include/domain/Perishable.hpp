#pragma once

#include "domain/Date.hpp"

namespace domain {

class Product;

/**
 * @brief Capability interface for anything that goes off on a known date.
 *
 * OOP ideas: Interface Segregation - only products that expire implement
 * it, so Product stays small. Dependency Inversion - ExpiryMarkdown depends
 * on this abstraction instead of the concrete Grocery class, so a new
 * perishable product (dairy, medicine...) gets expiry markdowns with no
 * change to the rule (Open/Closed).
 */
class Perishable {
public:
    virtual ~Perishable() = default;

    /// Last day the item may be sold.
    virtual const Date& expiryDate() const noexcept = 0;

    /// True once @p today is after the expiry date.
    bool isExpiredOn(const Date& today) const noexcept { return today > expiryDate(); }

protected:
    Perishable() = default;
    Perishable(const Perishable&) = default;
    Perishable& operator=(const Perishable&) = default;
};

/// The Perishable side of @p product, or nullptr if it never expires.
/// The one place that asks a Product for this capability.
const Perishable* asPerishable(const Product& product) noexcept;

/// True if @p product is perishable and already past its expiry date on @p day,
/// so it must not be sold.
bool isExpiredOn(const Product& product, const Date& day) noexcept;

} // namespace domain
