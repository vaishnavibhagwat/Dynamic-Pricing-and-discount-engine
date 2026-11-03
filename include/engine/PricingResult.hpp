#pragma once

#include <string>
#include <vector>

#include "domain/Money.hpp"
#include "engine/PriceTrace.hpp"
#include "rules/RuleStage.hpp"

namespace engine {

/// One offer as it was actually given.
struct AppliedDiscount {
    std::string ruleId;
    std::string ruleName;
    rules::RuleStage stage;
    domain::Money amount;
};

/// The priced version of one cart line. Holds copies, not references, so it
/// stays valid after the cart is gone.
struct PricedLine {
    std::string productId;
    std::string productName;
    std::string category;
    int quantity = 0;
    domain::Money unitPrice;              ///< selling price, before GST
    domain::Money unitMrp;                ///< MRP, includes GST
    domain::Money gross;                  ///< unitPrice x quantity
    std::vector<AppliedDiscount> itemDiscounts; ///< offers chosen by the resolver
    domain::Money guardAdjustment;        ///< change made by guards (+ = more discount)
    domain::Money itemDiscount;           ///< item offers after guards
    domain::Money cartDiscountShare;      ///< this line's share of coupon / bank offers
    domain::Money taxable;                ///< gross - itemDiscount - cartDiscountShare
    int gstRate = 0;
    domain::Money gst;
    domain::Money total;                  ///< taxable + gst

    domain::Money net() const noexcept { return gross - itemDiscount; }
    domain::Money mrpTotal() const noexcept { return unitMrp * quantity; }
};

/**
 * @brief The full outcome of pricing one order: priced lines, order-level
 *        offers, totals and the decision trace.
 *
 * OOP ideas: encapsulation (read-only after construction; totals are
 * derived, never stored twice) and separation of concerns - the engine
 * computes it, InvoicePrinter only displays it.
 */
class PricingResult {
public:
    PricingResult(std::string resolverName, std::vector<PricedLine> lines,
                  std::vector<AppliedDiscount> cartDiscounts, PriceTrace trace);

    const std::string& resolverName() const noexcept { return resolverName_; }
    const std::vector<PricedLine>& lines() const noexcept { return lines_; }
    const std::vector<AppliedDiscount>& cartDiscounts() const noexcept { return cartDiscounts_; }
    const PriceTrace& trace() const noexcept { return trace_; }

    domain::Money subtotal() const noexcept;          ///< sum of gross
    domain::Money itemDiscountTotal() const noexcept; ///< sum of item discounts after guards
    domain::Money cartDiscountTotal() const noexcept; ///< sum of coupon / bank offers
    domain::Money taxableTotal() const noexcept;
    domain::Money gstTotal() const noexcept;
    domain::Money grandTotal() const noexcept;        ///< amount the customer pays
    domain::Money mrpTotal() const noexcept;
    domain::Money savingsVsMrp() const noexcept { return mrpTotal() - grandTotal(); }

private:
    std::string resolverName_;
    std::vector<PricedLine> lines_;
    std::vector<AppliedDiscount> cartDiscounts_;
    PriceTrace trace_;
};

} // namespace engine
