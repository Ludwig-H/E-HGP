# Addendum — `ComponentDelta` (naissances/croissances) et frontière `PointId`

Date : 17 août 2026. Exécution conjointe de CINQ audits : les trois
convergents « naissances et croissances de composantes » (`ae9383b`,
`f4abad0`, audit ciblé après `5a08ab6`) et les deux bloquants « le fold
réel perd les `PointId` » (après `e7e4d5e`). Ordre respecté : frontière
d'identité corrigée et gravée AVANT de raccorder quoi que ce soit de
plus au flux réel ; deltas implémentés dans le même cycle.

## 1. `ComponentDelta` : le compteur devient des transitions identifiées

`ForestResult` ne réduisait les lots qu'à des fusions (`ForestNode`) plus
un cardinal global `new_attachments` — les naissances et croissances de
polyèdres étaient perdues (le carré cocyclique K=3 rendait un résultat
VIDE hors compteurs). Désormais (`src/forest/forest.hpp`) :

- `ComponentDelta{batch, level, output, parents, born}` émis pour chaque
  racine post-lot touchée dès que `parents.size() != 1 || !born.empty()` ;
  `0 parent -> naissance`, `1 parent -> croissance`, `>= 2 -> multifusion` ;
- identifiants canoniques déterministes : plus petite `FacetKey` de la
  composante, maintenue incrémentalement à travers les unions ;
- le niveau EXACT est conservé dans le delta (pas seulement l'indice de
  lot) ; `batch_levels` donne le représentant par lot ;
- `ForestNode{batch, absorbed}` est une VUE DÉRIVÉE des deltas à
  `>= 2` parents (vérifié par une porte croisée).

Fixtures gravées (`tests/forest_selftest.cpp`) :

```text
carré cocyclique K=3            : 1 delta, 0 parent, 4 nées (NAISSANCE) ;
q2_one_interior_attachment K=2  : 1 delta, 2 parents, 1 née {0,1} ;
croissance unaire a=(8,10,10), b=(12,10,10), z=(10,11,10), w=(10,13,10) :
  lot de niveau 13/4 = multifusion (3 parents, 2 nées aw/bw),
  lot de niveau 4    = CROISSANCE pure (1 parent, 1 née ab), AUCUN nœud.
```

Le juge du selftest construit ses PROPRES deltas (rôles par rayons de
naissance indépendants, canon/unions propres) et les compare SANS le
champ de niveau — le représentant de niveau d'un plateau n'est pas
re-dérivable indépendamment (support d'arité 4 non unique sur
cosphériques) ; les niveaux de lot sont recoupés séparément en OBig :
`num · cden² == jdist2(ref) · den`. Mutant `drop-nonmerge` (l'ancien
`ForestResult` exactement : partitions justes, naissances/croissances
absentes) : TUÉ (code 4). Le probe compare les deltas complets triés
entre flux WSPD et brut.

## 2. Frontière d'identité : `PointId != index dense != rang Morton`

La faute des deux audits bloquants : `forests_from_balls` castait les
indices denses de `ix.upos` (rangs Morton) en `PointId`. Corrigé :

- `InputPoint{id, position}` (`src/tree/radix_tree.hpp`) : l'API
  d'entrée porte les identités externes ; le tri spatial déplace les
  enregistrements sans réécrire `id` ; la surcharge `vector<P3>` reste
  une commodité de test (`id = index d'entrée`) dont le noyau ne déduit
  rien ;
- `CloudIndex::point_id(u)` : l'UNIQUE accesseur de conversion
  `GeometryIndex -> PointId` (représentant du bucket CSR, univoque sous
  le refus des positions dupliquées) ;
- la conversion a lieu une seule fois, à l'entrée de
  `forests_from_balls`, via la table `pid_of` ; l'ordre de `support`
  reste celui de `T` (aligné sur `active_mask`, jamais retrié
  indépendamment du masque) ;
- le juge du probe reconstruit sa table `geometry_index -> id`
  INDÉPENDAMMENT depuis les enregistrements d'entrée (balayage
  position → id), sans appeler la conversion du sujet.

Porte permanente `--relabel-gate` (nuage uniforme n=36 + carré
cocirculaire ; ids brouillés par bijection u32 non monotone, valeurs
au-delà du bit 31 exercées — plancher dédié) :

```text
run0 : ids A            -> toute clé publique ∈ A ;
run1 : ids pi(A)        -> BallKeys inchangées, événements = pi(run0)
                           point à point, blocs de partition transportés
                           par pi, deltas (lot, |parents|, nées) transportés ;
run2 : permutation physique des couples (id, position)
                        -> sortie BIT-IDENTIQUE à run0.
```

Nuance d'équivariance documentée (§ 5.5) : les représentants canoniques
sont des minima de `FacetKey` — équivariants par BLOCS, pas point à
point (un minimum ne commute pas avec une bijection non monotone) ; la
porte compare donc les partitions par blocs transportés et les identités
nées point à point. Mesures de la porte :

```text
n=40, 2845 boules, q2=509 q3=1305 q4=595, attach=29 (plateaux),
naissances=1, fusions=1594, nées=10599, violations=0.
```

Mutant `dense-pointid` (les deux casts de l'ancien code) : 2430
violations, TUÉ (code 4). Détail attendu de sa signature : les clés
publiques restent figées dans `0..n-1` au lieu de suivre `pi`, et
échouent au test d'appartenance à `A`.

Conflit de nom levé au passage : les `InputPoint` locaux des oracles
q3/q4 sont renommés `OraclePoint` — leur indépendance de représentation
vis-à-vis de la production (règle du juge) est désormais aussi lexicale.

## 3. État des portes

**74 portes CTest vertes** (72 + `mhgp4_forest_probe_relabel_gate` +
`mhgp4_forest_probe_mutant_dense_pointid`), dont les six mutants de
forêt (`binary-ties`, `repr-ties`, `drop-shell-plateau`,
`attach-prebatch`, `drop-nonmerge`, `dense-pointid`) tués.

Restent, dans l'ordre des audits : le rendu § 9.1 (`F_K^render` = TOUTES
les facettes y compris nées au lot, multiplicités, cartes verticales)
sur ce payload complet ; le pré-filtre de profondeur des boules (98 %
meurent après census) ; l'échelle n = 8000/16000/32000.
