// Deterministic "arrive in any order, emit in order" test for Step 4.
// We push 1..N in a reproducible shuffled order and verify try_emit()
// always returns a clean, strictly increasing run overall.

#include "streamguard/reorder_buffer.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <vector>

using namespace streamguard;

TEST(ReorderEmitNoGaps, ShuffledArrivalsEmitInOrder) {
    constexpr std::size_t N = 100;

    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.missing_k = 0; // disable promotions

    ReorderBuffer rb(cfg);

    std::vector<seq_t> arrivals;
    arrivals.reserve(N);
    for (seq_t s = 1; s <= N; ++s) {
        arrivals.push_back(s);
    }

    std::mt19937 rng(42);
    std::shuffle(arrivals.begin(), arrivals.end(), rng);

    std::vector<seq_t> all_emitted;
    all_emitted.reserve(N);

    for (auto s : arrivals) {
        rb.push(s);
        auto batch = rb.try_emit();
        all_emitted.insert(all_emitted.end(), batch.begin(), batch.end());
    }

    auto tail = rb.try_emit();
    all_emitted.insert(all_emitted.end(), tail.begin(), tail.end());

    ASSERT_EQ(all_emitted.size(), N);
    for (seq_t i = 0; i < N; ++i) {
        EXPECT_EQ(all_emitted[i], i + 1) << "Out of order at index " << i;
    }

    const auto st = rb.stats();
    EXPECT_EQ(st.received, N);
    EXPECT_EQ(st.emitted, N);
    EXPECT_EQ(st.dropped_duplicate, 0u);
    EXPECT_EQ(st.dropped_too_old, 0u);
}
