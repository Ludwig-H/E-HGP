# État de l'art et positionnement

## 1. Règle de lecture

Le projet ne peut pas revendiquer séparément :

- une représentation radiale ;
- des moments sphériques ;
- l'apprentissage sur maillages ou polyèdres ;
- un Transformer hiérarchique ;
- un JEPA 3D ;
- une distillation LiDAR–image ;
- un pré-entraînement multi-datasets.

Toutes ces briques ont des antécédents. La question est de savoir si leur articulation autour des **surfaces polyédriques HGP et de leur espace d'échelle de densité** constitue un objet distinct et utile.

Cette revue est ciblée sur les décisions de conception. Elle ne remplace pas une recherche d'antériorité exhaustive au moment de la soumission.

## 2. Représentations de surfaces

### 2.1 Descripteurs radiaux, sphériques et distributions de forme

Les fonctions radiales de corps étoilés, les descripteurs par harmoniques sphériques, les moments de Zernike et les shape distributions précèdent largement l'apprentissage profond. Ils montrent qu'un objet 3D peut être résumé par des fonctions directionnelles ou des moments invariants.

**Conséquence.** Le projet ne doit jamais revendiquer comme nouveauté :

> « représenter une forme par des distances depuis un centre »

ni :

> « projeter une surface sur une base sphéro-radiale ».

La contribution possible est plus étroite : utiliser une **mesure de surface observée, ouverte et potentiellement multicouche**, à tous les nœuds d'une filtration HGP, puis apprendre sa stabilité inter-capteurs.

### 2.2 Représentations par rayons

[RayDF, NeurIPS 2023](https://proceedings.neurips.cc/paper_files/paper/2023/hash/4f86833d5cc98ec32e470ef1c8cb82e3-Abstract-Conference.html) et les directed distance fields représentent une géométrie par des requêtes de rayon et une distance à la surface.

**Proximité.** Les deux approches prennent au sérieux l'information portée le long d'une direction.

**Différence.** HGP-PolyFM part d'une surface explicite déjà calculée, n'apprend pas un champ de rendu et doit encoder simultanément toutes les couches, les bords, la confiance et la trajectoire de densité.

### 2.3 Atlas de cartes

[AtlasNet, CVPR 2018](https://openaccess.thecvf.com/content_cvpr_2018/html/Groueix_A_Papier-Mache_Approach_CVPR_2018_paper.html), [Surface Networks via General Covers, ICCV 2019](https://openaccess.thecvf.com/content_ICCV_2019/html/Haim_Surface_Networks_via_General_Covers_ICCV_2019_paper.html), les [atlas métriquement cohérents, ICCV 2021](https://openaccess.thecvf.com/content/ICCV2021/html/Bednarik_Temporally-Coherent_Surface_Reconstruction_via_Metric-Consistent_Atlases_ICCV_2021_paper.html) et [MAtCha, CVPR 2025](https://openaccess.thecvf.com/content/CVPR2025/html/Guedon_MAtCha_Gaussians_Atlas_of_Charts_for_High-Quality_Geometry_and_Photorealism_CVPR_2025_paper.html) démontrent la puissance des atlas pour les surfaces générales.

**Leçon.** L'atlas est la meilleure solution de repli lorsque la projection globale est mal conditionnée.

**Pourquoi il n'est pas le premier choix.** Les coutures, ancres et correspondances peuvent changer sous thinning ; apprendre l'atlas en même temps que le backbone rendrait l'effet du tokenizer difficile à isoler.

### 2.4 Champs implicites pour surfaces ouvertes

[GIFS, CVPR 2022](https://openaccess.thecvf.com/content/CVPR2022/html/Ye_GIFS_Neural_Implicit_Function_for_General_Shape_Representation_CVPR_2022_paper.html), [NeuralUDF, CVPR 2023](https://openaccess.thecvf.com/content/CVPR2023/html/Long_NeuralUDF_Learning_Unsigned_Distance_Fields_for_Multi-View_Reconstruction_of_Surfaces_CVPR_2023_paper.html) et les UDF différentiables représentent des surfaces ouvertes et de topologie générale.

**Leçon.** La difficulté monocouche n'impose ni surface fermée ni SDF.

**Pourquoi ce n'est pas la voie principale.** Ajuster un champ neural à chaque polyèdre explicite serait coûteux et redondant. Les UDF sont plus pertinentes comme décodeurs de complétion que comme tokens d'entrée.

### 2.5 Apprentissage natif sur polyèdres et maillages

[PolyhedronNet, ICLR 2025](https://proceedings.iclr.cc/paper_files/paper/2025/hash/d551343f85fcf5e1a230fd393406306e-Abstract-Conference.html) construit un surface-attributed graph reliant sommets, arêtes et faces, puis apprend une représentation globale de polyèdres. [MGM-AE, WACV 2024](https://openaccess.thecvf.com/content/WACV2024/html/Yang_MGM-AE_Self-Supervised_Learning_on_3D_Shape_Using_Mesh_Graph_Masked_WACV_2024_paper.html) masque des graphes de faces pour l'auto-supervision de formes maillées.

Ce sont les antécédents les plus importants pour le tokenizer local.

**Différences du projet.**

- surfaces partielles issues d'un LiDAR, non objets CAD complets ;
- chaque scène contient une forêt de polyèdres à toutes les échelles ;
- niveaux de densité et événements de fusion ;
- sortie dense de segmentation ;
- pré-entraînement entre acquisitions donnant des surfaces et des arbres différents.

**Conséquence.** `SurfaceGraph` doit être une baseline forte. Une grille sphéro-radiale qui ne la bat sur aucun compromis ne mérite pas d'être protégée par son élégance mathématique.

## 3. Encodeurs sphériques

Les CNN sphériques et [Icosahedral CNN, ICML 2019](https://proceedings.mlr.press/v97/cohen19d.html) fournissent des opérateurs équivariants pour des signaux sur la sphère.

Ils justifient l'implémentation de la grille angulaire, mais ne constituent pas la contribution. Le projet doit comparer :

- repère gravité–capteur ;
- augmentation de lacet ;
- invariance par moments ;
- équivariance `SO(3)`.

Le régime outdoor n'exige pas automatiquement d'effacer la gravité et la direction du capteur.

## 4. Hiérarchies de régions et Transformers structurés

### 4.1 Superpoint Transformer

[Superpoint Transformer, ICCV 2023](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html) apprend sur une hiérarchie de régions et propose un mode sans étage point-wise.

**Ce qu'il prouve.** Une sortie point-wise peut être obtenue depuis des tokens de régions.

**Ce qu'il ne couvre pas.** Les régions restent des agrégats de points ; les nœuds n'ont pas une surface explicite normalisée ni une trajectoire de densité physique.

SPT-nano devient une baseline d'architecture et de partition, non le modèle conceptuel principal.

### 4.2 Sequoia, Tree Transformers et Set Transformer

[Sequoia, TMLR 2024](https://openreview.net/forum?id=qH4YFMyhce), les Tree-Structured Transformers et [Set Transformer, ICML 2019](https://proceedings.mlr.press/v97/lee19d.html) fournissent les primitives parent–enfants–frères et l'agrégation permutation-invariante.

Ils motivent le `MergeEventEncoder`. La spécificité HGP réside dans les surfaces directement observées à chaque nœud, les deltas géométriques et le niveau de densité.

### 4.3 Hierarchical Self-Attention

[HSA, NeurIPS 2025](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) dérive une projection de l'attention Softmax sous une contrainte hiérarchique de blocs.

**Limite pour ce projet.** Dans la formulation publiée, les nœuds internes servent surtout à factoriser un signal porté par les feuilles. Ici, chaque nœud HGP possède sa propre surface observée. Ajouter ces surfaces change le modèle mathématique ; le théorème HSA ne s'étend pas automatiquement.

HSA est donc :

- une baseline théorique sur les feuilles ;
- éventuellement une extension à redériver ;
- pas le socle initial.

## 5. Auto-supervision et modèles de fondation 3D

### 5.1 Qualité de représentation

[Sonata, CVPR 2025](https://openaccess.thecvf.com/content/CVPR2025/html/Wu_Sonata_Self-Supervised_Learning_of_Reliable_Point_Representations_CVPR_2025_paper.html) montre que les modèles 3D auto-supervisés peuvent exploiter un raccourci géométrique et paraître solides en fine-tuning tout en donnant de mauvais linear probes.

[NOMAE, CVPR 2025](https://openaccess.thecvf.com/content/CVPR2025/html/Abdelsamad_Multi-Scale_Neighborhood_Occupancy_Masked_Autoencoder_for_Self-Supervised_Learning_in_LiDAR_CVPR_2025_paper.html) reconstruit l'occupation dans des voisinages multi-échelles sans traiter indistinctement espace vide et non observé.

**Leçons pour HGP-PolyFM.**

- frozen probing obligatoire ;
- reconstruction brute seulement en diagnostic ;
- masques surfaciques cohérents ;
- supervision sur les unités observables ;
- séparation forme / capteur.

### 5.2 2D–3D et grande échelle

[Concerto, NeurIPS 2025](https://proceedings.neurips.cc/paper_files/paper/2025/hash/649a31f2cb31a73b92c68b15bbf44442-Abstract-Conference.html) joint auto-distillation 3D et alignement 2D–3D. Les pré-entraînements LiDAR multi-datasets et multimodaux fixent une barre élevée pour toute revendication de modèle de fondation.

La caméra est une extension puissante, mais elle ne doit être ouverte qu'après démonstration du gain géométrique. Sinon le teacher 2D peut transformer les polyèdres en simple support de pooling.

### 5.3 Un encodeur pour plusieurs domaines

[Utonia, 2026](https://arxiv.org/abs/2603.03283) pré-entraîne un encodeur point Transformer unique sur des domaines très hétérogènes : outdoor LiDAR, indoor RGB-D, CAD, télédétection et points issus de vidéos.

**Conséquence.** Un modèle SemanticKITTI seul n'est plus crédiblement nommé « foundation model ». Le seuil minimal pour HGP-PolyFM est un encodeur LiDAR multi-capteurs et multi-tâches ; un claim 3D général exige des domaines beaucoup plus variés.

## 6. Carte des antécédents

| Axe | Antécédent le plus proche | Couvert | Manquant par rapport au projet |
|---|---|---|---|
| code de polyèdre | PolyhedronNet | surface-attributed graph | LiDAR partiel, densité, scène, transfert |
| masquage de maillage | MGM-AE | SSL de faces | arbres variables, cross-range |
| code radial / rayon | RayDF, DDF | distance le long des rayons | mesure explicite, couches, hiérarchie |
| atlas de surface | AtlasNet / MAtCha | surfaces générales | tokenizer stable inter-capteurs |
| surface ouverte implicite | GIFS / NeuralUDF | topologie générale | surface explicite déjà disponible |
| hiérarchie 3D | SPT | régions et reprojection | surfaces polyédriques, niveaux physiques |
| attention d'arbre | Sequoia / HSA | interactions hiérarchiques | descripteur surfacique à chaque nœud |
| SSL LiDAR | Sonata / NOMAE / DOS | représentation point/voxel | polyèdres et matching inter-arbres |
| fondation 3D | Concerto / Utonia | multi-données, multi-domaines | tokenizer de surface HGP |

## 7. Revendication défendable

La proposition n'est pas :

> « nous appliquons un Transformer à une hiérarchie »

ni :

> « nous décrivons un objet par des rayons ».

La revendication défendable est :

> **Nous remplaçons les retours ponctuels comme unités apprises par des mesures de surfaces polyédriques explicites, organisées dans un espace d'échelle de densité HGP ; nous modélisons leurs innovations et fusions, puis pré-entraînons le code de forme à rester cohérent entre acquisitions qui reconstruisent des surfaces et des arbres différents.**

Cette revendication exige cinq résultats :

1. meilleur taux–distorsion qu'un code point/superpoint pertinent ;
2. stabilité au remeshing et au changement de densité ;
3. avantage de l'arbre HGP sur les contrôles ;
4. transfert cross-capteur ;
5. gain sur plusieurs tâches ou faibles régimes de labels.

## 8. Matrice de comparaison

| Méthode | Primitive principale | Surface explicite | Hiérarchie physique | Nœuds internes observés | SSL cross-range | Multi-tâches |
|---|---|---:|---:|---:|---:|---:|
| PTv3 / Sonata | points | non | non | non | partiel | oui |
| sparse voxel / NOMAE | voxels | non | pyramidale | non | partiel | oui |
| SPT | superpoints | non | non | oui, régions | non | surtout segmentation |
| PolyhedronNet | polyèdre CAD | oui | non | non | non | classification/retrieval |
| MGM-AE | maillage d'objet | oui | non | non | non | forme |
| HSA | signal hiérarchique | dépend | générique | structurels | non | générique |
| **HGP-PolyFM** | surface HGP | **oui** | **oui** | **oui, surfaces** | **oui** | **objectif** |

La dernière ligne est un contrat expérimental, pas un résultat acquis.

## 9. Baselines obligatoires

### Primitives

- Point Transformer V3 ou équivalent ;
- sparse voxel backbone ;
- SPT / SPT-nano ;
- SurfaceGraph de type PolyhedronNet ;
- quadrature d'aire + Set Encoder.

### Structures

- plat ;
- graphe latéral ;
- arbre aléatoire ;
- octree ;
- HDBSCAN/RSL ;
- HGP sans niveaux ;
- HGP complet.

### SSL

- from scratch ;
- reconstruction ;
- Surface-JEPA ;
- SSL point/voxel récent ;
- Cross-Range PolyJEPA ;
- distillation 2D à teacher identique.

## 10. Positionnement de publication

### Contribution représentation / robustesse

Une soumission majeure est crédible si le projet démontre :

- nouvelle unité de calcul surfacique ;
- théorie simple d'invariance et de remeshing ;
- benchmark de radialité et taux–distorsion ;
- gain de segmentation ou de robustesse ;
- transfert vers un second capteur.

### Contribution fondation

Il faut en plus :

- pré-entraînement multi-datasets ;
- frozen probing et faible supervision ;
- plusieurs tâches ;
- courbes de scaling ;
- comparaison aux fondations points/voxels contemporaines.

Le premier résultat peut viser ICCV, NeurIPS ou ICML sans prétendre avoir déjà terminé le second programme.
