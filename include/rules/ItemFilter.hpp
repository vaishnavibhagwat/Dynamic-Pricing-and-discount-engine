#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "domain/Product.hpp"

namespace rules {

/**
 * @brief Decides which products an item-level offer covers.
 *
 * OOP ideas: Single Responsibility (only "does this product match?") and
 * composition - item rules have an ItemFilter instead of each one
 * re-implementing category / product-id matching.
 *
 * An empty filter matches every product. If both a category and product
 * ids are given, a product must satisfy both.
 */
class ItemFilter {
public:
    ItemFilter() = default;
    explicit ItemFilter(std::optional<std::string> category,
                        std::vector<std::string> productIds = {});

    static ItemFilter all() { return ItemFilter(); }
    static ItemFilter forCategory(std::string category) { return ItemFilter(std::move(category)); }

    bool matches(const domain::Product& product) const;

    const std::optional<std::string>& category() const noexcept { return category_; }
    const std::vector<std::string>& productIds() const noexcept { return productIds_; }
    /// Human-readable scope, e.g. "Electronics" or "all products".
    std::string describe() const;

private:
    std::optional<std::string> category_;
    std::vector<std::string> productIds_;
};

} // namespace rules
