# Positionnement et choix des modèles

Ce document ne cherche pas à établir un classement figé. Il répond à trois questions :

1. quels travaux fournissent déjà les briques nécessaires ;
2. ce qui reste réellement distinct dans le projet ;
3. quel code reprendre plutôt que réinventer.

Les scores SemanticKITTI et les régimes de données doivent être réaudités avant toute soumission : le serveur officiel a été transféré vers CodaBench en janvier 2026, et les tableaux historiques mélangent souvent mono-scan, multi-scan, caméra, TTA et pré-entraînement externe.

## 1. Carte des antécédents

| Famille | Travail de référence | Ce qu'il apporte | Limite pour ce projet |
|---|---|---|---|
| hiérarchie de régions 3D | [Superpoint Transformer, ICCV 2023](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html) | U-Net de superpoints, arêtes horizontales/verticales, RPE, mode sans points | partitions apprises/heuristiques, niveaux non physiques |
| attention de famille | [Sequoia, TMLR 2024](https://openreview.net/forum?id=qH4YFMyhce) | attention sparse parent–enfants–frères, interactions longues | hiérarchie généralement apprise ; pas de LiDAR outdoor |
| attention hiérarchique dérivée | [HSA, NeurIPS 2025](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) | opérateur optimal sous contrainte hiérarchique, programmation dynamique | opérateur plus lourd, validation 3D non comparable à SemanticKITTI |
| hypergraphe / incidences | [AllSet, ICLR 2022](https://openreview.net/forum?id=hpBTIv2uy_E) | Set Transformer sur multiensembles nœud↔hyperarête | ignore par défaut la filtration et l'arbre de fusion |
| SSL latent | [I-JEPA, CVPR 2023](https://openaccess.thecvf.com/content/CVPR2023/html/Assran_Self-Supervised_Learning_From_Images_With_a_Joint-Embedding_Predictive_Architecture_CVPR_2023_paper.html) | prédiction dans l'espace latent, target encoder EMA, masques sémantiques | construit pour images |
| SSL point cloud | [Point-JEPA, WACV 2025](https://arxiv.org/abs/2404.16432) | JEPA sur patches 3D sans reconstruction brute | objets centrés ; pas d'arbre de densité outdoor |
| SSL LiDAR | [DOS, AAAI 2026](https://ojs.aaai.org/index.php/AAAI/article/view/39030) | distillation de softmaps observables, prototypes Zipfiens | backbone point/voxel et correspondance fixe des observations |
| invariance capteur | [LiDomAug, CVPR 2023](https://openaccess.thecvf.com/content/CVPR2023/html/Ryu_Instant_Domain_Augmentation_for_LiDAR_Semantic_Segmentation_CVPR_2023_paper.html) | simulation de configurations LiDAR et occultations | augmentation, pas représentation hiérarchique |

## 2. Le précédent le plus proche : Superpoint Transformer

SPT est la base d'implémentation la plus raisonnable pour le premier prototype.

Son dépôt officiel fournit :

- une structure hiérarchique de régions ;
- des graphes d'adjacence à chaque niveau ;
- des arêtes verticales vers les parents ;
- un encodeur-décodeur de type U-Net ;
- des encodages relatifs injectables dans `Q`, `K` et `V` ;
- un mode `nano=True` qui retire explicitement l'étage point-wise.

Ce dernier point est décisif : `SPT-nano` montre qu'un modèle de segmentation 3D entièrement porté par des régions n'est pas une impossibilité logicielle. Il ne prouve évidemment pas que les facettes proposées sont les bonnes unités ni que le modèle atteindra le meilleur score sur SemanticKITTI.

### Ce qui peut être repris

- les objets `NAG` et la logique de batching hiérarchique ;
- les `DownNFuseStage` et `UpNFuseStage` ;
- l'attention sparse sur les arêtes latérales ;
- les MLP d'arêtes verticales et horizontales ;
- les losses et outils de reprojection région→point, après vérification des poids.

### Ce qui doit être remplacé

- le partitionneur de superpoints ;
- les niveaux par nombre de régions ;
- les features artisanales propres à SPT ;
- l'unpooling par simple index si les feuilles se recouvrent ;
- toute hypothèse qu'un point appartient à une unique région.

### Ce qui doit être conservé comme contrôle

- SPT-nano officiel avec sa partition ;
- même architecture avec la structure polyédrique ;
- même structure avec un MLP ou message passing sans attention.

Cette triple comparaison sépare représentation, hiérarchie et opérateur.

## 3. Sequoia : meilleur modèle conceptuel pour l'arbre complet

Sequoia contraint chaque token à interagir avec sa famille immédiate : parent, enfants et frères. Cette topologie correspond mieux à un arbre événementiel complet qu'une succession de quatre partitions grossières.

### Adaptation proposée

- hiérarchie fixée par le prétraitement, non apprise ;
- un token par nœud persistant ;
- attention enfants→parent puis parent→enfants ;
- attention entre frères au sein d'un événement de fusion ;
- arêtes latérales géométriques en parallèle ;
- encodage relatif des écarts de niveau, masse, pose et interface.

### Pourquoi ne pas commencer par Sequoia

Le code et les kernels ne sont pas conçus pour les recouvrements de facettes, les poids de reprojection et les événements simultanés du projet. L'adaptation est une seconde étape. `PolyTreeFormer-Nano` doit d'abord démontrer que les tokens et canaux fonctionnent.

## 4. HSA : comparaison théorique, pas socle initial

HSA dérive une attention incorporant une hiérarchie comme projection optimale de l'attention Softmax sous les contraintes du modèle multi-échelle. C'est le contrôle théorique le plus propre lorsque l'arbre est donné.

Il doit être testé après les baselines simples, car :

- sa programmation dynamique suit la profondeur de l'arbre ;
- son coût dépend du branchement ;
- il est moins trivial à intégrer aux arêtes latérales ;
- un échec simultané de la tokenisation et de HSA serait impossible à diagnostiquer.

L'ordre expérimental est donc :

```text
MeanTree → SPT-nano adapté → Sequoia-fixed → HSA.
```

HSA devient une contribution utile seulement s'il bat un opérateur sparse plus simple à budget apparié ou apporte une propriété mesurable de robustesse.

## 5. AllSet et réseaux de complexes

Le graphe dual réduit chaque incidence à une relation par paire. Il peut perdre l'identité d'une coface ou d'un événement d'ordre supérieur.

`AllSetTransformer` est la première extension à tester si cette perte devient visible :

```text
sommet/facette → multiensemble de la coface → facette/sommet.
```

Un encodeur d'incidences complet n'est pas prioritaire parce qu'il augmente fortement le nombre de tokens et de messages. Il est justifié seulement si :

- deux structures de même graphe dual mais d'incidences différentes doivent être distinguées ;
- le null test `incidence-shuffled` dégrade les résultats ;
- la mémoire reste compatible avec un scan complet.

## 6. Pré-entraînement : pourquoi JEPA avant reconstruction

La reconstruction de coordonnées ou de descripteurs bas niveau favorise les détails d'acquisition. Or le but est précisément d'abstraire la dilution du capteur.

I-JEPA et Point-JEPA fournissent le principe adapté : prédire l'embedding d'une cible à partir d'un contexte, sans reconstruire l'entrée brute. Le projet ajoute une difficulté absente de ces travaux : les deux vues peuvent avoir des arbres différents.

DOS fournit deux leçons pratiques :

1. ne superviser que les unités réellement observables afin d'éviter une fuite d'information ;
2. une distribution douce de prototypes peut être plus informative qu'une cible dure.

La recette retenue est donc progressive :

1. JEPA latent sur nœuds appariés ;
2. prédiction des événements de la trajectoire ;
3. softmaps observables seulement après validation du matching.

## 7. Travaux d'augmentation LiDAR

LiDomAug montre qu'il faut simuler la physique du capteur, pas seulement supprimer des points uniformément. La banque de transformations doit couvrir :

- nombre et position des anneaux ;
- résolution azimutale ;
- portée ;
- occultations ;
- mouvement du capteur et des objets ;
- bruit et rémission.

Pour le pré-entraînement principal, seule une sous-partie contrôlée est utilisée au départ. Ajouter immédiatement toutes les corruptions rendrait l'appariement des arbres trop rare.

## 8. Paysage 2026 à surveiller

Quatre prépublications récentes empêchent de présenter le projet comme une simple première utilisation de hiérarchie ou de SSL en 3D :

| Travail | Signal pour le projet | Différence essentielle |
|---|---|---|
| [Utonia](https://arxiv.org/abs/2603.03283) | les encodeurs point cloud unifiés deviennent une baseline de transfert ambitieuse | backbone point Transformer multi-domaines, pas structure polyédrique exogène |
| [PointINS](https://arxiv.org/abs/2603.25165) | le SSL 3D se déplace vers les propriétés d'instance et la géométrie aval | offsets et pseudo-instances, pas filtration de densité |
| [HilDA](https://arxiv.org/abs/2606.20189) | le mot « hiérarchique » est déjà utilisé en distillation LiDAR | hiérarchie des couches du teacher et objectif temporel/cross-modal |
| [HASSL](https://arxiv.org/abs/2607.04353) | une loss SSL fondée sur une hiérarchie HDBSCAN existe désormais | arbre latent de batch en microscopie, pas arbre géométrique fixé par scan |

La formulation défendable n'est donc pas « hierarchy-aware SSL ». Elle est plus étroite : **prédiction latente entre deux réalisations capteur d'une même filtration géométrique exogène, avec matching explicite des branches et sortie polyèdre-only**.

## 9. Ce qui est réellement distinct

La nouveauté ne peut pas être revendiquée sur les briques suivantes prises isolément :

- segmenter des superpoints ;
- utiliser un Transformer hiérarchique ;
- injecter des biais relatifs ;
- prédire des embeddings masqués ;
- simuler un autre capteur LiDAR.

La proposition spécifique est leur articulation :

> **apprendre uniquement sur les feuilles et événements d'une filtration polyédrique fine, en séparant forme normalisée, métrique physique, niveau relatif et acquisition, puis pré-entraîner la représentation à conserver les branches sémantiques sous dégradation LiDAR.**

Cette revendication ne tient que si quatre résultats sont obtenus :

1. oracle de feuilles élevé ;
2. stabilité mesurée sous dilution ;
3. gain de la vraie hiérarchie sur les contrôles ;
4. transfert à un second capteur.

## 10. Matrice de comparaison pour le papier

| Méthode | Tokens points | Structure donnée | Niveaux physiques | Recouvrement | SSL portée | Sortie point-wise |
|---|---:|---:|---:|---:|---:|---:|
| PTv3 / DOS | oui | non | non | non | partiel | oui |
| SPT | optionnel | hiérarchie de superpoints | non | non | non | oui |
| Sequoia | dépend de la tâche | apprise ou fixée | non | non | non | dépend |
| HSA | dépend de la tâche | oui | générique | générique | non | dépend |
| AllSet | non requis | hypergraphe | non | oui | non | dépend |
| **PolyTreeFormer** | **non** | **oui** | **oui** | **oui** | **oui** | **oui** |

La dernière ligne est un objectif de conception, pas une preuve de supériorité.

## 11. Baselines de score

Le projet doit reproduire au moins :

- un backbone point/voxel moderne sur SemanticKITTI ;
- SPT-nano ou une adaptation fidèle ;
- DOS lorsque le budget le permet ;
- le graphe dual sans hiérarchie.

Le meilleur score public n'est pas une cible stable tant que les entrées, données de pré-entraînement, TTA et ensembles ne sont pas harmonisés. Le dossier maintient donc deux tableaux séparés :

1. **comparaison scientifique stricte**, même données et même inférence ;
2. **contexte leaderboard**, réaudité à la date de soumission.
