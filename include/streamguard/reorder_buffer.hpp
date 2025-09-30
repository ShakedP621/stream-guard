#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <mutex>

#include "streamguard/watchdog.hpp"

namespace streamguard {

// Sequence number type (duplicates by seq only per finalized decision).
using seq_t = std::uint64_t;

// Forward-declare policy enums for future steps (Capacity A/B).
enum class CapacityPolicy : std::uint8_t {
    Bounded, // Step 8
    SoftCap  // Step 9
};

// Statistics snapshot for the reorder buffer.
// NOTE: Initial skeleton — fields will be populated/updated in later steps.
struct ReorderStats {
    // Inputs/outputs
    std::uint64_t received = 0;           // total pushes observed
    std::uint64_t emitted = 0;            // total seqs emitted in order

    // Drops
    std::uint64_t dropped_duplicate = 0;  // same seq seen again
    std::uint64_t dropped_too_old = 0;    // seq < next_expected
    std::uint64_t evicted = 0;            // capacity pressure decisions

    // Gap/frontier accounting (for missing_k)
    std::uint64_t missing_k_promotions = 0; // frontier-based promotions
    std::uint64_t missing_k_dropped = 0;    // late arrivals after promotion

    // Useful for assertions/logging later
    std::uint64_t max_depth_observed = 0;
};

// User-facing config at construction time.
struct ReorderConfig {
    seq_t start_seq = 1;             // first expected sequence
    std::size_t capacity = 1024;     // hard capacity (Step 8 fleshes this out)
    std::size_t missing_k = 3;       // frontier gap K (Step 6)
    CapacityPolicy policy = CapacityPolicy::Bounded; // default for now
};

// Skeleton ReorderBuffer: maintains API but no reordering logic yet.
class ReorderBuffer {
public:
    explicit ReorderBuffer(ReorderConfig cfg);

    // Set a watchdog used to gate emission (Step 5 integrates logic).
    void set_watchdog(std::shared_ptr<Watchdog> wd);

    // The next sequence number we aim to emit.
    seq_t next_expected() const;

    // Return a snapshot of current stats.
    ReorderStats stats() const;

    // Capacity and parameters (read-only accessors).
    std::size_t capacity() const;
    std::size_t missing_k() const;
    CapacityPolicy policy() const;

    // Placeholders for upcoming behavior:
    // push(): accept a new sequence (behavior added in Step 4+).
    bool push(seq_t seq);

    // try_emit(): attempt to emit any ready-in-order sequences.
    // For now, returns an empty vector.
    std::vector<seq_t> try_emit();

private:
    ReorderConfig cfg_;
    mutable std::mutex mu_;
    seq_t next_expected_{1};
    ReorderStats stats_{};

    std::shared_ptr<Watchdog> watchdog_; // may be null until set

    // Internal storage will be added in Step 4+ (e.g., containers for holding OOO seqs).
};

} // namespace streamguard
