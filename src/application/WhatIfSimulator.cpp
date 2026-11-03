#include "application/WhatIfSimulator.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "engine/PricingEngine.hpp"

namespace application {

// ------------------------------------------------------------- WhatIfReport

WhatIfReport::WhatIfReport(std::vector<std::string> resolverNames,
                           std::vector<domain::MembershipTier> tiers,
                           std::vector<std::vector<WhatIfCell>> cells,
                           domain::MembershipTier currentTier)
    : resolverNames_(std::move(resolverNames)), tiers_(std::move(tiers)),
      cells_(std::move(cells)), currentTier_(currentTier) {
    if (cells_.size() != tiers_.size()) {
        throw std::invalid_argument("WhatIfReport: one row of cells per tier expected");
    }
    for (const auto& row : cells_) {
        if (row.size() != resolverNames_.size()) {
            throw std::invalid_argument("WhatIfReport: one cell per resolver expected");
        }
    }
}

const WhatIfCell& WhatIfReport::cell(std::size_t tierIndex, std::size_t resolverIndex) const {
    return cells_.at(tierIndex).at(resolverIndex);
}

std::size_t WhatIfReport::cheapestResolver(std::size_t tierIndex) const {
    const auto& row = cells_.at(tierIndex);
    std::size_t best = 0;
    for (std::size_t i = 1; i < row.size(); ++i) {
        if (row[i].grandTotal < row[best].grandTotal) best = i;
    }
    return best;
}

// ----------------------------------------------------------- WhatIfSimulator

WhatIfSimulator::WhatIfSimulator(std::vector<const engine::ConflictResolver*> resolvers,
                                 GuardFactory guards)
    : resolvers_(std::move(resolvers)), guards_(std::move(guards)) {
    if (resolvers_.empty()) throw std::invalid_argument("WhatIfSimulator: no resolvers");
    for (const auto* r : resolvers_) {
        if (r == nullptr) throw std::invalid_argument("WhatIfSimulator: null resolver");
    }
    if (!guards_) throw std::invalid_argument("WhatIfSimulator: guard factory must not be empty");
}

WhatIfReport WhatIfSimulator::run(const domain::PricingContext& ctx,
                                  const rules::Repository<rules::DiscountRule>& rules) const {
    return run(ctx, rules.all());
}

WhatIfReport WhatIfSimulator::run(const domain::PricingContext& ctx,
                                  const std::vector<const rules::DiscountRule*>& rules) const {
    std::vector<std::string> names;
    std::vector<std::unique_ptr<engine::PricingEngine>> engines;
    for (const auto* r : resolvers_) {
        names.push_back(r->name());
        engines.push_back(std::make_unique<engine::PricingEngine>(*r, guards_()));
    }

    const domain::Customer& real = ctx.customer();
    std::vector<domain::MembershipTier> tiers;
    std::vector<std::vector<WhatIfCell>> cells;
    for (const auto tier : domain::allMembershipTiers()) {
        // Same shopper, same order - only the tier differs.
        const domain::Customer asTier(real.id(), real.name(), tier);
        const domain::PricingContext variant(ctx.cart(), asTier, ctx.orderDate(), ctx.couponCode(),
                                             ctx.paymentBank());
        std::vector<WhatIfCell> row;
        for (const auto& eng : engines) {
            const auto result = eng->price(variant, rules);
            row.push_back({result.grandTotal(),
                           result.itemDiscountTotal() + result.cartDiscountTotal()});
        }
        tiers.push_back(tier);
        cells.push_back(std::move(row));
    }
    return WhatIfReport(std::move(names), std::move(tiers), std::move(cells), real.tier());
}

} // namespace application
