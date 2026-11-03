#include "infrastructure/CatalogLoader.hpp"

#include <set>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "domain/Clothing.hpp"
#include "domain/Electronics.hpp"
#include "domain/Furniture.hpp"
#include "domain/Grocery.hpp"
#include "infrastructure/Exceptions.hpp"

namespace infrastructure {

using nlohmann::json;

namespace {

domain::Size parseSize(const JsonRecord& r) {
    const std::string text = r.requireString("size");
    for (const auto s : {domain::Size::XS, domain::Size::S, domain::Size::M, domain::Size::L,
                         domain::Size::XL, domain::Size::XXL}) {
        if (domain::toString(s) == text) return s;
    }
    r.fail("unknown size \"" + text + "\" (expected XS, S, M, L, XL or XXL)");
}

domain::Season parseSeason(const JsonRecord& r) {
    const std::string text = r.requireString("season");
    for (const auto s : {domain::Season::Summer, domain::Season::Winter, domain::Season::Monsoon,
                         domain::Season::AllSeason}) {
        if (domain::toString(s) == text) return s;
    }
    r.fail("unknown season \"" + text + "\"");
}

std::unique_ptr<domain::Product> makeElectronics(const JsonRecord& r) {
    return std::make_unique<domain::Electronics>(r.id(), r.requireString("name"),
                                                 r.requireRupees("cost"), r.requireRupees("price"),
                                                 r.requireRupees("mrp"));
}

std::unique_ptr<domain::Product> makeClothing(const JsonRecord& r) {
    return std::make_unique<domain::Clothing>(r.id(), r.requireString("name"),
                                              r.requireRupees("cost"), r.requireRupees("price"),
                                              r.requireRupees("mrp"), parseSize(r),
                                              parseSeason(r));
}

std::unique_ptr<domain::Product> makeFurniture(const JsonRecord& r) {
    return std::make_unique<domain::Furniture>(r.id(), r.requireString("name"),
                                               r.requireRupees("cost"), r.requireRupees("price"),
                                               r.requireRupees("mrp"),
                                               r.requireBool("assembly_required"));
}

std::unique_ptr<domain::Product> makeGrocery(const JsonRecord& r) {
    return std::make_unique<domain::Grocery>(r.id(), r.requireString("name"),
                                             r.requireRupees("cost"), r.requireRupees("price"),
                                             r.requireRupees("mrp"), r.requireDate("expiry"));
}

} // namespace

CatalogLoader::CatalogLoader() {
    registerType("electronics", makeElectronics);
    registerType("clothing", makeClothing);
    registerType("furniture", makeFurniture);
    registerType("grocery", makeGrocery);
}

void CatalogLoader::registerType(const std::string& type, Creator creator) {
    if (type.empty()) throw std::invalid_argument("CatalogLoader: type must not be empty");
    if (!creator) throw std::invalid_argument("CatalogLoader: creator must not be empty");
    if (!creators_.emplace(type, std::move(creator)).second) {
        throw std::invalid_argument("CatalogLoader: type already registered: " + type);
    }
}

bool CatalogLoader::supports(const std::string& type) const {
    return creators_.count(type) > 0;
}

std::unique_ptr<domain::Product> CatalogLoader::create(const json& entry) const {
    const JsonRecord record(entry, "product");
    const std::string type = record.requireString("type");
    const auto it = creators_.find(type);
    if (it == creators_.end()) record.fail("unknown product type \"" + type + "\"");
    try {
        auto product = it->second(record);
        if (!product) record.fail("creator for type \"" + type + "\" returned nothing");
        return product;
    } catch (const ConfigError&) {
        throw;
    } catch (const std::invalid_argument& e) {
        // Product constructor validation (e.g. price above MRP).
        record.fail(e.what());
    } catch (const json::exception& e) {
        record.fail(std::string("bad JSON value: ") + e.what());
    }
}

std::vector<std::unique_ptr<domain::Product>> CatalogLoader::fromJson(const json& document) const {
    const json* list = &document;
    if (document.is_object()) {
        const auto it = document.find("products");
        if (it == document.end()) throw ConfigError("catalog has no \"products\" array");
        list = &*it;
    }
    if (!list->is_array()) throw ConfigError("\"products\" must be an array");

    std::vector<std::unique_ptr<domain::Product>> products;
    std::set<std::string> seen;
    for (const auto& entry : *list) {
        auto product = create(entry);
        if (!seen.insert(product->id()).second) {
            throw ConfigError("duplicate product id '" + product->id() + "'");
        }
        products.push_back(std::move(product));
    }
    return products;
}

std::vector<std::unique_ptr<domain::Product>> CatalogLoader::loadFile(
    const std::string& path) const {
    return fromJson(JsonRecord::readFile(path));
}

} // namespace infrastructure
