#pragma once

#include <iosfwd>
#include <string>

namespace domain {

/**
 * @brief A clock time within a day, to the minute (e.g. 18:30).
 *
 * OOP ideas: value object and encapsulation - the hour and minute are
 * checked once in the constructor, so an invalid time can never exist.
 * Operator overloading gives natural comparisons (`now < closing`).
 */
class TimeOfDay {
public:
    /// @throws std::invalid_argument unless 0 <= hour <= 23 and 0 <= minute <= 59.
    TimeOfDay(int hour, int minute);
    /// Parses "HH:MM" (24-hour clock; the hour may be one digit).
    /// @throws std::invalid_argument on bad text.
    static TimeOfDay fromString(const std::string& text);

    int hour() const noexcept { return minutes_ / 60; }
    int minute() const noexcept { return minutes_ % 60; }
    int minutesSinceMidnight() const noexcept { return minutes_; }
    /// "HH:MM", zero-padded.
    std::string toString() const;

    friend bool operator==(TimeOfDay a, TimeOfDay b) noexcept { return a.minutes_ == b.minutes_; }
    friend bool operator!=(TimeOfDay a, TimeOfDay b) noexcept { return !(a == b); }
    friend bool operator<(TimeOfDay a, TimeOfDay b) noexcept { return a.minutes_ < b.minutes_; }
    friend bool operator>(TimeOfDay a, TimeOfDay b) noexcept { return b < a; }
    friend bool operator<=(TimeOfDay a, TimeOfDay b) noexcept { return !(b < a); }
    friend bool operator>=(TimeOfDay a, TimeOfDay b) noexcept { return !(a < b); }

private:
    int minutes_;
};

std::ostream& operator<<(std::ostream& os, TimeOfDay t);

} // namespace domain
