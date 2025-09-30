#include "streamguard/reorder_buffer.hpp"
#include "streamguard/watchdog.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace std::chrono_literals;
using namespace streamguard;

class FakeClock : public IClock {
  public:
    using steady = std::chrono::steady_clock;
    FakeClock() : now_(steady::time_point(steady::duration(0))) {}
    steady::time_point now() const override {
        return now_;
    }
    void advance(steady::duration d) {
        now_ += d;
    }

  private:
    steady::time_point now_;
};

TEST(WatchdogGating, NoBeatNoEmit) {
    auto clk = std::make_shared<FakeClock>();
    auto wd = std::make_shared<Watchdog>(100ms, clk);

    ReorderConfig cfg;
    cfg.start_seq = 1;
    ReorderBuffer rb(cfg);
    rb.set_watchdog(wd);

    rb.push(1);
    rb.push(2);
    rb.push(3);

    auto out = rb.try_emit();
    EXPECT_TRUE(out.empty());

    auto st = rb.stats();
    EXPECT_EQ(st.received, 3u);
    EXPECT_EQ(st.emitted, 0u);
}

TEST(WatchdogGating, BeatOpensGateThenFlushes) {
    auto clk = std::make_shared<FakeClock>();
    auto wd = std::make_shared<Watchdog>(100ms, clk);

    ReorderConfig cfg;
    cfg.start_seq = 1;
    ReorderBuffer rb(cfg);
    rb.set_watchdog(wd);

    rb.push(2);
    rb.push(3);
    rb.push(1);

    auto out0 = rb.try_emit();
    EXPECT_TRUE(out0.empty());

    wd->beat();
    auto out1 = rb.try_emit();
    std::vector<seq_t> expected{1, 2, 3};
    EXPECT_EQ(out1, expected);

    clk->advance(200ms);
    rb.push(4);
    auto out2 = rb.try_emit();
    ASSERT_EQ(out2.size(), 1u);
    EXPECT_EQ(out2[0], 4u);

    auto st = rb.stats();
    EXPECT_EQ(st.received, 4u);
    EXPECT_EQ(st.emitted, 4u);
}
