#pragma once

#include <optional>
#include <string>

#include "domain/DateRange.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"
#include "domain/Product.hpp"
#include "rules/ItemFilter.hpp"
#include "rules/RuleStage.hpp"

namespace rules {

/**
 * @brief Abstract base for every pricing offer.
 *
 * OOP ideas: abstraction and polymorphism (pure virtual isApplicable() and
 * calculateDiscount()), and the Open/Closed Principle - the pricing engine
 * talks only to this interface, so a new offer is a new subclass and the
 * engine never changes. Copying is disabled to prevent slicing.
 */
class DiscountRule {
public:
    /// @throws std::invalid_argument if id or name is empty.
    DiscountRule(std::string id, std::string name, int priority, bool stackable);
    virtual ~DiscountRule() = default;

    DiscountRule(const DiscountRule&) = delete;
    DiscountRule& operator=(const DiscountRule&) = delete;
    DiscountRule(DiscountRule&&) = delete;
    DiscountRule& operator=(DiscountRule&&) = delete;

    /// True if this offer can be used for the given order.
    virtual bool isApplicable(const domain::PricingContext& ctx) const = 0;
    /// Rupee amount saved by this offer (zero if not applicable). Never negative.
    virtual domain::Money calculateDiscount(const domain::PricingContext& ctx) const = 0;
    /// Why isApplicable() is false, for the price trace. Empty if applicable.
    virtual std::string rejectionReason(const domain::PricingContext& ctx) const;
    /// Checkout step at which the engine applies this offer.
    virtual RuleStage stage() const = 0;

    // Self-description queries, used by the rule validator. Each rule answers
    // for itself, so the validator never needs to know the concrete types.

    /// Days the offer runs, or nullopt if it is not limited by date.
    virtual std::optional<domain::DateRange> activePeriod() const;
    /// Which products the offer covers, or nullptr for order-wide offers.
    virtual const ItemFilter* scope() const noexcept;
    /// Biggest percentage this offer could ever take off a line of @p product
    /// on its own, or nullopt if it never discounts that product's line.
    virtual std::optional<int> maxPercentOff(const domain::Product& product) const;

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    /// Higher priority rules are evaluated first.
    int priority() const noexcept { return priority_; }
    /// True if this offer may be combined with other offers.
    bool stackable() const noexcept { return stackable_; }

protected:
    /// Shared validation for subclasses. @throws std::invalid_argument unless 1 <= percent <= 100.
    static void requireValidPercent(int percent, const char* what);
    /// ASCII case-insensitive equality, for coupon codes and bank names.
    static bool equalsIgnoreCase(const std::string& a, const std::string& b) noexcept;

private:
    std::string id_;
    std::string name_;
    int priority_;
    bool stackable_;
};

} // namespace rules
