# Phase A physical-head sidecar (2026-09-23)

Scope: one complete 08/000000 no-ground 1 mm cloud, K1..10, s8, W48,
`static_threads=48`, all five generation levers on and `tower_meb_proposal=1`.
This is a local CPU exploration, not a G4 performance qualification.

- Product pin: `ec6d1b74`, `full_ball_tower.hpp` SHA-256
  `77588b5d1880c60aa881fdc6ffb1e847494f9e8a4a194b12a0885ec311ebf865`.
- Input `.u32le` SHA-256
  `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf`.
- Exact field-stream catalogue: 5,512,670 balls, 1,141,122,698 bytes,
  SHA-256 `c9829439ca819b48f5f0dc46cb7f5cdf7b4b4cae876c7881397e4c77fb461563`.
  Generator library SHA-256 `03fba8372ff3e7611434cc93a1ab7d847843cbf81e7f0005fb1be6df4dce27ba`.
  No struct padding is written; the stream uses the pinned native little-endian ABI.
- `shadow_measure.patch`: 6,440 bytes, SHA-256
  `65684fd29df3088240ef3c281d204ab7bf95aec9a6db537770e15558957b00f4`.
  It keeps the product DSU as authority and checks every query against physical heads.
- `catalogue_dump.patch`: 1,830 bytes, SHA-256
  `e25ce253e05b96ad871fa664155ec4f2bdfeef2121d9e5a008f9109c2796a3d3`.
  It applies to `tower_chain.cpp` at `ec6d1b74`, source SHA-256
  `6d97f5fd054bdefffdb3f031571fee3130307fc3e63ebfd312a612892e90785c`.
  The environment variable `MHGP9_AUDIT_CATALOGUE` writes every explicit
  BallData field after exact census and before FULL; `--no-tower` ends there.
- `physical_heads.patch`: 3,808 bytes, SHA-256
  `0465314cdef7c09d175b035a1d3b0e521d9aff2db85b59e3c63d6d5c6abbeb86`.
  `git apply --check` passes on the product pin. It is an ablation, not a proposed port.
- `build_tower.cpp`: 4,941 bytes, SHA-256
  `28fcb7634bcbe582ee399f7d12abbbac50fb4484b40c9af9284412897732de92`.
  It rebuilds the immutable cloud index, loads the exact catalogue, calls FULL,
  and serializes every population, node, parent, successor, contribution and vertical image field.
- `run_pairs.py`: archived 1,625 bytes, SHA-256
  original `1735e183dfa143a7ae5c4a41e503affea3a990e24f26408cb1b9870d728cadd6`.
  The archived copy changes only the raw input path to an equivalent
  workspace file of the same SHA-256; its SHA-256 is
  `01be8bd5a4604110edeee36599079b00167e386a2113537f23cad35f4a6b17ed`.
  Three local CPU pairs use order AB, BA, AB; A is the product header, B the minimal patch.
  Each of six 1,056,931,646-byte outputs passes `cmp` against the baseline;
  canonical stream SHA-256 `ea3c642bbcfe22ec28b8b7ce0474f85b3296eb19574612b05f91622fb7a0e31a`.
  The summaries are in `pair_summary.json`, SHA-256
  `441707713f213eb6120e7aeb928cc8f76b439f189de37b4e8c797acf48bbd384`.
- `shadow_counts.txt` keeps the ten K lines of the original sidecar stderr,
  SHA-256 `192ceb88d88bc48baa9c355e38b1f8125a2273634e8174af51a8133f094d455c`.
- `phase_a_timing.patch` (1,030 bytes; SHA-256
  `305b21b0a12c217ce0e8f0426ba2244c722c5c918f1d6d27cd6c8efce5963497`)
  adds the same per-thread CPU timer around `order_lots` in each variant.
  `timed_summary.json` (SHA-256
  `9e0468cf5947e3d5ff5c10a6c14899f099851651a2f4c862550925127608494b`)
  records the ten orders of one extra paired run and bytewise output equality.

Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` with
`-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread`.

Build commands (from `/workspaces/E-HGP`). Source copies under `/tmp` were
made from the pinned `ec6d1b74` tree; the catalogue, shadow and physical-head
patches are independent, each applied to a fresh copy of its target source.
The timing patch applies separately to both tower variants. The product checkout was
not edited. The local generator library is an external LIVE dependency,
identified by the SHA above; the catalogue and serialized outputs are not
archived because each exceeds one gigabyte.

```sh
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I/tmp/mhgp9_phase_a_sidecar/src/gen -c /tmp/mhgp9_phase_a_sidecar/src/chain/tower_chain.cpp -o /tmp/mhgp9_phase_a_sidecar/tower_chain.o
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I/tmp/mhgp9_phase_a_sidecar/src/gen -c /tmp/mhgp9_phase_a_sidecar/bench/tower_probe.cpp -o /tmp/mhgp9_phase_a_sidecar/tower_probe.o
g++ -O3 -DNDEBUG -std=c++20 /tmp/mhgp9_phase_a_sidecar/tower_probe.o /tmp/mhgp9_phase_a_sidecar/tower_chain.o /workspaces/E-HGP/build/v9-audit-current/libmhgp9_gen.a -pthread -o /tmp/mhgp9_phase_a_sidecar/tower_probe_dump
MHGP9_AUDIT_CATALOGUE=/tmp/mhgp9_phase_a_sidecar/scene00.catalogue.bin /tmp/mhgp9_phase_a_sidecar/tower_probe_dump morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le 10 48 --s=8 --static=48 --grid=1mm --no-tower --lever=atlas_saturate_deep=1 --lever=q3_leaf_census=1 --lever=q34_dead_lanes=1 --lever=q34_witness_cache=1 --lever=q34_dead_core=1 --lever=tower_meb_proposal=1
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src /tmp/mhgp9_phase_a_sidecar/build_tower.cpp -pthread -o /tmp/mhgp9_phase_a_sidecar/build_tower_baseline
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I/tmp/mhgp9_phase_a_fast/src /tmp/mhgp9_phase_a_sidecar/build_tower.cpp -pthread -o /tmp/mhgp9_phase_a_sidecar/build_tower_fast
/tmp/mhgp9_phase_a_sidecar/build_tower_baseline morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le /tmp/mhgp9_phase_a_sidecar/scene00.catalogue.bin /tmp/mhgp9_phase_a_sidecar/baseline.canonical
python3 /tmp/mhgp9_phase_a_sidecar/run_pairs.py > /tmp/mhgp9_phase_a_sidecar/pairs.log
```

The catalogue was generated once by the copied chain source with a sidecar-only
field-stream hook `MHGP9_AUDIT_CATALOGUE`, then held fixed for all pairs.
The build used the same pinned source tree and local `libmhgp9_gen.a`; the
catalogue stream hash is the authority for the ablation input. The complete
chain shadow run produced the same published R7b status, tower digest and
per-order summaries, but its timing is contaminated by audit checks.

Tower times A/B in paired order (ms): 13074.3/13194.8, 13158.7/13079.6,
13542.1/13816.9. Differences B−A: +120.5, −79.1, +274.8 ms. No stable gain.
RSS (KiB) for A/B by pair: 4490856/4606216, 4626192/4789444,
4572116/4627084; these are local process peaks, not additive allocations.

K10 shadow: 2,117,675 blocks; 3,831,431 root calls; 11,865,867 parent
traversals; 9,029,520 path-compression writes; 2,081,320 singleton lots;
16,336 grouped lots containing 36,355 blocks; maximum lot size 8;
3,940,560 physical-head relabels over 1,638,573 nodes, maximum 10 per node.
The full K1..10 histogram is in `shadow_counts.txt`.
The separately instrumented K10 phase A takes 2,179.729 ms thread CPU in A
and 2,290.302 ms in B, with equal serialized outputs. This is one local
pair, not a G4 comparison or a stable causal estimate.
