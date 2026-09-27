# Comparaison de hiérarchies de points à ordre K fixé

Ce protocole compare une **projection aval** de `T_K` FULL à HDBSCAN. Il ne
compare ni une tour multi-ordres fusionnée, ni les poids de facettes du chapitre
9 de la thèse. Le moteur géométrique n'est pas modifié. Aucun appel GCP/GPU
n'appartient à `eom.py` ou à ses tests ; leurs fixtures sont synthétiques.

## Contrat commun

- Même tableau de points, mêmes IDs, dimension, unités et prétraitement déclaré.
  Pas de sous-échantillonnage, jitter, suppression de doublons ou normalisation
  différente pour un bras. Les doublons des tests EOM ne qualifient pas l'entrée
  native, qui exige des sites distincts. La représentation u18 exportée reste
  celle réellement mesurée, sans revendication float32 sans perte.
- Un seul `K` par comparaison. La grille du benchmark principal est
  `K = 2, 5, 10` et `min_cluster_size = 20, 50`, lorsque `K <= n` ; `K=1`
  est conservé comme oracle de développement. Aucun choix à partir des labels
  de vérité terrain ; conserver tous les résultats, y compris bruit intégral.
- `min_samples` compte **le point lui-même** : sklearn reçoit `K`, contrib
  recevrait `K-1`. Contrib ne permet pas cette convention pour `K=1` avec son
  minimum admissible de 1 ; ne pas remplacer silencieusement par 1. Aucun
  adaptateur contrib n'est qualifié dans cette capture.
- Masses unitaires des points, même condensation, même EOM,
  `allow_single_cluster=False`, `cluster_selection_epsilon=0`, aucune taille
  maximale de cluster. Une égalité EOM choisit le parent, comme sklearn ; la
  racine n'est jamais sélectionnée. Les IDs de labels ne sont pas sémantiques.
- Primaire : multifusions atomiques, `atomize_ties=True`, pour **les deux**
  arbres. Les subdivisions internes de même rayon sont contractées ; les
  feuilles de points restent présentes. Aucun epsilon ne fusionne deux niveaux
  différents. L'appelant conserve les niveaux rationnels natifs et doit signaler
  des niveaux exacts distincts arrondis au même float : ce module n'en possède
  pas la preuve d'égalité.
- `exp_z=1` : `lambda=1/r`, échelle HDBSCAN standard. `exp_z=2` :
  `lambda=1/r²`, ablation appliquée aux deux arbres. À arbre de points fixé,
  l'exposant ne change ni les enfants, ni les tailles, ni la topologie condensée ;
  il peut changer les stabilités et donc la sélection EOM. À K fixé, les
  constantes communes `K/(n*omega_3)` multiplient tous les scores de la même
  façon et n'en changent pas les décisions hors limites numériques.

Les feuilles ont un rayon nul, les nœuds internes un rayon non négatif. Pour
des points HGP distincts, les fusions pertinentes ont un rayon positif. Les
rayons nuls de doublons/fixtures sont néanmoins définis : `lambda=+inf`, durée
d'un intervalle sans longueur `[inf,inf]` égale à zéro, stabilité infinie d'une
branche née à lambda fini qui subsiste jusqu'à l'infini. On ne calcule jamais
`inf-inf`. Si deux scores infinis s'opposent, le choix du parent est une convention
déclarée et un avertissement est émis. Les infinis retournés doivent être encodés
explicitement lors d'une exportation JSON stricte, jamais comme NaN.

## Projections HGP : choix distincts de la tour source

La première projection retenue par le pilote est `first_coverage` : première
date exacte d'apparition d'un point dans les couvertures datées de `T_K`,
normalisation des composantes à la coupe fermée, puis LCA de toutes les
composantes ex æquo. Le point est attaché définitivement au LCA à une date au
moins égale à sa naissance et à la première couverture. Ce report évite un
arbitrage par ID, mais peut retarder l'entrée du point : ce n'est **ni** l'union
des sites des composantes, **ni** la restriction géométrique `C ∩ X`.
Une fois ces attaches fixées, les sous-arbres définissent des ensembles de
points laminaires ; les contributions à des continuations ne doivent pas être
antidatées à la naissance de leur segment.

La variante `coherent_vote` utilise une antichaîne des premières entrées
indépendantes de couverture, avec poids `r^-exp_z`, puis un routage descendant
irréversible. Ce ne sont **pas** les sommes de poids coface–facette du chapitre
9. Les calculs float64 sont exploratoires ; score égal ou trop proche conduit
à un arrêt conservateur au LCA, pas à un vote indépendant à chaque coupe.
Le traitement des poids à rayon nul doit être déclaré par le projecteur,
sans division `inf/inf`. L'exposant peut changer ici à la fois le routage
**et** l'EOM : distinguer cette ablation de celle de l'EOM seul sur un arbre
figé. Aucune exactitude géométrique ou statistique du vote n'est revendiquée.

Les garanties d'emboîtement proviennent de l'attachement définitif à un arbre,
pas d'un accord avec HDBSCAN, d'un bon score supervisé ou du digest FULL. La
projection ne renforce pas le statut de complétude de sa source.

## Adaptateur HDBSCAN et provenance

Environnement inspecté : scikit-learn **1.9.1**, NumPy **2.5.3**, SciPy
**1.18.1**, Python 3.12. Contrib `hdbscan` n'est pas installé. Aucune installation
n'a été nécessaire. `fit_hdbscan` refuse une version sklearn non inspectée et
vérifie le schéma de son attribut privé `_single_linkage_tree_` :
`left_node, right_node, value, cluster_size`. Ce couplage privé est une limite
explicite, pas une promesse de compatibilité future.

Paramètres : métrique euclidienne, `alpha=1`, `algorithm="kd_tree"`, `n_jobs=1`,
`copy=True`, sélection EOM. L'adaptateur consomme l'arbre produit par la
bibliothèque ; il ne réimplémente ni les distances de mutual reachability ni
le MST. Il accepte uniquement des données denses finies et refuse par défaut
plus de 100 000 points, sans troncature. Ce plafond d'adaptateur, modifiable
explicitement, n'est pas une borne du moteur. Les versions et hashes des
fichiers sklearn Python/Cython chargés sont retournés avec chaque résultat.

Un seul fit suffit par `(points,K,min_cluster_size)` : l'arbre retourné se
rejoue pour `exp_z=1/2`. Toujours conserver :

1. `standard_labels_z1`, labels natifs sklearn ;
2. `common_z1_labels`, labels du post-traitement commun atomique ;
3. `common_z1_matches_standard`, égalité de partition et du masque bruit ;
4. `preserved_tree_z1_matches_standard`, contrôle avec binarisation sklearn
   conservée ;
5. labels et choix EOM de l'ablation commune `exp_z=2`.

Les plateaux sklearn peuvent créer des branches de durée nulle et des clusters
dépendant de leur binarisation. L'atomisation peut donc différer légitimement
des labels natifs ; ne jamais présenter les labels communs modifiés comme
« les labels HDBSCAN standard » ni supprimer les désaccords.

## Portée des tests et limites numériques

`test_eom.py` contient 10 gates, dont 80 fits sklearn sur cinq fixtures
(gaussiennes séparées, uniforme continu, grille entière, doublons, tout
identique), quatre K et quatre tailles minimales `2,5,20,50`.

Le rejeu z=1 conservant l'arbre de la bibliothèque concorde sur **80/80** fits.
L'atomisation diffère sur **7/80** : uniforme continu `(K,m)=(5,2),(5,5),(10,2)` ;
grille entière `(5,2),(5,5),(10,2),(10,5)`. Même des coordonnées sans égalités
métriques apparentes peuvent produire des plateaux de mutual reachability.
Les désaccords sont imprimés, pas assimilés à un défaut de la bibliothèque.
Deux arbres binaires artificiels supplémentaires, plateau positif et plateau
nul, exercent causalement cette différence. Aucun `NaN` de stabilité n'est admis.

Sont aussi vérifiés : condensation des petites branches, refus de la racine,
multifusions et permutations d'enfants, absence de mutation des entrées,
répétabilité, invariance par dilatation globale, changement EOM entre z=1 et
z=2 sur un exemple calculable, entrées malformées et débordements numériques.
Pour `K=1`, le facteur entre rayon HGP `d/2` et distance HDBSCAN `d` est une
dilatation globale : les labels EOM sont inchangés pour z=1 comme z=2 ; ce
contrôle ne qualifie pas les ordres supérieurs.

Le calcul commun est flottant et porte toujours `certification="not_claimed"`.
Les scores proches (64 eps machine relatifs) sont signalés sans transformer
leur ordre ; cette alerte n'est pas un filtre exact. Les lambdas positifs non
représentables et débordements de scores finis sont refusés. Les niveaux natifs
exacts, les dates d'attachement et les avertissements restent séparément
rejouables. Les passes normal et `python -O` utilisent les mêmes contrôles
explicites, aucun `assert` de production.

## Mesures et interprétation

Les univers de scores sont explicitement différents :

- `ari_all` et `nmi_all` utilisent tous les points ; chaque label `-1` forme
  **un bloc**, pas un ensemble de rejets indépendants. Exemple : vérité
  `[0,0,1,1]`, prédiction `[0,0,-1,-1]` donne ARI=NMI=1 malgré une couverture
  de 50 %. Publier obligatoirement `coverage`, `noise_count` et `clusters`
  avec ces scores ; jamais ARI/NMI seuls comme preuve de bonne segmentation.
- `ari_true_inliers` enlève le bruit de vérité, mais conserve les `-1` prédits
  en un bloc. `ari_classified` enlève seulement le bruit prédit, pas le bruit
  de vérité : il peut être flatteur à très faible couverture.
- `ari_inliers_noise_singletons` enlève le bruit de vérité et remplace chaque
  rejet prédit restant par un singleton distinct. Le contre-exemple précédent
  vaut alors `4/7`, pas 1. Ce score complémentaire ne mesure pas la détection
  du bruit et ne dispense pas de publier la couverture ; si les vraies classes
  sont elles-mêmes singletons, des rejets peuvent encore donner ARI=1.
- `noise_precision`, `noise_recall`, `noise_f1` traitent le rejet comme positif,
  seulement si la vérité contient du bruit. Sans bruit vérité ils valent
  `null` ; zéro rejet prédit avec bruit vérité donne précision/rappel/F1 nuls.
  Un univers de moins de deux points donne `null` pour les ARI conditionnels.
- `dendrogram_purity` est la moyenne, sur les paires de **vrais inliers de même
  classe**, de la proportion de cette classe parmi les **vrais inliers** sous
  leur LCA. Le bruit vérité est exclu aussi de la masse du LCA : absorber du
  bruit ne diminue donc pas cette pureté. Les plateaux de même hauteur sont
  atomisés avant calcul ; des subdivisions binaires ne peuvent améliorer ce
  score. S'il n'existe aucune paire admissible, la pureté vaut `null`.

`test_benchmark.py` vérifie ces conventions sur cas triviaux, purs/non purs,
bruit total/partiel, labels permutés et plateaux positifs/nuls. Un oracle
indépendant énumère les paires et remonte les LCA sur 132 petits arbres ; il
ne réutilise pas la programmation dynamique du score produit. Ces tests ne
consomment aucun jeu externe, ne lancent aucun binaire et n'écrivent rien.

Séparer préparation, construction native, export/lecture, projection, fit
HDBSCAN et condensation/EOM. L'export ne publie que `T_K`, mais le producteur
v9 calcule encore les ordres `1..K` : son temps n'est pas celui d'un producteur
optimisé pour le seul `T_K`. Les durées locales partagées ne sont pas des
résultats G4 et ne qualifient pas FULL sous 100 ms. Ne pas confondre stabilité
EOM, stabilité au bruit, qualité de segmentation et preuve d'exactitude.

Commande locale : `python3 -B morsehgp3D_v9/audits/b_point_hierarchy_k_20260927/test_eom.py`
(puis `python3 -B -O` avec le même chemin). Les tests n'écrivent aucun reçu et
n'utilisent aucune étiquette de vérité terrain pour choisir leurs paramètres.
