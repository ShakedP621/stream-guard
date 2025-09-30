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

// Wide sequence IDs; duplicates are by seq only (policy lands in Step 7).
using seq_t = std::uint64_t;

enum class CapacityPolicy : std::uint8_t {
    Bounded, // Step 8
    SoftCap  // Step 9
};

// Stats are a simple “snapshot” object – we return it by value.
struct ReorderStats {
    // Flow
    std::uint64_t received = 0; // push() calls we accepted/observed
    std::uint64_t emitted = 0;  // total items emitted in order

    // Drops (some fields are populated in later steps)
    std::uint64_t dropped_duplicate = 0; // Step 7
    std::uint64_t dropped_too_old = 0;   // Step 7
    std::uint64_t evicted = 0;           // Steps 8–9

    // Frontier accounting (this step)
    std::uint64_t missing_k_promotions = 0; // how many times we advanced the frontier
    std::uint64_t missing_k_dropped = 0;    // how many promoted seqs arrived late

    // Curiosity metric; helps us sanity-check growth.
    std::uint64_t max_depth_observed = 0;
};

struct ReorderConfig {
    seq_t start_seq = 1;         // first seq we expect
    std::size_t capacity = 1024; // capacity policy hooks land later
    std::size_t missing_k = 3;   // enabled by default
    CapacityPolicy policy = CapacityPolicy::Bounded;
};

// Minimal friendly reorder buffer.
// It buffers out-of-order arrivals, optionally gates emission behind a watchdog,
// and now can “promote” the frontier when we have K newer items waiting.
class ReorderBuffer {
  public:
    explicit ReorderBuffer(ReorderConfig cfg);

    // Optional watchdog. Emission remains blocked until a live beat is seen once.
    void set_watchdog(std::shared_ptr<Watchdog> wd);

    // Where the in-order stream should resume.
    seq_t next_expected() const;

    // Snapshot of counters.
    ReorderStats stats() const;

    // Parameters (read-only).
    std::size_t capacity() const;
    std::size_t missing_k() const;
    CapacityPolicy policy() const;

    // Accept a new sequence number.
    // For now we accept seq >= next_expected(). Older arrivals are “late”; if
    // they were previously promoted, we count them as missing_k_dropped.
    bool push(seq_t seq);

    // Emit any ready runs in order, starting at next_expected().
    // Watchdog must have opened the gate (Step 5).
    std::vector<seq_t> try_emit();

  private:
    // Helper: count how many buffered items are strictly beyond the frontier.
    std::size_t count_beyond_frontier_unsafe() const;

    ReorderConfig cfg_;
    mutable std::mutex mu_;
    seq_t next_expected_{1};
    ReorderStats stats_{};
    std::shared_ptr<Watchdog> watchdog_; // optional
    bool watchdog_gate_open_ = false;    // sticky once opened

    // Holding tank for out-of-order arrivals. Good enough for the happy path.
    std::unordered_set<seq_t> pending_;

    // Remember which sequence IDs we explicitly “promoted” past so that if they
    // wander in late, we can count them as such (not just “too old”).
    std::unordered_set<seq_t> promoted_missing_;
};

} // namespace streamguard
