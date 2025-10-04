#pragma once
#include "streamguard/reorder_buffer.hpp"
#include "streamguard/watchdog.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace streamguard {

enum class ThreadMode : std::uint8_t { Single, Multi };

struct SimConfig {
    std::uint64_t count = 100;            // how many unique seqs to try generating (1..count)
    double loss_rate = 0.0;               // probability [0..1] to drop a generated seq
    double dup_rate = 0.0;                // probability [0..1] to duplicate a generated seq (by seq id)
    double ooo_rate = 0.0;                // probability [0..1] to shove an item later (simple OOO)
    std::uint64_t seed = 42;              // determinism, always
    std::size_t capacity = 1024;          // bounded capacity from Step 8
    std::size_t missing_k = 3;            // frontier promotions threshold
    std::uint32_t hb_timeout_ms = 100;    // watchdog timeout; we beat at t=0
    bool verbose = false;                 // print extra lines (human summary always prints)
    ThreadMode mode = ThreadMode::Single; // NEW: single | multi
};

struct SimResult {
    ReorderStats stats{};
    std::uint64_t generated = 0;     // how many items we attempted to send (post-loss + dups included)
    std::uint64_t unique_source = 0; // how many unique seq ids we considered (~ count - dropped by loss)
    std::uint64_t emitted_last = 0;  // last emitted seq (0 if none)
    bool watchdog_ever_triggered = false;
    std::string json; // small JSON blob for humans/tools
};

// Run a deterministic simulation and return a compact summary.
// If mode == Multi, uses a simple SPSC queue (mutex+cv) with identical arrival order.
SimResult run_sim(const SimConfig& cfg);

// Format a one-line human summary (friendly).
std::string summarize_human(const SimConfig& cfg, const SimResult& res);

} // namespace streamguard
