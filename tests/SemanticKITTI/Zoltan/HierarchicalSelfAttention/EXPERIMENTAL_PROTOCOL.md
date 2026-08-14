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
- **dimension intrinsèque estimée du nœud**, lue sur les valeurs propres $\lambda_1\geq\lambda_2\geq\lambda_3$ de sa covariance et résumée par linéarité, planarité et diffusion, avec une strate filiforme déclarée par seuil versionné. Cette strate est obligatoire parce qu'elle rend visible le mode d'échec propre à la connexité d'ordre $K$ : le manuscrit rapporte sur `birch2` un ARI de $0{,}996$ et $99{,}7\,\%$ de points classés pour HDBSCAN contre $0{,}441$ et $83{,}9\,\%$ pour HGP-Clusterer, et attribue l'écart au fait que « les clusters sont essentiellement filiformes et sont donc mieux identifiés avec de simples graphes ». Le mécanisme attendu est qu'une structure mince échantillonnée de façon éparse ne fournit $K$ points simultanément proches qu'à un rayon nettement plus grand, donc naît tard dans la filtration, à un niveau où ses voisines l'ont déjà rejointe ; l'observable est une **sous-segmentation des objets fins**. Or la marge de progression du mIoU SemanticKITTI porte exactement sur `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist` et `fence`, et l'atténuation suggérée par le manuscrit sur `birch2` — changer d'estimateur, $\hat\rho=1/r^{2}$ — est testée comme variante, pas supposée ;
- **cardinal $n_v$ du nœud**, par octaves, pour rendre visible le mauvais calibrage du descripteur aux deux bouts : sur les gros nœuds, un latent de dimension $D$ ne représente fidèlement que des ensembles de cardinal au plus $D$ (Wagstaff et al., ICML 2019), donc le canal y est lossy par construction ; sur les petits nœuds, le nombre de directions ou de bins non vides est inférieur au nombre de points, donc le canal est majoritairement du masque. Les deux régimes sont rapportés séparément et jamais moyennés ensemble ;
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
- temps d'export du complexe HGP, initialisation des cellules, passages d'incidence et readout, séparément ;
- latence end-to-end, incluant toutes les étapes ;
- distribution des nœuds, degrés, profondeurs et $\sum_v d_v^2$ ;
- nombres uniques $N_V$, $N_F$, $N_Q$, $N_I$ et $N_A$ de sommets, facettes, cofaces de connexion, incidences et appartenances cellule–nœud, plus le taux de réutilisation des cellules entre niveaux ;
- pour `carrier_kind=witness_union`, nombre total $N_W$ d'opérations géométriques, ventilé en requêtes d'appartenance ou de distance, patches de frontière et échantillons, ainsi que l'erreur $\varepsilon_W$ contre l'oracle borné, le temps et les pics RAM/VRAM propres à cette voie ;
- nombre d'interactions point–sous-arbre $C_T=\sum_i|\Pi_T(i)|$ pour `QC-HSA`.

Le matériel, les versions CUDA/PyTorch, la précision numérique et les kernels utilisés sont enregistrés. Les chiffres de publications mesurés sur un autre GPU ne sont pas comparés comme s'ils provenaient du même banc.

## Baselines minimales

### Backbones métier

- une recette SemanticKITTI officielle ou publiquement complète et épinglable, par exemple MinkUNet/Cylinder3D ou SphereFormer auditée, pour WP0 ;
- PTv3 SemanticKITTI-only comme portage Transformer ambitieux après reproduction, son dépôt officiel ne fournissant pas actuellement un paquet SemanticKITTI complet config+poids+score ;
- PTv3+PPT, dans le track préentraînement ;
- LSK3DNet, concurrent sparse LiDAR fort ;
- SP2T, concurrent Transformer LiDAR mono-trame à proxies sparse, dont le score publié 75,4 emploie des augmentations au test ;
- RAPiD-Seg, concurrent range-aware et descripteur à deux inférences séquentielles ;
- SphereFormer, attention conçue pour la densité dépendante de la portée, dont la TTA n'est pas rapportée dans la source primaire auditée ;
- LitePT comme contrôle architectural « convolutions tôt, attention tard », même sans score SemanticKITTI publié ;
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
- support maximal du $K$-polyèdre source ;
- support source normalisé, puis support normalisé + taille/position ;
- support du carrier PL, qui doit coïncider numériquement avec le support source, et support de multicoverture séparé, auquel cette identité n'est pas attribuée ;
- complexe HGP marqué complet relativement au contrat : sommets, facettes, cofaces de connexion, incidences, niveaux, multiplicités, coupe $a_v$ et carrier déclaré ;
- encodeur d'incidences du complexe seul, puis support source + même encodeur complet ;
- accès à $\Gamma_K^{\mathrm{elem}}$ avec les mêmes tokens de facettes précalculés, mais sans identifiants/coordonnées bruts, requêtes témoins ni messages point–facette ;
- sac invariant des mêmes tokens de cellules précalculés, sans appartenances, identifiants bruts, arêtes ni messages ;
- null test `incidence-shuffled` à comptes et degrés par rang appariés autant que possible ; s'il ne respecte plus les axiomes du complexe, il passe par un loader diagnostique distinct et ne peut pas être présenté comme une sortie HGP valide ;
- rayon extérieur depuis un centre déclaré, avec masques `center_in_realization` et `center_in_kernel` ;
- intersections multi-segments ou occupations coniques, bande passante explicitement ablatée ;
- distributions de centres, formes et niveaux des facettes/cofaces du payload marqué ;
- ECT/WECT à directions et seuils finis comme baseline topologique antérieure ;
- sketch de Fourier ou embedding de mesure par noyau caractéristique, plus distance à une mesure pour le contrôle robuste ;
- support + densité/persistance HGP ;
- CDF/histogrammes de projections à bins fixes + max ;
- pile de quantiles + max ;
- moments/covariance/histogrammes radiaux ;
- mini-PointNet/Deep Sets à dimension, bits, paramètres, FLOPs et latence comparables.

Les cinq canaux spécifiés dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md) figurent explicitement dans cette liste, et chacun est évalué **deux fois à budget apparié**, une fois à $D$ nombres par nœud et une fois à $D\times B$ nombres par nœud, afin qu'aucun avantage ne soit crédité à la géométrie alors qu'il vient de la largeur du canal :

- **support dur**, $h_v(u_j)=\max_{x\in C_v}\left\langle u_j,x-c_0\right\rangle$, agrégé par $\max$ dans le repère capteur puis recentré et normalisé à la lecture. C'est la référence de la famille : le théorème de caractérisation en fait le seul canal directionnel simultanément fusionnable de façon exacte, recentrable en forme close et continu, au prix d'être aveugle à tout sauf un hyperplan d'appui de l'enveloppe convexe ;
- **support adouci par log-sum-exp** à température finie, $h_v^{\beta}(u_j)=\beta^{-1}\log\sum_{x\in C_v}e^{\beta\left\langle u_j,x-c_0\right\rangle}$, avec $\beta$ balayé et le support dur comme membre $\beta=+\infty$. Il reste exactement fusionnable, les sommes d'exponentielles étant additives sur des enfants disjoints, et il distribue le gradient sur plus d'un point par direction : l'écart au support dur mesure donc un effet d'optimisation, pas un supplément d'information, et doit être présenté ainsi ;
- **CDF projetée** $F_v(u_j,t_b)$ à bins fixes, accumulée en comptes non normalisés dans le repère global donc additive et exactement fusionnable, normalisée seulement à la lecture. C'est le seul canal de la famille qui ne soit pas un extremum, donc le seul qui voie la masse intérieure — un $\max$-pooling ne peut pas approcher une moyenne. Les directions antipodales y étant redondantes, $\mathbb{RP}^{2}$ suffit et le budget effectif est de deux fois moins de directions qu'au support à information égale ; cette économie est appliquée et déclarée. La reconstruction de la mesure par Cramér–Wold ne vaut qu'à résolution infinie et n'est pas revendiquée à grille finie ;
- **canaux radiaux centrés capteur** : $\rho_{\mathrm{in}}$, $\rho_{\mathrm{out}}$, épaisseur $\rho_{\mathrm{out}}-\rho_{\mathrm{in}}$, masque de bin et comptage par bin, tous calculés depuis l'origine LiDAR et fusionnés en une seule passe ascendante par $\min$, $\max$, disjonction et somme. Le masque et le comptage sont obligatoires, faute de quoi un bin vide et un bin à un seul point sont indiscernables. Deux clauses accompagnent ce canal : la perte d'invariance par translation est déclarée comme un choix, et l'antériorité des images de portée est citée, la nouveauté éventuelle portant sur le couplage à la hiérarchie et non sur la représentation. La variante centrée sur le barycentre du nœud reste dans la liste uniquement comme contrôle négatif, $\rho_{\mathrm{in}}$ y étant vide ou instable ;
- **écart au carrier témoin** $\Delta_v(u)=h_{V_v}(u)-h_{W_v(a_v)}(u)\geq0$, seule grandeur de cette famille que les points seuls ne donnent pas, le support du carrier PL des facettes coïncidant exactement avec celui des sommets. Elle se calcule sans énumérer les facettes, par dichotomie sur la cote du plan avec test d'appartenance par requête $K$-NN, soit $\mathcal{O}\left(D\log(1/\varepsilon)\right)$ requêtes par nœud, et porte obligatoirement `authority=witness_approx` avec $\varepsilon_W$ sérialisé, jamais `witness_exact`. Comme $W_v(a)$ dépend du niveau, elle ne se compose pas par $\max$ le long de l'arbre — on n'a que $h_{W_p}(u)\geq\max_{v}h_{W_v}(u)$ sur les enfants — donc son surcoût de recalcul par nœud est mesuré et ablaté séparément des quatre canaux à passe unique. L'ablation doit trancher si $\Delta_v$ est un descripteur utile ou un thermomètre de portée.

Le contrôle `support seul` / `radial seul` / `CDF seule` / `toutes` est exécuté à $D$ puis à $D\times B$ appariés. Avant toute variante de CDF calculée sur le carrier plutôt que sur les points, le piège dimensionnel est vérifié : dans $\mathbb{R}^{3}$, $\mathrm{conv}(F)$ avec $\left|F\right|=K$ est de dimension $K-1$, donc de mesure de Lebesgue nulle pour $K=2,3$ ; une CDF de volume y est identiquement dégénérée et doit être pondérée par la mesure de Hausdorff de la bonne dimension, longueur pour $K=2$ et aire pour $K=3$, la projection d'un segment donnant une densité uniforme et celle d'un triangle une densité affine par morceaux.

Chaque configuration fixe trois axes indépendants : `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only`. `witness_approx` exige une définition et une mesure de $\varepsilon_W$ ; `h0_only` est un contrôle refusé par la branche géométrique complète. Le support source est reconstructible depuis les sommets d'un payload `source_points`, `facet_pl` ou `coface_pl` qui les conserve ; cela n'établit aucune identité ni redondance avec le support de `witness_union`. Aucun run ne change l'un de ces axes en conservant le même nom de configuration.

La coupe est également contractuelle. `cut_policy` appartient à `pre_parent`, `post_birth` ou `explicit`. Pour une arête hiérarchique enfant–parent, la référence emploie `cut_policy=pre_parent`, `cut_level` égal au niveau de fusion du parent et `cut_side=strict`, donc seulement les cellules de niveau strictement inférieur ; la racine emploie une coupe terminale fermée explicitement enregistrée. Les événements de même niveau sont traités par lots atomiques : un cas où la coupe stricte ne définit pas l'objet demandé est sérialisé comme lot d'événement ou marqué `invalid`, jamais remplacé silencieusement par une coupe postérieure. Le champ `deltas` conserve les ajouts de cellules et de marques entre coupes. Les comparaisons support/rayon/ECT utilisent les mêmes conventions de centre, échelle et directions lorsque cela a un sens. Elles rapportent collisions, fraction de nœuds étoilés, rayons vides, nombre de composantes radiales par rayon, sensibilité au thinning et temps de prétraitement. Un canal plus large n'est pas crédité à la géométrie si son avantage disparaît à capacité appariée.

## Diagnostics préalables sans entraînement

Trois mesures précèdent la matrice d'expériences. Elles ne demandent aucun entraînement, seulement la construction de la hiérarchie et les labels, et elles testent l'effet arbre sans le confondre avec le descripteur ni avec l'opérateur. Leur ordre, leur justification et les chiffres de veille qui les calibrent sont dans [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md) ; on ne reprend ici que le protocole et la règle de décision. Aucune étape de la matrice n'est lancée avant que M1 et M2 aient un résultat consigné, y compris négatif.

### M1 — Oracle d'antichaîne

Construire la forêt HGP sur la séquence 08, choisir une antichaîne — un ensemble de nœuds deux à deux non emboîtés couvrant tous les points valides —, étiqueter chaque nœud par sa classe majoritaire et rapporter le mIoU obtenu. C'est le plafond atteignable si le modèle prédisait parfaitement une étiquette par région.

**Correction de formulation, à appliquer avant tout calcul.** Le mIoU n'est pas additif sur les régions : c'est une moyenne de rapports globaux par classe, et la contribution d'une région dépend de toutes les autres par les dénominateurs. « La meilleure antichaîne au sens du mIoU » n'est donc pas un problème d'optimisation bien posé, et ce vocabulaire est banni du dossier. Le protocole retenu comporte deux volets, rapportés ensemble.

**(i) Antichaîne à budget de régions.** À budget $R$ fixé, sélectionner l'antichaîne qui minimise l'**impureté totale** $\sum_{v}n_v H\left(\pi_v\right)$, où $n_v$ est le nombre de points valides du nœud $v$, $\pi_v$ sa distribution GT et $H$ l'entropie. Ce critère est additif sur les nœuds, donc le problème admet une **programmation dynamique exacte** sur l'arbre, en une seule passe ascendante, avec pour état le nombre de régions consommées dans le sous-arbre : pour chaque nœud et chaque budget $r$, on compare le coût de la coupe au nœud, qui consomme une région, au meilleur partage de $r$ entre les enfants. Le mIoU de l'antichaîne ainsi obtenue est ensuite rapporté **comme descripteur de cette antichaîne, jamais comme un optimum de mIoU**. L'erreur de classe majoritaire $\sum_{v}n_v\left(1-\max_c\pi_v(c)\right)$, également additive, est rapportée en même temps pour vérifier que la conclusion ne dépend pas du choix de $H$.

**(ii) Coupes à niveau fixé.** Rapporter en complément le mIoU-oracle de coupes à niveau constant, qui est la convention de la littérature superpoint et la seule qui permette la comparaison directe avec ses chiffres publiés. Sans ce volet, aucun nombre de M1 n'est comparable à un oracle de la littérature.

Les deux volets sont tracés en fonction du nombre de régions, à taux de compression apparié, contre les mêmes contrôles que la rubrique Hiérarchies des baselines : HDBSCAN/RSL, octree ou grille de voxels, partition superpoint, arbre aléatoire de mêmes tailles de régions, plus la permutation d'associations de feuilles comme null test. La stratification obligatoire ci-dessus s'applique intégralement, en particulier par portée, par dimension intrinsèque et par cardinal de nœud.

**Règle de décision.** Si HGP ne domine aucun contrôle à compression égale, ou si la domination disparaît après stratification par portée, le programme sémantique est réfuté et le résultat négatif est publié plutôt qu'absorbé.

**Asymétrie d'interprétation, à énoncer avec le résultat.** Cet oracle est déjà publié par la lignée superpoint, et son verdict est défavorable à l'hypothèse « meilleure partition, donc meilleure segmentation ». SPG rapporte sur S3DIS 6-fold un oracle de partition à $88{,}2$ mIoU pour un modèle à $62{,}1$ ; SPT constate que sa performance est à plus de vingt points sous l'oracle et en conclut que la partition ne limite pas fortement son résultat ; SuperCluster fait le même constat avec un oracle à $93{,}4$ PQ. Une vingtaine de points d'oracle sont donc déjà non convertis par ces méthodes, et améliorer le plafond d'une partition qui n'est pas saturée ne peut pas produire de gain. **M1 est une porte de réfutation, pas une porte de promotion** : le perdre tue le programme, le gagner ne prouve presque rien. Le résultat est présenté avec cette asymétrie explicite, et un gain d'oracle n'est jamais rapporté comme une preuve de la valeur de HGP.

### M2 — Stabilité sous transport et rééchantillonnage

Prendre des objets étiquetés observés à portée courte, les transporter à plusieurs portées et les rééchantillonner selon un modèle capteur déclaré et versionné — amincissement angulaire, disparition de retours, changement d'occultation. La règle de rééchantillonnage est celle des stress tests de portée et densité, et l'homothétie seule ne la remplace pas. Mesurer la dérive des niveaux de naissance et de mort, la stabilité de l'ancêtre commun des points de l'objet et la persistance relative.

Dans la même passe, tester la correction : remplacer le niveau brut $a$ par un niveau **normalisé par la densité d'échantillonnage attendue** du capteur à cette portée et à cet angle d'incidence, et mesurer la dérive résiduelle.

**Règle de décision.** Si la dérive intra-objet reste du même ordre que la séparation interclasse, la filtration encode la portée plutôt que la sémantique et R1 n'est pas levé : aucune configuration ne passe en aval sans correction déclarée et mesurée. Si la correction ramène la dérive nettement sous la séparation interclasse, elle est revendiquée comme une filtration consciente du capteur, et non comme un détail d'implémentation.

**Signal contraire à consigner d'avance.** L'ablation d'ALPINE montre qu'un seuil proportionnel à la portée, à la manière de LESS, donne $75{,}9$ PQ contre $76{,}3$ pour un seuil constant par classe, malgré l'optimisation de son coefficient à l'échelle du jeu de données : une correction de portée naïve **dégrade** leur clustering. Leur seuil est un rayon de liaison et non un niveau de densité $K$-NN, donc ce résultat ne se transporte pas mécaniquement ; il interdit néanmoins de présenter la correction comme acquise, et il est cité avec la mesure.

### M3 — Substitution de clusterer dans le pipeline ALPINE

Reprendre le pipeline ALPINE **tel quel** — mêmes logits sémantiques gelés publics, mêmes seuils $t_c$ par classe, même découpage récursif des boîtes — et remplacer **uniquement** les composantes connexes par les $K$-polyèdres à $K=2,3$. Une seule variable change. Rapporter PQ, PQthing, la ventilation par portée et le temps par trame, sur SemanticKITTI val.

Ce diagnostic ne rouvre pas la phase instance comme contribution : elle reste fermée pour la publication et aucun résultat de M3 n'entre dans le tableau principal de la phase sémantique. Il l'emploie comme **mesure**, parce que le clusterer d'ALPINE est littéralement du single-linkage — projection BEV par classe thing, graphe $k$-NN à $k=32$, suppression des arêtes plus longues que $t_c$, composantes connexes — c'est-à-dire HGP à $K=1$ avec un rayon dépendant de la classe, et parce que son mode d'échec déclaré est l'effet de chaînage, qui est la motivation centrale de la thèse.

**Barème fixé d'avance par les chiffres publiés.** À sémantique MinkUNet identique et découpage de boîtes désactivé pour tous, SemanticKITTI val : ALPINE $65{,}5$ PQ, D&M $61{,}8$, DBSCAN $56{,}7$, HDBSCAN $55{,}1$, soit $10{,}4$ PQ imputables au seul clusterer, HDBSCAN étant dernier. Le plafond est bas en regard : l'oracle d'instance à sémantique figée ne rapporte que $+4{,}3$ PQ sur nuScenes avec PTv3 et $+3{,}8$ avec WaffleIron-768, et sous sémantique parfaite le clusterer d'ALPINE ne perd que $1{,}0$ PQ sur SemanticKITTI, ses auteurs concluant que l'extraction d'instances est largement saturée. Dans ce cadre, $+1$ à $+2$ PQ serait un résultat fort et $+4$ le maximum atteignable.

**Le temps est rapporté au même titre que le PQ.** ALPINE tourne à $14{,}4$ Hz sur un seul cœur CPU, tandis que le pipeline HGP historique est de l'ordre de la seconde par trame : il y a plus d'un ordre de grandeur à combler, et un gain de PQ obtenu à ce prix est rapporté comme tel, jamais isolé de son coût.

**Règle de décision.** Si HGP à $K=2,3$ ne dépasse pas le single-linkage d'ALPINE sur la tâche que la thèse a conçue pour lui, c'est-à-dire la résistance au chaînage, l'hypothèse d'un apport en segmentation sémantique perd sa principale preuve d'existence à bas coût, et l'engagement de calcul en aval est rediscuté avant d'être poursuivi.

## Matrice d'expériences

La matrice complète est factorielle et trop coûteuse. Elle est déroulée séquentiellement :

| Étape | Variable changée | Variables gelées | Décision |
|---|---|---|---|
| E0 | granularité/coupe | labels GT, aucun réseau | composition des nœuds et diagnostic d'une sortie dure token-constante |
| E1 | hiérarchie | backbone + agrégateur simple | valeur de HGP |
| E2 | descripteur | HGP + backbone + budget | information du descripteur |
| E3 | opérateur | HGP + descripteur + budget | HSA contre QC-HSA et agrégateurs |
| E4 | placement/nombre de blocs | meilleure configuration E1–E3 | architecture finale |
| E5 | backbone | module HGP gelé | généralité |
| E6 | perturbation/capteur | modèle gelé | robustesse |
| E7 | coût/scaling | modèle gelé | viabilité système |

Chaque étape conserve l'identifiant de la configuration parente et ne change qu'un facteur principal. Les interactions jugées importantes sont testées ensuite, jamais absorbées dans une unique expérience finale.

### Décomposition causale du canal polyédral

E2 suit l'ordre ci-dessous avec même backbone, arbre, opérateur, largeur cachée, pertes, seeds et budget de paramètres. Les coûts bruts de structure ne peuvent pas être rendus artificiellement égaux : ils sont rapportés et une seconde comparaison Pareto apparie latence ou VRAM.

| Variante | Information disponible | Question isolée |
|---|---|---|
| P0 | features de points poolées | contrôle sans géométrie de nœud |
| P1 | support source normalisé + side channels | valeur du raccourci convexe |
| P2 | `marked_incidence`, carrier et autorité déclarés, sans support explicite | valeur de l'accès au carrier et aux incidences |
| P3 | P2 + support source calculé depuis les mêmes sommets | gain d'optimisation du shortcut |
| P4 | tokens de facettes précalculés + $\Gamma_K^{\mathrm{elem}}$ | effet de restreindre l'accès au graphe élémentaire |
| P5 | sac invariant des mêmes tokens de cellules précalculés | effet de retirer tous les messages et arêtes |
| P6 | incidences réassignées sous contrôle diagnostique | mutant invalide, jamais sortie HGP |
| P7 | support + rayon extérieur, sans complexe | coût de la compression radiale |

P2 et P3 sont exécutés avec les mêmes `payload_kind`, `carrier_kind`, `authority` et coupe ; ce choix n'est ni une seed ni un hyperparamètre caché. Pour les carriers source/PL dont P2 conserve les sommets, P3 ne reçoit aucune donnée absente de P2 : son gain mesure un biais d'optimisation. Cette conclusion ne vaut ni pour le support propre de `witness_union`, ni pour une approximation qui ne permet pas de le reconstruire.

P4 reçoit les mêmes tokens de facettes précalculés et l'adjacence de $\Gamma_K^{\mathrm{elem}}$, mais masque identifiants et coordonnées bruts, requêtes témoins et messages point–facette. P5 reçoit les mêmes tokens précalculés par cellule, masque appartenances et identifiants bruts, et interdit toute arête ou message ; son readout est invariant à l'ordre. Ces variantes restreignent l'accès calculatoire : elles ne prétendent pas effacer une information qu'un token précalculé pourrait déjà encoder. P6 passe exclusivement par le loader diagnostique et porte toujours le statut `invalid`.

Pour E3, de petits scans ou sous-échantillons où l'attention plate est calculable rapportent aussi le reverse-KL total et par feuille de HSA et `QC-HSA`, la fraction de feuilles dont la classe plate à marge fixée est préservée, et l'écart de sortie. Ce test utilise une même cible plate $P$ gelée, les mêmes features, scores et masque diagonal pour flat/HSA/QC-HSA ; comparer les KL de modèles entraînés séparément ne testerait pas la proposition. Ces diagnostics vérifient le résultat technique ; ils ne remplacent pas le mIoU contre la vérité terrain.

La fidélité HSA est testée sur arbres équilibré, étoile, chaîne/peigne, singleton et degré un. Les mutants couvrent indices positionnels, signe de normalisation, facteur de cardinalité, masque diagonal et ordre des feuilles. Deux scans concaténés restent explicitement block-diagonaux ; l'ajout, la suppression ou la perturbation du second scan ne doit pas modifier la sortie du premier.

Le canal polyédral possède en plus les fixtures obligatoires suivantes :

- round-trip exact des identifiants de points, facettes, cofaces, incidences, niveaux, multiplicités, $a_v$, `payload_kind`, `carrier_kind`, `authority`, `cut_policy`, `cut_level`, `cut_side` et `deltas` ;
- même support source/PL mais incidences différentes : l'oracle de sérialisation et le hash canonique doivent distinguer la paire, tandis que P1 doit collisionner ; le taux de collisions du learned encoder est mesuré, sans lui attribuer cette garantie avant preuve de T2 ;
- identité `support(source) == support(PL)` direction par direction, sans assertion correspondante pour $W_v(a_v)$ ;
- invariance de la sortie à une réindexation canonique des sommets et cellules ;
- mutation d'une incidence, d'un niveau ou d'une coface détectée par le hash canonique ; la sensibilité et les collisions de l'encodeur sont rapportées sur une fixture non symétrique ;
- recouvrement $K\geq2$ : tant qu'une application déterministe $w_{iv}$ avec domaine explicite, $\sum_v w_{iv}=1$ et tests de conservation n'est pas spécifiée, le chemin HSA reste limité à $K=1$ ou à une laminarisation auditée ; les fixtures DAG sont conditionnelles à la fermeture de cette obligation ;
- rejet par le loader de production d'une incidence shuffled invalide, même si le loader diagnostique l'accepte comme null test ;
- compteurs attestant que le modèle consomme l'export sparse contractuel sans reconstruire un Čech ou Delaunay global.

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

Une homothétie des coordonnées relatives est seulement un test algébrique du canal normalisé. Elle ne remplace pas le transport avec rééchantillonnage, car voir un objet plus loin modifie densité angulaire, occultation et nombre de retours plutôt que sa seule échelle métrique.

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
