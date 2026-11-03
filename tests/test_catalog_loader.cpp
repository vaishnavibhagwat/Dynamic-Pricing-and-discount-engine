#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "domain/Clothing.hpp"
#include "domain/Electronics.hpp"
#include "domain/Furniture.hpp"
#include "domain/Grocery.hpp"
#include "infrastructure/CatalogLoader.hpp"
#include "infrastructure/Exceptions.hpp"

using domain::Money;
using infrastructure::CatalogLoader;
using infrastructure::ConfigError;
using nlohmann::json;

namespace {

std::string errorOf(const CatalogLoader& loader, const std::string& text) {
    try {
        loader.fromJson(json::parse(text));
    } catch (const ConfigError& e) {
        return e.what();
    }
    return "";
}

} // namespace

TEST(CatalogLoader, BuildsEveryBuiltInType) {
    const auto products = CatalogLoader().fromJson(json::parse(R"({"products":[
        {"id":"E1","type":"electronics","name":"Phone","cost":100,"price":150,"mrp":200},
        {"id":"C1","type":"clothing","name":"Kurta","cost":40,"price":100,"mrp":120,
         "size":"XL","season":"Winter"},
        {"id":"F1","type":"furniture","name":"Chair","cost":200,"price":400,"mrp":500,
         "assembly_required":false},
        {"id":"G1","type":"grocery","name":"Milk","cost":40,"price":60,"mrp":65,
         "expiry":"2026-11-01"}
    ]})"));
    ASSERT_EQ(products.size(), 4u);

    const auto* phone = dynamic_cast<const domain::Electronics*>(products[0].get());
    ASSERT_NE(phone, nullptr);
    EXPECT_EQ(phone->name(), "Phone");
    EXPECT_EQ(phone->costPrice(), Money::fromRupees(100));
    EXPECT_EQ(phone->sellingPrice(), Money::fromRupees(150));
    EXPECT_EQ(phone->mrp(), Money::fromRupees(200));

    const auto* kurta = dynamic_cast<const domain::Clothing*>(products[1].get());
    ASSERT_NE(kurta, nullptr);
    EXPECT_EQ(kurta->size(), domain::Size::XL);
    EXPECT_EQ(kurta->season(), domain::Season::Winter);

    const auto* chair = dynamic_cast<const domain::Furniture*>(products[2].get());
    ASSERT_NE(chair, nullptr);
    EXPECT_FALSE(chair->assemblyRequired());

    const auto* milk = dynamic_cast<const domain::Grocery*>(products[3].get());
    ASSERT_NE(milk, nullptr);
    EXPECT_EQ(milk->expiryDate(), domain::Date(2026, 11, 1));
}

TEST(CatalogLoader, AcceptsBareArray) {
    const auto products = CatalogLoader().fromJson(json::parse(
        R"([{"id":"E1","type":"electronics","name":"Phone","cost":1,"price":2,"mrp":3}])"));
    EXPECT_EQ(products.size(), 1u);
}

TEST(CatalogLoader, ReportsBadDocuments) {
    const CatalogLoader loader;
    EXPECT_EQ(errorOf(loader, R"({"items":[]})"), "catalog has no \"products\" array");
    EXPECT_EQ(errorOf(loader, R"({"products":5})"), "\"products\" must be an array");
}

TEST(CatalogLoader, ReportsBadProductsByIdAndField) {
    const CatalogLoader loader;
    EXPECT_EQ(errorOf(loader, R"([{"id":"X1","type":"toy","name":"Top"}])"),
              "product 'X1': unknown product type \"toy\"");
    EXPECT_EQ(errorOf(loader, R"([{"id":"E1","type":"electronics","name":"Phone","cost":1,
                                   "price":2}])"),
              "product 'E1': missing field \"mrp\"");
    EXPECT_EQ(errorOf(loader, R"([{"id":"C1","type":"clothing","name":"Kurta","cost":1,
                                   "price":2,"mrp":3,"size":"XXXL","season":"Winter"}])"),
              "product 'C1': unknown size \"XXXL\" (expected XS, S, M, L, XL or XXL)");
    EXPECT_EQ(errorOf(loader, R"([{"id":"C1","type":"clothing","name":"Kurta","cost":1,
                                   "price":2,"mrp":3,"size":"M","season":"Spring"}])"),
              "product 'C1': unknown season \"Spring\"");
    EXPECT_NE(errorOf(loader, R"([{"id":"G1","type":"grocery","name":"Milk","cost":1,
                                   "price":2,"mrp":3,"expiry":"soon"}])")
                  .find("product 'G1': field \"expiry\""),
              std::string::npos);
}

TEST(CatalogLoader, ProductValidationBecomesConfigError) {
    // Selling price above MRP is rejected by the Product constructor.
    const std::string msg = errorOf(
        CatalogLoader(),
        R"([{"id":"E1","type":"electronics","name":"Phone","cost":1,"price":500,"mrp":300}])");
    EXPECT_EQ(msg.rfind("product 'E1': ", 0), 0u) << msg;
}

TEST(CatalogLoader, RejectsDuplicateIds) {
    EXPECT_EQ(errorOf(CatalogLoader(), R"([
        {"id":"E1","type":"electronics","name":"A","cost":1,"price":2,"mrp":3},
        {"id":"E1","type":"electronics","name":"B","cost":1,"price":2,"mrp":3}])"),
              "duplicate product id 'E1'");
}

TEST(CatalogLoader, NewTypeIsRegisteredWithoutEditingLoader) {
    CatalogLoader loader;
    EXPECT_FALSE(loader.supports("gadget"));
    loader.registerType("gadget", [](const infrastructure::JsonRecord& r) {
        return std::make_unique<domain::Electronics>(r.id(), r.requireString("name"),
                                                     Money::fromRupees(1), Money::fromRupees(2),
                                                     Money::fromRupees(3));
    });
    EXPECT_TRUE(loader.supports("gadget"));
    const auto products =
        loader.fromJson(json::parse(R"([{"id":"X","type":"gadget","name":"Widget"}])"));
    ASSERT_EQ(products.size(), 1u);
    EXPECT_EQ(products[0]->name(), "Widget");
}

TEST(CatalogLoader, RegisterTypeRejectsBadInput) {
    CatalogLoader loader;
    const CatalogLoader::Creator any = [](const infrastructure::JsonRecord&) {
        return std::unique_ptr<domain::Product>();
    };
    EXPECT_THROW(loader.registerType("", any), std::invalid_argument);
    EXPECT_THROW(loader.registerType("electronics", any), std::invalid_argument);
    EXPECT_THROW(loader.registerType("x", CatalogLoader::Creator()), std::invalid_argument);
}

TEST(CatalogLoader, CreatorReturningNothingIsAnError) {
    CatalogLoader loader;
    loader.registerType("empty", [](const infrastructure::JsonRecord&) {
        return std::unique_ptr<domain::Product>();
    });
    EXPECT_EQ(errorOf(loader, R"([{"id":"Z","type":"empty"}])"),
              "product 'Z': creator for type \"empty\" returned nothing");
}

TEST(CatalogLoader, LoadsProjectCatalogFile) {
    const auto products = CatalogLoader().loadFile(std::string(DATA_DIR) + "/catalog.json");
    ASSERT_EQ(products.size(), 8u);
    EXPECT_EQ(products[0]->id(), "E1");
    EXPECT_EQ(products[0]->category(), "Electronics");
}

TEST(CatalogLoader, MissingFileIsConfigError) {
    EXPECT_THROW(CatalogLoader().loadFile("no/such/catalog.json"), ConfigError);
}
