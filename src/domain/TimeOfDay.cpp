#include "domain/TimeOfDay.hpp"

#include <cctype>
#include <ostream>
#include <stdexcept>

namespace domain {

TimeOfDay::TimeOfDay(int hour, int minute) : minutes_(hour * 60 + minute) {
    if (hour < 0 || hour > 23) throw std::invalid_argument("hour must be 0-23");
    if (minute < 0 || minute > 59) throw std::invalid_argument("minute must be 0-59");
}

TimeOfDay TimeOfDay::fromString(const std::string& text) {
    const std::invalid_argument bad("time must be HH:MM, got \"" + text + "\"");
    const auto colon = text.find(':');
    if (colon == std::string::npos || colon == 0 || colon > 2 || text.size() != colon + 3) {
        throw bad;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i != colon && !std::isdigit(static_cast<unsigned char>(text[i]))) throw bad;
    }
    return TimeOfDay(std::stoi(text.substr(0, colon)), std::stoi(text.substr(colon + 1)));
}

std::string TimeOfDay::toString() const {
    const auto two = [](int n) { return (n < 10 ? "0" : "") + std::to_string(n); };
    return two(hour()) + ":" + two(minute());
}

std::ostream& operator<<(std::ostream& os, TimeOfDay t) {
    return os << t.toString();
}

} // namespace domain
