# État de l'art et espace de nouveauté

Veille arrêtée au **13 août 2026**. Les scores ci-dessous proviennent de sources primaires, mais les régimes d'entrée et d'entraînement diffèrent. Aucun tableau unique ne doit être lu comme une course de nombres sans ces colonnes.

## État des sources officielles SemanticKITTI

Trois sources coexistent :

1. l'ancien leaderboard CodaLab, fermé le 31 janvier 2026, conserve l'historique du test ;
2. le nouveau CodaBench n'a pas migré automatiquement cet historique ; ses méthodes et fiches sont généralement non documentées ;
3. le JSON du site SemanticKITTI liste des valeurs de papiers, mais il est incomplet et daté du 2 juin 2025.

Conséquences :

- l'ancien maximum serveur visible est **76,5 mIoU**, partagé par `SimpleSeg` et `TASeg` ; `SimpleSeg` n'a pas de méthode publiquement identifiable ;
- TASeg est le meilleur résultat publié identifiable à **76,5**, mais son modèle utilise historique LiDAR et images ;
- RAPiD-Seg à **76,1** est le comparateur publié le plus fort vérifié en LiDAR mono-trame, mais son absence de TTA/ensemble n'est pas explicitement certifiée et son inférence comporte deux passes apprises ;
- le JSON officiel des papiers place LSK3DNet en tête à **75,6**, mais omet notamment TASeg et RAPiD-Seg ;
- le nouveau CodaBench affiche le compte pseudonyme `kadir_yilmaz` à 75,2 au jour de la veille, sans méthode ni fiche renseignée ; il ne remplace pas l'historique.

Sources : [ancien classement officiel](https://codalab.lisn.upsaclay.fr/competitions/6280/results/9324), [nouvelle compétition](https://www.codabench.org/competitions/12448/), [API du leaderboard actuel](https://www.codabench.org/api/phases/20274/get_leaderboard/?page=1&page_size=100), [table officielle des papiers](https://semantic-kitti.org/data/semantic_single.json).

## Méthodes sémantiques à battre ou expliquer

Les tracks A–D sont définis dans [EXPERIMENTAL_PROTOCOL.md](EXPERIMENTAL_PROTOCOL.md#régimes-de-comparaison). `NR` signifie que le point n'est pas explicitement rapporté dans la source primaire auditée ; il ne signifie pas « non utilisé ».

| Méthode | Val / test mIoU | Entrée à l'inférence | Ressources d'entraînement | TTA | Inférences/étages | Statut de comparaison |
|---|---:|---|---|---|---|---|
| TASeg, CVPR 2024 | 72,7 / **76,5** | LiDAR temporel + images historiques | régime temporel/multimodal | NR | NR | track C, non strict |
| RAPiD-Seg, ECCV 2024 | 73,0 / **76,1** | LiDAR mono-trame | pipeline class-aware appris | NR | 2 inférences séquentielles | plus proche ; recette stricte à auditer |
| LSK3DNet, CVPR 2024 | 70,2 / **75,6** | LiDAR mono-trame | instance CutMix, davantage d'époques pour la recette test | oui | NR | track D, pas le claim strict |
| PTv3 + PPT, CVPR 2024 | 72,3 / **75,5** | LiDAR mono-trame | préentraînement multi-datasets | NR | NR | track B |
| SP2T, ICCV 2025 | 71,7 / **75,4** | LiDAR mono-trame | SemanticKITTI | oui | NR | track D ; concurrent conceptuel proche |
| M3Net, CVPR 2024 | 72,0 / **75,1** | LiDAR mono-trame | entraînement multi-datasets | NR | NR | track B |
| UniSeg, ICCV 2023 | 71,3 / **75,2** | RGB + point/voxel/range | multimodal | NR | NR | track C |
| SphereFormer, CVPR 2023 | 67,8 / **74,8** | LiDAR mono-trame | SemanticKITTI | NR | NR | contrôle géométrique proche |
| DITR, 3DV 2026 | 69,0 / **74,4** | LiDAR + image DINOv2 | préentraînement externe DINOv2 | NR | NR | tracks B+C ; image requise |
| PTv3 seul, CVPR 2024 | 70,8 / **74,2** | LiDAR mono-trame | SemanticKITTI | NR | NR | cible de portage, pas baseline WP0 prête |
| ProtoSEG, NeurIPS 2023 | **74,2** / NR | LiDAR | segmentation unifiée | NR | NR | contrôle semantic/panoptic, validation seulement |
| VaViT, arXiv 2026 | 68,0 / NR | LiDAR mono-trame | SemanticKITTI | non | NR | résultat validation strict, pas de test caché |

Les scores val et test ne sont jamais interchangeables. Par exemple, RWAFormer appelle parfois la séquence 08 « test » et rapporte 75,3 ; il s'agit du split public de validation, pas du serveur 11–21. Sa taxonomie/reporting doit en outre être reproduite avec le YAML officiel avant comparaison.

### Leçons pour HGP-HSA

- **RAPiD-Seg** montre qu'un descripteur géométrique doit intégrer la variation de densité avec la portée et la rémission. Il constitue un contrôle plus direct de la fonction support que les seuls Transformers, mais son coût complet comprend deux passes apprises.
- **SphereFormer** encode déjà la géométrie sphérique du capteur. Un gain HGP limité aux longues distances doit être comparé à ce biais, pas à un modèle cartésien naïf ; la source primaire auditée ne rapporte pas le statut TTA, qui reste donc `NR` plutôt que « non ».
- **LSK3DNet** rappelle qu'une attention sophistiquée doit battre un CNN sparse adaptatif en précision ou sur un axe Pareto clair ; son score test 75,6 ne doit toutefois pas être rangé dans le track sans TTA.
- **SP2T** est un concurrent direct : son double flux et ses proxies locaux réduisent l'attention point–point tout en conservant du contexte sparse. Son supplément applique rotation, scaling, flip et jitter au test ; son 75,4 reste donc hors track A. Le gain HGP doit être isolé d'un simple effet de tokens proxy.
- **PTv3** simplifie l'attention par sérialisation et grands patches locaux. HGP doit apporter un contexte complémentaire, pas reproduire une partition spatiale plus coûteuse. Le dépôt public ne fournit pas une recette SemanticKITTI complète config+poids+score ; WP0 doit donc commencer par une baseline réellement épinglable.
- **VaViT** fournit en 2026 une baseline ViT globale publique avec tokenisation BEV par piliers, stricte sans TTA mais limitée à 68,0 sur validation ; elle est utile pour la reproductibilité, pas comme seuil SOTA.
- **FLARES** rappelle que la portée est déjà un axe architectural et système explicite ; certaines recettes rapportées utilisent TTA et/ou données CARLA et doivent rester dans des tracks séparés.
- **TASeg, UniSeg, DITR et M3Net** prouvent la valeur des ressources supplémentaires. Ils restent dans des colonnes distinctes pour ne pas diluer le claim LiDAR mono-trame.

## Concurrents conceptuels : hiérarchie et attention

### HSA, NeurIPS 2025

[Hierarchical Self-Attention](https://proceedings.neurips.cc/paper_files/paper/2025/hash/0480adaf62a918405a5e3b1031e0c056-Abstract-Conference.html) formalise un signal imbriqué par un arbre. Les vecteurs de contenu sont aux feuilles ; les relations positionnelles sont attachées aux familles. Les coefficients entre deux sous-arbres frères sont partagés par blocs.

Pour **un arbre donné**, avec Q/K LayerNormés après projection, l'énergie quadratique, la température et le rescaling du papier, HSA minimise $\sum_i D_{\mathrm{KL}}\left(\theta^{\mathrm{HSA}}_i\,\Vert\,\theta^{\mathrm{flat}}_i\right)$ sur la famille de matrices stochastiques satisfaisant les contraintes de blocs. La cible plate utilise la même énergie et les mêmes positions ; il ne s'agit pas d'une attention Softmax arbitraire. Ce résultat porte sur les poids d'attention et ne garantit ni la qualité de la hiérarchie, ni les effets des projections V/gates/MLP, ni un gain de segmentation.

Le calcul est bottom-up puis top-down, en $\mathcal{O}(M b^{2})$ pour $M$ familles et un branchement maximal $b$. Le papier reconnaît que les parcours d'arbre sont mal adaptés au GPU et propose des opérations sparse par profondeur ainsi qu'une concaténation en largeur. Il n'évalue aucun nuage 3D ni aucune segmentation dense ; ses expériences portent surtout sur la classification et le remplacement de couches de RoBERTa. Certains remplacements complets dégradent fortement les résultats, ce qui motive une architecture hybride et des blocs tardifs.

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

### EZ-SP, ICRA 2026

[EZ-SP](https://arxiv.org/abs/2512.00385) remplace la partition CPU de SPT par un clustering appris sur GPU, annoncé à moins de 60k paramètres et beaucoup plus rapide que les partitions antérieures. Même sans score SemanticKITTI publié, il est le concurrent système direct de MorseHGP3D : HGP doit démontrer soit une meilleure structure, soit un coût Pareto comparable.

### SPCNet, ICCV 2025

[SPCNet](https://openaccess.thecvf.com/content/ICCV2025/html/Lu_Serialization_based_Point_Cloud_Oversegmentation_ICCV_2025_paper.html) apprend des superpoints par sérialisation de Hilbert, réaffectation par similarité et cross-attention, puis ajoute deux niveaux hiérarchiques. Il évalue explicitement SemanticKITTI et rapporte 71,9 mIoU test pour sa variante superpoint. Son score n'est pas la cible SOTA, mais son oversegmentation apprise et son gain hiérarchique en font un contrôle conceptuel direct pour la qualité des partitions.

### Autres précédents

- [OctFormer](https://arxiv.org/abs/2305.03045) exploite une structure octree pour l'attention efficace.
- [Cluster3Dseg](https://openaccess.thecvf.com/content/ICCV2023/html/Feng_Clustering_based_Point_Cloud_Representation_Learning_for_3D_Analysis_ICCV_2023_paper.html) utilise le clustering dans l'espace d'embedding et rapporte 70,4 sur SemanticKITTI ; ce n'est pas une hiérarchie géométrique par scan, mais le terrain lexical « clustering pour la représentation » est occupé.
- [SuperCluster](https://arxiv.org/abs/2401.06704) formule la segmentation panoptique comme clustering d'un graphe de superpoints.
- [SSTNet](https://openaccess.thecvf.com/content/ICCV2021/html/Liang_Instance_Segmentation_in_3D_Scenes_Using_Semantic_Superpoint_Tree_Networks_ICCV_2021_paper.html) apprend et coupe un arbre de superpoints pour les instances intérieures.

## Ce que HGP apporte réellement

Le papier [Generalization of single-linkage with higher-order interactions](https://link.springer.com/article/10.1007/s41109-025-00756-1) montre, sous ses hypothèses de position générale, que les $K$-polyèdres correspondent aux clusters de haute densité de son estimateur $K$-NN **sur l'échantillon fini**. C'est un fondement plus précis qu'une oversegmentation heuristique, mais ni une preuve de consistance vers l'arbre de Hartigan populationnel ni une garantie d'alignement sémantique.

Son expérience SemanticKITTI ne mesure toutefois pas la segmentation sémantique : elle utilise les **masques sémantiques de vérité terrain** de la séquence 08, puis regroupe les points de chaque classe thing. Les PQthing/RQthing/SQthing rapportés sont environ :

| Ordre | PQthing | RQthing | SQthing |
|---:|---:|---:|---:|
| $K=1$ | 0,876 | 0,921 | 0,949 |
| $K=2$ | **0,888** | **0,934** | 0,950 |
| $K=3$ | 0,829 | 0,917 | 0,903 |

Cela soutient l'exploration de $K=2$ et réfute l'idée « un ordre plus grand est toujours meilleur ». Cela ne démontre pas qu'HGP aide à prédire les classes. Le papier indique aussi que des clusters d'ordre supérieur peuvent partager des points et qu'une attribution dure perd cette structure.

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

Le positionnement le plus solide est :

> Un complexe HGP marqué et sa hiérarchie fournissent-ils un prior d'interactions d'ordre supérieur stable, efficace et vérifiable pour la propagation de contexte sémantique dans les scènes LiDAR irrégulièrement échantillonnées ?

Pour mériter ICML/NeurIPS, la réponse doit inclure une contribution générale — stabilité, analyse, opérateur ou descripteur fusionnable — et une validation au-delà de SemanticKITTI.
