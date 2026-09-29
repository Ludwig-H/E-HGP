# Audit : découpe « à la WSPD » (plus long côté) pour les boîtes de centres

29 septembre 2026. Question de l'utilisateur : la découpe des boîtes de centres équilibre-t-elle le nombre de
points ? L'idée de la WSPD, couper selon la plus longue composante pour garder des boîtes aussi cubiques que
possible, ferait-elle mieux ? Réponse mesurée avec une sonde hors dépôt, sans patch produit.

```text
phase=exploration_v10_hors_registre (audit de conception)
backend=reference_cpu
mode=sonde_locale_mesuree (codespace EPYC 7763, 1 a 4 fils)
public_status=not_claimed
GCP non utilise
```

## 1. Prémisse corrigée

La méthode actuelle **n'équilibre pas** le nombre de points. Chaque boîte est un cube, coupé en 8 cubes égaux par
ses trois plans médians (octree dyadique). Les boîtes sont donc déjà parfaitement cubiques, et la découpe ne dépend
pas des points. Les points n'interviennent que de trois façons :

- la racine est le plus petit cube de côté puissance de 2 qui contient les sites ;
- la découpe s'arrête quand la liste de la boîte a au plus M sites (16 à K ≤ 6, 24 à K ≤ 10) ;
- une boîte est abandonnée quand l'enveloppe de sa liste ne la touche pas (lemme K), ou celle de la liste parente
  (pré-ignorance de J2).

Une découpe équilibrée en nombre de points serait **moins bonne**. La liste d'une boîte, c'est son contenu plus une
marge de l'ordre du rayon K-NN. À volume égal, le cube minimise cette marge ; une coupe à la médiane allonge les
boîtes dans les zones denses.

## 2. Ce que l'idée WSPD apporte vraiment

L'arbre de la WSPD (« fair split ») fait deux choses :

1. il ajuste chaque boîte à l'enveloppe de ses points avant de la couper ;
2. il la coupe en deux au milieu de son plus long côté.

Transposé aux boîtes de centres :

1. **Ajustement.** La boîte Q du nœud est remplacée par S = Q ∩ [env.lo, env.hi + 1), où env est l'enveloppe de la
   liste de Q (coordonnées entières).
2. **Coupe.** S est coupée en deux au milieu de son plus long côté. Les boîtes ne sont plus cubiques mais restent de
   rapport ≤ 2 environ. La forme D-loc passe à un h par axe : 2hᵢ au lieu de 2h, bornes inchangées.

**Lemme (ajustement exact).** Soit B une boule admise de centre c ∈ Q.

1. Son intérieur pèse au plus K − 1. Le K-ième voisin de c est donc à distance ≥ r(B), et la boule K-NN fermée de c
   contient I ∪ U. La liste de Q contient cette boule (théorème C), donc S* ⊆ U ⊆ liste(Q).
2. c est dans l'intérieur relatif de conv(S*), donc dans l'enveloppe fermée [env.lo, env.hi].
3. Les coordonnées de l'enveloppe sont entières. Donc c ∈ [max(lo, env.lo), min(hi, env.hi + 1)) = S. La borne
   haute doit être env.hi + 1 : des centres tombent exactement sur env.hi (grilles coplanaires).
4. Les enfants pavent S et S contient tous les centres admis de Q : chaque boule reste énumérée exactement une fois.
5. Les listes des enfants sont certifiées à partir de celle de Q, car chaque enfant est inclus dans Q. Le théorème C
   vaut pour toute boîte.

∎

## 3. Contrôles de la sonde

La sonde (`fair_split/sonde_fair_split.diff`, sur le code J2 = HEAD `82fc2a6b5`) :
- ajuste la boîte à l'enveloppe de la liste ;
- coupe en deux au milieu du plus long côté ;
- passe D-loc à un h par axe ;
- porte la limite de stagnation à 3 × 3 niveaux binaires.

Résultats :

- **Différentiel des 10 entrées** : dumps identiques à `568d45297`, à 1 et 4 fils (`fair_split/differentiel_fs.txt`).
  Seul l'arbre change, ce qui est attendu ; le script signale donc « grand livre différent » et « échec » sur les
  contrôles d'arbre. Les nouveaux compteurs sont égaux à 1 et 4 fils.
- **Oracle T2** : 161 contrôles, 0 écart (`fair_split/oracle_fs.txt`).
- **Mutant « borne haute sans +1 »** (`fair_split/mutant_borne_haute.diff`) : **tué** par l'oracle, 120 écarts sur 161
  (`fair_split/oracle_fsm.txt`).
- **Forme de l'arbre, trame 02** (`fair_split/arbre_fs.txt`) :

| K | nœuds | feuilles | ignorés | m moyen | paires | triplets | quadruplets | jugées |
| ---: | --- | --- | --- | --- | --- | --- | --- | --- |
| 5 | 1 225 705 → 734 083 (−40 %) | 710 429 → 323 224 (−55 %) | 362 063 → 43 818 | 11,4 → 14,0 | −29 % | −9 % | +6 % | +13 % |
| 10 | 1 639 729 → 1 065 585 (−35 %) | 1 061 120 → 485 833 (−54 %) | 373 643 → 46 960 | 18,4 → 21,6 | −30 % | −12 % | +4 % | +11 % |

## 4. Mesures (1 fil, alternées, `fair_split/bench_fs.txt`)

| entrée | K | rép. | t_boxes J2 (s) | t_boxes fair split (s) | écart | filtre (Gtic) |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| quart 01 | 5 | 5 | 0,991 | 0,793 | −20 % | 0,699 → 0,460 |
| quart 01 | 10 | 3 | 3,918 | 3,268 | −17 % | 1,773 → 1,280 |
| trame 02 | 5 | 3 | 6,262 | 5,206 | −17 % | 4,150 → 3,010 |
| trame 02 | 10 | 2 | 25,460 | 22,167 | −13 % | 10,283 → 7,835 |
| trame 00 | 5 | 2 | 6,450 | 5,490 | −15 % | 4,094 → 3,036 |
| trame 00 | 10 | 2 | 26,839 | 24,234 | −10 % | 10,281 → 8,040 |

## 5. Verdict

**Oui, la découpe « à la WSPD » est meilleure** : −10 à −20 % sur l'étage des boîtes, en plus de J2, à catalogue
identique. Mais pas pour la raison supposée : les boîtes étaient déjà cubiques. Le gain vient de deux effets.

1. **L'ajustement à l'enveloppe.** Il retire le vide où aucun centre ne peut être, ce qui compte beaucoup en LiDAR,
   où les points sont sur des surfaces : les nœuds ignorés passent de 362 000 à 44 000.
2. **La granularité binaire.** Les feuilles s'arrêtent plus près du seuil M (m moyen 14 au lieu de 11,4) : deux fois
   moins de feuilles, 30 % de tests de paires en moins, quelques quadruplets et jugements en plus.

Pour mémoire : la conception CPU avait mesuré la découpe binaire seule, sans ajustement, à −5 %. L'essentiel vient
donc de l'ajustement.

## 6. Ce qu'il faudrait pour en faire un changement produit

- **Nettoyage du code.**
  - Retirer la pré-ignorance A6 : l'ajustement la subsume, elle ne se déclenche plus.
  - Mettre à jour l'identité du différentiel : pour un arbre binaire, nœuds = 1 + 2 × internes.
  - Réécrire les commentaires sur les « boîtes cubiques ».
- **Décisions à trancher.**
  - Règle de stagnation (dégénérescences cosphériques) : ici 9 niveaux binaires, à justifier ou fixer autrement.
  - Recalibrer M, dont l'optimum peut bouger avec une granularité binaire.
- **Contrôles à faire avant d'adopter.**
  - Les 9 portes (ici, seul l'oracle catalogue a été joué).
  - Le passage à 48 fils sur G4 : la frontière pilotée par la charge devrait s'adapter à des nœuds binaires.
  - Les conceptions GPU « arbre » supposent un octree : à revoir si elles sont reprises.
- **Grand livre.** Les valeurs de `nodes`, `leaves`, `sum_m` et des tests de feuille changent, pas leur sens. La
  comparaison d'arbres avec `568d45297` ne vaut donc plus ; seuls le catalogue et les niveaux restent comparables.

Proposition : un lot séparé (« J2c, arbre ajusté »), avec ces contrôles, avant J3.
