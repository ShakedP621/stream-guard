#include "streamguard/reorder_buffer.hpp"

#include <algorithm>

namespace streamguard {

// Basic ctor: remember config and initialize the "what are we waiting for?" cursor.
ReorderBuffer::ReorderBuffer(ReorderConfig cfg) : cfg_(cfg), next_expected_{cfg.start_seq} {}

void ReorderBuffer::set_watchdog(std::shared_ptr<Watchdog> wd) {
    std::scoped_lock lk(mu_);
    watchdog_ = std::move(wd);
}

seq_t ReorderBuffer::next_expected() const {
    std::scoped_lock lk(mu_);
    return next_expected_;
}

ReorderStats ReorderBuffer::stats() const {
    std::scoped_lock lk(mu_);
    return stats_;
}

std::size_t ReorderBuffer::capacity() const {
    return cfg_.capacity;
}
std::size_t ReorderBuffer::missing_k() const {
    return cfg_.missing_k;
}
CapacityPolicy ReorderBuffer::policy() const {
    return cfg_.policy;
}

bool ReorderBuffer::push(seq_t seq) {
    std::scoped_lock lk(mu_);
    ++stats_.received;

    // Step 7 will start counting “too-old”/duplicates.
    // For now: keep anything we haven't emitted yet; silently ignore older ones.
    if (seq < next_expected_) {
        return false; // too old for this step’s rules
    }

    const auto [it, inserted] = pending_.insert(seq);

    // Track how “deep” we ever got — just a curiosity metric for now.
    if (pending_.size() > stats_.max_depth_observed) {
        stats_.max_depth_observed = static_cast<std::uint64_t>(pending_.size());
    }

    return inserted; // true if newly stored; false if it was already there
}

std::vector<seq_t> ReorderBuffer::try_emit() {
    std::scoped_lock lk(mu_);

    // Step 5 will consult watchdog_->alive(). Today we always try to flush.
    std::vector<seq_t> out;

    // Greedy flush: emit the longest contiguous prefix starting at next_expected_.
    for (;;) {
        auto it = pending_.find(next_expected_);
        if (it == pending_.end())
            break;

        out.push_back(next_expected_);
        pending_.erase(it);
        ++next_expected_;
    }

    stats_.emitted += static_cast<std::uint64_t>(out.size());
    return out; // strictly increasing by construction
}

} // namespace streamguard
