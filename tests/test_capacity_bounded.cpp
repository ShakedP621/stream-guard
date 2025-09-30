#include "streamguard/reorder_buffer.hpp"

#include <gtest/gtest.h>
#include <vector>

using namespace streamguard;

TEST(CapacityBounded, DropsNewWhenItIsFarthestFuture) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.capacity = 3;  // tiny buffer for the test
    cfg.missing_k = 0; // disable promotions here
    ReorderBuffer rb(cfg);

    // Create a single early gap (1 missing), fill with future items.
    EXPECT_TRUE(rb.push(2));
    EXPECT_TRUE(rb.push(3));
    EXPECT_TRUE(rb.push(4));
    // Buffer is now full: {2,3,4}. Push a very far future item.
    EXPECT_FALSE(rb.push(10)) << "10 is the farthest; should be evicted (dropped)";

    // Nothing emits yet (no watchdog and no frontier item).
    auto out = rb.try_emit();
    EXPECT_TRUE(out.empty());

    const auto st = rb.stats();
    EXPECT_EQ(st.received, 4u);
    EXPECT_EQ(st.evicted, 1u);
    EXPECT_EQ(st.dropped_duplicate, 0u);
    EXPECT_EQ(st.dropped_too_old, 0u);
    EXPECT_EQ(st.missing_k_promotions, 0u);
    EXPECT_EQ(st.missing_k_dropped, 0u);
}

TEST(CapacityBounded, EvictsMaxFromPendingWhenNewIsNotMax) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.capacity = 3;
    cfg.missing_k = 0;
    ReorderBuffer rb(cfg);

    // Fill with far futures.
    EXPECT_TRUE(rb.push(5));
    EXPECT_TRUE(rb.push(7));
    EXPECT_TRUE(rb.push(9));
    // New candidate is 6; global max among {5,7,9,6} is 9, so evict 9 and keep 6.
    EXPECT_TRUE(rb.push(6));

    // We can confirm the shape by trying to emit after introducing the true frontier.
    EXPECT_TRUE(rb.push(1));  // Now frontier item arrives.
    auto out = rb.try_emit(); // emits 1 only (others are >1 and gap after 1)
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 1u);

    const auto st = rb.stats();
    EXPECT_EQ(st.evicted, 1u);
}

TEST(CapacityBounded, PressureTriggersPromotionButStillEvictsIfFull) {
    ReorderConfig cfg;
    cfg.start_seq = 1;
    cfg.capacity = 3;
    cfg.missing_k = 2; // with 2 newer items we can promote one step
    ReorderBuffer rb(cfg);

    EXPECT_TRUE(rb.push(2));
    EXPECT_TRUE(rb.push(3));
    EXPECT_TRUE(rb.push(4)); // full now
    // Pushing 5 causes pressure; we will promote 1 -> 2 (stats++), but still full, so we evict farthest future.
    EXPECT_FALSE(rb.push(5)) << "5 is farthest and should be evicted after promotion";

    const auto st = rb.stats();
    EXPECT_EQ(st.missing_k_promotions, 1u);
    EXPECT_EQ(st.evicted, 1u);
}
