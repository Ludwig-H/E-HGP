# Protocole expérimental

## 1. Hypothèse de travail

La hiérarchie HGP est supposée disponible, exacte au niveau déclaré et assez peu coûteuse pour ne pas être le verrou du projet. Le protocole commence **après** sa construction.

L'objet étudié est donc :

```text
hiérarchie de surfaces polyédriques HGP
        + niveaux de densité
        + incidences et trajectoires
        + reprojection vers les retours LiDAR
```

Le but n'est pas seulement d'obtenir un bon score SemanticKITTI. Il faut déterminer si cette structure fournit un meilleur **alphabet géométrique** que les points, voxels et superpoints, puis si cet alphabet peut soutenir un pré-entraînement transférable.

## 2. Questions causales

Le programme répond dans cet ordre à six questions :

1. **surface** : les polyèdres conservent-ils la géométrie observée et les frontières sémantiques ?
2. **tokenizer** : quelle représentation fixe de la surface offre le meilleur taux–distorsion ?
3. **robustesse** : le token polyédrique est-il plus stable sous rééchantillonnage et changement de capteur ?
4. **hiérarchie** : les trajectoires et fusions HGP ajoutent-elles de l'information au polyèdre isolé ?
5. **auto-supervision** : le pré-entraînement cross-range améliore-t-il probing, faible supervision et robustesse ?
6. **fondation** : le même encodeur transfère-t-il entre datasets, capteurs et tâches sans modifier le tokenizer ?

Chaque porte a une conclusion `continue`, `revise` ou `stop`. Une étape ultérieure ne sert pas à maquiller l'échec d'une étape antérieure avec davantage de paramètres, cette vieille coutume de la discipline.

## 3. Régimes d'évaluation

| Track | Pré-entraînement | Inférence | Claim permis |
|---|---|---|---|
| A — SemanticKITTI strict | SemanticKITTI seulement | mono-scan, LiDAR seul | backbone spécialisé |
| B — LiDAR multi-datasets | LiDAR non annotés multiples | mono-scan, LiDAR seul | fondation LiDAR de domaine |
| C — temporel | séquences et poses | mono-scan au test | invariance temporelle apprise |
| D — multimodal | images / vision-language au train | LiDAR seul ou multimodal déclaré | transfert sémantique / open vocabulary |
| E — TTA / ensemble | quelconque | augmenté | contexte leaderboard séparé |

Les résultats principaux n'agrègent jamais des tracks différents dans une même ligne.

## 4. Données et splits

### SemanticKITTI

- train : séquences `00–07, 09, 10` ;
- validation finale : séquence `08` ;
- test : `11–21` après gel ;
- développement : sous-splits internes des séquences d'entraînement ;
- 19 classes, sortie point-wise officielle ;
- résultat strict sans TTA, ensemble, caméra ni accumulation temporelle.

### Transfert minimal

- nuScenes-lidarseg ou autre capteur outdoor ;
- un troisième jeu si le claim devient « modèle de fondation LiDAR » ;
- mêmes dimensions, mêmes bases de surface et mêmes règles de normalisation ;
- recalibration statistique non supervisée autorisée, adaptation du tokenizer par dataset interdite dans le résultat principal.

### Tâches minimales pour un claim fondation

- segmentation sémantique ;
- segmentation d'instance ou panoptique ;
- détection ou localisation ;
- recherche de régions / retrieval cross-range ;
- une tâche géométrique : complétion, reconstruction ou estimation de frontières.

## 5. Porte G0 — intégrité géométrique

### Vérifications

Pour chaque polyèdre :

- facettes valides, aire positive et attributs finis ;
- incidences et bords cohérents ;
- surface identique après permutation des identifiants ;
- masse de mesure cohérente avec l'aire normalisée ;
- orientation des normales déclarée ou remplacée par un projecteur non orienté ;
- poids de reprojection positifs sommant à un ;
- ordre des points restauré exactement ;
- deltas de facettes cohérents le long des branches lorsqu'ils sont fournis.

### Tests synthétiques

- plan ouvert ;
- sphère triangulée ;
- tore ou surface concave ;
- deux couches parallèles ;
- surface non-manifold contrôlée ;
- même support avec plusieurs remeshings.

**Arrêt** si le descripteur dépend de l'ordre des facettes, de l'orientation arbitraire des normales ou d'une triangulation particulière sans que cette dépendance soit explicitement voulue.

## 6. Porte G1 — validité des représentations de surface

### 6.1 Diagnostic radial

Mesurer pour chaque ancre :

- couverture `C(c)` ;
- unicité `U(c)` ;
- histogramme de multiplicité `N_c(u)` ;
- masse d'aire en régime `N=0`, `N=1`, `N≥2` ;
- sensibilité de ces quantités à l'ancre et au thinning.

Ce diagnostic décide seulement si la carte radiale simple est admissible. Il ne décide pas de la validité du paradigme polyédrique.

### 6.2 Compétition du tokenizer

À dimension latente, paramètres et données comparables :

| ID | Représentation |
|---|---|
| P0 | moments, covariance, shape distributions |
| P1 | carte radiale monocouche |
| P2 | carte radiale `K` couches |
| P3 | moments sphéro-radiaux / Zernike-like |
| P4 | grille douce de mesure sphéro-radiale |
| P5 | P4 multi-ancre |
| P6 | quadrature d'aire + Set Encoder |
| P7 | atlas multi-cartes |
| P8 | SurfaceGraph / PolyhedronNet-like |

### 6.3 Métriques de fidélité

- Chamfer symétrique pondérée par l'aire ;
- Hausdorff et quantiles `95/99 %` ;
- cohérence des normales ;
- rappel et précision des bords ;
- erreur de masse par cellule ;
- erreur de reconstruction des surfaces multicouches ;
- invariance au remeshing ;
- sensibilité à l'ancre ;
- octets, FLOPs, VRAM et latence.

Tracer :

```math
\text{distorsion}
\quad\text{en fonction de}\quad
\text{octets, tokens et FLOPs}.
```

**Succès.** La représentation retenue doit appartenir à la frontière de Pareto et ne pas échouer systématiquement sur les classes fines ou les surfaces ouvertes.

**Révision.** Si la grille compacte perd surtout la connectivité, ajouter le résidu `SurfaceGraph`.

**Repli.** Si elle reste dominée, le backbone devient mesh-native. Le paradigme polyédrique survit ; la revendication d'un code sphéro-radial supérieur tombe.

## 7. Porte G2 — plafond sémantique des polyèdres

### Oracles

1. logits parfaits par facette ;
2. logits parfaits par polyèdre à plusieurs niveaux ;
3. meilleur choix par point parmi les ancêtres disponibles ;
4. oracle de branche persistante ;
5. oracle avec et sans bords de facettes.

Rapporter :

- mIoU global et par classe ;
- F-score de frontière ;
- résultats par portée ;
- fraction d'éléments mixtes ;
- ambiguïté de reprojection ;
- courbe oracle contre nombre de polyèdres et mémoire.

**Succès.** L'oracle doit garder une marge substantielle au-dessus de la cible apprise et ne pas détruire les petites classes. Une marge de `8–10` points de mIoU est un objectif de sécurité raisonnable, pas une loi cosmique.

**Arrêt ou révision de la sortie** si les polyèdres mélangent irrémédiablement des classes que le décodeur par facette ne peut séparer.

## 8. Porte G3 — stabilité à la portée et au capteur

### 8.1 Vues contrôlées

Construire des acquisitions de même scène sous :

- thinning uniforme `1, 1/2, 1/4, 1/8` ;
- suppression d'anneaux ;
- thinning dépendant de la portée ;
- changement de résolution azimutale ;
- occultation par secteurs ;
- bruit de mesure et rémission ;
- remeshing contrôlé de la même surface.

### 8.2 Vues réelles

À partir des séquences :

- compensation de l'ego-motion ;
- retrait ou traitement séparé des objets mobiles ;
- appariements de surfaces observées à plusieurs portées ;
- comparaison cross-capteur lorsque disponible.

### 8.3 Métriques

Pour des paires positives `(i,i')` et négatives `(i,j)` :

```math
I_{\mathrm{range}}
=
1-
\frac{\mathbb E\,d(z_i,z_{i'})}
{\mathbb E\,d(z_i,z_j)}.
```

Ajouter :

- Recall@1/5 en retrieval cross-range ;
- précision de matching des polyèdres ;
- stabilité des mesures et des embeddings ;
- conservation des relations ancêtre–descendant ;
- corrélation des persistances ;
- performance par portée et taux de thinning ;
- probe sémantique ;
- probes portée, anneau, capteur et thinning.

### Baselines

- retours bruts sous encodeur point moderne ;
- voxels / sparse convolution ;
- superpoints SPT ;
- quadrature d'aire sans hiérarchie ;
- polyèdres avec descripteurs analytiques ;
- polyèdres avec tokenizer retenu.

**Succès central.** À budget comparable, le token polyédrique doit améliorer soit le retrieval cross-range, soit la pente de performance avec la distance, soit le compromis sémantique–invariance, sur plusieurs classes et pas uniquement les grandes surfaces planes.

L'invariance à l'homothétie mathématique et la robustesse à la portée sont rapportées séparément. Les confondre produirait une jolie équation et une mauvaise conclusion.

## 9. Porte G4 — apprenabilité du polyèdre isolé

Entraîner :

1. `Analytic-MLP` ;
2. `Radial-Flat` ;
3. `Measure-Flat` ;
4. `SurfaceGraph` ;
5. `Measure+Topo` ;
6. baseline point/voxel forte.

Régimes :

- supervised full ;
- linear probe sur autoencodeur de surface ;
- `0.1 %`, `1 %`, `10 %` des labels ;
- cross-dataset frozen probe.

**Succès.** `Measure+Topo` doit battre les descripteurs analytiques et être compétitif avec les représentations de points à budget de tokens comparable. Un écart complet peut rester acceptable si l'avantage cross-range et low-shot est net ; un modèle inférieur partout n'est pas une révolution incomprise, seulement un modèle inférieur.

## 10. Porte G5 — effet propre de la hiérarchie HGP

À tokenizer, paramètres et budget appariés :

| Bras | Structure |
|---|---|
| H0 | polyèdres indépendants |
| H1 | graphe latéral seulement |
| H2 | arbre aléatoire apparié en profondeur, degrés et masses |
| H3 | octree / voxel tree |
| H4 | HDBSCAN/RSL |
| H5 | niveaux HGP permutés |
| H6 | topologie HGP sans valeurs de niveau |
| H7 | trajectoires HGP complètes |
| H8 | H7 + événements + graphe latéral |

Ablations spécifiques :

- états complets contre deltas de facettes ;
- branche contre quatre coupes ;
- rangs contre `Δlogλ` ;
- fusion moyenne contre Set Attention ;
- HSA, Sequoia-fixed et `MeanTree` à coût apparié.

**Succès.** La hiérarchie réelle doit améliorer au moins deux axes parmi :

- mIoU ;
- performance lointaine ;
- faible supervision ;
- robustesse au thinning ;
- instance / panoptique ;
- qualité–coût.

Si `H6≈H7`, les valeurs de densité n'apportent rien. Si `H4≈H7`, une hiérarchie générique suffit. Si `H1≈H8`, le graphe spatial explique tout. Ces résultats restent publiables comme réfutation, mais pas sous le claim prévu.

## 11. Porte G6 — pré-entraînement

### Bras

| ID | Objectif |
|---|---|
| S0 | from scratch |
| S1 | reconstruction de surface |
| S2 | Surface-JEPA même polyèdre |
| S3 | prédiction parent / innovations |
| S4 | Cross-Range PolyJEPA simulé |
| S5 | S4 + séquences réelles |
| S6 | S5 + distillation 2D |
| S7 | S6 + prototypes / langage |

### Évaluation

- frozen linear probing ;
- fine-tuning `0.1 %`, `1 %`, `10 %`, `100 %` ;
- retrieval cross-range ;
- robustesse corruption ;
- calibration ;
- transfert de dataset ;
- trois graines pour toute conclusion principale.

### Matching audit

Rapporter par classe, portée, aire, persistance et multiplicité :

- taux d'appariement ;
- précision estimée ;
- score moyen ;
- taux de rejet ;
- contribution à la loss.

**Succès.** Le pré-entraînement doit améliorer clairement le probing ou le faible régime de labels sur au moins deux datasets, tout en augmentant la stabilité cross-range. Un gain uniquement en fine-tuning complet peut relever d'une meilleure initialisation, pas d'une représentation fondation.

## 12. Porte G7 — statut fondation

Le terme `foundation model` n'est ouvert que si le même encodeur :

- est pré-entraîné sur plusieurs datasets et capteurs ;
- garde le même tokenizer ;
- transfère vers plusieurs tâches ;
- améliore frozen probing et faible supervision ;
- bénéficie de l'augmentation du volume de pré-entraînement ;
- reste compétitif face aux pré-entraînements points/voxels modernes.

Tracer des courbes de scaling :

```math
\operatorname{Perf}(N_{\rm scans}),
\qquad
\operatorname{Perf}(N_{\rm capteurs}),
\qquad
\operatorname{Perf}(N_{\rm tâches}).
```

Un modèle uniquement fine-tuné sur SemanticKITTI est nommé `HGP-PolyFM backbone`, pas modèle de fondation. Les mots sont gratuits, les preuves beaucoup moins.

## 13. Plan factoriel de l'apport

La table principale doit isoler :

| Facteur | Contrôle | Proposition |
|---|---|---|
| primitive | points / superpoints | surfaces polyédriques |
| code local | moments / mesh graph | mesure surfacique compacte |
| structure | modèle plat | espace d'échelle HGP |
| objectif | supervisé / SSL standard | Cross-Range PolyJEPA |
| contexte | local | branches + fusions + latéral |

Estimer les effets :

```math
\Delta_{\rm poly},
\quad
\Delta_{\rm measure},
\quad
\Delta_{\rm HGP},
\quad
\Delta_{\rm SSL},
```

et les interactions importantes :

```math
\Delta_{\rm poly\times HGP},
\qquad
\Delta_{\rm HGP\times SSL}.
```

Les budgets sont appariés par paramètres, FLOPs, octets d'entrée et nombre de tokens. Aucun unique appariement n'est parfait ; rapporter les quatre évite de choisir celui qui flatte la méthode.

## 14. Métriques système

Même si la hiérarchie HGP est supposée peu coûteuse, le modèle doit rapporter :

- nombre de polyèdres, facettes, événements et arêtes ;
- taille de la grille de surface ;
- temps de tokenization de surface ;
- temps du `SurfaceEncoder`, de l'arbre et du décodeur ;
- VRAM / RAM ;
- débit en scans, polyèdres et facettes par seconde ;
- taille disque ;
- latence end-to-end avec et sans cache HGP.

Le coût de construction HGP n'est pas un critère d'arrêt dans ce dossier, mais il n'est pas effacé des tableaux publics.

## 15. Statistiques

- trois graines pour les résultats principaux ;
- moyenne, écart-type et différences appariées par scan ;
- bootstrap par séquence ou scène pour les métriques de stabilité ;
- intervalles de confiance des gains ;
- sous-splits de développement internes ;
- `08` réservée aux jalons gelés ;
- serveur test utilisé après gel complet seulement.

## 16. Tableau de décision

| Observation | Conclusion |
|---|---|
| oracle faible | sortie polyédrique insuffisante, raffiner ou arrêter |
| radialité faible mais mesure compacte forte | abandon de `ρ(u)`, paradigme conservé |
| mesure compacte dominée par SurfaceGraph | backbone mesh-native |
| polyèdres stables mais sémantique faible | tokenizer géométrique, pas perceptif |
| polyèdres > superpoints en robustesse seulement | papier robustesse / transfert, pas SOTA pur |
| arbre HGP = arbre aléatoire | hiérarchie non démontrée |
| arbre HGP > flat mais niveaux inutiles | topologie utile, calibration de densité non démontrée |
| SSL améliore probing sur plusieurs capteurs | représentation générale crédible |
| gain sur un seul dataset | claim dataset-spécifique |
| multi-dataset + multi-tâche + scaling | statut fondation défendable |
