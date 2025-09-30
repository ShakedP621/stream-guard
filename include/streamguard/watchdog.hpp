#pragma once
#include <atomic>
#include <chrono>

namespace streamguard {

class Watchdog {
  public:
    using milliseconds = std::chrono::milliseconds;

    explicit Watchdog(milliseconds timeout) noexcept : timeout_{timeout} {}

    // Start the watchdog; returns true if it transitions to running.
    bool start() noexcept {
        bool expected = false;
        return running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel);
    }

    // Feed the watchdog (no-op in stub).
    void pet() noexcept {}

    // Stop the watchdog if running.
    void stop() noexcept {
        running_.store(false, std::memory_order_release);
    }

    // Query state.
    bool is_running() const noexcept {
        return running_.load(std::memory_order_acquire);
    }

    // Configured timeout.
    milliseconds timeout() const noexcept {
        return timeout_;
    }

  private:
    milliseconds timeout_;
    std::atomic<bool> running_{false};
};

} // namespace streamguard
