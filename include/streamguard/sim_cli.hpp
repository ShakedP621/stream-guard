#pragma once
#include "streamguard/sim.hpp"

#include <ostream>
#include <string>

namespace streamguard::cli {

enum class ParseOutcome {
    Ok,
    ShowHelp,
    Error,
};

ParseOutcome parse_sim_args(int argc, char** argv, SimConfig& cfg, bool& want_json, std::string& error);

void print_sim_help(std::ostream& os);

} // namespace streamguard::cli