#pragma once

#include <string>
#include <utility>

#include "domain/Product.hpp"

namespace domain {

enum class Size { XS, S, M, L, XL, XXL };
enum class Season { Summer, Winter, Monsoon, AllSeason };

std::string toString(Size size);
std::string toString(Season season);

/**
 * @brief A clothing product (12% GST) with a size and a season.
 *
 * OOP idea: inheritance with extension; strongly typed enums keep size and
 * season values valid (type safety instead of free-form strings).
 */
class Clothing final : public Product {
public:
    Clothing(std::string id, std::string name, Money costPrice, Money sellingPrice, Money mrp,
             Size size, Season season)
        : Product(std::move(id), std::move(name), costPrice, sellingPrice, mrp),
          size_(size),
          season_(season) {}

    int gstRate() const override { return 12; }
    std::string category() const override { return "Clothing"; }

    Size size() const noexcept { return size_; }
    Season season() const noexcept { return season_; }

private:
    Size size_;
    Season season_;
};

} // namespace domain
