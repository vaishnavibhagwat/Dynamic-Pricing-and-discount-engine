#include "domain/Customer.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace domain {

std::string toString(MembershipTier tier) {
    switch (tier) {
        case MembershipTier::Regular: return "Regular";
        case MembershipTier::Silver: return "Silver";
        case MembershipTier::Gold: return "Gold";
        case MembershipTier::Platinum: return "Platinum";
    }
    return "Unknown";
}

const std::array<MembershipTier, 4>& allMembershipTiers() noexcept {
    static const std::array<MembershipTier, 4> tiers{
        MembershipTier::Regular, MembershipTier::Silver, MembershipTier::Gold,
        MembershipTier::Platinum};
    return tiers;
}

MembershipTier membershipTierFromString(const std::string& text) {
    const auto sameIgnoringCase = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    };
    for (const MembershipTier tier : allMembershipTiers()) {
        if (sameIgnoringCase(toString(tier), text)) return tier;
    }
    throw std::invalid_argument("unknown membership tier \"" + text + "\"");
}

Customer::Customer(std::string id, std::string name, MembershipTier tier, int ordersPlaced)
    : id_(std::move(id)), name_(std::move(name)), tier_(tier), ordersPlaced_(ordersPlaced) {
    if (ordersPlaced_ < 0) throw std::invalid_argument("Customer orders placed must not be negative");
    if (id_.empty()) throw std::invalid_argument("Customer id must not be empty");
    if (name_.empty()) throw std::invalid_argument("Customer name must not be empty");
}

} // namespace domain
