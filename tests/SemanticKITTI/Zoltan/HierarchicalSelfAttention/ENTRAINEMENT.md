# Entraînement

## 1. Ordre des régimes

L'entraînement est ouvert par étapes. Chaque étape doit produire un résultat interprétable avant d'ajouter la suivante.

1. **supervisé minimal** : vérifier que les tokens portent la sémantique ;
2. **supervisé hiérarchique** : ajouter les cibles molles internes ;
3. **pré-entraînement de portée** : apprendre l'invariance au thinning ;
4. **softmaps/prototypes** : seulement si la distillation latente fonctionne ;
5. **multi-ordre ou incidence complète** : seulement après gain robuste.

## 2. Supervision sémantique

### 2.1 Cibles par facette

Pour une facette `τ`, la cible est une distribution pondérée :

```math
\pi_\tau(c)=
\frac{\sum_{x\in\tau}w_{x\tau}\mathbf 1[y_x=c]}
{\sum_{x\in\tau}w_{x\tau}}.
```

Un label majoritaire n'est utilisé que pour les métriques auxiliaires. Les facettes mixtes ne sont pas forcées à prétendre qu'une frontière n'existe pas.

### 2.2 Cibles par nœud

Pour un nœud `v`,

```math
\pi_v(c)=
\frac{\sum_x w_{x\to v}\mathbf 1[y_x=c]}
{m_v}.
```

La supervision interne est normalisée pour qu'un point ne soit pas compté une fois par ancêtre. Une option simple consiste à tirer un seul niveau interne par scan et par itération.

### 2.3 Loss principale

```math
\mathcal L_{\mathrm{sup}}=
\mathcal L_{\mathrm{CE,point}}
+\lambda_{\mathrm{Lovasz}}\mathcal L_{\mathrm{Lovasz,point}}
+\lambda_{\mathrm{leaf}}\mathcal L_{\mathrm{CE,soft,leaf}}
+\lambda_{\mathrm{node}}\mathcal L_{\mathrm{CE,soft,node}}
+\lambda_{\mathrm{bdry}}\mathcal L_{\mathrm{boundary}}.
```

Point de départ :

```yaml
lambda_lovasz: 1.0
lambda_leaf: 0.2
lambda_node: 0.05
lambda_boundary: 0.1
```

La loss de frontière est une BCE sur les arêtes latérales : deux facettes sont-elles séparées par une frontière sémantique ? Elle est désactivée lors de l'ablation principale de l'arbre afin de ne pas attribuer à la hiérarchie un gain fourni par une tête auxiliaire.

## 3. Augmentations supervisées

La hiérarchie est calculée avant l'entraînement. Une augmentation n'est autorisée que si elle peut être transportée exactement ou si la hiérarchie est reconstruite.

### Transport analytique

- rotation en lacet ;
- translation ;
- symétrie horizontale ;
- homothétie, avec transformation cohérente des tailles et niveaux ;
- bruit ou dropout de rémission ;
- dropout de canaux ;
- dropout de sous-arbres.

### Reconstruction obligatoire

- crop arbitraire pouvant couper une composante ;
- déformation élastique ;
- suppression structurée de points ou d'anneaux ;
- simulation d'occultation ;
- mélange de scans.

Réutiliser la hiérarchie du scan complet après une augmentation qui change sa connectivité produirait un objet commode, mais faux. Ce genre de raccourci donne parfois de bons scores, puis de très longues discussions avec les reviewers.

## 4. Pré-entraînement principal : `Range-Hierarchy JEPA`

### 4.1 Deux vues physiques

Pour chaque scan non annoté :

- **teacher view** : scan complet ou faiblement perturbé ;
- **student view** : scan dégradé comme s'il provenait d'une portée ou d'un capteur moins favorable.

La vue étudiante combine aléatoirement :

1. **beam drop** : conservation d'un sous-ensemble cohérent d'anneaux ;
2. **angular thinning** : suppression selon azimut et élévation ;
3. **range-dependent drop** : probabilité de conservation décroissante avec la portée simulée ;
4. **sector occlusion** : secteurs angulaires continus masqués ;
5. **local point drop** : absorption ou occultation locale ;
6. **radiometric corruption** : bruit et dropout de rémission.

Les paramètres sont inspirés des perturbations de LiDomAug, LiDAR Distillation et SemanticKITTI-C, puis calibrés sur les statistiques réelles des scans. Un thinning Bernoulli uniforme reste un contrôle, pas la vue principale.

Les deux hiérarchies sont reconstruites indépendamment. Cela teste la propriété réellement requise à l'inférence ; restreindre l'arbre dense aux points survivants rendrait la tâche artificiellement facile.

### 4.2 Appariement sans labels

Pour les vues synthétiques, les identifiants des retours survivants sont connus. Deux nœuds `u` et `v` sont appariés par recouvrement pondéré :

```math
J_w(u,v)=
\frac{\sum_x\min(w_{x\to u},w_{x\to v})}
{\sum_x\max(w_{x\to u},w_{x\to v})}.
```

Conserver les couples satisfaisant :

- `J_w ≥ τ_match` ;
- rapport de masses dans un intervalle déclaré ;
- cohérence de branche parent–enfant ;
- appariement mutuel ou solution bipartite sans collision.

Le seuil est choisi sur le train à partir de la précision d'appariement géométrique, jamais à partir des labels sémantiques.

### 4.3 Architecture teacher–student

- même encodeur PolyTreeFormer ;
- teacher mis à jour par EMA ;
- teacher reçoit le contexte complet ;
- student reçoit uniquement les tokens réellement observés ;
- un prédicteur MLP ou deux blocs Transformer légers n'existe que côté student ;
- stop-gradient sur les cibles teacher.

Le coefficient EMA suit un cosinus de `0.996` vers `0.9999`.

### 4.4 Objectif latent

Pour un couple apparié `(u, v)` :

```math
\mathcal L_{\mathrm{JEPA}}=
\sum_{(u,v)}\omega_{uv}
\left[1-
\cos\left(P(h_v^S),\operatorname{sg}(h_u^T)\right)
\right].
```

Les poids `ω` dépendent du score d'appariement et de la racine carrée de la masse, puis sont normalisés par scène.

La prédiction porte sur plusieurs profondeurs. Une seule racine géante ne doit pas absorber la loss.

### 4.5 Objectif de trajectoire

Masquer un segment de branche et prédire :

- son embedding teacher ;
- les prochains écarts `Δ log λ` ;
- la persistance résiduelle ;
- le rapport de masse au prochain événement ;
- le degré du prochain lot de fusion.

```math
\mathcal L_{\mathrm{event}}=
\operatorname{Huber}(\widehat{\Delta\log\lambda},\Delta\log\lambda)
+\operatorname{Huber}(\widehat{\log m},\log m)
+\operatorname{CE}(\widehat d,d).
```

Les valeurs de niveau sont relatives. Prédire le niveau absolu encouragerait le réseau à apprendre la densité du capteur plutôt que la structure.

### 4.6 Cohérence parent–enfants

Le parent teacher est prédit à partir des enfants student :

```math
\widehat h_p=\operatorname{SetAgg}
\{(h_v,m_v/m_p):v\in\operatorname{ch}(p)\},
```

```math
\mathcal L_{\mathrm{parent}}=
1-\cos(\widehat h_p,\operatorname{sg}(h_p^T)).
```

Cet objectif teste directement l'information portée par la fusion. Il n'est utile que si les familles ne sont pas triviales.

### 4.7 Anti-effondrement

La baseline utilise une régularisation de variance et covariance de type VICReg sur les embeddings de nœuds échantillonnés :

```math
\mathcal L_{\mathrm{reg}}=
\mathcal L_{\mathrm{variance}}+
\eta\mathcal L_{\mathrm{covariance}}.
```

Les négatifs contrastifs ne sont pas la première option : deux branches distinctes d'une scène peuvent partager la même sémantique, en particulier route, végétation et bâtiment.

### 4.8 Loss totale SSL

```math
\mathcal L_{\mathrm{SSL}}=
\mathcal L_{\mathrm{JEPA}}
+0.25\mathcal L_{\mathrm{event}}
+0.10\mathcal L_{\mathrm{parent}}
+0.05\mathcal L_{\mathrm{reg}}.
```

Ces poids sont des valeurs initiales. Le premier balayage ne dépasse pas trois configurations.

## 5. Variante DOS sur tokens polyédriques

DOS montre que l'observable self-distillation et les softmaps spatiales peuvent dépasser la simple régression de features sur des backbones point-wise. La transposition proposée est :

- prototypes appliqués aux facettes ou nœuds observables ;
- normalisation de chaque prototype sur les tokens d'une scène ;
- teacher complet, student aminci ;
- KL entre softmaps sur les couples appariés ;
- prior uniforme, puis Zipf, comme ablation séparée.

Cette variante n'est ouverte qu'après succès de `Range-Hierarchy JEPA`. Les correspondances entre deux arbres étant plus fragiles qu'entre deux copies voxelisées du même scan, commencer directement avec prototypes, Sinkhorn et hiérarchie variable rendrait l'échec presque impossible à diagnostiquer.

## 6. Masquage

### Bon masquage

- sous-arbre complet ;
- segment continu d'une branche ;
- secteur angulaire ;
- groupe de facettes adjacentes ;
- lot d'anneaux.

### Mauvais masquage principal

- facettes indépendantes uniformément tirées ;
- tokens masqués conservant un encodage de position exact ;
- masque construit après calcul des cibles à partir des features student ;
- suppression si forte qu'elle modifie la classe ou fait disparaître tout l'objet.

La force du masque est adaptée à la taille de l'objet et au taux d'appariement. Les paires sans correspondance fiable ne contribuent pas à la loss JEPA.

## 7. Optimisation

### Pilote supervisé

```yaml
optimizer: AdamW
base_lr: 0.0002
weight_decay: 0.05
schedule: cosine
warmup_epochs: 5
epochs: 80
precision: bf16
grad_clip: 1.0
layer_decay: 0.85
```

### Pré-entraînement

```yaml
optimizer: AdamW
base_lr: 0.0004
weight_decay: [0.04, 0.20]  # cosinus
schedule: cosine
warmup_epochs: 10
pilot_epochs: 50
full_epochs: 200
precision: bf16
grad_clip: 1.0
ema_start: 0.996
ema_end: 0.9999
```

Les learning rates sont redimensionnés selon le nombre effectif de tokens, pas seulement selon le nombre de scans.

### Fine-tuning

- initialiser l'encodeur depuis le SSL ;
- réinitialiser les têtes sémantiques ;
- learning rate du backbone `3–5×` plus faible que celui des têtes ;
- layer-wise decay `0.8–0.9` ;
- 80 à 120 époques ;
- même recette d'augmentation pour toutes les baselines appariées.

## 8. Curriculum expérimental

| Étape | Modèle | Objectif | Question |
|---|---|---|---|
| T0 | MLP par facette | supervisé | les canaux suffisent-ils localement ? |
| T1 | graphe dual | supervisé | l'adjacence locale aide-t-elle ? |
| T2 | SPT-nano adapté | supervisé | la hiérarchie aide-t-elle ? |
| T3 | arbre complet | supervisé | les événements fins valent-ils mieux que quatre coupes ? |
| T4 | arbre complet | Range-JEPA | l'invariance peut-elle être apprise ? |
| T5 | arbre complet | softmaps observables | gain supplémentaire ? |
| T6 | multi-ordre/incidence | meilleur objectif | extension seulement si T4/T5 réussissent |

## 9. Journalisation obligatoire

Pour chaque run :

- hash du code et des hiérarchies ;
- version du schéma de données ;
- statistiques de tokens et d'arêtes ;
- graine, matériel, précision et batch effectif ;
- loss par profondeur, masse, portée et taux de thinning ;
- taux et score moyen d'appariement teacher–student ;
- variance des embeddings et utilisation des prototypes ;
- mIoU global et par classe ;
- résultats par portée et classe filiforme ;
- coût complet prétraitement + réseau + reprojection.

Une loss SSL qui baisse pendant que le taux d'appariement s'effondre n'est pas une victoire. C'est le réseau qui apprend sur les rares cas faciles restants.
