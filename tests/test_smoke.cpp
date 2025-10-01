#include "streamguard/sim.hpp"

#include <gtest/gtest.h>

TEST(Smoke, RunSimDeterministic) {
    streamguard::SimConfig cfg;
    cfg.count = 4;
    cfg.seed = 42;

    const auto res = streamguard::run_sim(cfg);
    EXPECT_EQ(res.stats.received, res.stats.emitted);
    EXPECT_EQ(res.emitted_last, 4u);
}