# Une hiérarchie de points depuis T_K : implémentation et benchmarks

27 septembre 2026 — prototype CPU, hors registre, moteur v9 inchangé.

## Conclusion pratique

**Je retiens la première couverture comme première implémentation légère.**
Elle construit de vraies partitions emboîtées à partir du seul ordre K,
avec des décisions de rattachement sur les niveaux rationnels exacts.
Elle n'a besoin ni des autres ordres, ni des poids de toutes les facettes,
ni d'un nouveau graphe de distances entre tous les points.

Le vote cohérent alternatif est implémenté et testé, mais n'apporte pas
de bénéfice convaincant sur ce corpus pour son coût supplémentaire.
Cela ne démontre **ni l'optimalité de la première couverture, ni une
supériorité générale sur HDBSCAN** : sur la partie évaluation 3D, HDBSCAN
gagne en qualité des clusters extraits, malgré une meilleure pureté des
hiérarchies HGP. Ce désaccord est un résultat important, pas un détail.

Pour expZ, **1 reste le défaut recommandé pour cette expérience**. Passer
à 2 ne corrige pas les cas difficiles et augmente certaines sur-segmentations.
La voie mathématiquement plus canonique `C∩X`, décrite plus bas, reste une
proposition distincte non implémentée : ne pas lui attribuer les scores ici.

## 1. L'algorithme, simplement

Une composante HGP d'ordre K porte un ensemble de points, mais plusieurs
composantes peuvent porter le même point. Il faut résoudre ces recouvrements
avant de parler de partitions.

Pour chaque point :

1. Trouver le plus petit rayon où il apparaît dans une couverture de T_K.
2. Le rattacher à cette branche. Si plusieurs branches sont exactement à
   égalité, attendre leur premier ancêtre commun, sans priorité entre IDs.
3. Garder ce rattachement définitivement et suivre les fusions de la branche.

On greffe ensuite les points dans l'arbre, **à leur vraie date d'entrée**.
Les points non encore entrés restent des singletons séparés ; ils ne sont
pas réunis dans un faux « cluster bruit ». Une fois cette greffe faite,
couper l'arbre donne forcément des partitions emboîtées : les points ne
peuvent plus changer de branche à une autre coupe.

Le rayon de première couverture alpha_K(x) est compris entre d_K(x)/2
et d_K(x), où d_K compte le point lui-même parmi les K voisins. On cherche
ici une boule contenant x et K sites, avec centre libre. Cette relation
donne une interprétation de densité locale à la règle ; elle n'est pas une
preuve de qualité statistique. Le retard au plus proche ancêtre commun
en cas d'égalité peut dépasser ce rayon et est compté séparément.

La variante `entry_vote` fait voter les premières entrées dans des branches
incomparables, avec poids r^-expZ, puis suit un trajet irréversible. Elle
correspond à la variante nommée conceptuellement `coherent_vote` dans
FAIRNESS.md. Ce ne sont **pas** les votes facette/coface du manuscrit : les
informations nécessaires à ces derniers ne sont pas inventées depuis FULL.

Enfin, **le même code de condensation et de sélection EOM** est appliqué à
l'arbre HGP de points et à l'arbre fourni par HDBSCAN.

## 2. Une comparaison effectivement commune

Douze scènes entières, 8 364 points au total :

- FCPS 3D : Hepta (212), Tetra (400), Atom (800), Chainlink (1 000).
- Six scènes synthétiques 3D de 900 points : densités variables,
  anneaux entrelacés, deux groupes reliés par un pont avec bruit ; deux
  graines fixées par famille.
- SIPU Flame (240) et Spiral (312) : **jeux 2D plongés dans z=0**, jamais
  comptés comme vrais benchmarks 3D.

Sources : [FCPS, dépôt du mainteneur](https://github.com/Mthrun/FCPS) et
[SIPU, University of Eastern Finland](https://cs.uef.fi/sipu/datasets/).
Les provenances, hashes, conventions et transformations sont dans
[DATASETS.md](DATASETS.md). Les données ne sont pas redistribuées.

Même quantification isotrope u18 pour les deux méthodes, sans standardisation
par axe. Zéro point supprimé, doublon, collision ou fusion dans les douze cas.
Le pas dépend de l'unité de chaque jeu : ce n'est pas une mesure LiDAR à 1 mm.

Grille fixée avant lecture des scores : K=2,5,10 ; taille minimale=20,50 ;
expZ=1,2. Cas principal préannoncé : **K=5, taille minimale=20, expZ=1**.
Les classes de référence ne servent pas à choisir les attaches ou les paramètres.

HDBSCAN sklearn 1.9.1 reçoit `min_samples=K`, point lui-même inclus ; EOM,
distance euclidienne, alpha=1, epsilon=0, masses unitaires, racine non sélectionnée.
Voir la [convention officielle de min_samples](https://scikit-learn.org/stable/modules/generated/sklearn.cluster.HDBSCAN.html)
et la [définition EOM](https://hdbscan.readthedocs.io/en/latest/how_hdbscan_works.html).
expZ=1 utilise lambda=1/r ; expZ=2 utilise lambda=1/r² **pour les deux arbres**.
Ce dernier bras HDBSCAN est une ablation de sélection, pas le HDBSCAN standard.

Les multifusions simultanées sont traitées de la même façon. Le HDBSCAN standard
est aussi conservé séparément : notre rejeu de son arbre binaire retrouve ses
labels dans **72/72 fits**. L'atomisation commune change **16/72** partitions,
dont certaines à taille minimale 20/50 ; tous ces écarts sont publiés.

## 3. Résultats principaux, sans choisir le meilleur réglage par scène

ARI = accord avec les classes de référence, 1 étant parfait. Couverture =
proportion de points non rejetés. Le tableau utilise EOM commun, K5, taille20,
expZ1. Les six synthétiques restent six lignes dans le fichier détaillé ;
seules les quatre réalisations parfaites sont regroupées ici pour la lecture.

| Scène | ARI HGP première couverture | ARI HDBSCAN commun | Couverture HGP / HDBSCAN |
|---|---:|---:|---:|
| Hepta, 3D | 1,0000 | 1,0000 | 100 / 100 % |
| Tetra, 3D | 1,0000 | 0,7887 | 100 / 85,75 % |
| Atom, 3D | 0,5955 | 1,0000 | 85,75 / 100 % |
| Chainlink, 3D | 1,0000 | 1,0000 | 100 / 100 % |
| Densités variables, 2 réalisations 3D | 1,0000 | 1,0000 | 100 / 100 % |
| Anneaux entrelacés, 2 réalisations 3D | 1,0000 | 1,0000 | 100 / 100 % |
| Pont + bruit, développement 3D | 0,8447 | 0,6883 | 86,44 / 77,00 % |
| Pont + bruit, évaluation 3D | 0,7280 | 0,5827 | 79,22 / 74,44 % |
| Flame, SIPU **2D** | 0,9387 | 0,5862 | 99,17 / 77,08 % |
| Spiral, SIPU **2D** | 0,6215 | 0,9854 | 76,28 / 99,04 % |

### Agrégats sur les dix scènes réellement 3D

Moyennes par scène, pas par point, à K5/taille20. La pureté du dendrogramme
mesure si les classes restent séparables dans l'arbre, indépendamment d'EOM.
Elle ignore le bruit vérité ; ce n'est donc pas un score de détection du bruit.

| Arbre / sélection | ARI moyen | Couverture moyenne | Pureté du dendrogramme |
|---|---:|---:|---:|
| HGP première couverture, expZ1 | 0,9168 | 95,14 % | 0,9977 |
| HDBSCAN, EOM commun expZ1 | 0,9060 | 93,72 % | 0,9609 |
| HDBSCAN standard | 0,9061 | 93,78 % | 0,9609 |
| HGP première couverture, expZ2 | 0,8718 | 94,48 % | 0,9977 |
| HDBSCAN, EOM commun expZ2 | 0,8614 | 90,98 % | 0,9609 |

**Ne pas lire la petite avance agrégée comme une victoire générale.** Sur les
cinq scènes 3D réservées à l'évaluation, l'ARI moyen est **0,8647 pour HGP
contre 0,9165 pour HDBSCAN commun**. Sur développement, le sens s'inverse :
0,9689 contre 0,8954. Les jeux publics ne sont pas aveugles, et deux graines
d'une même famille ne sont pas deux populations indépendantes.

Autre piège : l'ARI ordinaire traite tous les labels -1 comme un bloc ; rejeter
une classe entière peut donner un très bon ARI. Le fichier publie donc aussi
l'ARI sur vrais inliers, **chaque point rejeté étant un singleton distinct**.
Sur les dix scènes 3D, ce score donne 0,9319 contre 0,9289 ; sur les cinq
scènes d'évaluation, **0,8732 contre 0,9427**. La réserve demeure donc intacte.

La qualité de l'arbre est néanmoins prometteuse : sur la partie évaluation 3D,
pureté 0,9958 contre 0,9718. Cela ne garantit ni un rayon global donnant les
classes, ni que la règle EOM choisira l'antichaîne attendue.

### Ce que montrent précisément les échecs

- **Atom** : l'arbre HGP a une pureté de 1, mais EOM choisit sept groupes au
  lieu de deux. Les classes sont séparables par une antichaîne ; le résultat
  EOM sur-segmente. Il ne suffit donc pas d'accuser le rattachement ou d'ajouter
  une nouvelle géométrie. expZ2 ne corrige pas ce cas dans le profil principal.
- **Chainlink** : expZ1 donne deux groupes parfaits ; expZ2 en choisit onze,
  ARI 0,5498, sans changer l'arbre première couverture. L'exposant est une
  vraie décision statistique, pas une optimisation neutre.
- **Spiral** : pureté HGP 0,7352 contre 0,9883. Ici l'écart existe déjà dans
  l'arbre projeté à K5 ; il ne relève pas seulement d'EOM. Les six points
  retardés pour égalité ne suffisent pas, à eux seuls, à expliquer causalement
  tout l'écart. Aucune correction opportuniste n'a été appliquée après coup.

Le vote d'entrées donne un ARI moyen 3D de 0,9162, contre 0,9168 pour la
première couverture, à expZ1. La projection Python est environ **4,83 fois
plus coûteuse** en ratio médian apparié sur les douze scènes K5. Un seul
passage : indication exploratoire, pas campagne de performance. Je ne porte
donc pas ce vote comme défaut industriel à ce stade.

Tous les autres K, tailles et exposants sont dans [scores.csv](results_r1/scores.csv)
et [macro_scores.csv](results_r1/macro_scores.csv). K2 peut gagner sur ce corpus,
K10 a une meilleure pureté moyenne mais pas forcément un meilleur EOM : aucun
de ces réglages n'est substitué rétrospectivement au profil principal.

## 4. Vérifications, coûts et portée réelle

La campagne close représente **36 exports natifs, 72 fits HDBSCAN, 504 lignes
de résultats, zéro échec**, environ 115,5 s au total sur CPU local partagé.
Le total inclut plusieurs projections, évaluations et écritures : ce n'est
pas le temps d'une seule exécution utilisatrice.

Neuf commandes de qualification passent, avec sorties conservées :

- Gate natif : 13 cas, 289 replays de couvertures ouvertes/fermées,
  196 paires de composantes partageant des points, une continuation.
- Données : 12 tests, normal et -O.
- EOM : 10 gates et 80 fits par mode, normal et -O.
- Projection : 2 696 contrôles exacts de coupes, 37 refus, 13 cas natifs,
  524 contrôles natifs dont 67 bornes exactes alpha_K/k-NN, normal et -O.
- Métriques : 16 tests et un oracle indépendant sur 132 arbres par mode.

La [contrelecture indépendante](REVIEW.md) recalcule les504scores et puretés,
rejoue144projections/432EOM/72fits et vérifie captures/hashes, sans incohérence.

Le build CPU neuf ferme ses dépendances et hashes avant/après ; son premier
essai R1 a refusé Boost absent avant compilation. R2 enlève cette dépendance
inutile pour l'export décimal U192. Pas de nouvelle qualification sanitizers
du moteur, pas de qualification numérique exacte d'EOM. La précision
géométrique native et la qualité statistique des classes sont distinctes.

À K5, projection première couverture Python : médiane 48,3 ms, min/max
9,3/105,0 ms sur les douze scènes ; EOM médiane 6,7 ms. Lecture/validation JSON
source médiane 231,4 ms, calcul natif médiane 400,4 ms. Les variations du CPU
partagé et le passage unique interdisent d'en faire une comparaison de vitesse
optimisée avec la bibliothèque HDBSCAN ; les détails de chaque phase restent
publiés. Le natif calcule encore 1..K, bien que seul T_K soit consommé.

**GCP/GPU non utilisés ; aucun nouveau contrat 100 ms, LiDAR ou de croissance
8k/16k/32k n'est acquis.** Les petits benchmarks servent à choisir la sémantique
du clustering, pas à qualifier le moteur massif.

## 5. Ce qu'il faut garder, et la meilleure piste distincte à étudier

À garder dès maintenant : couverture **datée**, rattachement irréversible,
singletons avant activation, multifusions exactes, même EOM de référence et
tests de non-régression contre HDBSCAN. Ne pas réintroduire les sommes sur
toutes les facettes pour ce premier projecteur.

Le traitement première couverture se prête à une réduction parallèle par
PointId : minimum du rang de niveau exact, puis résolution des égalités par
ancêtre commun et greffe. La sortie de points contient au plus n-1 nœuds
internes. Une version native peut éviter le JSON et développer moins de
populations intermédiaires. Ce port n'est pas réalisé par ce prototype.
Les bornes en nombre de nœuds H et incidences M sont dans [le protocole](README.md) :
elles ne prouvent pas que H ou M croissent sous-quadratiquement avec n.

### Alternative canonique : les points eux-mêmes dans les régions de densité

Soit C une composante de L_K(r), ensemble des **centres** dont la boule de
rayon r contient au moins K sites. Les ensembles **C∩X** sont déjà disjoints.
Leur suivi par fusions donne donc une hiérarchie de points sans vote ni
arbitrage de couvertures. Un point x devient actif exactement à d_K(x).

Raccord proposé, **non implémenté dans cette campagne** :

1. Calculer d_K(x) et choisir une seule facette A_x de K plus proches sites,
   x compris ; au plus n facettes choisies, pas tous les K-ensembles.
2. Localiser cette facette dans T_K à la coupe **fermée** d_K(x)².
3. Conserver seulement `(PointId, d_K², nœud source)` puis greffer comme ici.

Les ex-aequo entre voisins n'introduisent pas d'ambiguïté : toutes les lentilles
de centres admissibles contiennent x, donc toutes ces facettes appartiennent
à la même composante à cette coupe. Cette projection est mathématiquement
plus canonique que la première couverture, mais c'est une **autre sémantique**.

Le FULL exporté actuellement ne fournit pas cette localisation : être couvert
par delta_r(C) ne prouve jamais appartenir à C. Le moteur possède une descente
MEB/intrus vers une ancre, mais son résolveur privé traite des parents stricts
avec `MEB < before`. Un raccord de points doit accepter `MEB <= d_K` et fermer
le plateau, sans epsilon. Trois points alignés, K3, point central exercent déjà
l'égalité. Il faut temporairement le propriétaire/index, la recherche de
boules par clé et la correspondance boule terminale/ancre ; les n attaches
certifiées suffisent ensuite comme supplément durable.
Points de départ du code : `Builder::static_terminal` dans
`src/tower/forest/full_ball_tower.hpp`, puis `full_coverage_root_at` dans
`src/tower/forest/full_coverage_certificate.hpp`. La première fonction ne
constitue pas encore une API qualifiée de localisation de points.

Cette voie mérite une deuxième expérience séparée, avec oracle de localisation
et mesure du travail total. Elle n'est pas encore testée, et n'assure ni
meilleurs clusters, ni coût sous-quadratique de résolution, ni équivalence
avec HDBSCAN. Ne pas remplacer le prototype mesuré par une promesse.

## Rejouer et examiner les preuves

[Protocole figé](README.md), [conventions de comparaison](FAIRNESS.md),
[provenances](DATASETS.md), [archive des résultats](results_r1/archive.json),
[reçu complet](results_r1/benchmark_receipt.json),
[commandes de qualification](results_r1/checks_receipt.json).
Les exports complets et données restent aux chemins privés indiqués, non versionnés.
Les reçus publics sont des archives de cette expérience, pas un oracle géométrique
autonome de toutes les scènes ; le moteur conserve son statut antérieur.

```sh
python -B build_native.py --build /CHEMIN/NEUF/BUILD
python -B run_checks.py --native /CHEMIN/BUILD/native_export --output /CHEMIN/NEUF/CHECKS
python -B benchmark.py --manifest /CHEMIN/DATA/manifest.json --native /CHEMIN/BUILD/native_export --output /CHEMIN/NEUF/RUN --workers 4
python -B summarize.py --run /CHEMIN/RUN --checks /CHEMIN/CHECKS --build /CHEMIN/BUILD --output /CHEMIN/NEUF/PUBLIC
```

Préparer les données via `datasets.py` après récupération des sources épinglées,
avec le lecteur R privé décrit dans DATASETS.md. Les commandes n'appellent pas
GCP. La première capture partielle dix cas, les téléchargements SIPU expirés
et l'échec du premier build restent conservés en privé, non promus en réussites.
