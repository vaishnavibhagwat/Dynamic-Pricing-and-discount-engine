#include "domain/Clothing.hpp"

namespace domain {

std::string toString(Size size) {
    switch (size) {
        case Size::XS: return "XS";
        case Size::S: return "S";
        case Size::M: return "M";
        case Size::L: return "L";
        case Size::XL: return "XL";
        case Size::XXL: return "XXL";
    }
    return "Unknown";
}

std::string toString(Season season) {
    switch (season) {
        case Season::Summer: return "Summer";
        case Season::Winter: return "Winter";
        case Season::Monsoon: return "Monsoon";
        case Season::AllSeason: return "AllSeason";
    }
    return "Unknown";
}

} // namespace domain
