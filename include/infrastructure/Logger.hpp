#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "infrastructure/LogSink.hpp"

namespace infrastructure {

/**
 * @brief Application log: sends each message at or above a minimum level to
 *        every attached sink (console, file, ...).
 *
 * Records what happened in a session - data loaded, orders placed, rules
 * reloaded, errors - separately from the per-order price trace.
 *
 * OOP ideas: Dependency Inversion - it depends only on the LogSink
 * abstraction, never on files or consoles. Null Object - a Logger with no
 * sinks is a valid, silent logger, so callers never check "is logging on?".
 * RAII ownership of the sinks through std::unique_ptr; move-only, so a
 * logger can be handed to the app that uses it.
 */
class Logger {
public:
    /// A silent logger until a sink is added.
    explicit Logger(LogLevel minLevel = LogLevel::Info) noexcept;

    /// Takes ownership. @throws std::invalid_argument if @p sink is null.
    void addSink(std::unique_ptr<LogSink> sink);
    std::size_t sinkCount() const noexcept { return sinks_.size(); }

    LogLevel minLevel() const noexcept { return minLevel_; }
    void setMinLevel(LogLevel level) noexcept { minLevel_ = level; }
    /// True if a message at @p level would reach at least one sink.
    bool enabled(LogLevel level) const noexcept;

    void log(LogLevel level, const std::string& message);
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }

private:
    LogLevel minLevel_;
    std::vector<std::unique_ptr<LogSink>> sinks_;
};

} // namespace infrastructure
