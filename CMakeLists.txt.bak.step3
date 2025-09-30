cmake_minimum_required(VERSION 3.20)
project(StreamGuard VERSION 0.1.0 LANGUAGES CXX)
include(CMakePackageConfigHelpers) 
option(STREAMGUARD_BUILD_SHARED "Build streamguard_lib as a shared library" OFF)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Enable tests first
enable_testing()

# GoogleTest via FetchContent
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE    # silence CMP0135 & do the right thing
)
FetchContent_MakeAvailable(googletest)

# Library target
# Select library type explicitly (avoid literal "OFF" becoming a source file)
if(STREAMGUARD_BUILD_SHARED)
    add_library(streamguard_lib SHARED
        src/version.cpp
        src/watchdog.cpp
    )
else()
    add_library(streamguard_lib STATIC
        src/version.cpp
        src/watchdog.cpp
    )
endif()

# --- generated config header ---
set(STREAMGUARD_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${STREAMGUARD_GENERATED_DIR}/streamguard")

configure_file(
    ${CMAKE_SOURCE_DIR}/config/streamguard_config.hpp.in
    ${STREAMGUARD_GENERATED_DIR}/streamguard/config.hpp
    @ONLY
)

# Public include paths: source include/ + generated
target_include_directories(streamguard_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${STREAMGUARD_GENERATED_DIR}>
        $<INSTALL_INTERFACE:include>
)
# Export macro helper defs
target_compile_definitions(streamguard_lib
    PRIVATE STREAMGUARD_BUILDING_LIB=1
    PUBLIC  STREAMGUARD_BUILD_SHARED=$<BOOL:${STREAMGUARD_BUILD_SHARED}>
)
target_include_directories(streamguard_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(streamguard_lib PRIVATE cxx_std_20)

# Tests
add_executable(streamguard_tests
    tests/test_smoke.cpp
    tests/test_version.cpp
    tests/test_watchdog.cpp
)
target_link_libraries(streamguard_tests PRIVATE streamguard_lib GTest::gtest_main)
target_compile_features(streamguard_tests PRIVATE cxx_std_20)

include(GoogleTest)
gtest_discover_tests(streamguard_tests
    DISCOVERY_TIMEOUT 30
)

# --- install/export ---
include(GNUInstallDirs)

# headers
# Generate a ConfigVersion file so consumers can require a version
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/StreamGuardConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
# install generated config header
install(
    FILES "${STREAMGUARD_GENERATED_DIR}/streamguard/config.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/streamguard
)
install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StreamGuardConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/StreamGuardConfigVersion.cmake"   
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StreamGuard
)
install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# library
install(
    TARGETS streamguard_lib
    EXPORT StreamGuardTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# package config files
install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StreamGuardConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StreamGuard
)

# targets export
install(
    EXPORT StreamGuardTargets
    NAMESPACE StreamGuard::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StreamGuard
)
