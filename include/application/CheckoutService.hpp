#pragma once

#include "application/GuardFactory.hpp"
#include "domain/Customer.hpp"
#include "domain/PricingContext.hpp"
#include "engine/ConflictResolver.hpp"
#include "engine/PricingResult.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace application {

/**
 * @brief Prices an order and, on checkout, records that it was placed.
 *
 * quote() is a pure "what would this cost" call. checkout() prices the
 * order the same way and then tells every offer that was actually used and
 * keeps a usage count (rules::IRedeemable, e.g. a coupon with a limit) to
 * count one use, so a one-time coupon is refused on the next order.
 *
 * OOP ideas: Facade and layering - the CLI asks for a checkout without
 * wiring an engine, guards and resolver itself. Dependency Injection - the
 * rule repository, guards and conflict policy are passed in. Open/Closed -
 * usage is recorded through the IRedeemable interface, so a new limited
 * offer needs no change here.
 */
class CheckoutService {
public:
    /// @p rules must outlive the service; it is read on every call, so a
    /// reloaded repository is picked up automatically.
    /// @throws std::invalid_argument if @p guards is empty.
    explicit CheckoutService(rules::Repository<rules::DiscountRule>& rules,
                             GuardFactory guards = defaultGuardFactory());

    /// Prices the order; changes nothing.
    engine::PricingResult quote(const domain::PricingContext& ctx,
                                const engine::ConflictResolver& resolver) const;

    /// Prices the order, then records one use of each applied IRedeemable offer.
    /// @throws infrastructure::PricingError if the cart holds an item that has
    ///         expired by the order date (it must not be sold); nothing is recorded.
    engine::PricingResult checkout(const domain::PricingContext& ctx,
                                   const engine::ConflictResolver& resolver);
    /// As above, and also counts the order on @p buyer's history (so
    /// "first order only" offers stop applying).
    /// @throws std::invalid_argument if @p buyer is not the customer in @p ctx.
    engine::PricingResult checkout(const domain::PricingContext& ctx,
                                   const engine::ConflictResolver& resolver,
                                   domain::Customer& buyer);

private:
    rules::Repository<rules::DiscountRule>& rules_;
    GuardFactory guards_;
};

} // namespace application
