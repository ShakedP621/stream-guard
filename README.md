# StreamGuard

Minimal C++20 scaffold with GoogleTest and CI.

Build & Test (Windows / MSVC):
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build --config Debug --output-on-failure -j 4

Build & Test (Ubuntu):
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure -j 4

This repo uses:
- CMake ≥ 3.20
- C++20
- GoogleTest via FetchContent
- GitHub Actions (Windows + Ubuntu)
