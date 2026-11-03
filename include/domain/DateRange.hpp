#pragma once

#include <optional>
#include <string>

#include "domain/Date.hpp"

namespace domain {

/**
 * @brief An inclusive span of calendar days, e.g. a sale's start and end.
 *
 * OOP idea: a small immutable value object with a class invariant
 * (start <= end, checked once in the constructor), so callers never have
 * to re-check it.
 */
class DateRange {
public:
    /// @throws std::invalid_argument if end is before start.
    DateRange(Date start, Date end);

    const Date& start() const noexcept { return start_; }
    const Date& end() const noexcept { return end_; }

    /// True if @p day is within [start, end].
    bool contains(const Date& day) const noexcept { return start_ <= day && day <= end_; }
    /// Days both ranges share, or nullopt if they do not meet.
    std::optional<DateRange> intersect(const DateRange& other) const;

    /// "YYYY-MM-DD to YYYY-MM-DD"
    std::string toString() const;

    friend bool operator==(const DateRange& a, const DateRange& b) noexcept {
        return a.start_ == b.start_ && a.end_ == b.end_;
    }
    friend bool operator!=(const DateRange& a, const DateRange& b) noexcept { return !(a == b); }

private:
    Date start_;
    Date end_;
};

} // namespace domain
