#include "infrastructure/Logger.hpp"

#include <stdexcept>
#include <utility>

namespace infrastructure {

Logger::Logger(LogLevel minLevel) noexcept : minLevel_(minLevel) {}

void Logger::addSink(std::unique_ptr<LogSink> sink) {
    if (!sink) throw std::invalid_argument("Logger::addSink: null sink");
    sinks_.push_back(std::move(sink));
}

bool Logger::enabled(LogLevel level) const noexcept {
    return !sinks_.empty() && static_cast<int>(level) >= static_cast<int>(minLevel_);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!enabled(level)) return;
    for (const auto& sink : sinks_) sink->write(level, message);
}

} // namespace infrastructure
