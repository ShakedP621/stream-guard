#include <gtest/gtest.h>
#include <vector>
#include "streamguard/reorder_buffer.hpp"

using namespace streamguard;

TEST(FrontierMissingK, PromotesAfterKArrivalsBeyondFrontier) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.missing_k = 3;

    ReorderBuffer rb(cfg);

    rb.push(2);
    rb.push(4);
    rb.push(3);

    auto out = rb.try_emit();

    std::vector<seq_t> expected{2, 3, 4};
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
    cfg.missing_k = 2;
    ReorderBuffer rb(cfg);

    rb.push(12);
    rb.push(11);

    auto out = rb.try_emit();
    std::vector<seq_t> expected{11, 12};
    EXPECT_EQ(out, expected);

    rb.push(10);

    auto out2 = rb.try_emit();
    EXPECT_TRUE(out2.empty());

    const auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.missing_k_dropped,    1u);
    EXPECT_EQ(st.emitted,              2u);
    EXPECT_EQ(st.received,             3u);
}
