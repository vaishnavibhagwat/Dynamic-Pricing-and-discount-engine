#include "infrastructure/FileLogSink.hpp"

#include "infrastructure/Exceptions.hpp"

namespace infrastructure {

FileLogSink::FileLogSink(const std::string& path, bool append)
    : path_(path), file_(path, append ? std::ios::app : std::ios::trunc), lines_(file_) {
    if (!file_) throw ConfigError("cannot open log file: " + path);
}

void FileLogSink::write(LogLevel level, const std::string& message) {
    lines_.write(level, message);
}

} // namespace infrastructure
