# Plan de recherche

## Objectif

Déterminer si, et sous quelles conditions, la hiérarchie HGP est un meilleur support de propagation multi-échelle pour la segmentation sémantique LiDAR que les hiérarchies conventionnelles, puis construire un modèle compétitif sur SemanticKITTI si les portes causales sont franchies. La segmentation d'instance ne commence pas avant la fermeture positive de la phase sémantique.

Le plan est ordonné par information acquise : les expériences les moins coûteuses éliminent d'abord les hypothèses fragiles, avant l'entraînement d'un grand modèle.

## WP0 — Reproductibilité et contrat des données

### Travail

- figer la version de SemanticKITTI, le YAML officiel et le mapping des 19 classes ;
- produire pour chaque scan un manifeste : nombre de points, hash des entrées et hash de la hiérarchie ;
- définir le schéma de sérialisation HGP et vérifier le round-trip point–feuille–point ;
- vérifier déterminisme, absence de labels et invariants de forêt ;
- enregistrer ordre HGP $K$, distance $d_{\mathrm{geo}}$, condensation et paramètres ;
- épingler l'API officielle à un commit exact et l'exécuter directement ; tout wrapper local doit démontrer sa parité sur des fixtures positives et négatives ;
- reproduire au moins une baseline publique sur la séquence 08.

### Livrables

- spécification versionnée du format hiérarchique ;
- tests positifs et rejets de cycles, parents invalides, feuilles perdues et points dupliqués ;
- reçu de reproduction de baseline avec environnement, seed et métriques par classe.

### Porte

Pas de passage si l'ordre des points n'est pas exactement restauré, si une information de label fuit dans la hiérarchie ou si la baseline ne se situe pas dans une marge expliquée de la publication officielle.

## WP1 — Audit sans apprentissage de la hiérarchie

### Travail

Pour $K=1,2,3$, calculer sur train et validation. Vérifier d'abord par fixture que HGP $K=1$ est exactement le single-linkage ; ce cas n'est ensuite compté qu'une fois :

- nombre de nœuds, feuilles, profondeur, degrés et coût $\sum_v d_v^2$ ;
- distribution des tailles, niveaux de naissance/mort et persistance ;
- stabilité sous thinning aléatoire, suppression structurée en élévation, jitter et outliers ;
- histogramme normalisé des 19 classes de chaque nœud, puis pureté, entropie et séparation dérivées aux ancêtres communs ;
- courbe compression–composition–localisation–mIoU pour différentes coupes/condensations ;
- mêmes mesures pour RSL/HDBSCAN, octree/voxel tree et arbre aléatoire contrôlé.

Deux courbes diagnostiques d'une **sortie dure token-constante**, qui n'est pas le modèle principal, sont distinguées :

1. baseline réalisable du label majoritaire par token, optimale pour l'accuracy token-constante mais pas pour le mIoU ;
2. borne supérieure relaxée par classe, où chaque masque de classe choisit indépendamment une union de tokens, non nécessairement réalisable comme partition multiclasses.

Une optimisation multiclasses exacte ou bornée sur de petites fixtures peut compléter ces courbes. Le vote majoritaire ne doit pas être appelé à tort « upper bound mIoU exact ».

### Livrables

- rapport `hierarchy_audit` par scan, classe, portée et $K$ ;
- figures des courbes de sortie dure majoritaire/optimiste mIoU–compression et stabilité–portée ;
- fixtures minimales des échecs de laminarité, chaining et frontières traversées.

### Porte

- si HGP ne bat pas le meilleur contrôle structurel à coût apparié, le claim « meilleur arbre » est suspendu ; l'appariement porte sur compression, nombre de nœuds internes, paramètres, arêtes, profondeur/degrés, $\sum_v d_v^2$, latence et VRAM, pas sur le seul nombre de feuilles qui reste identique lorsque les points sont feuilles ;
- si la baseline majoritaire perd plus de 1 à 2 points au taux de compression utile, seule la tête dure cluster-constante est rejetée ; une feuille micro-token reste admissible si un décodeur point-wise relocalise les classes et si les proportions restent bien estimées ;
- si la profondeur ou les degrés rendent HSA impraticable, tester une condensation documentée avant tout modèle complet.

## WP2 — Audit des descripteurs

Ajouter une porte analytique avant apprentissage : vérifier sur chaque $K$-polyèdre que le support de sa réalisation simpliciale coïncide avec celui de ses sommets. Cette identité interdit de compter la réalisation comme un descripteur distinct. Tester ensuite, à budget égal, une mesure d'attributs simpliciaux — centres, forme, niveau de naissance et incidence — contre support, CDF des points, moments et mini-PointNet.

### Travail

Construire d'abord des contre-exemples à enveloppe convexe identique : sommets seuls, intérieur dense, coquille, forme concave et deux amas reliés par un pont intérieur. À centre et rayon fixés, le support maximal doit être identique alors que la densité et la topologie changent.

Sur les nœuds réels, comparer à dimension et budget proches :

- support maximal brut ;
- support maximal normalisé ;
- support + échelle/position/cardinalité ;
- support + attributs HGP ;
- CDF/histogrammes directionnels à bins fixes + max ;
- pile de quantiles directionnels + max ;
- moments/covariance/histogrammes radiaux ;
- mini-PointNet ou Deep Sets ;
- occupations multi-coquilles ou encodeur équivariant léger comme contrôle ambitieux.

Orthogonalement, ablater le contenu transmis : géométrie seule, proportions sémantiques déduites des descendants seules, puis combinaison avec entropie moyenne et désaccord. Aucune proportion GT n'entre dans cette comparaison à l'inférence.

Faire varier le nombre et la grille de directions ainsi que le nombre de bins. Mesurer le rayon de couverture de la sphère, l'erreur contre une référence dense, les collisions de features, la mémoire et la latence. Tester explicitement la fusion des histogrammes dans un repère commun et l'erreur introduite par le transport centre/échelle entre enfants et parent.

Séparer deux stress tests : arbre figé pour isoler le descripteur, puis arbre recalculé pour mesurer le pipeline. Injecter outliers, thinning dépendant de la portée et suppression de points extrêmes.

### Livrables

- benchmark de prédiction/retrieval de composition sémantique avec KL/JS, Brier et erreur sur les 19 proportions ;
- courbes directions–erreur–coût ;
- rapport de sensibilité par portée et dimension intrinsèque ;
- décision explicite sur centre, rayon et canaux robustes.

### Porte

Le support seul est rejeté comme représentation principale si une augmentation légère gagne de façon répétée environ 1 point de mIoU à budget égal, si les collisions sémantiquement contradictoires sont fréquentes, ou s'il est nettement moins stable que les contrôles. Il peut rester un canal complémentaire.

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

Implémenter d'abord le mapping fidèle : Q/K/V aux feuilles, descripteurs HGP comme relations/positions, passes bottom-up et top-down. Comparer sur le même arbre :

- `mean/max + MLP` ;
- message passing parent–enfant/frères ;
- Sequoia/attention hiérarchique locale si adaptable ;
- HSA ;
- `QC-HSA`, projection conditionnée par la feuille décrite dans [THEOREM_PROGRAM.md](THEOREM_PROGRAM.md) ;
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
- auditer l'antériorité face à Fast Multipole Attention, H-Transformer, MRA, HKT et aux projections de Bregman.

### Livrables

- test unitaire par comparaison à l'attention HSA directe sur petits arbres ;
- test de `QC-HSA` contre une optimisation convexe et une attention dense explicite sur petits arbres, y compris cas d'égalité et d'inégalité stricte avec HSA ;
- mutants d'indices de profondeur, normalisation des familles et ordre des feuilles ;
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
- comparer RAPiD-Seg, LSK3DNet, SP2T, SphereFormer, PTv3 et SPT/EZ-SP adapté.

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
- descripteur directionnel robuste et fusionnable avec analyse d'erreur ;
- projection hiérarchique conditionnée par la feuille avec garantie KL et correction de frontière ;
- étude causale des hiérarchies exogènes pour l'attention 3D.

Le second dataset doit tester le mécanisme, pas seulement ajouter une ligne de score.

## Phase instance — fermée jusqu'à validation sémantique

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
| semaines 3–5 | WP1 | audit HGP et décision points/tokens |
| semaines 4–7 | WP2 en parallèle | descripteur retenu ou support rétrogradé |
| semaines 8–11 | WP3 | effet propre de l'arbre |
| semaines 12–16 | WP4 | effet propre de HSA et profil système |
| semaines 17–21 | WP5 | modèle sémantique verrouillé, stress tests |
| semaines 22–26 | WP6 | second dataset et décision de soumission |

Une porte négative raccourcit le programme : elle déclenche le pivot associé au lieu d'ajouter des composants. La phase instance possède son propre calendrier et n'est pas incluse ici.
