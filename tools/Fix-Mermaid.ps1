# Fix-Mermaid.ps1
# Usage (from repo root): 
#   powershell -ExecutionPolicy Bypass -File tools\Fix-Mermaid.ps1
# This script replaces 4 Mermaid diagrams in README.md and docs/architecture.md
# with GitHub-compatible versions (no braces, use <br/>, etc.).

$ErrorActionPreference = "Stop"

function Replace-MermaidAfterHeading {
  param(
    [string]$Path,
    [string[]]$HeadingAlternatives,
    [string]$NewBlock  # must include the ```mermaid fence
  )
  if (-not (Test-Path $Path)) {
    Write-Host "Skip: $Path not found"
    return
  }
  $raw = Get-Content -Raw $Path -Encoding UTF8 -ErrorAction Stop
  $lines = [System.Collections.Generic.List[string]]($raw -split "`n")
  $idxHeading = -1
  for ($i = 0; $i -lt $lines.Count; $i++) {
    foreach ($h in $HeadingAlternatives) {
      if ($lines[$i].Trim() -eq $h) { $idxHeading = $i; break }
    }
    if ($idxHeading -ge 0) { break }
  }
  if ($idxHeading -lt 0) {
    Write-Host "Warn: Heading not found in $Path -> $($HeadingAlternatives -join ' | ')"
    return
  }
  # find first ```mermaid fence after heading
  $start = -1
  for ($j = $idxHeading + 1; $j -lt $lines.Count; $j++) {
    if ($lines[$j].TrimStart().StartsWith('```mermaid')) { $start = $j; break }
    if ($lines[$j].StartsWith('#')) { break } # another heading before block
  }
  if ($start -lt 0) {
    Write-Host "Warn: Mermaid start not found after heading in $Path -> $($HeadingAlternatives -join ' | ')"
    return
  }
  # find closing ```
  $end = -1
  for ($k = $start + 1; $k -lt $lines.Count; $k++) {
    if ($lines[$k].Trim() -eq '```') { $end = $k; break }
  }
  if ($end -lt 0) {
    Write-Host "Warn: Mermaid end fence not found in $Path after line $start"
    return
  }
  # replace block lines [$start..$end] with $NewBlock
  $before = $lines.GetRange(0, $start)
  $after  = $lines.GetRange($end + 1, $lines.Count - ($end + 1))
  $newLines = New-Object System.Collections.Generic.List[string]
  $newLines.AddRange($before)
  foreach ($l in ($NewBlock -split "`n")) { $newLines.Add($l) }
  $newLines.AddRange($after)
  [IO.File]::WriteAllText($Path, ($newLines -join "`n"))
  Write-Host "Patched: $Path -> $($HeadingAlternatives[0])"
}

# --- Corrected blocks (ASCII) ---

$Flow_EmitPromotion = @'
```mermaid
flowchart TD
  Start([try_emit])
  Gate{Watchdog set<br/>and gate closed?}
  Gate -->|yes and not alive()| Stop[return empty]
  Gate -->|yes and alive()| Open[open gate (sticky)]
  Gate -->|no| Scan

  Open --> Scan
  Scan{pending has<br/>next_expected?}
  Scan -->|yes| Emit[emit next_expected;<br/>advance next_expected] --> Scan
  Scan -->|no| Promote{at least missing_k<br/>newer buffered?}
  Promote -->|yes| Bump[record promoted;<br/>advance counters] --> Scan
  Promote -->|no| Stop[return batch]
```
'@

$Flow_Capacity = @'
```mermaid
flowchart TD
  Push[push(seq)]
  TooOld{seq < next_expected?}
  Promoted{was promoted?}
  Dup{already in pending?}
  Overfull{pending.size > capacity?}
  Evict{farthest in pending U {seq}?}
  Frontier{seq == next_expected?}

  Push --> TooOld
  TooOld -->|yes| Promoted
  Promoted -->|yes| MKDrop[inc missing_k_dropped; drop] --> End
  Promoted -->|no| TooOldDrop[inc dropped_too_old; drop] --> End
  TooOld -->|no| Dup
  Dup -->|yes| DupDrop[inc dropped_duplicate; drop] --> End
  Dup -->|no| Overfull
  Overfull -->|no| End
  Overfull -->|yes| Frontier
  Frontier -->|yes| End[keep seq; try_emit will consume]
  Frontier -->|no| Evict
  Evict -->|seq is farthest| EvictNew[inc evicted; drop new] --> End
  Evict -->|pending has farthest| EvictOld[inc evicted; drop farthest pending] --> End
```
'@

$Flow_CI = @'
```mermaid
flowchart LR
  A[Checkout] --> B[Configure (CMake Debug)]
  B --> C[Build: lib + tests + CLI]
  C --> D[ctest]
  D --> E1[CLI: single --json]
  D --> E2[CLI: multi --json]
  E1 --> F[done]
  E2 --> F
```
'@

$Seq_SingleMulti = @'
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
'@

# --- Patch README.md ---
$readme = "README.md"
Replace-MermaidAfterHeading -Path $readme -HeadingAlternatives @('### Emit & promotion flow') -NewBlock $Flow_EmitPromotion
Replace-MermaidAfterHeading -Path $readme -HeadingAlternatives @('### Capacity (bounded) rule of thumb') -NewBlock $Flow_Capacity
Replace-MermaidAfterHeading -Path $readme -HeadingAlternatives @('### CI overview (on every push)') -NewBlock $Flow_CI
Replace-MermaidAfterHeading -Path $readme -HeadingAlternatives @('### Simulator: single vs multi path') -NewBlock $Seq_SingleMulti

# --- Patch docs/architecture.md ---
$arch = "docs/architecture.md"
Replace-MermaidAfterHeading -Path $arch -HeadingAlternatives @('## Emit & promotion flow') -NewBlock $Flow_EmitPromotion
Replace-MermaidAfterHeading -Path $arch -HeadingAlternatives @('## Capacity (bounded)') -NewBlock $Flow_Capacity
Replace-MermaidAfterHeading -Path $arch -HeadingAlternatives @('## CI & simulator path') -NewBlock $Flow_CI
Replace-MermaidAfterHeading -Path $arch -HeadingAlternatives @('## Simulator single vs multi') -NewBlock $Seq_SingleMulti

Write-Host "Done."
