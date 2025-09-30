#pragma once
#include "streamguard/watchdog.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace streamguard {

// We keep sequence numbers wide; duplicates are by seq only (later step tightens behavior).
using seq_t = std::uint64_t;

enum class CapacityPolicy : std::uint8_t {
    Bounded, // Step 8
    SoftCap  // Step 9
};

struct ReorderStats {
    // Inputs/outputs
    std::uint64_t received = 0; // total pushes observed
    std::uint64_t emitted = 0;  // total seqs emitted in order

    // Drops (later steps)
    std::uint64_t dropped_duplicate = 0; // Step 7
    std::uint64_t dropped_too_old = 0;   // Step 7
    std::uint64_t evicted = 0;           // Step 8/9

    // Frontier (Step 6)
    std::uint64_t missing_k_promotions = 0;
    std::uint64_t missing_k_dropped = 0;

    // Handy for later assertions/telemetry
    std::uint64_t max_depth_observed = 0;
};

struct ReorderConfig {
    seq_t start_seq = 1;         // first expected sequence
    std::size_t capacity = 1024; // hard capacity (Step 8 fleshes this out)
    std::size_t missing_k = 3;   // frontier gap K (Step 6)
    CapacityPolicy policy = CapacityPolicy::Bounded;
};

// Minimal, friendly reorder buffer. Today it can stash future packets and
// flush the contiguous run starting at next_expected() when asked.
class ReorderBuffer {
  public:
    explicit ReorderBuffer(ReorderConfig cfg);

    // Inject a watchdog (we'll actually use it in Step 5).
    void set_watchdog(std::shared_ptr<Watchdog> wd);

    // The next sequence number we’re aiming to emit.
    seq_t next_expected() const;

    // Snapshot of stats (by value; thread-safe and easy to log).
    ReorderStats stats() const;

    // Parameters (read-only).
    std::size_t capacity() const;
    std::size_t missing_k() const;
    CapacityPolicy policy() const;

    // Accept a new sequence number. For now, we just record it.
    // Returns true if stored/accepted; false if ignored (later steps will use this).
    bool push(seq_t seq);

    // Try to emit any ready-in-order sequences, starting at next_expected().
    // Returns the batch we just emitted, strictly increasing.
    std::vector<seq_t> try_emit();

  private:
    ReorderConfig cfg_;
    mutable std::mutex mu_;
    seq_t next_expected_{1};
    ReorderStats stats_{};

    std::shared_ptr<Watchdog> watchdog_; // may be null until set

    // Holding area for out-of-order arrivals (Step 4).
    std::unordered_set<seq_t> pending_;
};

} // namespace streamguard
