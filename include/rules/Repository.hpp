#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace rules {

/**
 * @brief Generic in-memory store of objects looked up by their id().
 *
 * OOP ideas: generic programming (one template works for products,
 * customers, rules...), the Single Responsibility Principle (it only stores
 * and finds objects) and RAII ownership through std::unique_ptr, so
 * polymorphic objects such as Product subclasses are freed automatically.
 *
 * Requirement on T: `const std::string& id() const` (or anything
 * comparable to std::string).
 */
template <typename T>
class Repository {
public:
    /// Takes ownership. @throws std::invalid_argument on null or duplicate id.
    void add(std::unique_ptr<T> item) {
        if (!item) throw std::invalid_argument("Repository::add: null item");
        if (findById(item->id()) != nullptr) {
            throw std::invalid_argument("Repository::add: duplicate id " + std::string(item->id()));
        }
        items_.push_back(std::move(item));
    }

    /// @return the item, or nullptr if no item has this id.
    const T* findById(const std::string& id) const {
        for (const auto& item : items_) {
            if (item->id() == id) return item.get();
        }
        return nullptr;
    }

    /// Mutable lookup, for objects with state that changes after loading
    /// (e.g. CouponDiscount::recordUse()). @return nullptr if not found.
    T* findById(const std::string& id) {
        for (auto& item : items_) {
            if (item->id() == id) return item.get();
        }
        return nullptr;
    }

    /// @return every item for which pred(const T&) is true, in insertion order.
    template <typename Predicate>
    std::vector<const T*> filter(Predicate pred) const {
        std::vector<const T*> result;
        for (const auto& item : items_) {
            if (pred(static_cast<const T&>(*item))) result.push_back(item.get());
        }
        return result;
    }

    /// @return every item, in insertion order (non-owning pointers).
    std::vector<const T*> all() const {
        std::vector<const T*> result;
        result.reserve(items_.size());
        for (const auto& item : items_) result.push_back(item.get());
        return result;
    }

    std::size_t size() const noexcept { return items_.size(); }
    bool empty() const noexcept { return items_.empty(); }

private:
    std::vector<std::unique_ptr<T>> items_;
};

} // namespace rules
