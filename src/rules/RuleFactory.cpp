#include "rules/RuleFactory.hpp"

#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "domain/Customer.hpp"
#include "domain/TimeOfDay.hpp"
#include "infrastructure/Exceptions.hpp"
#include "infrastructure/JsonRecord.hpp"
#include "rules/BankOfferDiscount.hpp"
#include "rules/CappedRule.hpp"
#include "rules/CouponDiscount.hpp"
#include "rules/ExpiryMarkdown.hpp"
#include "rules/FirstOrderRule.hpp"
#include "rules/HappyHourRule.hpp"
#include "rules/MembershipDiscount.hpp"
#include "rules/PercentageDiscount.hpp"
#include "rules/QuantityDiscount.hpp"
#include "rules/SeasonalDiscount.hpp"

namespace rules {

using infrastructure::ConfigError;
using nlohmann::json;

// ---------------------------------------------------------------- RuleSpec

RuleSpec::RuleSpec(const json& j) : json_(j) {
    if (!json_.is_object()) throw ConfigError("rule entry must be a JSON object");
    const auto it = json_.find("id");
    if (it == json_.end() || !it->is_string() || it->get<std::string>().empty()) {
        throw ConfigError("rule entry is missing a non-empty string \"id\"");
    }
    id_ = it->get<std::string>();
}

void RuleSpec::fail(const std::string& message) const {
    throw ConfigError("rule '" + id_ + "': " + message);
}

bool RuleSpec::has(const std::string& key) const {
    return json_.contains(key) && !json_.at(key).is_null();
}

const json& RuleSpec::field(const std::string& key) const {
    if (!has(key)) fail("missing field \"" + key + "\"");
    return json_.at(key);
}

std::string RuleSpec::requireString(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_string()) fail("field \"" + key + "\" must be a string");
    return value.get<std::string>();
}

int RuleSpec::requireInt(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_number_integer()) fail("field \"" + key + "\" must be a whole number");
    return value.get<int>();
}

std::optional<int> RuleSpec::optionalInt(const std::string& key) const {
    if (!has(key)) return std::nullopt;
    return requireInt(key);
}

domain::Money RuleSpec::requireRupees(const std::string& key) const {
    const json& value = field(key);
    if (!value.is_number_integer()) fail("field \"" + key + "\" must be whole rupees");
    return domain::Money::fromRupees(value.get<std::int64_t>());
}

std::optional<domain::Money> RuleSpec::optionalRupees(const std::string& key) const {
    if (!has(key)) return std::nullopt;
    return requireRupees(key);
}

domain::Date RuleSpec::requireDate(const std::string& key) const {
    const std::string text = requireString(key);
    try {
        return domain::Date::fromString(text);
    } catch (const std::invalid_argument& e) {
        fail("field \"" + key + "\": " + e.what());
    }
}

std::string RuleSpec::name() const { return requireString("name"); }

int RuleSpec::priority() const { return requireInt("priority"); }

bool RuleSpec::stackable() const {
    if (!has("stackable")) return true;
    const json& value = json_.at("stackable");
    if (!value.is_boolean()) fail("field \"stackable\" must be true or false");
    return value.get<bool>();
}

ItemFilter RuleSpec::itemFilter() const {
    std::optional<std::string> category;
    if (has("category")) category = requireString("category");
    std::vector<std::string> ids;
    if (has("product_ids")) {
        const json& list = json_.at("product_ids");
        if (!list.is_array()) fail("field \"product_ids\" must be an array of strings");
        for (const auto& entry : list) {
            if (!entry.is_string()) fail("field \"product_ids\" must be an array of strings");
            ids.push_back(entry.get<std::string>());
        }
    }
    return ItemFilter(std::move(category), std::move(ids));
}

// ------------------------------------------------------ built-in creators

namespace {

domain::MembershipTier parseTier(const RuleSpec& spec, const std::string& text) {
    for (const auto tier : domain::allMembershipTiers()) {
        if (domain::toString(tier) == text) return tier;
    }
    spec.fail("unknown membership tier \"" + text + "\"");
}

std::unique_ptr<DiscountRule> makePercentage(const RuleSpec& s) {
    return std::make_unique<PercentageDiscount>(s.id(), s.name(), s.priority(), s.stackable(),
                                                s.requireInt("percent"), s.itemFilter());
}

std::unique_ptr<DiscountRule> makeQuantity(const RuleSpec& s) {
    return std::make_unique<QuantityDiscount>(s.id(), s.name(), s.priority(), s.stackable(),
                                              s.requireInt("min_quantity"),
                                              s.requireInt("percent"), s.itemFilter());
}

std::unique_ptr<DiscountRule> makeSeasonal(const RuleSpec& s) {
    return std::make_unique<SeasonalDiscount>(s.id(), s.name(), s.priority(), s.stackable(),
                                              s.requireDate("start"), s.requireDate("end"),
                                              s.requireInt("percent"), s.itemFilter());
}

std::unique_ptr<DiscountRule> makeExpiry(const RuleSpec& s) {
    if (!s.has("tiers") || !s.raw().at("tiers").is_array()) {
        s.fail("field \"tiers\" must be an array");
    }
    std::vector<ExpiryMarkdown::Tier> tiers;
    for (const auto& entry : s.raw().at("tiers")) {
        if (!entry.is_object() || !entry.contains("max_days_left") || !entry.contains("percent") ||
            !entry.at("max_days_left").is_number_integer() ||
            !entry.at("percent").is_number_integer()) {
            s.fail("each tier needs whole-number \"max_days_left\" and \"percent\"");
        }
        tiers.push_back({entry.at("max_days_left").get<int>(), entry.at("percent").get<int>()});
    }
    return std::make_unique<ExpiryMarkdown>(s.id(), s.name(), s.priority(), s.stackable(),
                                            std::move(tiers));
}

std::unique_ptr<DiscountRule> makeMembership(const RuleSpec& s) {
    if (!s.has("tiers") || !s.raw().at("tiers").is_object()) {
        s.fail("field \"tiers\" must be an object like {\"Gold\": 5}");
    }
    std::map<domain::MembershipTier, int> percents;
    for (const auto& [tierName, percent] : s.raw().at("tiers").items()) {
        if (!percent.is_number_integer()) s.fail("tier \"" + tierName + "\" percent must be a whole number");
        percents[parseTier(s, tierName)] = percent.get<int>();
    }
    return std::make_unique<MembershipDiscount>(s.id(), s.name(), s.priority(), s.stackable(),
                                                std::move(percents));
}

std::unique_ptr<DiscountRule> makeCoupon(const RuleSpec& s) {
    const bool hasPercent = s.has("percent");
    const bool hasFlat = s.has("flat");
    if (hasPercent == hasFlat) s.fail("coupon needs exactly one of \"percent\" or \"flat\"");
    if (hasFlat && s.has("cap")) s.fail("\"cap\" only applies to percent coupons");
    auto benefit = hasFlat ? CouponDiscount::Benefit::flat(s.requireRupees("flat"))
                           : CouponDiscount::Benefit::percent(s.requireInt("percent"),
                                                              s.optionalRupees("cap"));
    return std::make_unique<CouponDiscount>(
        s.id(), s.name(), s.priority(), s.stackable(), s.requireString("code"),
        s.optionalRupees("min_cart_value").value_or(domain::Money::zero()),
        s.requireInt("usage_limit"), std::move(benefit));
}

std::unique_ptr<DiscountRule> makeBank(const RuleSpec& s) {
    return std::make_unique<BankOfferDiscount>(
        s.id(), s.name(), s.priority(), s.stackable(), s.requireString("bank"),
        s.requireInt("percent"), s.requireRupees("cap"),
        s.optionalRupees("min_transaction").value_or(domain::Money::zero()));
}

// ---------------------------------------------------- built-in decorators

std::unique_ptr<DiscountRule> makeCap(std::unique_ptr<DiscountRule> rule, const RuleSpec& s) {
    return std::make_unique<CappedRule>(std::move(rule), s.requireRupees("max"));
}

std::unique_ptr<DiscountRule> makeHappyHour(std::unique_ptr<DiscountRule> rule, const RuleSpec& s) {
    return std::make_unique<HappyHourRule>(std::move(rule),
                                           domain::TimeOfDay::fromString(s.requireString("from")),
                                           domain::TimeOfDay::fromString(s.requireString("to")));
}

std::unique_ptr<DiscountRule> makeFirstOrder(std::unique_ptr<DiscountRule> rule, const RuleSpec&) {
    return std::make_unique<FirstOrderRule>(std::move(rule));
}

} // namespace

// ------------------------------------------------------------- RuleFactory

RuleFactory::RuleFactory() {
    registerType("percentage", makePercentage);
    registerType("quantity", makeQuantity);
    registerType("seasonal", makeSeasonal);
    registerType("expiry", makeExpiry);
    registerType("membership", makeMembership);
    registerType("coupon", makeCoupon);
    registerType("bank", makeBank);

    registerDecorator("cap", makeCap);
    registerDecorator("happy_hour", makeHappyHour);
    registerDecorator("first_order", makeFirstOrder);
}

void RuleFactory::registerDecorator(const std::string& type, DecoratorCreator creator) {
    if (type.empty()) throw std::invalid_argument("RuleFactory: decorator type must not be empty");
    if (!creator) throw std::invalid_argument("RuleFactory: decorator creator must not be empty");
    if (!decorators_.emplace(type, std::move(creator)).second) {
        throw std::invalid_argument("RuleFactory: decorator already registered: " + type);
    }
}

bool RuleFactory::supportsDecorator(const std::string& type) const {
    return decorators_.count(type) > 0;
}

std::unique_ptr<DiscountRule> RuleFactory::decorate(std::unique_ptr<DiscountRule> rule,
                                                    const RuleSpec& spec) const {
    if (!spec.has("decorators")) return rule;
    const json& list = spec.raw().at("decorators");
    if (!list.is_array()) spec.fail("field \"decorators\" must be an array");
    for (const auto& entry : list) {
        if (!entry.is_object()) spec.fail("each decorator must be a JSON object");
        json withId = entry;
        withId["id"] = spec.id(); // so errors name the rule being decorated
        const RuleSpec decoratorSpec(withId);
        const std::string type = decoratorSpec.requireString("type");
        const auto it = decorators_.find(type);
        if (it == decorators_.end()) spec.fail("unknown decorator type \"" + type + "\"");
        rule = it->second(std::move(rule), decoratorSpec);
        if (!rule) spec.fail("decorator \"" + type + "\" returned nothing");
    }
    return rule;
}

void RuleFactory::registerType(const std::string& type, Creator creator) {
    if (type.empty()) throw std::invalid_argument("RuleFactory: type must not be empty");
    if (!creator) throw std::invalid_argument("RuleFactory: creator must not be empty");
    if (!creators_.emplace(type, std::move(creator)).second) {
        throw std::invalid_argument("RuleFactory: type already registered: " + type);
    }
}

bool RuleFactory::supports(const std::string& type) const {
    return creators_.count(type) > 0;
}

std::vector<std::string> RuleFactory::supportedTypes() const {
    std::vector<std::string> types;
    for (const auto& entry : creators_) types.push_back(entry.first);
    return types;
}

std::unique_ptr<DiscountRule> RuleFactory::create(const json& spec) const {
    const RuleSpec reader(spec);
    const std::string type = reader.requireString("type");
    const auto it = creators_.find(type);
    if (it == creators_.end()) reader.fail("unknown rule type \"" + type + "\"");
    try {
        auto rule = it->second(reader);
        if (!rule) reader.fail("creator for type \"" + type + "\" returned nothing");
        return decorate(std::move(rule), reader);
    } catch (const ConfigError&) {
        throw;
    } catch (const std::invalid_argument& e) {
        // Constructor validation (bad percent, start after end, ...).
        reader.fail(e.what());
    } catch (const json::exception& e) {
        reader.fail(std::string("bad JSON value: ") + e.what());
    }
}

Repository<DiscountRule> RuleFactory::createAll(const json& document) const {
    const json* list = &document;
    if (document.is_object()) {
        const auto it = document.find("rules");
        if (it == document.end()) throw ConfigError("rules document has no \"rules\" array");
        list = &*it;
    }
    if (!list->is_array()) throw ConfigError("\"rules\" must be an array");

    Repository<DiscountRule> repo;
    for (const auto& entry : *list) {
        auto rule = create(entry);
        if (repo.findById(rule->id()) != nullptr) {
            throw ConfigError("duplicate rule id '" + rule->id() + "'");
        }
        repo.add(std::move(rule));
    }
    return repo;
}

Repository<DiscountRule> RuleFactory::loadFile(const std::string& path) const {
    return createAll(infrastructure::JsonRecord::readFile(path));
}

} // namespace rules
