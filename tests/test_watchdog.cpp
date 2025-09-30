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
    EXPECT_TRUE(w.start());
    EXPECT_TRUE(w.is_running());
    EXPECT_FALSE(w.start()); // already running
    EXPECT_TRUE(w.is_running());
    w.stop();
    EXPECT_FALSE(w.is_running());
    EXPECT_TRUE(w.start());
    EXPECT_TRUE(w.is_running());
    w.stop();
}

TEST(WatchdogSuite, PetIsNoopInStub) {
    Watchdog w{10ms};
    EXPECT_TRUE(w.start());
    w.pet(); // no-op
    EXPECT_TRUE(w.is_running());
    w.stop();
    EXPECT_FALSE(w.is_running());
}
