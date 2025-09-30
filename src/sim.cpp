#include "streamguard/sim.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace streamguard {

static std::string to_json(const SimConfig& c, const ReorderStats& s, std::uint64_t generated,
                           std::uint64_t unique_source, std::uint64_t emitted_last) {
    std::ostringstream os;
    os << "{";
    os << "\"seed\":" << c.seed << ",\"count\":" << c.count << ",\"capacity\":" << c.capacity
       << ",\"missing_k\":" << c.missing_k << ",\"loss_rate\":" << std::fixed << std::setprecision(3) << c.loss_rate
       << ",\"dup_rate\":" << std::fixed << std::setprecision(3) << c.dup_rate << ",\"ooo_rate\":" << std::fixed
       << std::setprecision(3) << c.ooo_rate << ",\"generated\":" << generated << ",\"unique_source\":" << unique_source
       << ",\"stats\":{"
       << "\"received\":" << s.received << ",\"emitted\":" << s.emitted
       << ",\"dropped_duplicate\":" << s.dropped_duplicate << ",\"dropped_too_old\":" << s.dropped_too_old
       << ",\"evicted\":" << s.evicted << ",\"missing_k_promotions\":" << s.missing_k_promotions
       << ",\"missing_k_dropped\":" << s.missing_k_dropped << "}"
       << ",\"emitted_last\":" << emitted_last << "}";
    return os.str();
}

SimResult run_sim(const SimConfig& cfg) {
    // PRNG: everything derives from this seed so we can reproduce runs exactly.
    std::mt19937 rng(static_cast<std::mt19937::result_type>(cfg.seed));
    std::bernoulli_distribution drop(cfg.loss_rate);
    std::bernoulli_distribution dup(cfg.dup_rate);
    std::bernoulli_distribution ooo(cfg.ooo_rate);

    // Wire up the reorder buffer + watchdog.
    ReorderConfig rc;
    rc.start_seq = 1;
    rc.capacity = cfg.capacity;
    rc.missing_k = cfg.missing_k;

    ReorderBuffer rb(rc);
    auto wd = std::make_shared<Watchdog>(std::chrono::milliseconds(cfg.hb_timeout_ms));
    rb.set_watchdog(wd);

    // Heartbeat at t=0 to open the gate; we’ll simulate a later quiet gap, but gating is sticky.
    wd->beat();

    // Build a working list of seq ids 1..count, then apply probabilistic loss/dup.
    std::vector<seq_t> seqs;
    seqs.reserve(static_cast<size_t>(cfg.count));
    for (seq_t s = 1; s <= cfg.count; ++s) {
        if (drop(rng))
            continue; // omitted entirely
        seqs.push_back(s);
        if (dup(rng))
            seqs.push_back(s); // duplicate by seq id
    }

    const std::uint64_t unique_source =
        static_cast<std::uint64_t>(std::unordered_set<seq_t>(seqs.begin(), seqs.end()).size());

    // Optionally inject simple out-of-order behavior by swapping neighbors.
    if (!seqs.empty()) {
        for (size_t i = 0; i + 1 < seqs.size(); ++i) {
            if (ooo(rng))
                std::swap(seqs[i], seqs[i + 1]);
        }
    }

    // Drive the pipeline: push then opportunistically drain.
    std::uint64_t emitted_last = 0;
    std::uint64_t generated = 0;

    for (auto s : seqs) {
        ++generated;
        rb.push(s);
        auto out = rb.try_emit();
        if (!out.empty())
            emitted_last = static_cast<std::uint64_t>(out.back());
        if (cfg.verbose && !out.empty()) {
            // breadcrumb left intentionally quiet
            (void)0;
        }
    }

    // Final drain, just in case.
    auto tail = rb.try_emit();
    if (!tail.empty())
        emitted_last = static_cast<std::uint64_t>(tail.back());

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
    os << "StreamGuard sim — seed " << c.seed << " | count " << c.count << " | cap " << c.capacity << " | K "
       << c.missing_k << " | loss " << c.loss_rate << " | dup " << c.dup_rate << " | ooo " << c.ooo_rate << "\n"
       << "generated=" << r.generated << " unique_src=" << r.unique_source << " received=" << r.stats.received
       << " emitted=" << r.stats.emitted << " dups=" << r.stats.dropped_duplicate
       << " too_old=" << r.stats.dropped_too_old << " evicted=" << r.stats.evicted
       << " K_promoted=" << r.stats.missing_k_promotions << " K_late=" << r.stats.missing_k_dropped
       << " last=" << r.emitted_last;
    return os.str();
}

} // namespace streamguard
