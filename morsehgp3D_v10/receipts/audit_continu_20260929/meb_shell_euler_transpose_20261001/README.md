# Exact shell-class counting by open sphere cells

Private independent Fraction proof R2, 1 October 2026. No engine, native import,
shared file modification, GCP, FULL performance or statistical qualification.
The authority is the closed manifest SHA supplied externally to `read.py`.
Normal and `-O` captures belong to the final frozen source only. Earlier direct
development preflights are not qualification receipts.

V1 remains unchanged at `/tmp/meb-shell-euler.B5OiSynB/`, manifest
`8b8c1167dfd0244b8a952df830ef23f6af32c8a7e9fa198253c18821260f48fb`.
It is an unpublished historical preflight: its face keys were length-u sign
tuples, so hashing those keys inside the annotation stage hid cubic work.
R2 confines those keys and their conversion to integer face IDs to the cubic
toy arrangement construction. The annotation program consumes only compact
IDs, rational 3-vectors and circle/vertex incidences. V1 also lacked the
fixed-K direct-formula tests below. Neither capture is retroactively rewritten.

## Mathematical object and formula

Sites are distinct, K-parties are unweighted sets of sites. Let B have positive
radius, center c, p strict interior sites I and u shell sites U. Write z=y-c.
A subset contained in B has MEB exactly B iff c belongs to the convex hull of
its selected shell sites. This does NOT mean it contains one canonical support.

For a nonempty subset S of shell sites, define
H_S={v in S^2 : v.z>0 for every z in S}. Strict separation gives H_S empty iff
c is in conv(S). Otherwise H_S is a nonempty open spherical convex region:
a gnomonic chart based on one member of S identifies it with an open convex
subset of R^2, hence its compactly supported Euler characteristic is 1.

Arrange the great circles v.z=0 as a genuine finite open-cell decomposition of
S^2. Coincident circles are grouped, antipodal points are NOT perturbed, and
zero signs are excluded from positive sets. A lone circle must be subdivided
with auxiliary vertices: S^1 itself is not an open one-dimensional cell.
For each open cell C let d(C) be its dimension, n(C) its number of strictly
positive sites, and P(C) their IDs. Additivity over open cells yields, for j>=1,

bad_j = sum_C (-1)^d(C) binom(n(C),j)
h_j = binom(u,j) - bad_j
h_jx = binom(u-1,j-1)
       - sum_C (-1)^d(C) 1[x in P(C)] binom(n(C)-1,j-1).

Set h_0=0 separately. H_empty=S^2 has Euler characteristic 2: the above
identity MUST NOT be used at j=0. Empty/invalid binomial terms are zero.

Consequently the number h_B(x) of K-parties containing x with MEB B is

x in I: sum_{j=1}^{K-1} binom(p-1,K-1-j) h_j
x in U: sum_{j=1}^{K} binom(p,K-j) h_jx.

All points outside B have h_B(x)=0. Zero-radius balls require their separate
singleton convention. Neither a native catalogue nor an owner generator is
supplied here; omitted ball classes stay a separate unresolved cost.

### Fixed K: one transpose instead of all shell orders

The shell generating polynomial is

H(t)=(1+t)^u+1-sum_C sign_C(1+t)^n(C), sign_C=(-1)^d(C).

The extra +1 corrects h_0=0. Multiplication by (1+t)^p accounts for strict
interior choices. Taking coefficients (equivalently, Vandermonde) gives

h_B(x in I)=binom(p+u-1,K-1)+binom(p-1,K-1)
            -sum_C sign_C binom(p-1+n(C),K-1), valid only when p>=1.

h_B(x in U)=binom(p+u-1,K-1)-[S^T lambda]_x,
lambda_C=sign_C binom(p+n(C)-1,K-1) if n(C)>0, and 0 otherwise.

Here S is the linear indicator map of positive-site incidence from the sparse
DAG below. Thus ONE transpose per K gives all shell incidences. All strict
interior sites share one scalar; p=0 has no fictitious interior scalar. The
total number of K-parts with MEB B is

binom(p+u,K)+binom(p,K)-sum_C sign_C binom(p+n(C),K).

Zero-positive cells are zero rows of S: arbitrary adjoints on those rows have
zero transpose. Setting their lambdas to zero also avoids binomials with
negative upper index when p=0. K greater than p+u correctly gives zero.

## Sparse annotations and their transpose

Give each shell point a FORMAL weight w_i, unrelated to statistical vote
weights. The desired cell annotation is the linear map
n_C(w)=sum_{i in P(C)} w_i.
Root a spanning tree of the face-adjacency graph. A root face is initialized
by one sum of at most u inputs. Crossing an edge toggles the weights in its
coincident-circle group (at most two distinct same-radius sites: antipodes).
Edge annotations are obtained from an adjacent face by removing its positive
zero-circle group. Vertex annotations similarly remove the positive groups
incident at that vertex. Total incidences are O(u^2), including multiple
circles through one vertex. These operations build a sparse ADD/SUB DAG.

First evaluate at w_i=1. For a fixed j, give cell output C the adjoint
lambda_C=(-1)^d(C) binom(n(C)-1,j-1), or zero when n(C)=0. Applying the
TRANSPOSE of that fixed linear DAG gives bad_jx for all x simultaneously.
This is NOT differentiation of binom(n,j); the lambdas are fixed scalars
after the integer counts have been evaluated.

Given a correct exact arrangement/DCEL and its incidences, annotation plus
all j<=K forward scalar counts/transposes costs O(K u^2) integer operations,
memory O(u^2+K u). A standard exact arrangement has O(u^2) cells and can be
constructed in O(u^2 log u) comparison/arithmetic operations, but rational
bit complexity and degeneracy handling must also be measured.

The toy arrangement in check.py deliberately scans sites/vertices and hashes
length-u sign tuples, including their conversion to compact IDs: O(u^3);
it is NOT an optimized arrangement implementation. The scan references and
debug validation of each output also cost O(u^3). Those are separate from
the sparse annotation program, which has no length-u face keys or per-cell
site scan. Its DAG and reverse-stage operation counts are measured separately.
At fixed K, each binomial costs O(K) elementary exact integer operations with
the direct multiplicative formula; or precompute binom(p+n,K-1) for n<=u
in O(Ku) operations, followed by O(u^2) annotation/transpose work. Big-integer
bit cost is not treated as constant. Over all balls,
the relevant totals include sum_B u_B^2, construction, class generation,
interior/shell census and ownership. There is no global subquadratic or
100-ms claim. Counts are exact arbitrary-size integers, not implicit u64:
at 100 million sites a point-incidence K10 count can need 221 bits; a total
shell 10-subset count can need 244 bits.

Those are bounds on FINAL counts, not on all accumulators. Euler sums and
transpose adjoints have both signs and can be larger before cancellation.
For this sparse face-tree/edge/vertex DAG, face adjoints are sums of output
lambdas; a conservative bound for input intermediate magnitudes is
T*sum_C abs(lambda_C), where T is the DAG term count. Thus additional
O(log(T)+log(number_of_cells)) bits may be needed. The scalar Euler sum has
the same cancellation issue. Integer arithmetic in this proof is arbitrary
precision throughout. Subsequent rational geometric weights and branch
masses need their own, generally larger, exact-arithmetic cost analysis.

## Constant-cost common case

If the shell consists of exactly one minimal positive support of size
q=2,3,4 (antipodal pair, acute triangle, strict tetrahedron), no proper
subset surrounds c. Then h_q=1 and all other h_j=0:

x in I: h_B(x)=binom(p-1,K-1-q)
x in U: h_B(x)=binom(p,K-q).

The condition is checked, not inferred solely from shell size or support ID.
For u<=4 arbitrary degeneracies can be handled by at most 16 shell subsets.
For larger degenerate shells the Euler/transpose fallback is the constructive
option above. Its port and gain on actual workloads remain unmeasured.

## Causal checks

Ten exact shells: one direction, antipodal pair, two non-antipodal directions,
square, octahedron, tetrahedron, cube, rational coplanar hexagon, hexagon with
poles, tetrahedron plus an extra rational same-radius point. Every h_j and
h_jx (j=1..5) is checked against an independent exact Caratheodory oracle:
628 subsets. Full MEBs are independently reconstructed by affine-support
Gram solves for 82 K3/K5 parts of two clouds with strict interior points.
The resulting point counts agree with the shell/interior convolutions.

R2 additionally tests all ten shells for every p=0..5 and K=1..8: 480 fixed-K
cases. All 41,808 subset occurrences (20,505 distinct subsets memoized) have
independent exact full MEBs. The oracle does not use shell convexity, Euler
or the convolution: if MEB(F minus z) covers z it is MEB(F); otherwise a
support of at most four essential points is solved by the original affine
Gram oracle. Exact center/radius and containment masks are memoized only to
avoid recomputing the same rational geometry. Every fixed-K scalar/one-
transpose count agrees with both these MEBs and the older h_j convolution.
The zero-positive-row property and p=0 scalar guard are explicit checks.

Every sparse linear annotation is checked at all-one and arbitrary signed
formal input weights. Its transposed point counts agree with both the
direct-cell reference and the independent convex-hull oracle.
Five mutants are rejected: treating zero as positive, omitting alternating
cell signs, counting only subsets containing one canonical support, and
replacing signed DAG coefficients by their absolute values in the transpose,
and omitting the empty-shell correction in the fixed-K interior scalar.

For the octahedral shell +/-e1,+/-e2,+/-e3: h_2=3, h_3=12, h_4=15, h_5=6.
Each site belongs to five K5 parts. Requiring the canonical diameter +/-e1
would count only four for x=+e1. With the center added as an interior point,
all 21 K5 parts have the unit ball as MEB and each point belongs to 15.

The cube has 50 cells, 58 DAG nodes, 127 sparse terms, 635 reverse term visits
over j=1..5. These are exact operation counters, not native/G4 timings.

## Replay

`python3 -B read.py EXPECTED_MANIFEST_SHA256`

The reader verifies the EXTERNAL manifest SHA before JSON loading/execution,
refuses symlinked root/ancestors/payloads and nonregular payloads without using
resolve(), checks the closed seven-file inventory and strict SHA/schema types,
rejects duplicate JSON keys and floating/nonfinite JSON values, and checks
every file hash, both captures and normal/-O
replays, and that the files remain unchanged after replay. Shared MEMO/mmc
hashes before/after recording are provenance only: no shared source is
imported and standalone replay does not need those files.
