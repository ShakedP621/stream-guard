cmake_minimum_required(VERSION 3.20)
project(min_consume LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Expect -DStreamGuard_DIR=".../_install/lib/cmake/StreamGuard"
find_package(StreamGuard CONFIG REQUIRED)

add_executable(min_consume main.cpp)
# Use the namespaced installed target:
target_link_libraries(min_consume PRIVATE StreamGuard::streamguard_lib)
