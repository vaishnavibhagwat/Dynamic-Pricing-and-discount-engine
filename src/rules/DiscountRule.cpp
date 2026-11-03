#include "rules/DiscountRule.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace rules {

DiscountRule::DiscountRule(std::string id, std::string name, int priority, bool stackable)
    : id_(std::move(id)), name_(std::move(name)), priority_(priority), stackable_(stackable) {
    if (id_.empty()) throw std::invalid_argument("DiscountRule: id must not be empty");
    if (name_.empty()) throw std::invalid_argument("DiscountRule: name must not be empty");
}

std::string DiscountRule::rejectionReason(const domain::PricingContext& ctx) const {
    return isApplicable(ctx) ? std::string() : std::string("conditions not met");
}

std::optional<domain::DateRange> DiscountRule::activePeriod() const {
    return std::nullopt;
}

const ItemFilter* DiscountRule::scope() const noexcept {
    return nullptr;
}

std::optional<int> DiscountRule::maxPercentOff(const domain::Product&) const {
    return std::nullopt;
}

void DiscountRule::requireValidPercent(int percent, const char* what) {
    if (percent < 1 || percent > 100) {
        throw std::invalid_argument(std::string(what) + ": percent must be 1..100, got " +
                                    std::to_string(percent));
    }
}

bool DiscountRule::equalsIgnoreCase(const std::string& a, const std::string& b) noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) return false;
    }
    return true;
}

} // namespace rules
