#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include "infrastructure/Logger.hpp"
#include "infrastructure/StreamLogSink.hpp"

using infrastructure::LogLevel;
using infrastructure::StreamLogSink;

TEST(StreamLogSink, WritesOneLabelledLinePerMessage) {
    std::ostringstream out;
    StreamLogSink sink(out);
    sink.write(LogLevel::Info, "Loaded 8 products");
    sink.write(LogLevel::Warning, "Rule reload failed");
    EXPECT_EQ(out.str(), "[INFO] Loaded 8 products\n[WARN] Rule reload failed\n");
}

TEST(StreamLogSink, FormatHasNoNewline) {
    EXPECT_EQ(StreamLogSink::format(LogLevel::Error, "boom"), "[ERROR] boom");
}

TEST(StreamLogSink, WorksThroughTheLogger) {
    std::ostringstream out;
    infrastructure::Logger log(LogLevel::Debug);
    log.addSink(std::make_unique<StreamLogSink>(out));
    log.debug("step 1");
    EXPECT_EQ(out.str(), "[DEBUG] step 1\n");
}
