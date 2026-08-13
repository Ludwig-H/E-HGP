# Références primaires

Veille effectuée le 13 août 2026. Les liens privilégient les pages officielles des benchmarks, les actes, OpenReview, les éditeurs et les dépôts des auteurs. Les chiffres de leaderboard sont des instantanés datés.

## SemanticKITTI et évaluations

- Behley et al., *SemanticKITTI: A Dataset for Semantic Scene Understanding of LiDAR Sequences*, ICCV 2019. [Actes CVF](https://openaccess.thecvf.com/content_ICCV_2019/html/Behley_SemanticKITTI_A_Dataset_for_Semantic_Scene_Understanding_of_LiDAR_Sequences_ICCV_2019_paper.html).
- Site officiel, [dataset et format](https://semantic-kitti.org/dataset.html). Les labels `uint32` encodent la classe dans les 16 bits bas et l'instance dans les 16 bits hauts.
- Site officiel, [définition des tâches et métriques](https://semantic-kitti.org/tasks.html).
- PRBonn, [SemanticKITTI API et configuration officielle](https://github.com/PRBonn/semantic-kitti-api), notamment `config/semantic-kitti.yaml`.
- CodaBench, [compétition single-scan actuelle](https://www.codabench.org/competitions/12448/) et [API du leaderboard](https://www.codabench.org/api/phases/20274/get_leaderboard/?page=1&page_size=100).
- CodaLab, [archive de l'ancien leaderboard](https://codalab.lisn.upsaclay.fr/competitions/6280/results/9324) et [export officiel](https://codalab.lisn.upsaclay.fr/competitions/6280/results/9324/data).
- Site officiel, [JSON des résultats publiés single-scan](https://semantic-kitti.org/data/semantic_single.json), `last_modified=2025-06-02`. Cette liste est incomplète et rapporte les nombres des papiers.
- Behley et al., *A Benchmark for LiDAR-based Panoptic Segmentation based on KITTI*, ICRA 2021. [PDF auteurs](https://www.ipb.uni-bonn.de/pdfs/behley2021icra.pdf). Référence pour la future extension, pas pour la phase sémantique active.

## HGP, cluster trees et hiérarchies de densité

- Hauseux, Avrachenkov et Zerubia, *Generalization of single-linkage with higher-order interactions*, Applied Network Science, 2026. [Article éditeur](https://link.springer.com/article/10.1007/s41109-025-00756-1), [PDF](https://link.springer.com/content/pdf/10.1007/s41109-025-00756-1.pdf).
- Chaudhuri et Dasgupta, *Rates of Convergence for the Cluster Tree*, NeurIPS 2010. [Actes NeurIPS](https://proceedings.neurips.cc/paper_files/paper/2010/hash/b534ba68236ba543ae44b22bd110a1d6-Abstract.html). Fondement de Robust Single Linkage et de la consistance Hartigan.
- Campello, Moulavi et Sander, *Density-Based Clustering Based on Hierarchical Density Estimates*, PAKDD 2013. [DOI](https://doi.org/10.1007/978-3-642-37456-2_14). Source primaire de HDBSCAN et de sa sélection de clusters par stabilité.
- Balakrishnan et al., *Cluster Trees on Manifolds*, NeurIPS 2013. [PDF NeurIPS](https://papers.neurips.cc/paper/4984-cluster-trees-on-manifolds.pdf).
- Eldridge, Belkin et Wang, *Beyond Hartigan Consistency: Merge Distortion Metric for Hierarchical Clustering*, COLT 2015. [PMLR](https://proceedings.mlr.press/v40/Eldridge15.html). Référence pour distinguer consistance de Hartigan, minimalité/séparation et proximité quantitative des niveaux de fusion.
- Biau et Devroye, *Lectures on the Nearest Neighbor Method*, Springer, 2015. [DOI](https://doi.org/10.1007/978-3-319-25388-6). Référence pour les régimes asymptotiques des estimateurs aux plus proches voisins ; le dossier ne suppose pas qu'un ordre $K$ fixe donne une estimation populationnelle consistante.

## Attention hiérarchique

- Amizadeh et al., *Hierarchical Self-Attention: Generalizing Neural Attention Mechanics to Multi-Scale Problems*, NeurIPS 2025. [Page officielle](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html), [PDF officiel](https://proceedings.neurips.cc/paper_files/paper/2025/file/0480adaf62a918405a5e3b1031e0c056-Paper-Conference.pdf), [copie locale](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf). La copie locale et le PDF officiel ont le SHA-256 `3658cec3bd1dacbe63c6daaf61ebed3a79f0c621bc9a28a30f52889d576c0aa1` lors de la vérification.
- Trang et al., *Scalable Hierarchical Self-Attention with Learnable Hierarchy for Long-Range Interactions*, TMLR 2024. [OpenReview](https://openreview.net/forum?id=qH4YFMyhce), [code auteurs](https://github.com/HySonLab/HierAttention).
- Cirrincione, *Hierarchical Kernel Transformer: Multi-Scale Attention with an Information-Theoretic Approximation Analysis*, prépublication arXiv, 2026. [arXiv](https://arxiv.org/abs/2604.08829). Concurrent théorique récent sur séquences ; pas de point cloud ni d'arbre HGP.
- Kang, Tran et De Sterck, *Fast Multipole Attention: A Scalable Multilevel Attention Mechanism for Text and Images*, prépublication arXiv, version 4 de 2025. [arXiv](https://arxiv.org/abs/2310.11960). Précédent le plus proche d'interactions requête fine–groupes cibles multi-échelles.
- Zhu et Soricut, *H-Transformer-1D: Fast One-Dimensional Hierarchical Attention for Sequences*, ACL-IJCNLP 2021. [ACL Anthology](https://aclanthology.org/2021.acl-long.294/). Attention à structure H-matrix et complexité linéaire sur séquences.
- Zeng et al., *Multi Resolution Analysis (MRA) for Approximate Self-Attention*, ICML 2022. [PMLR](https://proceedings.mlr.press/v162/zeng22a.html). Approximation et raffinement multi-résolution de l'attention.
- Csiszár, *I-Divergence Geometry of Probability Distributions and Minimization Problems*, Annals of Probability, 1975. [DOI](https://doi.org/10.1214/aop/1176996454). Fondement classique des projections en divergence ; la seule utilisation d'une projection KL n'est pas une nouveauté.
- Hoeffding, *Probability Inequalities for Sums of Bounded Random Variables*, JASA 1963. [DOI](https://doi.org/10.1080/01621459.1963.10500830). Source de la borne exponentielle utilisée pour contrôler le défaut intra-bloc.
- Fedotov, Harremoës et Topsøe, *Refinements of Pinsker's Inequality*, IEEE Transactions on Information Theory, 2003. [DOI](https://doi.org/10.1109/TIT.2003.813506). Référence pour le passage KL–variation totale.
- Chou, Lookabaugh et Gray, *Optimal Pruning with Applications to Tree-Structured Source Coding and Modeling*, IEEE Transactions on Information Theory, 1989. [DOI](https://doi.org/10.1109/18.32124). Antériorité pour l'élagage débit–distorsion d'un arbre.
- Lin, Storer et Cohn, *Optimal Pruning for Tree-Structured Vector Quantization*, Information Processing & Management, 1992. [DOI](https://doi.org/10.1016/0306-4573(92)90064-7). Complexité de l'élagage optimal sous différents budgets.

## Segmentation sémantique LiDAR forte

- Li et al., *RAPiD-Seg: Range-Aware Pointwise Distance Distribution Networks for 3D LiDAR Segmentation*, ECCV 2024. [Page ECVA](https://www.ecva.net/papers/eccv_2024/papers_ECCV/html/1129_ECCV_2024_paper.php), [PDF](https://www.ecva.net/papers/eccv_2024/papers_ECCV/papers/01129.pdf). Rapporte 76,1 mIoU SemanticKITTI avec un pipeline class-aware appris en deux passes ; le statut TTA doit être audité avant comparaison stricte.
- Feng et al., *LSK3DNet: Towards Effective and Efficient 3D Perception with Large Sparse Kernels*, CVPR 2024. [Actes CVF](https://openaccess.thecvf.com/content/CVPR2024/html/Feng_LSK3DNet_Towards_Effective_and_Efficient_3D_Perception_with_Large_Sparse_CVPR_2024_paper.html), [arXiv](https://arxiv.org/abs/2403.15173). Rapporte 75,6 mIoU test single-scan avec instance CutMix, TTA et entraînement test prolongé.
- Wu et al., *TASeg: Temporal Aggregation Network for LiDAR Semantic Segmentation*, CVPR 2024. [PDF CVF](https://openaccess.thecvf.com/content/CVPR2024/papers/Wu_TASeg_Temporal_Aggregation_Network_for_LiDAR_Semantic_Segmentation_CVPR_2024_paper.pdf). Rapporte 76,5 mIoU, avec entrées temporelles LiDAR et image.
- Wu et al., *Point Transformer V3: Simpler, Faster, Stronger*, CVPR 2024. [Actes CVF](https://openaccess.thecvf.com/content/CVPR2024/html/Wu_Point_Transformer_V3_Simpler_Faster_Stronger_CVPR_2024_paper.html), [code Pointcept](https://github.com/Pointcept/PointTransformerV3). Rapporte 74,2 test pour PTv3 sur SemanticKITTI dans le supplément ; le dépôt officiel ne fournit pas actuellement un paquet SemanticKITTI complet config+poids+résultat.
- Wu et al., *Towards Large-scale 3D Representation Learning with Multi-dataset Point Prompt Training*, CVPR 2024. [PDF CVF](https://openaccess.thecvf.com/content/CVPR2024/papers/Wu_Towards_Large-scale_3D_Representation_Learning_with_Multi-dataset_Point_Prompt_Training_CVPR_2024_paper.pdf). PTv3+PPT rapporte 75,5 avec entraînement multi-datasets.
- Wan et al., *SP2T: Sparse Proxy Attention for Dual-stream Point Transformer*, ICCV 2025. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2025/html/Wan_SP2T_Sparse_Proxy_Attention_for_Dual-stream_Point_Transformer_ICCV_2025_paper.html), [code auteurs](https://github.com/WallelWan/SP2T). Rapporte 71,7 validation et 75,4 test sur SemanticKITTI avec LiDAR mono-trame ; le statut TTA outdoor n'est pas rapporté dans la source auditée.
- Li et al., *RWAFormer: a Lightweight Road LiDAR Point Cloud Segmentation Network Based on Transformer*, Frontiers in Computer Science, 2025. [DOI](https://doi.org/10.3389/fcomp.2025.1542813). Le papier appelle la séquence 08 « independent test set » ; son 75,3 ne doit donc pas être confondu avec le test caché 11–21.
- Lai et al., *Spherical Transformer for LiDAR-Based 3D Recognition*, CVPR 2023. [Actes CVF](https://openaccess.thecvf.com/content/CVPR2023/html/Lai_Spherical_Transformer_for_LiDAR-Based_3D_Recognition_CVPR_2023_paper.html). La méthode SphereFormer rapporte 74,8 test.
- Liu et al., *UniSeg: A Unified Multi-Modal LiDAR Segmentation Network and the OpenPCSeg Codebase*, ICCV 2023. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Liu_UniSeg_A_Unified_Multi-Modal_LiDAR_Segmentation_Network_and_the_OpenPCSeg_ICCV_2023_paper.html), [code primaire PCSeg](https://github.com/PJLab-ADG/PCSeg). Le dépôt a depuis été transféré/redirigé vers OpenPCSeg. UniSeg rapporte 75,2 test et utilise RGB/point/voxel/range.
- Liu et al., *Multi-Space Alignments Towards Universal LiDAR Segmentation*, CVPR 2024. [PDF CVF](https://openaccess.thecvf.com/content/CVPR2024/papers/Liu_Multi-Space_Alignments_Towards_Universal_LiDAR_Segmentation_CVPR_2024_paper.pdf). Rapporte 75,1 avec entraînement joint SemanticKITTI/nuScenes/Waymo.
- Knaebel et al., *DINO in the Room: Leveraging 2D Foundation Models for 3D Segmentation*, 3DV 2026. [arXiv](https://arxiv.org/abs/2503.18944). Le modèle DITR rapporte 74,4 avec une branche image DINOv2 ; aucune valeur test image-free correspondante n'est fournie.
- Peng et al., *OA-CNNs: Omni-Adaptive Sparse CNNs for 3D Semantic Segmentation*, CVPR 2024. [Actes CVF](https://openaccess.thecvf.com/content/CVPR2024/html/Peng_OA-CNNs_Omni-Adaptive_Sparse_CNNs_for_3D_Semantic_Segmentation_CVPR_2024_paper.html). Rapporte 70,6 sur validation SemanticKITTI et sert de rappel qu'un CNN sparse adaptatif peut être plus efficace qu'un Transformer.
- Yang et al., *FLARES: Fast and Accurate LiDAR Multi-Range Semantic Segmentation*, WACV 2026. [Actes CVF](https://openaccess.thecvf.com/content/WACV2026/html/Yang_FLARES_Fast_and_Accurate_LiDAR_Multi-Range_Semantic_Segmentation_WACV_2026_paper.html). Référence récente sur le traitement range-view, la recette de données et la vitesse.

## Hiérarchies, superpoints et Transformers 3D

- Robert et al., *Efficient 3D Semantic Segmentation with Superpoint Transformer*, ICCV 2023. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html), [code auteurs](https://github.com/drprojects/superpoint_transformer).
- Geist, Landrieu et Robert, *EZ-SP: Fast and Lightweight Superpoint-Based 3D Segmentation*, ICRA 2026. [arXiv](https://arxiv.org/abs/2512.00385).
- Lu et al., *Serialization based Point Cloud Oversegmentation*, ICCV 2025. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2025/html/Lu_Serialization_based_Point_Cloud_Oversegmentation_ICCV_2025_paper.html), [code auteurs](https://github.com/CHL-glitch/SPCNet). Oversegmentation apprise, cross-attention point–superpoint et hiérarchie à deux niveaux, évaluées notamment sur SemanticKITTI.
- Wang, *OctFormer: Octree-based Transformers for 3D Point Clouds*, SIGGRAPH 2023. [arXiv](https://arxiv.org/abs/2305.03045), [code auteur](https://github.com/octree-nn/octformer).
- Feng et al., *Clustering based Point Cloud Representation Learning for 3D Analysis*, ICCV 2023. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2023/html/Feng_Clustering_based_Point_Cloud_Representation_Learning_for_3D_Analysis_ICCV_2023_paper.html).
- Robert, Raguet et Landrieu, *Scalable 3D Panoptic Segmentation As Superpoint Graph Clustering*, 3DV 2024. [arXiv](https://arxiv.org/abs/2401.06704).
- Liang et al., *Instance Segmentation in 3D Scenes using Semantic Superpoint Tree Networks*, ICCV 2021. [Actes CVF](https://openaccess.thecvf.com/content/ICCV2021/html/Liang_Instance_Segmentation_in_3D_Scenes_Using_Semantic_Superpoint_Tree_Networks_ICCV_2021_paper.html).
- Qin et al., *Unified 3D Segmenter As Prototypical Classifiers*, NeurIPS 2023. [Page officielle](https://papers.neurips.cc/paper_files/paper/2023/hash/916cb4e1aeafaa0757953c9bacd17337-Abstract-Conference.html).

## Fonction support, robustesse et descripteurs 3D

- Cramér et Wold, *Some Theorems on Distribution Functions*, Journal of the London Mathematical Society, 1936. [DOI](https://doi.org/10.1112/jlms/s1-11.4.290). La totalité des distributions projetées en une dimension détermine une mesure de probabilité ; une grille finie de directions et de bins reste seulement un sketch.
- Schneider, *Convex Bodies: The Brunn–Minkowski Theory*, Cambridge University Press. [DOI](https://doi.org/10.1017/CBO9780511526282). Référence pour support, reconstruction convexe et distance de Hausdorff.
- Bronshtein et Ivanov, *The Approximation of Convex Sets by Polyhedra*, Siberian Mathematical Journal, 1975. [Math-Net](https://www.mathnet.ru/eng/smj4199), [DOI](https://doi.org/10.1007/BF00967115). Les taux optimaux adaptatifs ne sont pas une garantie directe pour une grille arbitraire de directions fixes.
- Brauchart et al., *Covering of Spheres by Spherical Caps and Worst-Case Error for Equal Weight Cubature in Sobolev Spaces*, JMAA 2015. [arXiv](https://arxiv.org/abs/1407.8311), [DOI](https://doi.org/10.1016/j.jmaa.2015.05.079).
- Kong et Mizera, *Quantile Tomography: Using Quantiles with Multivariate Data*, Statistica Sinica 2012. [arXiv](https://arxiv.org/abs/0805.0056), [DOI](https://doi.org/10.5705/ss.2010.224). Référence pour quantiles directionnels et régions de profondeur.
- Nesterov, *Smooth Minimization of Non-smooth Functions*, Mathematical Programming 2005. [DOI](https://doi.org/10.1007/s10107-004-0552-5). Référence pour le lissage log-sum-exp.
- Osada et al., *Shape Distributions*, ACM TOG 2002. [Page auteurs](https://gfx.cs.princeton.edu/pubs/Osada_2002_SD/index.php).
- Kazhdan, Funkhouser et Rusinkiewicz, *Rotation Invariant Spherical Harmonic Representation of 3D Shape Descriptors*, SGP 2003. [Page auteurs](https://gfx.cs.princeton.edu/pubs/Kazhdan_2003_RIS/index.php).
- Qi et al., *PointNet: Deep Learning on Point Sets for 3D Classification and Segmentation*, CVPR 2017. [Actes CVF](https://openaccess.thecvf.com/content_cvpr_2017/html/Qi_PointNet_Deep_Learning_CVPR_2017_paper.html).
- Wu et al., *PointConv: Deep Convolutional Networks on 3D Point Clouds*, CVPR 2019. [PDF CVF](https://openaccess.thecvf.com/content_CVPR_2019/papers/Wu_PointConv_Deep_Convolutional_Networks_on_3D_Point_Clouds_CVPR_2019_paper.pdf).
- Thomas et al., *Tensor Field Networks: Rotation- and Translation-Equivariant Neural Networks for 3D Point Clouds*, 2018. [arXiv](https://arxiv.org/abs/1802.08219).
- Fuchs et al., *SE(3)-Transformers: 3D Roto-Translation Equivariant Attention Networks*, NeurIPS 2020. [Page officielle](https://papers.neurips.cc/paper/2020/hash/15231a7ce4ba789d13b722cc5c955834-Abstract.html).

## Instance future

- Sautier et al., *Is clustering enough for LiDAR instance segmentation? A state-of-the-art training-free baseline*, 3DV 2026. [arXiv](https://arxiv.org/abs/2503.13203), [PDF OpenReview](https://openreview.net/pdf?id=ymClQqU3is), [code auteurs](https://github.com/valeoai/Alpine). À utiliser seulement après validation sémantique, avec logits gelés identiques.
