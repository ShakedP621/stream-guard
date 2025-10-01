cmake_minimum_required(VERSION 3.20)
project(min_consume LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Require the installed package at version 0.1.0 EXACT
find_package(StreamGuard 0.1.0 EXACT CONFIG REQUIRED)

add_executable(min_consume main.cpp)
target_link_libraries(min_consume PRIVATE StreamGuard::streamguard_lib)
