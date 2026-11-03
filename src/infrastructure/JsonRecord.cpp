#include "infrastructure/JsonRecord.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "infrastructure/Exceptions.hpp"

namespace infrastructure {

using nlohmann::json;

JsonRecord::JsonRecord(const json& j, std::string kind) : json_(j), kind_(std::move(kind)) {
    if (!json_.is_object()) throw ConfigError(kind_ + " entry must be a JSON object");
    const auto it = json_.find("id");
    if (it == json_.end() || !it->is_string() || it->get<std::string>().empty()) {
        throw ConfigError(kind_ + " entry is missing a non-empty string \"id\"");
    }
    id_ = it->get<std::string>();
}

json JsonRecord::readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw ConfigError("cannot open file: " + path);
    json document;
    try {
        in >> document;
    } catch (const json::parse_error& e) {
        throw ConfigError("invalid JSON in " + path + ": " + e.what());
    }
    return document;
}

void JsonRecord::fail(const std::string& message) const {
    throw ConfigError(kind_ + " '" + id_ + "': " + message);
}

bool JsonRecord::has(const std::string& key) const {
    return json_.contains(key) && !json_.at(key).is_null();
}

const json& JsonRecord::field(const std::string& key) const {
    if (!has(key)) fail("missing field \"" + key + "\"");
    return json_.at(key);
}

std::string JsonRecord::requireString(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_string()) fail("field \"" + key + "\" must be a string");
    return value.get<std::string>();
}

bool JsonRecord::requireBool(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_boolean()) fail("field \"" + key + "\" must be true or false");
    return value.get<bool>();
}

int JsonRecord::requireInt(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_number_integer()) fail("field \"" + key + "\" must be a whole number");
    return value.get<int>();
}

domain::Money JsonRecord::requireRupees(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_number_integer()) fail("field \"" + key + "\" must be whole rupees");
    return domain::Money::fromRupees(value.get<std::int64_t>());
}

domain::Date JsonRecord::requireDate(const std::string& key) const {
    const std::string text = requireString(key);
    try {
        return domain::Date::fromString(text);
    } catch (const std::invalid_argument& e) {
        fail("field \"" + key + "\": " + e.what());
    }
}

} // namespace infrastructure
