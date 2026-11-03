#pragma once

// Shared products, customers and helpers for the rule tests.

#include <optional>
#include <string>

#include "domain/Cart.hpp"
#include "domain/Clothing.hpp"
#include "domain/Customer.hpp"
#include "domain/Date.hpp"
#include "domain/Electronics.hpp"
#include "domain/Furniture.hpp"
#include "domain/Grocery.hpp"
#include "domain/Money.hpp"
#include "domain/PricingContext.hpp"
#include "domain/TimeOfDay.hpp"

namespace fixtures {

using domain::Money;

inline Money rs(std::int64_t rupees, std::int64_t paise = 0) {
    return Money::fromRupees(rupees, paise);
}

inline const domain::Date kToday(2026, 10, 28);

/// Owns a cart and customer so a PricingContext can safely refer to them.
struct Order {
    domain::Cart cart;
    domain::Customer customer{"CU1", "Asha", domain::MembershipTier::Regular};
    domain::Date date = kToday;
    std::optional<std::string> coupon;
    std::optional<std::string> bank;
    std::optional<domain::TimeOfDay> time;

    domain::PricingContext context() const {
        return domain::PricingContext(cart, customer, date, coupon, bank, time);
    }
};

inline const domain::Electronics& phone() {
    static const domain::Electronics p("E1", "Phone", rs(12000), rs(15000), rs(18000));
    return p;
}
inline const domain::Electronics& oddPricedCharger() {
    // 999.99 - percentages of it need rounding to the nearest paisa.
    static const domain::Electronics p("E2", "Charger", rs(500), rs(999, 99), rs(1200));
    return p;
}
inline const domain::Clothing& kurta() {
    static const domain::Clothing p("C1", "Kurta", rs(400), rs(1000), rs(1200), domain::Size::M,
                                    domain::Season::AllSeason);
    return p;
}
inline const domain::Furniture& chair() {
    static const domain::Furniture p("F1", "Chair", rs(2000), rs(4000), rs(5000), true);
    return p;
}

/// A grocery item expiring @p daysFromToday days after kToday (negative = already expired).
inline domain::Grocery makeMilk(const std::string& id, long daysFromToday) {
    int y = kToday.year(), m = kToday.month(), d = kToday.day();
    for (long i = 0; i < daysFromToday; ++i) {
        if (++d > domain::Date::daysInMonth(y, m)) { d = 1; if (++m > 12) { m = 1; ++y; } }
    }
    for (long i = 0; i > daysFromToday; --i) {
        if (--d < 1) { if (--m < 1) { m = 12; --y; } d = domain::Date::daysInMonth(y, m); }
    }
    return domain::Grocery(id, "Milk", rs(40), rs(60), rs(65), domain::Date(y, m, d));
}

} // namespace fixtures
