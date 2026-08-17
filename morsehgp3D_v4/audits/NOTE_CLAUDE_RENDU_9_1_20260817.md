# Note de Claude — le rendu § 9.1 est fermé côté objet symbolique

Date : 17 août 2026. Le volet « rendu » de votre audit `5a08ab6` est
exécuté, sur le payload `ComponentDelta` et la frontière `PointId`
corrigés dans le cycle précédent. Reçu :
`receipts/forest_20260817/ADDENDUM_RENDU_9_1_20260817.md`.
**77 portes vertes.**

## Ce que vous voudrez vérifier

1. **Vos deux portes exigées sont gravées telles quelles** :
   `render_keeps_batch_born_facets` (carré K=3 : les quatre triangles,
   mutant active-only tué — rendu vide à la naissance) et
   `plateau_render_multiplicity` (carré K=2 : chaque côté ET chaque
   diagonale reçoit exactement 2 incidences des quatre triangles
   rectangles ; mutant collapse-mult tué — la signature d'une
   compression par arbre couvrant, que vous déclariez exacte pour
   `F_K^conn` et fausse pour le § 9.1).
2. **La table de naissance** suit votre prescription : miniboule EXACTE
   des `<= 10` points de la facette, jamais « la facette est un
   événement d'ordre inférieur ». L'argument que j'utilise pour me
   passer du test de convexité : le minimum sur les candidates
   CONTENANTES suffit (la miniboule est candidate ; toute contenante la
   majore), avec paires toutes / triplets strictement aigus seulement /
   quadruplets non coplanaires — les supports rectangles et coplanaires
   cocirculaires sont couverts par une candidate plus petite arité
   (§ 5.6). Un contre-exemple à cette couverture me ferait très mal :
   c'est le point que je vous demande de viser.
3. **Le mutant `birth-from-events`** meurt sur votre carré : côté à
   `rho² = 50` contre première incidence à 100. Le juge recoupe CHAQUE
   naissance par `jminiball` en identité croisée OBig.
4. **Le rendu est comparé sur flux réel** : `same_render` rejoint la
   comparaison par K du probe jugé (le juge paye ses propres événements
   bruts) — 0 désaccord à n=120 sur deux familles.

## Ce que je n'ai PAS fait, à dessein

Les chiffres `S_tau`/`T_x`/`m_tau`/votes : le rendu conserve l'objet
symbolique `facette -> (lot, multiplicité)` dont tout `psi` décroissant
se déduit — `1/t²` restera exact en rationnel, `1/t` exige une racine
(numérique documenté, jamais `exact`). Je préfère câbler ces chiffres
avec la condensation/persistance, où votre table de naissance devient
opérante. Si vous voulez l'ordre inverse, dites-le.

## Prochaine étape

Le pré-filtre de profondeur des boules (98 % des boules uniques meurent
en profondeur APRÈS avoir payé leur census — il faut un minorant de
profondeur par BOULE, pas par ancre), puis l'échelle n = 8000/16000/32000.
