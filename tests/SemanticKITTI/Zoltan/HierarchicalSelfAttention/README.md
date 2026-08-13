# HGP-HSA pour la segmentation sémantique de SemanticKITTI

Ce dossier définit un programme de recherche, pas un claim de résultat. Son objectif immédiat est de déterminer si une hiérarchie de densité HGP fondée sur les voisinages $K$-NN peut fournir un meilleur biais inductif multi-échelle pour la **segmentation sémantique point par point** d'un scan LiDAR. La segmentation d'instance est reportée à une phase ultérieure, après validation du modèle sémantique.

État du dossier au 13 août 2026 : conception et falsification, aucune expérience apprise rapportée, aucun statut SOTA revendiqué.

## Cadre et hypothèse fournie

Pour cette étude, on suppose disponible un outil qui reconstruit, pour chaque scan, la hiérarchie $K$-NN/HGP avec sa topologie, ses appartenances et ses niveaux de densité. Le coût de cet outil ne bloque donc pas la phase scientifique actuelle, mais il devra être mesuré séparément et de bout en bout avant tout claim d'efficacité. Dans tout le dossier, $K$ désigne l'ordre HGP, $d_{\mathrm{geo}}$ la distance géométrique et $k_{\mathrm{local}}$ un éventuel budget de voisins du backbone ; ces trois choix ne doivent pas être confondus.

Le contrat v3 associé demeure hors registre :

- `phase=exploration_v3_hors_registre`
- `backend=cpu_reference_bounded_oracles_and_g4_diagnostic`
- `profile=quantized_u16_input_only`
- `mode=audit_independant_math_and_architecture`
- `public_status=not_claimed`

Après synchronisation distante, l'audit v3 consulté n'est toujours pas frais : son observation live épingle `HEAD=33df59d451dc1c534a1fd5f1572e938472744fef`, tandis que `origin/main` était déjà à `81d24d05142219aa0c5e9b00d129b72b03f0e85e` avant ce commit documentaire. Ce constat interdit de transformer la disponibilité supposée de la hiérarchie en claim produit ou GPU sur MorseHGP3D.

## Verdict honnête

Chaque cluster doit porter un **vecteur de proportions sur les 19 classes**, et non un label unique. La cible d'un nœud est la distribution empirique de ses points ; les feuilles point sont le cas one-hot. La version minimale « fonction support normalisée seule + proportions de cluster comme seul décodeur » a néanmoins peu de chances d'atteindre l'état de l'art : la fonction support perd concavités, densité et intérieur, tandis que les proportions conservent la masse des classes mais pas leur localisation à l'intérieur d'un cluster mixte.

La réalisation géométrique du $K$-polyèdre ne résout pas cette perte par un second maximum. Si elle est l'union des simplexes de la composante, sa fonction support est exactement celle de l'union de leurs sommets. Le « maximum de la norme dans une direction » est plutôt une **fonction radiale extérieure** : elle reconstruit exactement seulement une forme étoilée autour du centre et remplit sinon les trous radiaux. Un cube plein et sa frontière ont ainsi chacun le même support et la même fonction radiale extérieure que l'autre. Ces faits deviennent des fixtures obligatoires, pas de nouveaux claims.

La piste élargie reste crédible et potentiellement forte :

1. un backbone local point/voxel conserve le détail et produit les features de feuilles ;
2. la hiérarchie HGP fournit une structure exogène multi-échelle ;
3. des sketches de masse ou de topologie et une mesure sur les simplexes — centres, formes, niveaux de naissance et incidences — complètent les canaux support/radial ;
4. les feuilles prédisent leurs distributions et chaque nœud déduit exactement ses proportions par moyenne massique de ses descendants ;
5. un ou deux blocs hiérarchiques tardifs propagent le contexte ; HSA sert de baseline fidèle et un raffinement conditionné par la requête n'est retenu que s'il apporte un certificat fidélité–coût calculable ;
6. un décodeur point-fin et un chemin résiduel localisent les classes et préservent les frontières.

La probabilité d'un SOTA par la seule idée initiale est faible. Une contribution de haut niveau reste plausible si les expériences démontrent causalement que l'arbre HGP apporte plus qu'un octree, des superpoints, RSL/HDBSCAN ou un arbre aléatoire, et que l'opérateur hiérarchique apporte plus qu'un simple pooling sur le même arbre.

## Question scientifique centrale

> À backbone, budget, features et protocole constants, une hiérarchie de niveaux de densité HGP améliore-t-elle la segmentation sémantique LiDAR, notamment pour les classes rares et lointaines, par rapport aux structures hiérarchiques géométriques ou apprises existantes ?

Cette question est séparée en trois effets identifiables :

- **effet arbre** : HGP contre les autres structures, avec le même opérateur ;
- **effet représentation** : support et attributs HGP contre des descripteurs de même dimension ;
- **effet opérateur** : HSA contre pooling, message passing et attention locale, avec le même arbre.

Le [programme du résultat théorique](THEOREM_PROGRAM.md) propose une famille `QC-HSA` : chaque feuille voit les cibles par sous-arbres frères successifs, sans partager sa ligne d'attention avec les autres feuilles de sa branche. Sa projection reverse-KL possède une forme fermée et optimise une famille plus large que HSA sur le même arbre, au prix d'environ $N\log_2N$ interactions sur un arbre binaire équilibré contre un coût structurel linéaire pour HSA. Elle est exacte pour des scores constants sur chaque couple de branches de fusion. Ce résultat reste technique ; il ne peut devenir central qu'avec un certificat HGP non vacu, un audit d'antériorité favorable et un coût réel compétitif.

Un gain global sans cette décomposition ne suffira pas pour une soumission ICML ou NeurIPS.

## Cible expérimentale

La piste principale est strictement comparable : un scan, LiDAR uniquement, entrées `(x, y, z, remission)`, entraînement SemanticKITTI uniquement, sans TTA ni ensemble. La métrique primaire est le mIoU sur la séquence 08, puis sur le test caché 11–21 après verrouillage.

Au moment de la veille, aucun nombre unique ne certifie le SOTA strict mono-scan, mono-modèle, sans TTA ni données externes : le leaderboard mélange les régimes et ses premières lignes sont souvent anonymes. Les repères audités sont :

- RAPiD-Seg rapporte **76,1 mIoU** avec un pipeline appris LiDAR mono-trame en deux inférences séquentielles ; c'est le concurrent publié le plus proche, mais l'absence de TTA ou d'ensemble n'est pas explicitement documentée ;
- LSK3DNet rapporte **75,6 mIoU** avec une architecture sparse point-voxel, instance CutMix, TTA et davantage d'époques dans sa recette de soumission au test ; il relève donc du track TTA ;
- SP2T rapporte **75,4 mIoU** avec une attention sparse à proxies en double flux LiDAR mono-trame, mais son supplément applique rotation, échelle, flip et jitter au test ; il relève donc du track TTA ;
- TASeg atteint **76,5 mIoU**, mais utilise de l'historique LiDAR et image et appartient donc à un régime de ressources distinct ;
- l'ancien leaderboard officiel contient aussi la soumission `SimpleSeg` à 76,5, sans méthode ni papier publiquement attribuable ; le CodaBench courant affiche 75,2 pour un compte pseudonyme sans fiche méthode exploitable.

Ces chiffres sont des instantanés, pas des seuils éternels. La concurrence devra être réauditée avant soumission. Le premier objectif n'est pas de dépasser un nombre isolé, mais d'obtenir un gain HGP reproductible sur validation, puis une position Pareto compétitive en précision, mémoire et latence.

## Décision architecturale de départ

Le prototype de référence utilisera les **points ou micro-voxels comme feuilles** et les descripteurs des nœuds HGP pour construire un embedding par enfant dans le domaine de son parent. HSA fidèle sera d'abord reproduit. `QC-HSA` sera ensuite testé comme opérateur principal candidat, car il retire les égalités entre requêtes susceptibles de propager les erreurs aux frontières. Faire de chaque nœud interne un token appris restera une variante distincte ; aucun théorème de projection ne lui sera attribué sans nouvelle démonstration.

Les feuilles produisent $p_i\in\Delta^{18}$ et un nœud calcule $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$, ou récursivement la moyenne de ses enfants pondérée par leurs cardinalités. Les proportions GT sur les seuls labels valides restent exclusivement des cibles d'entraînement et ne participent jamais au forward de validation/test ni à la construction HGP. La prédiction officielle demeure une distribution par point ; les proportions internes apportent un état multiscale cohérent, pas un label diffusé uniformément.

La fonction support est conservée comme canal de forme convexe et non comme identité complète du nœud. Le rayon extérieur, les intersections multi-segments et ECT/WECT sont des variantes contrôlées par [l'audit géométrique](GEOMETRIC_DESCRIPTOR_AUDIT.md), non des ajouts supposés gagnants. Le descripteur candidat combine d'abord :

- support maximal et CDF/histogrammes de projections directionnelles, avec quantiles robustes en complément ;
- centre, `log(rayon d'échelle)`, cardinalité et géométrie relative parent–enfant ;
- niveau de densité, naissance, mort et persistance HGP ;
- anisotropie/covariance, portée, hauteur et statistiques de rémission ;
- attributs simpliciaux ou topologiques seulement s'ils battent CDF, moments et Deep Sets à budget égal.

## Portes avant entraînement lourd

Le projet ne passe au modèle complet que si les diagnostics suivants sont satisfaits :

- l'arbre HGP présente un alignement sémantique supérieur aux arbres de contrôle à compression égale ;
- une tokenisation éventuelle conserve assez d'information pour prédire les proportions **et** relocaliser les classes au niveau point ; sinon les points restent feuilles ;
- les collisions du support, du rayon extérieur et des transformées finies sont mesurées, avec la fraction de nœuds étoilés et de rayons vides ;
- l'effet de portée du capteur ne domine pas les niveaux de densité ;
- la profondeur, les degrés et la mémoire de la hiérarchie sont compatibles avec des batches GPU utiles.

Les critères chiffrés et les règles d'arrêt sont dans [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md).

## Contenu du dossier

- [HGP_HSA_SemanticKITTI.md](HGP_HSA_SemanticKITTI.md) : hypothèse scientifique révisée et variantes.
- [REVIEWER_VERDICT.md](REVIEWER_VERDICT.md) : décision honnête, questions de reviewer et barre de preuve par venue.
- [GEOMETRIC_DESCRIPTOR_AUDIT.md](GEOMETRIC_DESCRIPTOR_AUDIT.md) : support, rayon, topologie, contre-exemples et théorèmes candidats.
- [THEOREM_PROGRAM.md](THEOREM_PROGRAM.md) : proposition technique, esquisse de preuve et pont conditionnel vers les niveaux de fusion.
- [ARCHITECTURE.md](ARCHITECTURE.md) : contrat d'entrée, descripteurs et modèle hybride proposé.
- [RESEARCH_PLAN.md](RESEARCH_PLAN.md) : lots de travail, ordre des décisions et livrables.
- [EXPERIMENTAL_PROTOCOL.md](EXPERIMENTAL_PROTOCOL.md) : splits, métriques, baselines et matrice d'ablation.
- [STATE_OF_THE_ART.md](STATE_OF_THE_ART.md) : concurrence et espace de nouveauté.
- [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md) : réfutations, kill gates et pivots.
- [PAPER_STRATEGY.md](PAPER_STRATEGY.md) : positionnement d'une soumission de haut niveau.
- [REFERENCES.md](REFERENCES.md) : sources primaires et statut des chiffres.
- [papier HSA local](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) : copie vérifiée du papier NeurIPS 2025.

## Hors périmètre immédiat

La segmentation panoptique, la segmentation d'instance open-world, le multi-scan et la fusion RGB ne font pas partie de la première phase. Le modèle conservera seulement une interface future : logits et features par point, proportions sémantiques prédites par nœud, mapping point–feuille–ancêtres, topologie HGP et descripteurs de nœuds. Quand la sémantique sera validée, ALPINE deviendra une baseline aval obligatoire avec **exactement les mêmes logits sémantiques gelés**.

## Règle de claim

Les mots « exact », « temps réel », « GPU-friendly » et « état de l'art » ne doivent apparaître comme résultats que s'ils sont soutenus par le protocole correspondant. Jusqu'à ce moment, le statut reste `not_claimed`.
