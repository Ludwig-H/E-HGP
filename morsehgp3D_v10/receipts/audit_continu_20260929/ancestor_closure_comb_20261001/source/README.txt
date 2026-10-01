PRIVATE R2 PREPARATION — EXACT 1D ANCESTOR CLOSURE, K5/K10
Created 2026-10-01. NO record/probe of THIS R2 package has been executed.
Main review is required before recording. R1 f59cd502... is immutable and unchanged.
No shared source, native engine, GCP or benchmark data is changed.

GEOMETRY AND FORMULAS
Sites x_j=(j(j+1)/2,0,0). Gaps are strictly increasing.
A consecutive K-window i carries [x_(i+K-1)-r, x_i+r], born at
r=(x_(i+K-1)-x_i)/2. Every other K-part's interval is contained in
a consecutive K-window's interval. Adjacent windows meet at
r=(x_(i+K)-x_i)/2, strictly increasing: their FULL_K forest is a comb.
H=2(n-K+1)-1. Each K-window leaf covers its K sites. Merge i covers
the prefix 0..i+K at its birth. D=K(n-K+1)+(n-K)(n+K+1)/2.
These facts determine the INDEPENDENT analytic signatures in read.py.

CATALOGUE IS NOT THE STRONG SEED STREAM
The 1D catalogue adapter contains q2 balls of populations 2..K+1.
Their shell has2 endpoints, q_min=2, with pop-2 strict interiors.
K+1-windows are cofaces, p=K-1; they are present in the catalogue.
The ORIGINAL witness_universe(...,strong=True) requires population>=K
and p+q_min<=K, selecting K-windows only: W=K(n-K+1).
Weak incidences include K+1-windows and are counted separately.

WHAT IS REPLAYED
Full exact source snapshots are original/fullk.py and original/frontier_core.py.
They are SHA-checked before AST compilation/execution; there are NO complete
module imports or executions of their external dependencies.
The original Tree/native_tree/check_cover/gamma_tree/witness_universe bodies
are extracted. Forest/export is an ANALYTIC 1D adapter, NOT a native exporter.
MEB for Gamma is exact 1D (max_x-min_x)^2/4. DSU is a standalone ordinary DSU.
Only3 exhaustive Gamma cases: n8/K5(56vertices,28edges),n16/K5(4368,8008),
n16/K10(8008,4368). n32/K5 and n32/K10 run closure AST/adapters only.
No exhaustive Gamma32 is permitted.
The reader independently derives every forest/coverage digest from the
closed-form comb, NOT from AST sources or the export adapter. Node IDs do
not enter signatures. Compact outputs contain counts/digests, not all parts.

R2 HARDENING AND AUTHORITY
origin.json gives source/predecessor provenance. The recorder observes its
canonical absolute source root and every file's absolute path/SHA; the
Python executable's resolved absolute path, SHA, version/version_info.
The child probe reports that same execution identity in its output.
argv/cwd must exactly equal the commands derived from those observations,
not merely have matching basenames. No unknown/empty case selection exists.
True integers are required (including Gamma counts), bool is not integer.
Duplicate JSON keys, NaN/Infinity/overflowed nonfinite floats are rejected.
UTC strings must be valid Z timestamps; durations/timeout30/timed_out are
explicit. Source/capture roots, all parents and payloads reject symlinks.
All inventories/hashes are checked BEFORE JSON/import and rechecked at end.
The capture reader is LIVE with respect to the observed Python interpreter
SHA/version, but NEVER runs the probe or modifies any archive.
Copied source packages remain readable: recorded absolute source origin is
checked internally against source manifest pins, not equated to a new copy's root.

AFTER MAIN REVIEW ONLY
Pin the SHA256SUMS hash EXTERNALLY, as supplied with this package.
Read-only source check:
  python3 -B read.py --source-manifest-sha SOURCE_PIN
  python3 -B -O read.py --source-manifest-sha SOURCE_PIN
Create a NEW absolute output path outside the source package:
  python3 -B record.py --source-manifest-sha SOURCE_PIN --out /tmp/FRESH_PATH
Each of2 child calls is limited to30s (shared CPU load). All stdout/stderr/codes/argv/UTC are
preserved before post-run checks, including failures. No cleanup is implicit.
Pin the new capture's SHA256SUMS hash externally, then read without replay:
  python3 -B read.py --source-manifest-sha SOURCE_PIN --capture /tmp/FRESH_PATH --capture-manifest-sha CAPTURE_PIN
Repeat reader under -O. Normal/-O probe payloads must be byte-identical.
The source inventory has9 TEXT files including its manifest. A complete
capture has4 TEXT files:normal.json,optimized.json,run_receipt.json,SHA256SUMS.
A partial/failing capture cannot qualify.

SCOPE
This is a realizable comb, not a LiDAR measurement. Coordinates fit u18
up to n724; arbitrary asymptotic n requires a widened integer domain.
H and strong seeds W are linear for fixedK; explicit ancestor closure D
is quadratic. This does not prove LiDAR quadraticity, G4 timing, or an
impossibility of an exact compressed representation. No new capture or
qualification is claimed by this preparation.

ANALYTIC BAND MODEL (NOT A NEW EXECUTED CASE)
For j>=K-1, Ax=(K-1)^2*(2j-K+2)^2/16 is the squared first-cover radius.
Prefix-merge i has b_i=K^2*(2i+K+1)^2/16 and covers x_j when i>=j-K
(also require i in 0..n-K-1). Its descendants' activation is exact at birth.
For mcs<=K the MMt band ends at (1+eta)*Ax.
If sqrt(1+eta)*(K-1)>K, that band can itself contain Theta(j) prefix
ancestors, away from the finite end of the comb. Thus early band filtering
alone does not guarantee linear total porteurs.
K10 with eta=1/3 or1/2 satisfies the condition; K5 with eta=1/2 does not
on this family. This is an analytic diagnostic, not a LiDAR inference.
Timeout30 is solely a replay watchdog, not a mathematical search cap.
