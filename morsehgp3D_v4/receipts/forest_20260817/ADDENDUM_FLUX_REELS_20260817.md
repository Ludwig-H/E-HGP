# Addendum — la forêt sur flux réels : WSPD → SpherePlateau → fold

Date : 17 août 2026. Le raccord déclaré par les deux audits de forêt
(« raccorder seulement ensuite les flux WSPD réels au fold ») est fait,
avec l'ABI `SpherePlateau` d'échelle de l'audit bloquant.

## La chaîne (`src/pipeline/ball_stream.hpp` + `bench/forest_probe.cpp`)

1. **Générateurs WSPD** : les trois lanes (vagues ternaires, histogrammes
   `h_a/h_b` 8 coins, seeds/complétions) n'émettent plus d'événements —
   seulement des CANDIDATS de boule (BallKey primitive + niveau).
2. **Lemme de complétude sous les seuils** (`derive_v4`, gravé en tête du
   header) : un plateau pertinent (`∃ σ, |σ| <= 11`) de support minimal
   d'arité q a `|I_B| <= K_max + 1 − q`, donc au plus `h_q − 1` témoins de
   fuseau : l'ancre de son support minimal SURVIT toujours aux filtres —
   les seuils `h_q = s_max − q + 1` du profil sont exactement calibrés.
3. **Sort/RLE par BallKey inter-lanes** ; représentant de niveau canonique
   par boule : générateur d'arité minimale puis plus petite représentation
   (sujet et juge appliquent la même règle).
4. **UN census exact par clé** : prédicat UNIFORME
   `P(z) = A|z|² + B·z + C` depuis la forme primitive (largeurs < 2^106,
   i128), descente d'arbre séparable par axe, collectant `I_B` (plafond 9 :
   au-delà, aucun K <= 10) ET `U_B` COMPLET (plafond explicite,
   `resource_exhausted` au-delà — jamais de troncature).
5. **Expansion des plateaux** (partagée avec le selftest via
   `expand_plateau`) puis **fold par K** (macro-lots, rôles § 5.2).

## Le juge (`--judge`, borné n <= 120)

La même sémantique refaite depuis l'énumération BRUTE aux prédicats de
production (toutes paires / triangles aigus / tétraèdres centrés) avec
census brut point à point : il juge d'un coup la COMPLÉTUDE WSPD, le
census d'arbre et le RLE. Comparaison par K : nombre d'événements, lots,
multiensemble des nœuds, attachements nés au lot, partition finale.

## Mesures

| configuration | boules uniques | événements | nœuds | désaccords |
|---|---|---|---|---|
| uniform n=120 (jugé) | 439 283 | 20 670 | 13 118 | **0** |
| eight_clusters n=120 (jugé) | — | — | — | **0** |
| uniform n=400 (smoke) | 7 597 781 | 104 802 | 67 029 | — |

À n=400 : flux 19,5 s, census 32,0 s, fold 1,6 s. Deux compteurs qui
décideront des prochaines optimisations : **98 % des boules uniques
meurent en profondeur** (7 451 476 / 7 597 781 — le census paie pour des
boules que la génération pourrait pré-filtrer), et le générateur q4
domine le flux (6,86 M candidats sur 7,6 M). `census_shell = 505 564` :
les plateaux u16 sont partout, le quotient n'était pas optionnel.

Mutants : `rle-drop` (dédupe sauté → événements dupliqués, comptes
faux) et `census-nonstrict` (coquille comptée intérieure) — tués via le
juge (code 4). Mémoire : les instantanés par lot restent réservés aux
petits n (`O(lots × facettes)`) ; le probe compare la partition FINALE
(`O(facettes)`, désormais dans `ForestResult`) plus les multiensembles
de nœuds par lot — la première version a été tuée par l'OOM à n=120,
leçon retenue.

**71 portes CTest vertes.** Restent, dans l'ordre : le pré-filtre de
profondeur des boules (les compteurs ci-dessus le désignent), le rendu
§ 9.1 (`F_K^render`, poids), puis l'échelle n = 8000/16000/32000.
