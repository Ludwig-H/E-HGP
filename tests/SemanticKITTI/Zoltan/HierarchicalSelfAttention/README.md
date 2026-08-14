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

Au dernier fetch avant livraison, l'audit versionné épinglait `HEAD=dfa9e1b2950a11ef67f7a57463770c5be68059fb`, tandis que `origin/main=75f16db686ca48c87c277742ec3b069ca9c49767` contenait déjà des commits ultérieurs. Il n'était donc pas frais. Ce contrôle doit être refait au runtime après toute synchronisation ; tant que le pin, le `HEAD` et le worktree ne coïncident pas, la disponibilité supposée de la hiérarchie ne devient ni un claim produit ni un claim GPU sur MorseHGP3D.

## Verdict honnête

Chaque cluster doit porter un **vecteur de proportions sur les 19 classes**, et non un label unique. La cible d'un nœud est la distribution empirique de ses points ; les feuilles point sont le cas one-hot. Une représentation qui se limiterait à la fonction support normalisée et à ces proportions aurait peu de chances d'atteindre l'état de l'art : le support perd la non-convexité, tandis que les proportions conservent la masse des classes mais pas leur localisation à l'intérieur d'un cluster mixte.

La proposition étudiée est plus forte : elle couple le support normalisé à un **objet HGP marqué complet relativement à un profil déclaré**, de taille variable, dont le carrier géométrique peut être non convexe. Ce second canal conserve les points, les $(K-1)$-facettes actives, les cofaces de connexion, les incidences, les coordonnées et les niveaux de filtration requis par ce profil. Il n'est ni une seconde fonction support, ni un seul maximum radial, ni un histogramme de cellules. Une fonction radiale extérieure n'est qu'une ablation compressée et lossy ; ses contre-exemples ne réfutent pas le canal polyédral complet.

La définition HGP exige toutefois de distinguer trois objets. À un niveau $a=r^2$, le $K$-polyèdre source est l'union $V_v=C_v$ des observations d'une composante du graphe des $(K-1)$-facettes. Le porteur PL $P_v^{\mathrm{PL}}=\bigcup_F\mathrm{conv}(F)$ est non convexe mais son support égale celui de $V_v$. En revanche, le porteur canonique de multicoverture $W_v(a)=\bigcup_F\bigcap_{x\in F}\overline{B}(x,\sqrt{a})$, composante du niveau de densité correspondant, n'a pas en général le support de $V_v$. L'identité support–sommets ne réfute donc ni le complexe complet ni $W_v(a)$ ; elle interdit seulement de faire du support du porteur PL un second canal informatif.

Le canal complet sérialise les facettes, cofaces de connexion, incidences, coordonnées et niveaux de façon suffisante pour reconstruire le carrier choisi. Comme une composante persistante évolue entre sa naissance et sa fusion, chaque readout doit aussi fixer une politique de coupe, son niveau, son côté ouvert ou fermé et ses deltas ; la baseline recommandée lit chaque branche juste avant sa fusion au parent. La v3 actuelle ne livre pas encore ce payload : son chemin réduit vise les composantes H0, leurs niveaux et unions de `PointId`, tandis que les facettes, cofaces et incidences exhaustives restent un oracle borné. Leur disponibilité demeure une dépendance de recherche, sous `public_status=not_claimed`.

La piste élargie reste crédible et potentiellement forte :

1. un backbone local point/voxel conserve le détail et produit les features de feuilles ;
2. la hiérarchie HGP fournit une structure exogène multi-échelle ;
3. un encodeur d'incidences traite l'objet HGP marqué sans réduire immédiatement ses facettes et cofaces à une moyenne ; le support fournit en parallèle un raccourci global de dimension fixe ;
4. les feuilles prédisent leurs distributions et chaque nœud déduit exactement ses proportions par moyenne massique de ses descendants ;
5. un ou deux blocs hiérarchiques tardifs propagent le contexte ; HSA sert de baseline fidèle et un raffinement conditionné par la requête n'est retenu que s'il apporte un certificat fidélité–coût calculable ;
6. un décodeur point-fin et un chemin résiduel localisent les classes et préservent les frontières.

Cette représentation corrigée traite réellement la convexification et constitue une hypothèse de papier cohérente. Elle ne rend pas le SOTA probable à elle seule : une contribution de haut niveau reste plausible seulement si les expériences démontrent causalement que le complexe et l'arbre HGP apportent plus qu'un octree, des superpoints, RSL/HDBSCAN ou des complexes contrôles, et que l'opérateur hiérarchique apporte plus qu'un simple pooling ou message passing au même budget.

## Question scientifique centrale

> À backbone, budget, features et protocole constants, un objet HGP marqué, son carrier déclaré et sa hiérarchie de niveaux de densité améliorent-ils la segmentation sémantique LiDAR, notamment pour les classes rares et lointaines, par rapport aux structures géométriques, simpliciales ou apprises existantes ?

Cette question est séparée en trois effets identifiables :

- **effet arbre** : HGP contre les autres structures, avec le même opérateur ;
- **effet représentation** : objet HGP seul et support + même objet contre des encodeurs de points, graphes et complexes à budget apparié ;
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

Le nœud reçoit deux branches géométriques explicites. La branche courte échantillonne le support normalisé des observations, canal de forme convexe global. La branche structurelle encode l'objet HGP marqué, avec tokens de points/facettes/cofaces, arêtes d'incidence et résumés des régions témoins de $W_v(a)$, avant un readout invariant au réindexage. Comme le payload marqué contient déjà les observations, leur support ne prétend pas ajouter de l'information au carrier source ou PL ; il doit justifier son rôle de raccourci calculatoire dans l'ablation `objet complet seul` contre `support + objet complet`. Cette redondance ne s'étend pas au support propre de $W_v(a)$.

Le descripteur candidat conserve aussi :

- centre, `log(rayon d'échelle)`, cardinalité et géométrie relative parent–enfant ;
- niveau de densité, naissance, mort, persistance HGP et plateaux de filtration ;
- géométrie normalisée des cellules, incidences et cofaces de fusion ;
- anisotropie/covariance, portée, hauteur et statistiques de rémission ;
- CDF, Deep Sets, ECT/WECT et rayon extérieur comme contrôles ou ablations, pas comme substituts implicites du complexe complet.

## Portes avant entraînement lourd

Le projet ne passe au modèle complet que si les diagnostics suivants sont satisfaits :

- l'arbre HGP présente un alignement sémantique supérieur aux arbres de contrôle à compression égale ;
- une tokenisation éventuelle conserve assez d'information pour prédire les proportions **et** relocaliser les classes au niveau point ; sinon les points restent feuilles ;
- le round-trip de l'objet marqué est exact à réindexage près, reconstruit le porteur déclaré et l'encodeur distingue des objets ayant mêmes points et même support mais des incidences ou régions témoins différentes ;
- toute condensation mesure les collisions qu'elle introduit ; le rayon extérieur conserve séparément ses diagnostics de forme étoilée comme simple ablation ;
- l'effet de portée du capteur ne domine pas les niveaux de densité ;
- la profondeur, les degrés et la mémoire de la hiérarchie sont compatibles avec des batches GPU utiles.

Les critères chiffrés et les règles d'arrêt sont dans [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md).

## Contenu du dossier

- [HGP_HSA_SemanticKITTI.md](HGP_HSA_SemanticKITTI.md) : hypothèse scientifique révisée et variantes.
- [REVIEWER_VERDICT.md](REVIEWER_VERDICT.md) : décision honnête, questions de reviewer et barre de preuve par venue.
- [POLYHEDRAL_COMPLEX_BRANCH.md](POLYHEDRAL_COMPLEX_BRANCH.md) : contrat du complexe HGP complet, architecture d'incidence et théorèmes candidats.
- [GEOMETRIC_DESCRIPTOR_AUDIT.md](GEOMETRIC_DESCRIPTOR_AUDIT.md) : audit des compressions directionnelles, dont support et rayon, et contrôles topologiques.
- [THEOREM_PROGRAM.md](THEOREM_PROGRAM.md) : proposition technique, esquisse de preuve et pont conditionnel vers les niveaux de fusion.
- [ARCHITECTURE.md](ARCHITECTURE.md) : contrat d'entrée, descripteurs et modèle hybride proposé.
- [RESEARCH_PLAN.md](RESEARCH_PLAN.md) : lots de travail, ordre des décisions et livrables.
- [EXPERIMENTAL_PROTOCOL.md](EXPERIMENTAL_PROTOCOL.md) : splits, métriques, baselines et matrice d'ablation.
- [STATE_OF_THE_ART.md](STATE_OF_THE_ART.md) : concurrence et espace de nouveauté.
- [RISKS_AND_GO_NO_GO.md](RISKS_AND_GO_NO_GO.md) : réfutations, kill gates et pivots.
- [PAPER_STRATEGY.md](PAPER_STRATEGY.md) : positionnement d'une soumission de haut niveau.
- [REFERENCES.md](REFERENCES.md) : sources primaires et statut des chiffres.
- [NOTE_CLAUDE_DEUX_CANAUX_DIRECTIONNELS.md](NOTE_CLAUDE_DEUX_CANAUX_DIRECTIONNELS.md) : théorème de caractérisation du canal support, statut exact des canaux « dernière sortie » et « première entrée », centre capteur et échelle de complétude radiale.
- [NOTE_CLAUDE_ORDRE_DES_PREUVES.md](NOTE_CLAUDE_ORDRE_DES_PREUVES.md) : ordre d'acquisition des preuves, deux diagnostics sans entraînement et placement du budget de nouveauté.
- [papier HSA local](NeurIPS-2025-hierarchical-self-attention-generalizing-neural-attention-mechanics-to-multi-scale-problems-Paper-Conference.pdf) : copie vérifiée du papier NeurIPS 2025.

## Hors périmètre immédiat

La segmentation panoptique, la segmentation d'instance open-world, le multi-scan et la fusion RGB ne font pas partie de la première phase. Le modèle conservera seulement une interface future : logits et features par point, proportions sémantiques prédites par nœud, mapping point–feuille–ancêtres, topologie HGP et descripteurs de nœuds. Quand la sémantique sera validée, ALPINE deviendra une baseline aval obligatoire avec **exactement les mêmes logits sémantiques gelés**.

## Règle de claim

Les mots « exact », « temps réel », « GPU-friendly » et « état de l'art » ne doivent apparaître comme résultats que s'ils sont soutenus par le protocole correspondant. Jusqu'à ce moment, le statut reste `not_claimed`.
