# Addendum — le fold en sort/reduce : les std::map chauds remplacés, −49 % à n=8000

Date : 17 août 2026. Le chantier désigné par les pentes (t_fold ×2,8
par doublement, la plus raide du tableau — std::map) et attendu par les
auditeurs (« pendant que le fold sort/reduce progresse en parallèle »).

## Ce qui change (`forest.hpp`, `render.hpp`) — sorties IDENTIQUES

- `build_forest` : INTERNEMENT GLOBAL PAR TRI — un tri unique des
  enregistrements (facette, événement, slot) attribue les fid en ordre
  de FacetKey et précalcule `first_batch` (« existed » en O(1)) ; les
  rôles par lot passent en TABLEAUX À ÉPOQUE (plus jamais une map par
  facette) ; les lots sont délimités AVANT le fold (mutants
  ties/représentation inchangés) ; les petits maps par racine de lot
  (pre_canon, touched) restent — ils n'ont jamais porté la pente ;
  snapshots (petits n) filtrés par `first_batch` ; partition finale
  construite en ordre croissant (`emplace_hint`).
- `build_render` : l'agrégation map<FacetKey, map<lot, mult>> devient
  un tri d'enregistrements (facette, lot) réduit par plages.
- Aucun changement d'ABI ni de sémantique : mêmes structures de sortie,
  mêmes ordres observables (facettes croissantes, deltas par racine
  croissante), mêmes six mutants de forêt et trois de rendu tués.

## Mesures (uniform, seed 3, smax=11)

| t_fold | avant | après |
|---|---|---|
| n=1600 | 12 813 ms | 7 400 ms (−42 %) |
| n=8000 | 111 973 ms | 56 633 ms (−49 %) |

Événements, fusions, nœuds : IDENTIQUES aux runs d'avant réécriture
(3 126 158 / 19 465 140 / 1 974 086 à n=8000). Selftest : 0 désaccord,
fixtures gravées inchangées. **93 portes CTest vertes.** Le gain croît
avec n (le tri bat l'arbre rouge-noir d'autant plus que F grandit) ; le
layout par tri est en outre la forme GPU-alignée du fold.

Restent au fold : les petits maps par lot (touched/pre_canon) si un
profil futur les désigne, et la parallélisation par K (les dix forêts
sont indépendantes — trivial sur la G4). Tâche ouverte inchangée par
ailleurs : la boule intérieure candidate (si les compteurs de
génération la redésignent).
