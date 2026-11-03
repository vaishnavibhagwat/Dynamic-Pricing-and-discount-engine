#include "infrastructure/StreamLogSink.hpp"

#include <ostream>

namespace infrastructure {

StreamLogSink::StreamLogSink(std::ostream& os) : os_(os) {}

std::string StreamLogSink::format(LogLevel level, const std::string& message) {
    return "[" + toString(level) + "] " + message;
}

void StreamLogSink::write(LogLevel level, const std::string& message) {
    // Flush every line so the log is complete even if the program stops early.
    os_ << format(level, message) << std::endl;
}

} // namespace infrastructure
