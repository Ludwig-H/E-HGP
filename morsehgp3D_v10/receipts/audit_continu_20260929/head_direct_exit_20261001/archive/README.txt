OPEN RECORDABLE PREPARATION — DIRECT-EXIT CAUSAL DIAGNOSTIC
=========================================================
NO recorder, reader, compiler or probe has been executed for this package.
Root must read all files and explicitly approve launch before record.py.
preparation.json is to be created only after root's complete review as a
SOURCE-ONLY hash inventory, not a closed receipt. It does not exist yet.
manifest.json / capture.json / source_close.json / record_state.json do not
exist either. The original OPEN diagnostic and original
preparation are immutable and are never copied as a newly closed capture.

Purpose and independent analytic expectations
---------------------------------------------
Three nodes: leaves born at squared radius 1; root fusion at level 4.
Two unit points per leaf enter at level 1; a fifth point is attached to
root and enters at level 9. mcs2/z1/EOM/allow_single=true.
Root has no parent, so the later entry is structurally valid.
Root birth lambda=0; child births=1/2; child stabilities=1 each.
Root stability=4*(1/2)+1/3=7/3 > 2. EOM chooses root.
Root membership threshold=max(child births1/2,direct exit1/3)=1/2.
Correct labels=[0,0,0,0,-1]; direct point exit lambda=1/3.

Single semantic mutant, source intact otherwise
----------------------------------------------
mutant/head.cpp changes only the direct-point lambda in condense:
point_rank[x] becomes node_rank[v], exactly one occurrence.
prepare remains byte-identical. Root stability becomes5/2, direct exit1/2,
labels=[0,0,0,0,0]; EOM still chooses root. This is not a crash test.
The pinned same probe judges both: baseline run0, mutant run1, with exactly
five semantic diagnostics (root stability twice, direct lambda twice,
wrong point label once).

Files to review and provenance
------------------------------
snapshot/: seven COMPLETE project dependencies, verbatim.
probe.cpp and mutant/head.cpp are verbatim from the reviewed preparation.
source_pins.json records that original project's hashes before/after.
README.txt, protocol.json, record.py, read.py are new for this package.
The preparatory SHA list supplied to root covers those fourteen payloads.
After review, preparation.json must pin that exact list; its external SHA
is required by record.py before changing preparation state.
No shared source/index/build, cloud/Scene input, FULL/GPU or GCP is touched.

Recorder protocol, AFTER ROOT APPROVAL ONLY
------------------------------------------
Launch with the preparation SHA from root's verified source inventory:
  python3 -B record.py --preparation-sha <EXTERNAL_PREPARATION_SHA>
The not-yet-created preparation.json needs status=source_only_review_pending
and files={relative_path:{sha256:<hex>,bytes:<int>}} for the fourteen BASE
files listed verbatim in record.py. No closed source manifest is prepared
or qualified by this agent; root decides whether to create this inventory.
It refuses reuse: any prior generated file or closed manifest is an error.
An exclusive record_state.json claims the fresh output. A new runtime
directory is created by tempfile outside the archive; binaries stay there.
Each child stage records exact argv/exit/signal/timeout/elapsed/stdout/stderr.
Checkpoints are saved before launch and after each stage; first_failure is
retained on error.
Timeouts/interruption during communicate() kill the PRIVATE child process
group (including GNU compiler children) and collect both streams. A signal
during Popen or between launch and communicate is not covered by that
cleanup block; this package does not qualify signal handling. Failed attempts remain OPEN;
no closed manifest is written and no automatic retry erases the failure.
Native compile timeout10s, run timeout5s; compiler version/backend and
project -MM dependency queries timeout10s. The recorder explicitly sets
LC_ALL=C; its full inherited environment, cwd and Python binary are not
captured as closed components. UTC strings are metadata, not a time authority.
No shell command dispatch and no source writes during record.

There are two compilations and two probe executions, plus two compiler
metadata queries and two GNU project-dependency queries. The compiler
driver and cc1plus bytes, all project source/header dependencies, and
compiled source union are hashed before and after. Each binary is absent
before compilation and its SHA/size is recorded afterwards.
On complete successful recording, manifest.json is written LAST with an
exact eighteen-payload inventory. Raw binaries are NEVER archive payloads.
The recorder does not execute read.py; recording alone is not qualification.

Static independent reading, AFTER A REAL RECORD EXISTS
-----------------------------------------------------
  python3 -B read.py <ARCHIVE> --manifest-sha <EXTERNAL_CLOSED_MANIFEST_SHA>
  python3 -B -O read.py <ARCHIVE> --manifest-sha <EXTERNAL_CLOSED_MANIFEST_SHA>
The reader refuses symlinks/special entries/extra or missing files.
It hashes the closed manifest against the supplied external SHA before
loading it; hashes ALL payload bytes before source/protocol/capture
interpretation. Duplicate JSON keys, bool-as-int in judged numeric values,
nonfinite numbers,
unexpected exits, signals, timeouts, wrong inventories and argv are refused.
It independently parses project deps, checks source/compiler before/after,
verifies the exact single mutation and the reviewed fixture/probe version,
and judges captured values by Fraction. Non-dyadic witness1/3 and7/3 use
16*2^-52*max(1,abs(exact rational)); all dyadic expected values are exact.
Labels, owners, selected IDs and five mutant diagnostics are exact.
All archive hashes are reread afterwards. Reader is STATIC: no native
execution, no imports/exec of recorded source, no original-build access.

Proposed bounded controls (not executed here)
---------------------------------------------
After review, root can record and read normal/-O, then test the wrong
external manifest SHA. Further mutations must use a NEW derived copy:
wrong source bytes without updating payload SHA => integrity refusal;
wrong labels or wrong1/3 in capture, with all derived hashes consistently
updated => semantic-oracle refusal. Preserve the original closed packet.
No original OPEN diagnostic becomes closed by changing a label.

Limits / no extrapolation
-------------------------
The archive attests the archived CODE/CAPTURE relations only. A static
reader cannot prove that a native process was really executed; root replay
is a separate observation and may later provide an independent check.
GNU -MM covers project deps, not the entire standard/system-header closure.
Compiler driver/backend are pinned, but loaded libraries/libm/system
headers are not exhaustively snapshotted. No bit-for-bit reproducibility
claim across machines/libm builds. Fresh-process floating environment and
the reviewed flags are assumed; the fixture does not measure all FENV modes.
The numeric oracle allows stated binary64 rounding; it does not claim
exact real arithmetic. Condensed parent/mass arrays are tested by the
reviewed probe but are not separately emitted as capture vectors.
This isolates one direct-point exit and associated root labelling. It does
not prove general condensation, mcs departures, EOM correctness, ER/MMtA,
FULL completeness, subquadratic LiDAR growth or any G4 timing contract.
An OS/filesystem failure while saving a checkpoint can leave an incomplete
OPEN attempt; the closed inventory reader will refuse it.
Signal/timeout branches are prepared, not exercised by this source-only
package. Repeated signals or a failure while draining/saving may interrupt
failure capture; incomplete attempts are not accepted as closed evidence.
