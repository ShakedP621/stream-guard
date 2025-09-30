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

// We stick with a wide type and define duplicates purely by sequence value.
using seq_t = std::uint64_t;

// Snapshot-style stats: easy to read, copy, and test.
struct ReorderStats {
    // Flow
    std::uint64_t received = 0; // push() calls we accepted/observed
    std::uint64_t emitted = 0;  // total items emitted in order

    // Drops
    std::uint64_t dropped_duplicate = 0; // same seq seen again while pending
    std::uint64_t dropped_too_old = 0;   // seq < next_expected() and not a promoted-late
    std::uint64_t evicted = 0;           // capacity pressure decisions (this step)

    // Frontier accounting (Step 6)
    std::uint64_t missing_k_promotions = 0; // how many times we advanced the frontier
    std::uint64_t missing_k_dropped = 0;    // how many promoted seqs arrived late
};

// User-facing config at construction time.
struct ReorderConfig {
    seq_t start_seq = 1;         // first seq we expect
    std::size_t capacity = 1024; // max items we can buffer in pending_
    std::size_t missing_k = 3;   // frontier promotion threshold
};

// Buffers out-of-order arrivals; emits contiguous runs when allowed.
// Gated by a watchdog (Step 5). Can promote frontier gaps (Step 6).
// Today we add bounded-capacity behavior with farthest-future eviction (Step 8).
class ReorderBuffer {
  public:
    explicit ReorderBuffer(ReorderConfig cfg);

    // Optional watchdog. Emission stays blocked until a live beat is seen once.
    void set_watchdog(std::shared_ptr<Watchdog> wd);

    // Where the in-order stream should resume.
    seq_t next_expected() const;

    // Snapshot of counters.
    ReorderStats stats() const;

    // Parameters (read-only).
    std::size_t capacity() const {
        return cfg_.capacity;
    }
    std::size_t missing_k() const {
        return cfg_.missing_k;
    }

    // Accept a new sequence number following the rules we’ve built so far.
    bool push(seq_t seq);

    // Emit any ready runs in order, starting at next_expected().
    // Watchdog must have opened the gate (Step 5).
    std::vector<seq_t> try_emit();

  private:
    // Helper: count how many buffered items are strictly beyond the frontier.
    std::size_t count_beyond_frontier_unsafe() const;

    // Attempt frontier promotions (Step 6) while under pressure.
    void maintenance_promote_under_pressure_unsafe();

    // Evict the farthest-future element among pending_ ∪ {candidate_new}.
    // Returns true if we should proceed to insert the candidate (i.e., we evicted from pending_),
    // or false if the candidate itself was the farthest and was dropped.
    bool evict_farthest_future_unsafe(seq_t candidate_new);

    ReorderConfig cfg_;
    mutable std::mutex mu_;
    seq_t next_expected_{1};
    ReorderStats stats_{};
    std::shared_ptr<Watchdog> watchdog_; // optional
    bool watchdog_gate_open_ = false;    // sticky once opened

    // What we’ve seen but not emitted yet.
    std::unordered_set<seq_t> pending_;

    // Which specific seq values we explicitly promoted past (frontier “give-ups”).
    // If one of these arrives later, we count it under missing_k_dropped.
    std::unordered_set<seq_t> promoted_missing_;
};

} // namespace streamguard
