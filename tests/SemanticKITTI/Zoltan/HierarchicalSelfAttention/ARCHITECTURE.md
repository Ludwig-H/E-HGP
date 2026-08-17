# Architecture de référence

## 1. Décision

Le modèle cible est natif de l'espace

```math
T_{\mathrm{HGP}}\times \mathcal S,
```

où `T_HGP` est l'arbre de densité et `S` l'espace des surfaces polyédriques. Il ne traite pas la hiérarchie comme quatre niveaux de pooling posés sur des facettes. Chaque nœud porte une surface observée et chaque branche décrit une trajectoire de formes.

Nom de travail : **`HGP-PolyFM`**. Le nom n'emporte aucun claim de nouveauté.

SPT-nano reste une baseline d'ingénierie. Sequoia et HSA restent des opérateurs comparatifs. L'architecture principale doit refléter l'objet mathématique plutôt que les commodités historiques d'un dépôt tiers.

## 2. Contrat d'entrée

Pour chaque scan :

```text
facets                    géométrie, aire, normale, attributs et bords
polyhedra                 liste des surfaces HGP utiles
polyhedron_facets         incidences polyèdre ↔ facettes
surface_representation    mesure surfacique, grille sphéro-radiale ou concurrent
parent, children          arbre de fusion
branch_next               succession d'états sur une branche
merge_events              groupes d'enfants simultanés
lateral_edges             voisinages spatiaux entre polyèdres
facet_point_index         incidences de sortie
facet_point_weight        poids de reprojection
```

Un polyèdre est un recollement de facettes, pas un index vers un paquet de points. Les points ne circulent pas comme tokens dans le backbone principal.

Le loader vérifie :

- validité des incidences ;
- orientation et aire finie des facettes ;
- forêt acyclique ;
- cohérence des états le long d'une branche ;
- conservation de masse ou explication explicite des deltas ;
- invariance aux permutations d'identifiants ;
- poids de reprojection positifs sommant à un ;
- ordre exact des points de sortie.

## 3. Décomposition des features

Pour un polyèdre `v`, on ne somme pas aveuglément quatre embeddings. On encode séparément puis on mélange explicitement :

```math
h_v^0=
M_{\mathrm{mix}}
\left[
E_{\mathrm{surf}}(X_v)
\,\|\,
E_{\mathrm{metric}}(g_v)
\,\|\,
E_{\mathrm{filter}}(f_v)
\,\|\,
E_{\mathrm{sensor}}(a_v)
\,\|\,
E_{\mathrm{topo}}(t_v)
\right].
```

Chaque famille possède sa normalisation, son masque de disponibilité et son dropout. La concaténation suivie d'un MLP gated rend les interactions observables et évite de supposer que des espaces latents entraînés indépendamment s'additionnent naturellement.

### 3.1 `surface`

Entrée principale : mesure surfacique attribuée normalisée, discrétisée en une grille sphéro-radiale douce `X_v∈R^{M×B×C}`, définie dans [REPRESENTATION_POLYEDRIQUE.md](REPRESENTATION_POLYEDRIQUE.md).

Canaux initiaux :

```text
normalized_area_mass
normal_projector_or_canonical_normal
absolute_incidence
boundary_mass
remission_mean_or_mass
confidence
```

Les valeurs radiales sont normalisées par une échelle robuste. Le masque de couverture et la masse normalisée restent explicites. L'aire physique et l'échelle sont conservées dans le canal `metric`.

### 3.2 `metric`

```text
log_scale
log_area
extent_gravity
extent_sensor
extent_tangent
center_ego
height_above_ground
orientation_to_gravity
range
azimuth
```

La taille physique n'est pas fusionnée dans la normalisation de forme. Elle est nécessaire à la localisation et aux tâches d'instance.

### 3.3 `filter`

```math
\Delta\log\lambda,
\quad
\operatorname{pers}_{\log}(v),
\quad
q_v=F_{\mathrm{scan}}(\log\lambda_v),
\quad
\log\frac{m_{p(v)}}{m_v},
```

plus : âge dans la branche, gain de masse, degré de fusion, niveau exact/approximé et rang global.

Les rangs sont invariants à une reparamétrisation monotone. Les écarts logarithmiques ne le sont que sous une transformation multiplicative ; les deux canaux sont donc séparés.

### 3.4 `sensor`

```text
ring_coverage
angular_coverage
range_dispersion
beam_pattern_id_or_mask
remission_statistics
estimated_incidence
thinning_fraction
occlusion_score
```

Le canal reçoit un dropout élevé et des probes dédiés. Il ne doit pas être supprimé : la qualité d'une observation est utile. Il ne doit pas non plus devenir le seul raccourci vers la classe.

### 3.5 `topo`

Configuration légère :

- nombre de composantes du graphe de facettes ;
- nombre et longueur des bords ;
- histogramme des degrés du dual ;
- premiers autovalues normalisées du Laplacien dual ;
- indicateurs non-manifold ;
- code optionnel d'une petite branche de graphe de facettes.

Le code topologique complète la discrétisation de mesure, qui ne conserve pas explicitement la combinatoire des facettes et peut lisser les bords fins.

## 4. `SurfaceEncoder`

### 4.1 Architecture pilote

Le tenseur `M×B×C` est traité par un opérateur séparable :

1. `RadialMixer` partagé pour chaque cellule angulaire ;
2. attention locale ou convolution sur le graphe icosaédrique ;
3. quatre tokens de résumé qui interrogent le signal ;
4. pooling vers `z_v^{shape}`.

```yaml
angular_cells: 162
radial_basis: 12
input_channels: 11
surface_width: 192
surface_blocks: 4
surface_heads: 6
summary_tokens: 4
ffn_ratio: 4
pre_norm: true
```

Le voisinage angulaire est celui de l'icosphère. Les biais relatifs dépendent de l'angle géodésique et, si nécessaire, du changement de repère local.

### 4.2 Équivariance

Le modèle principal utilise le repère `gravité–radial capteur–tangent`. Il est stable aux translations et aux homothéties après normalisation, mais conserve l'orientation physiquement utile.

Trois ablations :

- repère ego fixe ;
- encodeur sphérique `SO(3)`-équivariant ;
- augmentation de lacet sans équivariance explicite.

La PCA orientée n'est pas la référence : ses axes changent de signe et d'ordre près des symétries.

### 4.3 Branche de connectivité

`SurfaceGraph` traite un maillage simplifié ou un sous-échantillon de facettes avec poids d'aire. Son readout est ajouté comme résidu contrôlé, sans forcer les deux codes à partager la même sémantique :

```math
z_v^{\mathrm{surf}}
=
M_{\mathrm{surf}}
\left[
z_v^{\mathrm{measure}}
\,\|\,
g_v\odot z_v^{\mathrm{graph}}
\right].
```

Cette branche n'est ouverte que si :

- la grille de mesure lisse des bords, coutures ou couches très proches ;
- le gain survit au contrôle de budget ;
- le modèle reste stable au remeshing.

## 5. Représenter les deltas plutôt que dupliquer les mêmes facettes

Les états d'une branche sont emboîtés. Copier le code complet de la même surface à chaque niveau surcompte l'information et gaspille les tokens.

Lorsque HGP fournit le niveau d'apparition des facettes, définir pour chaque transition :

```math
\Delta F_t=F_{t+1}\setminus F_t.
```

Le `SurfaceEncoder` calcule :

- un code d'état `z_t` ;
- un code d'innovation `\delta_t` porté par les facettes nouvellement actives ;
- un code de confiance sur ce delta.

À défaut d'un delta exact, utiliser une innovation latente :

```math
\delta_t
=
M_\Delta
\left[
 z_{t+1}^{\mathrm{direct}}
 \,\|\,
 \widehat z_{t+1}(z_t)
 \,\|\,
 z_{t+1}^{\mathrm{direct}}-\widehat z_{t+1}(z_t)
\right].
```

Le delta exact est préférable : il donne un sens géométrique aux événements et évite de présenter une différence de features non linéaires comme une quantité physique.

## 6. `BranchEncoder`

Une branche `b` porte une suite

```math
(z_{b,1},\delta_{b,1},\lambda_{b,1}),
\ldots,
(z_{b,L},\delta_{b,L},\lambda_{b,L}).
```

Le modèle partage ses poids entre toutes les branches. Les positions relatives sont les vrais écarts de filtration :

```math
r_{ij}=\phi
\left(
\log\lambda_j-\log\lambda_i,
q_j-q_i,
\log\frac{m_j}{m_i}
\right).
```

Configuration initiale : Transformer sparse bidirectionnel de deux à quatre blocs. Si les branches sont longues, comparer une SSM 1D ou une attention fenêtrée ; la fidélité à la hiérarchie ne requiert pas religieusement une matrice Softmax quadratique.

Le readout de branche conserve :

- état courant ;
- tendance de croissance ;
- événements saillants ;
- persistance ;
- incertitude.

## 7. `MergeEventEncoder`

Un événement de fusion de parent `p` et d'enfants `c_1,…,c_k` est un ensemble :

```math
m_p
=
\operatorname{SetAttn}
\left\{
(h_{c_i}^{\mathrm{end}},e_{c_ip})
\right\}_{i=1}^k.
```

Les relations `e_{c_ip}` contiennent :

- déplacement et orientation relatifs ;
- rapport d'échelle, d'aire et de masse ;
- écart de niveau ;
- interface partagée ;
- recouvrement des mesures surfaciques normalisées ;
- persistance de l'enfant.

Le parent combine trois sources :

```math
h_p
=
M_p
\left[
 z_p^{\mathrm{direct}}
 \,\|\,
 m_p
 \,\|\,
 \delta_p
\right].
```

L'attention entre enfants est exacte sous `k≤k_max`. Au-delà, utiliser des inducing tokens ou un pooling segmenté. Aucune binarisation arbitraire des fusions simultanées dans le modèle principal.

## 8. Contexte latéral

Deux branches peuvent être spatialement voisines sans partager encore de parent utile. On ajoute un graphe sparse entre polyèdres à des niveaux compatibles.

Une arête `v→w` contient :

```text
relative_center_in_gravity_sensor_frame
normalized_distance
relative_scale
normal_compatibility
shared_boundary_or_nearest_interface
surface_measure_overlap
filter_level_gap
visibility_relation
```

Un bloc `LateralGraph` produit des corrections `Q/K/V` ou un message typé. Le voisinage est borné par rayon et `k`, puis audité pour éviter qu'il reconstitue simplement un k-NN point-wise dissimulé sous une terminologie plus distinguée.

## 9. Contexte global

Quelques latents de scène `G∈R^{K_g×d}` peuvent lire puis redistribuer l'information :

```math
G\leftarrow\operatorname{CrossAttn}(G,H),
\qquad
H\leftarrow H+\operatorname{CrossAttn}(H,G).
```

Ils sont insérés toutes les trois ou quatre couches, non à chaque bloc. Ils sont retenus seulement s'ils améliorent les interactions lointaines sans annuler l'utilisation de l'arbre.

Contrôle obligatoire : mesurer la performance lorsque les relations HGP sont permutées mais les latents globaux conservés.

## 10. Macro-architecture `HGP-PolyFM`

```text
grille de mesure / SurfaceGraph par polyèdre
              │
              ▼
        SurfaceEncoder partagé
              │
     ┌────────┼─────────┐
     ▼        ▼         ▼
Branch    MergeEvent   LateralGraph
Encoder    Encoder       Encoder
     └────────┼─────────┘
              ▼
       blocs répétés × L
              │
        latents globaux
              │
              ▼
       embeddings de nœuds
              │
              ▼
         FacetDecoder
              │
              ▼
       logits point-wise
```

Configuration pilote :

```yaml
model_dim: 256
surface_dim: 192
hierarchy_blocks: 6
branch_blocks_per_stage: 2
merge_heads: 8
lateral_heads: 8
global_tokens: 0          # ouvrir seulement après ablation
global_tokens_candidate: 16
global_every: 3
ffn_ratio: 4
drop_path: 0.15
precision: bf16
```

Ces valeurs sont des points de départ, pas une invitation à lancer une loterie de 200 configurations.

## 11. Décodeur par facette

Les facettes servent de résolution de sortie, non de tokens principaux du backbone.

Pour une facette `τ`, on construit :

```math
h_\tau^0=E_{\mathrm{facet}}(x_\tau),
```

puis elle interroge les polyèdres ancêtres sélectionnés :

```math
h_\tau
=
h_\tau^0
+
\operatorname{CrossAttn}
\left(
q=h_\tau^0,
K,V=\{h_v:v\in\operatorname{Anc}(\tau)\}
\right).
```

Les ancêtres sont échantillonnés par événements et persistance, jamais par quatre profondeurs arbitraires seulement. Une arête locale entre facettes peut affiner les frontières dans le décodeur.

Les logits sont reprojetés :

```math
\ell_x
=
\sum_{\tau\ni x}w_{x\tau}\ell_\tau.
```

Les points non couverts utilisent une sortie `uncovered` et sont rapportés séparément.

## 12. Baselines architecturales

| Modèle | Question isolée |
|---|---|
| `Analytic-MLP` | les descripteurs analytiques suffisent-ils ? |
| `SurfaceGraph` | que vaut le polyèdre sans grille de mesure ni hiérarchie ? |
| `Measure-Flat` | que vaut le tokenizer sans arbre ? |
| `Measure-Lateral` | le graphe spatial suffit-il ? |
| `SPT-nano-poly` | une hiérarchie U-Net standard suffit-elle ? |
| `MeanTree` | l'attention de fusion est-elle nécessaire ? |
| `Sequoia-fixed` | meilleure baseline arbre parent–enfants–frères |
| `HSA` | valeur de la contrainte de blocs hiérarchiques |
| `HGP-PolyFM` | modèle complet surface × échelle |

HSA est appliquée conformément à son régime publié sur des feuilles ou dans une extension explicitement distincte. Le fait que chaque nœud HGP porte sa propre surface retire toute permission de recycler son théorème d'optimalité sans nouvelle démonstration.

## 13. Batching et complexité

Le batch est défini par plusieurs budgets :

```yaml
max_polyhedra: 60000
max_surface_cells: 10000000
max_facets_decoder: 250000
max_lateral_edges: 500000
max_vertical_edges: 120000
```

Mesurer séparément :

- construction et encodage de la grille de mesure ;
- `SurfaceEncoder` ;
- branches ;
- fusions ;
- graphe latéral ;
- décodeur ;
- reprojection.

Le stockage ne duplique jamais toutes les facettes dans tous les ancêtres. Utiliser références, mesures agrégées et deltas.

## 14. Tests unitaires

1. permutation des facettes : code continu inchangé ;
2. remeshing d'une même surface : grille de mesure inchangée à la quadrature près ;
3. permutation des enfants : sortie identique ;
4. translation et homothétie : code de forme invariant, canal métrique transformé ;
5. rotation de lacet : transformation conforme au repère choisi ;
6. multicouche synthétique : aucune collision avec le cas monocouche ;
7. surface ouverte : masque et masse corrects ;
8. changement d'ancre : sensibilité mesurée, pas silencieuse ;
9. ordre des événements : cohérence avec les niveaux réels ;
10. conservation de masse jusqu'aux points ;
11. niveaux multipliés par une constante : rangs inchangés et `Δlogλ` inchangé ;
12. cas dégénérés : échec explicite sans `NaN`.
