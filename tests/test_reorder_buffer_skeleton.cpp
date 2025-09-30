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

    // Setting a watchdog should not change stats by itself...
    auto st0 = rb.stats();
    EXPECT_EQ(st0.received, 0u);
    EXPECT_EQ(st0.emitted, 0u);

    // ...but Step 5 gates emission until the first beat().
    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(100));
    rb.set_watchdog(wd);

    rb.push(1);

    // Gate is closed: no emit yet.
    auto out0 = rb.try_emit();
    EXPECT_TRUE(out0.empty());

    // Stats: received incremented by push(), emitted still zero.
    auto st1 = rb.stats();
    EXPECT_EQ(st1.received, 1u);
    EXPECT_EQ(st1.emitted, 0u);

    // First beat opens the sticky gate; now emit should flush 1.
    wd->beat();
    auto out1 = rb.try_emit();
    ASSERT_EQ(out1.size(), 1u);
    EXPECT_EQ(out1[0], 1u);

    auto st2 = rb.stats();
    EXPECT_EQ(st2.received, 1u);
    EXPECT_EQ(st2.emitted, 1u);
}
