ER entry construction: exact K1 star cost and parent-discard oracle
================================================================

Scope and geometric realization
-------------------------------
The n sites are (j,0,0), j=0,...,n-1, on the integer grid, with K=1.
The FULL1 leaves are the singleton sites, born at squared radius 0.
All adjacent unit edges arrive at squared radius 1/4. Atomic treatment of
this plateau joins the n leaves at one root; no intermediate plateau node
is retained. Thus H=n+1 and each point i has the ancestor-closed coverage
profile {leaf_i:0, root:1/4}; D=sum_x |V_x|=2n. This is a realizable FULL1
profile, not a claim that arbitrary abstract stars occur for K5 or LiDAR.

The complete original er.py is preserved as er_snapshot.py, with SHA256
99e8ba720f417af9b267f8b70ea554be26c7545ea20e673a7dc89248debe321d.
probe.py extracts and executes the actual StructureER class by AST. Its
data-only adapter supplies this exact tree/profile; CountedDict observes
membership probes in the constructor without changing their answers.
No native engine, Scene, GPU, GCP, EOM selection or positive-anchor ER
point rule is executed. In K1 alpha^2=0, so that rule is outside its domain.

Exact cost and constructive repair
----------------------------------
The actual constructor asks whether each covered node has a covered child.
For root it searches the ordered child list until the point's own leaf.
Summed over all points, this pays 1+...+n=n(n+1)/2 child memberships,
although D=2n. Reversing child order or coverage insertion order changes
individual positions, not the total. Counts are operation counts, not
timings or a claim of global K5/LiDAR quadratic work.

For an ancestor-closed profile V, start with entries=set(V) and for each
u in V discard parent[u]. This removes exactly the nodes with a covered
child, yielding the same minimal entries in O(|V|) set operations. For our
profile there are exactly 2n discard operations. This is an independent
RAM oracle, NOT a shared-engine patch. Its validity assumes ancestor closure
and an atomic plateau tree; it does not repair a profile missing ancestors.
Sorting, Euler prefixes, numeric arithmetic and other constructor stages
remain paid. In particular, replacing this stage does not prove that the
entire StructureER constructor is O(D).

Executed fixtures and causal control
------------------------------------
20 small fixtures: n=2,8,32,128,512, each with both child orders and both
coverage insertion orders. The probe compares the actual entry vectors
with [[i] for i in range(n)], and compares the parent-discard vectors with
the actual vectors. Each case archives three hashes of the vectors actually
produced: source entries, parent-discard entries, and mutant entries.
Canonical vector serialization is json.dumps(vector, sort_keys=True,
separators=(',',':'), ensure_ascii=True, allow_nan=False).encode('ascii'),
then SHA256; the vectors contain only lists of integers. A semantic mutant
discarding u itself, not parent[u],
must empty every profile and is rejected in all 20 cases. This is not a
compiled-engine mutant. All guards are explicit exceptions, not asserts.
The n=8000,16000,32000 lines are ANALYTIC ONLY; no quadratic run at these
sizes is performed. Their membership totals are 32004000,128008000,
512016000; their discard totals are 16000,32000,64000.

Capture and static verification protocol
----------------------------------------
record.py may be run ONCE after review. It pins the five source files and
the shared er.py before/after, runs probe.py normally and with -O using
the same executable and -B, preserves raw stdout/stderr and statuses, and
then closes the manifest. Each run has a disclosed 15-second timeout.
Timeout failures retain partial stdout/stderr, timed_out=true and a null
exit code; failing captures are retained and no manifest is closed.

The closed inventory is exactly nine regular files:
  README.txt er_snapshot.py probe.py read.py record.py
  capture_normal.json capture_optimized.json execution.json manifest.json
The manifest hashes the eight other files. Supply its SHA256 externally:
  python3 -B read.py --manifest-sha256 EXTERNAL_SHA256
  python3 -B -O read.py --manifest-sha256 EXTERNAL_SHA256
read.py hashes the manifest BEFORE parsing it, rejects symlinks in the
root/ancestors/payloads, rejects nonregular files, duplicate JSON keys and
nonfinite numbers, checks the closed inventory, all hashes, exact argv,
source pins before/after, terminal statuses and normal/-O identity.
It independently reconstructs the 20 theoretical minima/permutations,
all counts and the semantic mutant, and the three analytic-only rows.
It recomputes the canonical hashes of [[i] for i in range(n)] and of n
empty lists, checks both real/fix hashes equal the former and the mutant
hash equals the distinct latter. The full entry vectors are not duplicated
in stdout. It does NOT execute
archived Python, re-run native geometry, or validate a different LIVE engine.
All hashed payloads are rehashed after static verification.
The external SHA is the trust anchor: an internally consistent replacement
archive is not accepted under a different expected SHA. A bad external SHA
must be rejected before payload use. Closed packets must never be edited.
