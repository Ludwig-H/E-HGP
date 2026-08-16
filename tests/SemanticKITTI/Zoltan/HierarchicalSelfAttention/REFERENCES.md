# Références retenues

Cette bibliographie est volontairement courte. Chaque entrée doit justifier une décision de conception, une baseline ou un protocole. Les articles simplement voisins ne sont pas empilés pour donner au dossier l'apparence rassurante d'une revue systématique qu'il ne serait pas.

## 1. Sources internes normatives

### Manuscrit

Louis Hauseux, *Manuscrit de thèse*, parties I et II.

- fichier : [`../../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`](../../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) ;
- rôle : définition de la filtration, des objets polyédriques, de l'arbre et de la reprojection pondérée ;
- le présent dossier n'en répète que les interfaces nécessaires au réseau.

### Hierarchical Self-Attention

S. Amizadeh, S. Abdali, Y. Li, K. Koishida, “Hierarchical Self-Attention: Generalizing Neural Attention Mechanics to Multi-Scale Problems,” NeurIPS 2025.

- [page officielle](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) ;
- copie locale : [`NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf`](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) ;
- rôle : opérateur hiérarchique dérivé et baseline théorique ;
- statut : **pas** le premier modèle à implémenter.

## 2. Segmentation sur hiérarchies de régions

### Superpoint Transformer

D. Robert, H. Raguet, L. Landrieu, “Efficient 3D Semantic Segmentation with Superpoint Transformer,” ICCV 2023.

- [papier CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html) ;
- [code officiel](https://github.com/drprojects/superpoint_transformer) ;
- rôle : porteur de départ, représentation hiérarchique `NAG`, attention horizontale/verticale, mode `nano` sans étage point-wise.

### Superpoint Graph

L. Landrieu, M. Simonovsky, “Large-scale Point Cloud Semantic Segmentation with Superpoint Graphs,” CVPR 2018.

- [papier CVF](https://openaccess.thecvf.com/content_cvpr_2018/html/Landrieu_Large-Scale_Point_Cloud_CVPR_2018_paper.html) ;
- rôle : origine du paradigme région→graphe→reprojection et des oracles de partition.

### Oversegmentation LiDAR apprise

L. Hui, L. Tang, Y. Dai, J. Xie, J. Yang, “Efficient LiDAR Point Cloud Oversegmentation Network,” ICCV 2023.

- [papier CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Hui_Efficient_LiDAR_Point_Cloud_Oversegmentation_Network_ICCV_2023_paper.html) ;
- rôle : contrôle d'une tokenisation LiDAR en superpoints apprise ;
- limite : la partition dépend d'un encodeur point-wise et n'est pas l'objet principal ici.

## 3. Transformers hiérarchiques et structurés

### Sequoia

T. Trang et al., “Scalable Hierarchical Self-Attention with Learnable Hierarchy for Long-Range Interactions,” TMLR 2024.

- [papier OpenReview](https://openreview.net/forum?id=qH4YFMyhce) ;
- [code officiel](https://github.com/HySonLab/HierAttention) ;
- rôle : attention sparse limitée aux parents, enfants et frères ; modèle conceptuel du `PolyTreeFormer-Full`.

### Tree-Structured Transformer

W. Wang et al., “Learning Program Representations with a Tree-Structured Transformer,” 2022.

- [arXiv](https://arxiv.org/abs/2208.08643) ;
- rôle : propagation bidirectionnelle parent–enfants et attention entre frères ; antériorité générale hors 3D.

### Graphormer

C. Ying et al., “Do Transformers Really Perform Bad for Graph Representation?” NeurIPS 2021.

- [papier NeurIPS](https://proceedings.neurips.cc/paper/2021/hash/f1c1592588411002af340cbaedd6fc33-Abstract.html) ;
- rôle : biais structurels additifs dans l'attention ; antériorité pour les encodages d'arêtes et de distance.

### Set Transformer

J. Lee et al., “Set Transformer: A Framework for Attention-based Permutation-Invariant Neural Networks,” ICML 2019.

- [PMLR](https://proceedings.mlr.press/v97/lee19d.html) ;
- rôle : attention permutation-invariante et inducing points pour familles de haut degré.

## 4. Hypergraphes et incidences

### AllSet

E. Chien, C. Pan, J. Peng, O. Milenkovic, “You Are AllSet: A Multiset Function Framework for Hypergraph Neural Networks,” ICLR 2022.

- [OpenReview](https://openreview.net/forum?id=hpBTIv2uy_E) ;
- [code officiel](https://github.com/jianhao2016/AllSet) ;
- rôle : extension `AllSet-incidence` si le graphe dual perd les interactions d'ordre supérieur.

### HEAT

D. Georgiev, M. Brockschmidt, M. Allamanis, “HEAT: Hyperedge Attention Networks,” TMLR 2022.

- [page auteur](https://miltos.allamanis.com/publications/2022heat/) ;
- rôle : hyperarêtes typées et qualifiées ; référence si les rôles dans les incidences doivent être explicités.

### Cell complexes

C. Bodnar et al., “Weisfeiler and Lehman Go Cellular: CW Networks,” NeurIPS 2021.

- [papier NeurIPS](https://proceedings.neurips.cc/paper/2021/hash/157792e4abb490f99dbd738483e0d2d4-Abstract.html) ;
- rôle : baseline de réseaux cellulaires ; non prioritaire tant qu'un graphe typé suffit.

## 5. Pré-entraînement latent

### I-JEPA

M. Assran et al., “Self-Supervised Learning from Images with a Joint-Embedding Predictive Architecture,” CVPR 2023.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Assran_Self-Supervised_Learning_From_Images_With_a_Joint-Embedding_Predictive_Architecture_CVPR_2023_paper.html) ;
- rôle : target encoder EMA, prédiction latente et importance de masques de grande échelle.

### Point-JEPA

A. Saito, P. Kudeshia, J. Poovvancheri, “Point-JEPA: A Joint Embedding Predictive Architecture for Self-Supervised Learning on Point Cloud,” WACV 2025.

- [arXiv](https://arxiv.org/abs/2404.16432) ;
- [code officiel](https://github.com/Ayumu-J-S/Point-JEPA) ;
- rôle : précédent direct pour une JEPA 3D sans reconstruction de coordonnées.

### AD-L-JEPA

H. Zhu et al., “Self-Supervised Representation Learning with Joint Embedding Predictive Architecture for Automotive LiDAR Object Detection.”

- [page projet](https://ad-l-jepa.github.io/) ;
- rôle : JEPA en LiDAR automobile et masquage BEV ;
- statut : prépublication à utiliser comme voisin récent, non comme preuve établie.

### VICReg

A. Bardes, J. Ponce, Y. LeCun, “VICReg: Variance-Invariance-Covariance Regularization for Self-Supervised Learning,” ICLR 2022.

- [OpenReview](https://openreview.net/forum?id=xm6YD62D1Ub) ;
- rôle : régularisation anti-effondrement sans négatifs.

## 6. Auto-supervision LiDAR

### DOS

M. Abdelsamad et al., “DOS: Distilling Observable Softmaps of Zipfian Prototypes for Self-Supervised Point Representation,” AAAI 2026.

- [article officiel](https://ojs.aaai.org/index.php/AAAI/article/view/39030) ;
- rôle : distillation uniquement sur éléments observables et softmaps de prototypes ; comparaison prioritaire pour le fine-tuning complet.

### TARL

S. Nunes et al., “TARL: A Temporal Representation Learning Framework for Large-scale Point Clouds,” 2023.

- [arXiv](https://arxiv.org/abs/2201.04695) ;
- rôle : SSL LiDAR outdoor et cohérence de régions à travers scans ; régime temporel à ne pas mélanger au mono-scan strict.

### BEVContrast

H. Sautier et al., “BEVContrast: Self-Supervision in BEV Space for Automotive LiDAR Point Clouds,” 3DV 2024.

- [arXiv](https://arxiv.org/abs/2310.17281) ;
- rôle : baseline SSL automobile fondée sur une représentation BEV.

## 7. Prépublications 2026 à surveiller

### Utonia

Y. Zhang et al., “Utonia: Toward One Encoder for All Point Clouds,” 2026.

- [arXiv](https://arxiv.org/abs/2603.03283) ;
- rôle : baseline de représentation 3D multi-domaines et de transfert ;
- statut : prépublication récente, protocole et budget à réauditer avant comparaison.

### PointINS

B. Yang et al., “Towards Foundation Models for 3D Scene Understanding: Instance-Aware Self-Supervised Learning for Point Clouds,” 2026.

- [arXiv](https://arxiv.org/abs/2603.25165) ;
- rôle : SSL géométrique orienté instances et panoptic outdoor ; voisin direct pour le positionnement fondation.

### HilDA

M. Wozniak et al., “HilDA: Hierarchical Distillation with Diffusion for Advancing Self-Supervised LiDAR Pre-training,” 2026.

- [arXiv](https://arxiv.org/abs/2606.20189) ;
- rôle : distillation LiDAR hiérarchique, cross-modale et temporelle ;
- différence : sa hiérarchie porte sur les couches et le contexte du teacher, non sur une filtration géométrique.

### HASSL

J. Riel et al., “HASSL: Hierarchy-Aware Self-Supervised Learning Framework for Single Cell Microscopy,” 2026.

- [arXiv](https://arxiv.org/abs/2607.04353) ;
- rôle : antériorité explicite pour une loss hiérarchique fondée sur HDBSCAN et des prototypes multi-niveaux ;
- différence : arbre latent recalculé sur les embeddings de batch en microscopie.

Ces quatre travaux sont des prépublications mouvantes. Ils doivent être relus dans leur version la plus récente à la date de soumission.

## 8. Domaine capteur et robustesse

### LiDomAug

K. Ryu, S. Hwang, J. Park, “Instant Domain Augmentation for LiDAR Semantic Segmentation,” CVPR 2023.

- [papier CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Ryu_Instant_Domain_Augmentation_for_LiDAR_Semantic_Segmentation_CVPR_2023_paper.html) ;
- rôle : simulation de capteurs, mouvement et occultations ; source pour les dégradations physiques.

### SemanticKITTI-C

X. Yan et al., “Benchmarking the Robustness of LiDAR Semantic Segmentation Models,” 2023.

- [arXiv](https://arxiv.org/abs/2301.00970) ;
- [page benchmark](https://yanx27.github.io/RobustLidarSeg/) ;
- rôle : corruptions de mesure, météo et changement de capteur.

### Robo3D

L. Kong et al., “Robo3D: Towards Robust and Reliable 3D Perception against Corruptions,” ICCV 2023.

- [arXiv](https://arxiv.org/abs/2303.17597) ;
- [code et benchmark](https://github.com/worldbench/Robo3D) ;
- rôle : métriques `mCE`, `mRR` et entraînement insensible à la densité.

### Point-to-Voxel Distillation

Y. Hou et al., “Point-to-Voxel Knowledge Distillation for LiDAR Semantic Segmentation,” CVPR 2022.

- [arXiv](https://arxiv.org/abs/2206.02099) ;
- rôle : importance explicite des objets lointains et de la densité variable dans la distillation.

## 9. Dataset et évaluation

### SemanticKITTI

J. Behley et al., “SemanticKITTI: A Dataset for Semantic Scene Understanding of LiDAR Sequences,” ICCV 2019.

- [site officiel](https://semantic-kitti.org/) ;
- [papier](https://arxiv.org/abs/1904.01416) ;
- [API](https://github.com/PRBonn/semantic-kitti-api) ;
- rôle : splits, labels et évaluateur officiel.

Le site officiel annonce le transfert des compétitions vers CodaBench le 31 janvier 2026. Les anciens classements CodaLab ne doivent pas être utilisés comme source unique d'un claim actuel.

### nuScenes

H. Caesar et al., “nuScenes: A Multimodal Dataset for Autonomous Driving,” CVPR 2020.

- [site officiel](https://www.nuscenes.org/) ;
- [papier CVF](https://openaccess.thecvf.com/content_CVPR_2020/html/Caesar_nuScenes_A_Multimodal_Dataset_for_Autonomous_Driving_CVPR_2020_paper.html) ;
- rôle : second capteur et transfert.

## 10. Règle de citation

Une méthode n'entre dans la comparaison principale que si les éléments suivants sont vérifiables :

- type d'entrée à l'inférence ;
- nombre de scans ;
- données de pré-entraînement ;
- TTA et ensemble ;
- split et métrique ;
- code ou configuration suffisamment précise.

À défaut, elle reste dans le contexte bibliographique sans être placée dans une table numérique appariée.
