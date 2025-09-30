# StreamGuard

A small, test-driven C++20 library and tools for reordering out-of-order streams in a friendly, deterministic way.

- **Watchdog** gates emission and is dead before the first beat.
- **Reorder buffer** emits in-order runs, can promote frontier gaps after `missing_k` newer arrivals, and drops duplicates/too-old with clear counters.
- **Bounded capacity** evicts the farthest-future item under pressure.
- **Simulator CLI** drives reproducible runs with a tiny RNG model.

---

## Build & Test (Windows + Ubuntu)

### Prereqs

- CMake ≥ 3.20  
- A C++20 compiler (MSVC 2022 / clang / gcc)  
- No manual GTest setup needed (fetched via `FetchContent`)

### Windows (MSVC)

```powershell
# From C:\dev\stream-guard
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
# IMPORTANT: use -C (not --config) with ctest
ctest --test-dir build -C Debug --output-on-failure -j 4
Ubuntu (gcc/clang)
bash
Copy code
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
# Single-config generators usually ignore -C, but it's fine to pass it
ctest --test-dir build -C Debug --output-on-failure -j 4
Simulator CLI
We ship a tiny, deterministic simulator to poke the system without writing code.

Quickstart (Windows paths shown; adjust for your OS)
Run a small scenario:

powershell
Copy code
.\build\Debug\streamguard_sim.exe --count 50 --loss-rate 0.1 --dup-rate 0.2 --ooo-rate 0.25 --seed 42
Switch to JSON:

powershell
Copy code
.\build\Debug\streamguard_sim.exe --count 50 --seed 42 --json
Deterministic multi-thread path (SPSC queue):

powershell
Copy code
.\build\Debug\streamguard_sim.exe --count 50 --seed 42 --threads multi
Flags
lua
Copy code
--count <N>                 unique ids (1..N)
--loss-rate <0..1>         drop probability
--dup-rate <0..1>          duplicate probability
--ooo-rate <0..1>          out-of-order probability
--seed <int>               reproducibility
--capacity <N>             reorder buffer capacity
--missing-k <K>            promote a gap after K newer arrivals
--hb-timeout-ms <ms>       heartbeat timeout (sim beats at t=0)
--threads single|multi     run mode
--verbose                  human output
--json                     JSON output
Counters (quick guide)
received / emitted – traffic in/out.

dropped_duplicate – same sequence pushed twice before emit.

dropped_too_old – sequence < frontier (unless it was a promoted gap).

missing_k_promotions – frontier advanced after K newer arrivals.

missing_k_dropped – a previously promoted sequence later arrived.

evicted – bounded capacity kicked something out (farthest-future).

Design edges we care about
Duplicates are by sequence id only.

Watchdog gates emission and is dead before the first beat (the sim opens it at t=0).

Promotions are one step at a time and can chain when there are plenty of newer items.

Under capacity pressure, try promotions; otherwise evict farthest-future.

CI
We build and test on Windows and Ubuntu, and also run a tiny simulator smoke to catch wiring/link issues:

configure → build (Debug) → ctest

streamguard_sim --count 8 --seed 42 --json (first line only)

Troubleshooting
CTest says Unknown argument: --config: use -C Debug (short flag), not --config.

CI can’t find the CLI: ensure the CLI is built before the smoke step and paths match (./build/streamguard_sim on Ubuntu, .\build\Debug\streamguard_sim.exe on Windows).

CTest doesn’t see new tests: re-configure CMake (or delete build/ and regenerate).