#include <gtest/gtest.h>
#include "streamguard/reorder_buffer.hpp"

using namespace streamguard;

TEST(Drops, DuplicateBySeqIsDropped) {
    ReorderConfig cfg;
    cfg.start_seq  = 1;
    cfg.missing_k  = 0;     // keep promotions out of the picture here
    cfg.capacity   = 64;

    ReorderBuffer rb(cfg);

    // Push the same sequence twice before we ever emit it.
    EXPECT_TRUE(rb.push(1));
    EXPECT_FALSE(rb.push(1)) << "second push of same seq should be considered a duplicate";

    // Now emit: watchdog not set, so emission is allowed.
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
    cfg.missing_k  = 0;     // no promotions in this subtest
    ReorderBuffer rb(cfg);

    // Move the frontier forward by emitting 10 and 11.
    rb.push(10);
    rb.push(11);
    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 10u);
    EXPECT_EQ(out[1], 11u);
    EXPECT_EQ(rb.next_expected(), 12u);

    // Now 10 returns again — it's too-old (and was not promoted), so count dropped_too_old.
    EXPECT_FALSE(rb.push(10));
    auto st = rb.stats();
    EXPECT_EQ(st.dropped_too_old, 1u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(Drops, PromotedThenArrivedCountsAsMissingKDropNotTooOld) {
    ReorderConfig cfg;
    cfg.start_seq  = 1;
    cfg.missing_k  = 2;   // promote frontier after two newer are waiting
    ReorderBuffer rb(cfg);

    // Put two newer ones beyond the frontier (2,3), forcing a promotion of 1 in try_emit().
    rb.push(2);
    rb.push(3);
    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 2u);
    EXPECT_EQ(out[1], 3u);
    EXPECT_EQ(rb.next_expected(), 4u);

    // Now 1 wanders in late: this is *not* "too-old" — we promoted it earlier.
    EXPECT_FALSE(rb.push(1));
    auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.missing_k_dropped,    1u);
    EXPECT_EQ(st.dropped_too_old,      0u);
    EXPECT_EQ(st.dropped_duplicate,    0u);
}
