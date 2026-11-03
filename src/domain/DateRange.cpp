#include "domain/DateRange.hpp"

#include <algorithm>
#include <stdexcept>

namespace domain {

DateRange::DateRange(Date start, Date end) : start_(start), end_(end) {
    if (end_ < start_) {
        throw std::invalid_argument("DateRange: start " + start_.toString() + " is after end " +
                                    end_.toString());
    }
}

std::optional<DateRange> DateRange::intersect(const DateRange& other) const {
    const Date from = std::max(start_, other.start_);
    const Date to = std::min(end_, other.end_);
    if (to < from) return std::nullopt;
    return DateRange(from, to);
}

std::string DateRange::toString() const {
    return start_.toString() + " to " + end_.toString();
}

} // namespace domain
