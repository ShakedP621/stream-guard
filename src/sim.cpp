#include "streamguard/sim.hpp"

#include <algorithm>
#include <condition_variable>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_set>

namespace streamguard {

// --- tiny helper to emit JSON with mode included
static std::string to_json(const SimConfig& c, const ReorderStats& s, std::uint64_t generated,
                           std::uint64_t unique_source, std::uint64_t emitted_last) {
    std::ostringstream os;
    os << "{";
    os << "\"seed\":" << c.seed << ",\"count\":" << c.count << ",\"capacity\":" << c.capacity
       << ",\"missing_k\":" << c.missing_k << ",\"loss_rate\":" << std::fixed << std::setprecision(3) << c.loss_rate
       << ",\"dup_rate\":" << std::fixed << std::setprecision(3) << c.dup_rate << ",\"ooo_rate\":" << std::fixed
       << std::setprecision(3) << c.ooo_rate << ",\"mode\":\"" << (c.mode == ThreadMode::Single ? "single" : "multi")
       << "\""
       << ",\"generated\":" << generated << ",\"unique_source\":" << unique_source << ",\"stats\":{"
       << "\"received\":" << s.received << ",\"emitted\":" << s.emitted
       << ",\"dropped_duplicate\":" << s.dropped_duplicate << ",\"dropped_too_old\":" << s.dropped_too_old
       << ",\"evicted\":" << s.evicted << ",\"missing_k_promotions\":" << s.missing_k_promotions
       << ",\"missing_k_dropped\":" << s.missing_k_dropped << "}"
       << ",\"emitted_last\":" << emitted_last << "}";
    return os.str();
}

// Build the deterministic arrival list once from the seeded RNG.
// We apply loss/dup/OOO here so both modes consume the exact same values.
static std::vector<seq_t> build_arrivals(const SimConfig& cfg, std::mt19937& rng, std::uint64_t& unique_out) {
    std::bernoulli_distribution drop(cfg.loss_rate);
    std::bernoulli_distribution dup(cfg.dup_rate);
    std::bernoulli_distribution ooo(cfg.ooo_rate);

    std::vector<seq_t> seqs;
    seqs.reserve(static_cast<size_t>(cfg.count));
    for (seq_t s = 1; s <= cfg.count; ++s) {
        if (drop(rng))
            continue; // omitted entirely
        seqs.push_back(s);
        if (dup(rng))
            seqs.push_back(s); // duplicate by seq id
    }

    // Derive unique count from what actually appears after loss/dup.
    unique_out = static_cast<std::uint64_t>(std::unordered_set<seq_t>(seqs.begin(), seqs.end()).size());

    // Simple OOO: pairwise swaps under a coin flip; stable shape, deterministic.
    if (!seqs.empty()) {
        for (size_t i = 0; i + 1 < seqs.size(); ++i) {
            if (ooo(rng))
                std::swap(seqs[i], seqs[i + 1]);
        }
    }
    return seqs;
}

SimResult run_sim(const SimConfig& cfg) {
    std::mt19937 rng(static_cast<std::mt19937::result_type>(cfg.seed));

    // Wire up the reorder buffer + watchdog (gate is sticky once beaten).
    ReorderConfig rc;
    rc.start_seq = 1;
    rc.capacity = cfg.capacity;
    rc.missing_k = cfg.missing_k;

    ReorderBuffer rb(rc);
    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(cfg.hb_timeout_ms));
    rb.set_watchdog(wd);
    wd->beat(); // open the gate at t=0

    std::uint64_t unique_source = 0;
    auto arrivals = build_arrivals(cfg, rng, unique_source);

    std::uint64_t emitted_last = 0;
    std::uint64_t generated = 0;

    if (cfg.mode == ThreadMode::Single) {
        // Single-thread: push & drain inline, order = arrivals order.
        for (auto s : arrivals) {
            ++generated;
            rb.push(s);
            auto out = rb.try_emit();
            if (!out.empty())
                emitted_last = static_cast<std::uint64_t>(out.back());
        }
        // final drain
        auto tail = rb.try_emit();
        if (!tail.empty())
            emitted_last = static_cast<std::uint64_t>(tail.back());
    } else {
        // Multi-thread: deterministic SPSC with no sleeps; preserves FIFO.
        std::mutex qmu;
        std::condition_variable qcv;
        std::queue<seq_t> q;
        bool done = false;

        // Producer pushes in the exact 'arrivals' order.
        std::thread prod([&] {
            for (auto s : arrivals) {
                {
                    std::lock_guard<std::mutex> lk(qmu);
                    q.push(s);
                }
                qcv.notify_one();
            }
            {
                std::lock_guard<std::mutex> lk(qmu);
                done = true;
            }
            qcv.notify_one();
        });

        // Consumer pops FIFO and processes like single-thread.
        std::thread cons([&] {
            for (;;) {
                seq_t v{};
                {
                    std::unique_lock<std::mutex> lk(qmu);
                    qcv.wait(lk, [&] { return !q.empty() || done; });
                    if (q.empty()) {
                        if (done)
                            break;
                        continue;
                    }
                    v = q.front();
                    q.pop();
                }
                ++generated;
                rb.push(v);
                auto out = rb.try_emit();
                if (!out.empty())
                    emitted_last = static_cast<std::uint64_t>(out.back());
            }
            // final drain
            auto tail = rb.try_emit();
            if (!tail.empty())
                emitted_last = static_cast<std::uint64_t>(tail.back());
        });

        prod.join();
        cons.join();
    }

    const auto st = rb.stats();
    SimResult res;
    res.stats = st;
    res.generated = generated;
    res.unique_source = unique_source;
    res.emitted_last = emitted_last;
    res.json = to_json(cfg, st, generated, unique_source, emitted_last);
    return res;
}

std::string summarize_human(const SimConfig& c, const SimResult& r) {
    std::ostringstream os;
    os << "StreamGuard sim - seed " << c.seed << " | mode " << (c.mode == ThreadMode::Single ? "single" : "multi")
       << " | count " << c.count << " | cap " << c.capacity << " | K " << c.missing_k << " | loss " << c.loss_rate
       << " | dup " << c.dup_rate << " | ooo " << c.ooo_rate << "\n"
       << "generated=" << r.generated << " unique_src=" << r.unique_source << " received=" << r.stats.received
       << " emitted=" << r.stats.emitted << " dups=" << r.stats.dropped_duplicate
       << " too_old=" << r.stats.dropped_too_old << " evicted=" << r.stats.evicted
       << " K_promoted=" << r.stats.missing_k_promotions << " K_late=" << r.stats.missing_k_dropped
       << " last=" << r.emitted_last;
    return os.str();
}

} // namespace streamguard
