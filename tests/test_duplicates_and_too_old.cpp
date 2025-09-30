#include <gtest/gtest.h>
#include "streamguard/reorder_buffer.hpp"

using namespace streamguard;

TEST(Drops, DuplicateBySeqIsDropped) {
    ReorderConfig cfg;
    cfg.start_seq  = 1;
    cfg.missing_k  = 0;

    ReorderBuffer rb(cfg);

    EXPECT_TRUE(rb.push(1));
    EXPECT_FALSE(rb.push(1)) << "second push of same seq should be considered a duplicate";

    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 1u);

    const auto st = rb.stats();
    EXPECT_EQ(st.received,            2u);
    EXPECT_EQ(st.emitted,             1u);
    EXPECT_EQ(st.dropped_duplicate,   1u);
    EXPECT_EQ(st.dropped_too_old,     0u);
    EXPECT_EQ(st.missing_k_promotions,0u);
    EXPECT_EQ(st.missing_k_dropped,   0u);
}

TEST(Drops, TooOldIsDroppedUnlessItWasPromoted) {
    ReorderConfig cfg;
    cfg.start_seq  = 10;
    cfg.missing_k  = 0;
    ReorderBuffer rb(cfg);

    rb.push(10);
    rb.push(11);
    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 10u);
    EXPECT_EQ(out[1], 11u);
    EXPECT_EQ(rb.next_expected(), 12u);

    EXPECT_FALSE(rb.push(10));
    auto st = rb.stats();
    EXPECT_EQ(st.dropped_too_old, 1u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(Drops, PromotedThenArrivedCountsAsMissingKDropNotTooOld) {
    ReorderConfig cfg;
    cfg.start_seq  = 1;
    cfg.missing_k  = 2;
    ReorderBuffer rb(cfg);

    rb.push(2);
    rb.push(3);
    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 2u);
    EXPECT_EQ(out[1], 3u);
    EXPECT_EQ(rb.next_expected(), 4u);

    EXPECT_FALSE(rb.push(1));
    auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.missing_k_dropped,    1u);
    EXPECT_EQ(st.dropped_too_old,      0u);
    EXPECT_EQ(st.dropped_duplicate,    0u);
}
