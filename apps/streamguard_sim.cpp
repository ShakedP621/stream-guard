#include "streamguard/sim.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

using namespace streamguard;

static bool parse_args(int argc, char** argv, SimConfig& cfg, bool& want_json) {
    want_json = false;
    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        auto read_val = [&](auto& dst) -> bool {
            if (i + 1 >= argc)
                return false;
            std::string v(argv[++i]);
            if constexpr (std::is_same_v<decltype(dst), std::uint64_t&>)
                dst = std::stoull(v);
            else if constexpr (std::is_same_v<decltype(dst), std::size_t&>)
                dst = static_cast<std::size_t>(std::stoull(v));
            else if constexpr (std::is_same_v<decltype(dst), std::uint32_t&>)
                dst = static_cast<std::uint32_t>(std::stoul(v));
            else if constexpr (std::is_same_v<decltype(dst), double&>)
                dst = std::stod(v);
            else if constexpr (std::is_same_v<decltype(dst), bool&>)
                dst = (v == "1" || v == "true");
            return true;
        };

        if (a == "--count") {
            if (!read_val(cfg.count))
                return false;
        } else if (a == "--loss-rate") {
            if (!read_val(cfg.loss_rate))
                return false;
        } else if (a == "--dup-rate") {
            if (!read_val(cfg.dup_rate))
                return false;
        } else if (a == "--ooo-rate") {
            if (!read_val(cfg.ooo_rate))
                return false;
        } else if (a == "--seed") {
            if (!read_val(cfg.seed))
                return false;
        } else if (a == "--capacity") {
            if (!read_val(cfg.capacity))
                return false;
        } else if (a == "--missing-k") {
            if (!read_val(cfg.missing_k))
                return false;
        } else if (a == "--hb-timeout-ms") {
            if (!read_val(cfg.hb_timeout_ms))
                return false;
        } else if (a == "--threads") {
            if (i + 1 >= argc)
                return false;
            std::string v(argv[++i]);
            if (v == "single")
                cfg.mode = ThreadMode::Single;
            else if (v == "multi")
                cfg.mode = ThreadMode::Multi;
            else {
                std::cerr << "Unknown threads mode: " << v << "\n";
                return false;
            }
        } else if (a == "--verbose") {
            cfg.verbose = true;
        } else if (a == "--json") {
            want_json = true;
        } else if (a == "--help" || a == "-h") {
            std::cout << "streamguard_sim options:\n"
                      << "  --count <N>\n  --loss-rate <0..1>\n  --dup-rate <0..1>\n  --ooo-rate <0..1>\n"
                      << "  --seed <int>\n  --capacity <N>\n  --missing-k <K>\n  --hb-timeout-ms <ms>\n"
                      << "  --threads single|multi\n"
                      << "  --verbose\n  --json\n";
            return false;
        } else {
            std::cerr << "Unknown arg: " << a << "\n";
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    SimConfig cfg;
    bool want_json = false;
    if (!parse_args(argc, argv, cfg, want_json)) {
        return 2;
    }
    const auto res = run_sim(cfg);
    if (want_json) {
        std::cout << res.json << "\n";
    } else {
        std::cout << summarize_human(cfg, res) << "\n";
    }
    return 0;
}
