#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "domain/PricingContext.hpp"
#include "engine/PriceTrace.hpp"
#include "engine/PricingResult.hpp"

namespace presentation {

/**
 * @brief Prints an itemised GST invoice - with a "Why:" block of trace
 *        decisions under each item and under the order offers - and the
 *        full decision trace as text.
 *
 * OOP ideas: Single Responsibility and layering - it only formats; all
 * numbers come ready-made from engine::PricingResult, so changing the bill
 * layout never touches pricing logic. Writes to any std::ostream, so tests
 * print into a string stream.
 */
class InvoicePrinter {
public:
    /// @throws std::invalid_argument if width < 50.
    explicit InvoicePrinter(std::size_t width = 72);

    void print(std::ostream& os, const domain::PricingContext& ctx,
               const engine::PricingResult& result) const;
    void printTrace(std::ostream& os, const engine::PriceTrace& trace) const;

    /// Number of characters shown on screen (UTF-8 aware: "₹" counts as one).
    static std::size_t displayWidth(const std::string& text) noexcept;

private:
    void row(std::ostream& os, const std::string& label, const std::string& value) const;
    void rule(std::ostream& os, char c) const;
    /// One "Why:" block: the summary of each trace entry for a line or the order.
    void printWhy(std::ostream& os, const std::vector<engine::TraceEntry>& entries) const;

    std::size_t width_;
};

} // namespace presentation
