# Plan de recherche

## Objectif

Déterminer si, et sous quelles conditions, la hiérarchie HGP est un meilleur support de propagation multi-échelle pour la segmentation sémantique LiDAR que les hiérarchies conventionnelles, puis construire un modèle compétitif sur SemanticKITTI si les portes causales sont franchies. La segmentation d'instance ne commence pas avant la fermeture positive de la phase sémantique.

Le plan est ordonné par information acquise : les expériences les moins coûteuses éliminent d'abord les hypothèses fragiles, avant l'entraînement d'un grand modèle.

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

Aucun de ces jalons ne construit le complexe de Čech global ni une mosaïque de Delaunay d'ordre supérieur. Les comparaisons causales P0–P7 du protocole isolent `support seul`, `complexe seul`, `support + complexe`, accès à $\Gamma_K^{\mathrm{elem}}$ avec tokens précalculés, sac de tokens sans messages, mutant invalide et support + radial. P2 et P3 conservent exactement les mêmes `payload_kind`, `carrier_kind`, `authority` et coupe. Lorsque les sommets du carrier source/PL sont disponibles, le support source est calculable et tout gain P3 sur P2 est un shortcut d'optimisation ; cette redondance n'est jamais transférée au support de $W_v(a_v)$.

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
- `QC-HSA`, projection conditionnée par la feuille décrite dans [THEOREM_PROGRAM.md](THEOREM_PROGRAM.md) ;
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
| semaines 4–7 | WP2 en parallèle | décision support/complexe/carrier et coût d'incidence |
| semaines 8–11 | WP3 | effet propre de l'arbre |
| semaines 12–16 | WP4 | effet propre de HSA et profil système |
| semaines 17–21 | WP5 | modèle sémantique verrouillé, stress tests |
| semaines 22–26 | WP6 | second dataset et décision de soumission |

Une porte négative raccourcit le programme : elle déclenche le pivot associé au lieu d'ajouter des composants. La phase instance possède son propre calendrier et n'est pas incluse ici.
