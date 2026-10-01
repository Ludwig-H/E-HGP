PREPARED / NOT EXECUTED: distinct-point endpoint counts by LCA cancellation
===========================================================================

This directory is a NEW R2 recordable SOURCE candidate. It is NOT a receipt.
No probe/reader/record has been run here; no manifest is closed. The earlier OPEN diagnostic
/tmp/er-distinct-lca.k482f80Q remains untouched. Root independently ran that
earlier probe (SHA 8416513270022cdd0c5bb23cc2dc6f4bb9bd2bd5f4c4e396dad7b4cb6e7297a7)
normally and under -O: 20 abstract profiles, 180 endpoint cardinalities,
27 B and 4 F corrections; mutants all-B 4, all-F 12, child-sums 10,
mcs2-cap 17, signed-clamp 5. Those runs are NOT captures of this candidate.

Snapshots and source selection
-----------------------------
r1_snapshot.py is the complete original R1 source, byte-identical to SHA
debe1ec3eff15ccbb5965b4e29ccc2f648c5fb26f777605c89301c25b9e5f00f.
er_snapshot.py is the complete original ER source, byte-identical to SHA
99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d.
The new probe pins both before use and after all checks. It compiles only
the class StructureER from ER and these six definitions from R1, in their
original order: Tree, owned_seeds, full_profile_oracle, intervals,
distinct_counts, fixtures. It does NOT execute R1's module body, source(),
main(), or any shared/LIVE filesystem check. The small globals adapters
provide Fraction, bisect_right and explicit need/exiger guards only.
This removes all dependency on mutable engine/workflow files; snapshot
provenance is the pinned archived bytes, not the then-current shared tree.

The 20 fixtures are the original four rational trees times five modes.
Mode1 additionally owns one redundant late seed of point0 on its already
covered leaf: this preserves the R1 point profile and tests early/late
occurrences of the SAME label at the SAME owner. No geometric realization
of these abstract profiles is claimed. No Scene/native engine/GCP/GPU
call, seed-completeness qualification, or 100ms performance claim exists.

Mathematical identity
---------------------
Replace original v by F_v with child B_v. B_v contains own birth-equal
occurrence leaves and the F_child subtrees; F_v additionally contains own
late occurrence leaves. Occurrence DFS order is early, children, late.
F_v labels are precisely points covered strictly before death; B_v labels
are points covered at birth, inclusive. Seeds must be COMPLETE, owned by
living nodes, with every activation at death normalized to its live
parent BEFORE this stage. Positive-lifetime nodes are already the atomic
quotient of exact simultaneous plateaus. This fixture has one true root;
a forest requires a purely structural super-root, never an extra vote,
or separate per-root processing.

Give each occurrence leaf +1. For each point, subtract 1 at the augmented
LCA of every CONSECUTIVE pair of its occurrence leaves in DFS order. In
any subtree its k occurrences form a consecutive label subsequence.
Exactly k-1 such pair LCAs are inside if k>0; none are inside if k=0.
Thus the signed subtree sum is exactly the indicator of that point's
presence. A pair with an outside endpoint cannot have an inside LCA.

Let w be the ORIGINAL LCA of the two owners. The augmented LCA is F_w
iff an endpoint is OWN to w and LATE; otherwise B_w. In particular late
endpoints strictly below w still meet in B_w, not F_w. Initialize
dB_v=#own early occurrences, dF_v=#own late occurrences; subtract each
correction at its target. Original Euler prefix sums of w_v=dB_v+dF_v give
  before_death[v] = P[tout[v]] - P[tin[v]],
  at_birth[v]     = before_death[v] - dF_v.
dF_v equals the number of labels in F_v but not B_v, hence is nonnegative.
dB and prefix sums CAN be negative. Their absolute values are bounded by
the paid occurrence count T (T positive units and T-n corrections when
every one of n point labels has at least one occurrence). Do not use an
unsigned count or clamp corrections. mcs-capped cardinalities cannot
decide ER-n equality: different exact sets may both exceed mcs.

Cost and port scope
-------------------
Given COMPLETE normalized owners, layout, signed corrections and prefix
passes use O(H+T) operations PLUS at most T-n ORIGINAL LCA queries at the
cost of the selected index, PLUS label grouping. This RAM probe deliberately
uses a slow parent-walk LCA and reports that work separately. It does not
implement linear LCA preprocessing. A GPU port can stable-sort by
(pointID,eventIndex), issue adjacent-pair LCA queries, signed scatter,
then a prefix scan. Sorting, LCA preprocessing, IDs, owner normalization,
exact comparisons/bit cost, and complete seed extraction all remain paid.
No proof of O(H+T) for the whole HGP pipeline or native implementation is
inherited. Duplicate occurrences are counted in T, not assumed free.

Checks and independently static reader
-------------------------------------
The probe compares new counts against the R1 DISTINCT-label Fenwick,
independent expanded point-set oracle, and real snapshot AST StructureER
_cs prefixes (right/inclusive at birth, left/strict before finite death).
It separately builds the augmented tree and checks its LCA and subtree
sums. Guards require all-B/all-F/sum-children/mcs2/signed-clamp causal
mutants to fail nonvacuously. All checks survive Python -O.

read.py never imports or executes any archived Python. It validates the
external manifest SHA256 BEFORE parsing the manifest or other payloads,
rejects duplicate JSON keys/nonfinite constants, checks the exact REAL
inventory, rejects symlinks/nonregular payloads/root ancestors, and pins
all files. It reconstructs the 20 rational fixtures independently, their
normalized seed ownership, complete point sets, augmented LCAs, signed
arrays, all cardinalities and all causal outcomes. It requires exact
source inventories before/after, exact two argv vectors, integer code0,
no timeout, finite duration, empty stderr, normal/-O stdout identity,
and rechecks every hash after reading. Python provenance is archived,
not a requirement to find the original executable at read time.

R2 preparation / source closure
-------------------------------
The previous prepared candidate /tmp/er-distinct-lca-recordable.KuqGt1hx
is unchanged, not closed, never executed. This R2 corrects its collector,
NOT its mathematical probe. probe.py remains byte-identical SHA
3f57df80ecdbaff923b56a2eea18031892a6544c9a639429fa73c5ce31b6f27e.
ER99e8 and R1debe snapshots are identical too. origin.json preserves both
the OPEN replayed predecessor and the never-executed prepared predecessor.

Current source inventory: README.txt, er_snapshot.py, r1_snapshot.py,
probe.py, read.py, record.py, origin.json, protocol.json. No SHA256SUMS
exists pending parent review. Only after review, close these eight files
with an ASCII 'SHA256  filename\n' manifest. Its external SHA is authoritative.
Change NONE after source closure; any repair needs a NEW source directory.
Source closure is NOT a successful execution receipt.

record.py requires the external source pin BEFORE parsing its manifest.
It hashes ALL source bytes, then compiles/executes the exact verified
read.py bytes with a non-main namespace and __file__ adapter. No local
importlib/SourceFileLoader/.pyc is used. It never loads a cached reader.
read.py itself never executes any archived probe/snapshot/record. A
source-only read verifies manifests and protocol without running a probe.

Immutable attempt protocol / atomic aggregate
---------------------------------------------
A future record --out must be a fresh canonical absolute directory OUTSIDE
the immutable source, with nonsymlink ancestors. The collector is a dedicated
POSIX CLI, not an embeddable library. Python identity names the resolved
actual binary; this provenance resolution is NOT used for archive paths.

Before each possible child launch, write and fsync an immutable checkpoint:
normal.command.json or optimized.command.json. Exact fields:
schema, index, mode, argv, cwd, timeout_seconds, checkpoint_utc,
source_before, python_before, first_signal.
Its argv is [actualAbsolutePython,-B,(-O),sourceOrigin/probe.py], with
cwd=sourceOrigin. The checkpoint is INTENT, not a claim the child started.
A signal arriving before launch leaves launched=False and no invented code.

Each attempted run owns four files, written with exclusive xb creation
and fsync, never rewritten:
  MODE.command.json    checkpoint before launch
  MODE.stdout.json     FULL raw stdout bytes (partial on timeout)
  MODE.stderr.bin      FULL raw stderr bytes (partial on timeout)
  MODE.receipt.json    immutable exact attempt metadata
MODE is normal or optimized. Raw streams are stored BEFORE source-after
and semantic checks. The first attempt's files cannot be overwritten by
a second attempt or aggregate publication. Any code failure, launch error,
timeout, integrity or semantic failure stops the next attempt. No retry.

The immutable receipt's exact fields are:
schema, index, mode, checkpoint_file, checkpoint_sha256, argv, cwd,
timeout_seconds, launched, timed_out, exit_code, launch_error,
integrity_error, judge_performed, judge_error, start_utc, end_utc,
duration_seconds, stdout_file, stdout_sha256, stderr_file, stderr_sha256,
source_before, source_after, python_before, python_after,
first_signal_at_receipt_creation.
Terminal success means true integer code0; timeout has exit_code=null,
timed_out=True and partial raw streams; launch errors have no invented
code. Independent semantic judgment runs only after clean code0, empty
stderr, unchanged sources/Python and no known signal. Its real outcome
is recorded, not inferred from a nonempty output. Duration covers the
subprocess collection, not source verification/fsync/reader overhead.

run_receipt.json aggregates immutable attempt rows and provenance:
schema, source_manifest_sha, origin, protocol_sha256, timeout_seconds,
runs, interrupted, first_signal, collection_complete.
origin names package_root, exact source_files paths/hashes, manifest
path/hash, executable identity. executable fields: path, sha256, version,
version_info, implementation. collection_complete only means two attempt
receipts exist; it NEVER implies qualification.

The aggregate and capture SHA256SUMS are published separately with a fresh
temporary file, fsync, atomic os.replace and directory fsync. No attempt
file is replaced. A signal during serialization triggers republication
with refusal state. The two aggregate files are not one filesystem
transaction; an interruption/crash between them leaves a hash mismatch
and must be rejected, not silently healed by the reader.

Managed interruption protocol
-----------------------------
After creating the fresh output directory, install handlers for SIGINT,
SIGTERM, SIGHUP, SIGQUIT before checkpoints/collection/publication.
The FIRST handler delivery records exactly:
number (integer), name, received_at_utc, active_run_index.
It assigns this object once, defers raising while collecting/archiving,
and ignores subsequent managed signals so the first evidence is retained.
The scheduling gate skips a next attempt after a known signal; the dispatch
gate is checked again after checkpoint. Cancellation observed there yields
an unlaunched attempt. A signal racing with actual process dispatch may
still allow that committed attempt to start; it is collected and refused,
never retried. No subsequent attempt is scheduled after the signal.

Collection uses subprocess timeout 15 seconds. A parent managed signal
does not abandon communicate or destroy the first stdout/receipt; the
collector waits for terminal output or that timeout, then records it.
The four handlers are installed with the managed signals temporarily
blocked, then the original signal mask is restored BEFORE child creation.
An environment with any managed signal initially blocked is refused
before capture, rather than claiming that a permanently pending signal
was observable by this protocol.
The child is NOT protected from process-group signals: if it dies, its
real return code/streams remain a failed attempt. Signals received during
post-run verification or publication are also retained. The aggregate
interrupted flag makes ANY handled managed signal nonqualifiable, even
if the child itself returned code0 and happened to emit correct counts.

After the final collector seal, the handlers remain installed until CLI
exit. A first late managed signal republishes the aggregate as interrupted
and raises SystemExit(128+signal), preventing a stale passing aggregate/
exit code. A summary printed before a late signal can become stale: the
old capture pin must then fail against the changed archive. The archive
reader and its trusted external pin, not a printed accepted flag alone,
decide qualification.

Limits are explicit: SIGKILL, machine/power crash, uncatchable termination,
storage failure, interpreter fatal crash or hostile filesystem races
cannot guarantee a terminal receipt or even complete partial streams.
A checkpoint may remain without a completed receipt; collection status
is unknown and REJECTED. No hard real-time guarantee covers process
creation, kernel scheduling, uninterruptible I/O, fsync or Python cleanup.
The 15s subprocess timeout is a diagnostic policy, NOT a product quota.
The first preserved signal is first Python handler delivery; POSIX can
coalesce pending signals and does not promise physical arrival ordering.
The collector intentionally does not restore managed default handlers
before CLI exit and must not be called as a long-lived library function.

Static qualification / future commands
--------------------------------------
A complete capture contains exactly ten regular files: four per attempt,
run_receipt.json and SHA256SUMS. The manifest hashes the nine payloads.
Partial or interrupted captures are preserved but rejected. The static
reader first verifies the external source AND capture pins, exact REAL
inventories and nonsymlink paths. It rejects interrupted=True, non-null
signals in aggregate/checkpoints/receipts, unlaunched attempts, timeouts,
errors, false/float codes, missing semantic judgment, nonempty raw stderr,
changed sources/Python and all oracle mismatches. It independently rebuilds
all 20 exact fixtures/cardinalities/corrections/causal outcomes. It compares
aggregate entries to actual immutable attempt files, command hashes to
actual checkpoint bytes, then rechecks ALL source/capture hashes.
No LIVE mutable workflow or current Python executable dependency is needed
for static reading; archived Python provenance is checked structurally.

FUTURE commands, only after source review/closure and explicit run approval:
  python3 -B read.py --source-manifest-sha SOURCE_SHA
  python3 -B record.py --source-manifest-sha SOURCE_SHA --out NEW_ABS_PATH
  python3 -B read.py --source-manifest-sha SOURCE_SHA --capture PATH --capture-manifest-sha CAPTURE_SHA
  python3 -B -O read.py --source-manifest-sha SOURCE_SHA --capture PATH --capture-manifest-sha CAPTURE_SHA
A false external pin must refuse code2. No source/capture manifest or run
evidence exists for R2 yet. No probe/reader/record is executed by preparation.
