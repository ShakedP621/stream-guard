#include "streamguard/version.hpp"

#include <gtest/gtest.h>
#include <regex>
#include <string>

using namespace streamguard;

TEST(VersionSuite, ReturnsSemver) {
    const std::string v = version();
    std::regex semver(R"(^\d+\.\d+\.\d+$)");
    EXPECT_TRUE(std::regex_match(v, semver)) << "Version not semver: " << v;
}

TEST(VersionSuite, ComponentsConsistent) {
    const std::string v = version();
    const std::string expected =
        std::to_string(kVersionMajor) + "." + std::to_string(kVersionMinor) + "." + std::to_string(kVersionPatch);
    EXPECT_EQ(v, expected);
}

TEST(VersionSuite, UmbrellaHeaderProvidesAPI) {
// Intentionally include the umbrella header
#include "streamguard/streamguard.hpp"
    // Ensure we can call version() without needing version.hpp directly
    EXPECT_FALSE(streamguard::version().empty());
}
