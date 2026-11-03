#pragma once

#include <string>

namespace rules {

/**
 * @brief The step of the checkout at which an offer is applied, in order.
 *
 * Item stages (Expiry .. Membership) discount single cart lines before the
 * price guards run; cart stages (Coupon, BankOffer) discount the order total
 * after the guards. Each rule reports its own stage, so the pricing engine
 * sorts rules without knowing their concrete types (Open/Closed Principle).
 */
enum class RuleStage { Expiry, Seasonal, Percentage, Quantity, Membership, Coupon, BankOffer };

/// True for stages applied per cart line (before the guards).
bool isItemStage(RuleStage stage) noexcept;

/// e.g. "Expiry", "BankOffer"
std::string toString(RuleStage stage);

} // namespace rules
