#pragma once

#include <iosfwd>
#include <string>

namespace domain {

/**
 * @brief A calendar date (year, month, day), always valid once constructed.
 *
 * OOP idea: encapsulation with a class invariant. The constructor rejects
 * impossible dates (e.g. 30 Feb), so the rest of the program can trust any
 * Date it receives.
 */
class Date {
public:
    /// @throws std::invalid_argument if the date does not exist.
    Date(int year, int month, int day);

    /// Parses "YYYY-MM-DD". @throws std::invalid_argument on bad format or impossible date.
    static Date fromString(const std::string& text);

    int year() const noexcept { return year_; }
    int month() const noexcept { return month_; }
    int day() const noexcept { return day_; }

    /// Number of days from this date to @p other (negative if other is earlier).
    long daysUntil(const Date& other) const noexcept;

    /// "YYYY-MM-DD"
    std::string toString() const;

    static bool isLeapYear(int year) noexcept;
    static int daysInMonth(int year, int month) noexcept;

    friend bool operator==(const Date& a, const Date& b) noexcept {
        return a.year_ == b.year_ && a.month_ == b.month_ && a.day_ == b.day_;
    }
    friend bool operator!=(const Date& a, const Date& b) noexcept { return !(a == b); }
    friend bool operator<(const Date& a, const Date& b) noexcept {
        if (a.year_ != b.year_) return a.year_ < b.year_;
        if (a.month_ != b.month_) return a.month_ < b.month_;
        return a.day_ < b.day_;
    }
    friend bool operator>(const Date& a, const Date& b) noexcept { return b < a; }
    friend bool operator<=(const Date& a, const Date& b) noexcept { return !(b < a); }
    friend bool operator>=(const Date& a, const Date& b) noexcept { return !(a < b); }

private:
    long toDayNumber() const noexcept;

    int year_;
    int month_;
    int day_;
};

std::ostream& operator<<(std::ostream& os, const Date& d);

} // namespace domain
