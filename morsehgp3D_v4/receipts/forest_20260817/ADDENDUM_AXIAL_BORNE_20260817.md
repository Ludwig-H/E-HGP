# Addendum — sélection axiale bornée : exacte, appariée, et honnêtement négative sur CPU

Date : 17 août 2026. Exécution de la réponse auditeur
`REPONSE_A_CLAUDE_6EDAA43_MINORANT_Q4_ET_AXIAL_BORNE` (convergente avec
les deux autres réponses au minorant) : la sélection axiale SANS tri
complet, par seuils bornés `k = h_4 − p <= 8`, ties de frontière
conservés, une BallKey par groupe exact de `mu`, minimum canonique par
groupe.

## L'implémentation (`ball_stream.hpp`, chemin apparié)

- trois balayages linéaires du cover par seed : permanents `p`
  (`B = 0, A < 0` — intérieurs de toute la famille), seuils bornés par
  côté (tableau fixe trié, jamais un tri complet — la cause du négatif
  CPU d'origine), retenue ties inclus (`<=` — les éliminer selon
  l'ordre mémoire serait faux) ;
- côté `B < 0` normalisé `(−A, −B)` : `mu` INCHANGÉ, sélection
  DESCENDANTE (intérieur ⟺ `mu_z > mu_y`) — la première version
  sélectionnait les k plus petites des deux côtés, la porte appariée
  l'a immédiatement attrapée (2 587 clés manquantes, bogue de
  direction corrigé) ;
- une émission par groupe de `mu` : minimum `ball_candidate_less` des
  membres VALIDES (prédicats de production par membre — jamais
  l'autorité sur un seul représentant) ; le scan de profondeur ne paye
  plus que les représentants.

## Les portes (93 CTest vertes)

`--axial-pair-gate` : baseline énumérée CONTRE axial borné après
tri/RLE — clés, arité et REPRÉSENTATION identiques, sur uniform n=120,
eight_clusters n=120 et la sphère cosphérique R²=50 (84 points de
réseau — le pire cas de plateau). Mutants tués : `axial-short-group`
(k−1), `axial-drop-ties` (< au lieu de <=), `axial-first-rep`
(discriminé PRÉ-RLE : la re-canonicalisation inter-seeds masque le
mutant après RLE — le min global d'une clé revient par un autre seed ;
la porte compare donc les émissions brutes mutant/normal, et l'égalité
appariée hors mutant prouve que le minimum est le bon choix).
`--axial-on` n'est pas un mutant : c'est le chemin opt-in.

## La mesure honnête : NÉGATIF sur CPU

| n=1600 uniform | axial OFF (baseline) | axial ON |
|---|---|---|
| évaluations q4 tuées | 18 767 853 | 796 209 |
| t_gen | 32,5 s | 34,9 s (+7 %) |

Sur la sphère cosphérique la réduction de candidats est massive
(220 934 → 34 942) mais sur les familles réelles la baseline rejette la
plupart des complétions à la LENTILLE (~3 opérations i64) tandis que le
balayage axial paye `A, B` sur TOUS les sites de TOUS les seeds
(~40 ns × sites × seeds) — et les deux postes croissent au même rythme :
pas de croisement CPU en vue. Même verdict que la sélection axiale
d'origine, pour une raison différente (le tri alors, le balayage
maintenant) — la théorie est exacte, l'économie CPU ne suit pas quand
un filtre par candidat quasi gratuit existe déjà.

## Décision

Production CPU = baseline (filtre cover + W₄, chemin du reçu
`ADDENDUM_FILTRE_GENERATION`). Le chemin axial reste OPT-IN
(`--axial-on`), apparié et muté en permanence, comme CANDIDAT GPU : son
travail borné régulier (k <= 8, pas de sorties anticipées divergentes)
est la forme qu'un kernel veut, là où les early-exits de la baseline
divergent par warp. La décision GPU se prendra sur une mesure G4, pas
sur cette extrapolation.
