#include "domain/Date.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace domain {

Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 ||
        day > daysInMonth(year, month)) {
        throw std::invalid_argument("Invalid date: " + std::to_string(year) + "-" +
                                    std::to_string(month) + "-" + std::to_string(day));
    }
}

Date Date::fromString(const std::string& text) {
    // Strict format: exactly 4 digits, '-', 2 digits, '-', 2 digits.
    const bool shapeOk = text.size() == 10 && text[4] == '-' && text[7] == '-';
    bool digitsOk = shapeOk;
    for (std::size_t i = 0; digitsOk && i < text.size(); ++i) {
        if (i == 4 || i == 7) continue;
        digitsOk = text[i] >= '0' && text[i] <= '9';
    }
    if (!digitsOk) throw std::invalid_argument("Invalid date format (want YYYY-MM-DD): " + text);
    return Date(std::stoi(text.substr(0, 4)), std::stoi(text.substr(5, 2)),
                std::stoi(text.substr(8, 2)));
}

bool Date::isLeapYear(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int Date::daysInMonth(int year, int month) noexcept {
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return kDays[month - 1];
}

// Days since 1970-01-01 (Howard Hinnant's days_from_civil algorithm).
long Date::toDayNumber() const noexcept {
    const int y = month_ <= 2 ? year_ - 1 : year_;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned mp = static_cast<unsigned>(month_ > 2 ? month_ - 3 : month_ + 9);
    const unsigned doy = (153 * mp + 2) / 5 + static_cast<unsigned>(day_) - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return static_cast<long>(era) * 146097 + static_cast<long>(doe) - 719468;
}

long Date::daysUntil(const Date& other) const noexcept {
    return other.toDayNumber() - toDayNumber();
}

std::string Date::toString() const {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year_ << '-' << std::setw(2) << month_
        << '-' << std::setw(2) << day_;
    return out.str();
}

std::ostream& operator<<(std::ostream& os, const Date& d) {
    return os << d.toString();
}

} // namespace domain
