#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "infrastructure/Exceptions.hpp"
#include "infrastructure/FileLogSink.hpp"

using infrastructure::FileLogSink;
using infrastructure::LogLevel;

namespace {

std::string readAll(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
}

const std::string kPath = std::string(TEST_SCRATCH_DIR) + "/test_file_log_sink.log";

} // namespace

TEST(FileLogSink, WritesLinesAndClosesOnDestruction) {
    std::remove(kPath.c_str());
    {
        FileLogSink sink(kPath, false);
        EXPECT_EQ(sink.path(), kPath);
        sink.write(LogLevel::Info, "first");
        sink.write(LogLevel::Error, "second");
    } // file closed here (RAII)
    EXPECT_EQ(readAll(kPath), "[INFO] first\n[ERROR] second\n");
    std::remove(kPath.c_str());
}

TEST(FileLogSink, AppendsOrTruncates) {
    std::remove(kPath.c_str());
    { FileLogSink(kPath).write(LogLevel::Info, "one"); }
    { FileLogSink(kPath).write(LogLevel::Info, "two"); }
    EXPECT_EQ(readAll(kPath), "[INFO] one\n[INFO] two\n");
    { FileLogSink(kPath, false).write(LogLevel::Info, "fresh"); }
    EXPECT_EQ(readAll(kPath), "[INFO] fresh\n");
    std::remove(kPath.c_str());
}

TEST(FileLogSink, UnwritablePathThrowsConfigError) {
    EXPECT_THROW(FileLogSink("no/such/folder/app.log"), infrastructure::ConfigError);
}
