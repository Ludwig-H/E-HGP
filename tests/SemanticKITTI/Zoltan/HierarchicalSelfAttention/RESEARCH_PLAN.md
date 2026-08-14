# Plan de recherche

## Objectif

Déterminer si, et sous quelles conditions, la hiérarchie HGP est un meilleur support de propagation multi-échelle pour la segmentation sémantique LiDAR que les hiérarchies conventionnelles, puis construire un modèle compétitif sur SemanticKITTI si les portes causales sont franchies. La segmentation d'instance ne commence pas avant la fermeture positive de la phase sémantique.

Le plan est ordonné par information acquise : les expériences les moins coûteuses éliminent d'abord les hypothèses fragiles, avant l'entraînement d'un grand modèle.

## Constat préalable — la spécification est complète, la mesure est absente

Le dossier contient aujourd'hui une spécification très complète — objet, contrat de données, carriers, descripteurs de nœud, opérateur hiérarchique, protocole expérimental, registre de risques — et aucune mesure. Le rendement marginal d'une ligne de spécification supplémentaire est donc proche de zéro, alors que celui de la première mesure est très élevé : c'est elle, et non un raffinement du formalisme, qui décide si le programme continue. Le plan ci-dessous est réordonné en conséquence, de sorte que la première mesure arrive avant toute nouvelle spécification.

L'ordre des trois mesures d'entrée, leur protocole corrigé et le budget de nouveauté qu'elles conditionnent sont établis dans [VOIES.md](VOIES.md) ; ce plan les intègre sans les redémontrer. La caractérisation des canaux de descripteur, qui n'est pas sur le chemin critique, est traitée dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md). Concrètement, M1 et M2 sont absorbées par WP1, M3 devient WP1bis et précède WP2 ; aucune des trois ne demande d'entraînement.

## WP0 — Reproductibilité et contrat des données

### Travail

- figer la version de SemanticKITTI, le YAML officiel et le mapping des 19 classes ;
- produire pour chaque scan un manifeste : nombre de points, hash des entrées et hash de la hiérarchie ;
- définir le schéma de sérialisation HGP et vérifier le round-trip point–feuille–point ;
- versionner le contrat du complexe marqué avec `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only` ;
- sérialiser sommets, facettes, cofaces de connexion, incidences, niveaux, multiplicités, ainsi que `cut_policy`, `cut_level`, `cut_side` et les `deltas` de filtration ;
- distinguer dans le schéma le $K$-polyèdre source, les carriers PL de facettes et de cofaces, et l'union témoin $W_v(a_v)$, sans transférer leurs identités géométriques ;
- vérifier déterminisme, absence de labels et invariants de forêt ;
- enregistrer ordre HGP $K$, distance $d_{\mathrm{geo}}$, condensation et paramètres ;
- épingler l'API officielle à un commit exact et l'exécuter directement ; tout wrapper local doit démontrer sa parité sur des fixtures positives et négatives ;
- reproduire au moins une baseline publique sur la séquence 08.

### Livrables

- spécification versionnée du format hiérarchique et du complexe marqué, avec sémantique d'absence certifiée ;
- tests positifs et rejets de cycles, parents invalides, feuilles perdues, points dupliqués, incidences pendantes, cofaces manquantes et carrier ambigu ;
- reçu de reproduction de baseline avec environnement, seed et métriques par classe.

### Porte

Pas de passage si l'ordre des points n'est pas exactement restauré, si une information de label fuit dans la hiérarchie, si le payload ne distingue pas omission et absence certifiée, si le round-trip change la coupe ou ses deltas, ou si la baseline ne se situe pas dans une marge expliquée de la publication officielle. Le contrat sparse ne doit exiger ni reconstruire tout Čech ni matérialiser la mosaïque de Delaunay d'ordre supérieur.

## WP1 — Audit sans apprentissage de la hiérarchie

### Travail

Pour $K=1,2,3$, calculer sur train et validation. Vérifier d'abord par fixture que HGP $K=1$ est exactement le single-linkage ; ce cas n'est ensuite compté qu'une fois :

- nombre de nœuds, feuilles, profondeur, degrés et coût $\sum_v d_v^2$ ;
- compteurs $N_V,N_F,N_Q,N_I,N_A$, réutilisation des cellules entre niveaux et coût d'export du complexe marqué ;
- distribution des tailles, niveaux de naissance/mort et persistance ;
- stabilité sous thinning aléatoire, suppression structurée en élévation, jitter et outliers ;
- histogramme normalisé des 19 classes de chaque nœud, puis pureté, entropie et séparation dérivées aux ancêtres communs ;
- courbe compression–composition–localisation–mIoU pour différentes coupes/condensations ;
- mêmes mesures pour RSL/HDBSCAN, octree/voxel tree et arbre aléatoire contrôlé.

Deux de ces mesures sont prioritaires et portent les noms M1 et M2 dans [VOIES.md](VOIES.md). Elles ne demandent que la hiérarchie et les labels, aucun entraînement, et elles tranchent davantage que toute la matrice d'ablation prévue en aval : elles doivent donc être exécutées avant le reste de WP1.

**M1 — oracle d'antichaîne.** Choisir une antichaîne, c'est-à-dire un ensemble de nœuds deux à deux non emboîtés couvrant tous les points, étiqueter chaque nœud par sa classe majoritaire, puis calculer le mIoU obtenu. La formulation naïve est mal posée : le mIoU n'est pas additif sur les régions, donc « la meilleure antichaîne au sens du mIoU » n'est pas un problème d'optimisation bien posé et ne doit jamais être annoncé comme tel. Le protocole correct comporte deux volets. D'abord, à budget de régions fixé $R$, sélectionner l'antichaîne qui minimise l'impureté totale $\sum_{v} n_v H\left(\pi_v\right)$, critère additif sur les nœuds, qui admet une programmation dynamique exacte en une passe ascendante sur l'arbre avec un état « nombre de régions consommées dans le sous-arbre » ; rapporter ensuite le mIoU de l'antichaîne ainsi obtenue comme descripteur, jamais comme optimum. Ensuite, en complément, rapporter le mIoU-oracle de coupes à niveau fixé, qui est la convention de la littérature superpoint et permet la comparaison directe avec ses chiffres publiés. Tracer ces courbes en fonction du nombre de régions, à compression appariée, contre RSL/HDBSCAN au même $K$, un octree ou une grille de voxels, une partition superpoint et un arbre aléatoire de mêmes tailles de régions, en stratifiant obligatoirement par portée et par classe et en rapportant séparément les classes rares.

**Règle d'asymétrie, à fixer avant d'exécuter M1.** Cet oracle est déjà publié par la littérature superpoint, et son verdict y est défavorable à l'hypothèse « meilleure partition implique meilleure segmentation ». SPG rapporte sur S3DIS 6-fold un oracle de partition à $88{,}2$ mIoU pour un modèle à $62{,}1$ ; SPT observe que sa performance est à plus de vingt points sous l'oracle et en conclut que la partition ne limite pas fortement le résultat, soit un oracle $\gtrsim 89$ sur Area 5 pour un modèle à $68{,}9$ ; SuperCluster note qu'un oracle à $93{,}4$ PQ indique que très peu de précision est perdue en travaillant sur superpoints. Une vingtaine de points d'oracle sont donc déjà non convertis, et améliorer le plafond d'une partition qui n'est pas saturée ne peut pas payer. M1 est en conséquence une **porte de réfutation et non une porte de promotion** : le perdre réfute le programme sémantique, le gagner ne prouve presque rien. Le résultat doit être présenté avec cette asymétrie explicite, et un résultat négatif publié plutôt qu'absorbé. Le corollaire est à accepter dès maintenant : si le goulot n'est pas la partition, la valeur éventuelle de HGP ne peut pas venir de la qualité de ses régions.

**M2 — stabilité sous transport et rééchantillonnage.** Prendre des objets étiquetés à portée courte, les transporter à plusieurs portées et les rééchantillonner selon un modèle capteur déclaré — amincissement angulaire, disparition de retours, changement d'occultation — puis mesurer la dérive des niveaux de naissance et de mort, la stabilité de l'ancêtre commun et la persistance relative. Tester dans le même mouvement la correction : remplacer le niveau brut $a$ par un niveau normalisé par la densité d'échantillonnage attendue du capteur à cette portée et à cet angle d'incidence. Si la dérive intra-objet passe nettement sous la séparation interclasse, la filtration consciente du capteur devient une contribution en soi et répond à l'objection de portée au lieu de la contourner. Cette correction ne peut cependant pas être présentée comme acquise : l'ablation d'ALPINE montre qu'un seuil proportionnel à la portée, à la manière de LESS, donne $75{,}9$ PQ contre $76{,}3$ pour un seuil constant par classe, malgré une optimisation de son coefficient. Une correction de portée naïve dégrade donc leur clustering ; le cas n'est pas identique — leur seuil est un rayon de liaison et non un niveau de densité $K$-NN — mais ce résultat négatif doit être cité.

Deux courbes diagnostiques d'une **sortie dure token-constante**, qui n'est pas le modèle principal, sont distinguées :

1. baseline réalisable du label majoritaire par token, optimale pour l'accuracy token-constante mais pas pour le mIoU ;
2. borne supérieure relaxée par classe, où chaque masque de classe choisit indépendamment une union de tokens, non nécessairement réalisable comme partition multiclasses.

Une optimisation multiclasses exacte ou bornée sur de petites fixtures peut compléter ces courbes. Le vote majoritaire ne doit pas être appelé à tort « upper bound mIoU exact ».

### Livrables

- rapport `hierarchy_audit` par scan, classe, portée et $K$ ;
- courbes M1 impureté–mIoU–nombre de régions et tableau M2 de dérive des niveaux, avec et sans normalisation par la densité d'échantillonnage attendue ;
- mesure de la profondeur $D$ et de la distribution des degrés de l'arbre de fusion, avant et après condensation, avec la statistique du nombre de produits matrice creuse–vecteur séquentiels qu'elle implique : l'algorithme HSA en demande $D$ en séquence, donc la condensation de l'arbre est une **condition d'existence sur GPU** et non une optimisation, et ce chiffre doit être connu avant WP4 ;
- figures des courbes de sortie dure majoritaire/optimiste mIoU–compression et stabilité–portée ;
- fixtures minimales des échecs de laminarité, chaining et frontières traversées.

### Porte

- si HGP ne bat pas le meilleur contrôle structurel à coût apparié, le claim « meilleur arbre » est suspendu ; l'appariement porte sur compression, nombre de nœuds internes, paramètres, arêtes, profondeur/degrés, $\sum_v d_v^2$, latence et VRAM, pas sur le seul nombre de feuilles qui reste identique lorsque les points sont feuilles ;
- si la baseline majoritaire perd plus de 1 à 2 points au taux de compression utile, seule la tête dure cluster-constante est rejetée ; une feuille micro-token reste admissible si un décodeur point-wise relocalise les classes et si les proportions restent bien estimées ;
- si la profondeur ou les degrés rendent HSA impraticable, tester une condensation documentée avant tout modèle complet.

## WP1bis — Diagnostic à une seule variable par substitution du clusterer d'ALPINE

Ce lot est court, prioritaire, sans entraînement, et placé avant WP2. Il correspond à la mesure M3 de [VOIES.md](VOIES.md).

**Ce que ce lot n'est pas.** Il n'ouvre pas la phase instance comme contribution : celle-ci reste fermée dans les termes fixés plus bas, et l'usage d'un clustering pour les instances n'est plus une contribution suffisante. Il s'agit ici d'un diagnostic à une seule variable, qui teste l'effet arbre au coût de calcul le plus bas et dont le résultat conditionne l'engagement de semaines de GPU sur la voie sémantique. Une phase fermée pour la publication peut rester ouverte pour le diagnostic.

### Travail

Reprendre le pipeline ALPINE tel quel — mêmes logits sémantiques gelés publics, mêmes seuils $t_c$, même découpage récursif des boîtes — et remplacer uniquement les composantes connexes du graphe $k$-NN élagué par les $K$-polyèdres à $K=2,3$. Rapporter PQ, PQ des classes thing, la ventilation par portée et le temps par trame sur matériel déclaré.

### Pourquoi ce lot avant WP2

Le clusterer pèse lourd et sa contribution est isolable. À sémantique MinkUNet identique et découpage de boîtes désactivé pour tous, sur SemanticKITTI val, ALPINE obtient $65{,}5$ PQ, D&M $61{,}8$, DBSCAN $56{,}7$ et HDBSCAN $55{,}1$ : l'écart imputable au seul clusterer est de $10{,}4$ PQ, et HDBSCAN, dont HGP est le correctif de principe, est bon dernier.

Le clusterer d'ALPINE est littéralement du single-linkage : projection BEV par classe thing, graphe $k$-NN à $k=32$, suppression des arêtes plus longues qu'un seuil constant par classe tiré de dimensions d'objets trouvées sur le web, composantes connexes, puis découpage récursif des composantes dont la boîte dépasse de $30\,\%$ la boîte de référence. C'est donc HGP à $K=1$ avec un rayon dépendant de la classe et une projection BEV, et son mode d'échec déclaré par ses auteurs est le chaînage, « failure cases can be crafted by making two objects closer than the chosen threshold ». HGP à $K\geq2$ en est exactement la généralisation d'ordre supérieur, celle que la thèse construit pour corriger ce mode d'échec : la baseline et l'hypothèse coïncident, ce qui rend l'expérience identifiable.

Le plafond est bas et doit être annoncé avant de mesurer, sous peine de surinterpréter le résultat. L'oracle d'instance à sémantique figée ne rapporte que $+4{,}3$ PQ sur nuScenes avec PTv3, de $78{,}9$ à $83{,}2$, et $+3{,}8$ avec WaffleIron-768 ; sous sémantique parfaite, le clusterer d'ALPINE ne perd que $1{,}0$ PQ sur SemanticKITTI et $3{,}8$ sur nuScenes, ce qui conduit ses auteurs à conclure que l'extraction d'instances est largement saturée. ALPINE ne dépasse les têtes d'instance entraînées que de $+0{,}1$ à $+0{,}8$ PQ. Dans ce barème, $+1$ à $+2$ PQ serait un résultat fort et $+4$ le maximum atteignable.

La contrainte réelle est le temps, et elle est défavorable. ALPINE tourne à $14{,}4$ Hz sur un seul cœur CPU, quand le seul chiffre disponible pour le pipeline HGP historique est de l'ordre de la seconde par trame SemanticKITTI. Il y a plus d'un ordre de grandeur à combler, aucun jury ne l'ignorera, et le temps mesuré ici est un résultat au même titre que le PQ.

### Livrables

- reçu de reproduction d'ALPINE à l'identique, logits gelés inclus, obtenu avant toute substitution ;
- tableau PQ, PQ†, RQ, SQ et PQ des classes thing à sémantique gelée, ALPINE contre HGP $K=1,2,3$, avec ventilation par portée et temps par trame sur matériel déclaré ;
- fixtures de chaînage construites explicitement, où la prédiction de la thèse est vérifiable point par point, y compris les cas où elle échoue ;
- consignation du signal contraire déjà relevé en M2 sur la correction de portée.

### Porte

Si HGP à $K=2,3$ ne bat pas du single-linkage à sémantique gelée et à protocole identique, sur la tâche même que la thèse a construite pour lui, l'hypothèse d'un effet arbre utile est sévèrement affaiblie ; ce résultat doit être consigné avant d'engager des semaines de GPU sur la voie sémantique, et non absorbé. Un gain se lit dans le barème ci-dessus et non dans l'absolu. Un dépassement du budget de temps de plus d'un ordre de grandeur est un résultat négatif à part entière et non un détail d'implémentation. Enfin, un succès ici ne promeut aucun claim d'instance : il autorise seulement la poursuite du programme sémantique.

## WP2 — Canal support + complexe HGP marqué

La porte analytique distingue d'abord quatre carriers contractuels : `source_points`, `facet_pl`, `coface_pl` et `witness_union`. L'identité $h_{P_v^{\mathrm{PL}}}=h_{C_v}$ montre seulement que le support des carriers source/facettes PL est redondant avec leurs sommets ; le carrier de cofaces exige sa condition de couverture. Elle ne réduit pas le complexe marqué à ce support et ne vaut pas en général pour $W_v(a_v)$. L'hypothèse primaire est donc bien **support normalisé + représentation incidence-aware du complexe HGP complet relativement au contrat**. Le rayon extérieur reste une ablation compressée et lossy.

### Travail

Construire d'abord des fixtures à support source identique mais à incidences, niveaux ou carriers différents : mêmes sommets avec triangulations distinctes, coquille contre remplissage, coface de connexion supprimée, et deux multicovertures évaluées à des niveaux $a$ différents. Vérifier direction par direction `support(source) == support(PL)`, sans imposer cette égalité au carrier de multicoverture. Le cube plein contre sa frontière, les centres hors noyau et les intersections radiales multiples restent des tests de perte pour l'ablation radiale.

La voie implémentable procède ensuite ainsi :

1. consommer uniquement l'export sparse contractuel, stocker chaque cellule une fois et référencer ses appartenances ou deltas de filtration ;
2. initialiser sommets, facettes et cofaces avec features locales, géométrie normalisée, niveau, multiplicité et paramètres du carrier choisi ;
3. appliquer deux à quatre passes incidence-aware sommet–facette–coface et retour ;
4. produire un readout par nœud, le concaténer éventuellement au support source et aux side channels métriques, puis l'injecter dans le même opérateur hiérarchique ;
5. commencer par $K=1$ ou une laminarisation auditée ; la voie recouvrante reste bloquée tant qu'une application déterministe $w_{iv}$, son domaine, la contrainte $\sum_v w_{iv}=1$ et ses tests de conservation de masse ne sont pas spécifiés.

Aucun de ces jalons ne construit le complexe de Čech global ni une mosaïque de Delaunay d'ordre supérieur.

La voie de calcul réaliste en dimension 3 n'est d'ailleurs pas la voie exacte, et ce point doit être décidé explicitement plutôt que subi. Le manuscrit ne donne aucune borne de complexité pour $\mathrm{Del}_K$ en dimension $p$ — la seule borne citée, $O\left(n^{\lceil p/2 \rceil}\right)$, concerne la triangulation de Delaunay ordinaire — et sa section 9.3 propose à la place Vietoris-Rips, encadré par $\mathrm{Cech}\left(X,r\right) \subseteq \mathrm{VR}\left(X,r\right) \subseteq \mathrm{Cech}\left(X,\alpha_p r\right)$ avec $\alpha_p=\sqrt{2p/\left(p+1\right)}$ par le théorème de Jung, soit $\alpha_3=\sqrt{1{,}5}\approx 1{,}2247$ en dimension 3. Cette voie se réduit à quatre opérations de graphe massivement parallélisables et, pour $K=2$, à une énumération de triangles par intersection de listes d'adjacence triées dont le coût suit le nombre réel de triangles et non $C\left(n,K+1\right)$. Le prix payé n'est donc pas le temps mais l'exactitude : sous Vietoris-Rips, le Théorème 2 ne tient plus qu'à $\alpha_3$ près. En conséquence, le choix exact contre approché doit être inscrit au contrat comme un niveau d'`authority` distinct, avec le facteur $\alpha_3$ déclaré dans le reçu, et aucun résultat obtenu par cette voie ne peut être présenté comme exact. Le seul chiffre de temps disponible côté manuscrit est de l'ordre de la seconde par trame SemanticKITTI, pour une cible usuelle de dix trames par seconde. Les comparaisons causales P0–P7 du protocole isolent `support seul`, `complexe seul`, `support + complexe`, accès à $\Gamma_K^{\mathrm{elem}}$ avec tokens précalculés, sac de tokens sans messages, mutant invalide et support + radial. P2 et P3 conservent exactement les mêmes `payload_kind`, `carrier_kind`, `authority` et coupe. Lorsque les sommets du carrier source/PL sont disponibles, le support source est calculable et tout gain P3 sur P2 est un shortcut d'optimisation ; cette redondance n'est jamais transférée au support de $W_v(a_v)$.

Les contrôles compressés restent CDF/histogrammes directionnels, ECT/WECT fini, moments/covariance, kernel mean et mini-PointNet/Deep Sets à dimension, paramètres et coût aussi proches que possible. Orthogonalement, ablater géométrie seule, proportions sémantiques prédites seules, puis leur combinaison avec entropie moyenne et désaccord. Aucune proportion GT n'entre dans le forward.

Mesurer $N_V,N_F,N_Q,N_I,N_A$, collisions, sensibilité au thinning, mémoire, latence d'export, initialisation, passages d'incidence et readout. Pour `witness_union`, ajouter $N_W$ ventilé en requêtes d'appartenance/distance, patches de frontière et échantillons, l'erreur $\varepsilon_W$ contre l'oracle borné, ainsi que le temps et la mémoire correspondants. L'oracle de sérialisation et le hash canonique doivent séparer les fixtures distinctes ; tant que T2 n'est pas prouvé, le learned encoder est falsifié et son taux de collisions est mesuré, sans exiger qu'il distingue toute paire. Séparer arbre/complexe figés pour isoler l'encodeur, puis reconstruction recalculée pour mesurer le pipeline complet. Rapporter aussi performance par frontières, portée et classe rare : une meilleure reconstruction globale des proportions sans meilleure localisation point-wise ne suffit pas.

La voie théorique ambitieuse sur le DAG recouvrant reste une obligation non résolue, pas une variante implémentable acquise. Après définition de $w_{iv}$, elle devra prouver conservation de masse, stochasticité, équivariance aux réindexations, réduction exacte au cas laminaire et absence de double comptage, puis établir une séparation utile face à l'accès $\Gamma_K^{\mathrm{elem}}$ ou un certificat fidélité–coût. Les réseaux simpliciaux et cellulaires incidence-aware étant antérieurs, une simple preuve d'équivariance ou d'expressivité de type WL ne suffit pas pour porter le papier.

### Livrables

- schéma validé et loader du complexe marqué, avec oracle borné des quatre `carrier_kind` et des cinq niveaux d'`authority` ;
- encodeur incidence-aware sparse de référence et tests de réindexation, mutation et recouvrement ;
- tableau P0–P7 multi-seeds avec mIoU, frontières, proportions, RAM/VRAM et latence de bout en bout ;
- courbes qualité–$N_I$–$N_A$–coût et rapport de sensibilité par portée ;
- registre permanent des collisions synthétiques et réelles.

### Porte

Le canal complexe survit comme contribution empirique seulement s'il bat support seul, P4 avec accès $\Gamma_K^{\mathrm{elem}}$ restreint et P5 sans messages de façon reproductible à coût explicite, avec un objectif pré-enregistré d'au moins +0,5 mIoU ou un meilleur Pareto robustesse–coût. P4/P5 sont des restrictions d'accès calculatoires, pas des suppressions informationnelles garanties. `Support + complexe` n'a pas besoin de battre `complexe seul` pour valider l'information non convexe ; cette comparaison mesure uniquement l'aide du shortcut sur les carriers source/PL, sans claim correspondant pour le support de $W_v$. Si PL et multicoverture sont équivalents empiriquement, retenir le moins coûteux sans claim géométrique supérieur. Si l'encodeur complet est dominé ou si $N_A$ explose, rétrograder le complexe en étude négative. Le radial ne remplace jamais silencieusement le canal complet. La voie DAG ne devient centrale qu'avec un théorème non trivial et un gain sur au moins deux capteurs.

## WP3 — Effet arbre avec agrégateur simple

### Travail

Avant HSA, ajouter au backbone reproduit un passage bottom-up/top-down simple et peu paramétrique. Garder features, dimension, nombre de couches, recette et seeds constants, et échanger uniquement la structure :

- HGP $K=1$/single-linkage, avec identité vérifiée par fixture ;
- arbres HGP $K=2,3$ ;
- RSL/HDBSCAN ;
- octree ou voxel tree ;
- superpoints hiérarchiques ;
- arbre aléatoire conservant autant que possible nombre de nœuds, profondeurs et degrés ;
- permutation de l'arbre comme null test.

### Pourquoi avant HSA

Si HGP ne produit aucun gain avec un opérateur neutre, HSA risque de masquer l'absence d'alignement structurel. Inversement, un gain avec simple pooling démontre déjà une valeur de l'arbre et rend l'expérience HSA identifiable.

### Livrables

- tableau apparié multi-seeds ;
- intervalles de confiance obtenus en rééchantillonnant des blocs temporels puis en recalculant le mIoU depuis les matrices de confusion agrégées ;
- analyse des gains par classe, portée, taille de composante et pureté locale.

### Porte

Continuer vers un claim HGP seulement si l'intervalle de confiance du gain contre le meilleur arbre de contrôle exclut zéro et si l'effet est substantiel, avec un objectif pré-enregistré d'au moins +0,5 mIoU ou d'un meilleur compromis précision–coût. Un gain plus petit peut justifier une analyse, pas une promesse SOTA.

## WP4 — Effet de l'opérateur et proposition QC-HSA

### Travail

Geler avant E3 la représentation retenue en E2 : mêmes `payload_kind`, `carrier_kind`, `authority`, coupe, tokens, largeur, backbone, budget et seeds. Implémenter ensuite le mapping fidèle : Q/K/V aux feuilles, descripteurs HGP comme relations/positions, passes bottom-up et top-down. Comparer sur le même arbre :

- `mean/max + MLP` ;
- message passing parent–enfant/frères ;
- Sequoia/attention hiérarchique locale si adaptable ;
- HSA ;
- `QC-HSA`, projection conditionnée par la feuille décrite dans [THEOREMES.md](THEOREMES.md) ;
- raffinement adaptatif d'antichaînes par requête, seulement après validation de `QC-HSA` fixe ;
- contrôle de type Fast Multipole Attention si une adaptation fidèle est possible ;
- attention locale du backbone supplémentaire à paramètres égaux ;
- attention plate sur scans/sous-ensembles compatibles avec la mémoire.

Tester le nombre et le placement des blocs, les têtes locales de secours et le gate résiduel. Mesurer kernel time, synchronisations, débit et mémoire, pas seulement les FLOPs théoriques.

Pour les diagnostics KL, HSA et `QC-HSA` reçoivent une **même cible plate $P$ gelée**, les mêmes features, scores et masque diagonal. Deux modèles entraînés séparément ne définissent généralement pas le même $P$ et ne permettent pas d'attribuer causalement une différence de KL à la famille de contraintes.

Le travail théorique associé doit :

- prouver la projection reverse-KL avec masque diagonal, multifurcations et log-cardinalité ;
- prouver l'inclusion de la famille HSA et caractériser l'inégalité stricte ;
- prouver l'exactitude des scores constants sur les couples de branches de fusion ;
- formaliser le pont conditionnel entre distorsion des hauteurs de fusion et stabilité de la décision, sans le confondre avec une garantie de mIoU ;
- séparer le résultat fini HGP à $K$ fixe d'une éventuelle borne populationnelle en régime $K_n$, et conserver la distorsion de fusion comme diagnostic si aucune convergence n'est prouvée ;
- vérifier le certificat par oscillation des scores et le corollaire de marge ;
- chercher une borne d'oscillation calculable depuis les résumés géométriques sans attention dense ;
- définir une règle de raffinement monotone qui converge vers l'attention dense et garantit une tolérance KL ou sortie à coût mesuré ;
- pour une future voie recouvrante, définir d'abord les poids $w_{iv}$ et leur domaine, puis prouver partition de l'unité, conservation de masse, stochasticité, équivariance et réduction exacte à HSA lorsque les appartenances deviennent laminaires ;
- étudier une sélection de coupe sous budget seulement si son objectif et son coût GPU sont explicitement additifs, les DP génériques d'élagage étant antérieures ;
- auditer l'antériorité face à Fast Multipole Attention, H-Transformer, MRA, HKT, MPSN/CWN, attention simpliciale et projections de Bregman.

### Livrables

- test unitaire par comparaison à l'attention HSA directe sur petits arbres ;
- test de `QC-HSA` contre une optimisation convexe et une attention dense explicite sur petits arbres, y compris cas d'égalité et d'inégalité stricte avec HSA ;
- mutants d'indices de profondeur/position, signe de normalisation, facteur de cardinalité, normalisation des familles et ordre des feuilles ;
- arbres équilibré, étoile, chaîne/peigne, singleton et degré un, avec égalités de plaques HSA vérifiées entre lignes et colonnes ;
- test de racine factice et de batch block-diagonal : aucun autre scan ne modifie une sortie ;
- après spécification de $w_{iv}$ seulement, fixtures DAG recouvrantes vérifiant conservation, absence de double comptage et réduction au cas arbre ;
- tableau opérateur × arbre avec mêmes seeds ;
- profil GPU et end-to-end.

### Porte

Si ni HSA ni `QC-HSA` n'améliorent l'agrégateur simple ou son Pareto précision–coût, le projet garde éventuellement HGP mais abandonne le claim attention. Si `QC-HSA` ne réduit ni la distorsion mesurée ni les erreurs de frontière par rapport à HSA à coût acceptable, sa proposition reste un résultat d'appendice. Le retrait des égalités entre lignes supprime aussi une régularisation possible : un meilleur KL vers $P$ peut généraliser moins bien. Les tokens de nœuds internes ne sont ouverts qu'ensuite comme nouvelle hypothèse, jamais comme correction silencieuse.

## WP5 — Modèle sémantique compétitif

### Travail

- porter le module positif depuis la recette SemanticKITTI épinglée de WP0 vers PTv3, si sa reproduction est validée, puis vers un second backbone sparse fort ;
- ajuster capacité et profondeur sans changer le protocole principal ;
- tester pertes de frontière et rares classes après verrouillage de l'architecture ;
- lancer les stress tests complets par portée, thinning, outliers et densité ;
- vérifier le scaling en points, nœuds, profondeur et batch ;
- comparer RAPiD-Seg, LSK3DNet, SP2T, SphereFormer, PTv3, LitePT et SPT/EZ-SP adapté, en séparant strictement les régimes TTA, multi-pass et préentraînement ; la TTA de SphereFormer reste `NR` tant qu'une source primaire ne la documente pas.

### Livrables

- checkpoint et config de chaque seed ;
- tableau mIoU/per-class, robustesse et efficacité ;
- carte des succès/échecs et exemples qualitatifs choisis par règle fixe ;
- modèle verrouillé pour la soumission test.

### Porte SOTA

Une soumission test n'est justifiée que si le modèle strict LiDAR mono-trame est compétitif sur 08, que le gain causal HGP est confirmé, et que le benchmark concurrent a été réaudité. Dépasser un ancien score sans régime de données comparable ne constitue pas un SOTA.

## WP6 — Généralisation et contribution top-conférence

SemanticKITTI seul risque de produire un papier benchmark. Ajouter au moins un second dataset extérieur, de préférence nuScenes-LidarSeg pour le changement de capteur, puis SemanticPOSS ou KITTI-360 pour la robustesse de domaine.

Choisir une contribution générale selon les résultats :

- stabilité d'une hiérarchie de densité corrigée de la portée ;
- encodeur de complexe HGP marqué avec analyse de coût, d'incidence et de recouvrement ;
- attention conservatrice sur DAG de polyèdres recouvrants, si sa théorie et son bénéfice survivent ;
- projection hiérarchique conditionnée par la feuille avec garantie KL et correction de frontière ;
- étude causale des hiérarchies exogènes pour l'attention 3D.

Le second dataset doit tester le mécanisme, pas seulement ajouter une ligne de score.

## Phase instance — fermée jusqu'à validation sémantique

Cette fermeture porte sur la contribution, pas sur le diagnostic : WP1bis utilise la tâche d'instance comme instrument de mesure à sémantique gelée, sans en tirer aucun claim d'instance, et ne constitue donc pas une ouverture anticipée de cette phase.

Condition d'ouverture : modèle sémantique finalisé, logits reproductibles et gain HGP établi. Alors seulement :

- geler les mêmes logits sémantiques ;
- comparer ALPINE, DBSCAN/HDBSCAN, coupe HGP et coupe HGP apprise ;
- évaluer PQ, PQ†, RQ, SQ et PQ des classes thing ;
- envisager une sélection d'antichaîne et des corrections split/merge.

Cette phase ne peut pas expliquer ni compenser un échec sémantique.

## Journal de décisions attendu

Chaque lot se clôt par une note comprenant : hypothèse, configuration exacte, résultat, intervalle, coût, contre-exemple éventuel et décision `go`, `revise` ou `stop`. Une contradiction mathématique ou un échec structurel devient une fixture permanente avant toute optimisation suivante.

## Séquencement indicatif

Le calendrier dépend de la disponibilité du dataset et des GPU ; il exprime un ordre, pas une promesse de durée :

| Fenêtre | Jalon | Sortie attendue |
|---|---|---|
| semaines 1–2 | WP0 | contrat, baseline reproduite, évaluateur figé |
| semaines 3–5 | WP1, M1 et M2 d'abord | audit HGP, oracle d'antichaîne, stabilité en portée, décision points/tokens |
| semaines 3–4 | WP1bis, en parallèle de WP1 | PQ à sémantique gelée et temps par trame, verdict à une seule variable sur l'effet arbre |
| semaines 4–7 | WP2 en parallèle | décision support/complexe/carrier, choix exact contre Vietoris-Rips, coût d'incidence |
| semaines 8–11 | WP3 | effet propre de l'arbre |
| semaines 12–16 | WP4 | effet propre de HSA et profil système |
| semaines 17–21 | WP5 | modèle sémantique verrouillé, stress tests |
| semaines 22–26 | WP6 | second dataset et décision de soumission |

Une porte négative raccourcit le programme : elle déclenche le pivot associé au lieu d'ajouter des composants. La phase instance possède son propre calendrier et n'est pas incluse ici.
