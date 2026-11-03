#include "rules/ExpiryMarkdown.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "domain/Perishable.hpp"

namespace rules {

ExpiryMarkdown::ExpiryMarkdown(std::string id, std::string name, int priority, bool stackable,
                               std::vector<Tier> tiers)
    : ItemLevelRule(std::move(id), std::move(name), priority, stackable), tiers_(std::move(tiers)) {
    if (tiers_.empty()) throw std::invalid_argument("ExpiryMarkdown: needs at least one tier");
    std::sort(tiers_.begin(), tiers_.end(),
              [](const Tier& a, const Tier& b) { return a.maxDaysLeft < b.maxDaysLeft; });
    for (std::size_t i = 0; i < tiers_.size(); ++i) {
        if (tiers_[i].maxDaysLeft < 0) {
            throw std::invalid_argument("ExpiryMarkdown: maxDaysLeft must not be negative");
        }
        if (i > 0 && tiers_[i].maxDaysLeft == tiers_[i - 1].maxDaysLeft) {
            throw std::invalid_argument("ExpiryMarkdown: repeated maxDaysLeft " +
                                        std::to_string(tiers_[i].maxDaysLeft));
        }
        requireValidPercent(tiers_[i].percent, "ExpiryMarkdown");
    }
}

std::optional<int> ExpiryMarkdown::percentFor(long daysLeft) const noexcept {
    if (daysLeft < 0) return std::nullopt; // expired: not for sale
    // Tiers are sorted tightest first, so the first match is the closest deadline.
    for (const auto& tier : tiers_) {
        if (daysLeft <= tier.maxDaysLeft) return tier.percent;
    }
    return std::nullopt;
}

std::optional<int> ExpiryMarkdown::percentForItem(const domain::CartItem& item,
                                                  const domain::Date& today) const {
    const auto* perishable = dynamic_cast<const domain::Perishable*>(&item.product());
    if (perishable == nullptr) return std::nullopt;
    return percentFor(today.daysUntil(perishable->expiryDate()));
}

bool ExpiryMarkdown::appliesToItem(const domain::CartItem& item,
                                   const domain::PricingContext& ctx) const {
    return percentForItem(item, ctx.orderDate()).has_value();
}

domain::Money ExpiryMarkdown::discountForItem(const domain::CartItem& item,
                                              const domain::PricingContext& ctx) const {
    const auto percent = percentForItem(item, ctx.orderDate());
    if (!percent) return domain::Money::zero();
    return item.lineTotal().percentOf(*percent);
}

std::optional<int> ExpiryMarkdown::maxPercentOff(const domain::Product& product) const {
    if (dynamic_cast<const domain::Perishable*>(&product) == nullptr) return std::nullopt;
    int best = 0;
    for (const auto& tier : tiers_) best = std::max(best, tier.percent);
    return best;
}

std::string ExpiryMarkdown::noMatchingItemReason() const {
    return "no perishable item within " + std::to_string(tiers_.back().maxDaysLeft) +
           " days of expiry (expired items are never discounted)";
}

} // namespace rules
