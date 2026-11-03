#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "domain/Customer.hpp"
#include "domain/Electronics.hpp"
#include "domain/Furniture.hpp"
#include "domain/Grocery.hpp"
#include "rules/Repository.hpp"

using namespace domain;
using rules::Repository;

namespace {
class ProductRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo.add(std::make_unique<Electronics>("E1", "Laptop", Money::fromRupees(40000),
                                               Money::fromRupees(55000), Money::fromRupees(60000)));
        repo.add(std::make_unique<Grocery>("G1", "Atta", Money::fromRupees(200),
                                           Money::fromRupees(280), Money::fromRupees(300),
                                           Date(2027, 1, 1)));
        repo.add(std::make_unique<Electronics>("E2", "Mouse", Money::fromRupees(300),
                                               Money::fromRupees(499), Money::fromRupees(699)));
        repo.add(std::make_unique<Furniture>("F1", "Sofa", Money::fromRupees(15000),
                                             Money::fromRupees(22000), Money::fromRupees(25000),
                                             true));
    }

    Repository<Product> repo;
};
} // namespace

TEST_F(ProductRepositoryTest, AddAndSize) {
    EXPECT_EQ(repo.size(), 4u);
    EXPECT_FALSE(repo.empty());
}

TEST_F(ProductRepositoryTest, FindByIdReturnsPolymorphicObject) {
    const Product* p = repo.findById("G1");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name(), "Atta");
    EXPECT_EQ(p->gstRate(), 5);
    EXPECT_NE(dynamic_cast<const Grocery*>(p), nullptr);
}

TEST_F(ProductRepositoryTest, FindByIdMissingReturnsNull) {
    EXPECT_EQ(repo.findById("NOPE"), nullptr);
}

TEST_F(ProductRepositoryTest, RejectsDuplicateAndNull) {
    EXPECT_THROW(repo.add(std::make_unique<Electronics>("E1", "Other", Money::fromRupees(1),
                                                        Money::fromRupees(2), Money::fromRupees(3))),
                 std::invalid_argument);
    EXPECT_THROW(repo.add(nullptr), std::invalid_argument);
    EXPECT_EQ(repo.size(), 4u);
}

TEST_F(ProductRepositoryTest, FilterByCategoryKeepsInsertionOrder) {
    const auto electronics =
        repo.filter([](const Product& p) { return p.category() == "Electronics"; });
    ASSERT_EQ(electronics.size(), 2u);
    EXPECT_EQ(electronics[0]->id(), "E1");
    EXPECT_EQ(electronics[1]->id(), "E2");
}

TEST_F(ProductRepositoryTest, FilterByPrice) {
    const Money limit = Money::fromRupees(1000);
    const auto cheap = repo.filter([&](const Product& p) { return p.sellingPrice() < limit; });
    ASSERT_EQ(cheap.size(), 2u);
    EXPECT_EQ(cheap[0]->id(), "G1");
    EXPECT_EQ(cheap[1]->id(), "E2");
    EXPECT_TRUE(repo.filter([](const Product&) { return false; }).empty());
}

TEST_F(ProductRepositoryTest, AllReturnsEveryItemInInsertionOrder) {
    const auto all = repo.all();
    ASSERT_EQ(all.size(), 4u);
    EXPECT_EQ(all[0]->id(), "E1");
    EXPECT_EQ(all[1]->id(), "G1");
    EXPECT_EQ(all[3]->id(), "F1");
    EXPECT_TRUE(Repository<Product>().all().empty());
}

TEST(Repository, WorksForOtherTypes) {
    Repository<Customer> customers;
    EXPECT_TRUE(customers.empty());
    customers.add(std::make_unique<Customer>("CU1", "Asha", MembershipTier::Gold));
    customers.add(std::make_unique<Customer>("CU2", "Ravi"));

    const Customer* asha = customers.findById("CU1");
    ASSERT_NE(asha, nullptr);
    EXPECT_EQ(asha->tier(), MembershipTier::Gold);

    const auto gold = customers.filter(
        [](const Customer& c) { return c.tier() == MembershipTier::Gold; });
    ASSERT_EQ(gold.size(), 1u);
    EXPECT_EQ(gold[0]->name(), "Asha");
}

TEST(RepositoryMutable, NonConstFindByIdAllowsChangingState) {
    struct Counter {
        std::string id_;
        int uses = 0;
        const std::string& id() const { return id_; }
    };
    Repository<Counter> repo;
    repo.add(std::make_unique<Counter>(Counter{"A", 0}));
    Counter* c = repo.findById("A");
    ASSERT_NE(c, nullptr);
    ++c->uses;
    EXPECT_EQ(static_cast<const Repository<Counter>&>(repo).findById("A")->uses, 1);
    EXPECT_EQ(repo.findById("B"), nullptr);
}
