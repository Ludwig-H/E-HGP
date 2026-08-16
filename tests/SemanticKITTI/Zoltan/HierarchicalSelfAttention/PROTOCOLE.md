# Protocole expérimental

## 1. Tâche principale

Segmentation sémantique SemanticKITTI à partir d'un **scan unique**, LiDAR seul.

- entrée : `(x, y, z, remission)` et attributs déterministes dérivés ;
- train : `00–07, 09, 10` ;
- validation : `08` ;
- test : `11–21`, seulement après gel complet ;
- 19 classes après fusion moving/non-moving ;
- métrique principale : mIoU point-wise officielle ;
- résultat principal : sans TTA, ensemble, caméra ni contexte temporel.

La construction de la hiérarchie n'utilise jamais les labels.

## 2. Régimes à ne pas mélanger

| Track | Données d'entraînement | Inférence |
|---|---|---|
| A — strict | SemanticKITTI uniquement | un scan, LiDAR seul |
| B — pré-entraînement externe | autres LiDAR non annotés autorisés | un scan, LiDAR seul |
| C — multimodal ou temporel | caméra, poses ou séquences | régime séparé |
| D — TTA / ensemble | quelconque | régime séparé |

Un chiffre n'est comparable que si son track, ses augmentations de test et ses données sont explicités.

## 3. Préparation des données

Pour chaque scan :

1. construire la structure polyédrique sans labels ;
2. sérialiser feuilles, nœuds, arêtes et attributs ;
3. sérialiser les poids facette→point ;
4. enregistrer un hash du scan, des paramètres et du schéma ;
5. produire les mêmes structures de contrôle : HDBSCAN/RSL, octree, arbre aléatoire apparié.

Les fichiers sont immuables pendant une campagne. Toute modification du prétraitement crée une nouvelle version de dataset.

## 4. Portes de validation

### G0 — intégrité du contrat

Tests sur tous les scans :

- aucune incidence invalide ;
- forêt acyclique ;
- ordre des points restauré exactement ;
- conservation de masse à `1e-5` relatif ;
- invariance à la permutation ;
- aucune statistique non finie ;
- points non couverts comptés et rapportés.

**Arrêt** si une correction heuristique dépend de l'ordre d'itération ou des labels.

### G1 — oracle de représentation

Attribuer à chaque facette sa distribution GT, reprojeter, puis calculer le mIoU. Répéter pour plusieurs granularités.

Rapporter :

- mIoU oracle global ;
- IoU oracle des 19 classes ;
- F-score de frontière ;
- résultats par portée ;
- fraction de facettes mixtes ;
- ambiguïté de reprojection.

**Règle d'arrêt.** L'oracle retenu doit dépasser le meilleur score point-wise visé d'au moins `10` points de mIoU et ne perdre aucune classe fine de manière catastrophique. Sinon, le problème est la tokenisation, pas le Transformer.

### G2 — stabilité sous densité

Créer des vues à taux de conservation approximatifs `1, 1/2, 1/4, 1/8`, avec :

- thinning uniforme ;
- suppression d'anneaux ;
- thinning dépendant de la portée ;
- occultation par secteurs ;
- corruption SemanticKITTI-C pertinente.

Mesurer :

- rappel et précision des nœuds appariés ;
- Weighted Jaccard moyen ;
- conservation des relations ancêtre–descendant ;
- corrélation des persistances ;
- variation de profondeur et de branchement ;
- distance entre embeddings analytiques ;
- dispersion inter-scan des courbes niveau–masse.

Comparer à HDBSCAN/RSL, octree et partitions SPT.

**Règle d'arrêt.** La structure proposée doit dominer au moins un contrôle pertinent avec intervalle bootstrap excluant zéro, et ne pas s'effondrer au thinning `1/4`. Une invariance visible uniquement à `1/2` sur les routes proches ne justifie pas le projet.

### G3 — apprenabilité polyèdre-only

Entraîner successivement :

1. MLP sur les facettes ;
2. Transformer du graphe dual ;
3. `PolyTreeFormer-Nano` ;
4. modèle sur arbre complet.

**Règle d'arrêt.** Le modèle hiérarchique doit :

- améliorer le graphe dual sur trois graines ;
- rester à moins de `5` points d'une baseline point-wise forte reproduite dans le même régime lors du pilote ;
- ne pas dégrader systématiquement les classes fines.

Un écart supérieur à `8` points après réglages raisonnables rend la route SOTA polyèdre-only très improbable.

### G4 — effet propre de la hiérarchie

À architecture et budget appariés :

| Bras | Structure |
|---|---|
| H0 | aucune hiérarchie, graphe dual seul |
| H1 | arbre aléatoire conservant profondeur et degrés |
| H2 | octree / voxel tree |
| H3 | HDBSCAN/RSL complet |
| H4 | hiérarchie proposée, niveaux permutés |
| H5 | hiérarchie proposée complète |

**Succès.** `H5` doit battre `H1–H4` en moyenne sur trois graines et améliorer la robustesse sous thinning. Si `H3 = H5`, la hiérarchie générique suffit ; la revendication doit être réduite en conséquence.

### G5 — utilité des canaux

Ablations cumulatives :

1. forme normalisée ;
2. `+` grandeurs physiques ;
3. `+` filtration relative ;
4. `+` acquisition ;
5. support remplacé par moments/CDF ;
6. niveaux relatifs remplacés par niveaux bruts ;
7. canaux capteur permutés entre scans.

**Succès central.** Le canal de filtration relative doit améliorer soit le mIoU, soit la robustesse à densité à budget identique. Si seuls la portée et la rémission expliquent le gain, l'hypothèse scientifique n'est pas confirmée.

### G6 — pré-entraînement

Comparer :

| Bras | Objectif |
|---|---|
| S0 | entraînement supervisé from scratch |
| S1 | masked reconstruction de descripteurs |
| S2 | feature regression teacher–student |
| S3 | Range-Hierarchy JEPA |
| S4 | S3 + événements |
| S5 | softmaps observables |

Évaluer :

- linear probing ;
- fine-tuning à `0.1 %`, `1 %`, `10 %`, `100 %` des labels ;
- trois graines en supervision complète ;
- robustesse aux vues amincies.

**Règle de continuation.** Le pré-entraînement retenu doit produire au moins `+0.5` mIoU en probing ou fine-tuning préliminaire, puis un gain moyen positif avec intervalle de confiance raisonnable sur trois graines. Pour une revendication SOTA, viser `+1` point robuste au-dessus de la meilleure recette reproduite.

### G7 — transfert

Rejouer au minimum sur nuScenes ou un autre capteur outdoor.

- même architecture ;
- mêmes canaux ;
- niveaux recalibrés sans labels ;
- protocole de thinning comparable.

Sans transfert, la conclusion doit rester limitée à SemanticKITTI.

## 5. Métriques

### Segmentation

- mIoU et IoU des 19 classes ;
- mAcc et accuracy globale en secondaire ;
- matrice de confusion ;
- thing/stuff ;
- F-score de frontière sur graphe 16-NN diagnostique ;
- NLL, Brier et ECE.

### Strates obligatoires

- portée : `0–10`, `10–20`, `20–30`, `30–40`, `40–50`, `>50 m` ;
- classes fines : `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist`, `fence` ;
- linéarité, planarité et diffusion locale ;
- masse et degré des nœuds ;
- profondeur de la hiérarchie ;
- taux de thinning ;
- pureté GT des nœuds, uniquement pour diagnostic.

### Stabilité et raccourcis

- capacité d'un probe linéaire à prédire portée, anneau et taux de thinning depuis les embeddings ;
- invariance des embeddings de branches appariées ;
- taux de correspondances rejetées ;
- sensibilité à la permutation des niveaux ;
- performance avec canal `sensor` masqué.

### Système

- paramètres, FLOPs/MACs avec convention ;
- VRAM et RAM maximales ;
- temps P50/P95 ;
- prétraitement, chargement, forward et reprojection séparés ;
- débit en scans/s et tokens/s ;
- `N_leaf`, `N_node`, `N_edge` et distributions de degrés ;
- taille disque de la structure ;
- latence end-to-end incluant la hiérarchie.

## 6. Baselines

### Point-wise

- PTv3 ou backbone moderne reproduit ;
- DOS avec le même jeu de pré-entraînement lorsque possible ;
- MinkUNet ou SPUNet comme contrôle sparse plus simple.

Ces modèles ne sont pas des composants de PolyTreeFormer. Ils mesurent le coût de renoncer aux tokens points.

### Région / hiérarchie

- SPT et SPT-nano ;
- graphe dual sans hiérarchie ;
- HDBSCAN/RSL ;
- octree ;
- arbre aléatoire apparié ;
- hiérarchie réelle avec niveaux permutés.

### Opérateurs

- mean/max + MLP ;
- message passing parent–enfants ;
- SPT attention ;
- Sequoia-fixed ;
- HSA ;
- attention locale de même budget sans arbre.

## 7. Statistiques et sélection

- trois graines pour toute conclusion principale ;
- moyenne, écart-type et différences appariées par scan ;
- bootstrap par scan pour métriques de stabilité ;
- hyperparamètres choisis uniquement sur train/val ;
- aucun balayage sur le serveur test ;
- une configuration gelée avant soumission.

Le meilleur run isolé est rapporté en annexe, jamais comme résultat principal. Les GPU ont déjà assez de privilèges sans leur accorder le droit de choisir la graine qui raconte l'histoire.

## 8. Tableau de décision final

| Observation | Conclusion |
|---|---|
| oracle faible | raffiner les feuilles ou arrêter |
| stabilité faible | abandonner le claim d'invariance |
| modèle nano faible, oracle fort | problème d'encodeur/canaux |
| arbre réel = arbre aléatoire | contexte global générique seulement |
| HDBSCAN = structure proposée | exactitude sans effet aval |
| SSL améliore probing mais pas fine-tuning | contribution représentation, pas SOTA |
| gain SemanticKITTI sans transfert | résultat dataset-spécifique |
| gain robuste + transfert + stabilité | dossier crédible pour conférence majeure |
