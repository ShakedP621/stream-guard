# Add-VisualAids.ps1
# Usage:
#   From repo root (e.g., C:\dev\stream-guard):
#     powershell -ExecutionPolicy Bypass -File tools\Add-VisualAids.ps1
# This script:
#  - Inserts "Design at a glance" (Mermaid) into README.md (appends if "## Simulator CLI" not found)
#  - Creates docs/architecture.md
#  - Creates tests/test_docs_presence.cpp
#  - ASCII-only content

$ErrorActionPreference = "Stop"

function Ensure-Dir($p) {
  if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p | Out-Null }
}

$repo = Get-Location
Write-Host "Repo: $repo"

# --- 1) Patch README.md ---
$readmePath = Join-Path $repo "README.md"
if (-not (Test-Path $readmePath)) { throw "README.md not found at $readmePath" }

$designBlock = @'
## Design at a glance

> These diagrams render on GitHub via its built-in Mermaid support -- no extra tools needed.

### Architecture (what's in the box)
```mermaid
flowchart LR
  subgraph App["apps/streamguard_sim (CLI)"]
    A1["Flag parsing\n(seed, K, capacity, modes)"]
    A2["Event generator\n(loss/dup/OOO, seed=42)"]
  end

  subgraph Lib["streamguard_lib"]
    WD["Watchdog\n(dead until first beat,\nsticky open)"]
    RB["ReorderBuffer\n(in-order emit,\npromotions, drops,\nfarthest-future eviction)"]
  end

  A1 --> A2 --> RB
  WD --> RB
  RB -->|emits batches| OUT["stdout: human or JSON"]
```

### Watchdog lifecycle (sticky gate)
```mermaid
stateDiagram-v2
  [*] --> Dead: start
  Dead --> Alive: beat()/pet() within timeout\n(alive()==true)
  Alive --> Dead: no beat for > timeout
  Alive --> StickyOpen: first time RB observes alive()==true
  StickyOpen --> StickyOpen: stays open (even if WD times out later)
```

### Emit & promotion flow
```mermaid
flowchart TD
  Start([try_emit()])
  Gate{Watchdog set\nand gate closed?}
  Gate -->|yes & !alive()| Stop[return {}]
  Gate -->|yes & alive()| Open[open gate (sticky)]
  Gate -->|no| Scan

  Open --> Scan
  Scan{pending has\nnext_expected?}
  Scan -->|yes| Emit["emit next_expected;\n++next_expected"] --> Scan
  Scan -->|no| Promote{>= missing_k\nnewer buffered?}
  Promote -->|yes| Bump["record promoted;\n++missing_k_promotions;\n++next_expected"] --> Scan
  Promote -->|no| Stop[return batch]
```

### Capacity (bounded) rule of thumb
```mermaid
flowchart TD
  Push[push(seq)]
  TooOld{seq < next_expected?}
  Promoted{was promoted?}
  Dup{already in pending?}
  Overfull{pending.size > capacity?}
  Evict{who is farthest\nin pending ??? {seq}?}
  Frontier{seq == next_expected?}

  Push --> TooOld
  TooOld -->|yes| Promoted
  Promoted -->|yes| MKDrop["++missing_k_dropped; drop"] --> End
  Promoted -->|no| TooOldDrop["++dropped_too_old; drop"] --> End
  TooOld -->|no| Dup
  Dup -->|yes| DupDrop["++dropped_duplicate; drop"] --> End
  Dup -->|no| Overfull
  Overfull -->|no| End
  Overfull -->|yes| Frontier
  Frontier -->|yes| End["keep seq; try_emit will consume"]
  Frontier -->|no| Evict
  Evict -->|seq is farthest| EvictNew["++evicted; drop new"] --> End
  Evict -->|pending has farthest| EvictOld["++evicted; erase farthest"] --> End
```

### CI overview (on every push)
```mermaid
flowchart LR
  A[Checkout] --> B[Configure (CMake Debug)]
  B --> C[Build: lib + tests + CLI]
  C --> D[ctest (deterministic, fast)]
  D --> E1[CLI smoke: single --json]
  D --> E2[CLI smoke: multi --json]
  E1 --> F[OK]
  E2 --> F
```

### Simulator: single vs multi path
```mermaid
sequenceDiagram
  participant Gen as Generator (seed=42)
  participant Q as SPSC Queue (multi only)
  participant RB as ReorderBuffer
  participant WD as Watchdog

  Note over WD: beat() at t=0 -> sticky gate open
  Gen->>Gen: Make arrivals (loss/dup/OOO)
  alt single-thread
    Gen->>RB: push(seq)
    RB-->>RB: try_emit()
  else multi-thread
    Gen->>Q: enqueue(seq)
    Q->>RB: dequeue(seq) FIFO
    RB-->>RB: try_emit()
  end
  RB->>RB: update stats; emit batches
```

> See also: [docs/architecture.md](docs/architecture.md) for the same diagrams with a tiny bit more detail.
'@

$readme = Get-Content -Raw $readmePath

if ($readme -match '##\s*Design at a glance') {
  Write-Host "README already contains 'Design at a glance' -- skipping insert."
} else {
  if ($readme -match '##\s*Simulator CLI') {
    Write-Host "Inserting design section after '## Simulator CLI'..."
    $updated = $readme -replace '(##\s*Simulator CLI[^\r\n]*[\r\n]+(?:.*?(\r?\n\r?\n|$)))', "`$1`r`n$designBlock`r`n"
    Set-Content -Path $readmePath -Value $updated -NoNewline
  } else {
    Write-Host "'## Simulator CLI' not found -- appending design section to README end."
    Add-Content -Path $readmePath -Value "`r`n`r`n$designBlock"
  }
}

# --- 2) docs/architecture.md ---
Ensure-Dir (Join-Path $repo "docs")
$archPath = Join-Path $repo "docs\architecture.md"
$archContent = @'
# StreamGuard Architecture

These GitHub-native Mermaid diagrams summarize the design. No external tools required.

## High-level
```mermaid
flowchart LR
  subgraph App["apps/streamguard_sim (CLI)"]
    A1["Flag parsing\n(seed, K, capacity, modes)"]
    A2["Event generator\n(loss/dup/OOO, seed=42)"]
  end
  subgraph Lib["streamguard_lib"]
    WD["Watchdog\n(dead until first beat,\nsticky open)"]
    RB["ReorderBuffer\n(in-order emit,\npromotions, drops,\nfarthest-future eviction)"]
  end
  A1 --> A2 --> RB
  WD --> RB
  RB -->|emits batches| OUT["stdout: human or JSON"]
```

## Watchdog lifecycle
```mermaid
stateDiagram-v2
  [*] --> Dead: start
  Dead --> Alive: beat()/pet() within timeout\n(alive()==true)
  Alive --> Dead: no beat for > timeout
  Alive --> StickyOpen: first time RB observes alive()==true
  StickyOpen --> StickyOpen: stays open (even if WD times out later)
```

## Emit & promotion flow
```mermaid
flowchart TD
  Start([try_emit()])
  Gate{Watchdog set\nand gate closed?}
  Gate -->|yes & !alive()| Stop[return {}]
  Gate -->|yes & alive()| Open[open gate (sticky)]
  Gate -->|no| Scan
  Open --> Scan
  Scan{pending has\nnext_expected?}
  Scan -->|yes| Emit["emit next_expected;\n++next_expected"] --> Scan
  Scan -->|no| Promote{>= missing_k\nnewer buffered?}
  Promote -->|yes| Bump["record promoted;\n++missing_k_promotions;\n++next_expected"] --> Scan
  Promote -->|no| Stop[return batch]
```

## Capacity (bounded)
```mermaid
flowchart TD
  Push[push(seq)]
  TooOld{seq < next_expected?}
  Promoted{was promoted?}
  Dup{already in pending?}
  Overfull{pending.size > capacity?}
  Evict{who is farthest\nin pending ??? {seq}?}
  Frontier{seq == next_expected?}
  Push --> TooOld
  TooOld -->|yes| Promoted
  Promoted -->|yes| MKDrop["++missing_k_dropped; drop"] --> End
  Promoted -->|no| TooOldDrop["++dropped_too_old; drop"] --> End
  TooOld -->|no| Dup
  Dup -->|yes| DupDrop["++dropped_duplicate; drop"] --> End
  Dup -->|no| Overfull
  Overfull -->|no| End
  Overfull -->|yes| Frontier
  Frontier -->|yes| End["keep seq; try_emit will consume"]
  Frontier -->|no| Evict
  Evict -->|seq is farthest| EvictNew["++evicted; drop new"] --> End
  Evict -->|pending has farthest| EvictOld["++evicted; erase farthest"] --> End
```

## CI & simulator path
```mermaid
flowchart LR
  A[Checkout] --> B[Configure (CMake Debug)]
  B --> C[Build: lib + tests + CLI]
  C --> D[ctest]
  D --> E1[CLI: single --json]
  D --> E2[CLI: multi --json]
```

## Simulator single vs multi
```mermaid
sequenceDiagram
  participant Gen as Generator (seed=42)
  participant Q as SPSC Queue (multi only)
  participant RB as ReorderBuffer
  participant WD as Watchdog
  Note over WD: beat() at t=0 -> sticky gate open
  Gen->>Gen: Make arrivals (loss/dup/OOO)
  alt single-thread
    Gen->>RB: push(seq); RB-->>RB: try_emit()
  else multi-thread
    Gen->>Q: enqueue(seq); Q->>RB: dequeue(seq) FIFO; RB-->>RB: try_emit()
  end
```
'@
Set-Content -Path $archPath -Value $archContent -NoNewline

# --- 3) tests/test_docs_presence.cpp ---
Ensure-Dir (Join-Path $repo "tests")
$testPath = Join-Path $repo "tests\test_docs_presence.cpp"
$testContent = @'
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
'@

Set-Content -Path $testPath -Value $testContent -NoNewline

Write-Host "Done. Files created/updated:"
Write-Host " - $readmePath (patched)"
Write-Host " - $archPath"
Write-Host " - $testPath"
