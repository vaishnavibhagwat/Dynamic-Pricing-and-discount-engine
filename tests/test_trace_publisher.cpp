#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "engine/PriceTrace.hpp"
#include "engine/TracePublisher.hpp"

using domain::Money;
using engine::PriceTrace;
using engine::TraceEntry;
using engine::TracePublisher;

namespace {

/// Remembers which observer heard what, in one shared log.
class TaggingObserver final : public engine::TraceObserver {
public:
    TaggingObserver(std::string tag, std::vector<std::string>& log)
        : tag_(std::move(tag)), log_(&log) {}
    void onDecision(const TraceEntry& e) override { log_->push_back(tag_ + ":" + e.ruleId); }

private:
    std::string tag_;
    std::vector<std::string>* log_;
};

TraceEntry entry(const std::string& id) {
    return TraceEntry::rejected("Coupon", id, id, PriceTrace::kOrder, "test");
}

} // namespace

TEST(TraceObserver, IsAnAbstractInterface) {
    EXPECT_TRUE((std::is_abstract_v<engine::TraceObserver>));
    EXPECT_TRUE((std::has_virtual_destructor_v<engine::TraceObserver>));
    EXPECT_TRUE((std::is_base_of_v<engine::TraceObserver, PriceTrace>));
}

TEST(TracePublisher, PublishesToEveryObserverInSubscriptionOrder) {
    std::vector<std::string> log;
    TaggingObserver a("a", log);
    TaggingObserver b("b", log);
    TracePublisher pub;
    pub.subscribe(a);
    pub.subscribe(b);
    EXPECT_EQ(pub.observerCount(), 2u);

    pub.publish(entry("X"));
    pub.publish(entry("Y"));
    EXPECT_EQ(log, (std::vector<std::string>{"a:X", "b:X", "a:Y", "b:Y"}));
}

TEST(TracePublisher, PublishingWithNoObserversDoesNothing) {
    const TracePublisher pub;
    EXPECT_EQ(pub.observerCount(), 0u);
    EXPECT_NO_THROW(pub.publish(entry("X")));
}

TEST(TracePublisher, DuplicateSubscribeIsIgnored) {
    std::vector<std::string> log;
    TaggingObserver a("a", log);
    TracePublisher pub;
    pub.subscribe(a);
    pub.subscribe(a);
    EXPECT_EQ(pub.observerCount(), 1u);
    pub.publish(entry("X"));
    EXPECT_EQ(log.size(), 1u);
}

TEST(TracePublisher, UnsubscribeStopsDelivery) {
    std::vector<std::string> log;
    TaggingObserver a("a", log);
    TaggingObserver b("b", log);
    TracePublisher pub;
    pub.subscribe(a);
    pub.subscribe(b);
    pub.unsubscribe(a);
    pub.unsubscribe(a); // not subscribed any more: no effect
    pub.publish(entry("X"));
    EXPECT_EQ(log, (std::vector<std::string>{"b:X"}));
}

TEST(TracePublisher, PriceTraceCollectsAsAnObserver) {
    PriceTrace trace;
    TracePublisher pub;
    pub.subscribe(trace);
    pub.publish(entry("X"));
    pub.publish(TraceEntry::applied("Seasonal", "D", "D", "E1", Money::fromRupees(5)));
    ASSERT_EQ(trace.size(), 2u);
    EXPECT_EQ(trace.entries()[0].ruleId, "X");
    EXPECT_EQ(trace.entries()[1].amount, Money::fromRupees(5));
}
