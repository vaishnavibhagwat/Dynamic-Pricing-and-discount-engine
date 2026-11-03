#pragma once

#include <array>
#include <string>

namespace domain {

enum class MembershipTier { Regular, Silver, Gold, Platinum };

std::string toString(MembershipTier tier);

/// Every tier, lowest first. The single list loaders, parsers and the what-if
/// simulator loop over, so a new tier is added in one place.
const std::array<MembershipTier, 4>& allMembershipTiers() noexcept;

/// Parses "Gold", "GOLD", "gold"... @throws std::invalid_argument on an unknown name.
MembershipTier membershipTierFromString(const std::string& text);

/**
 * @brief A shopper, identified by id, with a membership tier and order count.
 *
 * OOP ideas: encapsulation (private data, read-only accessors) and the
 * Single Responsibility Principle - it only describes who is buying, not
 * how prices are calculated.
 */
class Customer {
public:
    /// @throws std::invalid_argument if id or name is empty or ordersPlaced is negative.
    Customer(std::string id, std::string name, MembershipTier tier = MembershipTier::Regular,
             int ordersPlaced = 0);

    const std::string& id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    MembershipTier tier() const noexcept { return tier_; }
    /// Orders this customer completed before the current one.
    int ordersPlaced() const noexcept { return ordersPlaced_; }
    bool isFirstOrder() const noexcept { return ordersPlaced_ == 0; }
    /// Counts one completed order (called after a successful checkout).
    void recordOrder() noexcept { ++ordersPlaced_; }

private:
    std::string id_;
    std::string name_;
    MembershipTier tier_;
    int ordersPlaced_;
};

} // namespace domain
