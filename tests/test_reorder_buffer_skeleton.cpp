#include <gtest/gtest.h>
#include "streamguard/reorder_buffer.hpp"
#include "streamguard/watchdog.hpp"

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
    ReorderConfig cfg;
    ReorderBuffer rb(cfg);

    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(100));
    rb.set_watchdog(wd);

    // Pushing without behavior should only affect 'received'.
    EXPECT_TRUE(rb.push(1));
    auto st = rb.stats();
    EXPECT_EQ(st.received, 1u);
    EXPECT_EQ(st.emitted, 0u);

    // try_emit() is a no-op for now
    auto out = rb.try_emit();
    EXPECT_TRUE(out.empty());
}
