# Plan de tests v6

Conventions héritées v5, toutes reconduites :

- Portes à code de sortie **exact** via `cmake/run_expect.cmake`
  (`mhgp6_gate`) : 0 conforme, 1 désaccord du juge, 2 refus avant calcul,
  3 plancher/invariant violé, 4 mutant tué. Crash par signal refusé partout.
- Labels : `gate` (défaut), `oracle` (petits n, vérité établie),
  `scale8000/16000/32000` (les seules tailles où une conclusion de coût
  s'énonce), `slow`.
- **Planchers `--min-*`** sur toute porte contre le vert-par-vacuité
  (violation = code 3).
- **Mutants causaux** : registre unique exhaustif
  (`src/core/mutants.hpp::kMutants`), points d'injection
  `MHGP6_MUTANT("nom")` dans le code de production, compilés seulement sous
  `MHGP6_TESTING` (posé par `mhgp6_executable`, jamais par
  `mhgp6_product_executable`) ; nom inconnu refusé code 2 ; chaque nom = un
  point d'injection + une porte code 4 ; `tests/mutants_gate.py` vérifie
  registre ≡ grep ≡ portes.
- Équivariance par permutation physique et par réétiquetage (`PointId` ≠
  index dense ≠ rang Morton, mutant `dense-pointid`).
- Sortie **bit-identique** quel que soit le nombre de fils (fils ∈ {1, 8, max})
  et `fold_inflight` ∈ {1, 2, 8} ; ouvriers mesurés, jamais déclarés.
- Jamais de vérification exhaustive : les théorèmes s'invoquent, on grave
  leurs fixtures d'égalité ; exception : oracles bornés n ≤ 12–14 qui
  **établissent** la vérité.
- Oracle à arithmétique volontairement autre (`oracle/obig.hpp`, limbes
  32 bits signe-magnitude, échec fermé par drapeau collant) ; le juge du juge
  (`mhgp6_obig_selftest`) contre `__int128` et une reconstruction
  indépendante.

## Portes par étage (liste tenue à jour avec le code)

| Étage | Portes |
|---|---|
| cœur arithmétique | `mhgp6_arith_selftest` (largeurs aux extrêmes u16), `mhgp6_level_cmp` (U192/U320 contre oracle 384 bits, mutant `level-trunc-hi`), `mhgp6_dint_gate`, `mhgp6_sha256_gate` |
| familles | `mhgp6_families_fixture` : digests v5 gravés pour les familles portées ; digests v6 gravés pour les stationnaires ; mutant `family-scanline-overshoot` |
| index | `mhgp6_tree_selftest` (structure, boîtes, équivariance) |
| WSPD | ledger exact `Σ émis + Σ tués = C(n,2) − Σ C(mult,2)` ; mutants `wspd-cap-terminal`, `wspd-split-heaviest`, `wspd-drop-rect` ; `--check-permutation` |
| descente fusionnée | `mhgp6_fused_descent_gate` : listes identiques à la triple descente test-only, avec `smax_effective` (cas `collinear_seven` à 9 points gravé) ; mutant `fused-mask-stuck` |
| fuseaux/facteurs | fixtures W2 ⊃ W3 ⊃ W4, boule-cœur ⊆ fuseau ; route M : porte différentielle contre le produit direct (`min(hist, need)` par lane), mutants `endpoint-credit-use-weight`, `factor-none-overclaim` |
| crédits/tape | mutants `credit-compose-sum`, `core-partial-exclude` ; fixture croisée de lanes (W3-pas-W4) ; fixtures rôles A∪B (membre de A complétion valide, seed valide) |
| tueurs d'ancre | fixtures F1–F11 portées ; secteurs : fixture croisée + mutant `sector-credit-global` ; grille : fixtures F9/F10 + mutants `cell-kill-h-minus-one`, `cell-kill-nonstrict` |
| sweep q4 | **oracle du sweep** (re-balayage exhaustif en μ, échange des quantificateurs, racines/frontières) ; fixtures : relais `F1=μ+1, F2=1−μ`, racines confondues, complétion incidente (compte zéro), clip d'égalité à μ*, les trois cas B=0, sortie dans cellule profonde avant portion shallow ; mutants `sweep-drop-exit-root`, `sweep-nonstrict-depth`, `sweep-skip-fragment`, `sweep-completion-from-witness-tape`, `chord-dead-skip-positive` (hérité) |
| barrière de sortie | `mhgp6_linked_arcs_gate` : littéraux, comptes exacts q3/q4, profondeur zéro, coquille = support, exact-once pré-RLE, équivariance, mutant de troncature i64 de l'oracle |
| RLE/census | mutants `rle-drop`, `depth-threshold-minus-one`, `range-add-max-le-zero`, `census-nonstrict`, `skip-full-census` ; fixtures plateau (carré cocyclique) |
| fold/rendu | juge borné n ≤ 14 (miniboule + cliques + Kruskal à lots) ; K=1 ≡ MST indépendant ; rejeu « catalogue + deltas → partition » ; mutants `drop-nonmerge`, `attach-prebatch`, `repr-ties`, `render-active-only` ; planchers deltas/facettes |
| conformité v5 | `mhgp6_conformity_*` : `digest_all` + `digest_forest_K*` ≡ `receipts/conformite_v5/` sur 5 familles × {8000, 16000, 32000} (labels scale*) et petites tailles rapides en `gate` |
| parallélisme | `par_gate` (digest identique fils ∈ {1,8}), mutants `par-drop-shard`, `parallel-sort-unstable` |

## Fixtures permanentes aux coordonnées exactes

Corpus hérité : carré cocyclique (110/100/90...), `q2_one_interior_attachment`,
croissance unaire, cœur q4 discriminant, « dix témoins q2 qui ne ferment pas
q4 », fixtures q4 13/22 points, skinny 89°–89°–2°, témoin de forte
annulation, contre-familles `two_lines`/`collinear_seven`, F1–F11 des tests
d'ancre, sphère diamétrale à 37 sites. Corpus neuf : fixtures du sweep
(ci-dessus), `linked_arcs_u16`, fixture de masque de lane
`a=(1000,1000,1000), b=(2000,1000,1000), z=(1010,1016,1000)`,
calotte–lentille (V6-Q4), peigne de facteurs singletons.

## Campagnes

Mesures d'échelle : compteurs déterministes (grand-livre), 5 familles
dilatées + 2 stationnaires × {8000, 16000, 32000} × graines {3,4,5} ; pentes
sécantes par terme ; reçus immuables dans `receipts/` (pin, hash de binaire,
sorties brutes). Temps : localement seulement en banc apparié contrebalancé ;
sinon G4 avec reçu.
