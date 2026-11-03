#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace domain {

/**
 * @brief An amount of Indian money stored as whole paise (int64).
 *
 * OOP idea: encapsulation and value objects. The paise count is private, so
 * money can only change through well-defined operators. Floating point is
 * never used, which avoids rounding errors. Operator overloading lets
 * prices read like ordinary arithmetic.
 */
class Money {
public:
    constexpr Money() noexcept = default;

    static constexpr Money fromPaise(std::int64_t paise) noexcept { return Money(paise); }
    /// e.g. fromRupees(499, 50) == ₹499.50
    static constexpr Money fromRupees(std::int64_t rupees, std::int64_t paise = 0) noexcept {
        return Money(rupees * 100 + paise);
    }
    static constexpr Money zero() noexcept { return Money(); }

    constexpr std::int64_t paise() const noexcept { return paise_; }
    constexpr bool isNegative() const noexcept { return paise_ < 0; }
    constexpr bool isZero() const noexcept { return paise_ == 0; }

    /// percent% of this amount, rounded half away from zero to the nearest paisa.
    Money percentOf(std::int64_t percent) const noexcept;

    /// Formats as ₹ with Indian digit grouping, e.g. "₹1,23,456.78".
    std::string toString() const;

    Money& operator+=(Money other) noexcept { paise_ += other.paise_; return *this; }
    Money& operator-=(Money other) noexcept { paise_ -= other.paise_; return *this; }
    Money& operator*=(std::int64_t factor) noexcept { paise_ *= factor; return *this; }

    friend constexpr Money operator+(Money a, Money b) noexcept { return Money(a.paise_ + b.paise_); }
    friend constexpr Money operator-(Money a, Money b) noexcept { return Money(a.paise_ - b.paise_); }
    friend constexpr Money operator-(Money a) noexcept { return Money(-a.paise_); }
    friend constexpr Money operator*(Money a, std::int64_t f) noexcept { return Money(a.paise_ * f); }
    friend constexpr Money operator*(std::int64_t f, Money a) noexcept { return Money(a.paise_ * f); }

    friend constexpr bool operator==(Money a, Money b) noexcept { return a.paise_ == b.paise_; }
    friend constexpr bool operator!=(Money a, Money b) noexcept { return a.paise_ != b.paise_; }
    friend constexpr bool operator<(Money a, Money b) noexcept { return a.paise_ < b.paise_; }
    friend constexpr bool operator>(Money a, Money b) noexcept { return a.paise_ > b.paise_; }
    friend constexpr bool operator<=(Money a, Money b) noexcept { return a.paise_ <= b.paise_; }
    friend constexpr bool operator>=(Money a, Money b) noexcept { return a.paise_ >= b.paise_; }

private:
    constexpr explicit Money(std::int64_t paise) noexcept : paise_(paise) {}

    std::int64_t paise_ = 0;
};

std::ostream& operator<<(std::ostream& os, const Money& m);

} // namespace domain
