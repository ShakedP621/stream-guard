#include "streamguard/watchdog.hpp"

#include <chrono>
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using streamguard::Watchdog;

TEST(WatchdogSuite, ConstructsAndReportsTimeout) {
    Watchdog w{100ms};
    EXPECT_EQ(w.timeout(), 100ms);
    EXPECT_FALSE(w.is_running());
}

TEST(WatchdogSuite, StartStopTransitionsAreDeterministic) {
    Watchdog w{50ms};
    // First start should return true (transition false -> true)
    EXPECT_TRUE(w.start());
    EXPECT_TRUE(w.is_running());
    // Second start should return false (already running)
    EXPECT_FALSE(w.start());
    EXPECT_TRUE(w.is_running());
    // Stop should bring it down
    w.stop();
    EXPECT_FALSE(w.is_running());
    // Starting again should work
    EXPECT_TRUE(w.start());
    EXPECT_TRUE(w.is_running());
    w.stop();
}

TEST(WatchdogSuite, PetIsNoopInStub) {
    Watchdog w{10ms};
    EXPECT_TRUE(w.start());
    w.pet(); // should be noexcept and not change state
    EXPECT_TRUE(w.is_running());
    w.stop();
    EXPECT_FALSE(w.is_running());
}
