#include "streamguard/reorder_buffer.hpp"
#include <algorithm>

namespace streamguard {

ReorderBuffer::ReorderBuffer(ReorderConfig cfg)
    : cfg_(cfg), next_expected_{cfg.start_seq} {}

void ReorderBuffer::set_watchdog(std::shared_ptr<Watchdog> wd) {
    std::scoped_lock lk(mu_);
    watchdog_ = std::move(wd);
    // Don’t open the gate here; we only open it once we actually observe wd->alive().
    watchdog_gate_open_ = false;
}

seq_t ReorderBuffer::next_expected() const {
    std::scoped_lock lk(mu_);
    return next_expected_;
}

ReorderStats ReorderBuffer::stats() const {
    std::scoped_lock lk(mu_);
    return stats_;
}

std::size_t ReorderBuffer::capacity() const { return cfg_.capacity; }
std::size_t ReorderBuffer::missing_k() const { return cfg_.missing_k; }
CapacityPolicy ReorderBuffer::policy() const { return cfg_.policy; }

bool ReorderBuffer::push(seq_t seq) {
    std::scoped_lock lk(mu_);
    ++stats_.received;

    // 1) Too-old? If so, figure out *which* bucket to count it in.
    if (seq < next_expected_) {
        // If we previously “gave up” on this exact seq via missing_k, it’s a late arrival.
        if (promoted_missing_.erase(seq) > 0) {
            ++stats_.missing_k_dropped;   // late because we promoted earlier
        } else {
            ++stats_.dropped_too_old;     // general too-old case
        }
        return false;
    }

    // 2) Duplicates by seq: if it’s already in our holding tank, drop it.
    auto [it, inserted] = pending_.insert(seq);
    if (!inserted) {
        ++stats_.dropped_duplicate;
        return false;
    }

    // Track how deep the buffer has ever been — just a helpful telemetry nugget.
    if (pending_.size() > stats_.max_depth_observed) {
        stats_.max_depth_observed = static_cast<std::uint64_t>(pending_.size());
    }
    return true;
}

std::size_t ReorderBuffer::count_beyond_frontier_unsafe() const {
    // Caller must hold mu_. We simply count how many buffered seqs are > next_expected_.
    std::size_t cnt = 0;
    for (const auto s : pending_) {
        if (s > next_expected_) ++cnt;
    }
    return cnt;
}

std::vector<seq_t> ReorderBuffer::try_emit() {
    std::scoped_lock lk(mu_);

    // Watchdog gating (Step 5): if present and not yet opened, check it.
    if (watchdog_ && !watchdog_gate_open_) {
        if (watchdog_->alive()) {
            watchdog_gate_open_ = true; // sticky
        } else {
            return {};
        }
    }

    std::vector<seq_t> out;
    for (;;) {
        // 1) If we have exactly the frontier item, emit it (and keep going).
        if (auto it = pending_.find(next_expected_); it != pending_.end()) {
            out.push_back(next_expected_);
            pending_.erase(it);
            ++next_expected_;
            continue;
        }

        // 2) Otherwise, consider promoting the frontier when clearly behind:
        // at least K newer items are already waiting.
        if (cfg_.missing_k > 0 && count_beyond_frontier_unsafe() >= cfg_.missing_k) {
            promoted_missing_.insert(next_expected_);
            ++stats_.missing_k_promotions;
            ++next_expected_;
            // Loop again: maybe we can now emit (or even chain multiple promotions).
            continue;
        }

        // 3) Can’t emit nor promote – we’re stuck until new arrivals show up.
        break;
    }

    stats_.emitted += static_cast<std::uint64_t>(out.size());
    return out;
}

} // namespace streamguard
