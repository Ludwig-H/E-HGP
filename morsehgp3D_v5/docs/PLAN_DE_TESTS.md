# MorseHGP3D v5 — Plan de tests

Hérite du plan racine (`docs/TEST_PLAN_MORSEHGP3D.md`, § 3.1–3.2) ; ce fichier
fixe son application v5.

## 1. Tailles, régimes, labels

- **Tailles d'intérêt : n = 8000, 16000, 32000** — les seules où une
  conclusion de coût, de sélectivité, de mémoire ou d'échelle se lit. Labels
  CTest `scale8000`, `scale16000`, `scale32000`. Les petites tailles
  (n ≤ 2000) ont UN rôle : oracle de correction — jamais une pente. Aucun
  « smoke » ne remplace une taille d'intérêt.
- **Familles** : `uniform`, `terrain`, `eight_clusters`,
  `scanline_single_pass`, `scanline_overlap_multiecho` — générateurs bit à
  bit v3/v4 (digests gravés). Contre-familles gravées : `two_lines`,
  `collinear_seven`.
- **Séparation WSPD s = 8** par défaut (6 et 10 en campagne), graine 3,
  `smax = 11` (K_max = 10).
- Labels : `gate` (fixtures, mutants, invariants — rapides), `oracle`
  (petits n, la vérité établie par un juge indépendant), `scaleNNNN`.

## 2. Interdits et obligations

- **Jamais de vérification exhaustive** : ce qu'un théorème garantit est
  invoqué ; on grave ses fixtures d'égalité. Ce qui reste à tester est la
  faute d'implémentation : invariant global, juge d'échantillon, mutant.
  Exception : les oracles bornés (n ≤ 12–14 pour la forêt ; quelques
  centaines de points pour les triangles/tétraèdres) *établissent* la vérité.
- **Planchers de couverture** (`--min-*`) sur toute porte.
- **Mutants tués** (`--inject=<nom>`, code 4) : chaque nom de `kMutants` a un
  point d'injection dans `src/` et une porte à code 4 (`mhgp5_mutants_gate`
  vérifie la première moitié).
- **Codes de sortie exacts** : 0 conforme, 1 désaccord du juge, 2 refus avant
  calcul, 3 plancher/invariant violé, 4 mutant tué ; crash par signal refusé
  (`cmake/run_expect.cmake`).
- **Sortie bit-identique** quel que soit le nombre de fils ; ouvriers
  **mesurés** (retournés par `parallel_*`), jamais déclarés.
- À l'échelle : invariants globaux et juges d'échantillon, jamais un juge
  $O(n^3)$ ni un tableau indexé par paire.

## 3. Portes par étage

| étage | invariant / vérité | porte |
|---|---|---|
| index | clés strictement croissantes ; plages enfants = partition ; boîte ⊆ cellule alignée ; poids ; équivariance ; garde d'entrée | `mhgp5_tree_selftest` |
| familles | digests v4 gravés ; cardinalité ≤ n | `mhgp5_families_fixture` (+ `family-scanline-overshoot`) |
| WSPD | ledger $\sum \vert A \vert \vert B \vert = \binom{n}{2} - \sum_u \binom{m_u}{2}$ exact ; plancher ; équivariance ; portes appariées cap / scission | `mhgp5_wspd_*` |
| fuseaux | $W_2 \supset W_3 \supset W_4$, coquille exclue ; boule-cœur ⊆ fuseau ; compte fusionné ≤ vrai compte pour toute ancre (fail-open) | `mhgp5_spindle_*` |
| q3 | accord total sujet/oracle sur tous les triangles ; fixtures u16 extrêmes ; plancher de limbes | `mhgp5_q3_oracle` |
| q4 | accord sur tous les tétraèdres ; préfiltres sans faux rejet et frontière rejetée ; fixture 13/22 points (découplage des lanes) | `mhgp5_q4_oracle`, `mhgp5_q4_source_fixture` |
| niveaux | U192/U320 contre l'oracle 384 bits ; antisymétrie ; canonicité ; plancher de plateaux ; mot-haut | `mhgp5_level_cmp` |
| arithmétique de l'oracle | le juge du juge : contre `__int128` (et `cpp_int` si présent) ; frontière du débordement | `mhgp5_obig_selftest` |
| forêt | K = 1 ≡ single-linkage ; juge par miniboule indépendante + cliques + Kruskal à lots (n ≤ 14) ; fixtures plateau / attachement / croissance ; **porte de rejeu** « catalogue + deltas → partition » (T5, autorité indépendante du fold : union-find frais sur les fid du catalogue, `output = min(parents ∪ nées)`, naissance unique, parents = canoniques de blocs vivants, partition reconstruite ≡ `final_canon_fid`) sur six familles (dont `two_lines`, `collinear_seven`), planchers 10⁴ deltas / 10⁵ facettes ; mutants `drop-nonmerge`, `attach-prebatch` tués | `mhgp5_forest_judge`, `mhgp5_delta_replay` (+ mutants) |
| parallélisme | bit-identique 1 fil / N fils, ouvriers mesurés ; tri stable parallèle ≡ `std::stable_sort` ; SHA-256 accéléré ≡ portable ; étage B concurrent par ordre : `digest_all` identique pour `fold_inflight` ∈ {1, 2, 3, 8, 16}, domaine [1, 16] refusé sinon (`invalid_input`), callbacks dans l'ordre des K et jamais simultanés (journal sous mutex), **pic mesuré d'ordres en vol** (`peak_fold_inflight` ≥ 2 quand `fold_inflight` ≥ 2, == 1 sinon, entrelacement forcé par l'observateur de phases `on_fold_phase`), premier défaut décidé **à son tour de publication** (un échec d'étage A ou une exception d'un K supérieur n'annule jamais la publication d'un K inférieur), aucune fenêtre `terminate`, fautes injectées `fold-inject-a-failure-k2` / `fold-inject-b-exception-k3` (code 4), observateur terminal `kPublished` appelé **avant** l'ouverture du tour K+1 (une exception de l'observateur de K est la faute de K : K+1 jamais publié — scénario N5), porte compilée avec `-fno-elide-constructors` et rejouée sous TSan | `mhgp5_par_gate`, `mhgp5_parallel_sort_gate`, `mhgp5_sha256_gate`, `mhgp5_api_guard_gate`, `mhgp5_fold_inflight_safety` (+ mutants) |
| coût (instrument) | banc apparié contrebalancé du fold, signature identique exigée, médiane des rapports par paire | `mhgp5_fold_bench` |
| tests d'ancre et de seed | $W_q$ exact, témoins sectoriels, morceaux de corde et **grille de cellules** (théorème 10.5) suffisants : mêmes candidats avec les prétests ON et OFF sur **toutes** les paires $(a,b)$ de petits nuages (grille sur toute ancre : seuil 0) ; $J > 0$ pour tout seed q4 aigu ; identité de signe $P/B$ ; non-vacuité de chaque test (dont seeds tués par cellules en q3 et q4) | `mhgp5_anchor_tests_oracle` (label `oracle`) ; fixtures gravées F1–F11 (`mhgp5_anchor_kill_fixture` ; F9 : ancre au-dessus d'une vallée, W₄ et secteurs impuissants, 172/172 cellules mortes ; F10 : 13 sites entiers de la sphère diamétrale exactement sur la frontière des sommets $i' = 0$ de la grille — tue `cell-kill-nonstrict` ; F11 : centre $v_3$ exactement sur une arête et un coin de cellule, corde q4 dont la boîte traverse une cellule vivante — cellules fermées consultées, tue `cell-locate-eps-zero`), sphère diamétrale (`mhgp5_sector_kill_fixture`) ; **oracle brut du comptage** : `CellGrid::build` (deux pointeurs) ≡ évaluation directe i128 des 289 sommets, cellule par cellule, sur toutes les paires de petits nuages et F9/F10, planchers $\ge 10^5$ comparaisons, $du < 0$, $du = 0$, chemin i128 (`mhgp5_cell_grid_oracle`, labels `oracle gate`) ; mode force-grid (`cell_min_sites = 0` ⟹ grille sur toute ancre, ratio et `near_m` ignorés) dans l'oracle ON/OFF ; mutants `sector-kill-nonstrict`, `anchor-kill-h-minus-one`, `chord-nonstrict`, `cell-kill-nonstrict`, `cell-kill-h-minus-one`, `cell-locate-eps-zero` (code 4) |
| lanes par lots / device (point 2) | vecteur post-RLE et compteurs (7 q3, 22 q4, dont $W_3$/$W_4$/secteurs, cellules : `grids_attempted`/`grids_built`/`grids_all_dead`, seeds tués) identiques à la production sur les chemins hôte, device et surdimensionné ; étage grille **unique** (jeton « grille déjà appliquée ») et **autorité unique** du seuil (`GenerateOptions::cell_grid_min_sites` gouverne `BatchLimits`) ; ordre brut à un fil en routage nul ; bornes dures de lot ; non-vacuité des routes ; contrat structurel des lots ; exceptions jointes puis relancées ; **instrument recevable** : sommes d'exécuteur nommées (`executor_ms_sum`, jamais un mur), étapes device par événements sans barrière nouvelle, octets H2D/D2H, quantiles de lots par classe log2, pic de flux actifs (jauge atomique), cycle de vie des exécuteurs (`executeurs_crees`, `cycle_de_vie_ms_sum`) | `mhgp5_q3/q4_lane_batched_*` (nominal, petit lot, ancre trop grande, routes, mutants `q3-batched-emit-dead`, `q4-batched-emit-deep`, `route-ignore-threshold`), `mhgp5_batch_contract`, `mhgp5_parallel_exception`, `mhgp5_gpu_instrument_*` (mutants `log2hist-class-shift`, `gauge-no-peak`) ; device (label `gpu`, G4 seulement) : `mhgp5_device_witness`, `mhgp5_q3/q4_lane_device_*` |
| préfixe exact (portée K = 5) | ordres $K \le s_{\max} - 1$ d'un run à `smax` = objet complet restreint : `digest_forest[K]`, cardinalités, **événements canoniques** (q, d, masque, support, intérieurs, niveau) et **niveaux de tous les lots** identiques au run `smax = 11` ; plancher d'événements à l'ordre maximal et plancher d'ex æquo (`--min-tie-excess`, excès événements − lots, sur le réseau quantifié déterministe `uniform n = 48, coord = 14`) ; sortie `tower_scope=profile_complete_k10` / `prefix_k<K>` avec `smax_requested` / `smax_effective` ; mutants `anchor-kill-h-minus-one` (h dépend de `smax`), `prefix-tamper-event-order` (ordre des intérieurs : champ omis par le digest v4, vu par la signature des événements) et `prefix-tamper-batch-levels` (niveaux de lots seuls) brisent le préfixe | `mhgp5_prefix_k5_*`, `mhgp5_prefix_k5_ex_aequo`, `mhgp5_prefix_k4_uniform`, `mhgp5_prefix_mutant_h1`, `mhgp5_prefix_mutant_event_order`, `mhgp5_prefix_mutant_batch_levels` (code 4) |
| **conformité v4** | `digest_balls` et `digest_all` (format v4) identiques sur les mêmes entrées — c'est aussi la preuve d'invariance de l'objet des tests d'ancre | `mhgp5_conformity_*` (n=400…2000 : `gate` ; 8000/16000/32000 : `scaleNNNN`) |

## 4. Fixtures permanentes reprises (coordonnées exactes)

Carré cocyclique `(110,100,100), (100,110,100), (90,100,100), (100,90,100)`
(plateau K=2, naissance K=3) ; `q2_one_interior_attachment`
`{(0,0,0), (4,0,0), (2,1,0)}` ; croissance unaire
`{(8,10,10), (12,10,10), (10,11,10), (10,13,10)}` ; « dix témoins q2 dans la
boule diamétrale qui ne ferment pas q4 » (`a=(100,100,100)`, `b=(200,100,100)`,
`x=(150,30,120)`, `y=(150,30,80)`, `z_i=(150+i,140,100)`) ; `collinear_seven` ;
`two_lines` ; arrondi q3 `a=(0,0,0), b=(14,0,0), z=(7,1,4)` et q4
`a=(0,0,0), b=(8,0,0), z=(4,1,2)` ; fixture q4 13 points (ancre q3-morte,
q4-vivante) et sa version 22 points ; cœur q4 exact
`a=(10000,10000,0), b=(20000,10000,0), z=(15000,12585,0)`.

Tests d'ancre (`MATHEMATIQUES.md` § 10 ; ancre `a=(0,0,0)`, `b=(2000,0,0)`,
$h_3 = 9$, translation `+1000` en $y$/$z$) : F1 `(1000+e, ±900, 0)`, $e = 0..8$,
plus le seed `x=(1000,1200,0)` (secteurs tuent, $W_3$ non, aucune boule) ;
F2 idem $e = 0..7$ plus `x` (profondeur 8 : boule émise, perdue par le mutant
$h-1$ via les secteurs) ; F3 `(1000+e, 550, 0)`, $e = 0..8$, plus `x` ($W_3$
tue, secteurs non) ; F4 idem $e = 0..7$ plus `x` (frontière $W_3$ à $h-1$) ;
F5 28 sites sur la sphère diamétrale plus `x` (aucun test ne tue, tout seed
mort : nécessité réfutée) ; F6 `(1000,400,−200)` exactement sur la frontière
du demi-plan du sommet `u` ; F7 secteurs q4 sur F1 ; F8 frontière des morceaux
de corde ($v_j = 0$) ; F9 ancre `a=(800,0,0)`, `b=(2800,0,0)` au-dessus d'une
vallée à fond plat ($h = -600$ pour $\left\vert x - 1800 \right\vert \le 900$, parois
de pente 6, remontée au-delà, pas 40, $y \in [-600, 600]$) ; F10 ancre
`a=(0,0,0)`, `b=(2000,0,0)`, sites `(1000+s, 0, t)` avec $s^2 + t^2 = 10^6$,
$t > 0$ (13 solutions entières) ; sphère diamétrale
`a=(0,0,0)`, `b=(50,0,0)`, 37 sites (frontière diamétrale du mutant non strict).

## 5. Mesures d'échelle et reçus

Campagnes counter-only aux 36 configurations (4 familles × 3 tailles × 3 s),
graine 3 ; « vrai vivant » recalculé par juge d'échantillon. Trois exposants
successifs par arité avant toute phrase sur une pente. Les temps G4 exigent
un reçu (commande, HEAD, hashes, codes, arrêt certifié) ; un OK CPU ou une
extrapolation ne qualifie aucun SLO. Étape suivante : la campagne G4
`gcp-migration/session_campagne_v5_scale_g4.sh` (conformité v4 aux trois
tailles sur la VM, puis contrat 50 000 points mesuré ; porte transactionnelle
`selftest_campagne_v5.sh`), puis les dizaines de millions de points. Phase
optionnelle **`SCALE_THREADS`** (P0 des auditeurs) : matrice contrebalancée
`threads` × `fold_inflight` × `digest` avec répétitions, topologie et
`time -v` gravés ; le validateur ne juge que la complétude, l'égalité des
digests et des compteurs de travail, et produit un résumé sans conclusion de
speedup ; pilote ≤ 24 runs avant toute matrice complète ; le protocole A
(cpuset, `fold_inflight = 1`) mesure un speedup, le protocole B (`inflight`)
un chevauchement — jamais confondus.
