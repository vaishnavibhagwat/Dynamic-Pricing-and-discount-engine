#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "engine/TraceEntry.hpp"
#include "engine/TraceObserver.hpp"

namespace engine {

/**
 * @brief Audit log of every rule and guard decision for one order.
 *
 * OOP ideas: a concrete Observer - the engine publishes decisions and this
 * class simply collects them (it is the copy kept in PricingResult). Single
 * Responsibility - it only stores and filters; wording lives in
 * TraceEntry::summary() and layout in InvoicePrinter. Encapsulation: entries
 * are only added through record()/onDecision() and read as const.
 */
class PriceTrace : public TraceObserver {
public:
    /// Target used for decisions about the whole order rather than one line.
    static constexpr const char* kOrder = "order";

    void record(TraceEntry entry);
    void onDecision(const TraceEntry& entry) override { record(entry); }

    const std::vector<TraceEntry>& entries() const noexcept { return entries_; }
    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }

    /// Entries about one product id (or kOrder), in the order they were recorded.
    std::vector<TraceEntry> forTarget(const std::string& target) const;
    /// Entries about one rule id (or guard name).
    std::vector<TraceEntry> forRule(const std::string& ruleId) const;
    std::size_t count(TraceOutcome outcome) const noexcept;

private:
    std::vector<TraceEntry> entries_;
};

} // namespace engine
