# Addendum — le rendu § 9.1 : `F_K^render`, multiplicités, naissances de facettes

Date : 17 août 2026. Exécution du volet « rendu » de l'audit
« naissances, croissances et rendu » (`5a08ab6`), sur le payload
`ComponentDelta` complet et la frontière `PointId` corrigée.

## L'objet (`src/forest/render.hpp`)

- `build_render(events)` : agrégation exacte
  `facette -> (lot, multiplicité)` — `F_K^render` = TOUTES les facettes
  distinctes de tous les événements, nés au lot compris ; multiplicité =
  CHAQUE K-simplexe incident (le flux de plateaux énumère déjà chaque
  simplexe une fois, `mult_B(tau)` est l'agrégation). Les macro-lots
  sont IDENTIQUES à ceux de la forêt (même tri stable, même égalité
  U320) — vérifié par une porte de cohérence interne. L'objet est
  SYMBOLIQUE : tout `psi` décroissant (`S_tau`, `T_x`, `m_tau`, votes)
  se déduit en aval sans re-parcours.
- `facet_birth_level(points, k)` : MINIBOULE EXACTE de `k <= 10` points
  pour la table `FacetKey -> rho(facette)²` (condensation/persistance).
  Minimum sur les candidates CONTENANTES (fermées), sans test de
  convexité : paires diamétrales toutes ; triplets strictement aigus
  seulement (le support rectangle est couvert par la paire hypoténuse,
  l'obtus n'est jamais support) ; quadruplets non coplanaires (le
  coplanaire cocirculaire est couvert par un triplet aigu ou une
  paire). Arithmétique de production (clés primitives i128, niveaux
  promus Q4Level, largeurs du census < 2^106).

## Le juge

- Selftest : `jrender` accumulé indépendamment depuis les simplexes du
  juge (Déf. 28 pure, `jminiball`), comparé lot par lot, multiplicité
  par multiplicité ; chaque naissance de facette recoupée par
  `jminiball` en identité croisée OBig (`num·cden² == jdist2(ref)·den`).
- Probe (`--judge`) : `build_render` payé des deux côtés (flux WSPD
  contre énumération brute) — `same_render` rejoint la comparaison par K
  (0 désaccord, uniform et eight_clusters n=120).

## Fixtures gravées et mutants (tous tués, code 4)

```text
render_keeps_batch_born_facets : carré K=3 -> F_3^render = 4 triangles
  (tous attachements nés au lot) ; mutant render-active-only : rendu
  VIDE à la naissance -> tué.
plateau_render_multiplicity : carré K=2 -> 6 facettes (4 côtés + 2
  diagonales), chacune EXACTEMENT 2 incidences (4 triangles rectangles),
  12 incidences, un lot ; mutant render-collapse-mult (multiplicités
  écrasées à 1 — la signature d'une compression par arbre couvrant,
  exacte pour F_K^conn, fausse pour S_tau) -> tué.
naissances : côté {0,1} à rho² = 50, diagonale {0,2} à rho² = 100 — PAS
  le niveau d'incidence (100 pour les deux) ; mutant birth-from-events
  -> tué.
```

Mesures selftest : 510 événements, 2561 incidences, 147 plateaux
multiples, 0 désaccord. **77 portes CTest vertes** (74 + 3 mutants de
rendu).

## Ce qui reste déclaré

- `S_tau`/`T_x`/`m_tau`/votes chiffrés : consommateurs du payload
  symbolique — `psi(t) = 1/t²` reste exact en rationnel ; `1/t` exige
  une racine (numérique documenté, jamais `exact`). À câbler avec la
  condensation.
- Table de naissance à l'échelle : exercée ici en régime oracle (le
  selftest la paye sur toutes les facettes de tous les nuages) ; le
  chemin d'échelle viendra avec la condensation/persistance.
- Puis : pré-filtre de profondeur des boules (98 % meurent après
  census), échelle n = 8000/16000/32000.
