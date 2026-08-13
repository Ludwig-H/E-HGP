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

L'audit v3 consulté après synchronisation n'est pas frais par rapport au dépôt : son observation live documente `HEAD=96156f6a1dd569c1c7e0371b0599e3b9ff08afd4`, alors que des commits ultérieurs étaient déjà présents pendant cette revue. Ce constat interdit de transformer la disponibilité supposée de la hiérarchie en claim produit ou GPU sur MorseHGP3D.

## Verdict honnête

La version minimale « fonction support normalisée seule, un label par cluster » a **peu de chances** d'atteindre l'état de l'art. Même connue dans toutes les directions, une fonction support décrit l'enveloppe convexe, pas les concavités, la densité, la distribution intérieure ni les parties occultées. Sa normalisation supprime aussi une taille métrique utile pour distinguer, par exemple, voiture et camion. Enfin, une prédiction uniforme par cluster crée un plafond irréversible aux frontières.

La piste élargie reste crédible et potentiellement forte :

1. un backbone local point/voxel conserve le détail et produit les features de feuilles ;
2. la hiérarchie HGP fournit une structure exogène multi-échelle ;
3. un sketch directionnel robuste — quantiles et canal de support maximal — contribue aux relations hiérarchiques, avec échelle, portée, densité et persistance comme canaux séparés ;
4. quelques blocs hiérarchiques tardifs propagent le contexte ; HSA sert de baseline fidèle et `QC-HSA`, si sa proposition est validée, conserve une requête distincte par feuille ;
5. un décodeur point-fin et un chemin résiduel préservent les frontières.

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

Au moment de la veille :

- RAPiD-Seg rapporte **76,1 mIoU** avec un pipeline appris LiDAR mono-trame en deux passes ; c'est le concurrent publié le plus proche, mais sa recette et l'usage éventuel de TTA doivent encore être audités pour une comparaison stricte ;
- LSK3DNet rapporte **75,6 mIoU** avec une architecture sparse point-voxel, instance CutMix, TTA et un entraînement test prolongé ; il relève donc du track TTA ;
- SP2T rapporte **75,4 mIoU** avec une attention sparse à proxies en double flux LiDAR mono-trame ; son statut TTA outdoor n'étant pas rapporté, la comparabilité stricte reste à auditer ;
- TASeg atteint **76,5 mIoU**, mais utilise de l'historique LiDAR et image et appartient donc à un régime de ressources distinct ;
- l'ancien leaderboard officiel contient aussi la soumission `SimpleSeg` à 76,5, sans méthode ni papier publiquement attribuable.

Ces chiffres sont des instantanés, pas des seuils éternels. La concurrence devra être réauditée avant soumission. Le premier objectif n'est pas de dépasser un nombre isolé, mais d'obtenir un gain HGP reproductible sur validation, puis une position Pareto compétitive en précision, mémoire et latence.

## Décision architecturale de départ

Le prototype de référence utilisera les **points ou micro-voxels comme feuilles** et les descripteurs des nœuds HGP pour construire un embedding par enfant dans le domaine de son parent. HSA fidèle sera d'abord reproduit. `QC-HSA` sera ensuite testé comme opérateur principal candidat, car il retire les égalités entre requêtes susceptibles de propager les erreurs aux frontières. Faire de chaque nœud interne un token appris restera une variante distincte ; aucun théorème de projection ne lui sera attribué sans nouvelle démonstration.

La fonction support est conservée comme canal de forme convexe et non comme identité complète du nœud. Le descripteur candidat combine :

- support maximal et quantiles directionnels ;
- centre, `log(rayon)`, cardinalité et géométrie relative parent–enfant ;
- niveau de densité, naissance, mort et persistance HGP ;
- anisotropie/covariance, portée, hauteur et statistiques de rémission.

## Portes avant entraînement lourd

Le projet ne passe au modèle complet que si les diagnostics suivants sont satisfaits :

- l'arbre HGP présente un alignement sémantique supérieur aux arbres de contrôle à compression égale ;
- une tokenisation éventuelle conserve une borne optimiste mIoU très supérieure à la cible, sinon les points restent feuilles ;
- les collisions de fonction support et sa fragilité aux outliers sont mesurées, pas ignorées ;
- l'effet de portée du capteur ne domine pas les niveaux de densité ;
- la profondeur, les degrés et la mémoire de la hiérarchie sont compatibles avec des batches GPU utiles.

Les critères chiffrés et les règles d'arrêt sont dans [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md).

## Contenu du dossier

- [HGP_HSA_SemanticKITTI.md](HGP_HSA_SemanticKITTI.md) : hypothèse scientifique révisée et variantes.
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

La segmentation panoptique, la segmentation d'instance open-world, le multi-scan et la fusion RGB ne font pas partie de la première phase. Le modèle conservera seulement une interface future : logits et features par point, mapping point–feuille–ancêtres, topologie HGP et descripteurs de nœuds. Quand la sémantique sera validée, ALPINE deviendra une baseline aval obligatoire avec **exactement les mêmes logits sémantiques gelés**.

## Règle de claim

Les mots « exact », « temps réel », « GPU-friendly » et « état de l'art » ne doivent apparaître comme résultats que s'ils sont soutenus par le protocole correspondant. Jusqu'à ce moment, le statut reste `not_claimed`.
