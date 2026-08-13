# Protocole expérimental

## Tâche primaire

Segmentation sémantique LiDAR **single-scan** SemanticKITTI : une classe prédite pour chaque point à partir de ses coordonnées 3D et de sa rémission. Les classes moving/non-moving sont fusionnées et 19 classes sont évaluées.

La métrique primaire officielle est $\mathrm{mIoU}=\frac{1}{C}\sum_{c=1}^{C}\frac{TP_c}{TP_c+FP_c+FN_c}$ avec $C=19$.

## Splits

- entraînement : séquences `00–07`, `09`, `10` ;
- validation : séquence `08` ;
- test caché : séquences `11–21` ;
- source de vérité : `semantic-kitti-api/config/semantic-kitti.yaml` versionné avec chaque expérience.

Les labels test ne sont pas accessibles. La compétition actuelle autorise au plus dix soumissions ; le projet vise une soumission finale par configuration revendiquée, après gel du code, des poids et du protocole. Aucun hyperparamètre n'est choisi à partir du serveur test.

## Régimes de comparaison

### Track A — claim principal strict

- un scan ;
- `x`, `y`, `z`, rémission et features déterministes dérivées ;
- entraînement SemanticKITTI uniquement ;
- aucun RGB, passé/futur, préentraînement externe annoté ou pseudo-label externe ;
- aucune TTA ni ensemble.

### Track B — préentraînement

Même entrée à l'inférence, mais préentraînement ou entraînement multi-datasets. La provenance et les labels utilisés sont rapportés. PTv3+PPT et M3Net appartiennent ici.

### Track C — temporal ou multimodal

RGB, poses, plusieurs scans ou historique autorisés. TASeg et UniSeg appartiennent ici. Ce track contextualise le plafond pratique, mais ne décide pas le claim du track A.

### Track D — TTA/ensemble

Toute augmentation au test, fusion de checkpoints ou ensemble est isolé. Le résultat principal ne doit jamais dépendre de ce track.

## Métriques obligatoires

### Qualité

- mIoU global et IoU des 19 classes ;
- moyenne thing/stuff en diagnostic, sans remplacer le mIoU officiel ; les huit classes thing sont `car`, `bicycle`, `motorcycle`, `truck`, `other-vehicle`, `person`, `bicyclist`, `motorcyclist`, et les onze autres classes évaluées sont stuff ;
- accuracy globale, seulement comme métrique secondaire ;
- matrice de confusion ;
- F-score de frontière diagnostique sur un graphe $G_{\mathrm{diag}}$ symétrique 16-NN en XYZ, dont les arêtes supérieures à 1 m sont retirées : les ensembles de frontières GT et prédites sont construits avec la même règle, un point valide étant frontière si un voisin valide porte une autre classe ; la précision est la fraction des frontières prédites situées à au plus 0,2 m d'une frontière GT, le rappel la fraction des frontières GT situées à au plus 0,2 m d'une frontière prédite, et le F-score leur moyenne harmonique ; les labels ignorés sont exclus ;
- expected calibration error et NLL pour vérifier les gates/incertitudes ;
- KL/JS, Brier et erreur absolue de $\widehat\pi_v^{\mathrm{lab}}$ contre $\pi_v$, calculés hors forward sur les seuls points valides, par profondeur, masse, classe et portée ; ces diagnostics de nœuds ne remplacent jamais le mIoU point-wise.

### Stratification LiDAR

- distances `0–10`, `10–20`, `20–30`, `30–40`, `40–50` et `>50 m` ; l'API officielle retient strictement `depth > lo` et `depth < hi`, tandis que `>50 m` est une extension locale versionnée ;
- cardinalité de l'instance GT pour les classes thing et, pour stuff, de la composante de même classe dans $G_{\mathrm{diag}}$ ; ces identifiants GT ne servent qu'au diagnostic ;
- densité locale et élévation ;
- classes rares et classes thing séparément ;
- profondeur et distribution GT $\pi_v$ des ancêtres HGP sollicités, avec pureté définie par $\max_c\pi_v(c)$, rapportées séparément par bloc et par tête ; si un gate existe, une profondeur effective pondérée par ce gate est ajoutée, sans inventer un unique « premier ancêtre ».

Utiliser l'évaluateur officiel par distance lorsqu'il s'applique, et documenter toute extension au-delà de 50 m.

### Système

- paramètres entraînables et taille du checkpoint ;
- MACs/FLOPs avec convention déclarée ;
- pic RAM et VRAM ;
- batch size maximal et effectif ;
- latence P50/P95, débit et warm-up ;
- temps de construction $K$-NN/HGP, descripteurs, forward réseau et reprojection, séparément ;
- latence end-to-end, incluant toutes les étapes ;
- distribution des nœuds, degrés, profondeurs et $\sum_v d_v^2$.
- nombre d'interactions point–sous-arbre $C_T=\sum_i|\Pi_T(i)|$ pour `QC-HSA`.

Le matériel, les versions CUDA/PyTorch, la précision numérique et les kernels utilisés sont enregistrés. Les chiffres de publications mesurés sur un autre GPU ne sont pas comparés comme s'ils provenaient du même banc.

## Baselines minimales

### Backbones métier

- une recette SemanticKITTI officielle ou publiquement complète et épinglable, par exemple MinkUNet/Cylinder3D ou SphereFormer auditée, pour WP0 ;
- PTv3 SemanticKITTI-only comme portage Transformer ambitieux après reproduction, son dépôt officiel ne fournissant pas actuellement un paquet SemanticKITTI complet config+poids+score ;
- PTv3+PPT, dans le track préentraînement ;
- LSK3DNet, concurrent sparse LiDAR fort ;
- SP2T, concurrent Transformer LiDAR mono-trame à proxies sparse ;
- RAPiD-Seg, concurrent range-aware et descripteur ;
- SphereFormer, attention conçue pour la densité dépendante de la portée ;
- MinkUNet ou Cylinder3D, baseline sparse simple et largement reproductible.

### Hiérarchies

- aucune hiérarchie ;
- voxel tree/octree équilibré ;
- HGP $K=1$/single-linkage, dont l'identité est une fixture de cohérence et non deux baselines ;
- RSL/HDBSCAN ;
- superpoints hiérarchiques SPT ou approximation fidèle ;
- HGP $K=2,3$ ;
- arbre aléatoire préservant taille, degré et profondeur autant que possible ;
- permutation des associations de feuilles comme null test.

### Opérateurs sur arbre

- mean/max pooling + MLP ;
- passage bottom-up/top-down ;
- message passing parent–enfant et frères ;
- HSA fidèle ;
- `QC-HSA` conditionnée par la requête ;
- Sequoia/attention hiérarchique locale si l'adaptation est raisonnable ;
- attention locale supplémentaire de même budget.

### Représentations de nœuds

- aucune géométrie, seulement pooling des features ;
- support maximal ;
- support normalisé ;
- support normalisé + taille/position ;
- support de la réalisation géométrique du $K$-polyèdre, qui doit coïncider numériquement avec le support de ses sommets ;
- distributions de centres/formes/niveaux des simplexes du $K$-polyèdre ;
- support + densité/persistance HGP ;
- CDF/histogrammes de projections à bins fixes + max ;
- pile de quantiles + max ;
- moments/covariance/radial ;
- mini-PointNet/Deep Sets à dimension comparable.

## Matrice d'expériences

La matrice complète est factorielle et trop coûteuse. Elle est déroulée séquentiellement :

| Étape | Variable changée | Variables gelées | Décision |
|---|---|---|---|
| E0 | granularité/coupe | labels GT, aucun réseau | composition des nœuds et diagnostic d'une sortie dure token-constante |
| E1 | hiérarchie | backbone + agrégateur simple | valeur de HGP |
| E2 | descripteur | HGP + backbone + budget | valeur du support |
| E3 | opérateur | HGP + descripteur + budget | HSA contre QC-HSA et agrégateurs |
| E4 | placement/nombre de blocs | meilleure configuration E1–E3 | architecture finale |
| E5 | backbone | module HGP gelé | généralité |
| E6 | perturbation/capteur | modèle gelé | robustesse |
| E7 | coût/scaling | modèle gelé | viabilité système |

Chaque étape conserve l'identifiant de la configuration parente et ne change qu'un facteur principal. Les interactions jugées importantes sont testées ensuite, jamais absorbées dans une unique expérience finale.

Pour E3, de petits scans ou sous-échantillons où l'attention plate est calculable rapportent aussi le reverse-KL total et par feuille de HSA et `QC-HSA`, la fraction de feuilles dont la classe plate à marge fixée est préservée, et l'écart de sortie. Ce test utilise une même cible plate $P$ gelée, les mêmes features, scores et masque diagonal pour flat/HSA/QC-HSA ; comparer les KL de modèles entraînés séparément ne testerait pas la proposition. Ces diagnostics vérifient le résultat technique ; ils ne remplacent pas le mIoU contre la vérité terrain.

## Seeds et incertitude

- trois seeds au minimum pour les explorations retenues ;
- cinq seeds pour les ablations centrales du papier si le coût le permet ;
- même liste de seeds entre variantes appariées ;
- moyenne, écart-type et résultats individuels ;
- bootstrap apparié de blocs temporels contigus, car les frames de la séquence 08 ne sont pas indépendantes : chaque réplication somme les matrices de confusion des blocs tirés, puis recalcule les 19 IoU et le mIoU officiel ; aucune moyenne de « mIoU par scan » n'est utilisée ;
- incertitude entre seeds rapportée séparément, ou bootstrap hiérarchique seed × blocs explicitement pré-enregistré ;
- intervalle de confiance à 95 % avec longueur de bloc et nombre de réplications pré-enregistrés.

Les gains de quelques dixièmes sans intervalle ne sont pas considérés établis. Les hyperparamètres et la seed de sélection ne doivent pas être choisis après inspection du test.

## Stress tests pré-enregistrés

### Portée et densité

- thinning aléatoire à plusieurs taux ;
- thinning croissant avec la portée ;
- suppression de bandes d'élévation simulant des beams manquants ;
- même objet ou patch déplacé synthétiquement à plusieurs distances, avec règle de rééchantillonnage déclarée ;
- métrique HGP brute contre correction de portée/densité.

### Bruit et points extrêmes

- jitter des coordonnées ;
- un outlier, puis 1/2/5 % d'outliers à plusieurs rayons ;
- suppression ciblée des points exposés contre suppression de points intérieurs ;
- arbre figé, puis arbre recalculé, pour séparer descripteur et hiérarchie.

### Vue et occultation

- correspondances entre objets/patches visibles dans des scans voisins, uniquement comme diagnostic ;
- variation de support, niveau HGP et prédiction selon angle et nombre de retours ;
- aucun scan voisin n'entre dans le modèle du track A.

### Transformations

- translation et scaling diagnostiques du canal normalisé ;
- rotations yaw cohérentes avec l'augmentation ;
- rotations SO(3) seulement comme test de compréhension, car l'invariance totale à la gravité est indésirable ;
- vérification de la cohérence entre transformation des points, directions support et embeddings relatifs.

## Protocole de reproduction

Chaque run sauvegarde :

- commit et état du worktree ;
- config complète, dépendances et commande ;
- hash du dataset manifest et des hiérarchies ;
- seed Python/NumPy/PyTorch/CUDA et mode déterministe ;
- métriques brutes par scan et par classe ;
- distributions point-wise et proportions de nœuds nécessaires aux diagnostics de composition, sans employer les proportions GT comme features ;
- logs de latence/mémoire ;
- checkpoint final et critère de sélection ;
- statut `completed`, `failed` ou `invalid`, sans suppression des échecs.

Une table de résultats n'accepte que les runs dont tous ces champs sont présents.

## Règle de soumission test

Avant le test caché :

1. réauditer les papiers et leaderboards avec date ;
2. geler architecture, données, entraînement et post-traitement ;
3. exécuter les seeds et stress tests prévus ;
4. produire un manifeste de prédictions et valider son format avec l'API officielle ;
5. documenter le régime de ressources ;
6. envoyer une seule configuration principale, puis n'utiliser une autre soumission que pour une raison pré-déclarée.

## Extension instance ultérieure

Une fois la sémantique fermée, le protocole panoptique rapportera PQ, PQ†, RQ, SQ, PQthing, PQstuff et mIoU. ALPINE, coupe HGP et coupe HGP apprise recevront les mêmes logits sémantiques gelés. Aucun résultat panoptique ne sera mélangé au tableau principal de la phase sémantique.
