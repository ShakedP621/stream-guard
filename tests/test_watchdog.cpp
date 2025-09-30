#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include "streamguard/watchdog.hpp"

using namespace std::chrono_literals;
using streamguard::IClock;
using streamguard::Watchdog;

// A fake clock we can drive manually.
class FakeClock : public IClock {
public:
    using steady = std::chrono::steady_clock;
    FakeClock() : now_(steady::time_point(steady::duration(0))) {}
    steady::time_point now() const override { return now_; }
    void advance(std::chrono::steady_clock::duration d) { now_ += d; }
private:
    steady::time_point now_;
};

TEST(WatchdogSuite, IsDeadBeforeFirstBeat) {
    auto clk = std::make_shared<FakeClock>();
    Watchdog wd(100ms, clk);
    EXPECT_FALSE(wd.alive());
    EXPECT_FALSE(wd.was_ever_triggered());
}

TEST(WatchdogSuite, BecomesAliveAfterBeat) {
    auto clk = std::make_shared<FakeClock>();
    Watchdog wd(100ms, clk);
    wd.beat(); // t=0
    EXPECT_TRUE(wd.alive());
    EXPECT_TRUE(wd.was_ever_triggered()); // sticky flips once alive observed
}

TEST(WatchdogSuite, TimesOutAfterInactivity) {
    auto clk = std::make_shared<FakeClock>();
    Watchdog wd(100ms, clk);
    wd.beat();              // t=0
    clk->advance(50ms);
    EXPECT_TRUE(wd.alive()); // still within timeout
    clk->advance(51ms);      // total 101ms > 100ms
    EXPECT_FALSE(wd.alive()); // timed out
    // Sticky remains true once it was alive at least once
    EXPECT_TRUE(wd.was_ever_triggered());
}

TEST(WatchdogSuite, PetAliasWorks) {
    auto clk = std::make_shared<FakeClock>();
    Watchdog wd(100ms, clk);
    wd.pet();
    EXPECT_TRUE(wd.alive());
}

TEST(WatchdogSuite, ReBeatingRevivesWithinTimeout) {
    auto clk = std::make_shared<FakeClock>();
    Watchdog wd(100ms, clk);
    wd.beat();          // t=0
    clk->advance(120ms); // timeout -> dead
    EXPECT_FALSE(wd.alive());
    wd.beat();           // t=120
    EXPECT_TRUE(wd.alive()); // alive again
    EXPECT_TRUE(wd.was_ever_triggered()); // sticky
}
