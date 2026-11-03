#include "rules/RuleStage.hpp"

namespace rules {

bool isItemStage(RuleStage stage) noexcept {
    return stage != RuleStage::Coupon && stage != RuleStage::BankOffer;
}

std::string toString(RuleStage stage) {
    switch (stage) {
    case RuleStage::Expiry: return "Expiry";
    case RuleStage::Seasonal: return "Seasonal";
    case RuleStage::Percentage: return "Percentage";
    case RuleStage::Quantity: return "Quantity";
    case RuleStage::Membership: return "Membership";
    case RuleStage::Coupon: return "Coupon";
    case RuleStage::BankOffer: return "BankOffer";
    }
    return "Unknown";
}

} // namespace rules
