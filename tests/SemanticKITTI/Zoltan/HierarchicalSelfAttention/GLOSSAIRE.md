# Glossaire

Tous les termes du dossier, une ou deux lignes chacun. Les définitions mathématiques normatives sont dans [CONTRAT_HGP.md](CONTRAT_HGP.md) et [THEOREMES.md](THEOREMES.md) ; ici, on cherche à comprendre vite.

Trois symboles à ne **jamais** confondre, c'est la confusion la plus fréquente du dossier :

| Symbole | Ce que c'est |
|---|---|
| $K$ | l'**ordre HGP** : le nombre de points qui doivent être simultanément proches |
| $d_{\mathrm{geo}}$ | la **distance géométrique** utilisée pour la filtration |
| $k_{\mathrm{local}}$ | le **budget de voisins du backbone** neuronal, sans rapport avec $K$ |

---

## Clustering hiérarchique : le socle

| Terme | Définition |
|---|---|
| **Single-Linkage** | Relie deux points dès qu'ils sont à distance $\leq 2r$, fait croître $r$, et prend les composantes connexes. C'est HGP à $K=1$. |
| **Effet de chaînage** | Défaut du Single-Linkage en dimension $\geq2$ : une chaîne de points de bruit soude prématurément deux amas distincts. Motivation centrale de la thèse. |
| **DBSCAN** | Single-Linkage restreint aux points « cœurs », ayant assez de voisins dans un rayon fixé ; gère explicitement une frontière et du bruit. |
| **Robust Single-Linkage** | Variante qui n'autorise la liaison qu'entre points de densité suffisante. Moteur de HDBSCAN. |
| **HDBSCAN** | État de l'art usuel : Robust Single-Linkage, condensation de l'arbre par `min_cluster_size`, sélection par excès de masse. |
| **Modèle de Hartigan** | Cadre statistique où les clusters sont les composantes connexes des ensembles de niveau $\lbrace f\geq\lambda\rbrace$ d'une densité $f$. |
| **Estimateur $K$-NN** | Estimation de densité par la distance $r_K(y)$ au $K$-ième plus proche voisin. Plus $r_K$ est petit, plus la densité est élevée. |
| **Amas de forte densité** | Composante connexe d'un ensemble de niveau supérieur. Sa version **discrète** est l'ensemble des points couverts par cette composante. |
| **Filtration** | Famille croissante d'objets indexée par le rayon ou le niveau. Les clusters ne peuvent que croître et fusionner. |
| **Excès de masse** | Critère de sélection de clusters dans une hiérarchie, favorisant ceux qui persistent longtemps avec beaucoup de masse. |
| **Naissance, mort, persistance** | Niveau d'apparition d'un nœud, niveau de sa fusion dans son parent, et leur écart. |

---

## HGP : l'objet propre à cette thèse

| Terme | Définition |
|---|---|
| **HGP-Clusterer** | *Hypergraphe-Percol'*. Généralise le Single-Linkage en faisant percoler des simplexes au lieu de points. |
| **Complexe de Čech** $\check{C}(X,r)$ | Ensemble des sous-ensembles de points dont les boules de rayon $r$ ont une intersection commune non vide. |
| **$(K-1)$-simplexe** | Groupe de $K$ points formant une cellule du complexe. Pour $K=2$, une arête ; pour $K=3$, un triangle. |
| **Facette** | Dans le vocabulaire du dossier, un $(K-1)$-simplexe actif, c'est-à-dire de cardinal $K$ et né avant le niveau considéré. |
| **Coface** | Cellule de cardinal $K+1$ qui **connecte** deux facettes. C'est elle qui porte l'adjacence. |
| **Région témoin** $T_r(\sigma)$ | $\bigcap_{x\in\sigma}\overline{B}(x,r)$ : l'ensemble des centres de boules de rayon $r$ attrapant tous les points de $\sigma$. Convexe compact. |
| **$\Gamma_K$** | Graphe dont les sommets sont les facettes et les arêtes l'adjacence. C'est **exactement** le graphe d'intersection des régions témoins. |
| **$\Gamma_K^{\mathrm{full}}$ / $\Gamma_K^{\mathrm{elem}}$** | Version complète et version restreinte aux cofaces élémentaires. Elles ont les mêmes composantes, **pas** les mêmes arêtes. |
| **$K$-polyèdre** | Ensemble des points apparaissant dans une composante connexe de $\Gamma_K$. C'est le cluster de HGP. |
| **Recouvrement** | Pour $K\geq2$, un même point peut appartenir à plusieurs $K$-polyèdres d'un même niveau. La sortie n'est **pas** une partition. |
| **Clique percolation** | Nom usuel de ce mécanisme dans la littérature des réseaux ; voisin de la $q$-connectivité d'Atkin (1972). |
| **Percolation** | Apparition d'une composante géante sous contraintes locales. Outil d'analyse des performances asymptotiques. |
| **Vitesse de percolation** | Indice de la thèse comparant les algorithmes par la fraction d'un amas récupérable avant fusion parasite. |
| **Mosaïque de Delaunay d'ordre $K$** | Structure géométrique qui porte les simplexes candidats, sans énumérer $\binom{n}{K+1}$ possibilités. |
| **Simplexe de Gabriel** | Simplexe dont la **plus petite boule englobante** ne contient aucun autre point. Condition nécessaire pour être séparant. |
| **Miniball** | Plus petite boule englobante d'un ensemble ; son rayon est le niveau de naissance du simplexe. À ne pas confondre avec la boule circonscrite. |
| **$K$-MST** | Arbre couvrant minimal du graphe de Gabriel, qui contient toute l'information de la hiérarchie. L'objet réellement calculé. |
| **Vietoris–Rips** | Complexe de drapeau du graphe seuil : un simplexe dès que tous ses sommets sont deux à deux proches. Voie de repli parallélisable. |
| **$\alpha_p$** | Constante d'intercalage $\sqrt{2p/(p+1)}$ du théorème de Jung. Vaut $\approx1{,}22$ en dimension 3 : c'est le prix en exactitude de Vietoris–Rips. |

---

## Les quatre carriers, à ne jamais confondre

Un **carrier** est l'objet géométrique qu'un nœud « occupe ». Il y en a quatre, et aucun théorème ne se transfère de l'un à l'autre.

| Notation | Nom | Ce que c'est | Support |
|---|---|---|---|
| $V_v$ | $K$-polyèdre source | l'ensemble des **observations** du nœud | référence |
| $C_v^{F}$ | carrier PL des facettes | $\bigcup_F\mathrm{conv}(F)$ | **égal** à celui de $V_v$ |
| $C_v^{Q}$ | carrier PL des cofaces | $\bigcup_Q\mathrm{conv}(Q)$ | égal si les cofaces couvrent $V_v$ |
| $W_v(a)$ | union témoin canonique | $\bigcup_F T_a(F)$, composante exacte du niveau de densité | **différent** en général |

$W_v$ vit dans l'espace des **centres de boules** : ce n'est ni un polytope, ni une surface LiDAR reconstruite.

| Terme | Définition |
|---|---|
| **`payload_kind`** | Nature des données sérialisées ; `marked_incidence` pour l'objet HGP marqué. |
| **`carrier_kind`** | Lequel des quatre carriers est visé : `source_points`, `facet_pl`, `coface_pl`, `witness_union`. |
| **`authority`** | Force du certificat : `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx` (avec $\varepsilon_W$), `h0_only`. |
| **`cut_policy` / `cut_level` / `cut_side`** | Comment on fige un nœud persistant, qui évolue entre sa naissance et sa fusion. La baseline lit chaque branche juste avant sa fusion au parent. |

---

## Descripteurs de nœud

Détail et démonstrations dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md).

| Terme | Définition |
|---|---|
| **Fonction support** $h_A(u)$ | $\max_{x\in A}\left\langle u,x\right\rangle$ : où se pose le plan d'appui dans la direction $u$. Décrit **exactement** l'enveloppe convexe. |
| **Direction $u$** | Un **vecteur unitaire** de $S^2$, pas un plan. Mais ses lignes de niveau sont des plans : $u$ désigne une famille de plans parallèles, $(u,t)$ en désigne un. |
| **Fonction radiale** $\rho_{\mathrm{out}}$ | Dernière sortie du carrier le long du rayon issu du centre. Décrit l'**enveloppe étoilée**. |
| **Première entrée** $\rho_{\mathrm{in}}$ | Premier contact le long du rayon. Vacue si le centre est dans le carrier ; interprétable comme **surface visible** si le centre est le capteur. |
| **Épaisseur** | $\rho_{\mathrm{out}}-\rho_{\mathrm{in}}$ : ce qui sépare un mur d'un buisson. |
| **Étoilement** | Propriété d'être visible en ligne droite depuis le centre. La reconstruction radiale est exacte **si et seulement si** le carrier est étoilé. |
| **CDF projetée** $F(u,t)$ | Fraction des points dont la projection sur $u$ est $\leq t$. Garde toute la **masse**, pas seulement l'extremum. |
| **Cramér–Wold** | Théorème garantissant que la collection de **toutes** les projections 1-D détermine la mesure. Justifie la complétude asymptotique du canal CDF. |
| **ECT / WECT** | Transformée de caractéristique d'Euler : même filtration directionnelle, mais on résume la **topologie** au lieu de la masse. |
| **Log-sum-exp** | Version adoucie du $\max$, $\beta^{-1}\log\sum e^{\beta\cdot}$. Exactement fusionnable elle aussi, et distribue le gradient sur plus d'un point. |
| **PointNet / DeepSets** | Encodeurs d'ensembles par $\max$-pooling et par somme. Un $\max$ ne peut pas approcher une moyenne : c'est pourquoi il faut un canal de masse. |
| **Grille de Fibonacci / icosphère** | Deux façons d'échantillonner uniformément les directions sur la sphère. |
| **Antipode** | Pour le support, $u$ et $-u$ portent des informations **différentes** ; pour la CDF, ils sont **redondants**. |
| **$\Delta_v(u)$** | $h_{V_v}(u)-h_{W_v(a)}(u)$ : de combien il faut rentrer avant que la condition d'ordre $K$ soit satisfaite. Densité de bord directionnelle, propre à HGP. |

---

## Arbre et opérateur

| Terme | Définition |
|---|---|
| **Arbre de fusion** | Arbre décrivant qui fusionne avec qui, et à quel niveau, le long de la filtration. |
| **Laminaire** | Se dit d'une famille d'ensembles où deux membres sont soit disjoints, soit emboîtés. Condition requise par HSA — et **incompatible** avec le recouvrement pour $K\geq2$. |
| **Laminarisation** | Projection forcée d'un recouvrement vers un arbre laminaire. Supprime exactement ce qui distingue HGP de HDBSCAN. |
| **DAG de recouvrement** | Alternative à la laminarisation : traiter les appartenances multiples comme l'objet. C'est la voie T6. |
| **Ownership $w_{iv}$** | Poids d'appartenance d'un point à une composante, avec $w_{iv}\geq0$ et $\sum_v w_{iv}=1$. Requis pour éviter le double comptage. |
| **Antichaîne** | Ensemble de nœuds deux à deux non emboîtés couvrant tous les points. C'est une « coupe » de la hiérarchie. |
| **Condensation** | Élagage de l'arbre pour supprimer les longues chaînes de fusions ponctuelles. Condition d'existence de HSA sur GPU, pas une optimisation. |
| **HSA** | *Hierarchical Self-Attention*, NeurIPS 2025. Attention dont les scores sont constants par blocs entre sous-arbres frères. |
| **Contrainte de blocs** | Le cœur de HSA : tous les couples entre deux sous-arbres frères partagent un même coefficient. |
| **QC-HSA** | Variante proposée par ce dossier, conservant une ligne d'attention propre à chaque feuille requête. |
| **Embedding positionnel** | Le seul point d'entrée du descripteur dans HSA fidèle : un scalaire de biais par couple de frères. |

---

## Évaluation

| Terme | Définition |
|---|---|
| **mIoU** | Moyenne **non pondérée** des IoU sur les 19 classes. Chaque classe pèse $1/19$, d'où le poids décisif des classes rares. |
| **PQ** | *Panoptic Quality*, métrique de la segmentation panoptique, produit d'un terme de reconnaissance et d'un terme de segmentation. |
| **LSTQ** | Métrique 4D combinant qualité de classification et qualité d'association temporelle. |
| **Séquence 08** | La séquence de validation de SemanticKITTI. Le test (11–21) est caché et son serveur ne doit jamais servir à régler un hyperparamètre. |
| **Mono-scan** | Un seul balayage, sans accumulation temporelle. Régime du dossier. |
| **TTA** | *Test-Time Augmentation* : moyenner les prédictions sur plusieurs transformations à l'inférence. Change de régime, donc de comparabilité. |
| **Oracle de partition** | mIoU obtenu en étiquetant chaque région par sa classe majoritaire. Plafond atteignable avec cette partition. |
| **Pureté** | Fraction des points d'une région appartenant à sa classe majoritaire. |
| **Superpoint** | Région d'une sur-segmentation géométrique, utilisée comme unité de calcul. SPG, SSP, SPT, SuperCluster, EZ-SP en sont la lignée. |
| **ALPINE** | Baseline d'instance sans entraînement dont le clusterer est du Single-Linkage en BEV. C'est HGP à $K=1$ avec un rayon par classe. |
| **Backbone** | Le réseau local qui produit les features de point ; MinkUNet, WaffleIron, PTv3, SphereFormer. |

---

## Statuts et vocabulaire du dépôt

| Terme | Définition |
|---|---|
| **`public_status`** | Statut public d'un résultat. Ici : `not_claimed`. Aucun benchmark ne peut promouvoir vers `exact`. |
| **`exploration_v3_hors_registre`** | Phase de l'exploration v3, sans statut public ni claim produit. |
| **Fixture** | Cas minimal gravé aux coordonnées exactes, conservé de façon permanente dès qu'il a invalidé un claim. |
| **Porte** | Condition d'entrée ou de sortie d'une phase, vérifiée automatiquement. |
| **No-go** | Règle d'arrêt écrite **à l'avance**, qui déclenche un pivot ou l'abandon d'une piste. |
| **Plancher de couverture** | Seuil `--min-*` qui empêche une porte de passer au vert par vacuité. |
| **Mutant** | Défaut injecté volontairement, qu'une porte doit détecter. |
