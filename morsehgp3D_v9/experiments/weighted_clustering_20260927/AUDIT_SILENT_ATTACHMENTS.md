# Correction : conserver les attaches silencieuses des facettes pondérées

27 septembre 2026. Supplément postérieur à la qualification r1, dont aucun
fichier épinglé n'est modifié. Les 28 cas précédents passent réellement,
mais ne contiennent pas E5 ; ils ne qualifient pas la suppression générale
des connexions non-Gabriel. Ce document corrige cette généralisation et
la lecture trop confiante de la proposition 6 / du théorème 5 du manuscrit.

## Défaut matériel confirmé

Le pilote `spherical_g2_d8_s1`, K5, possède 28 678 cofaces Gabriel et
108 909 facettes. Le graphe brut a 26 composantes finales : une de
108 704 facettes couvre les 1 200 points, les 25 autres couvrent chacune
6 à 9 points déjà contenus dans cette grande couverture. Le FULL natif
a une seule racine. Ces recouvrements ne sont pas des groupes disjoints
à réunir sous une racine artificielle.

La [régression E5 historique](../../../morsehgp3D_v7/audits/receipts_gabriel_20260905/counterfixture_scope.md)
montre le défaut sans grand nuage. À K2, prendre
A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2).
Son catalogue de cofaces Gabriel est exactement ABC, ADE, BCD, BCE, CDE.

| Rayon carré fermé | Čech / FULL | Graphe des seules cofaces Gabriel |
|---|---|---|
| 162/25 | CDE | CDE |
| 189/17 | ACDE | ACDE |
| 33/2 | AC est rattachée à ACDE sans nouveau point couvert | Attache de AC absente |
| 83886/3563 | Une composante ABCDE | Deux composantes ABC et ACDE |
| 24 | Toujours une composante | Fusion artificielle des deux composantes |

L'oracle `Fraction` précédent, exécuté en lecture seule sur ces cinq points,
reproduit ce désaccord. Il ne s'agit donc pas simplement d'un écart flottant,
de cofaces manquantes dans ce petit catalogue, ou d'un problème d'EOM.
Kruskal conserve exactement le mauvais graphe qu'on lui donne : il ne
reconstitue pas une incidence supprimée avant son entrée.

La [contrelecture du 5 septembre](../../../morsehgp3D_v7/audits/receipts_gabriel_20260905/level_proof_review.md)
signalait déjà précisément que la proposition 6 ne pouvait pas servir de
prémisse : l'inertie locale d'une coface ne justifie pas d'effacer définitivement
les rattachements qu'elle crée. Une graduation réduite aux valeurs Gabriel
reste possible **avec** la résolution des facettes vers les bons parents.

## Mesure et connectivité sont deux contrats différents

Le nouveau fichier `full_attachment_oracle.py` conserve sans changement la
mesure du §9.1 : catalogue C de cofaces Gabriel complet, F=∂C, scores Sτ,
dénominateurs T_x et masses mτ. Aucune coface non-Gabriel ne contribue à Sτ.
En revanche, sa **connectivité** est calculée dans tout Čech : tous les
K-sous-ensembles et tous les (K+1)-sous-ensembles du petit nuage.

Les facettes hors F ont une masse nulle pour cette mesure mais peuvent
transmettre des chemins. Elles sont conservées pour calculer les composantes,
puis seulement le résultat est restreint aux facettes positives de F.
Une restriction du graphe aux seuls sommets F avant ce calcul n'est pas
la même opération et n'est pas autorisée par cet oracle.

Les masses sont recalculées par les mêmes équations, sans appeler le graphe
du modèle précédent ; un test différentiel vérifie leur identité avec
`weighted_model.build_facet_model` pour z1 et z2. En z2 elles sont rationnelles
exactes. En z1, seules la géométrie et la topologie sont exactes ; la mesure
reste une référence binary64, sans nouvelle certification des seuils EOM.

## Trois dates à ne pas confondre

Pour une facette τ de F :

- **Naissance** bτ : rayon carré de sa miniball. Avant bτ, elle n'existe pas.
- **Première connexion non triviale** aτ : première coupe fermée où sa
  composante Čech contient au moins deux K-facettes. On a bτ≤aτ.
- **Première incidence Gabriel** gτ : première coface contributrice qui la
  contient. On a aτ≤gτ, mais l'inégalité peut être stricte.

Dans E5, AC a bτ=aτ=33/2, puis gτ=83886/3563. L'attache silencieuse doit
donc transférer sa masse positive à 33/2. Elle ne doit attendre ni le
prochain nouveau point couvert, ni la première coface Gabriel.

Dans le triangle équilatéral entier (0,0,0),(2,2,0),(2,0,2), K2, chaque
arête naît à bτ=2 et reste isolée jusqu'à aτ=gτ=8/3. Il serait incorrect
d'affirmer que toute facette rejoint immédiatement une composante antérieure
non triviale à sa naissance. Son ancre peut être sa propre composante naissante.

Le raccord FULL doit donc fournir pour chaque τ son niveau bτ et sa
composante **fermée à bτ**, puis suivre l'histoire de cette composante.
L'ancre peut avoir une naissance antérieure ; elle ne permet pas d'antidater
la masse de τ. Si le niveau coïncide avec une fusion, il faut normaliser
dans l'état fermé atomique, sans ordre arbitraire des facettes.

## Arbre pondéré de référence et feuilles virtuelles

À chaque niveau exact, l'oracle reconstruit les composantes de Čech par
adjacence/BFS. Il garde les groupes non vides de facettes F qu'elles contiennent.
Une nouvelle facette qui rejoint un groupe existant crée une entrée datée
dans l'arbre pondéré, même sans événement topologique ou ponctuel du FULL.
Les plateaux sont atomiques ; il n'y a aucune racine de fusion inventée.

La borne mτ≤1 demeure vraie. Si m>maxτ mτ, donner un rayon virtuel zéro
aux seules feuilles ne change pas les historiques des composantes admissibles,
car les feuilles isolées sont trop légères. **Cela n'autorise pas à effacer
leurs attaches silencieuses aux composantes lourdes.** Tous les niveaux
internes d'attachement sont conservés par `virtual_tree_for_threshold`.

Cette conversion permet d'utiliser l'EOM pondéré actuel sans modifier son
contrat de feuilles virtuelles, pour les seuils qui satisfont la condition.
Pour m≤maxτ mτ, elle est refusée : il faudrait un EOM tenant compte des
vraies naissances terminales, pas une durée infinie ajoutée implicitement.

## API pure pour qualifier l'adaptateur FULL

```python
from fractions import Fraction
from full_attachment_oracle import build_reference, reference_cut, tree_cut

ref = build_reference(points, K, exp_z=2)
groups = reference_cut(ref, Fraction(num, den), closed=True)
partition_F = sorted(row["facet_ids"] for row in groups if row["facet_ids"])
assert partition_F == tree_cut(ref, Fraction(num, den), closed=True)
```

`ref['facets']` associe un identifiant de feuille à son tuple de PointId.
`children`, `squared_levels` et `roots` donnent l'arbre restreint exact.
`leaf_birth_betas`, `first_nontrivial_betas` et
`first_gabriel_incidence_betas` exposent les trois dates.
`reference_cut` donne aussi toutes les facettes de chaque composante Γ,
sa couverture de points et la masse de ses facettes F.

Pour un export d'attaches `{vertices,beta,node,terminal_ball}`, comparer
à chaque coupe ouverte et fermée les groupes de facettes F actives, pas
seulement leurs unions de points. Il faut également vérifier beta=MEB(τ),
la normalisation fermée des ancres et l'absence de feuille perdue ou doublée.
Cet oracle ne calcule pas `node` ou `terminal_ball` avec le résolveur produit :
il reste indépendant du futur raccord natif.

Le domaine est explicitement borné à douze points. L'énumération exhaustive
et la reconstruction par niveau sont des méthodes de contrôle, jamais une
architecture proposée pour n1200 ou LiDAR. Le cas K=n a une composante Čech
terminale, mais C est vide et la mesure choisie F=∂C est vide ; aucune masse
unitaire de remplacement n'est fabriquée.

Tests purs :

```text
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/test_full_attachment_oracle.py
python3 -B -O morsehgp3D_v9/experiments/weighted_clustering_20260927/test_full_attachment_oracle.py
```

Ils comprennent E5, les deux côtés des attaches, des plateaux carré/octaèdre,
les trois dates, la conservation de la mesure, les coupes directes contre
l'arbre construit, les permutations, translations/dilatations et les limites
de la conversion virtuelle. Aucun moteur natif ni GCP n'est lancé par ces tests.
