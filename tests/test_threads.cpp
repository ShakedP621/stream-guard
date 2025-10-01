#include <gtest/gtest.h>
#include "streamguard/sim.hpp"

using namespace streamguard;

// This tiny parity test ensures multi-thread mode preserves exact stats.
TEST(Threads, SingleAndMultiProduceSameStatsOnTinyRun) {
    SimConfig base;
    base.count = 64;
    base.loss_rate = 0.15;
    base.dup_rate  = 0.10;
    base.ooo_rate  = 0.25;
    base.seed = 42;
    base.capacity = 64;
    base.missing_k = 2;

    auto s = base; s.mode = ThreadMode::Single;
    auto m = base; m.mode = ThreadMode::Multi;

    auto rs = run_sim(s);
    auto rm = run_sim(m);

    EXPECT_EQ(rs.stats.received, rm.stats.received);
    EXPECT_EQ(rs.stats.emitted,  rm.stats.emitted);
    EXPECT_EQ(rs.stats.dropped_duplicate, rm.stats.dropped_duplicate);
    EXPECT_EQ(rs.stats.dropped_too_old,   rm.stats.dropped_too_old);
    EXPECT_EQ(rs.stats.evicted,           rm.stats.evicted);
    EXPECT_EQ(rs.stats.missing_k_promotions, rm.stats.missing_k_promotions);
    EXPECT_EQ(rs.stats.missing_k_dropped,    rm.stats.missing_k_dropped);
    EXPECT_EQ(rs.generated, rm.generated);
    EXPECT_EQ(rs.unique_source, rm.unique_source);
    EXPECT_EQ(rs.emitted_last, rm.emitted_last);
}
