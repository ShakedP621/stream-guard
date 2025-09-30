// Deterministic “arrive in any order, emit in order” test for Step 4.
// We push 1..N in a reproducible shuffled order and verify try_emit()
// always returns a clean, strictly increasing run overall.

#include "streamguard/reorder_buffer.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <vector>

using namespace streamguard;

TEST(ReorderEmitNoGaps, ShuffledArrivalsEmitInOrder) {
    // Push 1..N in random order, draining after each push.
    constexpr std::size_t N = 100;

    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.capacity = 4096; // plenty of room for this test
    ReorderBuffer rb(cfg);

    std::vector<seq_t> arrivals;
    arrivals.reserve(N);
    for (seq_t s = 1; s <= N; ++s) {
        arrivals.push_back(s);
    }

    // Deterministic shuffle (seeded) so failures are reproducible.
    std::mt19937 rng(42);
    std::shuffle(arrivals.begin(), arrivals.end(), rng);

    std::vector<seq_t> all_emitted;
    all_emitted.reserve(N);

    for (auto s : arrivals) {
        rb.push(s);
        auto batch = rb.try_emit();
        all_emitted.insert(all_emitted.end(), batch.begin(), batch.end());
    }

    // Final drain (should be empty if everything already flushed).
    auto tail = rb.try_emit();
    all_emitted.insert(all_emitted.end(), tail.begin(), tail.end());

    // Expect exactly 1..N in order.
    ASSERT_EQ(all_emitted.size(), N);
    for (seq_t i = 0; i < N; ++i) {
        EXPECT_EQ(all_emitted[i], i + 1) << "Out of order at index " << i;
    }

    // Sanity-check stats match the story.
    const auto st = rb.stats();
    EXPECT_EQ(st.received, N);
    EXPECT_EQ(st.emitted, N);
    EXPECT_EQ(st.dropped_duplicate, 0u); // Step 7 will start counting these
    EXPECT_EQ(st.dropped_too_old, 0u);   // Step 7 will start counting these
}
