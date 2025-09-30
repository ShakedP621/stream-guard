#include "streamguard/reorder_buffer.hpp"
#include "streamguard/watchdog.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>

using namespace streamguard;

TEST(ReorderBufferSkeleton, DefaultsAreSane) {
    ReorderConfig cfg;
    ReorderBuffer rb(cfg);

    EXPECT_EQ(rb.next_expected(), cfg.start_seq);
    EXPECT_EQ(rb.missing_k(), cfg.missing_k);

    const auto st = rb.stats();
    EXPECT_EQ(st.received, 0u);
    EXPECT_EQ(st.emitted, 0u);
    EXPECT_EQ(st.dropped_duplicate, 0u);
    EXPECT_EQ(st.dropped_too_old, 0u);
    EXPECT_EQ(st.missing_k_promotions, 0u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(ReorderBufferSkeleton, CustomStartSeqIsHonored) {
    ReorderConfig cfg;
    cfg.start_seq = 42;
    cfg.missing_k = 5;
    ReorderBuffer rb(cfg);

    EXPECT_EQ(rb.next_expected(), 42u);
    EXPECT_EQ(rb.missing_k(), 5u);
}

TEST(ReorderBufferSkeleton, SetWatchdogDoesNotAffectStats) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    ReorderBuffer rb(cfg);

    auto st0 = rb.stats();
    EXPECT_EQ(st0.received, 0u);
    EXPECT_EQ(st0.emitted, 0u);

    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(100));
    rb.set_watchdog(wd);

    rb.push(1);

    auto out0 = rb.try_emit();
    EXPECT_TRUE(out0.empty());

    auto st1 = rb.stats();
    EXPECT_EQ(st1.received, 1u);
    EXPECT_EQ(st1.emitted, 0u);

    wd->beat();
    auto out1 = rb.try_emit();
    ASSERT_EQ(out1.size(), 1u);
    EXPECT_EQ(out1[0], 1u);

    auto st2 = rb.stats();
    EXPECT_EQ(st2.received, 1u);
    EXPECT_EQ(st2.emitted, 1u);
}
