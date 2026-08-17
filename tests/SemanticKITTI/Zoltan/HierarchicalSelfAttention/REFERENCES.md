# Références retenues

Cette bibliographie est orientée vers les décisions du dossier. Une référence doit justifier un objet mathématique, une baseline, une loss ou un protocole. La proximité lexicale ne suffit pas ; le monde contient déjà assez de bibliographies décoratives.

## 1. Source géométrique normative

### Manuscrit de thèse

Louis Hauseux, *Manuscrit de thèse*, parties I et II.

- fichier : [`../../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`](../../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) ;
- rôle : définition de la filtration HGP, des facettes, polyèdres, niveaux de densité, branches et reprojection ;
- le présent dossier suppose cette hiérarchie disponible.

## 2. Descripteurs de forme et bases sphériques

### Shape distributions

R. Osada, T. Funkhouser, B. Chazelle, D. Dobkin, “Shape Distributions,” *ACM Transactions on Graphics*, 2002.

- [page Princeton](https://collaborate.princeton.edu/en/publications/shape-distributions/) ;
- rôle : précédent classique pour représenter un modèle polygonal par des distributions invariantes et robustes au remeshing ;
- usage : baseline analytique `P0`.

### Spherical harmonic descriptors

M. Kazhdan, T. Funkhouser, S. Rusinkiewicz, “Rotation Invariant Spherical Harmonic Representation of 3D Shape Descriptors,” 2003.

- [publication Eurographics](https://diglib.eg.org/items/28cde4b2-3ca2-4b32-8fef-8c4fe8f53e3f) ;
- rôle : antériorité pour les représentations sphériques et les invariants de rotation ;
- conséquence : aucune revendication de nouveauté sur une simple expansion en harmoniques.

### 3D Zernike descriptors

M. Novotni, R. Klein, “Shape Retrieval using 3D Zernike Descriptors,” *Computer-Aided Design*, 2004.

- [page éditeur](https://www.sciencedirect.com/science/article/pii/S0010448504000077) ;
- rôle : moments orthogonaux dans la boule et invariants de similitude ;
- usage : baseline spectrale `P3`.

### Évaluation directe sur maillages

J. Houdayer, P. Koehl, “Stable Evaluation of 3D Zernike Moments for Surface Meshes,” *Algorithms*, 2022.

- [article](https://www.mdpi.com/1999-4893/15/11/406) ;
- rôle : calcul de moments à partir de surfaces maillées sans dépendre uniquement d'une voxelisation.

## 3. Représentations par rayons et couches

### RayDF

Z. Liu, B. Yang, Y. Luximon, A. Kumar, J. Li, “RayDF: Neural Ray-surface Distance Fields with Multi-view Consistency,” NeurIPS 2023.

- [papier officiel](https://proceedings.neurips.cc/paper_files/paper/2023/hash/4f86833d5cc98ec32e470ef1c8cb82e3-Abstract-Conference.html) ;
- rôle : précédent pour une représentation continue par rayons et distance à la surface ;
- différence : champ appris pour reconstruction, non mesure explicite de polyèdres HGP.

### Layered depth images

H. Dhamo, N. Navab, F. Tombari, “Object-Driven Multi-Layer Scene Decomposition From a Single Image,” ICCV 2019.

- [papier CVF](https://openaccess.thecvf.com/content_ICCV_2019/html/Dhamo_Object-Driven_Multi-Layer_Scene_Decomposition_From_a_Single_Image_ICCV_2019_paper.html) ;
- rôle : précédent pour plusieurs profondeurs ordonnées par rayon ;
- usage : motivation et limite de `Radial-K`.

## 4. Atlas et paramétrisations de surface

### AtlasNet

T. Groueix, M. Fisher, V. G. Kim, B. Russell, M. Aubry, “A Papier-Mâché Approach to Learning 3D Surface Generation,” CVPR 2018.

- [papier CVF](https://openaccess.thecvf.com/content_cvpr_2018/html/Groueix_A_Papier-Mache_Approach_CVPR_2018_paper.html) ;
- rôle : collection de cartes paramétriques pour représenter une surface ;
- usage : baseline/repli `SurfaceAtlas`.

### Surface Networks via General Covers

N. Haim et al., “Surface Networks via General Covers,” ICCV 2019.

- [papier CVF](https://openaccess.thecvf.com/content_ICCV_2019/html/Haim_Surface_Networks_via_General_Covers_ICCV_2019_paper.html) ;
- rôle : couverture faible distorsion d'une surface pour utiliser des réseaux 2D.

### Atlas métriquement cohérents

J. Bednarik et al., “Temporally-Coherent Surface Reconstruction via Metric-Consistent Atlases,” ICCV 2021.

- [papier CVF](https://openaccess.thecvf.com/content/ICCV2021/html/Bednarik_Temporally-Coherent_Surface_Reconstruction_via_Metric-Consistent_Atlases_ICCV_2021_paper.html) ;
- rôle : atlas et correspondances cohérentes dans le temps ;
- intérêt : voie de repli pour des surfaces HGP temporelles.

### MAtCha Gaussians

A. Guedon, T. Ichikawa, K. Yamashita, K. Nishino, “MAtCha Gaussians: Atlas of Charts for High-Quality Geometry and Photorealism From Sparse Views,” CVPR 2025.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2025/html/Guedon_MAtCha_Gaussians_Atlas_of_Charts_for_High-Quality_Geometry_and_Photorealism_CVPR_2025_paper.html) ;
- rôle : précédent récent d'atlas explicite de scène ;
- différence : reconstruction/rendu depuis images, non tokenizer LiDAR hiérarchique.

## 5. Surfaces ouvertes et topologie générale

### GIFS

J. Ye, Y. Chen, N. Wang, X. Wang, “GIFS: Neural Implicit Function for General Shape Representation,” CVPR 2022.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2022/html/Ye_GIFS_Neural_Implicit_Function_for_General_Shape_Representation_CVPR_2022_paper.html) ;
- rôle : représentation de surfaces non watertight et multicouches sans inside/outside ;
- usage : décodeur ou baseline implicite, non tokenizer principal.

### NeuralUDF

X. Long et al., “NeuralUDF: Learning Unsigned Distance Fields for Multi-View Reconstruction of Surfaces With Arbitrary Topologies,” CVPR 2023.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Long_NeuralUDF_Learning_Unsigned_Distance_Fields_for_Multi-View_Reconstruction_of_Surfaces_CVPR_2023_paper.html) ;
- rôle : UDF pour surfaces ouvertes et topologies arbitraires.

### DUDF

M. Fainstein, V. Siless, E. Iarussi, “DUDF: Differentiable Unsigned Distance Fields with Hyperbolic Scaling,” CVPR 2024.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2024/html/Fainstein_DUDF_Differentiable_Unsigned_Distance_Fields_with_Hyperbolic_Scaling_CVPR_2024_paper.html) ;
- rôle : difficultés de différentiabilité et extraction des UDF ;
- conséquence : confirme que l'implicite n'est pas un repli gratuit.

## 6. Apprentissage natif sur polyèdres et maillages

### PolyhedronNet

D. Yu, G. Zhang, L. Zhao, “PolyhedronNet: Representation Learning for Polyhedra with Surface-attributed Graph,” ICLR 2025.

- [papier officiel](https://proceedings.iclr.cc/paper_files/paper/2025/hash/d551343f85fcf5e1a230fd393406306e-Abstract-Conference.html) ;
- [code](https://github.com/dyu62/3D_polyhedron) ;
- rôle : baseline locale majeure, graphe attribué sommets–arêtes–faces et message passing géométrique ;
- différence : objets polyédriques complets, classification/retrieval, pas scènes LiDAR ni filtration HGP.

### MGM-AE

L. Yang et al., “MGM-AE: Self-Supervised Learning on 3D Shape Using Mesh Graph Masked Autoencoders,” WACV 2024.

- [papier CVF](https://openaccess.thecvf.com/content/WACV2024/html/Yang_MGM-AE_Self-Supervised_Learning_on_3D_Shape_Using_Mesh_Graph_Masked_WACV_2024_paper.html) ;
- rôle : antériorité du masked modeling sur graphes de faces ;
- conséquence : `Surface-JEPA` doit se distinguer par les surfaces HGP et le cross-range.

## 7. Encodeurs sphériques et équivariance

### Icosahedral CNN

T. Cohen, M. Weiler, B. Kicanaoglu, M. Welling, “Gauge Equivariant Convolutional Networks and the Icosahedral CNN,” ICML 2019.

- [papier PMLR](https://proceedings.mlr.press/v97/cohen19d.html) ;
- rôle : opérateur scalable pour signaux sur une icosphère ;
- usage : baseline du `SurfaceEncoder`.

### Scaling Spherical CNNs

C. Esteves, J.-J. Slotine, A. Makadia, “Scaling Spherical CNNs,” ICML 2023.

- [papier PMLR](https://proceedings.mlr.press/v202/esteves23a.html) ;
- rôle : architectures sphériques plus profondes et scalables ;
- usage : comparaison si l'équivariance complète devient nécessaire.

## 8. Hiérarchies de régions 3D

### Superpoint Graph

L. Landrieu, M. Simonovsky, “Large-scale Point Cloud Semantic Segmentation with Superpoint Graphs,” CVPR 2018.

- [papier CVF](https://openaccess.thecvf.com/content_cvpr_2018/html/Landrieu_Large-Scale_Point_Cloud_CVPR_2018_paper.html) ;
- rôle : paradigme région→graphe→reprojection et oracle de partition.

### Superpoint Transformer

D. Robert, H. Raguet, L. Landrieu, “Efficient 3D Semantic Segmentation with Superpoint Transformer,” ICCV 2023.

- [papier CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html) ;
- [code](https://github.com/drprojects/superpoint_transformer) ;
- rôle : baseline de régions hiérarchiques, attention horizontale/verticale et sortie région→point ;
- statut : porteur comparatif, non architecture conceptuelle principale.

## 9. Transformers structurés

### Set Transformer

J. Lee et al., “Set Transformer: A Framework for Attention-based Permutation-Invariant Neural Networks,” ICML 2019.

- [papier PMLR](https://proceedings.mlr.press/v97/lee19d.html) ;
- rôle : agrégation des enfants et inducing tokens pour grandes fusions.

### Graphormer

C. Ying et al., “Do Transformers Really Perform Bad for Graph Representation?” NeurIPS 2021.

- [papier officiel](https://proceedings.neurips.cc/paper/2021/hash/f1c1592588411002af340cbaedd6fc33-Abstract.html) ;
- rôle : biais relationnels dans l'attention.

### Sequoia

T. Trang et al., “Scalable Hierarchical Self-Attention with Learnable Hierarchy for Long-Range Interactions,” TMLR 2024.

- [OpenReview](https://openreview.net/forum?id=qH4YFMyhce) ;
- [code](https://github.com/HySonLab/HierAttention) ;
- rôle : meilleure baseline parent–enfants–frères sur arbre complet.

### Hierarchical Self-Attention

S. Amizadeh, S. Abdali, Y. Li, K. Koishida, “Hierarchical Self-Attention: Generalizing Neural Attention Mechanics to Multi-Scale Problems,” NeurIPS 2025.

- [papier officiel](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) ;
- copie locale : [`NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf`](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) ;
- rôle : baseline théorique sous contrainte hiérarchique ;
- réserve : les nœuds internes HGP portent ici leurs propres surfaces, hors formulation publiée.

## 10. Pré-entraînement latent et 3D

### I-JEPA

M. Assran et al., “Self-Supervised Learning from Images with a Joint-Embedding Predictive Architecture,” CVPR 2023.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Assran_Self-Supervised_Learning_From_Images_With_a_Joint-Embedding_Predictive_Architecture_CVPR_2023_paper.html) ;
- rôle : teacher EMA et prédiction latente de cibles structurées.

### Point-JEPA

A. Saito et al., “Point-JEPA: A Joint Embedding Predictive Architecture for Self-Supervised Learning on Point Cloud,” WACV 2025.

- [papier CVF](https://openaccess.thecvf.com/content/WACV2025/html/Saito_Point-JEPA_A_Joint_Embedding_Predictive_Architecture_for_Self-Supervised_Learning_on_WACV_2025_paper.html) ;
- rôle : JEPA sur patches 3D sans reconstruction brute.

### Sonata

X. Wu et al., “Sonata: Self-Supervised Learning of Reliable Point Representations,” CVPR 2025.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2025/html/Wu_Sonata_Self-Supervised_Learning_of_Reliable_Point_Representations_CVPR_2025_paper.html) ;
- rôle : diagnostic du raccourci géométrique et importance du linear probing.

### NOMAE

M. Abdelsamad et al., “Multi-Scale Neighborhood Occupancy Masked Autoencoder for Self-Supervised Learning in LiDAR Point Clouds,” CVPR 2025.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2025/html/Abdelsamad_Multi-Scale_Neighborhood_Occupancy_Masked_Autoencoder_for_Self-Supervised_Learning_in_LiDAR_CVPR_2025_paper.html) ;
- rôle : masquage multi-échelle et distinction entre vide, masqué et non observé.

### DOS

M. Abdelsamad et al., “DOS: Distilling Observable Softmaps of Zipfian Prototypes for Self-Supervised Point Representation,” AAAI 2026.

- [article officiel](https://ojs.aaai.org/index.php/AAAI/article/view/39030) ;
- rôle : supervision uniquement sur les éléments observables et prototypes doux ;
- usage : baseline SSL LiDAR et inspiration du matching partiel.

### VICReg

A. Bardes, J. Ponce, Y. LeCun, “VICReg: Variance-Invariance-Covariance Regularization,” ICLR 2022.

- [OpenReview](https://openreview.net/forum?id=xm6YD62D1Ub) ;
- rôle : anti-effondrement sans négatifs.

## 11. Multimodalité et modèles de fondation 3D

### Concerto

Y. Zhang et al., “Concerto: Joint 2D-3D Self-Supervised Learning Emerges Spatial Representations,” NeurIPS 2025.

- [papier officiel](https://proceedings.neurips.cc/paper_files/paper/2025/hash/649a31f2cb31a73b92c68b15bbf44442-Abstract-Conference.html) ;
- rôle : auto-distillation 3D et alignement 2D–3D ;
- usage : baseline multimodale après validation géométrique.

### Utonia

Y. Zhang et al., “Utonia: Toward One Encoder for All Point Clouds,” 2026.

- [arXiv](https://arxiv.org/abs/2603.03283) ;
- [page projet](https://pointcept.github.io/Utonia/) ;
- rôle : barre actuelle pour un encodeur 3D multi-domaines ;
- statut : travail 2026 à réauditer dans sa version de soumission/publication.

### PointINS

B. Yang et al., “Towards Foundation Models for 3D Scene Understanding: Instance-Aware Self-Supervised Learning for Point Clouds,” 2026.

- [arXiv](https://arxiv.org/abs/2603.25165) ;
- rôle : rappelle qu'une représentation fondation doit transférer vers les instances et la localisation ;
- statut : prépublication récente.

## 12. Robustesse LiDAR

### LiDomAug

K. Ryu, S. Hwang, J. Park, “Instant Domain Augmentation for LiDAR Semantic Segmentation,” CVPR 2023.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Ryu_Instant_Domain_Augmentation_for_LiDAR_Semantic_Segmentation_CVPR_2023_paper.html) ;
- rôle : simulations de configurations capteur, mouvement et occultations.

### SemanticKITTI-C

X. Yan et al., “Benchmarking the Robustness of LiDAR Semantic Segmentation Models,” 2023.

- [arXiv](https://arxiv.org/abs/2301.00970) ;
- rôle : corruptions et métriques de robustesse.

### Robo3D

L. Kong et al., “Robo3D: Towards Robust and Reliable 3D Perception against Corruptions,” ICCV 2023.

- [papier CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Kong_Robo3D_Towards_Robust_and_Reliable_3D_Perception_Against_Corruptions_ICCV_2023_paper.html) ;
- rôle : `mCE`, `mRR` et benchmarks de robustesse 3D.

## 13. Datasets

### SemanticKITTI

J. Behley et al., “SemanticKITTI: A Dataset for Semantic Scene Understanding of LiDAR Sequences,” ICCV 2019.

- [site](https://semantic-kitti.org/) ;
- [papier](https://arxiv.org/abs/1904.01416) ;
- [API](https://github.com/PRBonn/semantic-kitti-api) ;
- rôle : tâche pilote mono-scan.

### nuScenes

H. Caesar et al., “nuScenes: A Multimodal Dataset for Autonomous Driving,” CVPR 2020.

- [papier CVF](https://openaccess.thecvf.com/content_CVPR_2020/html/Caesar_nuScenes_A_Multimodal_Dataset_for_Autonomous_Driving_CVPR_2020_paper.html) ;
- rôle : second capteur et transfert.

## 14. Règle de comparaison

Une méthode n'entre dans la table numérique principale que si sont vérifiables :

- primitive d'entrée et modalités à l'inférence ;
- données de pré-entraînement ;
- nombre de scans / frames ;
- TTA et ensemble ;
- split et métrique ;
- code ou configuration ;
- budget de paramètres et calcul lorsque la comparaison porte sur l'architecture.

Les prépublications 2026 sont citées pour le positionnement, pas utilisées comme chiffres immuables avant réaudit.
