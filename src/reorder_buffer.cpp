#include "streamguard/reorder_buffer.hpp"

namespace streamguard {

ReorderBuffer::ReorderBuffer(ReorderConfig cfg)
    : cfg_(cfg), next_expected_{cfg.start_seq} {}

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

bool ReorderBuffer::push(seq_t /*seq*/) {
    std::scoped_lock lk(mu_);
    // Skeleton: just increment 'received' for visibility; no storage yet.
    ++stats_.received;
    return true;
}

std::vector<seq_t> ReorderBuffer::try_emit() {
    // Skeleton: no emissions yet.
    return {};
}

} // namespace streamguard
