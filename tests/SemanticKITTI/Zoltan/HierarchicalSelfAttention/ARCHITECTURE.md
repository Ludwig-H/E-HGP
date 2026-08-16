# Architecture de référence

## 1. Objet appris

Le réseau reçoit une structure sparse sérialisée :

```text
leaves                 facettes élémentaires
leaf_dual_edges        adjacences géométriques entre facettes
nodes                  composantes persistantes / nœuds de fusion
parent, children       arbre ou forêt laminaire sur les facettes
node_lateral_edges     voisinages géométriques entre branches
point_leaf_index       incidences utilisées uniquement pour la sortie
point_leaf_weight      poids de reprojection conservant la masse
```

Le loader refuse toute scène pour laquelle :

- les facettes ne sont pas canoniquement ordonnées ;
- un indice est hors domaine ;
- un cycle apparaît dans l'arbre ;
- les masses parent–enfants ne se conservent pas à la tolérance déclarée ;
- les poids d'un point ne somment pas à un ;
- l'ordre de sortie ne correspond plus au fichier SemanticKITTI.

## 2. Représentation des cellules et des nœuds

### 2.1 Principe de séparation des canaux

Le token initial est la concaténation de quatre embeddings indépendants :

```math
z_v^0=
E_{\mathrm{shape}}(s_v)
+E_{\mathrm{metric}}(m_v)
+E_{\mathrm{filtration}}(f_v)
+E_{\mathrm{sensor}}(a_v).
```

Chaque famille possède son propre MLP, sa normalisation et son dropout. Cette séparation permet de mesurer les raccourcis et d'empêcher une normalisation de forme d'effacer la taille physique.

### 2.2 Canal `shape`

#### Cellule élémentaire

Pour une facette de sommets `x_1, …, x_r`, on centre puis on normalise par un rayon robuste `R` :

```math
X=[(x_1-c)/R,\ldots,(x_r-c)/R].
```

Entrées recommandées :

- partie triangulaire supérieure de `XᵀX` ou du Gram centré ;
- valeurs propres normalisées et leurs ratios ;
- longueurs d'arêtes normalisées, triées ;
- contenu dimensionnel normalisé ;
- masque de rang et masque de dégénérescence.

Le Gram est invariant à une rotation globale et à la permutation si les longueurs ou spectres sont triés. Les indices orientés ne sont utilisés que si leur orientation est contractuellement définie.

#### Nœud interne

Les descripteurs doivent être fusionnables depuis les enfants :

| Descripteur | Fusion | Information |
|---|---|---|
| support dur sur 42 directions | `max` | extrêmes et enveloppe convexe |
| CDF de projections sur 21 bins | somme des comptes | masse intérieure |
| quantiles sur 9 niveaux | dérivés de la CDF | résumé robuste |
| moments d'ordre 0, 1 et 2 | somme | masse, centre, covariance |
| occupation radiale ou conique | somme / min / max | épaisseur et distribution |

Le support est normalisé par une taille robuste avant encodage. Il ne doit jamais être présenté comme une représentation complète : deux objets non convexes distincts peuvent avoir le même support.

#### Repère directionnel

Trois options sont distinguées :

1. **directions fixes dans le repère ego** : stable et simple, mais dépend de l'orientation du véhicule ;
2. **repère gravité–radial–tangent** : recommandé pour le LiDAR outdoor ;
3. **repère PCA** : seulement comme ablation, avec règle de signe et masque lorsque les valeurs propres sont proches.

Le repère PCA est instable près des symétries. Une petite perturbation peut retourner un axe ou échanger deux directions, transformant une voiture en créature géométrique différente selon l'humeur des valeurs propres. Le modèle principal l'évite.

### 2.3 Canal `metric`

Ce canal conserve ce que l'invariance ne doit pas détruire :

- `log_diameter`, `log_extent_radial`, `log_extent_tangent`, `log_extent_vertical` ;
- aire ou volume proxy, avec masque lorsque la dimension ne le permet pas ;
- centre `(x, y, z)` normalisé par une constante de scène déclarée ;
- hauteur au-dessus d'un sol estimé hors labels ;
- normale ou direction principale exprimée par ses produits scalaires avec gravité et rayon capteur ;
- distance au capteur et azimut.

La taille physique aide à distinguer, par exemple, une façade d'un panneau. Elle est séparée de la forme normalisée pour permettre un dropout ou un gate spécifique.

### 2.4 Canal `filtration`

Les niveaux absolus sont sensibles à la densité. Le canal principal utilise des coordonnées relatives :

```math
\delta_v^{\uparrow}=\log \lambda_{p(v)}-\log\lambda_v,
```

```math
\operatorname{pers}(v)=
\log\lambda_{\mathrm{mort}}-
\log\lambda_{\mathrm{naissance}},
```

```math
r_v=\operatorname{rank}_{\mathrm{scan}}(\log\lambda_v),
\qquad
q_v=F_{\mathrm{scan}}(\log\lambda_v).
```

Ajouter :

- rapport `log(m_parent/m_v)` ;
- fraction de masse gagnée à l'événement ;
- nombre d'enfants et multiplicité du lot de fusion ;
- profondeur normalisée et distance à la racine ;
- indicateur de niveau exact ou approximé.

Le niveau brut est conservé dans un canal séparé et soumis à ablation. Une augmentation exacte ajoute une constante aléatoire à tous les `log λ` d'une scène ; le modèle doit rester invariant lorsque seuls les canaux relatifs sont actifs.

### 2.5 Canal `sensor`

Entrées recommandées :

- moyenne, écart-type et quantiles de rémission ;
- portée moyenne et dispersion de portée ;
- nombre d'anneaux distincts et étendue verticale ;
- occupation de bins azimut–élévation ;
- fraction de points/facettes survivants après augmentation ;
- incidence estimée entre normale locale et rayon ;
- masque de disponibilité.

Le canal `sensor` reçoit un **channel dropout** de `0.2–0.5` pendant l'entraînement. Les métriques de décodage vérifient qu'il n'est pas devenu un raccourci vers l'anneau ou la portée.

## 3. Encodage des relations

### 3.1 Arêtes latérales entre facettes ou nœuds

Pour une arête orientée `u → v`, utiliser :

- déplacement des centres dans le repère gravité–radial–tangent de `u` ;
- distance normalisée par la moyenne des diamètres ;
- angle entre normales ;
- rapport de tailles et de masses ;
- mesure ou compte d'interface partagée ;
- écart de support dans la direction `u → v` ;
- différence de niveau et de persistance ;
- type d'incidence.

Un MLP d'arête produit trois termes distincts :

```math
r^K_{uv},\quad r^Q_{uv},\quad r^V_{uv},
```

injectés respectivement dans les clés, requêtes et valeurs, comme dans SPT. Un simple biais scalaire est l'ablation minimale.

### 3.2 Arêtes verticales

Pour `v → p(v)` :

- position du centre de `v` relative au parent ;
- rapport de masse ;
- rapport de taille ;
- écart de niveau logarithmique ;
- persistance de l'enfant ;
- fraction d'interface avec chaque frère, si disponible.

Les positions relatives sont normalisées à l'intérieur de chaque famille d'enfants, tandis que les tailles physiques restent dans le canal `metric`.

### 3.3 Événements simultanés

Un lot de fusion de degré élevé est un **ensemble**, pas une séquence. Il est traité par :

- attention de set sur les enfants si le degré est inférieur au seuil `b_max` ;
- pooling par inducing tokens ou attention segmentée au-delà ;
- aucune binarisation arbitraire des égalités.

Une binarisation peut servir de contrôle système, mais elle ne doit pas modifier silencieusement le sens de la filtration.

## 4. Modèle A — `PolyTreeFormer-Nano`

### 4.1 But

Tester rapidement si une architecture sans tokens points peut apprendre sur SemanticKITTI. Le code de départ est SPT en mode `nano`.

### 4.2 Adaptation

- remplacer le `NAG` de superpoints par quatre antichaînes déterministes extraites de l'arbre ;
- prendre les facettes comme niveau le plus fin ;
- remplacer les features artisanales SPT par les quatre familles de canaux ;
- conserver les graphes latéraux et les arêtes verticales ;
- prédire au niveau des facettes ;
- remplacer le broadcast superpoint→point par la reprojection pondérée.

### 4.3 Choix des quatre niveaux

Les coupes ne sont pas choisies par nombre de clusters. Elles sont déterminées par quatre quantiles globaux de la coordonnée relative de filtration, fixés sur le train :

```text
L0 : facettes
L1 : avant la zone de croissance rapide
L2 : zone de susceptibilité maximale
L3 : juste après la transition principale
```

Cette vue compressée ne constitue pas la contribution finale ; elle sert à isoler la question de faisabilité.

### 4.4 Configuration initiale

```yaml
leaf_dim: 128
stage_dims: [128, 192, 256, 256]
blocks_down: [2, 2, 3, 3]
blocks_up: [2, 2, 2]
heads: [4, 6, 8, 8]
qk_dim_per_head: 16
ffn_ratio: 4
attention_dropout: 0.0
feature_dropout: 0.1
drop_path: 0.1
normalization: layer_norm
```

Contrairement au réglage historique très petit de SPT, un FFN est conservé. La largeur est bornée parce que la hiérarchie compresse fortement les données et peut surapprendre.

## 5. Modèle B — `PolyTreeFormer-Full`

### 5.1 But

Exploiter l'arbre complet des événements sans le réduire à quelques coupes.

### 5.2 Bloc de famille

Pour chaque nœud `p` :

1. **children-to-parent** : le parent interroge ses enfants ;
2. **sibling mixing** : les enfants échangent dans leur famille ;
3. **parent-to-children** : chaque enfant interroge le parent enrichi ;
4. **lateral mixing** : échanges avec quelques branches adjacentes ;
5. FFN pré-normalisé et résidus.

```math
h_p' = h_p + \operatorname{MHA}
\left(q=h_p,\,K=H_{\mathrm{ch}(p)},\,V=H_{\mathrm{ch}(p)}\right),
```

```math
H_{\mathrm{ch}(p)}' = H_{\mathrm{ch}(p)}
+\operatorname{SetMHA}
\left(H_{\mathrm{ch}(p)},H_{\mathrm{ch}(p)}\right),
```

```math
h_v'' = h_v' + \operatorname{MHA}
\left(q=h_v',\,K=[h_p'],\,V=[h_p']\right).
```

Les familles sont batchées par segments CSR. Le coût est `O(E d + Σ_p deg(p)^2 d)` ; au-delà de `b_max`, l'attention entre frères est remplacée par `m` inducing tokens, donnant `O(deg(p) m d)`.

### 5.3 Passes

- deux passes ascendantes ;
- deux passes descendantes ;
- skip direct des feuilles ;
- readout final par facette.

Une profondeur plus grande n'est ouverte qu'après mesure du receptive field effectif et du sur-lissage.

### 5.4 Trajectoire de branche

Entre deux fusions, une même composante peut porter plusieurs états de niveau. Ceux-ci forment une séquence courte encodée par un Transformer partagé :

```yaml
trajectory_dim: 192
trajectory_layers: 2
trajectory_heads: 6
max_states: 16
relative_position: delta_log_lambda
```

Les états sont choisis parmi les événements réels et, si nécessaire, quelques quantiles intermédiaires. Le token de branche transmis à l'arbre est le readout de cette trajectoire.

## 6. Modèles comparatifs

### 6.1 `MeanTree`

Somme ou moyenne massique, MLP, remontée et descente. Ce contrôle vérifie si l'attention est nécessaire.

### 6.2 `Sequoia-fixed`

Attention limitée au parent, aux enfants et aux frères, en utilisant la hiérarchie exogène. C'est le concurrent le plus proche du bloc de famille.

### 6.3 `HSA`

Remplacement des blocs de famille par l'attention hiérarchique dérivée dans le papier NeurIPS 2025. À comparer :

- même largeur ;
- même nombre de têtes ;
- même nombre de paramètres à `±5 %` ;
- même topologie ;
- coût wall-clock et mémoire rapportés.

### 6.4 `AllSet-incidence`

Points, facettes et cofaces sont des types de nœuds, les incidences des hyperarêtes. Cette variante teste l'information d'ordre supérieur complète, mais elle n'est pas requise pour la première soumission.

## 7. Décodeur et sortie

Chaque facette reçoit des logits `ℓ_τ ∈ R^19`. La reprojection est :

```math
\ell_x=\sum_{\tau\ni x}w_{x\tau}\ell_\tau.
```

La combinaison porte sur les logits avant softmax, sauf ablation explicite sur les probabilités. La version logits préserve mieux les marges ; la version probabilités garantit une combinaison convexe. Les deux sont comparées une fois puis gelées.

Les points sans facette valide utilisent un token `uncovered` appris et sont comptés séparément. Leur proportion doit rester négligeable ; elle ne peut pas être masquée de la métrique officielle.

## 8. Batching et coût

Le batch est défini par un budget, non par un nombre fixe de scans :

```yaml
max_leaf_tokens: 160000
max_tree_nodes: 220000
max_lateral_edges: 1200000
max_vertical_edges: 440000
```

Les valeurs sont déterminées après profilage. Le sampler trie grossièrement les scans par nombre de tokens, puis mélange les buckets.

Mesures obligatoires :

- `N_leaf`, `N_node`, `N_lateral`, `N_vertical` ;
- degré maximal et quantiles de degré ;
- profondeur maximale et médiane ;
- taux de réutilisation des descripteurs entre niveaux ;
- coût de prétraitement ;
- VRAM et temps de chaque bloc ;
- coût de reprojection ;
- latence totale incluant la construction de la hiérarchie.

Le polyèdre-only n'est pas automatiquement plus léger. Si le nombre de facettes dépasse massivement le nombre de points, l'argument d'efficacité disparaît, même si le projet peut rester scientifiquement intéressant.

## 9. Tests unitaires architecturaux

1. permutation des points avant prétraitement : sortie identique après permutation inverse ;
2. permutation des sommets d'une facette : token identique pour les canaux invariants ;
3. permutation des enfants d'un nœud : sortie identique ;
4. translation globale : seuls les canaux de pose changent comme prévu ;
5. rotation autour de la gravité : cohérence analytique des canaux ;
6. multiplication globale des niveaux : canaux relatifs inchangés ;
7. conservation de masse facettes→points ;
8. égalité des descripteurs fusionnés et recalculés sur un échantillon ;
9. lot de fusion simultané : invariance à l'ordre d'entrée ;
10. un scan vide ou dégénéré échoue explicitement, sans `NaN` silencieux.
