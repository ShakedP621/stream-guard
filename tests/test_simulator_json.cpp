#include "streamguard/sim.hpp"

#include <gtest/gtest.h>
#include <string>

using namespace streamguard;

// A tiny "did we print the expected JSON keys?" check.
// We don't pull a JSON lib; we just assert presence of fields.
TEST(SimJson, IncludesModeAndCoreCounters) {
    SimConfig cfg;
    cfg.count = 10;
    cfg.seed = 42;
    cfg.mode = ThreadMode::Multi; // exercise the mode key path
    cfg.capacity = 32;
    cfg.missing_k = 2;

    auto res = run_sim(cfg);

    const std::string& j = res.json;
    // must contain mode
    EXPECT_NE(j.find("\"mode\":\"multi\""), std::string::npos);
    // and a couple of representative counters
    EXPECT_NE(j.find("\"received\":"), std::string::npos);
    EXPECT_NE(j.find("\"emitted\":"), std::string::npos);
    EXPECT_NE(j.find("\"dropped_duplicate\":"), std::string::npos);
    EXPECT_NE(j.find("\"dropped_too_old\":"), std::string::npos);
    EXPECT_NE(j.find("\"evicted\":"), std::string::npos);
    EXPECT_NE(res.json.find("\"watchdog_ever_triggered\":true"), std::string::npos);
}
