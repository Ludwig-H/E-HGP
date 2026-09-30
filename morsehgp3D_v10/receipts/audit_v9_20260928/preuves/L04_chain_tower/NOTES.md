# L04 catalogue et tour — notes de travail (audit v9, ce8a649dd)

Chemins (worktree build/v9-open-worktree/morsehgp3D_v9) :
- tour : src/tower/forest/full_ball_tower.hpp (3097 l.), Builder l.564 ; run 590 ; run_orders (seq, temporel) 621-692 ;
  finish (banque+encodage) 694-743 ; run_orders_parallel 788-851 ; run_orders_overlapped 878-1132 ;
  order_prepare_lean 1180 ; order_block_lean 1240-1275 ; order_lot 1277-1352 ; order_lots 1354-1406 ;
  populations 1408-1555 ; order_images 1557-1590 ; key index 1654-1687 ; validate_catalogue 1795-2044 ;
  intruder_work 2061 ; static_terminal 2095-2146 ; prepare_external_batch 2171 (mort) ; resolve_hashed_order 2360-2539 ;
  prepare_static_order 2541-2756 ; resolve (temporel) 2772 ; count/visit_block_at 2843-2907 ; close_lot 2946-3037.
- banque/journal : full_coverage_certificate.hpp (FullCoveragePopulation l.20 vecteurs ; FlatDraft 175 ; build_from 287 revalide+copie).
- quotient local : local_plateau.hpp rank(k) l.100 ; prepare_contains l.189.
- chaine : tower_chain.cpp census_key l.881 ; Euler 2024-2039 ; sceau 1214 ; appel tour 2065-2092 ; tower_digest 1249 (niveaux bruts non reduits).
- ExactLevel 48 o, BallKey 80 o, BallData 224 o, FullNode 64 o, FullDatedContribution 72 o.

Mesures relues (recus) :
- R22 G4 ng00 sans sol 39 885 sites : K5 tour 289 ms (val 34, st 84, lots 102, pop 11, img 17, enc 39), lots_by_k [71,51,78,106,148];
  K10 tour 1212 ms (val 117, st 738 dont K10 208, lots_by_k K10 585, images own K10 125, enc K10 93), RSS 6,36 Go ;
  noeuds K1..K10 = 79 681 ... 1 638 573 (7,43 M) ; MEB 11,3 M ; intrus 403 M noeuds ; boules 5,51 M.
- core_warm probe_1 : helper_threads 409 (K5), 889 (K10 R22) threads crees par construction.
- local W4 (q3_payload_local_20260926) uniforme 8k/16k/32k K5 tour 433/892/2132 ms (x2,06, x2,39), boules x2,08/x2,05.

Proxy synthetique (lean_phase_a.cpp, EPYC 7763 partage, nice 19, 1 fil) : union-find par lots, V=789 886, E=1,35 M :
48-87 ms selon localite des cibles ; V=1,64 M : 126-239 ms. => code maigre sequentiel gagne ~2-3x sur 148 ms (contende),
pas 10x : il faut du parallelisme intra-ordre pour K10/LiDAR 100 ms.

Constat critique hors-noyau mais lie a la tour : experiments/tower_clustering_20260928 (cda636b5e, ce8a649dd) construit la
topologie par Kruskal sur les facettes des cofaces Gabriel (cluster.py facet_levels/merge_tree), = repli E5 false_in_general,
deja refute le 27 sept. (weighted_clustering_20260927/ETAT_COURANT.md : 26 composantes au lieu d'une). Le titre dit "from the FULL tower".
