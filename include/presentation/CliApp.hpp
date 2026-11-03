#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "application/CheckoutService.hpp"
#include "application/ConflictPolicies.hpp"
#include "application/StoreLoader.hpp"
#include "domain/Cart.hpp"
#include "domain/Customer.hpp"
#include "domain/Date.hpp"
#include "domain/TimeOfDay.hpp"
#include "domain/PricingContext.hpp"
#include "domain/Product.hpp"
#include "engine/PricingResult.hpp"
#include "infrastructure/Logger.hpp"
#include "engine/ConflictResolver.hpp"
#include "rules/DiscountRule.hpp"
#include "rules/Repository.hpp"

namespace presentation {

/// Where the CLI reads its data and which day it prices for.
struct AppConfig {
    /// Folder holding catalog.json, customers.json, rules.json and rules.txt.
    std::string dataDir;
    domain::Date orderDate;
    /// Clock time used for happy-hour offers; nullopt means unknown.
    std::optional<domain::TimeOfDay> orderTime = std::nullopt;
};

/**
 * @brief The menu-driven console front end: browse the catalog, build a
 *        cart, choose a customer, coupon and bank, check out, and explore
 *        the trace, what-if table, conflict report, best offer set and
 *        smart upsell suggestions.
 *
 * Reads commands line by line from any std::istream and writes to any
 * std::ostream, so tests drive it with scripted input. End of input exits
 * cleanly; an error in one action is reported and the menu carries on.
 *
 * OOP ideas: Facade and layering - this is the only class that knows about
 * menus; each action hands off to an application service (StoreLoader,
 * CheckoutService, WhatIfSimulator, DiscountOptimizer) or a printer, and
 * never loads files or prices anything itself (Single Responsibility).
 * Composition - it owns the catalog, rules, cart and conflict policies for
 * the session (RAII, no raw new/delete). Session events (data loaded,
 * orders placed, reloads, errors) go to an injected infrastructure::Logger,
 * which is silent by default. Dependency Injection and
 * Open/Closed - the conflict policies are passed in as ConflictResolver
 * objects and the policy menu is built from their names, so a new policy
 * needs no change here.
 */
class CliApp {
public:
    /// Loads catalog, customers and rules from config.dataDir. @p policies are
    /// offered at checkout in list order; the first is the starting policy.
    /// @throws infrastructure::ConfigError if a data file is missing or bad,
    ///         or there are no customers.
    /// @throws std::invalid_argument if @p policies is empty or holds a null.
    CliApp(std::istream& in, std::ostream& out, AppConfig config,
           application::ConflictPolicies policies = application::standardConflictPolicies(),
           infrastructure::Logger logger = infrastructure::Logger());

    CliApp(const CliApp&) = delete;
    CliApp& operator=(const CliApp&) = delete;

    /// Runs the menu until "0" or end of input. @return process exit code (0).
    int run();

    // Read-only views of the session, for tests.
    std::size_t ruleCount() const noexcept { return rules_.size(); }
    const domain::Cart& cart() const noexcept { return cart_; }
    const domain::Customer& customer() const noexcept { return customers_[customerIndex_]; }
    const engine::ConflictResolver& policy() const noexcept { return *policies_[policyIndex_]; }

private:
    // Menu actions.
    void viewCatalog();
    void buildCart();
    void pickCustomer();
    void applyCouponAndBank();
    void checkout();
    void viewTrace();
    void runWhatIf();
    void viewConflictReport();
    void reloadRules();
    void findBestCombination();
    void suggestUpsells();

    // Helpers.
    void printMenu();
    void printCart();
    /// Next input line, trimmed; nullopt (and the session ends) at end of input.
    std::optional<std::string> ask(const std::string& prompt);
    bool requireCart();
    domain::PricingContext context() const;
    std::vector<const domain::Product*> productList() const;
    /// One log line per placed order: who, policy, total and offers used.
    void logOrder(const domain::Customer& buyer, const engine::PricingResult& result);

    std::istream& in_;
    std::ostream& out_;
    AppConfig config_;
    application::StoreLoader store_;
    infrastructure::Logger log_;
    bool finished_ = false;

    rules::Repository<domain::Product> catalog_;
    std::vector<domain::Customer> customers_;
    std::size_t customerIndex_ = 0;
    rules::Repository<rules::DiscountRule> rules_;

    domain::Cart cart_;
    std::optional<std::string> coupon_;
    std::optional<std::string> bank_;

    application::ConflictPolicies policies_;
    std::size_t policyIndex_ = 0;

    application::CheckoutService checkout_; // refers to rules_, so declared after it
    std::optional<engine::PricingResult> lastResult_;
};

} // namespace presentation
