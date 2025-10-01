StreamGuard

StreamGuard is a small, deterministic C++20 library for reordering out-of-order streams. It ships with a watchdog gate, a bounded reorder buffer, and a simulator CLI for reproducible experiments.

Build
Windows (MSVC)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel

Ubuntu (gcc/clang)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j


GoogleTest v1.14.0 is fetched automatically via FetchContent; no manual setup is required.

Run the simulation (CLI)

The simulator exercises the reorder buffer with a deterministic RNG. All runs are fully reproducible via --seed.

Binaries

Windows: build\Debug\streamguard_sim.exe

Ubuntu: build/streamguard_sim

Quick starts

Windows

# Minimal run (defaults: seed=42, single-thread)
build\Debug\streamguard_sim.exe --count 50 --json

# Deterministic multi-thread + capacity + missing-k + watchdog
build\Debug\streamguard_sim.exe --count 100 --dup-rate 0.1 --loss-rate 0.05 --ooo-rate 0.2 `
  --capacity 16 --missing-k 3 --hb-timeout-ms 1500 --threads multi --seed 1337 --json


Ubuntu

# Minimal run (defaults: seed=42, single-thread)
build/streamguard_sim --count 50 --json

# Deterministic multi-thread + capacity + missing-k + watchdog
build/streamguard_sim --count 100 --dup-rate 0.1 --loss-rate 0.05 --ooo-rate 0.2 \
  --capacity 16 --missing-k 3 --hb-timeout-ms 1500 --threads multi --seed 1337 --json

Useful flags (summary)

--count <N>: how many unique sequence IDs to generate (1..N)

--loss-rate <0..1>: drop probability before push()

--dup-rate <0..1>: probability of generating a duplicate

--ooo-rate <0..1>: adjacent-swap probability for out-of-order

--capacity <int>: reorder-buffer capacity

--missing-k <int>: promote frontier gap after K newer pending

--hb-timeout-ms <ms>: watchdog timeout (sim beats at t=0)

--threads single|multi: deterministic single-thread or SPSC multi-thread

--seed <int>: RNG seed (deterministic)

--verbose: extra human-readable details

--json: emit a JSON summary

--help: print the option list

Example JSON summary keys include: received, emitted, dropped_duplicate, dropped_too_old, missing_k_promotions, missing_k_dropped, evicted.

Run the tests
Windows (MSVC)
# Build first (see above), then:
ctest --test-dir build -C Debug --output-on-failure -j 4


Note: use -C Debug with multi-config generators like Visual Studio.

Ubuntu (gcc/clang)
# Build first (see above), then:
ctest --test-dir build --output-on-failure -j 4


Note: on single-config generators (Makefiles/Ninja) omit -C Debug.

Design diagrams

These render on GitHub via Mermaid — no extra tools needed.

Architecture
flowchart LR
  A[Producer] -->|seq, payload| B[ReorderBuffer]
  B -->|in-order| C[Consumer]
  A --> D[Watchdog beats]
  D --> E{Watchdog Alive?}
  E -- No -->|pause emit| B
  E -- Yes -->|allow emit| B

Emit & promotion flow
flowchart TD
  Start([try_emit])
  GateWD{watchdog set?}
  AliveQ{alive?}
  Scan{has next_expected}
  Emit[emit and advance]
  PromoteQ{promote possible?}
  Bump[promote; advance]
  StopEmpty[return empty]
  StopBatch[return batch]

  Start --> GateWD
  GateWD -->|no| Scan
  GateWD -->|yes| AliveQ
  AliveQ -->|no| StopEmpty
  AliveQ -->|yes| Scan
  Scan -->|found| Emit --> Scan
  Scan -->|not found| PromoteQ
  PromoteQ -->|no| StopBatch
  PromoteQ -->|yes| Bump --> Scan

Capacity
flowchart TD
  Push[push(seq)]
  TooOld{seq < next_expected}
  WasPromoted{was promoted?}
  IsDup{already in pending?}
  Overfull{pending.size > capacity}
  Frontier{seq == next_expected}
  EvictQ{evict farthest}
  End[done]

  Push --> TooOld
  TooOld -->|yes| WasPromoted -->|yes| End
  TooOld -->|no| IsDup -->|yes| End
  IsDup -->|no| Overfull
  Overfull -->|no| Frontier --> End
  Overfull -->|yes| EvictQ --> End

Notes

The watchdog uses an injectable clock for tests; the simulator beats at t=0 and never sleeps.

All randomness comes from std::mt19937 with an explicit seed (default 42).

The multi-thread simulator path uses a single-producer/single-consumer queue with condition variables, yet remains deterministic by preserving arrival order without sleeps.