#include "streamguard/reorder_buffer.hpp"

#include <algorithm>

namespace streamguard {

ReorderBuffer::ReorderBuffer(ReorderConfig cfg) : cfg_(cfg), next_expected_{cfg.start_seq} {}

void ReorderBuffer::set_watchdog(std::shared_ptr<Watchdog> wd) {
    std::scoped_lock lk(mu_);
    watchdog_ = std::move(wd);
    // Don’t auto-open the gate here; we only open it after we actually see wd->alive().
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

    // Too-old filtering is formally added in Step 7; we ignore silently for now.
    if (seq < next_expected_)
        return false;

    const auto [_, inserted] = pending_.insert(seq);
    if (pending_.size() > stats_.max_depth_observed) {
        stats_.max_depth_observed = static_cast<std::uint64_t>(pending_.size());
    }
    return inserted;
}

std::vector<seq_t> ReorderBuffer::try_emit() {
    std::scoped_lock lk(mu_);

    // New: watchdog gating. If a watchdog is present and we haven’t opened
    // the gate yet, check if it’s alive. If not alive, we politely decline to emit.
    if (watchdog_ && !watchdog_gate_open_) {
        if (watchdog_->alive()) {
            watchdog_gate_open_ = true; // sticky — once opened, stays open
        } else {
            return {};
        }
    }

    // Same greedy “flush the contiguous prefix starting at next_expected_”.
    std::vector<seq_t> out;
    for (;;) {
        auto it = pending_.find(next_expected_);
        if (it == pending_.end())
            break;
        out.push_back(next_expected_);
        pending_.erase(it);
        ++next_expected_;
    }

    stats_.emitted += static_cast<std::uint64_t>(out.size());
    return out;
}

} // namespace streamguard
