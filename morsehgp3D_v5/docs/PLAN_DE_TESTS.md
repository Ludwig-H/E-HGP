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
| forêt | K = 1 ≡ single-linkage ; juge par miniboule indépendante + cliques + Kruskal à lots (n ≤ 14) ; fixtures plateau / attachement / croissance | `mhgp5_forest_judge` |
| **conformité v4** | `digest_balls` et `digest_all` (format v4) identiques sur les mêmes entrées | `mhgp5_conformity_*` (n=400…2000 : `gate` ; 8000/16000/32000 : `scaleNNNN`) |

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

## 5. Mesures d'échelle et reçus

Campagnes counter-only aux 36 configurations (4 familles × 3 tailles × 3 s),
graine 3 ; « vrai vivant » recalculé par juge d'échantillon. Trois exposants
successifs par arité avant toute phrase sur une pente. Les temps G4 exigent
un reçu (commande, HEAD, hashes, codes, arrêt certifié) ; un OK CPU ou une
extrapolation ne qualifie aucun SLO. Étapes suivantes : contrats à 50 000
points sur G4 (`gcp-migration/`, scripts gardés), puis dizaines de millions.
