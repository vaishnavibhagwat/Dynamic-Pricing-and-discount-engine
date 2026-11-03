#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "infrastructure/Logger.hpp"

using infrastructure::LogLevel;
using infrastructure::Logger;
using infrastructure::LogSink;

namespace {

using Lines = std::vector<std::pair<LogLevel, std::string>>;

/// Keeps every message it receives, so tests can inspect them.
class RecordingSink final : public LogSink {
public:
    explicit RecordingSink(Lines& lines) : lines_(lines) {}
    void write(LogLevel level, const std::string& message) override {
        lines_.emplace_back(level, message);
    }

private:
    Lines& lines_;
};

} // namespace

TEST(LogLevel, Names) {
    EXPECT_EQ(infrastructure::toString(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(infrastructure::toString(LogLevel::Info), "INFO");
    EXPECT_EQ(infrastructure::toString(LogLevel::Warning), "WARN");
    EXPECT_EQ(infrastructure::toString(LogLevel::Error), "ERROR");
}

TEST(Logger, WithoutSinksItIsSilent) {
    Logger log;
    EXPECT_EQ(log.sinkCount(), 0u);
    EXPECT_FALSE(log.enabled(LogLevel::Error));
    EXPECT_NO_THROW(log.error("nobody hears this"));
}

TEST(Logger, DropsMessagesBelowTheMinimumLevel) {
    Lines lines;
    Logger log(LogLevel::Warning);
    log.addSink(std::make_unique<RecordingSink>(lines));
    log.debug("d");
    log.info("i");
    log.warning("w");
    log.error("e");
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], std::make_pair(LogLevel::Warning, std::string("w")));
    EXPECT_EQ(lines[1], std::make_pair(LogLevel::Error, std::string("e")));
    EXPECT_FALSE(log.enabled(LogLevel::Info));
    EXPECT_TRUE(log.enabled(LogLevel::Warning));
}

TEST(Logger, DefaultLevelIsInfoAndCanBeChanged) {
    Lines lines;
    Logger log;
    log.addSink(std::make_unique<RecordingSink>(lines));
    EXPECT_EQ(log.minLevel(), LogLevel::Info);
    log.debug("hidden");
    log.setMinLevel(LogLevel::Debug);
    log.debug("shown");
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0].second, "shown");
}

TEST(Logger, SendsEachMessageToEverySink) {
    Lines first, second;
    Logger log;
    log.addSink(std::make_unique<RecordingSink>(first));
    log.addSink(std::make_unique<RecordingSink>(second));
    EXPECT_EQ(log.sinkCount(), 2u);
    log.log(LogLevel::Info, "hello");
    EXPECT_EQ(first, second);
    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first[0].second, "hello");
}

TEST(Logger, CanBeMovedWithItsSinks) {
    Lines lines;
    Logger original;
    original.addSink(std::make_unique<RecordingSink>(lines));
    Logger moved = std::move(original);
    moved.info("after move");
    ASSERT_EQ(lines.size(), 1u);
}

TEST(Logger, RejectsNullSink) {
    Logger log;
    EXPECT_THROW(log.addSink(nullptr), std::invalid_argument);
}
