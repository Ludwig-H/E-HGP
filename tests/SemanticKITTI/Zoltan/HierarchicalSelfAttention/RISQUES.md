# Risques et réfutations

Ce document ne recense pas tout ce qui pourrait mal tourner. Il isole les échecs capables d'invalider la thèse scientifique ou de rendre le système impraticable.

## 1. Carte des risques

| ID | Risque | Gravité | Test précoce | Réponse |
|---|---|---:|---|---|
| R1 | l'invariance à la portée n'existe pas sur les données réelles | critique | G2 | réduire ou abandonner le claim |
| R2 | les facettes détruisent les frontières sémantiques | critique | G1 | raffiner les feuilles |
| R3 | le modèle sans points n'apprend pas assez de texture locale | critique | G3 | enrichir les canaux, puis comparer à l'hybride |
| R4 | la hiérarchie n'apporte rien au-delà d'un graphe de régions | critique | G4 | publication recentrée ou arrêt |
| R5 | les canaux capteur deviennent des raccourcis | élevé | G5 | dropout, permutation et adversarial probe |
| R6 | les arbres des deux vues ne sont pas appariables | élevé | G2/G6 | JEPA intra-arbre ou matching partiel |
| R7 | trop de feuilles ou de fusions de haut degré | élevé | audit système | compression contrôlée / inducing tokens |
| R8 | le repère local est instable | élevé | tests synthétiques | invariants + masques de dégénérescence |
| R9 | le prétraitement interdit les augmentations utiles | moyen | audit data loader | reconstruire la structure ou limiter l'augmentation |
| R10 | l'opérateur sophistiqué masque l'effet de la représentation | moyen | ablations appariées | ordre strict des modèles |
| R11 | le gain dépend d'un split, d'une graine ou d'une recette | élevé | trois graines + transfert | réduire le claim |
| R12 | le coût end-to-end annule l'intérêt | élevé | profiling G0–G3 | cache, sérialisation et simplification |

## 2. R1 — Invariance seulement idéale

Sous une dilution homogène, multiplier la densité par une constante reparamètre les niveaux sans changer l'arbre idéal. Un LiDAR réel applique plutôt une modulation spatiale :

```math
q(x) = q(R(x),\theta(x),\text{anneau},\text{matériau},\text{occultation}).
```

La topologie peut donc changer, notamment aux extrémités fines et derrière les occultations.

### Test

Comparer scan original et vues dégradées sous quatre familles distinctes :

1. thinning uniforme ;
2. thinning dépendant de la portée ;
3. suppression d'anneaux ;
4. occultation angulaire.

Mesurer le matching des nœuds, les relations ancêtre–descendant et la persistance, pas seulement une corrélation globale de descripteurs.

### Décision

- stabilité sur les quatre familles : claim d'invariance approchée défendable ;
- stabilité uniquement au thinning uniforme : claim limité à l'équivariant d'échantillonnage ;
- instabilité générale : abandonner l'argument central avant d'entraîner un gros modèle.

## 3. R2 — Plafond de résolution insuffisant

Une prédiction par facette ne peut pas séparer deux classes qui traversent systématiquement la même feuille. Aucun Transformer ne répare une information supprimée en amont, même avec beaucoup de têtes et une figure particulièrement colorée.

### Test

Oracle de facettes : attribuer à chaque feuille sa distribution GT exacte, reprojeter et évaluer mIoU et frontières.

### Atténuations, dans cet ordre

1. conserver toutes les feuilles élémentaires plutôt qu'une coupe grossière ;
2. raffiner uniquement les feuilles mixtes selon une règle sans label à l'inférence ;
3. utiliser plusieurs ordres ou plusieurs structures de feuilles ;
4. ajouter un décodeur de sous-facettes ;
5. en dernier recours, ouvrir une variante hybride comme plafond.

Une subdivision apprise avec les labels dans le prétraitement est interdite pour le modèle principal.

## 4. R3 — Perte d'information locale

Des descripteurs fixes peuvent manquer :

- une texture de rémission fine ;
- un motif d'échantillonnage discriminant ;
- la courbure interne d'une grande feuille ;
- des détails non captés par l'enveloppe convexe.

### Test

Comparer à budget égal :

- moments et Gram ;
- fonction support ;
- CDF/quantiles projetés ;
- petit Deep Sets sur les sommets, utilisé **hors backbone point-wise** ;
- encodeur d'incidences.

Le petit Deep Sets est admissible comme encodeur de cellule si sa sortie seule est conservée et si aucune feature par point ne circule entre les cellules. Il teste si les canaux analytiques, et non le paradigme polyèdre-only, constituent le goulot.

## 5. R4 — Hiérarchie inutile

Un graphe dual local peut déjà suffire. Un gain obtenu par davantage de couches ou de paramètres ne prouve rien sur l'arbre.

### Contrôles obligatoires

- graphe dual seul ;
- même modèle avec arbre aléatoire apparié ;
- octree ;
- HDBSCAN/RSL complet ;
- niveaux permutés ;
- hiérarchie réelle.

Les distributions de profondeur, degré, nombre de nœuds et budget de calcul doivent être appariées autant que possible.

### Décision

Si l'arbre réel n'améliore ni segmentation ni robustesse, le modèle peut rester utile comme réseau de régions, mais la contribution hiérarchique tombe.

## 6. R5 — Raccourcis capteur

Portée, anneau et rémission sont corrélés aux classes dans SemanticKITTI. Un réseau peut gagner en validation en mémorisant ces corrélations puis échouer sur un autre capteur.

### Défenses

- séparation explicite du canal `sensor` ;
- channel dropout pendant l'entraînement ;
- permutation du canal entre scans comme null test ;
- probe linéaire de la portée, de l'anneau et du taux de thinning ;
- adversarial head avec gradient reversal en ablation ;
- transfert vers un second capteur.

Le but n'est pas de supprimer toute information capteur, ce qui serait artificiel, mais de vérifier qu'elle ne porte pas seule le résultat.

## 7. R6 — Matching teacher–student biaisé

En `Range-Hierarchy JEPA`, les deux vues ont des arbres différents. Ne garder que les nœuds faciles à apparier peut sélectionner les grandes surfaces proches et ignorer exactement les petits objets difficiles.

### Mesures obligatoires

- taux d'appariement par classe, portée, masse et dimension intrinsèque ;
- histogramme des scores de Weighted Jaccard ;
- taux de rejet ;
- couverture des classes fines ;
- comparaison matching symétrique / asymétrique.

### Plans de repli

1. loss uniquement sur les facettes survivantes et leurs ancêtres partiels ;
2. transport optimal sparse entre ensembles de feuilles ;
3. teacher et student sur le même arbre, avec masquage d'attributs plutôt que reconstruction indépendante ;
4. pré-entraînement intra-arbre sans claim d'invariance inter-vues.

Le plan 3 est moins pur mais beaucoup plus diagnostiquable.

## 8. R7 — Explosion combinatoire et degré élevé

Le nombre de facettes peut dépasser le nombre de points. Les fusions simultanées peuvent également créer des familles très larges, rendant l'attention entre frères quadratique.

### Contrat système

Rapporter pour chaque scan :

```text
N_points, N_leaves, N_nodes, N_lateral_edges,
max_degree, P95_degree, tree_depth, bytes_on_disk.
```

### Réponses possibles

- stockage global des cellules et références par nœud ;
- deltas entre niveaux, jamais copie complète par ancêtre ;
- attention parent–enfants linéaire ;
- un petit nombre d'`inducing tokens` pour les fratries de haut degré ;
- découpage par composante racine ;
- gradient checkpointing et batching par budget de tokens.

La binarisation arbitraire d'une fusion simultanée est déconseillée : elle invente un ordre absent de la filtration et peut créer un faux signal positionnel.

## 9. R8 — Repères locaux instables

Un repère PCA change de signe et permute ses axes lorsque deux valeurs propres sont proches. Une petite perturbation du scan peut donc produire une grande variation du token.

### Baseline retenue

- invariants de Gram et rapports spectraux ;
- directions globales : gravité, radial capteur, tangente azimutale ;
- masques de dégénérescence lorsque les axes ne sont pas définis.

La PCA orientée n'est qu'une ablation. Toute règle de signe doit être déterministe et testée près des cas dégénérés.

## 10. R9 — Prétraitement et augmentations incompatibles

Une rotation rigide transporte la structure. Un crop, un elastic distortion ou un dropout de points peuvent la changer.

### Règle

Chaque augmentation appartient à une classe explicite :

- `transportable` : topologie conservée, attributs transformés analytiquement ;
- `rebuild_required` : structure reconstruite ;
- `forbidden_main` : incompatible avec le claim ou trop coûteuse.

Ne jamais appliquer un crop après prétraitement en prétendant que la forêt restreinte est la hiérarchie exacte du crop.

## 11. R10 — Mauvais ordre des innovations

Le risque expérimental classique consiste à changer en même temps :

- la tokenisation ;
- les canaux ;
- l'opérateur ;
- la loss ;
- les augmentations.

Un échec devient alors parfaitement inexpliqué, cette forme très contemporaine de connaissance négative.

### Ordre imposé

1. oracle et stabilité ;
2. MLP de feuilles ;
3. graphe dual ;
4. `PolyTreeFormer-Nano` supervisé ;
5. modèle d'arbre complet ;
6. JEPA ;
7. HSA, AllSet et multi-ordre.

## 12. R11 — Surapprentissage expérimental

SemanticKITTI ne contient qu'une séquence de validation standard. Des ajustements répétés sur `08` deviennent un entraînement indirect.

### Défenses

- sous-splits internes dans les séquences d'entraînement pour le développement ;
- `08` réservée aux jalons gelés ;
- trois graines ;
- différences appariées par scan ;
- second dataset avant claim général ;
- test caché après gel complet uniquement.

## 13. R12 — Coût end-to-end

Un réseau compact peut être inutile si la construction géométrique prend plusieurs secondes ou si la structure occupe des centaines de mégaoctets par scan.

### Mesure

Séparer :

1. construction hors ligne ;
2. sérialisation ;
3. lecture disque ;
4. transfert GPU ;
5. forward ;
6. reprojection.

Deux régimes doivent être annoncés honnêtement :

- **offline hierarchy** pour l'étude scientifique ;
- **online end-to-end** seulement lorsque le prétraitement est intégré et profilé.

## 14. Critères d'abandon du paradigme strict

La voie polyèdre-only stricte doit être arrêtée ou reléguée si deux des conditions suivantes sont observées après réglages raisonnables :

- oracle à moins de `5` points du meilleur modèle appris ;
- instabilité forte dès un thinning `1/4` ;
- écart supérieur à `8` points avec une baseline point-wise forte ;
- absence de gain de la hiérarchie sur le graphe dual ;
- coût mémoire supérieur au nuage brut sans gain mesurable ;
- transfert fortement négatif vers un second capteur.

Dans ce cas, la variante hybride devient un diagnostic, pas une manière discrète de renommer le même projet.
