#include "presentation/InvoicePrinter.hpp"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace presentation {

using domain::Money;

namespace {

/// "-₹400.00" for a discount of ₹400.
std::string minus(Money amount) {
    return (-amount).toString();
}

/// "+₹10.00" / "-₹10.00" / "₹0.00"
std::string signedMoney(Money amount) {
    if (amount > Money::zero()) return "+" + amount.toString();
    return amount.toString();
}

std::string padRight(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : text + std::string(width - shown, ' ');
}

} // namespace

InvoicePrinter::InvoicePrinter(std::size_t width) : width_(width) {
    if (width_ < 50) throw std::invalid_argument("InvoicePrinter: width must be at least 50");
}

std::size_t InvoicePrinter::displayWidth(const std::string& text) noexcept {
    std::size_t count = 0;
    for (const char ch : text) {
        // Count every byte except UTF-8 continuation bytes (10xxxxxx).
        if ((static_cast<unsigned char>(ch) & 0xC0) != 0x80) ++count;
    }
    return count;
}

void InvoicePrinter::rule(std::ostream& os, char c) const {
    os << std::string(width_, c) << '\n';
}

void InvoicePrinter::row(std::ostream& os, const std::string& label,
                         const std::string& value) const {
    const std::size_t valueWidth = displayWidth(value);
    const std::size_t room = width_ - std::min(width_ - 1, valueWidth + 1); // label space on a row
    // Label left, value right-aligned; no trailing spaces when there is no value.
    const auto emit = [&](const std::string& text) {
        os << (value.empty() ? text : padRight(text, width_ - valueWidth) + value) << '\n';
    };
    if (displayWidth(label) <= room) {
        emit(label);
        return;
    }

    // Too long: word-wrap the label, continuation lines indented, value on the last line.
    const std::size_t lead = label.find_first_not_of(' ');
    const std::string indent(lead == std::string::npos ? 0 : lead + 2, ' ');
    std::vector<std::string> lines;
    std::string current = label.substr(0, lead == std::string::npos ? 0 : lead);
    bool firstWord = true;
    std::istringstream words(label);
    std::string word;
    while (words >> word) {
        const std::string candidate = firstWord ? current + word : current + " " + word;
        if (!firstWord && displayWidth(candidate) > width_) {
            lines.push_back(current);
            current = indent + word;
        } else {
            current = candidate;
        }
        firstWord = false;
    }
    for (const auto& l : lines) os << l << '\n';
    if (displayWidth(current) <= room) {
        emit(current);
    } else {
        os << current << '\n' << std::string(width_ - std::min(width_, valueWidth), ' ') << value
           << '\n';
    }
}

void InvoicePrinter::print(std::ostream& os, const domain::PricingContext& ctx,
                           const engine::PricingResult& result) const {
    const auto& customer = ctx.customer();
    const std::string title = "TAX INVOICE";

    rule(os, '=');
    os << std::string((width_ - title.size()) / 2, ' ') << title << '\n';
    rule(os, '-');
    row(os, "Customer : " + customer.name() + " (" + customer.id() + ")",
        "Tier : " + domain::toString(customer.tier()));
    row(os, "Date     : " + ctx.orderDate().toString(),
        "Offers resolved by : " + result.resolverName());
    if (ctx.couponCode()) row(os, "Coupon   : " + *ctx.couponCode(), "");
    if (ctx.paymentBank()) row(os, "Paid with: " + *ctx.paymentBank(), "");
    rule(os, '-');

    int n = 0;
    for (const auto& line : result.lines()) {
        row(os, std::to_string(++n) + ". " + line.productName + " [" + line.productId + "]",
            line.category + ", GST " + std::to_string(line.gstRate) + "%");
        row(os, "   " + std::to_string(line.quantity) + " x " + line.unitPrice.toString() +
                    "  (MRP " + line.unitMrp.toString() + ")",
            line.gross.toString());
        for (const auto& d : line.itemDiscounts) {
            row(os, "   - " + d.ruleName, minus(d.amount));
        }
        if (!line.guardAdjustment.isZero()) {
            // A guard that cut the discount raises the price, and vice versa.
            row(os, "   * Guard adjustment", signedMoney(-line.guardAdjustment));
        }
        if (!line.cartDiscountShare.isZero()) {
            row(os, "   - Share of order offers", minus(line.cartDiscountShare));
        }
        row(os, "   Taxable value", line.taxable.toString());
        row(os, "   GST @ " + std::to_string(line.gstRate) + "%", line.gst.toString());
        row(os, "   Line total", line.total.toString());
        printWhy(os, result.trace().forTarget(line.productId));
        os << '\n';
    }

    const auto orderDecisions = result.trace().forTarget(engine::PriceTrace::kOrder);
    if (!result.cartDiscounts().empty() || !orderDecisions.empty()) {
        rule(os, '-');
        os << "Order offers\n";
        for (const auto& d : result.cartDiscounts()) row(os, "   - " + d.ruleName, minus(d.amount));
        printWhy(os, orderDecisions);
    }

    rule(os, '-');
    row(os, "Subtotal", result.subtotal().toString());
    row(os, "Item discounts", minus(result.itemDiscountTotal()));
    row(os, "Order offers", minus(result.cartDiscountTotal()));
    row(os, "Taxable value", result.taxableTotal().toString());
    row(os, "GST", result.gstTotal().toString());
    rule(os, '-');
    row(os, "GRAND TOTAL", result.grandTotal().toString());
    rule(os, '-');
    row(os, "MRP total " + result.mrpTotal().toString(),
        "You saved " + result.savingsVsMrp().toString());
    rule(os, '=');
}

void InvoicePrinter::printWhy(std::ostream& os,
                              const std::vector<engine::TraceEntry>& entries) const {
    if (entries.empty()) return;
    os << "   Why:\n";
    for (const auto& e : entries) row(os, "     " + e.summary(), "");
}

void InvoicePrinter::printTrace(std::ostream& os, const engine::PriceTrace& trace) const {
    rule(os, '=');
    os << "PRICE TRACE (" << trace.size() << " decisions)\n";
    rule(os, '-');
    for (const auto& e : trace.entries()) {
        std::string head = padRight(engine::toString(e.outcome), 9) + padRight(e.step, 11) +
                           padRight(e.ruleId, 20) + padRight(e.target, 7);
        if (e.outcome == engine::TraceOutcome::Applied) head += minus(e.amount);
        // Shown as the effect on price: less discount = price goes up.
        if (e.outcome == engine::TraceOutcome::Adjusted) head += signedMoney(-e.amount);
        head.erase(head.find_last_not_of(' ') + 1);
        os << head << '\n';
        if (!e.reason.empty()) row(os, "         " + e.reason, "");
    }
    rule(os, '=');
}

} // namespace presentation
