# Croissance du census q3 natif sur une arête

Source : [matrice close](matrix/matrix_wnpily8o/COMPLETION.json),
36 commandes Release, 24 payloads appariés égaux. Lectures et douze
corruptions du lecteur : [READBACK.json](READBACK.json).

## Ce qui est réellement mesuré

Une arête fixe et toutes les graines du sous-arbre racine du même index.
Les sites sont exactement représentables en float32. La colonne a x=z=0
hors endpoints ; `slab` fait varier x/z périodiquement dans une section
étroite. Dans les deux cas, y augmente de 1/4 à chaque site ajouté.
Ce sont donc deux nuages allongés avec des préfixes imbriqués, pas une
représentation des scènes LiDAR ni un test d'un domaine spatial fixe.

Kmax=5 produit quatre supports et 12 IDs de coquille ; Kmax=10 en produit
neuf et 27 IDs, dans toutes ces observations. Cette sortie reste constante
quand n croît. Les modes et grains donnent les mêmes profondeurs et digests.
Le contrôle exhaustif indépendant porte sur les petites scènes de la porte,
pas sur tous les grands nuages. `column` a en plus un oracle analytique.

Le travail principal est la somme de trois registres disjoints :
`shared_witness_visits + count_node_visits + shell_node_visits`.
Ne pas ne compter que le suffixe après relais. Les préparations de bornes,
leurs requêtes, leurs évaluations paraboliques, l'arithmétique exacte,
la construction de l'index et les capacités restent publiées séparément.
On n'additionne pas arbitrairement des types d'opérations de coûts différents.

## Doublements 8k→16k→32k

Douze séries (deux régimes × deux K × trois configurations), soit
24 doublements. Le lecteur publie 122 postes par doublement, y compris
les pics/capacités — qui ne sont pas des opérations CPU.
La comparaison au seuil quadratique est faite sur les comptes entiers.

| Poste | Maximum sur les 24 doublements | Valeurs donnant le maximum |
|---|---:|---|
| Visites géométriques totales | ×2,116563 | 136 141 → 288 151, Individual K5, 8k→16k |
| Préparations de bornes totales | ×2,000250 | 7 998 → 15 998, Individual, 8k→16k |
| Requêtes de bornes totales | ×2,141703 | 112 122 → 240 132, Individual K5, 8k→16k |
| Évaluations paraboliques totales | ×2,141703 | 2 018 196 → 4 322 376, même cas |
| Comparaisons des tris de l'index | ×2,217511 | 798 211 → 1 770 042, slab, 16k→32k |

**Aucun poste ne fait ×4 ou davantage** sur ces 24 doublements ; aucun
passage zéro→non-zéro n'y apparaît. Le maximum de tous les ratios définis
parmi les 122 postes est celui des comparaisons d'index ci-dessus.
Cela établit une croissance sous-quadratique **observée dans cette matrice**,
pas une borne générale du census ni du nombre global de graines/arêtes.

| Configuration | Maximum visites totales | Maximum préparations | Maximum requêtes de bornes |
|---|---:|---:|---:|
| Individual | ×2,116563 | ×2,000250 | ×2,141703 |
| SharedPrefix, grain1 | ×1,061776 | ×1,051282 | ×1,070485 |
| SharedPrefix, grain8 | ×1,057658 | ×1,040000 | ×1,071429 |

Exemple exact K10 :

| Régime/mode | Visites 8k/16k/32k | Préparations 8k/16k/32k | Requêtes de bornes 8k/16k/32k |
|---|---|---|---|
| column, Individual | 144 372 / 304 397 / 640 422 | 7 998 / 15 998 / 31 998 | 120 319 / 256 344 / 544 369 |
| column, Shared1 | 537 / 569 / 601 | 49 / 51 / 53 | 465 / 497 / 529 |
| column, Shared8 | 555 / 587 / 619 | 50 / 52 / 54 | 465 / 497 / 529 |
| slab, Individual | 144 372 / 304 397 / 640 422 | 7 998 / 15 998 / 31 998 | 120 319 / 256 344 / 544 369 |
| slab, Shared1 | 547 / 579 / 611 | 51 / 53 / 55 | 469 / 501 / 533 |
| slab, Shared8 | 555 / 587 / 619 | 50 / 52 / 54 | 465 / 497 / 529 |

Le partage réduit donc réellement le nombre de préparations et de visites
dans ces deux régimes. L'index reste payé en O(n log n) ; son coût devient
dominant dans les voies partagées. Rien ne prouve que les mêmes préfixes
rejetteront efficacement les blocs d'une trame SemanticKITTI réelle.

## Temps mono-thread Kmax=10

Millisecondes du **census seul**, callback de digest inclus, préparation
de l'index séparée. Une seule observation, hôte partagé non isolé :
des travaux d'audit concurrents étaient signalés. Aucun gain stable de
latence ou contrat G4 ne se déduit de ces chronos.

Précision après audit B `f7b220c4` : la charge ne venait pas seulement de
l'auditeur. Treize des36chronos chevauchent aussi la capture de mutations
du constructeur ; les qualifications Release/Sanitize étaient également
lancées pendant la campagne. Les comptes restent ceux des reçus ; ces
temps ne permettent pas de choisir une configuration performante stable.

| Régime/mode | 8k | 16k | 32k |
|---|---:|---:|---:|
| column, Individual | 324,759 | 845,079 | 1 501,299 |
| column, Shared1 | 1,180 | 1,292 | 1,344 |
| column, Shared8 | 1,250 | 1,510 | 1,399 |
| slab, Individual | 375,274 | 776,654 | 1 640,404 |
| slab, Shared1 | 1,254 | 1,330 | 1,393 |
| slab, Shared8 | 1,286 | 1,345 | 1,421 |

Pour éviter de cacher la préparation, voici **index + census** des mêmes
observations ; la génération synthétique d'entrée est hors ces deux chronos.

| Régime/mode | 8k | 16k | 32k |
|---|---:|---:|---:|
| column, Individual | 327,764 | 853,145 | 1 514,784 |
| column, Shared1 | 4,877 | 7,659 | 14,534 |
| column, Shared8 | 4,232 | 7,851 | 14,588 |
| slab, Individual | 379,765 | 788,666 | 1 662,769 |
| slab, Shared1 | 5,841 | 10,951 | 22,033 |
| slab, Shared8 | 5,807 | 10,912 | 22,386 |

Les 36 lignes brutes conservent les temps K5 également. Leur fluctuation
illustre la non-isolation ; choisir un grain gagnant sur ces seuls chronos
n'est pas justifié. Le grain règle le partage, jamais un plafond de recherche.

## Limites et suite

Le résultat utile est un census q3 exact partagé, exercé contre oracle et
mesuré avec préparation payée. Il manque toujours l'accès natif global
aux arêtes/graines, leur propriété/canonisation, les autres voies et la tour.
Les primitives ne fournissent ici ni intérieurs matérialisés, ni catalogue
dédupliqué, ni parallélisation du census. Aucune trame SemanticKITTI entière,
aucun GPU et aucun contrat de tour n'ont été exécutés dans cette matrice.
GCP non utilisé.

Le [plan du prochain raccord](../../docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md)
impose des sorties croissant avec n et la couverture des branches à l'échelle,
notamment les rejets extérieurs partagés absents de cette matrice. Les tests
de correction variés ne remplacent pas cette couverture de performance.
