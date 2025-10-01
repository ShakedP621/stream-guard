// Tiny presence guard to prevent accidental removal of Mermaid diagrams.
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>

#ifndef STREAMGUARD_SOURCE_DIR
#error "STREAMGUARD_SOURCE_DIR must be defined for test_docs_presence.cpp"
#endif

static std::string slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

TEST(DocsPresence, ReadmeHasMermaidBlocks) {
    const std::string readme = slurp(std::string(STREAMGUARD_SOURCE_DIR) + "/README.md");
    ASSERT_FALSE(readme.empty());
    EXPECT_NE(readme.find("```mermaid"), std::string::npos);
    EXPECT_NE(readme.find("Design at a glance"), std::string::npos);
}

TEST(DocsPresence, ArchitectureDocHasMermaidBlocks) {
    const std::string arch = slurp(std::string(STREAMGUARD_SOURCE_DIR) + "/docs/architecture.md");
    ASSERT_FALSE(arch.empty());
    EXPECT_NE(arch.find("```mermaid"), std::string::npos);
    EXPECT_NE(arch.find("# StreamGuard Architecture"), std::string::npos);
}