#include "streamguard/reorder_buffer.hpp"
#include "streamguard/watchdog.hpp"

#include <gtest/gtest.h>

using namespace streamguard;

TEST(ReorderBufferSkeleton, DefaultsAreSane) {
    ReorderConfig cfg; // defaults: start=1, capacity=1024, missing_k=3
    ReorderBuffer rb(cfg);

    EXPECT_EQ(rb.next_expected(), 1u);
    EXPECT_EQ(rb.capacity(), 1024u);
    EXPECT_EQ(rb.missing_k(), 3u);
    EXPECT_EQ(static_cast<int>(rb.policy()), static_cast<int>(CapacityPolicy::Bounded));

    const auto st = rb.stats();
    EXPECT_EQ(st.received, 0u);
    EXPECT_EQ(st.emitted, 0u);
    EXPECT_EQ(st.dropped_duplicate, 0u);
    EXPECT_EQ(st.dropped_too_old, 0u);
    EXPECT_EQ(st.evicted, 0u);
    EXPECT_EQ(st.missing_k_promotions, 0u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(ReorderBufferSkeleton, CustomStartSeqIsHonored) {
    ReorderConfig cfg;
    cfg.start_seq = 42;
    cfg.capacity = 64;
    cfg.missing_k = 5;
    ReorderBuffer rb(cfg);

    EXPECT_EQ(rb.next_expected(), 42u);
    EXPECT_EQ(rb.capacity(), 64u);
    EXPECT_EQ(rb.missing_k(), 5u);
}

TEST(ReorderBufferSkeleton, SetWatchdogDoesNotAffectStats) {
    using namespace streamguard;

    ReorderConfig cfg;
    cfg.start_seq = 1;
    ReorderBuffer rb(cfg);

    // Inject a watchdog; in Step 4 it doesn't gate anything yet.
    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(10));
    rb.set_watchdog(wd);

    // Pushing the next expected seq should be emitted regardless of watchdog.
    EXPECT_EQ(rb.next_expected(), 1u);
    EXPECT_TRUE(rb.push(1));

    // Emission should occur now (Step 4 behavior).
    auto out = rb.try_emit();
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 1u);

    // Stats reflect exactly one received and one emitted, and no drops.
    const auto st = rb.stats();
    EXPECT_EQ(st.received, 1u);
    EXPECT_EQ(st.emitted, 1u);
    EXPECT_EQ(st.dropped_duplicate, 0u);
    EXPECT_EQ(st.dropped_too_old, 0u);
}
