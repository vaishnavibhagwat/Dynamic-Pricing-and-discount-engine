#include "engine/PricingEngine.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "engine/DiscountCapPolicy.hpp"
#include "engine/MarginGuard.hpp"
#include "engine/MrpCeilingPolicy.hpp"
#include "rules/ICartLevelRule.hpp"
#include "rules/IItemLevelRule.hpp"

namespace engine {

using domain::Money;
using rules::DiscountRule;

/// Working state for one cart line while the pipeline runs.
struct PricingEngine::LineWork {
    const domain::CartItem* item;
    std::vector<AppliedDiscount> itemDiscounts;
    Money offered;  ///< sum of item offers chosen by the resolver
    Money discount; ///< after guards
};

namespace {

constexpr const char* kGuardStep = "Guard";

std::string stepOf(const DiscountRule& rule) {
    return rules::toString(rule.stage());
}

/// Publishes "rule rejected" for @p target.
void reject(const TracePublisher& trace, const DiscountRule& rule, const std::string& target,
            std::string reason) {
    trace.publish(TraceEntry::rejected(stepOf(rule), rule.id(), rule.name(), target,
                                       std::move(reason)));
}

/// Publishes "rule applied" for @p target.
void accept(const TracePublisher& trace, const DiscountRule& rule, const std::string& target,
            Money amount, std::string reason) {
    trace.publish(TraceEntry::applied(stepOf(rule), rule.id(), rule.name(), target, amount,
                                      std::move(reason)));
}

/// Rules sorted by stage, then higher priority first, then id (deterministic).
std::vector<const DiscountRule*> inStageOrder(std::vector<const DiscountRule*> rules) {
    std::stable_sort(rules.begin(), rules.end(), [](const DiscountRule* a, const DiscountRule* b) {
        return std::make_tuple(static_cast<int>(a->stage()), -a->priority(), a->id()) <
               std::make_tuple(static_cast<int>(b->stage()), -b->priority(), b->id());
    });
    return rules;
}

/// Asks the resolver and checks it kept its side of the contract.
std::vector<Verdict> resolveChecked(const ConflictResolver& resolver,
                                    const std::vector<Candidate>& candidates) {
    auto verdicts = resolver.resolve(candidates);
    if (verdicts.size() != candidates.size()) {
        throw std::logic_error("ConflictResolver " + resolver.name() +
                               " returned the wrong number of verdicts");
    }
    return verdicts;
}

/// Splits @p total over lines in proportion to @p weights, to the paisa.
/// Uses the largest-remainder method so the shares always add up to total.
std::vector<Money> allocate(Money total, const std::vector<Money>& weights) {
    std::vector<Money> shares(weights.size());
    std::int64_t weightSum = 0;
    for (const auto& w : weights) weightSum += w.paise();
    if (weightSum <= 0 || total.isZero()) return shares;

    std::vector<std::int64_t> remainders(weights.size());
    std::int64_t given = 0;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        const std::int64_t scaled = total.paise() * weights[i].paise();
        shares[i] = Money::fromPaise(scaled / weightSum);
        remainders[i] = scaled % weightSum;
        given += shares[i].paise();
    }

    std::vector<std::size_t> order(weights.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) { return remainders[a] > remainders[b]; });
    for (std::size_t k = 0; given < total.paise(); ++k, ++given) {
        shares[order[k % order.size()]] += Money::fromPaise(1);
    }
    return shares;
}

} // namespace

std::vector<std::unique_ptr<PriceGuard>> PricingEngine::defaultGuards() {
    std::vector<std::unique_ptr<PriceGuard>> guards;
    guards.push_back(std::make_unique<DiscountCapPolicy>(60));
    guards.push_back(std::make_unique<MarginGuard>(8));
    guards.push_back(std::make_unique<MrpCeilingPolicy>());
    return guards;
}

PricingEngine::PricingEngine(const ConflictResolver& resolver,
                             std::vector<std::unique_ptr<PriceGuard>> guards)
    : resolver_(resolver), guards_(std::move(guards)) {
    for (const auto& g : guards_) {
        if (!g) throw std::invalid_argument("PricingEngine: null guard");
    }
}

std::vector<std::string> PricingEngine::guardNames() const {
    std::vector<std::string> names;
    for (const auto& g : guards_) names.push_back(g->name());
    return names;
}

PricingResult PricingEngine::price(const domain::PricingContext& ctx,
                                   const rules::Repository<DiscountRule>& rules) const {
    return price(ctx, rules.all());
}

PricingResult PricingEngine::price(const domain::PricingContext& ctx,
                                   const std::vector<const DiscountRule*>& rules) const {
    for (const auto* r : rules) {
        if (r == nullptr) throw std::invalid_argument("PricingEngine::price: null rule");
    }

    std::vector<const DiscountRule*> itemRules;
    std::vector<const DiscountRule*> cartRules;
    for (const auto* r : inStageOrder(rules)) {
        (rules::isItemStage(r->stage()) ? itemRules : cartRules).push_back(r);
    }

    // The result keeps its own copy of the trace; outside observers hear the same decisions.
    PriceTrace collected;
    TracePublisher trace = publisher_;
    trace.subscribe(collected);

    std::vector<LineWork> lines;
    for (const auto& item : ctx.cart().items()) lines.push_back({&item, {}, {}, {}});

    // 1. Item offers, 2. guards.
    priceItemOffers(ctx, itemRules, lines, trace);
    applyGuards(lines, trace);

    // 3. Order offers on what is left after item offers and guards.
    std::vector<Money> nets;
    Money afterItems;
    for (const auto& l : lines) {
        nets.push_back(l.item->lineTotal() - l.discount);
        afterItems += nets.back();
    }
    auto cartDiscounts = priceCartOffers(ctx, cartRules, afterItems, trace);

    // 4. Split order offers over lines, 5. GST per line.
    Money cartTotal;
    for (const auto& d : cartDiscounts) cartTotal += d.amount;
    const std::vector<Money> shares = allocate(cartTotal, nets);

    std::vector<PricedLine> priced;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const auto& work = lines[i];
        const domain::CartItem& item = *work.item;
        const domain::Product& p = item.product();
        PricedLine line;
        line.productId = p.id();
        line.productName = p.name();
        line.category = p.category();
        line.quantity = item.quantity();
        line.unitPrice = p.sellingPrice();
        line.unitMrp = p.mrp();
        line.gross = item.lineTotal();
        line.itemDiscounts = work.itemDiscounts;
        line.guardAdjustment = work.discount - work.offered;
        line.itemDiscount = work.discount;
        line.cartDiscountShare = shares[i];
        line.taxable = nets[i] - shares[i];
        line.gstRate = p.gstRate();
        line.gst = gst_.gstOn(line.taxable, p);
        line.total = line.taxable + line.gst;
        priced.push_back(std::move(line));
    }

    return PricingResult(resolver_.name(), std::move(priced), std::move(cartDiscounts),
                         std::move(collected));
}

void PricingEngine::priceItemOffers(const domain::PricingContext& ctx,
                                    const std::vector<const DiscountRule*>& itemRules,
                                    std::vector<LineWork>& lines,
                                    const TracePublisher& trace) const {
    // Order-wide checks first, so a rule that cannot apply is reported once, not per line.
    std::vector<std::pair<const DiscountRule*, const rules::IItemLevelRule*>> live;
    for (const auto* rule : itemRules) {
        const auto* perItem = dynamic_cast<const rules::IItemLevelRule*>(rule);
        if (perItem == nullptr) {
            reject(trace, *rule, PriceTrace::kOrder,
                   "rule has an item stage but no per-item pricing");
        } else if (!rule->isApplicable(ctx)) {
            reject(trace, *rule, PriceTrace::kOrder, rule->rejectionReason(ctx));
        } else {
            live.emplace_back(rule, perItem);
        }
    }

    for (auto& line : lines) {
        const domain::CartItem& item = *line.item;
        const std::string& target = item.product().id();

        std::vector<Candidate> candidates;
        for (const auto& [rule, perItem] : live) {
            if (!perItem->appliesToItem(item, ctx)) {
                reject(trace, *rule, target, "does not cover " + item.product().name());
                continue;
            }
            const Money d = perItem->discountForItem(item, ctx);
            if (d <= Money::zero()) {
                reject(trace, *rule, target, "gives no discount on this line");
                continue;
            }
            candidates.push_back({rule, d});
        }

        const auto verdicts = resolveChecked(resolver_, candidates);
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            const DiscountRule& rule = *candidates[i].rule;
            if (verdicts[i].selected) {
                line.offered += candidates[i].discount;
                line.itemDiscounts.push_back(
                    {rule.id(), rule.name(), rule.stage(), candidates[i].discount});
                accept(trace, rule, target, candidates[i].discount, verdicts[i].reason);
            } else {
                reject(trace, rule, target, resolver_.name() + ": " + verdicts[i].reason);
            }
        }
        line.discount = line.offered;
    }
}

void PricingEngine::applyGuards(std::vector<LineWork>& lines,
                                    const TracePublisher& trace) const {
    for (auto& line : lines) {
        const domain::CartItem& item = *line.item;
        const std::string& target = item.product().id();
        for (const auto& guard : guards_) {
            GuardOutcome out = guard->limit(item, line.discount);
            if (out.reason.empty()) out.reason = "check passed";
            if (out.discount == line.discount) {
                trace.publish(TraceEntry::passed(kGuardStep, guard->name(), guard->name(), target,
                                                 std::move(out.reason)));
                continue;
            }
            trace.publish(TraceEntry::adjusted(kGuardStep, guard->name(), guard->name(), target,
                                               out.discount - line.discount,
                                               std::move(out.reason)));
            line.discount = out.discount;
        }
        // Engine invariant, whatever guards are installed: 0 <= discount <= line total.
        const Money clamped = std::clamp(line.discount, Money::zero(), item.lineTotal());
        if (clamped != line.discount) {
            trace.publish(TraceEntry::adjusted(kGuardStep, "PricingEngine", "PricingEngine", target,
                                               clamped - line.discount,
                                               "discount must stay within the line total"));
            line.discount = clamped;
        }
    }
}

std::vector<AppliedDiscount> PricingEngine::priceCartOffers(
    const domain::PricingContext& ctx, const std::vector<const DiscountRule*>& cartRules,
    Money amount, const TracePublisher& trace) const {
    std::vector<Candidate> candidates;
    for (const auto* rule : cartRules) {
        const auto* onOrder = dynamic_cast<const rules::ICartLevelRule*>(rule);
        if (onOrder == nullptr) {
            reject(trace, *rule, PriceTrace::kOrder,
                   "rule has an order stage but no order-level pricing");
            continue;
        }
        if (!rule->isApplicable(ctx)) {
            reject(trace, *rule, PriceTrace::kOrder, rule->rejectionReason(ctx));
            continue;
        }
        const Money d = onOrder->discountOnAmount(amount, ctx);
        if (d <= Money::zero()) {
            reject(trace, *rule, PriceTrace::kOrder, "gives no discount on " + amount.toString());
            continue;
        }
        candidates.push_back({rule, d});
    }

    const auto verdicts = resolveChecked(resolver_, candidates);

    // Chosen offers apply in stage order, each on what is still left to pay.
    std::vector<AppliedDiscount> applied;
    Money remaining = amount;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const DiscountRule& rule = *candidates[i].rule;
        if (!verdicts[i].selected) {
            reject(trace, rule, PriceTrace::kOrder, resolver_.name() + ": " + verdicts[i].reason);
            continue;
        }
        const auto& onOrder = dynamic_cast<const rules::ICartLevelRule&>(rule);
        const Money d = std::min(onOrder.discountOnAmount(remaining, ctx), remaining);
        if (d <= Money::zero()) {
            reject(trace, rule, PriceTrace::kOrder, "nothing left to discount");
            continue;
        }
        std::string reason = verdicts[i].reason;
        if (remaining != amount) reason += "; taken on remaining " + remaining.toString();
        accept(trace, rule, PriceTrace::kOrder, d, std::move(reason));
        applied.push_back({rule.id(), rule.name(), rule.stage(), d});
        remaining -= d;
    }
    return applied;
}

} // namespace engine
