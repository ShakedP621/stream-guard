#include <gtest/gtest.h>
#include <vector>
#include "streamguard/reorder_buffer.hpp"

using namespace streamguard;

TEST(FrontierMissingK, PromotesAfterKArrivalsBeyondFrontier) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.missing_k = 3;     // need 3 newer arrivals to “give up” on 1
    cfg.capacity  = 128;

    ReorderBuffer rb(cfg);

    // Arrivals beyond the frontier: {2,3,4}. Frontier is 1 and missing.
    rb.push(2);
    rb.push(4);
    rb.push(3);

    // No watchdog – gating doesn’t apply here.
    auto out = rb.try_emit();

    // We should have promoted seq 1 once (skipped it), then emitted 2,3,4.
    std::vector<seq_t> expected{2,3,4};
    EXPECT_EQ(out, expected);

    const auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.emitted, 3u);
    EXPECT_EQ(st.received, 3u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(FrontierMissingK, LateArrivalAfterPromotionCountsAsDropped) {
    ReorderConfig cfg;
    cfg.start_seq = 10;
    cfg.missing_k = 2; // promote after two arrivals beyond 10
    ReorderBuffer rb(cfg);

    // Push two newer ones (11,12) – enough to promote 10.
    rb.push(12);
    rb.push(11);

    // This try emits 11 and 12; also promotes 10 once.
    auto out = rb.try_emit();
    std::vector<seq_t> expected{11, 12};
    EXPECT_EQ(out, expected);

    // Now 10 wanders in late.
    rb.push(10);

    // No new emissions; just ensure the late arrival didn’t sneak in.
    auto out2 = rb.try_emit();
    EXPECT_TRUE(out2.empty());

    const auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.missing_k_dropped,    1u); // late and counted
    EXPECT_EQ(st.emitted,              2u);
    EXPECT_EQ(st.received,             3u);
}
