# État de l'art et espace de nouveauté

Veille arrêtée au **14 août 2026**, sources primaires. Ce fichier est la référence unique du dossier sur « qui fait quoi, et dans quel régime » : chaque chiffre y porte son régime, et aucun n'est comparable à un autre sans lui.

## Avertissement liminaire

1. **Il n'existe aucun classement officiel attribuable méthode par méthode.** `semantic-kitti.org/tasks.html` annonce un classement des « approches publiées avec au moins un lien arXiv », mais la page ne contient qu'un `<!--PLACEHOLDER FOR SINGLE SCAN LEADERBOARD-->` et un « Last updated: » vide.
2. **Le serveur a déménagé et le classement a redémarré à zéro.** CodaBench, seul serveur vivant, ne reporte aucune entrée historique ; son maximum est 75,21 et ses cinq têtes sont pseudonymes, donc non citables.
3. **Le rang 1 historique s'appelle `SimpleSeg`** : 76,5, huit soumissions, aucune publication trouvable. Le « record SemanticKITTI » n'a pas d'article.

Toute citation de « l'état de l'art SemanticKITTI » doit donc préciser sa source **et** son régime, sous peine de comparer un serveur mort, un serveur remis à zéro et une page vide.

### Les trois serveurs

| Serveur | État au 14 août 2026 | Contenu |
|---|---|---|
| `codalab.org/competitions/20331` | mort depuis le 31 août 2022 | — |
| `codalab.lisn.upsaclay.fr/competitions/6280` | gelé, bannière « we transferred the competition to Codabench! », fin le 31 janvier 2026 | historique complet : 381 utilisateurs, phase single scan 9324, dernière entrée 2026-01-19 |
| `codabench.org/competitions/12448` | vivant, créé le 2025-12-31 | phase unique « Single Scan » (id 20274, leaderboard 13733) : 108 participants, 517 soumissions, 62 lignes |

Le multi-scan est désormais une compétition distincte (CodaBench 17382, créée le 2026-07-04, 2 soumissions).

Tête CodaBench : `kadir_yilmaz` 75,2142 (2026-03-03), `xoxosos` 74,9406 (2026-03-30), `pic-iii` 72,7475, `nwfi-ll` 72,3853, `yutta` 72,0568 (2026-08-07). Pour toutes, `display_name`, `organization` et `fact_sheet_answers` sont nuls et les résultats détaillés en `AccessDenied` : aucune n'est scientifiquement citable.

Classement gelé (CSV officiel, mIoU à 0,1) : `SimpleSeg` 76,5 rang 1 (2023-11-19, aucune publication), TASeg 76,5 rang 2 (2023-11-18), RAPiD 76,1, `Cluster3DSeg_` 75,6, `Seger` 75,5, `PointTransformers` 75,5, `PointSeg` 75,3, UniSeg 75,2, SphereFormer 74,8, `kabouzeid` 74,4 (2024-11-13). Aucune entrée postérieure au 1er janvier 2024 ne dépasse 74,4 ; aucune postérieure au 1er janvier 2025 ne dépasse 73,7.

Sources : [ancien classement](https://codalab.lisn.upsaclay.fr/competitions/6280/results/9324), [compétition vivante](https://www.codabench.org/competitions/12448/), [API du leaderboard](https://www.codabench.org/api/phases/20274/get_leaderboard/?page=1&page_size=100).

## Ce que « single scan » veut dire, et ne veut pas dire

C'est le piège principal du benchmark. Le texte officiel évalue 25 classes sur 28 : « Single scan ... we combine the moving classes with the corresponding non-moving class resulting in a total number of 19 classes for evaluation » ; « Multiple scans : we evaluate all 28 classes including moving and non-moving ». **La différence formelle entre les deux tracks est uniquement le jeu d'étiquettes, pas l'entrée.**

L'intention était mono-trame, et l'inobservabilité est avouée — Behley, issue `PRBonn/semantic-kitti-api` #4, 31 juillet 2019, verbatim :

> a. Single scan: You take only scan 245 and predict 245 with this information... b. Multi scan: You take information from 245, 244, 243, 242, 241... (You can also take more scans, we don't care. Since we only see the results of scan 245.)

Ce que cela autorise en pratique : TASeg (CVPR 2024) fixe « the window size of temporal point clouds is set to 16 » et utilise **en plus** des images caméra temporelles (fenêtre 48). Sa table 11 en validation donne « TASeg wo/ TIAF — L+T — 37,9 M — 79 ms — 71,8 » et « TASeg — L+C+T — 46,7 M — 116 ms — 72,7 ». Le 76,5 test est donc multimodal et temporel, soumis au track single scan, accepté par le serveur.

Conséquence pour ce dossier : le track « single scan » ne certifie aucun régime d'entrée. Seul le texte de l'article le fait, et son silence ne vaut pas déclaration négative.

## La barre en validation, notre régime

Régime de ce projet : séquence 08, **une trame à l'inférence, LiDAR seul, sans TTA ni ensemble, entraînement mono-jeu**. Les tracks de comparaison sont définis dans [PROTOCOLE.md](PROTOCOLE.md#régimes-de-comparaison).

| Recette | mIoU val | Config | Poids | Log | Notes |
|---|---:|:-:|:-:|:-:|---|
| MinkUNet34v2-W32, mmdetection3d, torchsparse + AMP + laser-polar-mix, 3x | **70,3** | oui | oui | oui | `tta_model`/`tta_pipeline` présents mais activés seulement par `--tta` : le 70,3 est bien sans TTA. Le README avertit d'environ 1,5 mIoU de fluctuation selon la graine |
| MinkowskiNet, OpenPCSeg/PCSeg | **70,04** | oui | oui | oui | « trained with merely train split », « without employing any Test Time Augmentation or ensembling » ; repris comme baseline par RAPiD-Seg |
| RPVNet, OpenPCSeg | 68,86 | oui | oui | oui | même protocole déclaré |
| SPVCNN, OpenPCSeg | 68,58 | oui | oui | oui | même protocole déclaré |
| WaffleIron-48-256 | 68,0 | oui | oui | non | README garantit « a final mIoU of 68.0% », 6,8 M paramètres, `instance_cutmix` à l'entraînement ; **aucune extension CUDA à compiler** (ni torchsparse, ni MinkowskiEngine, ni spconv, ni flash-attn), installation pip sous PyTorch 2.2 ; ablation val 62,5 -> 66,8 (cutmix+polarmix) -> 67,6 (features 5D) -> 68,0 (stochastic depth) ; aucun chiffre val avec TTA n'a jamais été publié, le 68,0 est nu |
| SphereFormer | 67,8 | oui | oui | non | table du dépôt : « Val mIoU (tta) 69.0 » contre « Val mIoU 67.8 » |
| Cylinder3D, OpenPCSeg | 66,07 | oui | oui | oui | même protocole déclaré |

**La barre à battre est 70,3 (mmdetection3d) ou 70,04 (OpenPCSeg), et non 68.** Ces deux lignes sont les seules à publier config, poids et log dans notre régime exact.

### Pourquoi le tableau de VaViT n'est pas une table d'état de l'art

VaViT ([arXiv 2605.31177](https://arxiv.org/abs/2605.31177), valeo.ai, 29 mai 2026), table 3 « results without test-time augmentation » : MinkUNet 63,8 ; Cylinder3D 64,3 ; SPVNAS 64,7 ; FlatFormer-S 65,3 ; PTv3 reproduit 66,2 ; VaViT-B 67,6 ; SphereFormer 67,8 ; VaViT-B\* 68,0 ; WaffleIron-256 68,0. C'est une **comparaison d'architectures à protocole apparié** : quatre des neuf chiffres sont recopiés de l'article PTv3, `*` signifie « meilleure époque et non la dernière », et VaViT ne soumet rien au test.

Surtout, le 63,8 de MinkUNet mesure une recette de 2019, pas une architecture : l'étude empirique [arXiv 2405.14870](https://arxiv.org/abs/2405.14870) porte le même MinkUNet à 71,8 en validation — « improved by 5.0% with our default settings, and an additional 3.5% with mixing data augmentation. TTA further boosted performance by 1.4% ». Opposer 63,8 à 68,0 compare donc des protocoles d'entraînement, pas des modèles.

### « Mono-trame » ne vaut qu'à l'inférence

WaffleIron, VaViT, MinkUNet mmdetection3d et LSK3DNet mélangent tous des scans **à l'entraînement** : instance CutMix, LaserMix, PolarMix, PillarMix. La contrainte mono-trame porte sur l'entrée du réseau au test, jamais sur la construction des lots ; l'exiger à l'entraînement fabriquerait une baseline artificiellement faible.

## PTv3 : ce qu'il faut savoir avant de s'en servir

| Chiffre val | Origine |
|---:|---|
| 70,8 annoncé (test 74,2 ; PTv3+PPT 72,3 / 75,5) | article CVPR 2024 |
| 66,2 sans TTA, 68,8 avec TTA | VaViT |
| 68,3 | DITR, 3DV 2026 |
| 66,8 | meilleur de trois runs communautaires, Pointcept #410 |
| 69,30 / 66,52 / 66,53 | trois reproductions indépendantes, Pointcept #556, ouverte le 13 janvier 2026 |
| 69,1 | cité par Sonata, même premier auteur, repris par Volt |

L'issue primaire est Pointcept #186, « About the config file of PTv3 on semantickitti », ouverte le 27 mars 2024 et **toujours ouverte** en août 2026, citée par VaViT et par DITR. Dans #410, le mainteneur Gofinge écrit : « I am pretty sure these number in the paper is exactly observed with my experiments. Unfortunately, I cannot access the original experiment record ». DITR résume : « Reproducing PTv3 results on the SemanticKITTI dataset has been notoriously hard for the community ». LitePT (CVPR 2026) déclare « we follow PTv3 and use test time augmentation (TTA) » avec chunking et vote, et chiffre le retrait à environ 2 mIoU : le 70,8 n'appartient donc probablement pas au régime sans TTA.

Dans #481, le mainteneur admet « I already forgot where I got this number during paper writing ».

**Fait décisif pour le choix de baseline** : `configs/semantic_kitti/` de Pointcept (dépôt poussé le 3 août 2026) ne contient **aucune** config PTv3, alors que nuScenes, Waymo, ScanNet et S3DIS en ont une, et les lignes SemanticKITTI du model zoo PointTransformerV3 sont vides — ni config, ni poids, ni exp record. Pointcept n'implémente par ailleurs ni LaserMix ni PolarMix (0 occurrence), ce qui explique probablement une part de l'écart annoncé/reproduit. Pointcept est donc inutilisable pour PTv3 sur SemanticKITTI, et PTv3 ne peut pas être la baseline de départ de WP0 ; s'il est utilisé malgré tout, reporter sa propre baseline réentraînée, jamais le 70,8 du papier.

## Le test, et pourquoi ses chiffres ne sont pas comparables

| Méthode | Test | Val | Régime déclaré | Régime vérifié |
|---|---:|---:|---|---|
| TASeg, CVPR 2024 | **76,5** | 72,7 | fenêtre LiDAR 16 + images caméra fenêtre 48 | temporel et multimodal, confirmé par l'article |
| RAPiD-Seg, ECCV 2024 Oral | **76,1** | 73,02 | « single-modal (LiDAR-only) », un seul passage avant par trame, 105 ms | LiDAR seul confirmé ; **aucune occurrence** de « test-time augmentation », « TTA », « ensemble », « voting » ni « multi-frame » : absence de mention, pas déclaration négative ; rien sur un entraînement train+val ; écart val→test de +3,1 inexpliqué |
| LSK3DNet | 75,6 | 70,2 | « We apply instance CutMix and Test Time Augmentation (TTA) ... and enhance the model with extra training epochs » | augmenté, honnêtement déclaré ; 28,8 M paramètres |
| PTv3 + PPT, CVPR 2024 | 75,5 | 72,3 | préentraînement multi-jeux | multi-jeux |
| UniSeg, ICCV 2023 | 75,2 | — | multimodal RGB | multimodal |
| SphereFormer, CVPR 2023 | 74,8 | 67,8 | rien de déclaré pour le test | écart val→test de **+7,0** totalement non documenté |
| PTv3 seul, CVPR 2024 | 74,2 | 70,8 annoncé | — | val non reproductible, cf. section précédente |
| 2DPASS | 72,9 | — | dépôt officiel, issue #13 : « The results on benchmarks are gained by training with additional validation set and using instance-level augmentation » | reproduction propre : 68,2, soit 4,7 points imputables au seul protocole |
| WaffleIron | 70,8 | 68,0 | « 10 different augmentations » moyennées, entraînement train+val, « We do not use model ensemble to boost the test or validation performance » | déclaré et cohérent |

RAPiD-Seg (backbone MinkUNet34 réimplémenté par PCSeg, baseline 70,04 ; C-RAPiD-Seg 73,02 val, R-RAPiD-Seg 72,3) est **le meilleur candidat LiDAR seul** de ce tableau, mais sa recette est silencieuse, pas certifiée propre : plausiblement stricte, non auditable. Aucune valeur de cette colonne test ne borne un résultat obtenu en régime strict, ni par le haut ni par le bas.

## 2025-2026 : où en est le domaine

| Travail | Chiffres | Régime |
|---|---|---|
| Volt / Volume Transformer, RWTH Aachen, [arXiv 2604.19609](https://arxiv.org/abs/2604.19609), 21 avril 2026 | test **75,2** (Volt-B), val 72,5 ; Volt-S 70,5 mono-jeu et 72,2 multi-jeux ; nuScenes val 82,2 | entraînement conjoint multi-jeux ; meilleur test 2026 identifié, toujours sous 76,5 ; présomption forte mais **non confirmée** que `kadir_yilmaz` 75,21 = Volt |
| Sonata, CVPR 2025 Highlight, [arXiv 2503.16429](https://arxiv.org/abs/2503.16429) | val **72,6** | fine-tuning complet d'un PTv3 préentraîné en auto-supervisé multi-jeux ; meilleur val LiDAR seul recensé, mais poids extérieurs non publiés — section « Sonata et sa lignée » |
| UniD-Shift, [arXiv 2605.07356](https://arxiv.org/abs/2605.07356), 8 mai 2026 | val 71,8 | fusion LiDAR + caméra, 359,8 M paramètres, 240 ms |
| SP2T, ICCV 2025, [arXiv 2412.11540](https://arxiv.org/abs/2412.11540) | val 71,7 / test 75,4 | TTA, sans ensemble |
| OA-CNNs, CVPR 2024, [arXiv 2403.14418](https://arxiv.org/abs/2403.14418) | val 70,6 | sans TTA déclarée |
| DITR puis D-DITR, 3DV 2026, [arXiv 2503.18944](https://arxiv.org/abs/2503.18944) | val 69,0 / test 74,4 ; D-DITR val 69,8 | DITR projette des features DINOv2 à l'inférence ; D-DITR les distille et reste LiDAR seul à l'inférence ; correspond à l'entrée `kabouzeid` 74,4 |
| LitePT, CVPR 2026 | **aucun chiffre SemanticKITTI** | nuScenes 82,2 / Waymo 73,1 / ScanNet / Structured3D seulement : toute ligne SemanticKITTI attribuée à LitePT serait fabriquée |

Constat global : aucun nouveau SOTA entre le 29 mai et le 14 août 2026, et **aucun dépassement de 76,5 depuis 2023**. Le domaine a migré vers nuScenes, Waymo, ScanNet, l'occupancy et la complétion de scène ; CVPR 2026 n'évalue plus systématiquement SemanticKITTI ; Volt, VaViT et DITR ne soumettent plus au serveur. Un benchmark qui stagne récompense peu un gain marginal : c'est un argument de [STRATEGIE_PUBLICATION.md](archive/STRATEGIE_PUBLICATION.md), pas seulement de veille.

Ces chiffres sont des instantanés et non des constantes : ils doivent être réaudités contre leurs sources primaires avant toute soumission.

## Leçons pour HGP-HSA

- **Baseline WP0 : WaffleIron-48-256**, et non OpenPCSeg comme recommandé précédemment. Motif décisif : config unique `configs/WaffleIron-48-256__kitti.yaml`, checkpoint vivant, aucune extension CUDA à compiler, et un mainteneur qui répond précisément (Gilles Puy, #19 : 68,0 val et 70,8 test avec `--trainval` et 12 votes ; #5 : 4x RTX 2080 Ti, environ 2 jours pour 45 époques, un seul V100 32 Go suffit).
- **Second porteur : MinkUNet**, famille radicalement différente (convolution sparse voxelisée contre MLP et convolutions 2D denses sur projections) : mmdetection3d `minkunet34_w32_spconv` 69,3 avec poids et log vivants, variante torchsparse annoncée à 70,3, OpenPCSeg 70,04. Attention : OpenPCSeg annonce 70,04 en liant `minkunet_mk34_cr10.yaml` alors que le fichier de poids s'appelle `mk34_cr16` — vérifier l'appariement config/poids avant de citer.
- **Le facteur dominant est la recette, pas l'architecture** : LaserMix/PolarMix valent +3,5 sur MinkUNet (66,9 -> 70,4), instance cutmix + polarmix +4,3 sur WaffleIron (62,5 -> 66,8), soit plus que l'écart entre la plupart des architectures publiées. Les deux bras d'une évaluation HGP doivent activer les mêmes augmentations, sinon le gain mesuré est un gain d'augmentation déguisé.
- **Plancher de bruit** : environ 1,5 mIoU de fluctuation selon la graine (avertissement mmdetection3d), 66,5 à 69,3 selon GPU et batch (Pointcept #556). Tout gain sous environ 1,5 point sur un run unique n'est pas distinguable du bruit : trois graines minimum par bras, et jamais de comparaison sans TTA contre avec TTA (+1,4 MinkUNet, +2,4 Cylinder3D, +1,2 SphereFormer ; la TTA mmdetection3d coûte 36 passes avant).
- **RAPiD-Seg** montre qu'un descripteur géométrique doit intégrer la variation de densité avec la portée et la rémission. C'est le contrôle le plus direct de la fonction support, et son val 73,02 le comparateur le plus exigeant — sous réserve d'une recette non auditable.
- **SphereFormer** encode déjà la géométrie sphérique du capteur. Un gain HGP limité aux longues distances doit être comparé à ce biais, pas à un modèle cartésien naïf ; son 67,8 val sans TTA est le point de comparaison, pas son 74,8 test.
- **LSK3DNet** rappelle qu'une attention sophistiquée doit battre un CNN sparse adaptatif en précision ou sur un axe Pareto clair ; son 75,6 test est un chiffre TTA déclaré, hors de notre régime.
- **SP2T** est un concurrent conceptuel direct : son double flux et ses proxies locaux réduisent l'attention point–point tout en conservant du contexte sparse. Le gain HGP doit être isolé d'un simple effet de tokens proxy.
- **VaViT** fournit une baseline ViT globale publique, sans TTA, à 68,0 val : utile pour la reproductibilité, pas comme seuil, et son tableau n'est pas un état de l'art.
- **TASeg, UniSeg et DITR** prouvent la valeur des ressources supplémentaires — temps, caméra, préentraînement externe. Ils restent dans des colonnes distinctes pour ne pas diluer le claim LiDAR mono-trame.

## Sonata et sa lignée

[Sonata](https://arxiv.org/abs/2503.16429) (CVPR 2025 Highlight, Wu et al., Meta + HKU) : PTv3 de 108 M paramètres, encodeur seul, auto-distillation de type DINOv2 — enseignant EMA, Sinkhorn-Knopp, KoLeo, 4096 prototypes. Vues : 2 globales (40-100 % des points), 4 locales (5-40 %), 2 masquées. **Les unités de la tâche prétexte sont donc des sous-ensembles aléatoires**, jamais des régions structurées.

Son diagnostic est le *geometric shortcut*, verbatim : « This shortcut refers to the tendency of the model to collapse to easily accessible, low-level geometric cues, such as normal direction or point height. This spatial information is inevitably introduced into point cloud operators along with point coordinates rather than through input features, making it difficult to obscure and nearly impossible to mask effectively. » Le grief porte sur les **coordonnées**, pas sur les features d'entrée : Sonata fournit lui-même les normales (`feat_keys=("coord","color","normal")`). Preuve quantitative : linear probing ScanNet à 5,6 (PointContrast) et 21,8 (MSC), contre 63,1 pour des features DINOv2 reprojetées. Remèdes : suppression du décodeur (20,7 -> 60,4 à elle seule), jitter accru sur les points masqués, curriculum sur la taille et le ratio de masque.

Table 8, tout en validation :

| Val | Sonata linéaire | Sonata décodeur | PPT supervisé | Sonata fine-tuning | PTv3 supervisé |
|---|---:|---:|---:|---:|---:|
| SemanticKITTI | 62,0 | 68,4 | 72,3 | **72,6** | 69,1 |
| nuScenes | 66,1 | 77,3 | 81,2 | 81,7 | 80,4 |
| Waymo | 60,5 | 70,8 | 72,1 | 72,9 | 71,3 |

- **Le gain extérieur sur PPT supervisé est de +0,3 mIoU**, à comparer aux +50 points de linear probing obtenus en intérieur. La marge réelle d'une meilleure représentation auto-supervisée en extérieur est l'écart linéaire-supervisé, soit **10,3 points sur SemanticKITTI** (62,0 contre 72,3) : c'est le seul budget que ce dossier puisse viser.
- **Non reproductible en l'état.** Le modèle extérieur est un préentraînement séparé — « we adapt pre-training paradigm of Sonata to outdoor LiDAR scenarios through joint training on nuScenes, Waymo, and SemanticKITTI » — donc les 139 769 scènes intérieures (72 % synthétiques ASE, 200 époques, batch 96, 32 GPU, ni type de GPU ni temps rapportés) ne servent pas aux chiffres KITTI, dont le coût n'est pas documenté. Les poids extérieurs n'ont jamais été publiés ; l'énumération complète de Pointcept (726 fichiers) ne contient aucune config Sonata kitti/nuscenes/waymo ; issues #456 et #469 ouvertes. Code Apache 2.0 mais **poids en CC-BY-NC 4.0** (restriction héritée de HM3D et ArkitScenes) : à traiter comme `HGP-old/`, jamais importé dans la ligne produit.
- **Sonata nomme lui-même la densité** comme obstacle principal à l'unification intérieur-extérieur : « The main challenges lie in point density and input features: point density can be aligned by scaling, while enhancing outdoor LiDAR data with color from lifted images and pseudo normal vectors based on LiDAR viewing direction helps bridge feature gaps », avec la limitation déclarée « Currently, Sonata separates pre-training for each setting ». C'est exactement l'objet de ce dossier : une structure de densité explicite plutôt qu'un rééchelonnement.

Les successeurs du même groupe, eux, publient des configs KITTI : `configs/concerto/semseg-ptv3-large-v1m1-kitti-4a-lin.py` et sa variante `-withcolornormal-`, `configs/utonia/...-6a/6b/6c-kitti-lin/dec/ft.py`. Les poids Concerto sont également CC-BY-NC.

## La lignée complète, et qui il faut réellement battre

Sonata n'est ni le plus fort ni le plus utilisable de sa famille. Sur SemanticKITTI val :

| Modèle | linéaire | décodeur | fine-tuning | coût de pré-entraînement | poids |
|---|---|---|---|---|---|
| PTv3 supervisé | $70{,}8$ annoncé | — | — | — | pas de config KITTI |
| Sonata, CVPR 2025 | $62{,}0$ | $68{,}4$ | $72{,}6$ | 32 GPU | **outdoor non publiés** |
| Concerto, NeurIPS 2025 | $66{,}6$ | $69{,}3$ | $71{,}2$ | 85 h $\times$ 16 H20 | publiés, config KITTI |
| Utonia, ICML 2026 | $\mathbf{67{,}7}$ | $\mathbf{70{,}0}$ | $72{,}0$ | 64 H20 | publiés, config KITTI |
| **DOS**, hors lignée | $67{,}5$ | — | $\mathbf{73{,}5}$ | **2 A100 $\times$ 20 h** | publiés |

Trois lectures, et la troisième décide de la stratégie.

**Le fine-tuning sature** autour de $72$–$73$ ; c'est le probing qui sépare. C'est l'argument même de Sonata sur ce qui mesure une représentation, donc c'est le probing qu'il faut rapporter.

**DOS bat toute la lignée sur SemanticKITTI pour deux ordres de grandeur de calcul en moins** — $2$ A100 pendant $20$ h contre $64$ H20. Sur ce benchmark, une meilleure idée bat davantage de calcul, ce qui rend le programme finançable sans cluster.

**Aucun de ces modèles ne dérive de structure depuis la géométrie du nuage.** Sonata en fait une doctrine explicite : aucun algorithme conçu par l'humain, aucune segmentation pré-calculée. Concerto a bien de la structure, mais **importée de l'image** — patchs DINOv2, appariement pixel–point par calibration — et elle s'effondre sans couleur : $36{,}8$ contre $77{,}0$ mIoU, c'est-à-dire précisément le régime du LiDAR nu. Utonia a de la structure, mais elle vient de la **trajectoire du capteur**, par agrégation multi-trames alignée en pose, et non de la scène.

### PointINS : à la fois la validation et le concurrent

PointINS (Bosch Research et Lübeck, mars 2026) construit des pseudo-instances sans annotation — $k$-means grossier, graphe $k$-NN, composantes connexes par parcours en largeur — et gagne $+3{,}2$ PQ sur SemanticKITTI et $+4{,}8$ sur nuScenes contre DOS. En probing panoptique : $52{,}8$ PQ contre $49{,}6$.

C'est la démonstration que **des unités structurées paient**, obtenue avec un pipeline **plat, ad hoc, sans garantie de correction ni de stabilité**. C'est exactement le substitut grossier qu'un arbre de fusion exact remplacerait — et c'est donc la baseline à battre, avec DOS.

**Conséquence de protocole :** se comparer à Concerto sur du LiDAR nu serait attaquer un modèle hors de son régime, et un relecteur le verrait. Les concurrents sont **DOS** et **PointINS**.

## Auto-supervision LiDAR : l'état de l'art et ses unités

| Méthode | Unité utilisée | Comment elle est obtenue | Caméra |
|---|---|---|:-:|
| SegContrast, RA-L 2022 | segments **plats**, un seul niveau | RANSAC (sol) + DBSCAN ; contraste au niveau segment ; fondateur de la ligne | non |
| TARL, CVPR 2023 | segments d'objet sur 12 scans accumulés | Patchwork (sol) + HDBSCAN ; motif explicite : meilleurs segments en région peu dense | non |
| UNIT, [arXiv 2409.07887](https://arxiv.org/abs/2409.07887) | idem, 40 scans | Patchwork++ + HDBSCAN | non |
| STSSL, CVPR 2023 | contraste spatio-temporel | unité non vérifiée par cette veille | non |
| BEVContrast | cellule BEV régulière | grille fixe | non |
| ALSO | aucune | reconstruction de surface, sans segments | non |
| Seal, NeurIPS 2023 Spotlight, [arXiv 2306.09347](https://arxiv.org/abs/2306.09347) | superpoints | superpixels SAM / X-Decoder / OpenSeeD / SEEM sur les images caméra, projetés par calibration, InfoNCE spatial + cohérence temporelle bidirectionnelle ; mais son point-to-segment repose sur **RANSAC + HDBSCAN**, donc sur du clustering géométrique et non sur les segments VFM | oui |

Seal : nuScenes linear probing 44,95 contre 38,8 pour SLidR ; SemanticKITTI à 1 % d'étiquettes 46,63 contre 44,60.

SemanticKITTI, mIoU par fraction d'étiquettes, tels que rapportés par BEVContrast (protocole BEVContrast, non celui de la section « barre en validation ») :

| Préentraînement | 0,1 % | 1 % | 10 % | 50 % | 100 % |
|---|---:|---:|---:|---:|---:|
| depuis zéro | 30,0 | 46,2 | 57,6 | 61,8 | 62,7 |
| PointContrast | 32,4 | 47,9 | 59,7 | 62,7 | 63,4 |
| SegContrast | 32,3 | 48,9 | 58,7 | 62,1 | 62,3 |
| DepthContrast | 32,5 | 49,0 | 60,3 | 62,9 | 63,9 |
| STSSL | 32,0 | 49,4 | 60,0 | 62,9 | 63,3 |
| ALSO | 35,0 | 50,0 | 60,5 | 63,4 | 63,6 |
| TARL | 37,9 | 52,5 | 61,2 | 63,4 | 63,7 |
| BEVContrast | **39,7** | **53,8** | **61,4** | 63,4 | **64,1** |

**Toutes condensent.** Chacune réduit la hiérarchie de densité à une partition plate — excess-of-mass ou équivalent — et jette l'arbre : personne, dans la littérature LiDAR consultée, n'utilise le cluster tree lui-même. Conserver les nœuds internes, la relation parent-enfant et les niveaux comme signal n'est revendiqué par aucun travail LiDAR ; c'est le seul point de nouveauté qui tienne dans cette lignée.

## Concurrents conceptuels : hiérarchie et attention

### HSA, NeurIPS 2025

[Hierarchical Self-Attention](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) formalise un signal imbriqué par un arbre. Les vecteurs de contenu sont aux feuilles ; les relations positionnelles sont attachées aux familles. Les coefficients entre deux sous-arbres frères sont partagés par blocs.

Pour **un arbre donné**, avec Q/K LayerNormés après projection, l'énergie quadratique, la température et le rescaling du papier, HSA minimise $\sum_i D_{\mathrm{KL}}\left(\theta^{\mathrm{HSA}}_i\,\Vert\,\theta^{\mathrm{flat}}_i\right)$ sur la famille de matrices stochastiques satisfaisant les contraintes de blocs. La cible plate utilise la même énergie et les mêmes positions ; il ne s'agit pas d'une attention Softmax arbitraire. Ce résultat porte sur les poids d'attention et ne garantit ni la qualité de la hiérarchie, ni les effets des projections V/gates/MLP, ni un gain de segmentation.

Le calcul est bottom-up puis top-down, en $\mathcal{O}(M b^{2})$ pour $M$ familles et un branchement maximal $b$. Le papier reconnaît que les parcours d'arbre sont mal adaptés au GPU et propose des opérations sparse par profondeur ainsi qu'une concaténation en largeur. Il n'évalue aucun nuage 3D ni aucune segmentation dense ; ses expériences portent surtout sur la classification et le remplacement de couches de RoBERTa. Certains remplacements complets dégradent fortement les résultats, ce qui motive une architecture hybride et des blocs tardifs.

Trois conséquences mécaniques doivent être posées avant toute intégration, parce qu'elles déterminent ce qu'un gain observé pourrait signifier.

La première est une réduction. Sous LayerNorm appliqué après projection, l'énergie d'interaction entre deux sous-arbres ne dépend que des moyennes des requêtes et des clés de chaque sous-arbre, pondérées par leur taille. Une HSA fidèle est donc mécaniquement une attention sur des moyennes de sous-arbres, c'est-à-dire un objet très proche d'un contrôle bottom-up/top-down par moyenne suivie d'un MLP. Ce contrôle doit figurer dans le protocole : sans lui, tout gain sera attribué à HSA alors qu'il peut venir de la seule hiérarchie.

La deuxième concerne l'endroit exact où la géométrie entre. Les auteurs indiquent que leur cadre n'ajoute aucun paramètre apprenable le long de la hiérarchie. Le descripteur géométrique n'intervient donc que par le produit scalaire $\epsilon(A')^{\top}\epsilon(B')$ entre frères, soit un unique scalaire de biais par couple de frères. Quelle que soit la richesse du descripteur de nœud, il est comprimé à cet endroit sur une seule dimension ; c'est un goulot structurel, à confronter aux ablations rapportées plus bas et à l'analyse de [DESCRIPTEURS_DE_NOEUD.md](archive/DESCRIPTEURS_DE_NOEUD.md).

La troisième est un coût. L'algorithme demande $D$ produits matrice creuse–vecteur strictement séquentiels, où $D$ est la profondeur de l'arbre. La condensation de l'arbre de fusion n'est donc pas une optimisation facultative mais une condition d'existence sur GPU : un arbre de fusion binaire brut sur un scan produit une profondeur qui interdit l'entraînement. Le papier mesure lui-même le prix d'un remplacement mal placé : en zero-shot, QNLI tombe à 0,5072, soit le niveau du hasard. Aucune expérience 3D ni dense n'accompagne ces résultats ; les validations portent uniquement sur du texte.

### Sequoia, TMLR 2024

[Scalable Hierarchical Self-Attention with Learnable Hierarchy](https://openreview.net/forum?id=qH4YFMyhce) apprend une hiérarchie pour les graphes et contraint les échanges à la famille immédiate. Il rapporte aussi des tâches de classification/segmentation de point clouds avec une hiérarchie fixe. Il réduit donc la nouveauté d'un simple slogan « hiérarchie + attention ». HGP doit être défendu par son fondement de densité, sa stabilité et son effet causal.

### HKT, prépublication 2026

[Hierarchical Kernel Transformer](https://arxiv.org/abs/2604.08829) étudie une attention multi-résolution pour séquences avec noyau hiérarchique, décomposition de l'erreur d'approximation et résultats informationnels. Il s'agit d'une prépublication soumise à Neurocomputing, sans point clouds ni arbre de densité, mais elle occupe déjà le terrain « attention hiérarchique + théorème d'approximation ». Une contribution théorique HGP-HSA doit donc porter sur une famille de contraintes, une stabilité ou une garantie sémantique réellement différente.

### LitePT, CVPR 2026

[LitePT](https://openaccess.thecvf.com/content/CVPR2026/html/Yue_LitePT_Lighter_Yet_Stronger_Point_Transformer_CVPR_2026_paper.html) rend explicite le motif convolutions efficaces dans les premiers étages puis attention dans les étages tardifs, avec PointROPE. Il ne rapporte pas SemanticKITTI, mais menace directement la nouveauté architecturale « backbone local + quelques attentions tardives ». HGP-HSA doit montrer que la structure de densité et son certificat ajoutent autre chose à ce motif déjà publié.

### Précédents directs de QC-HSA

[Fast Multipole Attention](https://arxiv.org/abs/2310.11960) conserve les requêtes fines tout en représentant des interactions lointaines à des résolutions progressivement plus grossières ; sa variante annonce $\mathcal{O}(N\log N)$ lorsque les requêtes ne sont pas sous-échantillonnées. C'est le précédent algorithmique le plus proche de `QC-HSA`. Il emploie cependant une hiérarchie régulière/apprise et des bases entraînées, sans caractérisation comme projection reverse-KL ni arbre de densité HGP. La partition canonique de `QC-HSA` est générique à tout arbre laminaire ; HGP n'en est qu'une instanciation particulière.

[H-Transformer-1D](https://aclanthology.org/2021.acl-long.294/) exploite des H-matrices pour une attention linéaire sur séquences, tandis que [MRA](https://proceedings.mlr.press/v162/zeng22a.html) raffine une approximation multi-résolution sous contraintes pratiques. Ces travaux interdisent de revendiquer comme nouveauté la seule structure point–sous-arbre ou la complexité sous-quadratique. L'espace potentiel de `QC-HSA` est plus étroit : partition canonique induite par HGP, projection KL exacte, inclusion/domination de HSA et certificat par oscillation intra-branche.

### Superpoint Transformer, ICCV 2023

[SPT](https://openaccess.thecvf.com/content/ICCV2023/html/Robert_Efficient_3D_Semantic_Segmentation_with_Superpoint_Transformer_ICCV_2023_paper.html) utilise déjà une partition géométrique hiérarchique et une attention multi-échelle. Il n'est pas publié sur SemanticKITTI, mais son ablation sur KITTI-360 attribue plusieurs points de mIoU à la hiérarchie et à l'adjacence. Une adaptation SemanticKITTI est une baseline conceptuelle obligatoire.

Le détail de la recette compte, car il fixe précisément ce qu'une variante HGP devrait déplacer. La partition est obtenue par $l_{0}$-cut-pursuit hiérarchique parallélisé sur deux niveaux, avec une réduction du nombre d'éléments d'environ trente fois puis cinq fois. Chaque point porte huit features — trois radiométriques, puis linéarité, planarité, dispersion, verticalité et élévation estimées sur ses 50 plus proches voisins — et chaque arête en porte dix-huit : sept d'interface, quatre de ratio, sept de pose. L'attention est une graph-attention intra-niveau entre superpoints adjacents, complétée par un décodeur en U qui réinjecte le parent ; il n'y a donc pas d'attention entre niveaux au sens de HSA. Les résultats sont 76,0 sur S3DIS 6-fold, 68,9 sur Area 5, 63,5 sur la validation KITTI-360 et 79,6 sur DALES, pour 0,21 M de paramètres, 3,0 GPU-heures d'entraînement et environ 2 s d'inférence.

Un fait de veille doit être isolé, car il oriente la stratégie plus qu'aucun score : aucune méthode de la lignée superpoint ne publie SemanticKITTI mono-trame. Ni SPG, ni SSP, ni SPNet, ni SPT, ni SuperCluster, ni EZ-SP. Toutes rapportent S3DIS, ScanNet, DALES ou KITTI-360 accumulé. La lecture est à double tranchant. D'un côté le créneau est libre, et un premier résultat superpoint crédible en mono-trame serait en soi une contribution de positionnement. De l'autre, cette communauté a de fait concédé le mono-trame aux méthodes voxel et point denses, probablement parce qu'une trame unique offre trop peu de points par région pour qu'une agrégation par superpoint conserve du signal. Une absence prolongée dans une communauté active est plus souvent un obstacle identifié qu'un oubli : la charge de la preuve revient à celui qui ouvre le créneau.

### EZ-SP, ICRA 2026

[EZ-SP](https://arxiv.org/abs/2512.00385) remplace la partition CPU de SPT par un clustering appris sur GPU, annoncé à moins de 60k paramètres et beaucoup plus rapide que les partitions antérieures. Même sans score SemanticKITTI publié, il est le concurrent système direct de MorseHGP3D : HGP doit démontrer soit une meilleure structure, soit un coût Pareto comparable.

### SPCNet, ICCV 2025

[SPCNet](https://openaccess.thecvf.com/content/ICCV2025/html/Lu_Serialization_based_Point_Cloud_Oversegmentation_ICCV_2025_paper.html) apprend des superpoints par sérialisation de Hilbert, réaffectation par similarité et cross-attention, puis ajoute deux niveaux hiérarchiques. Il évalue explicitement SemanticKITTI et rapporte 71,9 mIoU test pour sa variante superpoint. Son score n'est pas la cible SOTA, mais son oversegmentation apprise et son gain hiérarchique en font un contrôle conceptuel direct pour la qualité des partitions.

### Autres précédents

- [OctFormer](https://arxiv.org/abs/2305.03045) exploite une structure octree pour l'attention efficace.
- [Cluster3Dseg](https://openaccess.thecvf.com/content/ICCV2023/html/Feng_Clustering_based_Point_Cloud_Representation_Learning_for_3D_Analysis_ICCV_2023_paper.html) utilise le clustering dans l'espace d'embedding et rapporte 70,4 sur SemanticKITTI ; ce n'est pas une hiérarchie géométrique par scan, mais le terrain lexical « clustering pour la représentation » est occupé.
- [SuperCluster](https://arxiv.org/abs/2401.06704) formule la segmentation panoptique comme clustering d'un graphe de superpoints.
- [SSTNet](https://openaccess.thecvf.com/content/ICCV2021/html/Liang_Instance_Segmentation_in_3D_Scenes_Using_Semantic_Superpoint_Tree_Networks_ICCV_2021_paper.html) apprend et coupe un arbre de superpoints pour les instances intérieures.

## L'oracle de partition, et pourquoi il réfute plus qu'il ne promeut

La lignée superpoint publie systématiquement un oracle de partition : on attribue à chaque région la meilleure étiquette possible et on mesure le plafond ainsi atteint. Ce diagnostic est régulièrement invoqué comme argument en faveur d'une meilleure partition ; les chiffres publiés disent l'inverse.

- SPG (CVPR 2018), tableau 5, S3DIS en 6-fold : l'oracle « Perfect » atteint 88,2 mIoU et 92,7 mAcc, alors que SPG lui-même rapporte 62,1.
- SPT (ICCV 2023) conclut explicitement « The performance of SPT is more than 20 points below the oracle, suggesting that the partition does not strongly limit its performance ». Avec 68,9 sur S3DIS Area 5, cela place son oracle au-delà de 89.
- SuperCluster (3DV 2024) écrit de même « The high performance of this oracle (93,4 PQ) indicates that very little precision is lost by working with superpoints », son second oracle de clustering restant à 83,6 PQ.

Un fait de mesure doit accompagner ces chiffres : **aucun oracle de partition n'a jamais été publié sur SemanticKITTI mono-scan**. La métrique existe et circule (SPG, courbes oracle de SPT ICCV 2023, EZ-SP ICRA 2026), mais toujours sur S3DIS, Semantic3D, ScanNet, KITTI-360 ou DALES. Le mesurer ici produirait donc un chiffre qui n'existe nulle part : sans point de comparaison sur ce benchmark, il ne peut servir qu'à réfuter la voie, jamais à la promouvoir.

Une vingtaine de points d'oracle sont donc déjà disponibles et non convertis. Relever le plafond d'une partition qui n'est pas saturée ne peut pas payer, puisque le facteur limitant se trouve en aval, dans le modèle qui exploite les régions. Il en résulte une règle d'usage pour ce projet : un diagnostic d'oracle est une porte de réfutation et non de promotion. Un oracle HGP nettement inférieur à ceux ci-dessus condamne la voie ; un oracle supérieur ne prouve rien, puisque le concurrent laisse déjà vingt points sur la table. [VOIES.md](VOIES.md) place cette mesure en première position et explique pourquoi le mIoU n'est pas le critère à optimiser sur l'arbre.

Les ablations publiées sur cette famille exacte pointent dans la même direction et hiérarchisent les leviers. Superpoint Transformer mesure, en mIoU perdu :

| Ablation | S3DIS 6-fold | KITTI-360 | DALES |
|---|---:|---:|---:|
| retirer toutes les features de nœud handcrafted | -0,7 | -4,1 | -1,4 |
| retirer l'encodage d'adjacence | -6,3 | -5,4 | -3,0 |
| passer à un seul niveau de partition | -8,4 | -5,1 | -0,9 |

EZ-SP (ICRA 2026) complète le tableau : remplacer les features handcrafted par un petit réseau appris change le résultat de plus ou moins 0,1 mIoU. Le descripteur de nœud est donc le plus faible des trois leviers, loin derrière l'adjacence et le nombre de niveaux. Un travail dont la contribution principale serait un meilleur descripteur de nœud viserait précisément l'axe où la littérature mesure le moins d'effet ; c'est un argument à intégrer avant le choix du budget de nouveauté, et non après. [DESCRIPTEURS_DE_NOEUD.md](archive/DESCRIPTEURS_DE_NOEUD.md) traite séparément la question de ce que ce descripteur peut au mieux contenir.

## Ce que HGP apporte réellement

Le papier [Generalization of single-linkage with higher-order interactions](https://link.springer.com/article/10.1007/s41109-025-00756-1) montre, sous ses hypothèses de position générale, que les $K$-polyèdres correspondent aux clusters de haute densité de son estimateur $K$-NN **sur l'échantillon fini**. C'est un fondement plus précis qu'une oversegmentation heuristique, mais ni une preuve de consistance vers l'arbre de Hartigan populationnel ni une garantie d'alignement sémantique.

Son expérience SemanticKITTI ne mesure toutefois pas la segmentation sémantique : elle utilise les **masques sémantiques de vérité terrain** de la séquence 08, puis regroupe les points de chaque classe thing. Les PQthing/RQthing/SQthing rapportés sont environ :

| Ordre | PQthing | RQthing | SQthing |
|---:|---:|---:|---:|
| $K=1$ | 0,876 | 0,921 | 0,949 |
| $K=2$ | **0,888** | **0,934** | 0,950 |
| $K=3$ | 0,829 | 0,917 | 0,903 |

Cela soutient l'exploration de $K=2$ et réfute l'idée « un ordre plus grand est toujours meilleur ». Cela ne démontre pas qu'HGP aide à prédire les classes. Le papier indique aussi que des clusters d'ordre supérieur peuvent partager des points et qu'une attribution dure perd cette structure.

Ce recouvrement entre en tension directe avec HSA, et la tension doit être nommée ici plutôt que découverte à l'implémentation. Le lemme de sous-structure optimale de HSA est énoncé sur une partition, donc sur des ensembles de feuilles disjoints : l'arbre doit être strictement laminaire. Or pour $K \geq 2$ les $K$-polyèdres se recouvrent, le manuscrit notant qu'« on voit déjà apparaître le phénomène essentiel : pour $K \geq 2$, les polyèdres peuvent se recouvrir ». Laminariser l'arbre pour le rendre acceptable par HSA supprime donc exactement ce qui distingue HGP de HDBSCAN, et à $K = 1$ HGP est le single-linkage. Le dilemme ne se résout pas par un réglage : soit l'on rabote la structure et la nouveauté doit alors porter sur autre chose que l'arbre, soit l'on conserve le recouvrement et il faut un opérateur d'attention sur le DAG de recouvrement plutôt que sur un arbre. Si un budget de nouveauté doit aller à un opérateur, c'est vers ce second terme qu'il doit aller ; [VOIES.md](VOIES.md) en fait la cible T6 du [programme théorique](archive/THEOREMES.md) et non un préalable.

L'apport potentiel au projet n'est pas seulement une hiérarchie donnée à HSA. La proposition conserve aussi un **complexe HGP marqué** : facettes d'une composante du graphe complet $\Gamma_K^{\mathrm{full}}$, cofaces élémentaires qui certifient une sous-adjacence $\Gamma_K^{\mathrm{elem}}$ de mêmes composantes $H_0$, incidences, coordonnées et niveaux de filtration. Ce payload est plus riche que le $K$-polyèdre défini dans la source comme ensemble de points. Son schéma fixe `payload_kind=marked_incidence`, un `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et une `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only`. Ces réalisations et autorités ne doivent pas être confondues. Le payload n'est pas actuellement une sortie certifiée de MorseHGP3D v3. Sa construction sparse, sans matérialiser le complexe de Čech ambiant, fait donc partie de la question scientifique et système.

## Antériorités pour encoder le canal non convexe

[MPSN](https://proceedings.mlr.press/v139/bodnar21a.html) apprend déjà sur les incidences d'un complexe simplicial et relie son expressivité à Simplicial Weisfeiler–Lehman ; [CW Networks](https://proceedings.neurips.cc/paper_files/paper/2021/hash/157792e4abb490f99dbd738483e0d2d4-Abstract.html) étend déjà cette idée aux complexes cellulaires réguliers. Les [réseaux simpliciaux principiels](https://proceedings.mlr.press/v139/roddenberry21a.html) formalisent équivariance aux permutations, équivariance aux orientations et dépendance à toutes les dimensions du complexe. [EMPSN](https://proceedings.mlr.press/v202/eijkelboom23a.html) ajoute une géométrie $\mathrm{E}(n)$-équivariante et vise explicitement graphes géométriques et nuages de points. [Simplicial Attention Networks](https://openreview.net/forum?id=ScfRNWkpec) pondère déjà les interactions entre simplexes voisins, tandis que [TopNets](https://proceedings.mlr.press/v235/verma24a.html) combine message passing topologique, persistance, continuité et équivariance. [Topological Point Cloud Clustering](https://proceedings.mlr.press/v202/grande23a.html) exploite déjà plusieurs Laplaciens de Hodge d'un complexe pour caractériser et regrouper des points.

Ces travaux invalident le claim générique « première attention sur un polyèdre non convexe ». La nouveauté défendable doit être spécifique au contrat HGP : extraction sparse des facettes/cofaces et niveaux, invariance à des certificats sparse équivalents, traitement des recouvrements, composition le long de la hiérarchie, stabilité au capteur ou certificat fidélité–coût. MPSN, CWN, EMPSN, SAT et TopNets deviennent des baselines de la branche non convexe, pas seulement des citations.

## Complexe HGP et fonction support : rôles distincts

Le complexe marqué conserve les incidences et peut représenter un objet non convexe. La critique ci-dessous ne le réfute pas. Elle réfute seulement le remplacement de ce canal variable par un unique maximum directionnel. Un payload source ou PL qui conserve ses sommets détermine déjà le support de ces sommets, et le carrier de facettes PL partage ce support ; l'ajouter explicitement n'augmente alors pas l'information théorique, mais peut fournir un raccourci global utile à l'optimisation. Cette redondance ne vaut jamais comme identité entre le support des observations et celui de `witness_union`. L'ablation obligatoire est donc `complexe seul` contre `support source + complexe`, avec le carrier et l'autorité inchangés.

La fonction support est un objet classique de géométrie convexe. Pour tout ensemble borné, elle est identique à celle de la fermeture de son enveloppe convexe. Même avec une infinité de directions, elle ne distingue donc pas :

- un volume plein et seulement ses points extrêmes ;
- une coquille et un intérieur dense ;
- une forme concave et son enveloppe ;
- plusieurs composantes intérieures ayant la même enveloppe ;
- différentes densités ou rémissions.

Avec un nombre fini de directions, des enveloppes distinctes peuvent également produire le même vecteur. Le taux d'erreur dépend de la couverture de la sphère et du conditionnement de la forme. Le max est sensible à un outlier et à la disparition d'un point exposé.

Le maximum de la norme sur un rayon est une **fonction radiale extérieure**, pas un support. Elle identifie une forme depuis son centre seulement lorsque celle-ci est étoilée ; sinon elle reconstruit son remplissage radial. Un cube plein et sa frontière ont par exemple mêmes support et rayon depuis le centre. Les intersections multi-segments conservent davantage d'information, mais une grille finie reste non injective et les rayons génériques manquent souvent les complexes de faible dimension.

Les transformées ECT/PHT complètes ont déjà des résultats d'injectivité pour des classes de complexes et formes constructibles. WECT et l'ECT différentiable sont également publiées. Une représentation topologique directionnelle peut donc être une baseline ou un composant HGP-spécifique, mais ni « ECT sur les simplexes » ni sa discrétisation finie ne constitue seule une nouveauté.

Le rayon extérieur reste une compression facultative et lossy, pas le second canal proposé. La contribution ne peut donc être ni « invention de la fonction support » ni « support + rayon ». Elle peut être :

- un encodeur qui, après preuve d'expressivité, distingue à budget borné des complexes aux mêmes sommets et au même support mais aux incidences différentes ; avant cette preuve, seul l'oracle ou le hash canonique doit les séparer et les collisions apprises sont mesurées ;
- une représentation indépendante des identifiants et du certificat sparse particulier choisi pour la même composante HGP ;
- une composition du complexe marqué le long des fusions, avec coût proportionnel aux incidences actives et erreur de condensation certifiée ;
- une analyse de stabilité sous échantillonnage LiDAR et changement de filtration ;
- un couplage entre contexte simplicial local et attention hiérarchique, contrôlé contre MPSN/EMPSN/SAT et message passing simple.

## Concurrence instance future

Cette section ne pilote pas la phase actuelle. [ALPINE](https://arxiv.org/abs/2503.13203), publié à 3DV 2026, prend les prédictions sémantiques, construit un graphe $k_{\mathrm{local}}$-NN BEV par classe thing, coupe ses arêtes par seuil et extrait les composantes, avec split de gros objets. Sans entraînement d'instance, UniSeg+ALPINE rapporte 70,2 PQ dans son snapshot test de juin 2025.

La leçon utile aujourd'hui est simple : améliorer la sémantique est le premier levier, et « utiliser un clustering pour les instances » n'est plus une contribution suffisante. Si la phase instance ouvre, ALPINE doit recevoir les mêmes logits gelés que HGP.

## Positionnement défendable

Le papier potentiel ne doit pas raconter « tous les clusters ont un vecteur de même taille ». Cette propriété est commode, mais ni nouvelle ni suffisante.

### Ce qui ne distingue pas

- **« Sans caméra » ne distingue pas** : TARL, SegContrast, BEVContrast, ALSO, STSSL et ALPINE le sont tous ; seuls Seal et HilDA utilisent la caméra.
- **« Nous utilisons la densité » ne distingue pas non plus** : HDBSCAN est le producteur de segments standard du domaine LiDAR depuis TARL.
- Antériorités à citer obligatoirement : [cTree](https://arxiv.org/abs/2009.14168) (Sharma & Kaul, NeurIPS 2020) partitionne par cover tree — hiérarchie métrique et non de densité — avec deux tâches prétexte qui prédisent la décomposition hiérarchique, mais à l'échelle de l'objet et non de la scène LiDAR ; [HASSL](https://arxiv.org/abs/2607.04353) (7 juillet 2026) fait **déjà** hiérarchie HDBSCAN multi-niveaux comme structure SSL avec prototypes par niveau, en microscopie cellule unique ; Part2Object (ECCV 2024) pour l'instance non supervisée ; Superpoint Transformer a une partition hiérarchique multi-niveaux, mais supervisée et issue de cut-pursuit.
- **L'ablation qui décide de tout**, à architecture et budget identiques : (i) HGP exact, (ii) HDBSCAN à hiérarchie conservée, (iii) HDBSCAN condensé plat (protocole TARL), (iv) arbre aléatoire. Le bras (ii) sépare l'apport de l'exactitude de celui de la hiérarchie ; si (i) et (ii) sont équivalents, la contribution se réduit à « utiliser une hiérarchie », déjà prise par cTree et HASSL.
- L'argument « exact » est la vraie singularité du dépôt, mais il est **orthogonal** à cette littérature : personne n'y exige l'exactitude de la hiérarchie, HDBSCAN heuristique suffit. Il faut donc démontrer que l'exactitude déplace une métrique aval, faute de quoi un reviewer répondra qu'HDBSCAN approché suffit et coûte moins cher.

Le positionnement le plus solide est :

> Un complexe HGP marqué et sa hiérarchie fournissent-ils un prior d'interactions d'ordre supérieur stable, efficace et vérifiable pour la propagation de contexte sémantique dans les scènes LiDAR irrégulièrement échantillonnées ?

Pour mériter ICML/NeurIPS, la réponse doit inclure une contribution générale — stabilité, analyse, opérateur ou descripteur fusionnable — et une validation au-delà de SemanticKITTI.
