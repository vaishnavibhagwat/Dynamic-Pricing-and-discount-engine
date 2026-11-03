#pragma once

#include "domain/Date.hpp"

namespace domain {

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

} // namespace domain
