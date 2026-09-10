# FULL residence: small qualified deltas and remaining streaming work

Private bounded investigation, 2026-09-10, active source lineage d188e3de. No active source, Git/index or GCP mutation. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Recommendation

Take the two small deltas in `both_final/morsehgp3D_v7/src/forest/`: release dead construction state before the immutable bank is copied, and reserve exact journal arena sizes before encoding. They preserve the existing API, immutable bank and all-or-nothing result publication. Their combined qualification passes the strengthened 28-cloud oracle gate O2 and ASan/UBSan, the structural journal gate with every observed allocation failure injected, and strict physical digest comparison on n=200/400/800, uniform seed3, s=8, K=1..10, one CPU thread.

These are useful but do **not** solve the large-cloud residency problem. On n=800 they reduce retained output by 23.09% and requested allocation volume by 17.46%, but the requested-byte peak by only 3.01%. The construction of order10 already reaches the remaining peak before the immutable bank or any journal is encoded. A later flat or streaming construction is still needed.

## Measured sizes and scope

Local ABI: ExactLevel48, FullNode64, FullDatedContribution80, Population48, Ref16, Action48, Batch80, Draft48, History48, Block40 bytes. A singleton batch therefore has at least 128 bytes of batch/action headers, before parent/contribution arrays and allocator costs. A regular birth adds a16-byte reference. The final node/contribution/successor representation is not free: a single birth occupies152 logical bytes, before its vertical image and bank data.

Instrumentation overrides ordinary new/delete and tracks requested bytes only during `build_full_ball_tower`. It excludes the already-constructed CloudIndex and BallData input, malloc metadata/rounding/fragmentation, the instrument's own16-byte allocation headers, shared libraries, stack and process RSS. No over-aligned allocation occurs in the inspected source path. `requested_total` counts reallocations cumulatively; it is not live memory. The probe is single-threaded. Generation/census precede measurement and use the same frozen source closure for every variant. This is **not** the requested 8k/16k/32k scaling campaign and is not a50k or GPU contract test.

| n | baseline requested peak | both requested peak | baseline output retained | both output retained |
| ---: | ---: | ---: | ---: | ---: |
| 200 | 20,484,772 | 20,484,772 | 13,560,236 | 10,327,116 |
| 400 | 46,025,976 | 43,098,194 | 32,494,364 | 25,476,444 |
| 800 | 108,452,176 | 105,186,724 | 75,097,940 | 57,757,460 |

All values are bytes, not MiB. At n800: 238,123 balls, 308,430 nodes, 308,420 parent references, 186,495 contributions/population rows. Requested total allocation falls428,722,888→353,885,600 bytes. New calls fall4,984,339→4,983,611: exact reservation saves large arena reallocations, not the millions of small construction allocations. Final arena capacities fall60,424,192→43,083,712 bytes, including unchanged lower-node capacities. Release alone keeps output size unchanged and reaches105,186,724 bytes peak. Reserve alone reaches107,702,972 bytes peak.

Wall times are single exploratory samples on a shared machine with other agents compiling/running. They are recorded, not promoted into a speedup claim. This packet demonstrates representation/memory reductions and preserved outputs, not the50k latency contract.

## Phase localization: separate instrumented baseline n800

The `phase` tree adds only no-allocation diagnostic markers and runs n800. Counts, memory totals and physical digest match the baseline. Markers themselves affect timing, so their durations are explanatory observations only.

- Construction of order10: peak105,186,724 bytes. After its lower cursor is destroyed, live93,030,664 bytes.
- Immutable bank copy: live/peak107,702,972 bytes; construction populations and immutable copy coexist. Releasing construction populations lowers live memory to88,104,496 bytes.
- Last journal encoding: peak108,452,176 bytes while its geometrically growing arenas overlap old storage; after all drafts are freed, live89,957,664 bytes.
- Builder destruction finally leaves the75,097,940-byte result. Dead state accounts for14,859,724 requested live bytes at that final boundary, but this number must **not** be subtracted from the earlier global peak.

This refines the independent auditor's `receipts_tower_cost_review_20260910/README.md`: its642MB dead-state and2.19GB draft-header estimates at32k are logical volumes at different phases, not additive RSS savings. We have not rerun32k here.

## Exactness and failure semantics

The release patch runs only after all orders have completed and the final lower cursor has gone out of scope. Only domain, populations and drafts are read by the remaining bank/encoder work. Draft levels, node IDs and contribution refs are values; they borrow none of the released storage. Empty-vector/map swaps release capacity; clear alone would not.

The reservation patch makes one linear pass over the existing batches. It counts a node and its parent references exactly when parent cardinality differs from one; continuations add contributions but no new node/parent references. Counts use checked size_t addition. The ordinary validator remains unchanged and still checks the entire pre-lot root set before publishing any post-lot node. A failed allocation/count/validation returns an empty failure result. Under memory exhaustion a malformed journal can fail allocation before reaching its semantic rejection, as already possible in the old interleaved builder; no successful malformed result is introduced.

The immutable population bank, lazy row order, population IDs, CSR offsets, node IDs, parents, successors, all dates, refs and vertical images are unchanged. The physical digest includes those actual arrays and population rows, not only point-cover equivalence. All four variants produce the same three digests.

Final oracle gate SHA256: `bf1a28242dd2d6897f3dc02edff8c6077dbec1caa3f9b9a67813dcc748e3d75a`. O2 and ASan/UBSan outputs agree:170,320 checks,28clouds,112orders,2,508cuts,45,948vertical checks,8rejections; the grouped growth/inert fixtures are included. Structural gate:823checks,30rejects,30replay cuts,40Gamma cuts,20allocation refusals. The previous34allocation sites shrink to20 because14geometric arena allocations disappear; **every currently observed allocation is still injected** and the original non-vacuity floor20 passes unchanged. Both gates return0 on `--selftest`,2 on unsupported arguments. Successful stderr is empty.

## Next design: streaming without changing immutable/public contracts

1. Keep lazy populations in the existing first-contribution order. Eagerly prebuilding them by BallKey or by level can change physical population IDs: within a tied level, DSU groups may visit blocks0,3 before blocks1,2. Building all BallData populations also retains top-window connection rows that never contribute.
2. The smallest useful streaming extension is an **internal, unpublished journal assembler**, sharing the current structural validation code. It consumes one complete exact-level lot, validates all pre-lot roots and refs against currently available construction rows, appends to flat private arenas, then discards the lot actions. Never expose a certificate with a mutable bank or an order marked valid before it is sealed.
3. After each order, release its resolver state except the lower history/anchors needed by the next order. Keep completed arenas private and sealed against further writes, but unattached to a public immutable bank. After the global population rows are finalized and validated, create one immutable bank, attach it to all privately staged forests, check vertical/node lengths, and publish only if **every** order succeeds. All failures discard the whole pending tower.
4. Prefer ownership transfer into the immutable bank through an internal factory, after exactly the existing bank validation, to avoid the second population copy. A shared_ptr<const T> must not alias any still-mutable external object. This is an internal refactor, not permission to mutate an already-published bank.
5. Prove physical equivalence and transactional allocation failures against this packet and the strengthened grouped oracle gate. Preserve batch boundaries, parent pre-lot state, inert anchors and dated growth; preserve all population/ref IDs and vertical identities. Instrument residence before claiming any saving.

Per-order encoding after an already-finalized immutable bank is an alternative, but the current lazy row order makes that require another semantic classification/replay or a new indirection. Per-lot private assembly avoids that complication. Output size itself remains a lower bound: holding the full tower necessarily keeps all final arenas. The extra construction residency can become order-local (plus shared catalogue/programs/populations and lower history), rather than the sum of all Draft.batches. No universal linear-in-n or subquadratic claim follows from this output-sensitive statement.

A new output format storing a birth's population directly in the leaf could eliminate redundant80-byte birth contributions, as the auditor suggests, but would change the v2 CSR/dated-record contract and physical digests. It is explicitly **not** part of these API-preserving deltas.

## Capture and reproduction

`record.py` freezes project-header closures, compiles private trees and records commands/stdout/stderr/codes. The frozen baseline pins are in `sources_frozen.json`; the strengthened-gate snapshot has its own before/after pins. Boost is the existing extracted system include tree `build/v7_boost_gate/extracted/usr/include`, not an installed dependency. The initial gate compile omitted that include path and failed before compilation; its logs are retained, followed by the corrected successful compile. No source defect was inferred from that setup error.

`verify.py` checks hashes, expected codes, physical output equality, O2/SAN identity and source closures without running large work or using assert. `seal.py` creates the portable manifest/summary and source-after captures. The local ELF files are deliberately outside the portable manifest but their individual SHA256 files are recorded. No executable, cloud resource or active process is needed to read the report.

GCP non utilisé.
