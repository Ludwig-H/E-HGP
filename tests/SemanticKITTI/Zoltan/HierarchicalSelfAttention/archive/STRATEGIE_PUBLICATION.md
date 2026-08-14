# Stratégie de publication

## La revendication, en une phrase

> **Ne pas condenser.** La littérature LiDAR auto-supervisée à segments fabrique ses unités d'entraînement avec HDBSCAN, condense l'arbre de densité en une partition plate, puis jette l'arbre. Nous le gardons : les nœuds internes, la relation parent–enfant et les niveaux sont le signal.

SegContrast (RA-L 2022) contraste des segments plats issus de RANSAC + DBSCAN, à un seul niveau ; TARL (CVPR 2023) applique Patchwork puis HDBSCAN sur 12 scans accumulés, motif déclaré étant d'obtenir de meilleurs segments en région peu dense ; UNIT reprend Patchwork++ et HDBSCAN sur 40 scans ; Seal (NeurIPS 2023) tire ses superpixels de VFM sur les images caméra mais fait son point-to-segment par RANSAC + HDBSCAN. L'unité d'entraînement y est toujours plate ; ailleurs elle l'est encore plus, Sonata (CVPR 2025) prenant pour unités des sous-ensembles aléatoires — 2 vues globales à 40–100 % des points, 4 locales à 5–40 %, 2 masquées. Verbatim de l'audit de concurrence : « Personne, dans la littérature LiDAR consultée, n'utilise le cluster tree lui-même. »

Ce que la revendication **n'est pas** :

| Formulation tentante | Pourquoi elle ne distingue pas |
|---|---|
| « nous utilisons une hiérarchie » | cTree (NeurIPS 2020) prédit déjà une décomposition hiérarchique ; HASSL (juillet 2026) fait déjà hiérarchie multi-niveaux + prototypes par niveau comme structure d'auto-supervision |
| « nous utilisons la densité » | HDBSCAN est le producteur de segments standard du domaine LiDAR depuis TARL |
| « sans caméra » | TARL, SegContrast, BEVContrast, ALSO, STSSL et ALPINE sont tous sans caméra ; seuls Seal et HilDA en utilisent une |
| « segments issus de la géométrie » | Seal lui-même, malgré ses superpixels VFM, obtient ses segments par RANSAC + HDBSCAN |

Le point de nouveauté est donc unique et étroit : **conserver l'arbre au lieu de le condenser**, en LiDAR extérieur. La revendication survit si HSA n'est pas le meilleur opérateur ; elle ne survit pas si l'arbre non condensé ne bat pas sa propre condensation plate.

Les tableaux du papier conservent les noms contractuels exacts : `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only`. Ils enregistrent aussi `cut_policy`, `cut_level`, `cut_side` et `deltas` ; changer l'un de ces champs crée une configuration distincte.

## Pourquoi HGP plutôt que l'arbre condensé de HDBSCAN

C'est la première question d'un relecteur : HDBSCAN est gratuit, disponible et déjà standard. La réponse est une chaîne dont chaque maillon dépend du précédent.

| # | Maillon | Conséquence |
|---|---|---|
| 1 | on ne condense pas | l'arbre entier est exposé à l'entraînement |
| 2 | les niveaux servent de signal | prototypes, tâches prétextes ou pertes indexés par niveau |
| 3 | un niveau doit alors avoir un sens | sinon le niveau $\ell$ d'une scène et le niveau $\ell$ d'une autre ne désignent pas la même chose, et le signal est du bruit indexé |
| 4 | l'exactitude de HGP donne ce sens | coïncidence exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN, cf. [THEOREMES.md](THEOREMES.md) |
| 5 | la percolation dit quels niveaux sont utilisables | fonction et vitesse de percolation, limite gaussienne $\mu = K + a\sqrt{K}$, cf. [VOIES.md](../VOIES.md) |

L'exactitude ne devient load-bearing **que par le maillon 2**. Tant qu'on condense, un arbre heuristique suffit : c'est précisément pourquoi personne n'a jamais eu besoin d'exactitude dans cette littérature, et pourquoi l'argument « exact » y est resté orthogonal.

D'où l'obligation, consignée honnêtement : il faut **démontrer que l'exactitude change une métrique aval**. Sinon la réponse du relecteur est écrite d'avance — HDBSCAN approché suffit et coûte moins cher. C'est le rôle du bras (ii) de l'ablation, et rien ne peut le remplacer.

Deux appuis externes, l'un favorable, l'autre défavorable :

- Sonata nomme la densité comme obstacle principal à l'unification intérieur–extérieur, verbatim : « point density can be aligned by scaling, while enhancing outdoor LiDAR data with color from lifted images and pseudo normal vectors based on LiDAR viewing direction helps bridge feature gaps ». Une structure indexée par la densité répond nommément à une limitation déclarée par l'état de l'art.
- Le « geometric shortcut » diagnostiqué par Sonata porte sur les **coordonnées**, pas sur les features d'entrée (Sonata fournit lui-même les normales, `feat_keys=("coord","color","normal")`) : « This shortcut refers to the tendency of the model to collapse to easily accessible, low-level geometric cues, such as normal direction or point height. » Un arbre construit sur les coordonnées est exposé au même grief et doit être testé contre lui, ce qui est exactement R1 : la hiérarchie encode-t-elle le capteur plutôt que la sémantique ?

## Où est la marge

Sonata (CVPR 2025 Highlight) fournit la mesure la plus nette, tout en validation (Table 8) :

| Jeu (val, mIoU) | Sonata linéaire | Sonata décodeur | PPT supervisé | Sonata fine-tuning | PTv3 supervisé |
|---|---|---|---|---|---|
| SemanticKITTI | 62,0 | 68,4 | 72,3 | 72,6 | 69,1 |
| nuScenes | 66,1 | 77,3 | 81,2 | 81,7 | 80,4 |
| Waymo | 60,5 | 70,8 | 72,1 | 72,9 | 71,3 |

Deux lectures opposées de la même table :

| Régime | Marge disponible | Plancher de bruit | Verdict |
|---|---|---|---|
| supervision complète | $+0{,}3$ mIoU (Sonata fine-tuning contre PPT supervisé) | environ $1{,}5$ mIoU | un gain supervisé n'est pas mesurable proprement |
| peu d'étiquettes | $10{,}3$ mIoU (linéaire $62{,}0$ contre supervisé $72{,}3$) | le même $1{,}5$ | rapport signal sur bruit dix fois meilleur |

En intérieur, la même méthode gagne plus de cinquante points de linear probing sur ScanNet (PointContrast $5{,}6$ ; MSC $21{,}8$ ; contre $63{,}1$ pour des features DINOv2 reprojetées, la suppression du décodeur valant à elle seule $20{,}7 \to 60{,}4$). L'écart entre les deux régimes ne vient donc pas de la méthode mais du régime de mesure.

Les concurrents à battre sont chiffrés — SemanticKITTI, mIoU par fraction d'étiquettes, tels que rapportés par BEVContrast :

| Méthode | 0,1 % | 1 % | 10 % | 50 % | 100 % |
|---|---|---|---|---|---|
| scratch | 30,0 | 46,2 | 57,6 | 61,8 | 62,7 |
| PointContrast | 32,4 | 47,9 | 59,7 | 62,7 | 63,4 |
| SegContrast | 32,3 | 48,9 | 58,7 | 62,1 | 62,3 |
| DepthContrast | 32,5 | 49,0 | 60,3 | 62,9 | 63,9 |
| STSSL | 32,0 | 49,4 | 60,0 | 62,9 | 63,3 |
| ALSO | 35,0 | 50,0 | 60,5 | 63,4 | 63,6 |
| TARL | 37,9 | 52,5 | 61,2 | 63,4 | 63,7 |
| BEVContrast | 39,7 | 53,8 | 61,4 | 63,4 | 64,1 |

À 100 %, huit méthodes tiennent dans $1{,}8$ point, sous le bruit ; à 0,1 %, l'écart scratch–BEVContrast est de $9{,}7$ points. C'est la seule raison méthodologique qui suffise à justifier le déplacement du plan expérimental vers le régime à peu d'étiquettes : ce n'est pas un régime plus facile, c'est le seul où un effet réel est distinguable du bruit.

Sonata ne peut pas servir de baseline reproduite. Son modèle extérieur est un préentraînement séparé — « we adapt pre-training paradigm of Sonata to outdoor LiDAR scenarios through joint training on nuScenes, Waymo, and SemanticKITTI », les 140k scènes intérieures ne servant pas aux chiffres KITTI —, son coût n'est pas documenté, et il déclare lui-même : « Currently, Sonata separates pre-training for each setting ». Les poids extérieurs n'ont jamais été publiés (aucune config Sonata kitti/nuscenes/waymo dans Pointcept, issues #456 et #469 ouvertes) et les poids publiés sont en CC-BY-NC 4.0 : à traiter comme `HGP-old`, cité, jamais importé dans la ligne produit. Les successeurs du même groupe ont en revanche des configs KITTI publiées (`configs/concerto/semseg-ptv3-large-v1m1-kitti-4a-lin.py`, `configs/utonia/...-6a/6b/6c-kitti-lin/dec/ft.py`, poids Concerto également CC-BY-NC) : toute comparaison reproductible à cette famille passe par eux, et par les chiffres publiés pour Sonata.

## Risques de nouveauté

Quatre travaux occupent déjà une partie de la revendication. Ils se citent en tête de l'état de l'art, jamais en note de bas de page.

| Travail | Ce qu'il possède déjà | Ce qui nous en sépare |
|---|---|---|
| **HASSL** (arXiv 2607.04353, 7 juillet 2026) | hiérarchie HDBSCAN multi-niveaux comme structure d'auto-supervision, avec prototypes par niveau | domaine : microscopie cellule unique. Notre position ne peut être qu'un **transfert** au LiDAR extérieur assorti d'une **exactification** : niveaux exacts, récupérabilité prédite par percolation, échantillonnage capteur inhomogène. C'est le risque le plus sérieux du dossier et il doit être traité comme tel |
| **cTree** (Sharma & Kaul, NeurIPS 2020, arXiv 2009.14168) | deux tâches prétextes prédisant la décomposition hiérarchique d'un cover tree | hiérarchie **métrique** et non de densité ; échelle de l'objet et non de la scène LiDAR |
| **Part2Object** (ECCV 2024) | décomposition hiérarchique pour la segmentation d'instance non supervisée en 3D | antériorité de la hiérarchie non supervisée en 3D ; objet instance, niveaux non exposés comme signal de représentation |
| **Superpoint Transformer** (ICCV 2023) | partition hiérarchique multi-niveaux réellement utilisée par le réseau | **supervisée**, et issue de cut-pursuit, pas de la densité |

Conséquence de rédaction : la phrase « nous utilisons une hiérarchie comme signal d'auto-supervision » est déjà prise. Seule tient la phrase complète — hiérarchie **de densité**, **non condensée**, **exacte**, avec **prédiction du niveau utilisable**, en LiDAR **extérieur**.

## Les concurrents réels

Ce ne sont ni Sonata ni Concerto. **DOS** (Bosch et Freiburg, décembre 2025) mène le fine-tuning SemanticKITTI à $73{,}5$ pour $2$ A100 pendant $20$ h. **PointINS** (mars 2026) démontre déjà que des unités structurées paient — $+3{,}2$ PQ sur SemanticKITTI contre DOS — avec un pipeline plat et ad hoc, sans garantie. Se comparer à Concerto sur du LiDAR nu serait attaquer un modèle hors de son régime : sans couleur il tombe de $77{,}0$ à $36{,}8$ mIoU.

PointINS est donc à la fois la validation de l'hypothèse et le concurrent direct. La revendication doit se lire contre lui : **la structure doit être une hiérarchie de densité, gardée entière plutôt qu'aplatie, et ses niveaux se prédisent au lieu de se régler.** Voir [CONCURRENCE.md](../CONCURRENCE.md).

## L'ablation qui décide de tout

À architecture, recette et budget identiques, quatre bras :

| Bras | Hiérarchie fournie au modèle | Ce qu'il isole |
|---|---|---|
| (i) | HGP exact, arbre conservé | la proposition complète |
| (ii) | HDBSCAN, arbre **conservé** | l'apport de la hiérarchie **sans** l'exactitude |
| (iii) | HDBSCAN **condensé plat** (protocole TARL) | l'état de l'art du domaine |
| (iv) | arbre aléatoire de mêmes tailles et profondeur | le contrôle de vacuité |

| Observation | Conclusion |
|---|---|
| (i) $\approx$ (ii) $>$ (iii) | la contribution se réduit à « utiliser une hiérarchie », déjà pris par cTree et HASSL ; l'exactitude sort du papier et la soumission redevient une application |
| (i) $>$ (ii) $>$ (iii) | cas nominal : hiérarchie **et** exactitude sont toutes deux load-bearing |
| (ii) $\approx$ (iii) | l'arbre non condensé n'apporte rien : la revendication tombe entièrement |
| (iv) au niveau de (iii) | la mesure est vacue et ne conclut rien |

Conditions de validité, sans lesquelles aucun des quatre bras ne signifie quoi que ce soit :

| Règle | Fait qui l'impose |
|---|---|
| mêmes augmentations sur tous les bras | la recette domine l'architecture : LaserMix/PolarMix valent $+3{,}5$ mIoU sur MinkUNet ($66{,}9 \to 70{,}4$), instance cutmix + polarmix $+4{,}3$ sur WaffleIron ($62{,}5 \to 66{,}8$), plus que l'écart entre la plupart des architectures publiées. Un gain mesuré sans cette règle est un gain d'augmentation déguisé |
| trois graines minimum par bras | le backend TorchSparse fluctue d'environ $1{,}5$ mIoU selon la graine ; Pointcept #556 donne $66{,}5$ à $69{,}3$ selon le nombre de GPU et le batch. Tout gain sous environ $1{,}5$ point sur un run unique n'est pas distinguable du bruit |
| jamais un chiffre sans TTA contre un chiffre avec TTA | la TTA vaut $+1{,}4$ (MinkUNet $70{,}4 \to 71{,}8$), $+2{,}4$ (Cylinder3D), $+1{,}2$ (SphereFormer) ; celle de mmdetection3d coûte 36 passes avant |

Porteur de l'expérience — ceci **corrige** la recommandation antérieure qui désignait OpenPCSeg :

| Rang | Porteur | Pourquoi |
|---|---|---|
| 1 | **WaffleIron** | config unique et complète `configs/WaffleIron-48-256__kitti.yaml` ; checkpoint vivant ; README annonçant littéralement « This should give you a final mIoU of 68.0% » ; mainteneur répondant précisément sur les issues ($68{,}0$ en val, $70{,}8$ en test avec `--trainval` et 12 votes ; 4× RTX 2080 Ti pour environ 2 jours et 45 époques, un seul V100 32 Go suffisant) ; 6,8M paramètres ; **aucune extension CUDA à compiler** — ni torchsparse, ni MinkowskiEngine, ni spconv, ni flash-attn — installation pip simple compatible PyTorch 2.2. Ablation val du papier : $62{,}5 \to 66{,}8 \to 67{,}6 \to 68{,}0$ |
| 2 | **MinkUNet** | second porteur de famille radicalement différente (convolution sparse voxelisée contre MLP et convolutions 2D denses sur projections) : mmdetection3d `minkunet34_w32_spconv` à $69{,}3$ avec poids et log vivants, variante torchsparse annoncée à $70{,}3$ ; OpenPCSeg à $70{,}04$ sur 2 A100 en environ 12 h, « trained with merely train split » et « without employing any Test Time Augmentation or ensembling ». Vérifier l'appariement config/poids : $70{,}04$ est annoncé via `minkunet_mk34_cr10.yaml` alors que le fichier de poids s'appelle `mk34_cr16` |
| — | **PTv3/Pointcept, à éviter** | issue #556 (13 janvier 2026) : trois reproductions indépendantes à $69{,}30$ / $66{,}52$ / $66{,}53$ contre $70{,}8$ annoncé ; issue #186 ouverte depuis mars 2024 ; issue #481 où le mainteneur admet « I already forgot where I got this number during paper writing ». Pointcept n'implémente ni LaserMix ni PolarMix, ce qui explique probablement une partie de l'écart. S'il est utilisé malgré tout, ne reporter que sa propre baseline réentraînée |

Le $68{,}0$ de WaffleIron est nu : aucun chiffre val avec TTA n'a jamais été publié pour lui, ce qui en fait une référence propre tant qu'on ne lui oppose pas un chiffre augmenté.

## Classement des actifs

Ce classement décide de ce qui va dans le titre et de ce qui reste en appendice ; l'ordre des mesures qui l'instruit figure dans [VOIES.md](../VOIES.md).

| Actif | Rang | Rôle |
|---|---|---|
| arbre non condensé, exactitude des niveaux | **fort, en hausse** | la revendication elle-même : titre et résumé |
| analyse par percolation | **fort, en hausse** | dit quels niveaux sont utilisables, donc rend l'exactitude opérationnelle au lieu de décorative |
| descripteur de nœud | **faible, en baisse** | lemme justificatif d'appendice, jamais une contribution |
| HSA | **opérateur, pas contribution** | baseline nécessaire et correcte ; le reproduire ne produit aucune nouveauté |

**HGP et percolation.** La correspondance exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN, la fonction de percolation, sa vitesse et la limite gaussienne $\mu=K+a\sqrt{K}$ forment une théorie quantitative de la fraction d'un cluster récupérable avant fusion parasite. Aucune équipe de vision 3D ne dispose de cet outil, et il répond exactement à la question que la littérature des partitions ne sait pas poser : quelle hiérarchie est récupérable, et à quel niveau. C'est le maillon 5 de la chaîne ci-dessus, donc une pièce structurelle et non un ornement.

**Descripteur.** Le théorème de caractérisation du canal support ([DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md)) reste vrai mais recombine du folklore — mesures idempotentes et maxitives, trichotomie de Fung-Fu, équation de translation d'Aczél, fonctions support de Schneider — et la grandeur caractérisée, $h(u_{k})=\max_{i}\langle x_{i},u_{k}\rangle$, est littéralement un PointNet à première couche linéaire, le maximum sur les enfants étant déjà l'équation 1 de Superpoint Transformer. Les ablations publiées sur cette famille exacte le confirment et le font descendre encore : sur Superpoint Transformer, retirer toutes les caractéristiques manuelles de nœud coûte $-0{,}7$ mIoU sur S3DIS 6-fold, $-4{,}1$ sur KITTI-360 et $-1{,}4$ sur DALES, tandis que retirer l'encodage d'adjacence coûte $-6{,}3$, $-5{,}4$ et $-3{,}0$, et que **passer à un seul niveau de partition** coûte $-8{,}4$, $-5{,}1$ et $-0{,}9$ ; EZ-SP rapporte que remplacer les caractéristiques manuelles par un petit réseau appris ne déplace le résultat que de $\pm 0{,}1$ mIoU. Le poste le plus coûteux après l'adjacence est le nombre de niveaux : c'est un appui externe direct au « ne pas condenser », et la démonstration que le descripteur est le levier le plus faible des trois.

**HSA.** Papier d'une autre équipe, validé sur du texte uniquement, sans aucune expérience 3D ni dense. Sous LayerNorm, l'énergie d'interaction ne dépend que des moyennes pondérées par la taille des requêtes et des clés de chaque sous-arbre : une HSA fidèle est mécaniquement une attention sur moyennes de sous-arbres, très proche du contrôle bottom-up/top-down `mean` + MLP déjà prévu dans la matrice d'ablation. Les auteurs indiquent eux-mêmes que leur cadre n'ajoute aucun paramètre apprenable le long de la hiérarchie ; le descripteur n'y entre que par $\varepsilon(A')^{\top}\varepsilon(B')$, soit un scalaire de biais par couple de frères ; le remplacement zero-shot fait tomber QNLI à $0{,}5072$, le niveau du hasard. L'algorithme demande enfin $D$ produits matrice creuse–vecteur séquentiels, où $D$ est la profondeur : la condensation de l'arbre de fusion est une condition d'existence sur GPU, pas une optimisation. Elle **modifie l'objet** et entre donc en tension frontale avec la revendication « ne pas condenser » ; il faut la versionner, l'ablater, et rapporter jusqu'où l'on condense pour tenir en mémoire.

### Le point théorique le plus prometteur

L'extension de l'analyse de percolation à une **intensité inhomogène** $\lambda(x)$ modélisant la portée du capteur est le seul développement qui porterait à lui seul la partie mathématique du papier. En toute généralité c'est difficile, et il faut le dire sans détour : les résultats disponibles sont énoncés pour un processus homogène, la fonction de percolation et sa vitesse perdent leur sens usuel dès que l'intensité varie, et rien ne garantit qu'un seuil critique global subsiste sous une intensité décroissant avec la portée.

La version atteignable est locale : sur une fenêtre où $\lambda$ varie peu, un changement d'échelle ramène au cas homogène et prédit un déplacement du niveau critique comme fonction explicite de la portée, à confronter sur SemanticKITTI au niveau de fusion mesuré par bins de portée. Même local et conditionnel à ses hypothèses, ce résultat répondrait à R1 — la hiérarchie encode le capteur, pas la sémantique — dans le geste même où il fournit le maillon 5. À tenter tôt, avec un critère d'abandon explicite si l'approximation locale ne se vérifie pas.

## Contribution minimale pour ICML/NeurIPS

| # | Élément | Non négociable ? |
|---|---|---|
| 1 | l'ablation à quatre bras ci-dessus, recette et budget identiques, trois graines, IC à 95 % | oui : sans elle il n'y a pas de revendication |
| 2 | régime à peu d'étiquettes (0,1 / 1 / 10 %) contre TARL et BEVContrast au même protocole | oui |
| 3 | récupérabilité des niveaux par percolation, version inhomogène locale, confrontée aux niveaux de fusion mesurés par bins de portée | oui : c'est le maillon 5 |
| 4 | contrat reproductible du complexe HGP marqué, sans construction exhaustive du complexe ambiant | oui |
| 5 | coût complet : extraction des incidences, construction de la hiérarchie et, pour `witness_union`, $N_W$, $\varepsilon_W$, requêtes, patches et échantillons | oui |
| 6 | second capteur ou dataset, avec mécanisme cohérent | oui |
| 7 | comparaison SPT/EZ-SP, PTv3, SP2T, LSK3DNet, SphereFormer, RAPiD-Seg, LitePT en contrôle architectural même sans score SemanticKITTI publié, statut TTA de SphereFormer conservé à `NR` | oui |

Au moins une contribution générale par-dessus : **optimalité conditionnelle** (`QC-HSA`, projection reverse-KL conditionnée par la feuille, certificat d'oscillation — centrale seulement avec un certificat HGP non vacu ou un résultat fidélité–coût réellement nouveau) ; **stabilité** de la hiérarchie sous échantillonnage range-dependent ; **représentation** (encodeur invariant aux identifiants et aux certificats sparse équivalents, face à MPSN/CWN/EMPSN/SAT/TopNets) ; **statistique** (lien vérifiable entre qualité d'un cluster tree et erreur de propagation sémantique).

Un seul nouveau score SemanticKITTI, même premier, est trop fragile pour porter seul une soumission généraliste — et, à 100 % d'étiquettes, il serait sous le plancher de bruit.

## Choix de venue

Les backbones de segmentation 3D vont en CVPR, ICCV ou ECCV, où ils sont jugés sur l'ingénierie du backbone autant que sur l'idée — recette, budget de calcul, augmentations —, ce qui n'est pas l'avantage comparatif de ce projet. NeurIPS et ICML acceptent la perception 3D quand le cadrage est représentation, auto-supervision, passage à l'échelle ou théorie ; les précédents sont nommables : PTv2 (NeurIPS 2022), Seal (NeurIPS 2023, spotlight), SFCNet (NeurIPS 2024), Concerto (NeurIPS 2025), Utonia (ICML 2026). Une revendication « ne pas condenser », validée en régime à peu d'étiquettes et adossée à une théorie de la récupérabilité des niveaux, est une soumission ICML/NeurIPS plausible **sans exiger le premier rang d'un classement**. La venue se choisit avant de figer le plan expérimental, non après.

## Claims autorisés et preuves requises

| Claim potentiel | Preuve minimale |
|---|---|
| l'arbre non condensé bat sa propre condensation plate | bras (ii) contre (iii), même recette et mêmes augmentations, trois graines, IC excluant zéro |
| l'exactitude de la hiérarchie change une métrique aval | bras (i) contre (ii), même consommateur d'arbre et même budget ; sans ce résultat, l'exactitude sort du papier |
| les niveaux ont le même sens d'une scène à l'autre | prédiction de percolation confrontée au niveau de fusion mesuré, stratifiée par bins de portée |
| gain en régime à peu d'étiquettes | 0,1 / 1 / 10 %, protocole TARL/BEVContrast, trois graines, contre scratch $46{,}2$, TARL $52{,}5$ et BEVContrast $53{,}8$ à 1 % |
| HGP est un meilleur prior | arbres échangés à budget constant, seeds appariées, IC excluant zéro |
| le complexe HGP apporte de la géométrie utile | points, accès à $\Gamma_K^{\mathrm{elem}}$, sac de tokens sans messages, incidences du contrat, mutant invalide, MPSN/CWN/EMPSN/SAT et Deep Sets à capacité égale |
| le support source aide le complexe source/PL | complexe seul contre support source + complexe, mêmes `payload_kind`, `carrier_kind`, `authority`, coupe, cellules, capacité et recette ; aucun transfert au support de `witness_union` |
| HSA exploite mieux HGP | même arbre/features contre pooling et message passing |
| QC-HSA est la projection optimale annoncée | preuve complète, solveur dense sur petits arbres, inclusion HSA et facteur de cardinalité vérifiés |
| QC-HSA préserve mieux les points | reverse-KL et erreurs de frontière inférieurs à HSA, coût $C_T$ et latence inclus |
| robuste à longue portée | gains par bins et perturbations, pas seulement moyenne globale |
| plus efficace | latence et mémoire end-to-end sur même matériel |
| SOTA LiDAR mono-trame | audit frais, protocole strict, résultat test caché |
| général | second dataset/capteur et mécanisme cohérent |

Ne pas revendiquer :

- nouveauté par le seul usage d'une hiérarchie (cTree, HASSL), de la densité (HDBSCAN standard depuis TARL) ou de l'absence de caméra (TARL, SegContrast, BEVContrast, ALSO, STSSL, ALPINE) ;
- l'exactitude comme avantage tant que le bras (ii) n'a pas été mesuré ;
- un gain obtenu contre un bras dont les augmentations diffèrent, ou sous $1{,}5$ point sur un run unique ;
- une comparaison à un chiffre publié avec TTA, ensemble ou recette différente, ni au $70{,}8$ de PTv3 non reproduit ;
- une reproduction de Sonata extérieur — poids jamais publiés — ni l'import de poids CC-BY-NC dans la ligne produit ;
- invariance rotationnelle d'un vecteur sur directions fixes ;
- préservation complète de la géométrie par fonction support ;
- préservation de la non-convexité par seul rayon extérieur hors cas étoilé ;
- nouveauté par le seul usage d'un réseau ou d'une attention simpliciale ;
- « complexe complet » sans préciser le contrat reconstruit et les cellules omises ;
- opérateur DAG conservatif tant que les poids $w_{iv}$, leur domaine et la contrainte $\sum_v w_{iv}=1$ ne sont pas définis et testés ;
- nouveauté par la seule utilisation d'ECT/WECT, de Fourier ou d'un kernel mean ;
- complexité linéaire sans bornes de degré et mesure réelle ;
- optimalité sémantique ou approximation d'une attention arbitraire à partir du théorème KL très borné de HSA ;
- certificat mIoU à partir de Pinsker ou d'une marge point-wise ;
- exactitude ou statut GPU de MorseHGP3D hors registre.

## Titre et résumé de travail

Titres possibles, à choisir après les résultats ; tous portent la même revendication :

- *Don't Condense: Keeping the Density Cluster Tree for LiDAR Self-Supervision*
- *Levels That Mean Something: Exact K-NN Density Hierarchies for Outdoor Pre-Training*
- *From Flat Segments to Trees: Hierarchy-Level Supervision for LiDAR*

Résumé de travail :

> L'auto-supervision LiDAR fabrique ses unités d'entraînement avec HDBSCAN, condense la hiérarchie de densité en une partition plate, puis jette l'arbre. Nous le conservons : nœuds internes, relation parent–enfant et niveaux deviennent le signal. Utiliser les niveaux exige qu'un niveau ait le même sens d'une scène à l'autre, ce que fournit la coïncidence exacte entre $K$-polyèdres et amas discrets de forte densité $K$-NN, et exige de savoir lesquels sont récupérables sous échantillonnage capteur inhomogène, ce que fournit l'analyse de percolation. Une ablation à quatre bras — hiérarchie exacte, hiérarchie heuristique conservée, hiérarchie heuristique condensée, arbre aléatoire — sépare, à recette et budget identiques, l'apport de la hiérarchie de celui de l'exactitude. L'évaluation porte sur SemanticKITTI en régime à peu d'étiquettes, où la marge disponible dépasse le plancher de bruit, puis sur un second capteur.

Ce résumé ne doit recevoir aucun chiffre avant que les expériences soient terminées.

## Figures décisives

1. **Schéma d'architecture** : backbone local, graphe d'incidence point–facette, hiérarchie HGP, support global, HSA tardif et décodeur point-fin.
2. **Résultat QC-HSA** : rectangles HSA contre partitions feuille–sous-arbre, projection fermée, coût supplémentaire et pont conditionnel vers les hauteurs de fusion.
3. **Diagnostic de compression dure** : vote majoritaire réalisable et union par classe optimiste, distincts du modèle à proportions et de sa sortie point-wise.
4. **Ablation à quatre bras** : (i) exact conservé, (ii) heuristique conservé, (iii) heuristique condensé, (iv) aléatoire, par fraction d'étiquettes, avec IC sur trois graines.
5. **Stabilité capteur** : variation de hiérarchie et mIoU selon portée/thinning.
6. **Représentations et collisions** : mêmes sommets/support mais incidences distinctes, certificats sparse équivalents et perturbations de filtration ; le hash canonique les sépare, tandis que les collisions du learned encoder sont mesurées jusqu'à preuve d'expressivité.
7. **Pareto système** : mIoU contre latence/VRAM, coût HGP inclus, avec $N_W$ et $\varepsilon_W$ pour l'union témoin.
8. **Analyse d'erreurs** : frontière sémantique traversée par une branche et rôle du gate résiduel.

## Tables décisives

- les quatre bras × fractions d'étiquettes 0,1 / 1 / 10 / 100 %, contre scratch, TARL et BEVContrast au même protocole ;
- comparaison track A strict, avec colonnes modality, temporal, external data, TTA, ensemble ;
- ablation HGP $K=1$/SL comme fixture, puis HGP $K=2,3$ vs RSL/octree/superpoints/random ;
- points seuls, $\Gamma_K^{\mathrm{elem}}$ avec tokens précalculés, sac des mêmes tokens sans messages, complexe d'incidence, complexe + support source et mutant d'incidences invalide, à budget égal ;
- encodeur proposé vs MPSN/CWN/EMPSN/SAT/TopNets et Deep Sets ;
- HSA vs pooling/message passing/local attention ;
- QC-HSA vs HSA : KL, sortie, frontière, mIoU, $C_T$, VRAM et latence ;
- IoU par classe et distance ;
- temps construction/arbre/extraction des incidences/encodeur/réseau/reprojection ;
- second dataset et changement de capteur.

## Expériences qui doivent pouvoir être négatives

- $K=2$ n'est pas forcément meilleur pour la sémantique malgré son résultat instance avec masques GT ;
- le complexe contractuel peut être redondant avec un backbone local fort ;
- le support peut ne rien ajouter au complexe seul ;
- un MPSN standard peut dominer l'encodeur proposé ;
- HSA peut être dominé par un simple passage top-down ;
- HGP brut peut être moins stable qu'un octree à longue portée ;
- une projection laminaire peut perdre l'intérêt des chevauchements d'ordre supérieur.

Ces résultats ne doivent pas être cachés. Bien analysés, ils déterminent le vrai papier et préviennent un claim faux.

## Pivots de publication

| Résultat | Observation | Papier |
|---|---|---|
| A | l'arbre non condensé gagne et HSA l'exploite | soumission complète : revendication, théorie de la récupérabilité, deux capteurs, Pareto ; sous réserve de l'audit de nouveauté |
| B | l'arbre gagne, HSA non | même revendication, agrégateur simple, contribution de stabilité/efficacité ; le mot HSA sort du titre |
| C | l'arbre n'aide que sous perturbation | robustesse et domain shift : arbre ou métrique corrigée de la portée, capteurs multiples |
| D | le support échoue, le complexe réussit | le support sort du modèle, la contribution devient structurelle (incidences contre graphes mélangés, encodeurs appariés) |
| E | aucune valeur face aux contrôles, (ii) $\approx$ (iii) | arrêt du projet modèle ; étude négative publiable si elle contient bornes, contre-exemples et audit reproductible, jamais présentée comme voie SOTA |
| G | (i) $\approx$ (ii) : l'exactitude n'ajoute rien | repli explicite sur « hiérarchie non condensée en LiDAR », positionné comme transfert de HASSL et cTree ; contribution nettement plus faible, viser alors une venue vision |

### Résultat F — HGP perd sur les classes fines et gagne sur les classes volumiques

Ce pivot manquait, alors qu'il correspond au mode d'échec que le manuscrit documente lui-même. Sur le jeu `birch2`, HDBSCAN à $k=100$ obtient un ARI de $0{,}996$ en classant $99{,}7$ % des points, contre $0{,}441$ et $83{,}9$ % pour HGP-Clusterer à $k=84$, la cause citée étant que « les clusters sont essentiellement filiformes et sont donc mieux identifiés avec de simples graphes ». Le mécanisme est structurel et non anecdotique : la connexité d'ordre $K$ exige $K$ points simultanément proches, condition qu'une structure mince échantillonnée de façon éparse ne satisfait qu'à un rayon nettement plus grand, si bien que l'objet fin naît tard dans la filtration et qu'à ce niveau ses voisines l'ont déjà rejoint. Le résultat observable est une sous-segmentation des objets fins, et non une fragmentation. Or la marge de progression du mIoU SemanticKITTI porte précisément sur `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist` et `fence`, les classes volumiques plafonnant déjà très haut.

Si les mesures confirment ce profil — gain sur les classes volumiques, perte sur les classes fines —, la publication honnête n'est pas un papier d'architecture mais une étude de la limite de la connexité d'ordre supérieur : à quelle dimension intrinsèque et à quelle densité d'échantillonnage l'exigence de $K$ points simultanément proches cesse d'être payante, mesurée par IoU et par oracle d'antichaîne stratifiés par classe fine contre classe volumique. Le manuscrit suggère une atténuation observée sur `birch2`, le changement d'estimateur $\hat{\rho}=1/r^{2}$, qui doit être testée mais ne dispense pas de mesurer d'abord l'ampleur du problème. Ce résultat reste publiable et utile pour la communauté ; il ne doit pas être présenté comme une voie vers le premier rang d'un classement.

## Segmentation d'instance : appendice futur, pas promesse actuelle

Après fermeture positive de la sémantique, l'arbre pourra fournir des propositions d'instances. La comparaison centrale sera ALPINE contre coupe HGP, à logits gelés. Cette extension ne doit apparaître dans une première soumission que si elle renforce une contribution déjà établie et n'affaiblit pas la profondeur de l'étude sémantique.

## Questions qui doivent recevoir une réponse falsifiable

1. L'arbre conservé bat-il sa propre condensation plate, bras (ii) contre (iii) ?
2. L'exactitude change-t-elle une métrique aval, bras (i) contre (ii) ?
3. Un niveau donné désigne-t-il la même chose d'une scène à l'autre, et la percolation prédit-elle lesquels sont utilisables par bins de portée ?
4. Les niveaux HGP restent-ils stables quand un même patch est transporté puis rééchantillonné selon la portée ?
5. Le canal vise-t-il l'union d'observations, le porteur PL des facettes, le porteur continu de multicoverture ou leur certificat commun ?
6. Facettes, cofaces, incidences, coordonnées, niveaux et plateaux suffisent-ils à reconstruire exactement cet objet à réindexage près ?
7. Pour $K\geq2$, comment les recouvrements ponctuels sont-ils conservés dans la branche d'incidence puis projetés vers la forêt HSA sans double comptage ?
8. L'arbre HGP améliore-t-il pureté, oracle de coupe et frontières face à RSL, octree, superpoints et arbre aléatoire à compression égale ?
9. L'encodeur distingue-t-il mêmes points et même support avec incidences différentes, puis bat-il points seuls, graphe $\Gamma^{K}$ seul et réseaux simpliciaux de même budget ?
10. Le gain vient-il de l'arbre, de la représentation ou de l'opérateur dans une ablation factorisée ?
11. HSA/QC-HSA améliore-t-il les points proches des frontières sans dégrader les classes rares, et jusqu'où faut-il condenser l'arbre pour tenir en mémoire ?
12. Le coût reste-t-il utile pour les arbres réels, y compris degrés, profondeur, condensation et $C_T$ ?
13. La méthode reste-t-elle compétitive sans TTA, ensemble, historique, RGB, pseudo-labels externes ou données externes ?
14. Le mécanisme se reproduit-il sur un second capteur/dataset ?

## Conditions de poursuite

Continuer vers un modèle complet seulement si :

- l'arbre conservé bat sa condensation plate, à recette et augmentations identiques, sur trois graines ;
- HGP exact bat l'arbre heuristique conservé, faute de quoi l'exactitude quitte la revendication ;
- l'effet ne disparaît pas sous stratification par portée et densité ;
- le complexe complet est sérialisé sans ambiguïté et son encodeur bat les mêmes points, le graphe $\Gamma^{K}$ seul et des contrôles simpliciaux à budget comparable ;
- `support + complexe` améliore le Pareto face à `complexe seul`, sinon le support est retiré sans remettre en cause la branche non convexe ;
- HSA ou son successeur bat pooling/message passing sur le même arbre ;
- les contraintes de mémoire permettent des batches et trois seeds réalistes.

L'ordre dans lequel ces conditions sont éprouvées n'est pas libre, et il ne commence pas par le descripteur : les chiffres SPT et EZ-SP cités plus haut placent ce levier au dernier rang, derrière l'adjacence et surtout le nombre de niveaux. Les mesures suivent l'ordre fixé par [VOIES.md](../VOIES.md), et la définition des canaux celle de [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md).

Sinon, publier le résultat négatif le plus informatif ou pivoter : étude de stabilité des arbres de densité LiDAR, descripteur topologique borné, ou benchmark causal de hiérarchies. Ne pas protéger l'histoire initiale en changeant simultanément backbone, arbre, descripteur et recette.

## Verdict honnête

Il n'existe aujourd'hui aucune base honnête pour promettre le SOTA, et à 100 % d'étiquettes le SOTA lui-même est sous le plancher de bruit. Le chemin solide est : « la littérature LiDAR condense la hiérarchie de densité et jette l'arbre ; nous le gardons, nous rendons ses niveaux comparables par l'exactitude, nous prédisons lesquels sont récupérables sous échantillonnage capteur, et nous le mesurons là où la marge dépasse le bruit ». Deux échecs suffisent à arrêter : si l'arbre conservé ne bat pas sa condensation plate, la revendication n'existe pas ; si l'exactitude ne change aucune métrique aval, elle sort du papier et il ne reste qu'un transfert de HASSL et cTree, à publier comme tel.

## Checklist avant rédaction

- concurrence réauditée à la date de soumission ;
- splits, ressources et test submissions documentés ;
- hypothèses et kill gates datés avant les runs finaux ;
- baselines reproduites avec leur code officiel lorsque possible ;
- seeds et intervalles présents ;
- coût HGP inclus ;
- théorie invoquée exactement dans son domaine ;
- limitations et résultats négatifs explicites ;
- second dataset terminé ;
- artefacts reproductibles et licence vérifiés.
