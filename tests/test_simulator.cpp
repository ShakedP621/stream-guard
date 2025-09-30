#include "streamguard/sim.hpp"

#include <gtest/gtest.h>

using namespace streamguard;

TEST(Sim, TinyDeterministicRunHasConsistentStats) {
    SimConfig cfg;
    cfg.count = 20;
    cfg.loss_rate = 0.1;
    cfg.dup_rate = 0.2;
    cfg.ooo_rate = 0.3;
    cfg.seed = 42;
    cfg.capacity = 32;
    cfg.missing_k = 2;

    auto res = run_sim(cfg);

    // Basic invariants
    EXPECT_LE(res.stats.emitted, res.stats.received);
    EXPECT_GE(res.generated, res.stats.received);

    // JSON contains stats section
    EXPECT_NE(res.json.find("\"stats\""), std::string::npos);
}

TEST(Sim, HumanSummaryMentionsKeyFields) {
    SimConfig cfg;
    cfg.count = 5;
    cfg.seed = 42;
    auto res = run_sim(cfg);
    auto human = summarize_human(cfg, res);
    EXPECT_NE(human.find("seed 42"), std::string::npos);
    EXPECT_NE(human.find("count 5"), std::string::npos);
}
