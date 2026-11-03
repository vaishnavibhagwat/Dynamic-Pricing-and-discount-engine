#include "domain/Money.hpp"

#include <ostream>

namespace domain {

namespace {
constexpr const char* kRupeeSign = "\xE2\x82\xB9"; // ₹ in UTF-8
}

Money Money::percentOf(std::int64_t percent) const noexcept {
    const std::int64_t product = paise_ * percent;
    // Round half away from zero: add/subtract 50 before dividing by 100.
    const std::int64_t rounded = product >= 0 ? (product + 50) / 100 : (product - 50) / 100;
    return Money(rounded);
}

std::string Money::toString() const {
    // Work with the unsigned magnitude so INT64_MIN is handled safely.
    const bool negative = paise_ < 0;
    const std::uint64_t magnitude = negative
        ? static_cast<std::uint64_t>(-(paise_ + 1)) + 1
        : static_cast<std::uint64_t>(paise_);

    const std::uint64_t rupees = magnitude / 100;
    const std::uint64_t fraction = magnitude % 100;

    // Indian grouping: last three digits, then groups of two (1,23,45,678).
    const std::string digits = std::to_string(rupees);
    std::string grouped;
    if (digits.size() <= 3) {
        grouped = digits;
    } else {
        const std::string lastThree = digits.substr(digits.size() - 3);
        const std::string rest = digits.substr(0, digits.size() - 3);
        const std::size_t firstGroup = rest.size() % 2 == 0 ? 2 : 1;
        grouped = rest.substr(0, firstGroup);
        for (std::size_t i = firstGroup; i < rest.size(); i += 2) {
            grouped += ',';
            grouped += rest.substr(i, 2);
        }
        grouped += ',';
        grouped += lastThree;
    }

    std::string result;
    if (negative) result += '-';
    result += kRupeeSign;
    result += grouped;
    result += '.';
    if (fraction < 10) result += '0';
    result += std::to_string(fraction);
    return result;
}

std::ostream& operator<<(std::ostream& os, const Money& m) {
    return os << m.toString();
}

} // namespace domain
