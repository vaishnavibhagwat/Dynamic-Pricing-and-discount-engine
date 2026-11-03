#pragma once

#include <iosfwd>
#include <string>

#include "infrastructure/LogSink.hpp"

namespace infrastructure {

/**
 * @brief Writes each log message as one line, "[LEVEL] message", to a stream
 *        such as std::clog or a std::ostringstream in tests.
 *
 * OOP ideas: a concrete LogSink (Liskov - usable wherever a LogSink is
 * expected). The stream is not owned; it must outlive the sink.
 */
class StreamLogSink final : public LogSink {
public:
    explicit StreamLogSink(std::ostream& os);

    void write(LogLevel level, const std::string& message) override;

    /// The line written for a message, without the newline: "[INFO] hello".
    static std::string format(LogLevel level, const std::string& message);

private:
    std::ostream& os_;
};

} // namespace infrastructure
