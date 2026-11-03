#pragma once

#include <optional>
#include <string>
#include <utility>

#include "domain/Cart.hpp"
#include "domain/Customer.hpp"
#include "domain/Date.hpp"
#include "domain/TimeOfDay.hpp"

namespace domain {

/**
 * @brief Everything a pricing rule may look at, bundled in one read-only view.
 *
 * OOP ideas: Dependency Inversion and Interface Segregation - rules depend
 * on this small, stable, immutable object instead of reaching into the
 * checkout service. It only refers to the cart and customer, so both must
 * outlive the context.
 */
class PricingContext {
public:
    PricingContext(const Cart& cart, const Customer& customer, Date orderDate,
                   std::optional<std::string> couponCode = std::nullopt,
                   std::optional<std::string> paymentBank = std::nullopt,
                   std::optional<TimeOfDay> orderTime = std::nullopt)
        : cart_(cart), customer_(customer), orderDate_(orderDate),
          couponCode_(std::move(couponCode)), paymentBank_(std::move(paymentBank)),
          orderTime_(orderTime) {}

    // Binding to temporaries would leave dangling references.
    PricingContext(Cart&&, const Customer&, Date, std::optional<std::string> = std::nullopt,
                   std::optional<std::string> = std::nullopt,
                   std::optional<TimeOfDay> = std::nullopt) = delete;
    PricingContext(const Cart&, Customer&&, Date, std::optional<std::string> = std::nullopt,
                   std::optional<std::string> = std::nullopt,
                   std::optional<TimeOfDay> = std::nullopt) = delete;

    const Cart& cart() const noexcept { return cart_; }
    const Customer& customer() const noexcept { return customer_; }
    const Date& orderDate() const noexcept { return orderDate_; }
    const std::optional<std::string>& couponCode() const noexcept { return couponCode_; }
    /// Bank of the card/UPI used to pay, e.g. "HDFC" (for bank offers).
    const std::optional<std::string>& paymentBank() const noexcept { return paymentBank_; }
    /// Clock time of the order, for time-limited offers; nullopt if unknown.
    const std::optional<TimeOfDay>& orderTime() const noexcept { return orderTime_; }

private:
    const Cart& cart_;
    const Customer& customer_;
    Date orderDate_;
    std::optional<std::string> couponCode_;
    std::optional<std::string> paymentBank_;
    std::optional<TimeOfDay> orderTime_;
};

} // namespace domain
