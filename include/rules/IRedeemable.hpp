#pragma once

namespace rules {

/**
 * @brief Interface for offers that count how often they have been used
 *        (e.g. a coupon with a usage limit).
 *
 * OOP ideas: Interface Segregation Principle - only offers with usage state
 * implement it, so DiscountRule stays free of a method most rules would
 * ignore. Open/Closed - CheckoutService records a use through this
 * interface, so a new limited offer needs no change to checkout.
 */
class IRedeemable {
public:
    virtual ~IRedeemable() = default;

    /// Counts one use after an order is placed.
    /// @throws std::logic_error if no uses are left.
    virtual void recordUse() = 0;

protected:
    IRedeemable() = default;
    IRedeemable(const IRedeemable&) = default;
    IRedeemable& operator=(const IRedeemable&) = default;
};

} // namespace rules
