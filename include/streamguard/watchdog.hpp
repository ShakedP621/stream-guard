#pragma once
#include <chrono>
#include <concepts>
#include <memory>
#include <mutex>
#include <optional>

namespace streamguard {

// A minimal clock concept: now() -> time_point
template <typename ClockT>
concept ClockLike = requires {
    typename ClockT::time_point;
    { ClockT::now() } -> std::same_as<typename ClockT::time_point>;
};

// Interface for polymorphic clocks (used in tests without RTTI complications).
struct IClock {
    using steady = std::chrono::steady_clock;
    virtual ~IClock() = 0; // <-- declare only (pure virtual)
    virtual steady::time_point now() const = 0;
};

// Default clock adapter using std::chrono::steady_clock.
class SteadyClock final : public IClock {
  public:
    steady::time_point now() const override {
        return std::chrono::steady_clock::now();
    }
};

// Watchdog semantics:
// - DEAD before first beat().
// - beat() records "last_beat_" and makes watchdog alive.
// - alive() == (seen_first_beat_ && now - last_beat_ <= timeout_).
// - was_ever_triggered() becomes true the first time alive() is observed to be true
//   (sticky, does not reset across subsequent dead/alive transitions).
class Watchdog {
  public:
    using steady = std::chrono::steady_clock;
    using duration = steady::duration;

    explicit Watchdog(duration timeout, std::shared_ptr<IClock> clock = std::make_shared<SteadyClock>())
        : timeout_(timeout), clock_(std::move(clock)) {}

    // Record a heartbeat (a.k.a. pet).
    void beat() {
        std::scoped_lock lk(mu_);
        last_beat_ = clock_->now();
        if (!seen_first_beat_) {
            seen_first_beat_ = true;
        }
    }

    // Backward-compatible alias for beat().
    void pet() {
        beat();
    }

    // Returns true iff we've seen at least one beat and the last beat is within timeout.
    bool alive() const {
        std::scoped_lock lk(mu_);
        if (!seen_first_beat_ || !last_beat_.has_value())
            return false;
        const auto now = clock_->now();
        const bool a = (now - *last_beat_) <= timeout_;
        if (a)
            ever_triggered_ = true; // sticky
        return a;
    }

    // Sticky flag: true once alive() has ever been true since construction.
    bool was_ever_triggered() const {
        std::scoped_lock lk(mu_);
        return ever_triggered_;
    }

    // Expose configured timeout.
    duration timeout() const {
        return timeout_;
    }

  private:
    duration timeout_;
    std::shared_ptr<IClock> clock_;

    // mutable because alive()/was_ever_triggered() are const but update stickiness
    mutable std::mutex mu_;
    mutable std::optional<steady::time_point> last_beat_;
    mutable bool seen_first_beat_ = false;
    mutable bool ever_triggered_ = false;
};

} // namespace streamguard
