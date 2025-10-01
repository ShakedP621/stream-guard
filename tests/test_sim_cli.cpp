#include <vector>
#include "streamguard/sim_cli.hpp"

#include <gtest/gtest.h>

namespace {

std::vector<std::string> make_store(std::initializer_list<const char*> inputs) {
    std::vector<std::string> store;
    store.reserve(inputs.size());
    for (const auto* s : inputs) {
        store.emplace_back(s);
    }
    return store;
}

std::vector<char*> make_argv(std::vector<std::string>& store) {
    std::vector<char*> argv;
    argv.reserve(store.size());
    for (auto& s : store) {
        argv.push_back(s.data());
    }
    return argv;
}

} // namespace

using streamguard::cli::ParseOutcome;

TEST(SimCli, HelpTriggersOutcome) {
    auto store = make_store({"streamguard_sim", "--help"});
    auto argv = make_argv(store);

    streamguard::SimConfig cfg;
    bool want_json = false;
    std::string error;
    const auto result = streamguard::cli::parse_sim_args(static_cast<int>(argv.size()), argv.data(), cfg, want_json, error);

    EXPECT_EQ(result, ParseOutcome::ShowHelp);
    EXPECT_FALSE(want_json);
    EXPECT_TRUE(error.empty());
}

TEST(SimCli, ThreadsMultiParses) {
    auto store = make_store({"streamguard_sim", "--threads", "multi", "--json"});
    auto argv = make_argv(store);

    streamguard::SimConfig cfg;
    bool want_json = false;
    std::string error;
    const auto result = streamguard::cli::parse_sim_args(static_cast<int>(argv.size()), argv.data(), cfg, want_json, error);

    EXPECT_EQ(result, ParseOutcome::Ok);
    EXPECT_EQ(cfg.mode, streamguard::ThreadMode::Multi);
    EXPECT_TRUE(want_json);
    EXPECT_TRUE(error.empty());
}

TEST(SimCli, InvalidThreadsReturnsError) {
    auto store = make_store({"streamguard_sim", "--threads", "bogus"});
    auto argv = make_argv(store);

    streamguard::SimConfig cfg;
    bool want_json = false;
    std::string error;
    const auto result = streamguard::cli::parse_sim_args(static_cast<int>(argv.size()), argv.data(), cfg, want_json, error);

    EXPECT_EQ(result, ParseOutcome::Error);
    EXPECT_FALSE(error.empty());
    EXPECT_FALSE(want_json);
}