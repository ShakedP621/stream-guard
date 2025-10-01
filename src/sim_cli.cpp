#include "streamguard/sim_cli.hpp"

#include <cstdlib>
#include <sstream>
#include <string>

namespace streamguard::cli {

namespace {

template <typename Dest, typename ParseFn>
bool assign_value(const std::string& flag, const char* const* argv, int& index, int argc,
                  Dest& out, ParseFn&& parse, std::string& error) {
    if (index + 1 >= argc) {
        std::ostringstream oss;
        oss << "Missing value for " << flag;
        error = oss.str();
        return false;
    }
    const std::string value(argv[++index]);
    try {
        out = parse(value);
    } catch (const std::exception&) {
        std::ostringstream oss;
        oss << "Invalid value for " << flag << ": " << value;
        error = oss.str();
        return false;
    }
    return true;
}

} // namespace

ParseOutcome parse_sim_args(int argc, char** argv, SimConfig& cfg, bool& want_json, std::string& error) {
    want_json = false;
    error.clear();

    for (int i = 1; i < argc; ++i) {
        const std::string flag(argv[i]);

        if (flag == "--count") {
            if (!assign_value(flag, argv, i, argc, cfg.count,
                              [](const std::string& v) { return static_cast<std::uint64_t>(std::stoull(v)); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--loss-rate") {
            if (!assign_value(flag, argv, i, argc, cfg.loss_rate,
                              [](const std::string& v) { return std::stod(v); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--dup-rate") {
            if (!assign_value(flag, argv, i, argc, cfg.dup_rate,
                              [](const std::string& v) { return std::stod(v); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--ooo-rate") {
            if (!assign_value(flag, argv, i, argc, cfg.ooo_rate,
                              [](const std::string& v) { return std::stod(v); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--seed") {
            if (!assign_value(flag, argv, i, argc, cfg.seed,
                              [](const std::string& v) { return static_cast<std::uint64_t>(std::stoull(v)); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--capacity") {
            if (!assign_value(flag, argv, i, argc, cfg.capacity,
                              [](const std::string& v) { return static_cast<std::size_t>(std::stoull(v)); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--missing-k") {
            if (!assign_value(flag, argv, i, argc, cfg.missing_k,
                              [](const std::string& v) { return static_cast<std::size_t>(std::stoull(v)); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--hb-timeout-ms") {
            if (!assign_value(flag, argv, i, argc, cfg.hb_timeout_ms,
                              [](const std::string& v) { return static_cast<std::uint32_t>(std::stoul(v)); }, error))
                return ParseOutcome::Error;
        } else if (flag == "--threads") {
            if (i + 1 >= argc) {
                error = "Missing value for --threads";
                return ParseOutcome::Error;
            }
            const std::string mode(argv[++i]);
            if (mode == "single") {
                cfg.mode = ThreadMode::Single;
            } else if (mode == "multi") {
                cfg.mode = ThreadMode::Multi;
            } else {
                std::ostringstream oss;
                oss << "Unknown threads mode: " << mode;
                error = oss.str();
                return ParseOutcome::Error;
            }
        } else if (flag == "--verbose") {
            cfg.verbose = true;
        } else if (flag == "--json") {
            want_json = true;
        } else if (flag == "--help" || flag == "-h") {
            return ParseOutcome::ShowHelp;
        } else {
            std::ostringstream oss;
            oss << "Unknown arg: " << flag;
            error = oss.str();
            return ParseOutcome::Error;
        }
    }

    return ParseOutcome::Ok;
}

void print_sim_help(std::ostream& os) {
    os << "streamguard_sim options:\n"
       << "  --count <N>\n"
       << "  --loss-rate <0..1>\n"
       << "  --dup-rate <0..1>\n"
       << "  --ooo-rate <0..1>\n"
       << "  --seed <int>\n"
       << "  --capacity <N>\n"
       << "  --missing-k <K>\n"
       << "  --hb-timeout-ms <ms>\n"
       << "  --threads single|multi\n"
       << "  --verbose\n"
       << "  --json\n";
}

} // namespace streamguard::cli