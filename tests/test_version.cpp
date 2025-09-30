#include "streamguard/config.hpp"
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
TEST(VersionSuite, MacrosMatchVersionFunction) {
    const std::string from_fn = streamguard::version();
    const std::string from_macros = STREAMGUARD_VERSION_STRING;
    EXPECT_EQ(from_fn, from_macros);

    // Also check components match kVersion* constants
    const std::string expected = std::to_string(streamguard::kVersionMajor) + "." +
                                 std::to_string(streamguard::kVersionMinor) + "." +
                                 std::to_string(streamguard::kVersionPatch);
    EXPECT_EQ(from_fn, expected);

    // Basic sanity of component macros
    EXPECT_EQ(STREAMGUARD_VERSION_MAJOR, streamguard::kVersionMajor);
    EXPECT_EQ(STREAMGUARD_VERSION_MINOR, streamguard::kVersionMinor);
    EXPECT_EQ(STREAMGUARD_VERSION_PATCH, streamguard::kVersionPatch);
}
