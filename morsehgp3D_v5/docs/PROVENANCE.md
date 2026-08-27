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
| `src/lanes/keys.hpp` | re_derive | `EdgeKey`, `SupportKey3/4`, `BallKey` primitive (« une boule est une boule »), `FacetKey` | oracles q3/q4 (à venir), `mhgp5_level_cmp` |
| `src/lanes/level.hpp` | re_derive | fraction canonique i128, `ExactLevel` U192/i128 non réduit, égalité sémantique par produits croisés | `mhgp5_level_cmp` |
| `src/lanes/edge_cover.hpp` | re_derive | cover d'arête à coefficient (1/3/4), handles par rectangle, 32 seaux | porte appariée du cover (à venir, mutant `cover-rect-dmin`) |
| `src/lanes/q2.hpp` | re_derive | boule diamétrale, forme primitive directe | `mhgp5_q2_fixture` (arrondi plancher du rayon) |
| `src/lanes/q3.hpp` | re_derive | forme de Gram, puissance, descente de profondeur, owner, seed aigu, niveau | `mhgp5_q3_oracle` (oracle rationnel indépendant, fixtures u16 extrêmes, mutants `q3-*`) |
| `src/lanes/q4.hpp` | re_derive | Cramer relatif, orientation canonique, centre strict (un seul volume), owner 6 arêtes, préfiltres i64 et puissance de face, niveau U192 | `mhgp5_q4_oracle` (juge brut C(n,4), mutants `q4-*`) |
| `src/pipeline/float_filter.hpp` | re_derive | séquence FMA figée, borne par seed 2^-48, intervalles de Jung 2^-40, garde d'arrondi, `cmp_2p2_jb2` | `mhgp5_q3_affine` (identité L = 4P, divisibilité, témoin de forte annulation), `mhgp5_float_rounding` |
| `src/parallel/pool.hpp` | neuf | tirage dynamique, fusion en ordre d'index, ouvriers RETOURNÉS | `mhgp5_par_gate` (bit-identique 1 fil / N fils), mutant `parallel-one-worker` |
| `src/pipeline/candidates.hpp` | re_derive | ordre canonique (clé, arité minimale, représentation), RLE | conformité `digest_balls` v4 ; mutant `rle-drop` |
| `src/pipeline/census.hpp` | re_derive | passe count-only (mn >= 0 élague, mx < 0 range-add strict), census I_B/U_B à plafonds | `mhgp5_depth_gate` (mutants `range-add-max-le-zero`, `depth-threshold-minus-one`, `skip-full-census`, `shell-cap-before-depth`) |

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
