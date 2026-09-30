L07 perf ledger — notes de travail (28 sept. 2026), lecture ce8a649dd

Mesures propres (hôte local partagé 8 coeurs, charge 5-13, nice 19, W2) :
- native_weighted_export (sha a53f1c4d..., build scratch session b64b3f68) K=2 :
  spherical 8k 2.57 s chaîne / 32k 14.14 s (x5.5, p~1.23) ; balls 91.7k / 388k ; tour 126 / 704 ms
  shells 2k 0.89 s ; 8k 15.2 s ; 32k 366 s (671 CPU-s) -> x24 pour x4 n, p~2.29 ; balls 43k / 172k (lineaire) ; tour 44 / 195 ms
- mhgp9_tower_probe (build v9-q3-payload-integration) K2 W2 :
  shells 8k : q34 11.45 s / chain 11.72 s ; expanded pairs 1.96M ; core_sites 143M ; dead_core_uniform_tests 328M ; q3 emis 9645
  spherical 8k : q34 1.35 s / chain 1.81 s ; expanded 105k ; core_sites 0.43M
- Python clustering (cluster.py/measure.py WIP) spherical 8k : json 1.2 + read 0.9 + births 0.4 + levels 1.4 + merge_tree 10.5 + measure 2.4 + condense 0.9 = 17.6 s
  merge_tree : 23 304 noeuds, 17 racines (FULL K2 = 1 racine), somme des members = 77.9M, RSS 3.8 Go
  32k : tué (SIGTERM, exit 143) deux fois pendant merge_tree (projection ~16x memoire ~60 Go > 31 Go)
- Campagnes precedentes (scratch b64b3f68/bench) : tour K2 z1 n500 0.26 s, n2000 1.6-1.75 s, n8000 16.5 s median ; sklearn HDBSCAN defaut 0.008/0.033/0.51/5.5 s (500/2k/8k/32k), oracle 8 tailles 47 s a 32k.

R22 ledger : r22_ledger.txt

shells K2 W2 probe (compteurs deterministes) 8k -> 32k :
  expanded_pairs 1 955 565 -> 32 290 448 (x16.5, p=2.02)
  core_sites 142 907 386 -> 9 131 487 059 (x63.9, p=3.00)
  dead_core_uniform_tests 327 743 595 -> 21 668 665 068 (x66, p=3.02)
  cover_sites 33 334 587 -> 1 983 731 327 (x59.5, p=2.95)
  q2_candidate_pairs 339 581 -> 4 125 738 (x12.1, p=1.80)
  q34 ms 11 446 -> 295 172 (x25.8) ; chain 11.7 s -> 297.7 s ; balls 43 266 -> 171 812 (x3.97)
  tour 42 -> 154 ms ; census 46 -> 137 ms
bd709ffb8d11003c2c7c06587e0254532dd396bcf45892e25731410736ed6fa8  shells_32000.u32le
2e1845012aa845f03d98c4d410140bf04a54e23bd92aa7d276bfcac6b02225a2  shells_8000.u32le
be860cfaa5793c0dbcae8a771124350e254caebde7156bee75f8bcca002ad68f  spherical_32000.u32le
dc1f10f9ba280dbf67fe32524e315688ec876ce71e5d75a21a1c22cc480d5ca8  spherical_8000.u32le
333e79d847c380ccb6f156cfc829b6fc0ee0c1f2176598e1ab120f8cc08ebf46  probe_shells_32000_k2.json
2ac381fec69ad62459517c7863a067a561f015c260b2a22c09c73c7463ad3637  probe_shells_8000_k2.json
f7426b82565a6587e370f718b5ef20b7b8bab4c85fce4110288e4dd1c279a80a  probe_spherical_8000_k2.json
