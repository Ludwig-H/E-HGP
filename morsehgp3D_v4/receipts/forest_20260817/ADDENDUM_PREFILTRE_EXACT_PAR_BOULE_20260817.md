# Addendum — l'audit « préfiltre exact par boule » exécuté : deux passes, seuil par arité, garde d'entrée

Date : 17 août 2026. Exécution intégrale de
`AUDIT_CIBLE_EC683B_PREFILTRE_EXACT_PAR_BOULE_20260817.md`, qui a croisé
mon premier pré-filtre (commit `33fc524`, résultat neutre documenté) et
l'a raffiné sur trois points que je n'avais pas : le seuil PAR ARITÉ, la
séparation stricte des passes avec recoupement, et le garde d'entrée.

## 1. Les deux passes (`prefilter_balls` / `census_balls`)

- **Passe 1 count-only** (`ball_depth_at_least`) : `mn >= 0` élague — y
  compris les régions de coquille pure que le census, lui, doit
  descendre (mon premier jet descendait `mn == 0` pour rien) ; `mx < 0`
  STRICT range-add saturé en O(1) ; feuille = test exact au point ;
  compte EXACT restitué pour toute survivante.
- **Seuil par arité minimale** : `h_qmin = 12 − q_min` — mort à 10/9/8
  intérieurs pour `q_min = 2/3/4` (preuve : tout `T` d'un événement du
  plateau contient un support minimal, donc `|T| >= q_min`, et
  `K <= 10` exige `|I_B| <= 11 − q_min`). Le premier candidat du groupe
  RLE porte `q_min` (le tri met l'arité avant la représentation). La
  règle est JUGÉE, pas supposée : le juge brut garde ses boules au
  plafond uniforme et les expanse — un label `q_min` faux (complétude de
  lane violée) donnerait des événements que le sujet n'a pas.
- **Passe 2** : census complet `I_B`/`U_B` sur les seules survivantes,
  plafond `11 − q_min`, avec RECOUPEMENT `passe1 == passe2` (invariant,
  code 3 ; sous mutant, l'invariant qui se déclenche EST la mise à
  mort — convention du selftest).

## 2. Mesures séparées (n=400 uniform, exigence § 3 de l'audit)

| poste | avant audit | après |
|---|---|---|
| mortes en profondeur | 7 451 476 | 7 493 839 (seuils 9/8 : +42 k tuées plus tôt) |
| clés au census complet | 146 305 | 103 942 |
| census_int / census_shell | 890 798 / 505 564 | 524 454 / 345 806 |
| t_census complet | 30 926 ms | **557 ms** |
| t_prefiltre (count-only) | — | 26 763 ms |
| t_flux (génération + tri RLE) | ~16 600 ms | 17 259 ms |
| désaccords jugés (n=120, 2 familles) | 0 | 0 |

Lecture honnête : la passe 2 s'effondre (le census ne paye plus jamais
une boule morte), la passe 1 PORTE le coût des 7,5 M de boules mortes
(~3,6 µs/boule) — gain net census+préfiltre ≈ 12 %. `prefiltre_feuilles
= 0` : les décisions boîte (`mn >= 0` / `mx < 0`) concluent toujours
avant la feuille. La séparation des temps fait exactement ce que l'audit
voulait : elle EMPÊCHE le tri et la passe 1 de se cacher derrière la
chute du census, et redésigne le vrai poste — le NOMBRE de candidats du
générateur q4 (6,86 M pour ~104 k événements), question déjà posée
(`QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md`, la pré-clé axiale
comme filtre de candidats).

## 3. Portes et mutants (§ 4 de l'audit, tous en place)

```text
depth_gate (0)      : mort EXACTE au seuil par arite SANS census —
  q2 : 10 interieurs -> morte a h=10 ; jumelle a 9 survit, compte 9 ;
  q3 : circonscrite du triangle aigu (plan z=10), 9 -> morte a h=9 ;
       jumelle a 8 survit, compte 8 ;
  q4 : tetraedre regulier (coins alternes du cube 10..30, centre
       (20,20,20), R²=300), 8 -> morte a h=8 ; jumelle a 7 survit ;
  coquille : 2 points SUR la sphere jamais comptes (survit a 9).
threshold-minus-one (4)      : les jumelles meurent a tort (depth gate).
range-add-max-le-zero (4×2)  : coquilles comptees — depth gate ET flux
  juge (l'invariant de recoupement passe1 != passe2 se declenche).
skip-full-census (4)         : la passe 1 ne connait pas U_B et ne
  remplace jamais la passe 2 (evenements manquants, juge).
```

Anecdote de fixture méritée : la première version du triangle q3 vivait
dans le plan `z = 0` avec des intérieurs à `z = −4..−1` — refusée par
MON PROPRE garde u16 fraîchement posé. Le garde fonctionne.

## 4. Garde d'entrée (§ 5 de l'audit)

`build_cloud_index(InputPoint)` refuse AVANT toute construction
(`invalid_input`, index vide) : `PointId` dupliqués (FacetKey non
injectives sinon) et coordonnées hors `[0, 65535]` (`morton_spread3`
masque à seize bits — une coordonnée hors profil serait placée dans une
cellule qui ne la contient pas et invaliderait les élagages). Portes
`mhgp4_probe_guard_dup_id` et `mhgp4_probe_guard_coord_range` (code 2),
négatif et au-delà de 65535 couverts.

**84 portes CTest vertes.**
