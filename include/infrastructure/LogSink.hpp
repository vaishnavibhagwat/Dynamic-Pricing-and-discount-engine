#pragma once

#include <string>

namespace infrastructure {

/// How important a log message is, least important first.
enum class LogLevel { Debug, Info, Warning, Error };

/// "DEBUG", "INFO", "WARN" or "ERROR".
std::string toString(LogLevel level);

/**
 * @brief Somewhere log messages go: a console, a file, a test buffer...
 *
 * OOP ideas: abstraction (pure virtual interface with a virtual destructor)
 * and the Open/Closed Principle - Logger only talks to this interface, so a
 * new destination is a new subclass and Logger does not change. Strategy
 * pattern - the destination is chosen at run time.
 */
class LogSink {
public:
    virtual ~LogSink() = default;

    /// Writes one message. Called only for messages at or above the logger's level.
    virtual void write(LogLevel level, const std::string& message) = 0;

protected:
    LogSink() = default;
    LogSink(const LogSink&) = default;
    LogSink& operator=(const LogSink&) = default;
};

} // namespace infrastructure
