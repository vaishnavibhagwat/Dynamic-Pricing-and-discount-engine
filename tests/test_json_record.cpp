#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include "infrastructure/Exceptions.hpp"
#include "infrastructure/JsonRecord.hpp"

using infrastructure::ConfigError;
using infrastructure::JsonRecord;
using nlohmann::json;

namespace {

/// Runs @p f and returns the ConfigError message it throws ("" if none).
template <typename F>
std::string errorOf(F f) {
    try {
        f();
    } catch (const ConfigError& e) {
        return e.what();
    }
    return "";
}

} // namespace

TEST(JsonRecord, ReadsTypedFields) {
    const json j = json::parse(R"({"id":"P1","name":"Pen","price":25,"ok":true,"on":"2026-10-30"})");
    const JsonRecord r(j, "product");
    EXPECT_EQ(r.id(), "P1");
    EXPECT_EQ(r.requireString("name"), "Pen");
    EXPECT_EQ(r.requireRupees("price"), domain::Money::fromRupees(25));
    EXPECT_TRUE(r.requireBool("ok"));
    EXPECT_EQ(r.requireDate("on"), domain::Date(2026, 10, 30));
    EXPECT_TRUE(r.has("name"));
    EXPECT_FALSE(r.has("missing"));
}

TEST(JsonRecord, RejectsNonObjectAndMissingId) {
    const json array = json::array();
    EXPECT_THROW(JsonRecord(array, "product"), ConfigError);
    const json noId = json::parse(R"({"name":"Pen"})");
    EXPECT_EQ(errorOf([&] { JsonRecord(noId, "product"); }),
              "product entry is missing a non-empty string \"id\"");
    const json emptyId = json::parse(R"({"id":""})");
    EXPECT_THROW(JsonRecord(emptyId, "customer"), ConfigError);
}

TEST(JsonRecord, ErrorsNameTheRecordAndField) {
    const json j = json::parse(R"({"id":"P1","price":"cheap","ok":1,"on":"2026-02-30","n":5})");
    const JsonRecord r(j, "product");
    EXPECT_EQ(errorOf([&] { r.requireString("mrp"); }), "product 'P1': missing field \"mrp\"");
    EXPECT_EQ(errorOf([&] { r.requireRupees("price"); }),
              "product 'P1': field \"price\" must be whole rupees");
    EXPECT_EQ(errorOf([&] { r.requireBool("ok"); }),
              "product 'P1': field \"ok\" must be true or false");
    EXPECT_EQ(errorOf([&] { r.requireString("n"); }),
              "product 'P1': field \"n\" must be a string");
    EXPECT_NE(errorOf([&] { r.requireDate("on"); }).find("product 'P1': field \"on\""),
              std::string::npos);
}

TEST(JsonRecord, RupeesMustBeWholeNumbers) {
    const json j = json::parse(R"({"id":"P1","price":9.99})");
    EXPECT_THROW(JsonRecord(j, "product").requireRupees("price"), ConfigError);
}

TEST(JsonRecord, ReadFileReportsMissingAndInvalidFiles) {
    EXPECT_NE(errorOf([] { JsonRecord::readFile("no/such/file.json"); }).find("cannot open file"),
              std::string::npos);

    const std::string path = std::string(TEST_SCRATCH_DIR) + "/bad_record.json";
    {
        std::ofstream out(path);
        out << "{ not json";
    }
    EXPECT_NE(errorOf([&] { JsonRecord::readFile(path); }).find("invalid JSON"),
              std::string::npos);
    std::remove(path.c_str());
}

TEST(JsonRecord, ReadsWholeNumbers) {
    const json j = json::parse(R"({"id":"C1","n":7,"x":1.5})");
    const JsonRecord r(j, "customer");
    EXPECT_EQ(r.requireInt("n"), 7);
    EXPECT_THROW(r.requireInt("x"), ConfigError);
    EXPECT_THROW(r.requireInt("missing"), ConfigError);
}
