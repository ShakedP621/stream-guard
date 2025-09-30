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

// Nice and wide sequence numbers; duplicates are by seq only.
using seq_t = std::uint64_t;

enum class CapacityPolicy : std::uint8_t {
    Bounded, // Step 8
    SoftCap  // Step 9
};

struct ReorderStats {
    // Inputs/outputs
    std::uint64_t received = 0;
    std::uint64_t emitted = 0;

    // Drops — populated in later steps
    std::uint64_t dropped_duplicate = 0;
    std::uint64_t dropped_too_old = 0;
    std::uint64_t evicted = 0;

    // Frontier bookkeeping (Step 6)
    std::uint64_t missing_k_promotions = 0;
    std::uint64_t missing_k_dropped = 0;

    // Occasional curiosity metric
    std::uint64_t max_depth_observed = 0;
};

struct ReorderConfig {
    seq_t start_seq = 1;
    std::size_t capacity = 1024;
    std::size_t missing_k = 3;
    CapacityPolicy policy = CapacityPolicy::Bounded;
};

// Minimal, friendly reorder buffer.
// Today: stashes out-of-order arrivals and emits the longest in-order run
// starting at next_expected(). Emission is now gated by the watchdog.
class ReorderBuffer {
  public:
    explicit ReorderBuffer(ReorderConfig cfg);

    // Provide a watchdog. We’ll use it to decide if we’re allowed to emit.
    void set_watchdog(std::shared_ptr<Watchdog> wd);

    // The next sequence number we’d love to emit.
    seq_t next_expected() const;

    // Stats snapshot (copy = thread-safe and simple).
    ReorderStats stats() const;

    // Parameters (read-only).
    std::size_t capacity() const;
    std::size_t missing_k() const;
    CapacityPolicy policy() const;

    // Accept a new sequence number. For Step 5, we just stash seq >= next_expected().
    bool push(seq_t seq);

    // Emit all ready-in-order items (if the gate allows it).
    std::vector<seq_t> try_emit();

  private:
    ReorderConfig cfg_;
    mutable std::mutex mu_;
    seq_t next_expected_{1};
    ReorderStats stats_{};

    std::shared_ptr<Watchdog> watchdog_; // optional
    bool watchdog_gate_open_ = false;    // sticky once a live beat is observed

    // Holding tank for out-of-order arrivals.
    std::unordered_set<seq_t> pending_;
};

} // namespace streamguard
