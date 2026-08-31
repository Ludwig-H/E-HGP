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
- **Mutants causaux** : registre unique (`src/core/mutants.hpp::kMutants`),
  points d'injection `MHGP6_MUTANT("nom")` dans le code de production,
  compilés seulement sous `MHGP6_TESTING` (posé par `mhgp6_executable`,
  jamais par `mhgp6_product_executable`) ; nom inconnu refusé code 2. Cible :
  chaque nom = un point d'injection + une porte code 4 EXÉCUTÉE. État réel
  (recompté au cinquième cycle d'audit) : 60 noms au registre, 64 points
  d'injection (63 sous src/, un sous oracle/) ; **27 noms distincts** tués
  par une porte exécutée (le quatrième cycle annonçait 28 : la porte
  `mhgp6_fused_mutant_droprect` tue `wspd-drop-rect`, déjà compté dans la
  boucle de conformité — deux portes, un seul nom) : mutants dédiés + boucle
  de divergence d'objet `mhgp6_mutant_*` + `family-scanline-overshoot`.
  `wspd-drop-rect` est désormais UNE omission par DESCENTE appliquée après la
  fusion ordonnée, masse omise soustraite du grand-livre reconstruit
  (`emis + tués + omis == attendu`, delta −1 littéral gravé par
  `mhgp6_fused_mutant_droprect` ; un mutant hors déclaration rend 3, jamais
  4). Le reste `[PRÉVU]` avec les portes v5 à porter
  (`fold-inject-b-exception-k3` exige le juge d'in-flight dédié : il termine
  par signal, jamais par la boucle de conformité). Un contrôle textuel
  registre ≡ grep est un complément, jamais un kill.
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

## Portes par étage — état RÉEL au 31 août ; tout ce qui n'est pas dans
`CMakeLists.txt` est `[PRÉVU]`, jamais implicite

| Étage | Portes |
|---|---|
| cœur arithmétique | `mhgp6_arith_selftest` (bornes, U192/U320, DI128 vs __int128 échantillonné), `mhgp6_sha256_selftest` (FIPS + streaming). `[PRÉVU]` : `mhgp6_level_cmp` contre l'oracle 384 bits (mutant `level-trunc-hi`), `mhgp6_dint_gate` complet, porte d'égalité SHA-NI/portable |
| familles | `mhgp6_families_fixture` : déterminisme, profil, unicité, cardinalité (l'égalité bit à bit aux nuages v5 a été vérifiée hors porte à la livraison — 36 configurations). `[PRÉVU]` : digests gravés par famille et mutant `family-scanline-overshoot` raccordé |
| index | `mhgp6_tree_selftest` (structure, boîtes, équivariance) |
| WSPD | ledger exact `Σ émis + Σ tués = C(n,2) − Σ C(mult,2)` ; mutants `wspd-cap-terminal`, `wspd-split-heaviest`, `wspd-drop-rect` ; `--check-permutation` |
| descente fusionnée | `mhgp6_fused_descent_gate` : listes identiques à la triple descente test-only, avec `smax_effective` (cas `collinear_seven` à 9 points gravé) ; mutant `fused-mask-stuck` |
| fuseaux/facteurs | fixtures W2 ⊃ W3 ⊃ W4, boule-cœur ⊆ fuseau ; route M : porte différentielle contre le produit direct (`min(hist, need)` par lane), mutants `endpoint-credit-use-weight`, `factor-none-overclaim` |
| crédits/tape | mutants `credit-compose-sum`, `core-partial-exclude` ; fixture croisée de lanes (W3-pas-W4) ; fixtures rôles A∪B (membre de A complétion valide, seed valide) |
| tueurs d'ancre | fixtures F1–F11 portées ; secteurs : fixture croisée + mutant `sector-credit-global` ; grille : fixtures F9/F10 + mutants `cell-kill-h-minus-one`, `cell-kill-nonstrict` |
| sweep q4 | **oracle du sweep** (re-balayage exhaustif en μ, échange des quantificateurs, racines/frontières) ; fixtures : relais `F1=μ+1, F2=1−μ`, racines confondues, complétion incidente (compte zéro), clip d'égalité à μ*, les trois cas B=0, sortie dans cellule profonde avant portion shallow ; mutants `sweep-drop-exit-root`, `sweep-nonstrict-depth`, `sweep-skip-fragment`, `sweep-completion-from-witness-tape`, `chord-dead-skip-positive` (hérité) |
| RLE/census | mutants `rle-drop`, `depth-threshold-minus-one`, `range-add-max-le-zero`, `census-nonstrict`, `skip-full-census` ; fixtures plateau (carré cocyclique) |
| fold/rendu | mutants `drop-nonmerge`, `attach-prebatch`, `repr-ties`, `binary-ties`, `canonical-is-uf-root`, `fold-inject-a-failure-k2` tués (boucle de divergence d'objet). `[PRÉVU]` : juge borné n ≤ 14 (miniboule + cliques + Kruskal à lots), K=1 ≡ MST indépendant, rejeu « catalogue + deltas → partition », juge d'in-flight (`fold-inject-b-exception-k3`), `render-active-only`, planchers |
| conformité v5 | `mhgp6_conformity_*` : `digest_all` + `digest_forest_K*` (l'OBJET) ≡ `receipts/conformite_v5/` sur 5 familles × {8000, 16000, 32000} (labels scale*) et petites tailles en `gate` ; le digest candidats v5-compat est rapporté, jamais un critère (cover q4 coefficient 4) ; golden post-préfiltre v6 gravé (uniform 400) |
| cover q4 | `mhgp6_cover_coef4` (contre-fixture tétraèdre régulier + z, frontière de génération) + mutant `q4-cover-coef3` |
| barrière de génération/census | `mhgp6_linked_arcs_u16` + mutant d'oracle i64 (portée : génération→census ; l'extension aux facettes de forêt est `[PRÉVU]`) |
| frontières du sweep | `mhgp6_sweep_frontieres` (F1–F5 : racines égales, extrémité de Jung exacte, B=0, complétion dans le facteur, profondeur h4−1) + 2 mutants |
| parallélisme | mutants `par-drop-shard`, `par-drop-ball-chunk` tués (boucle). `[PRÉVU]` : `par_gate` digest identique fils ∈ {1,8}, `parallel-sort-unstable`, `fold_inflight` ∈ {1,2,8} |

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

Le validateur `bench/pentes.py` est prouvé fail-closed par
`tests/pentes_gate.py` (cinquième cycle : nominal + 20 falsifications à
code 3 et stdout vide — dont famille dupliquée du META, entier invalide sans
traceback, compteur dupliqué, digest dupliqué/non hexadécimal, fichier
d'extension inattendue, identités fermantes des octaves violées — + zéro
légitime sur un compteur réellement parsé avec `-` affiché). Le juge de
conformité refuse une référence à clefs de forêt HORS PROFIL (ensemble exact
`{1..kmax_eff}` exigé ; porte `mhgp6_juge_refus_k_en_trop`, K1 correct + K10
en trop à n=2) en plus du narrowing et de la référence tronquée. Le
protocole G4 v6 (`gcp-migration/session_campagne_v6_g4.sh` : conformité
v5≡v6 à 50 000, bench apparié ABBA sans digest, queue stationnaire) a son
selftest transactionnel à faux pilotes (`selftest_campagne_v6.sh`, à lancer
à la main avant toute session payante).
