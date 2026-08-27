# Provenance des modules v5

`AGENTS.md` l'exige : aucun code, mesure, preuve, reçu ou statut v4 n'est hérité
implicitement. Ce document dit, module par module, **ce que la v5 reprend de
la v4, sous quelle forme, comment c'est épinglé et par quelle porte v5 c'est
requalifié**. Trois régimes de provenance :

- `port_contractuel` — données ou algorithmes dont l'égalité bit à bit avec la
  v4 est **le contrat** (familles de nuages, digest de conformité) ; épinglés
  par des digests gravés dans `receipts/conformite_v4/` ;
- `re_derive` — écrit à neuf depuis `docs/MATHEMATIQUES.md` ; la formule
  coïncide avec la v4 parce que l'objet mathématique est le même, pas parce
  que le code a été copié ; requalifié par oracle indépendant et mutants v5 ;
- `neuf` — sans équivalent v4.

Le pin v4 de référence est `main@d4f3ce59` (27 août 2026).

| module v5 | provenance | ce qui est repris | épinglage / requalification |
|---|---|---|---|
| `src/core/types.hpp` | re_derive | largeurs u16 (deltas 17 bits, carrés < 2^34, produits < 2^68), `PointId` u32 stable | `mhgp5_arith_selftest` (extrêmes u16) |
| `src/core/morton.hpp` | re_derive | clé 48 bits, écartement au pas trois | `mhgp5_tree_selftest` (cellules alignées) |
| `src/core/wide.hpp` | re_derive | U192 (produits croisés q3 < 2^171), U320 (q4 < 2^260) | `mhgp5_level_cmp` contre l'oracle 384 bits ; mutant `level-trunc-hi` |
| `src/core/intmath.hpp` | re_derive | `floor_sqrt` / `ceil_sqrt` (vrai plafond), pgcd hybride 128 bits | `mhgp5_arith_selftest` |
| `src/core/mutants.hpp` | neuf | registre unique `MHGP5_MUTANT`, liste exhaustive `kMutants` | `mhgp5_mutants_gate` (grep des points d'injection) |
| `src/core/sha256.hpp` | port_contractuel | SHA-256 de référence (FIPS 180-4) | vecteurs de test FIPS dans `mhgp5_arith_selftest` |
| `src/cloud/families.hpp` | port_contractuel | générateurs v3/v4 **bit à bit** (mêmes PRNG, mêmes tirages, même ordre) | `receipts/conformite_v4/familles_v4.txt` (12 digests calculés par la v4) gravés dans `mhgp5_families_fixture` ; mutant `family-scanline-overshoot` |
| `src/tree/cloud_index.hpp` | re_derive | tri Morton, bucketisation, arbre radix de Karras, boîtes serrées, garde d'entrée | `mhgp5_tree_selftest` (I1–I6, équivariance) |
| `src/wspd/wavefront.hpp` | re_derive | prédicat de séparation entier, vagues, scission du plus grand diamètre, ledger | `mhgp5_wspd_gate` (ledger exact, équivariance, `wspd-drop-rect`, portes appariées `wspd-cap-terminal` / `wspd-split-heaviest`) |
| `src/spindle/spindle.hpp` | re_derive | formes `W_q` (H, Ξ), bornes Hmin/Hmax⁴, boule-cœur (constantes point-fixe sous-approchées à preuves compilables), autorités aux coins | `mhgp5_spindle_gate` (fixtures ponctuelles, boule-cœur ⊆ fuseau, juge fail-open, `core-ball-ceil-distance`) |
| `src/spindle/witness_count.hpp` | re_derive | descente fusionnée trois lanes, masques de lanes, collecte d'ids, juge ponctuel | `mhgp5_spindle_gate` (`witness-no-lane-mask`) |
| `src/lanes/keys.hpp` | re_derive | `EdgeKey` (HD : appelé depuis `anchor_owns_q3` sur device), `SupportKey3/4`, `BallKey` primitive (« une boule est une boule »), `FacetKey` | `mhgp5_q3_oracle`, `mhgp5_q4_oracle`, `mhgp5_level_cmp` |
| `src/lanes/level.hpp` | re_derive | fraction canonique i128, `ExactLevel` U192/i128 non réduit, égalité sémantique par produits croisés | `mhgp5_level_cmp` |
| `src/lanes/edge_cover.hpp` | re_derive | cover d'arête à coefficient (1/3/4), handles par rectangle, 32 seaux | porte appariée du cover (à venir, mutant `cover-rect-dmin`) |
| `src/lanes/q2.hpp` | re_derive | boule diamétrale, forme primitive directe | `mhgp5_q2_fixture` (arrondi plancher du rayon) |
| `src/lanes/q3.hpp` | re_derive | forme de Gram, puissance, descente de profondeur, owner, seed aigu, niveau | `mhgp5_q3_oracle` (oracle rationnel indépendant, fixtures u16 extrêmes, mutants `q3-*`) |
| `src/lanes/q4.hpp` | re_derive | Cramer relatif, orientation canonique, centre strict (un seul volume), owner 6 arêtes, préfiltres i64 et puissance de face, niveau U192 | `mhgp5_q4_oracle` (juge brut C(n,4), mutants `q4-*`) |
| `src/pipeline/float_filter.hpp` | re_derive | séquence FMA figée, borne par seed 2^-48, intervalles de Jung 2^-40, garde d'arrondi, `cmp_2p2_jb2` | `mhgp5_q3_affine` (identité L = 4P, divisibilité, témoin de forte annulation), `mhgp5_float_rounding` |
| `src/parallel/pool.hpp` | neuf | tirage dynamique, fusion en ordre d'index, ouvriers RETOURNÉS | `mhgp5_par_gate` (bit-identique 1 fil / N fils), mutant `parallel-one-worker` |
| `src/parallel/sort.hpp` | neuf | tri stable parallèle (tranches + fusions par rangs, barrière), identique à `std::stable_sort` par contrat | `mhgp5_parallel_sort_gate` (1/2/3/8/13 fils, ex æquo massifs, mutant `parallel-sort-unstable`) |
| `src/core/dint.hpp` | neuf | entier signé 128 bits portable (limbes u64, `__umul64hi` sur device), variantes à limbes de U192 | `mhgp5_dint_gate` (200 000 cas contre `__int128`, bords, mutant `dint-mulhi-dropped`) |
| `src/lanes/device_forms.hpp` | derive_v5 | formes q3/q4 sur `DI128`, largeurs re-dérivées | `mhgp5_dint_gate` (égalité avec les formes de production sur tous les triangles/tétraèdres de petits nuages et fixtures u16 extrêmes) |
| `src/core/sha256.hpp` (SHA-NI) | neuf | chemin accéléré `sha256rnds2` à répartition dynamique, même fonction de compression | `mhgp5_sha256_gate` (vecteurs FIPS, égalité portable/accéléré sur tampons aléatoires et découpages) |
| `src/pipeline/candidates.hpp` | re_derive | ordre canonique (clé, arité minimale, représentation), RLE | conformité `digest_balls` v4 ; mutant `rle-drop` |
| `src/pipeline/census.hpp` | re_derive | passe count-only (mn >= 0 élague, mx < 0 range-add strict), census I_B/U_B à plafonds, singleton = feuille | `mhgp5_api_guard_gate` ; mutants `range-add-max-le-zero`, `depth-threshold-minus-one`, `skip-full-census`, `census-nonstrict` tués par divergence de digest (`mhgp5_pipeline_mutant_*`) et par `mhgp5_forest_judge` |
| `src/pipeline/generate.hpp` | re_derive | les trois lanes (WSPD ternaire, h_a/h_b, cover coef 3, seeds aigus, cœur de seed de Jung, préfiltres q4, filtre de profondeur certifié), parallélisme par rectangle | conformité v4 (`digest_balls`), `mhgp5_q4_cover_fixture` (+ `q4-cover-coef4`), `mhgp5_q4_source_fixture` (+ `q4-seeds-from-q3-live`), `mhgp5_par_gate` (+ `par-drop-shard`), mutants q4 |
| `src/pipeline/expand.hpp` | derive_v5 | préfiltre et census par tranches, comptage par K sans matérialisation, expansion **par ordre K** (régime régulier sans expansion, plateaux filtrés), frontière d'identité | conformité v4, `mhgp5_relabel_gate` (+ `dense-pointid`), `mhgp5_forest_judge`, `mhgp5_api_guard_gate` |
| `src/forest/plateau.hpp` | re_derive | quotient exact des plateaux (Carathéodory : paire diamétrale, triangle fermé, tétraèdre fermé), entier signé 192 bits **de production** (plus l'arithmétique de l'oracle) | `mhgp5_forest_judge` (carré cocyclique K=2/K=3), `mhgp5_render_gate` |
| `src/forest/fold.hpp` | re_derive + derive_v5 | macro-lots par égalité sémantique, canonique min-fid, `ComponentDelta`, garde de capacité ; **v5** : `prepare_fold` (tri parallèle, internement partitionné par empreinte, fusion par rangs) / `reduce_fold` (séquentiel), pipelinés par `run.hpp` | `mhgp5_forest_judge` (+ mutants), conformité v4 (`digest_all`), `mhgp5_par_gate`, `mhgp5_fold_bench` (signature identique, médiane appariée) |
| `src/forest/render.hpp` | re_derive | `F_K^render`, incidences (lot, multiplicité), naissance par miniboule exacte | `mhgp5_render_gate` (+ `render-active-only`, `render-collapse-mult`, `birth-from-events`) |
| `src/pipeline/digest.hpp` | port_contractuel | sérialisation `mhgp4-digest-v1` reproduite à l'identique (tag sans longueur, i128/u64 petit-boutiste, `FacetKey` k + 10 mots) | `receipts/conformite_v4/digests_v4.txt` (19 entrées calculées par la v4) |
| `src/pipeline/run.hpp` | derive_v5 | gardes de bibliothèque, pipeline en bibliothèque, gardes de capacité de tous les K avant publication, streaming par K, callbacks provisoires | `mhgp5_api_guard_gate`, conformité v4, `mhgp5_par_gate` |
| `src/core/device.hpp` | re_derive | macro `MHGP5_HD` (vide hors nvcc) | cibles CUDA sous `MHGP5_ENABLE_CUDA` (ci-dessous) ; statut par étape dans `GPU.md` |
| `src/core/dint.hpp`, `src/lanes/device_forms.hpp` | neuf | DI128 portable (limbes u64), formes q3/q4 device | `mhgp5_dint_gate` (contre `__int128`, mutant `dint-mulhi-dropped`) ; témoin device (0 désaccord sur G4, reçu `campagne_g4_v5_20260827_temoin_device`) |
| `src/lanes/sector_kill.hpp`, `src/lanes/chord_kill.hpp` | neuf (27 août 2026) | tests d'ancre $W_q$ exact + témoins sectoriels, prétests avant cover sur candidats diamétraux par rectangle ; test de seed q4 par morceaux de corde — tous suffisants, objet inchangé (`MATHEMATIQUES.md` § 10) | `mhgp5_anchor_tests_oracle` (ON/OFF sur toutes les paires de petits nuages), fixtures F1–F8, mutants `sector-kill-nonstrict`, `anchor-kill-h-minus-one`, `chord-nonstrict`, conformités v4 ; reçus `mesures_secteurs_635951d6_20260827`, `campagne_g4_v5_20260827_tests_ancre` |
| `src/gpu/q3_scan_shaped.hpp`, `q4_core_shaped.hpp`, `q4_completion_shaped.hpp` | neuf | scans « en forme de kernel » (tableaux plats, DI128) | portes `mhgp5_q3_scan_shaped_*`, `mhgp5_q4_core_shaped_*`, `mhgp5_q4_completion_shaped_*` (égalité par seed / par complétion avec la production, mutants `q3-shaped-strict-flip`, `q4-shaped-jung-skip-kills`, `q4-shaped-once-flip`) — **source présente, exécutée sur CPU** |
| `src/gpu/q3_lane_batched.hpp`, `q4_lane_batched.hpp` | neuf | lanes par lots (étage hôte : lots, validation structurelle, exécuteur paramétrable, émission) | `mhgp5_q3_lane_batched_*`, `mhgp5_q4_lane_batched_*` (post-RLE et compteurs = production ; petit lot ; seuils refusés), `mhgp5_batch_contract`, `mhgp5_parallel_exception` — **CPU** |
| `src/gpu/q3_scan_kernel.cuh`, `q4_kernels.cuh`, `q3_lane_device.cuh`, `q4_lane_device.cuh`, `device_witness.cu`, `cli/mhgp5_cuda.cu` | neuf | kernels, exécuteurs device, témoin, pilote `--gpu` | `nvcc` sur G4 seulement ; **compilés et exécutés conformes** : témoin et lane q3 (reçu `24b3f164`), lane q4 (`2e75cb42`), pilote `--gpu` sur les quatre familles à 50 k avec `digest_all` = CPU et mutant tué sur device (`8f95df2e`) ; **mesurés plus lents que le CPU** (voir `GPU.md`) ; syntaxe vérifiée hors nvcc par un stub CUDA |
| `cli/mhgp5.cpp` | neuf | pilote produit sans mutant (`mhgp5_product_executable`), RSS mesuré | — |
| `oracle/obig.hpp` | neuf | entier signé à limbes **u32** (autre que la production u64 et que la v4), débordement fail-closed, mutant `obig-carry-lost` | `mhgp5_obig_selftest` (contre `__int128`, `cpp_int` si Boost), `mhgp5_level_cmp`, oracles q3/q4, `mhgp5_forest_judge` |

## Ce que la v5 ne reprend PAS

- la sélection axiale bornée (`q4_axial.hpp`, `--axial-on`, sweep à deux
  côtés, `cmp_mu`) : opt-in négatif sur CPU (+7 % de `t_gen`), conservé en v4
  « pour le GPU » — retiré ; voir `PISTES_FERMEES.md` ;
- `build_forest_legacy` et les modes d'internement de banc : le témoin d'un
  fold n'est pas une seconde implémentation figée, c'est l'oracle borné et les
  invariants globaux ;
- les modes de banc (`q4_eq_mode`, `q4_bench_control`, `intern_mode`) enfilés
  dans les signatures de production ;
- le plafond mémoire « pic projeté » et son stopgap : remplacés par une
  comptabilité explicite par rôle (persistant / en construction / temporaire
  / amont) — `docs/ARCHITECTURE.md`.

## Conformité v4 ≡ v5

La porte de conformité est l'égalité des digests canoniques **au format v4**
(`mhgp4-digest-v1:*`, sérialisation reproduite à l'identique dans
`src/pipeline/digest.hpp`) sur les mêmes entrées `(famille, n, s=8, smax=11,
seed=3)`, aux tailles 8000, 16000 et 32000, calculés une fois par la v4 et
gravés dans `receipts/conformite_v4/digests_v4.txt`.
