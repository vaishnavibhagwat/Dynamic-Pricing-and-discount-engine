#include "presentation/CliApp.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <set>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "application/DiscountOptimizer.hpp"
#include "application/UpsellAdvisor.hpp"
#include "application/WhatIfSimulator.hpp"
#include "domain/Perishable.hpp"
#include "infrastructure/Exceptions.hpp"
#include "presentation/ConflictReportPrinter.hpp"
#include "presentation/InvoicePrinter.hpp"
#include "presentation/OptimizationPrinter.hpp"
#include "presentation/UpsellPrinter.hpp"
#include "presentation/WhatIfPrinter.hpp"
#include "validation/RuleValidator.hpp"

namespace presentation {

using domain::Money;

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string upper(std::string text) {
    for (auto& ch : text) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return text;
}

/// Whole number, or nullopt if @p text is not one.
std::optional<int> parseInt(const std::string& text) {
    std::istringstream in(text);
    int value = 0;
    if (!(in >> value)) return std::nullopt;
    char extra;
    if (in >> extra) return std::nullopt;
    return value;
}

std::string padRight(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : text + std::string(width - shown, ' ');
}

std::string padLeft(const std::string& text, std::size_t width) {
    const std::size_t shown = InvoicePrinter::displayWidth(text);
    return shown >= width ? text : std::string(width - shown, ' ') + text;
}

std::string orNone(const std::optional<std::string>& value) {
    return value ? *value : "none";
}

} // namespace

CliApp::CliApp(std::istream& in, std::ostream& out, AppConfig config,
               application::ConflictPolicies policies, infrastructure::Logger logger)
    : in_(in), out_(out), config_(std::move(config)), store_(config_.dataDir),
      log_(std::move(logger)), policies_(std::move(policies)), checkout_(rules_) {
    if (policies_.empty()) throw std::invalid_argument("CliApp: no conflict policies");
    for (const auto& p : policies_) {
        if (!p) throw std::invalid_argument("CliApp: null conflict policy");
    }
    catalog_ = store_.loadCatalog();
    customers_ = store_.loadCustomers();
    rules_ = store_.loadRules();
    log_.info("Loaded " + std::to_string(catalog_.size()) + " products, " +
              std::to_string(customers_.size()) + " customers and " +
              std::to_string(rules_.size()) + " rules from " + config_.dataDir);
}

std::optional<std::string> CliApp::ask(const std::string& prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        finished_ = true;
        out_ << '\n';
        return std::nullopt;
    }
    // Windows PowerShell starts piped input with a UTF-8 byte order mark; drop it.
    const std::string bom = "\xEF\xBB\xBF";
    if (line.compare(0, bom.size(), bom) == 0) line.erase(0, bom.size());
    return trim(line);
}

domain::PricingContext CliApp::context() const {
    return domain::PricingContext(cart_, customers_[customerIndex_], config_.orderDate, coupon_,
                                  bank_, config_.orderTime);
}

std::vector<const domain::Product*> CliApp::productList() const {
    return catalog_.all();
}

bool CliApp::requireCart() {
    if (!cart_.empty()) return true;
    out_ << "Your cart is empty - add products with 2 (Build cart) first.\n";
    return false;
}

// ------------------------------------------------------------------- menu

int CliApp::run() {
    out_ << "Dynamic Pricing & Discount Rule Engine\n"
         << "Loaded " << catalog_.size() << " products, " << customers_.size() << " customers and "
         << rules_.size() << " rules from " << config_.dataDir << "\n";

    while (!finished_) {
        printMenu();
        const auto choice = ask("Choose an option: ");
        if (!choice) break;
        try {
            if (*choice == "1") viewCatalog();
            else if (*choice == "2") buildCart();
            else if (*choice == "3") pickCustomer();
            else if (*choice == "4") applyCouponAndBank();
            else if (*choice == "5") checkout();
            else if (*choice == "6") viewTrace();
            else if (*choice == "7") runWhatIf();
            else if (*choice == "8") viewConflictReport();
            else if (*choice == "9") reloadRules();
            else if (*choice == "10") findBestCombination();
            else if (*choice == "11") suggestUpsells();
            else if (*choice == "0") finished_ = true;
            else if (!choice->empty()) out_ << "Unknown option \"" << *choice << "\" - enter 0 to 11.\n";
        } catch (const std::exception& e) {
            out_ << "error: " << e.what() << '\n';
            log_.error("Menu option " + *choice + " failed: " + e.what());
        }
    }
    log_.info("Session ended");
    out_ << "Goodbye.\n";
    return 0;
}

void CliApp::printMenu() {
    const auto& c = customers_[customerIndex_];
    out_ << "\n------------------------------------------------------------------------\n"
         << "Date " << config_.orderDate
         << (config_.orderTime ? " " + config_.orderTime->toString() : std::string())
         << " | Customer " << c.name() << " ("
         << domain::toString(c.tier()) << ") | Policy " << policy().name() << '\n'
         << "Cart " << cart_.items().size() << (cart_.items().size() == 1 ? " line, " : " lines, ")
         << cart_.subtotal() << " | Coupon " << orNone(coupon_) << " | Bank " << orNone(bank_)
         << '\n'
         << "------------------------------------------------------------------------\n"
         << "  1  View catalog              6  View price trace\n"
         << "  2  Build cart                7  Run what-if\n"
         << "  3  Pick customer             8  View conflict report\n"
         << "  4  Apply coupon and bank     9  Reload rules\n"
         << "  5  Checkout                 10  Find best offer combination\n"
         << "  0  Exit                     11  Smart upsell suggestions\n";
}

// --------------------------------------------------------------- actions

void CliApp::viewCatalog() {
    out_ << '\n'
         << padRight("ID", 5) << padRight("Category", 13) << padRight("Name", 20)
         << padLeft("Price", 12) << padLeft("MRP", 12) << padLeft("GST", 6) << '\n';
    for (const auto* p : productList()) {
        out_ << padRight(p->id(), 5) << padRight(p->category(), 13) << padRight(p->name(), 20)
             << padLeft(p->sellingPrice().toString(), 12) << padLeft(p->mrp().toString(), 12)
             << padLeft(std::to_string(p->gstRate()) + "%", 6)
             << (domain::isExpiredOn(*p, config_.orderDate) ? "  EXPIRED" : "") << '\n';
    }
    out_ << "Prices are before GST; MRP includes GST. EXPIRED items cannot be sold.\n";
}

void CliApp::printCart() {
    if (cart_.empty()) {
        out_ << "Cart is empty.\n";
        return;
    }
    for (const auto& line : cart_.items()) {
        const auto& p = line.product();
        out_ << "  " << padRight(p.id(), 5) << padRight(p.name(), 20) << " x"
             << padRight(std::to_string(line.quantity()), 4) << padLeft(line.lineTotal().toString(), 14)
             << '\n';
    }
    out_ << "  " << padRight("Subtotal (before GST)", 30) << padLeft(cart_.subtotal().toString(), 14)
         << '\n';
}

void CliApp::buildCart() {
    while (!finished_) {
        out_ << "\nCart:\n";
        printCart();
        const auto choice = ask("[a]dd  [r]emove  [c]lear  [b]ack: ");
        if (!choice) return;
        const std::string c = upper(*choice);
        if (c == "B" || c.empty()) return;

        if (c == "A") {
            const auto id = ask("Product id: ");
            if (!id) return;
            const domain::Product* product = catalog_.findById(upper(*id));
            if (product == nullptr) {
                out_ << "No product with id \"" << *id << "\" - see 1 (View catalog).\n";
                continue;
            }
            if (const domain::Perishable* p = domain::asPerishable(*product);
                p != nullptr && p->isExpiredOn(config_.orderDate)) {
                out_ << product->name() << " expired on " << p->expiryDate()
                     << " and cannot be sold.\n";
                continue;
            }
            const auto qtyText = ask("Quantity [1]: ");
            if (!qtyText) return;
            const auto qty = qtyText->empty() ? std::optional<int>(1) : parseInt(*qtyText);
            if (!qty || *qty < 1 || *qty > domain::CartItem::kMaxQuantity) {
                out_ << "Quantity must be a whole number from 1 to "
                     << domain::CartItem::kMaxQuantity << ".\n";
                continue;
            }
            try {
                cart_ += domain::CartItem(*product, *qty);
            } catch (const std::invalid_argument& e) {
                out_ << "Not added: " << e.what() << ".\n";
                continue;
            }
            out_ << "Added " << *qty << " x " << product->name() << ".\n";
        } else if (c == "R") {
            const auto id = ask("Product id to remove: ");
            if (!id) return;
            out_ << (cart_.remove(upper(*id)) ? "Removed.\n" : "That product is not in the cart.\n");
        } else if (c == "C") {
            cart_ = domain::Cart{};
            out_ << "Cart cleared.\n";
        } else {
            out_ << "Enter a, r, c or b.\n";
        }
    }
}

void CliApp::pickCustomer() {
    out_ << '\n';
    for (std::size_t i = 0; i < customers_.size(); ++i) {
        const auto& c = customers_[i];
        out_ << (i == customerIndex_ ? "* " : "  ") << padLeft(std::to_string(i + 1), 2) << ". "
             << padRight(c.id(), 6) << padRight(c.name(), 18) << domain::toString(c.tier())
             << '\n';
    }
    const auto choice = ask("Customer number or id (Enter = keep): ");
    if (!choice || choice->empty()) return;

    std::optional<std::size_t> picked;
    if (const auto n = parseInt(*choice); n && *n >= 1 &&
                                          static_cast<std::size_t>(*n) <= customers_.size()) {
        picked = static_cast<std::size_t>(*n - 1);
    } else {
        for (std::size_t i = 0; i < customers_.size(); ++i) {
            if (upper(customers_[i].id()) == upper(*choice)) picked = i;
        }
    }
    if (!picked) {
        out_ << "No such customer.\n";
        return;
    }
    customerIndex_ = *picked;
    out_ << "Customer is now " << customers_[customerIndex_].name() << " ("
         << domain::toString(customers_[customerIndex_].tier()) << ").\n";
}

void CliApp::applyCouponAndBank() {
    out_ << "\nEnter keeps the current value; '-' removes it.\n";
    const auto code = ask("Coupon code [" + orNone(coupon_) + "]: ");
    if (!code) return;
    if (*code == "-") coupon_.reset();
    else if (!code->empty()) coupon_ = upper(*code);

    const auto bank = ask("Paying with bank [" + orNone(bank_) + "]: ");
    if (!bank) return;
    if (*bank == "-") bank_.reset();
    else if (!bank->empty()) bank_ = upper(*bank);

    out_ << "Coupon: " << orNone(coupon_) << ", bank: " << orNone(bank_)
         << ". Whether they apply is decided at checkout (see the trace).\n";
}

void CliApp::checkout() {
    if (!requireCart()) return;
    std::string prompt = "Conflict policy:";
    for (std::size_t i = 0; i < policies_.size(); ++i) {
        prompt += (i == 0 ? " " : ", ") + std::to_string(i + 1) + " " + policies_[i]->name();
    }
    const auto choice = ask(prompt + " [" + policy().name() + "]: ");
    if (!choice) return;
    if (const auto n = parseInt(*choice);
        n && *n >= 1 && static_cast<std::size_t>(*n) <= policies_.size()) {
        policyIndex_ = static_cast<std::size_t>(*n - 1);
    } else if (!choice->empty()) {
        out_ << "Unknown policy; keeping " << policy().name() << ".\n";
    }

    const auto ctx = context();
    const domain::Customer& buyer = customers_[customerIndex_];
    lastResult_ = checkout_.checkout(ctx, policy(), customers_[customerIndex_]);
    logOrder(buyer, *lastResult_);
    out_ << '\n';
    InvoicePrinter().print(out_, ctx, *lastResult_);
    out_ << "\nOrder placed. The cart is kept so you can compare (7, 10); clear it in 2 to start "
            "a new order.\n";
}

void CliApp::viewTrace() {
    if (!lastResult_) {
        out_ << "No checkout yet - choose 5 (Checkout) first.\n";
        return;
    }
    out_ << '\n';
    InvoicePrinter().printTrace(out_, lastResult_->trace());
}

void CliApp::runWhatIf() {
    if (!requireCart()) return;
    std::vector<const engine::ConflictResolver*> columns;
    for (const auto& p : policies_) columns.push_back(p.get());
    const application::WhatIfSimulator simulator(std::move(columns));
    out_ << '\n';
    WhatIfPrinter().print(out_, simulator.run(context(), rules_));
}

void CliApp::viewConflictReport() {
    out_ << '\n';
    const validation::RuleValidator validator;
    ConflictReportPrinter().print(out_,
                                  validator.validate(rules_, productList(), config_.orderDate));
}

void CliApp::reloadRules() {
    try {
        auto fresh = store_.loadRules();
        rules_ = std::move(fresh);
        out_ << "Reloaded " << rules_.size() << " rules. Coupon usage counts start again from 0.\n";
        log_.info("Reloaded " + std::to_string(rules_.size()) + " rules");
    } catch (const infrastructure::ConfigError& e) {
        log_.warning("Rule reload failed, kept " + std::to_string(rules_.size()) +
                     " rules: " + e.what());
        out_ << "Could not reload rules, keeping the " << rules_.size()
             << " rules already loaded.\nerror: " << e.what() << '\n';
    }
}

void CliApp::findBestCombination() {
    if (!requireCart()) return;
    const auto ctx = context();
    const auto result = application::DiscountOptimizer().optimize(ctx, rules_);
    out_ << '\n';
    OptimizationPrinter().print(out_, result);

    const Money current = checkout_.quote(ctx, policy()).grandTotal();
    const Money best = result.bestCombination().grandTotal;
    out_ << "\n  Your current policy (" << policy().name() << ") gives " << current << ".\n  ";
    if (best < current) {
        out_ << "The best combination saves " << (current - best) << " more.\n";
    } else if (best == current) {
        out_ << "That already matches the best combination.\n";
    } else {
        out_ << "That is " << (best - current)
             << " lower: the policy picks offers line by line or relies\n"
                "  on a guard cut, which the combination search (whole offer sets, no cuts)\n"
                "  does not count.\n";
    }
}

void CliApp::logOrder(const domain::Customer& buyer, const engine::PricingResult& result) {
    if (!log_.enabled(infrastructure::LogLevel::Info)) return;
    std::set<std::string> seen;
    std::string offers;
    const auto note = [&](const std::string& id) {
        if (seen.insert(id).second) offers += (offers.empty() ? "" : ", ") + id;
    };
    for (const auto& line : result.lines()) {
        for (const auto& d : line.itemDiscounts) note(d.ruleId);
    }
    for (const auto& d : result.cartDiscounts()) note(d.ruleId);
    log_.info("Order placed by " + buyer.id() + " (" + buyer.name() + "), policy " +
              result.resolverName() + ": " + std::to_string(result.lines().size()) +
              (result.lines().size() == 1 ? " line" : " lines") + ", total " +
              result.grandTotal().toString() + ", offers: " + (offers.empty() ? "none" : offers));
}

void CliApp::suggestUpsells() {
    if (!requireCart()) return;
    const auto suggestions =
        application::UpsellAdvisor().suggest(context(), rules_, policy(), productList());
    out_ << '\n';
    UpsellPrinter().print(out_, suggestions, policy().name());
}

} // namespace presentation
