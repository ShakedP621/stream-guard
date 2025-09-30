cmake_minimum_required(VERSION 3.20)
project(StreamGuard LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable tests first
enable_testing()

# GoogleTest via FetchContent
include(FetchContent)

# On MSVC, use shared CRT for GTest to avoid mismatch warnings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Test target (trivial smoke)
add_executable(streamguard_tests
    tests/test_smoke.cpp
)
target_link_libraries(streamguard_tests PRIVATE GTest::gtest_main)
target_compile_features(streamguard_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(streamguard_tests
    DISCOVERY_TIMEOUT 30
)
