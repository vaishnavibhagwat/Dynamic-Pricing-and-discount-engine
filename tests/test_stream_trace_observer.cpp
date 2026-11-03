#include <gtest/gtest.h>

#include <sstream>
#include <type_traits>

#include "engine/PriceTrace.hpp"
#include "presentation/StreamTraceObserver.hpp"

using domain::Money;
using engine::PriceTrace;
using engine::TraceEntry;
using presentation::StreamTraceObserver;

TEST(StreamTraceObserver, IsATraceObserver) {
    EXPECT_TRUE((std::is_base_of_v<engine::TraceObserver, StreamTraceObserver>));
}

TEST(StreamTraceObserver, WritesOneSummaryLinePerDecision) {
    std::ostringstream out;
    StreamTraceObserver log(out, "> ");
    log.onDecision(TraceEntry::rejected("Coupon", "SAVE10", "Save 10", PriceTrace::kOrder,
                                        "no coupon code entered"));
    log.onDecision(TraceEntry::applied("Seasonal", "DIWALI", "Diwali", "E1",
                                       Money::fromRupees(300)));

    EXPECT_EQ(out.str(), "> Coupon SAVE10 rejected: no coupon code entered\n"
                         "> Seasonal DIWALI applied " +
                             Money::fromRupees(300).toString() + " off [E1]\n");
}
