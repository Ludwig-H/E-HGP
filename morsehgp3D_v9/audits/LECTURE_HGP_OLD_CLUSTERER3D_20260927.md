# Lecture de HGP-old et HGP-Clusterer3D

27 septembre 2026. Audit statique des deux répertoires **locaux** ; aucun
paquet historique importé, compilé ou exécuté, aucune mesure nouvelle.
Cette note complète la [relecture du manuscrit](RELECTURE_THESE_ET_HGP_OLD_20260927.md),
sans modifier les sources, expériences ou reçus gelés.
Les deux [licences historiques](../../HGP-old/LICENSE) et
[licence 3D](../../HGP-Clusterer3D/LICENSE) imposent un usage non commercial.
La présente lecture n'autorise aucun transfert de leur code vers la ligne MIT.

## Verdict

`HGP-Clusterer3D` est bien présent dans le dépôt. Son estimateur conserve
**facettes pondérées → condensation → sélection → vote vers les points**,
comme le chemin actif de `HGP-old`. Il n'est donc pas une implémentation
du nouveau dendrogramme ponctuel à masses unitaires. Il ne fournit pas non
plus directement un dendrogramme complet de points à feuilles individuelles.

Les formules communes ne rendent pas les programmes identiques : catalogue
contributif, perturbations géométriques, normalisation, précision, tolérance
du seuil, politique des racines et paramètres de comparaison diffèrent.
Ni leur documentation ni les tests locaux lus ne prouvent une supériorité
universelle d'ARI sur HDBSCAN. Les pertes mesurées doivent rester publiées.

## 1. Chemins réellement actifs

`HGP-old/src/hgp_clusterer/__init__.py:8` exporte la classe de `core.py` ;
`estimator.py` n'est qu'un alias de compatibilité. La voie active est
`core.py:87–92,109–383`, non les anciennes implémentations commentées à partir
de la ligne 386. La condensation appelle effectivement le Cython depuis
[clustering.py](../../HGP-old/src/hgp_clusterer/clustering.py), lignes 82–123.

`HGP-Clusterer3D/src/hgp_clusterer/__init__.py:3` exporte au contraire la
classe de [estimator.py](../../HGP-Clusterer3D/src/hgp_clusterer/estimator.py).
`fit`, lignes 102–175, appelle `build_hypergraph`, Kruskal, `condense_tree`,
puis `_extract_labels`. La sélection vient de
[hierarchy.py](../../HGP-Clusterer3D/src/hgp_clusterer/hierarchy.py).

| Aspect | HGP-old, voie active | HGP-Clusterer3D |
|---|---|---|
| Géométrie usuelle | Conversion float32 ; backend configurable CGAL/Geogram ; autres complexes possibles | Entrée 3D float64 ; Geogram ; perturbations déterministes |
| Ordre et catalogue | Par défaut `min_samples=K+1` ; l'appel Delaunay reçoit `min_samples−1` | K entier 1 à 63 ; simplexes de K+1 sommets à cellule d'ordre K+1 non vide |
| Masse de facette | `S_faces × somme(1/T_points)` ; tableaux largement float32 | Même formule ; S/T en float64, masse de facette stockée float32 |
| Seuil par défaut | `round(sqrt(n_core))`, converti en entier avant condensation | `round(sqrt(n))`, ou réel positif fourni |
| Seuil effectivement comparé | Masse ≥ seuil entier | Masse ≥ seuil × `(1−1e−6)` |
| Exposant par défaut | `expZ=2.0` | `expZ=2.0`, réel strictement positif ≤20 |
| Racine EOM | Sélectionnable | Sélectionnable |
| Labels ponctuels | Vote S après sélection ; bruit sans vote ; options de propagation | Vote S après sélection ; bruit sans vote ; pas d'option 1-NN |
| Hiérarchie exposée | Forêt de facettes ; helper séparé `whole_tree=True` | Forêt condensée de facettes ; `refine_clusters` renvoie des labels plats |

Sources : [core.py](../../HGP-old/src/hgp_clusterer/core.py), lignes 41–80,
123–189, 202–263, 275–383 ; [estimateur 3D](../../HGP-Clusterer3D/src/hgp_clusterer/estimator.py),
lignes 86–173, 205–250 ; [Cython 3D](../../HGP-Clusterer3D/src/hgp_clusterer/_hierarchy.pyx),
lignes 338–397. Cette table décrit des programmes, pas des équivalences certifiées.

## 2. Le catalogue contributif ne se déduit pas du seul mot « HGP »

La voie 3D annonce les ensembles σ de K+1 sites dont la cellule de Voronoï
d'ordre K+1 est non vide : [hypergraph.py](../../HGP-Clusterer3D/src/hgp_clusterer/hypergraph.py),
lignes 56–78. Le C++ part des arêtes Delaunay, itère sur les diagrammes de
puissance, forme les unions de la cardinalité suivante, déduplique, puis
calcule la boule englobante minimale de chaque ensemble :
[_geometry.cpp](../../HGP-Clusterer3D/src/hgp_clusterer/_geometry.cpp),
lignes 151–273 et 296–327. Il n'applique pas à ces sorties un filtre global
« tous les sites de la boule MEB appartiennent à σ ».

Ce catalogue n'est donc pas à identifier au catalogue Gabriel strict du
prototype pondéré v9. Exemple analytique, sans exécution : avec K=1 et les
trois sites `(0,0,0)`, `(4,0,0)`, `(1,1,0)`, la voie `n≤3` retient toutes
les paires. La boule de diamètre reliant les deux premiers contient le
troisième strictement (`2<4` pour les distances au carré au centre).
Cette paire est contributive dans ce catalogue, mais n'est pas Gabriel.
L'inégalité stricte survit à des perturbations suffisamment petites.

Dans `HGP-old`, [hypergraph.py](../../HGP-old/src/hgp_clusterer/hypergraph.py),
lignes 40–85, transmet `min_samples−1` au constructeur d'ordre supérieur,
avec repli Rips sur certaines erreurs. Le graphe dual reçoit ensuite K,
lignes 220–222. Il faut donc figer ces deux paramètres : modifier
`min_samples` n'est pas démontré ici équivalent à régler le voisinage de
HDBSCAN à K constant. Par défaut ils sont raccordés par `min_samples=K+1`.
Les voies Rips/Delaunay ordinaires sont des objets supplémentaires, non
analysés exhaustivement dans cette note.

La préparation 3D normalise les coordonnées puis ajoute un bruit relatif
`1e−8`, graine 42 : `hypergraph.py:18–53`. Les sites barycentriques reçoivent
en outre une perturbation `1e−12` dépendant des IDs : `_geometry.cpp:33,181–194`.
La [documentation 3D](../../HGP-Clusterer3D/docs/ALGORITHME.md), lignes 28–34,
reconnaît des dépendances aux IDs et quelques ensembles indus sur des
configurations difficiles. Sa phrase favorable sur les labels « en pratique »
n'est pas une garantie générale. Les tests géométriques lisibles comparent
des petits exemples à une programmation linéaire et à des boules flottantes :
[test_order_k.py](../../HGP-Clusterer3D/tests/test_order_k.py), lignes 12–49.
Ils ne constituent pas des oracles rationnels FULL sur le nuage u18 original.

## 3. Mesure, exposant et normalisation

Sur le catalogue réellement produit C, les deux voies mettent en œuvre :

$$S_\tau=\sum_{\sigma\in C,\ \tau\subset\sigma}\psi_\sigma,\qquad T_x=\sum_{\tau\ni x}S_\tau,\qquad m_\tau=S_\tau\sum_{x\in\tau}1/T_x.$$

Les scores sont agrégés **avant** Kruskal, non sur les seules cofaces retenues
dans le MST. Réduire la connexité puis calculer les poids sur ce sous-ensemble
changerait la mesure. Sources : [hypergraph old](../../HGP-old/src/hgp_clusterer/hypergraph.py),
lignes 119–128 et 215–224 ; [Cython old](../../HGP-old/src/hgp_clusterer/_cython.pyx),
lignes 838–848 ; [Cython 3D](../../HGP-Clusterer3D/src/hgp_clusterer/_hierarchy.pyx),
lignes 185–221 ; [estimateur 3D](../../HGP-Clusterer3D/src/hgp_clusterer/estimator.py),
lignes 133–159.

Dans old, le niveau transmis est approximativement `r^z` ; le score vaut
son inverse, plafonné à `1e12`. La condensation emploie
`1/(niveau+1e−12)` : `_cython.pyx:439–483`. Dans 3D, h est la médiane des
rayons positifs, le niveau est `r/h` stocké float32, et le score comme la
densité valent `max(niveau,1e−12)^(-z)` : `hypergraph.py:95–106`,
`_hierarchy.pyx:21–25`. Les deux régularisations ne sont pas identiques.

En arithmétique exacte, hors plancher, multiplier tous les S par la même
constante positive h^z laisse m et les vainqueurs des votes inchangés ;
multiplier toutes les densités par cette constante multiplie également les
stabilités sans changer EOM. Ce raisonnement ne prouve pas une identité
bit-à-bit en présence des planchers, arrondis ou catalogues différents.

Changer z affecte les contributions relatives, les masses et les votes,
ainsi que les stabilités. Ce n'est pas la seule transformation λ d'un arbre
ponctuel à masses unitaires. La masse minimale des facettes n'impose pas
une cardinalité minimale aux labels durs **après compétition des votes**.

## 4. Datation, condensation et racines

Dans les deux Cython, toutes les facettes du graphe sont initialisées comme
atomes disjoints avec leur masse. Le parcours porte sur les arêtes MST,
non sur un calendrier indépendant des naissances MEB des facettes :
old `_cython.pyx:456–483`, 3D `_hierarchy.pyx:344–356`.
`join_r` date l'entrée d'une facette dans un **cluster atteignant le seuil**,
pas nécessairement sa naissance géométrique. Les dates old sont déjà
transformées en `r^z`, celles de 3D sont des rayons relatifs `r/h`.

La construction croissante crée un cluster à zéro enfant qualifié,
prolonge le même cluster avec un seul, et crée un parent lorsque plusieurs
clusters se rencontrent. Les racines sont prolongées jusqu'à λ=0 :
old `_cython.pyx:538–638`, 3D `_hierarchy.pyx:397–470`.
Les ex æquo des niveaux float32 sont groupés ; old expose en outre
`epsilon_fusion`, nul par défaut, qui peut grouper des niveaux différents.

EOM conserve le parent en cas d'égalité et **n'exclut pas la racine** :
old `clustering.py:184–228`, 3D `hierarchy.py:20–41`.
Le nouveau comparateur commun avec `allow_single_cluster=False` est un
choix de protocole explicite, pas une reproduction de ce défaut historique.
À petit seuil, ne pas présumer non plus que l'initialisation d'atomes
virtuels reproduit des naissances géométriques positives.

Une preuve éventuelle de préservation des composantes ne suffit pas à
identifier ces dates, ces masses et le FULL exact. Le raccord v9 corrigé
des [attaches silencieuses](../experiments/weighted_clustering_20260927/AUDIT_SILENT_ATTACHMENTS.md)
reste une qualification distincte ; cette lecture ne la transfère à aucun
des deux programmes historiques.

## 5. Vote plat, whole_tree et véritables partitions emboîtées

Le `fit` usuel old appelle `GetClusters` **sans** `whole_tree=True`
(`core.py:289`). Comme 3D, il vote ensuite entre les clusters sélectionnés
par somme des S incidents ; T est commun aux candidats d'un point et
inutile à l'argmax. L'absence de vote laisse `-1`. Old offre en supplément
un remplissage 1-NN optionnel, désactivé par défaut, et une propagation des
points sous-échantillonnés : `core.py:297–383`. 3D ne les expose pas.

Le helper old `GetClusters(..., whole_tree=True)` traite seulement les
**descendants des clusters déjà sélectionnés**, agrège les contributions
directes et descendantes, puis route les points du parent vers un enfant
unique ; les points sans contribution enfant restent au parent :
`clustering.py:418–561`. Toutefois, chaque racine sélectionnée démarre
séparément avec tous ses points. Des racines peuvent partager un même point :
ce helper ne prouve pas une partition globale exclusive de tous les sites.
Il retourne une structure de sous-arbres et plages d'IDs, pas un dendrogramme
complet de n singletons muni de toutes leurs dates MEB (`613–622`).

3D ne possède pas cette option. `forest_` contient des arbres **de facettes**,
et `refine_clusters` produit une nouvelle sélection suivie d'un nouveau vote.
Son callback `splitting` emploie seulement les points portés par les facettes
des feuilles du sous-arbre ; les contributions directement attachées aux
nœuds internes ne participent pas à cet état de décision. C'est visible dans
`hierarchy.py:90–138,195–225` et explicitement décrit dans
`docs/ALGORITHME.md:84–91`. Les facettes directement attachées à un parent
découpé quittent la sélection. Ces raffinements ne sont pas un engagement
global préalable point→branche garantissant l'emboîtement des labels votés.

## 6. Ce que les comparaisons locales permettent de dire

Le [notebook de démonstration 3D](../../HGP-Clusterer3D/notebooks/HGP-Clusterer3D_demo.ipynb),
cellule de code 7 (index zéro du tableau `cells`), oppose HGP K=2, m=50,
z=2 implicite à HDBSCAN sklearn `min_samples=10`, m=50. La cellule 9 explore
K=1,2,3. Ce n'est pas le protocole commun K auto-inclus, même EOM/racine/z.
Les [tests de clustering](../../HGP-Clusterer3D/tests/test_clustering.py),
lignes 12–36, vérifient notamment un ARI >0,95 sur un exemple d'amas bruités ;
ils ne démontrent pas que HDBSCAN ne puisse jamais faire mieux.

La voie publique SIPU K2 examinée séparément par l'autre auditeur reconstruit
au contraire une liaison ponctuelle par première incidence, puis appelle
`hdbscan._tree_to_labels`, sans ces masses de facettes. Elle ne doit pas être
confondue avec les deux chemins locaux ci-dessus ni servir à changer
rétroactivement leur interprétation. Voir la
[note de protocole SIPU](PROTOCOLE_POINTS_THESE_SIPU_20260927.md).

Pour la suite, publier distinctement la variante ponctuelle, les anciens
votes pondérés et HDBSCAN standard/commun ; fixer avant comparaison le
catalogue, les exposants, m, la politique racine et le traitement du bruit.
Les scores des régimes connus guident une exploration, pas une garantie de
domination ni une validation tenue à l'écart du développement.

## Empreintes des chemins lus

Empreintes SHA256 des sources locales, pas reçus d'exécution :

| Source | SHA256 |
|---|---|
| `HGP-old/src/hgp_clusterer/core.py` | `b8d2763b3c51541b4e86d75384a53be115034fc092678f4218fb74e12f88cf0c` |
| `HGP-old/src/hgp_clusterer/clustering.py` | `3cc4488327357b49eac0bcf5ce3acffbd4d6609e0ff618d71ffca46f8fe416ce` |
| `HGP-old/src/hgp_clusterer/hypergraph.py` | `0d1a888229191b9be8405ceaf78e55fcb4fbc02af2aca42f0e3daa534182d60d` |
| `HGP-old/src/hgp_clusterer/_cython.pyx` | `d3ba8b97a0a54d59f280f70db08ae53842c0e661690d8b644fdafc97ac322722` |
| `HGP-Clusterer3D/src/hgp_clusterer/estimator.py` | `b68189fc6827e5121abffb918652e77e9ba4e086aaebd7a319a5671d6b3d25d9` |
| `HGP-Clusterer3D/src/hgp_clusterer/hypergraph.py` | `064d7eb2c8c2287abb10a75ae09740df8f841fb8a7f25f183abf4522aeb5c888` |
| `HGP-Clusterer3D/src/hgp_clusterer/hierarchy.py` | `a366039c5b692069147693e5b141e1f4a3dbfd4e3a9db151663032c3a11ae1be` |
| `HGP-Clusterer3D/src/hgp_clusterer/_hierarchy.pyx` | `321bdba6860e73cd424d1e652c457838f78646b3bc0f7883c7ba1af8fd7f3295` |
| `HGP-Clusterer3D/src/hgp_clusterer/_geometry.cpp` | `60b6fb0cac6b0e03b2e7898d4e54158632f6841e988e4542bae9c70b361ff950` |
