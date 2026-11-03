#include "engine/PricingResult.hpp"

#include <utility>

namespace engine {

namespace {
template <typename Field>
domain::Money sumLines(const std::vector<PricedLine>& lines, Field field) {
    domain::Money total;
    for (const auto& line : lines) total += field(line);
    return total;
}
} // namespace

PricingResult::PricingResult(std::string resolverName, std::vector<PricedLine> lines,
                             std::vector<AppliedDiscount> cartDiscounts, PriceTrace trace)
    : resolverName_(std::move(resolverName)), lines_(std::move(lines)),
      cartDiscounts_(std::move(cartDiscounts)), trace_(std::move(trace)) {}

domain::Money PricingResult::subtotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.gross; });
}

domain::Money PricingResult::itemDiscountTotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.itemDiscount; });
}

domain::Money PricingResult::cartDiscountTotal() const noexcept {
    domain::Money total;
    for (const auto& d : cartDiscounts_) total += d.amount;
    return total;
}

domain::Money PricingResult::taxableTotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.taxable; });
}

domain::Money PricingResult::gstTotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.gst; });
}

domain::Money PricingResult::grandTotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.total; });
}

domain::Money PricingResult::mrpTotal() const noexcept {
    return sumLines(lines_, [](const PricedLine& l) { return l.mrpTotal(); });
}

} // namespace engine
