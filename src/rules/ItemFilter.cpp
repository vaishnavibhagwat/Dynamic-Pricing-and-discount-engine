#include "rules/ItemFilter.hpp"

#include <algorithm>

namespace rules {

ItemFilter::ItemFilter(std::optional<std::string> category, std::vector<std::string> productIds)
    : category_(std::move(category)), productIds_(std::move(productIds)) {}

bool ItemFilter::matches(const domain::Product& product) const {
    if (category_ && product.category() != *category_) return false;
    if (!productIds_.empty() &&
        std::find(productIds_.begin(), productIds_.end(), product.id()) == productIds_.end()) {
        return false;
    }
    return true;
}

std::string ItemFilter::describe() const {
    if (!category_ && productIds_.empty()) return "all products";
    std::string text;
    if (category_) text = *category_;
    if (!productIds_.empty()) {
        if (!text.empty()) text += " ";
        text += "products [";
        for (std::size_t i = 0; i < productIds_.size(); ++i) {
            if (i > 0) text += ", ";
            text += productIds_[i];
        }
        text += "]";
    }
    return text;
}

} // namespace rules
