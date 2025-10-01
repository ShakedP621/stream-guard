#include "streamguard/sim_cli.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using streamguard::cli::ParseOutcome;

int main(int argc, char** argv) {
    streamguard::SimConfig cfg;
    bool want_json = false;
    std::string error;

    const auto result = streamguard::cli::parse_sim_args(argc, argv, cfg, want_json, error);
    switch (result) {
    case ParseOutcome::Error:
        std::cerr << error << "\n";
        streamguard::cli::print_sim_help(std::cerr);
        return EXIT_FAILURE;
    case ParseOutcome::ShowHelp:
        streamguard::cli::print_sim_help(std::cout);
        return EXIT_SUCCESS;
    case ParseOutcome::Ok:
        break;
    }

    const auto res = streamguard::run_sim(cfg);
    if (want_json) {
        std::cout << res.json << "\n";
    } else {
        std::cout << streamguard::summarize_human(cfg, res) << "\n";
    }
    return EXIT_SUCCESS;
}