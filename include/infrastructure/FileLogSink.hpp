#pragma once

#include <fstream>
#include <string>

#include "infrastructure/LogSink.hpp"
#include "infrastructure/StreamLogSink.hpp"

namespace infrastructure {

/**
 * @brief Appends log messages to a text file, one "[LEVEL] message" line each.
 *
 * OOP ideas: RAII - the file is opened in the constructor and closed by the
 * destructor, on every path. Composition and reuse - it owns the file and
 * hands the line formatting to a StreamLogSink instead of repeating it.
 */
class FileLogSink final : public LogSink {
public:
    /// Opens @p path for appending (or truncates it first if @p append is false).
    /// @throws ConfigError if the file cannot be opened.
    explicit FileLogSink(const std::string& path, bool append = true);

    void write(LogLevel level, const std::string& message) override;

    const std::string& path() const noexcept { return path_; }

private:
    std::string path_;
    std::ofstream file_;   // declared before lines_, so it is opened first
    StreamLogSink lines_;  // writes into file_
};

} // namespace infrastructure
