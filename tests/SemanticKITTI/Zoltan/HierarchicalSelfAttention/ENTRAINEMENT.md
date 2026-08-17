# Entraînement et pré-entraînement

## 1. Principe

Le projet ne doit pas commencer par une loss compliquée sur deux arbres mal appariés. L'entraînement est ouvert par étapes, chacune répondant à une question identifiable :

1. le tokenizer conserve-t-il la géométrie et la sémantique ?
2. le modèle plat apprend-il sur les polyèdres ?
3. l'espace d'échelle HGP apporte-t-il une information supplémentaire ?
4. peut-on apprendre une invariance à l'acquisition sans effacer taille et pose ?
5. les représentations transfèrent-elles entre capteurs, datasets et tâches ?

## 2. Décomposition du latent

Chaque polyèdre produit des sous-espaces explicites :

```math
z_v=
 z_v^{\mathrm{shape}}
 \oplus z_v^{\mathrm{metric}}
 \oplus z_v^{\mathrm{hier}}
 \oplus z_v^{\mathrm{sensor}}.
```

- `shape` : géométrie normalisée et contenu sémantique ;
- `metric` : taille, pose et localisation ;
- `hier` : croissance, persistance et rôle dans l'arbre ;
- `sensor` : qualité de mesure et caractéristiques d'acquisition.

Les losses d'invariance ne s'appliquent pas au token complet. Une voiture observée plus loin doit garder une forme proche, mais sa position, son incidence et son incertitude peuvent légitimement changer.

## 3. Étape A — autoencodage diagnostique de la surface

Avant toute sémantique, évaluer la capacité des représentations candidates à reconstruire `Σ_v`.

### Cibles

- mesure surfacique normalisée ou grille sphéro-radiale basse fréquence ;
- points de surface échantillonnés par aire ;
- normales ;
- bords ;
- multiplicité radiale ;
- scalaires de connectivité.

### Loss

```math
\mathcal L_{\mathrm{repr}}
=
\lambda_{\mathrm{grid}}\mathcal L_{\mathrm{grid}}
+
\lambda_{\mathrm{Chamfer}}\mathcal L_{\mathrm{Chamfer}}^{A}
+
\lambda_{\mathrm{normal}}\mathcal L_{\mathrm{normal}}
+
\lambda_{\mathrm{boundary}}\mathcal L_{\mathrm{boundary}}.
```

Le Chamfer est pondéré par l'aire afin qu'une triangulation dense n'obtienne pas davantage de voix qu'une triangulation sobre de la même surface.

Cette étape sert à tracer la frontière taux–distorsion. Elle ne constitue pas le pré-entraînement final : reconstruire parfaitement les détails d'acquisition peut encourager le raccourci géométrique identifié dans les travaux récents de SSL 3D.

## 4. Étape B — supervision SemanticKITTI

### 4.1 Cibles par facette

Pour une facette `τ` :

```math
\pi_\tau(c)
=
\frac{\sum_{x\in\tau}w_{x\tau}\mathbf 1[y_x=c]}
{\sum_{x\in\tau}w_{x\tau}}.
```

Les facettes mixtes reçoivent une distribution douce. Le label majoritaire est réservé aux métriques auxiliaires.

### 4.2 Cibles par polyèdre

Pour un polyèdre `v` :

```math
\pi_v(c)
=
\frac{\sum_x w_{x\to v}\mathbf 1[y_x=c]}{m_v}.
```

La supervision interne ne doit pas compter le même point une fois par ancêtre sans correction. Trois options :

- échantillonner un seul niveau par branche et par itération ;
- normaliser par le nombre d'ancêtres supervisés ;
- superviser uniquement les événements persistants.

### 4.3 Loss principale

```math
\mathcal L_{\mathrm{sup}}
=
\mathcal L_{\mathrm{CE,point}}
+
\mathcal L_{\mathrm{Lovasz,point}}
+
0.2\mathcal L_{\mathrm{soft,facet}}
+
0.05\mathcal L_{\mathrm{soft,poly}}
+
0.1\mathcal L_{\mathrm{boundary}}.
```

La loss de frontière est désactivée dans l'ablation principale de la hiérarchie, afin de ne pas attribuer à l'arbre un gain fourni par une tête auxiliaire.

## 5. Étape C — `Surface-JEPA`

### 5.1 Masquage

Masquer des régions cohérentes :

- secteur contigu sur la sphère ;
- intervalle radial ;
- bloc `S²×R` ;
- groupe connecté de facettes ;
- partie entière de la surface visible.

Éviter le masquage indépendant de cases dispersées, trop facile à interpoler localement et peu lié à une notion de partie.

### 5.2 Teacher–student

- même `SurfaceEncoder` ;
- teacher mis à jour par EMA ;
- teacher reçoit la surface complète ou moins dégradée ;
- student reçoit les secteurs visibles ;
- predictor présent seulement côté student ;
- stop-gradient sur le teacher.

### 5.3 Objectif latent

```math
\mathcal L_{\mathrm{surfJEPA}}
=
1-
\cos
\left(
P(z_{v,\mathrm{visible}}^{S,\mathrm{shape}}),
\operatorname{sg}(z_{v}^{T,\mathrm{shape}})
\right).
```

Une petite reconstruction basse fréquence de la grille de mesure peut stabiliser le départ, mais la cible principale reste latente.

## 6. Étape D — apprentissage de l'espace d'échelle

### 6.1 Prédiction parent depuis enfants

Pour un événement `p←{c_i}` :

```math
\widehat z_p
=
\operatorname{MergePredictor}
\{z_{c_i}^{\mathrm{end}},e_{c_ip}\},
```

```math
\mathcal L_{\mathrm{parent}}
=
1-
\cos
\left(
\widehat z_p^{\mathrm{shape}},
\operatorname{sg}(z_p^{T,\mathrm{shape}})
\right).
```

La taille et la pose ne sont pas contraintes à être identiques à une moyenne des enfants. Elles sont prédites par des têtes métriques séparées.

### 6.2 Prédiction des innovations

Lorsque les deltas de facettes sont disponibles, masquer `ΔF_t` et prédire son embedding teacher. Sinon, prédire l'innovation latente définie dans [ARCHITECTURE.md](ARCHITECTURE.md).

### 6.3 Trajectoire de branche

Masquer un intervalle de niveaux et prédire :

- embedding du segment ;
- prochain `Δlogλ` ;
- persistance restante ;
- gain de masse ou d'aire ;
- type du prochain événement.

```math
\mathcal L_{\mathrm{event}}
=
\mathcal L_{\mathrm{latent}}
+
\operatorname{Huber}(\widehat{\Delta\log\lambda},\Delta\log\lambda)
+
\operatorname{Huber}(\widehat{\Delta\log m},\Delta\log m)
+
\operatorname{CE}(\widehat{\mathrm{type}},\mathrm{type}).
```

La partie événementielle reste auxiliaire. Un réseau capable de prédire le prochain artefact d'anneau n'a pas nécessairement compris un objet.

## 7. Étape E — `Cross-Range PolyJEPA`

### 7.1 Deux observations

Teacher :

```math
\mathcal H^T=\operatorname{HGP}(X).
```

Student :

```math
\mathcal H^S=\operatorname{HGP}(\mathcal A(X)),
```

où `A` simule une acquisition différente :

- suppression cohérente d'anneaux ;
- thinning azimutal et vertical ;
- conservation dépendante de la portée ;
- occultation par secteurs ;
- bruit et dropout de rémission ;
- incidence dégradée ;
- changement de pattern LiDAR.

Les deux hiérarchies sont reconstruites indépendamment. Restreindre l'arbre dense aux retours survivants reste un contrôle optimiste, jamais la condition principale.

### 7.2 Matching des polyèdres

Pour les vues synthétiques, les retours survivants fournissent une correspondance partielle. Le score combine :

```math
J_w(u,v),
\quad
\operatorname{overlap}_{\mathrm{surface}},
\quad
\operatorname{dist}_{\mathrm{measure}},
\quad
\operatorname{coherence}_{\mathrm{tree}}.
```

Conserver les couples satisfaisant :

- recouvrement minimal ;
- rapport d'aire et d'échelle admissible ;
- cohérence parent–enfant ;
- matching mutuel ou transport optimal sparse ;
- couverture suffisante du student.

Rapporter le taux d'appariement par classe, portée, taille, multiplicité radiale et persistance. Une loss qui baisse parce que tous les petits objets ont été rejetés n'est pas un progrès.

### 7.3 Alignement partiel

Pour un couple `(u,v)` :

```math
\mathcal L_{\mathrm{range}}
=
\omega_{uv}
\left[
1-
\cos
\left(
P(z_v^{S,\mathrm{shape}}),
\operatorname{sg}(z_u^{T,\mathrm{shape}})
\right)
\right].
```

La pondération dépend de la qualité du matching et de la masse observée, avec plafonnement pour empêcher les routes et bâtiments de monopoliser l'objectif.

Ajouter une loss de cohérence des sous-espaces :

- `shape` aligné fortement ;
- `hier` aligné lorsque les événements correspondent ;
- `metric` prédit mais non invariant ;
- `sensor` décorrélé partiellement du `shape` par adversarial probe ou covariance penalty en ablation.

## 8. Étape F — supervision temporelle

Les séquences LiDAR donnent des observations naturelles de la même géométrie à des portées différentes.

Pipeline :

1. compensation de l'ego-motion ;
2. exclusion ou traitement séparé des objets mobiles ;
3. appariement surfacique entre scans ;
4. matching de branches HGP ;
5. teacher utilisant éventuellement un agrégat temporel plus dense ;
6. student limité à un scan mono-scan.

Objectifs :

- cohérence de forme ;
- recherche cross-range ;
- prédiction de parties occultées ;
- stabilité des instances ;
- calibration d'incertitude.

Le teacher temporel est autorisé au pré-entraînement mais doit être déclaré séparément. L'inférence principale reste mono-scan.

## 9. Étape G — distillation 2D et langage

Après validation de la géométrie :

1. projeter chaque facette dans les images synchronisées ;
2. agréger les features d'un modèle visuel sur le polyèdre ;
3. distiller vers `z^{shape}` et `z^{sem}` ;
4. aligner éventuellement avec un espace texte pour l'open vocabulary.

Cette étape ne doit pas précéder la preuve de valeur du tokenizer. Un teacher 2D puissant peut fournir d'excellents scores tout en transformant la hiérarchie HGP en décoration coûteuse.

## 10. Anti-effondrement et prototypes

Baseline : variance–invariance–covariance sur des polyèdres échantillonnés par strates de masse et de niveau.

```math
\mathcal L_{\mathrm{reg}}
=
\mathcal L_{\mathrm{variance}}
+
\eta\mathcal L_{\mathrm{covariance}}.
```

Les prototypes doux ne sont ouverts qu'après succès du JEPA latent. Leur distribution doit être auditée par taille, portée, classe et niveau ; sinon un prototype peut devenir un élégant synonyme d'anneau LiDAR.

## 11. Loss totale par étape

### Pilote géométrique

```math
\mathcal L_A=\mathcal L_{\mathrm{repr}}.
```

### Pré-entraînement surface

```math
\mathcal L_C
=
\mathcal L_{\mathrm{surfJEPA}}
+0.05\mathcal L_{\mathrm{repr,lowfreq}}
+0.05\mathcal L_{\mathrm{reg}}.
```

### Pré-entraînement hiérarchique

```math
\mathcal L_D
=
\mathcal L_C
+0.20\mathcal L_{\mathrm{parent}}
+0.10\mathcal L_{\mathrm{event}}.
```

### Pré-entraînement cross-range

```math
\mathcal L_E
=
\mathcal L_{\mathrm{range}}
+0.20\mathcal L_{\mathrm{surfJEPA}}
+0.10\mathcal L_{\mathrm{parent}}
+0.05\mathcal L_{\mathrm{event}}
+0.05\mathcal L_{\mathrm{reg}}.
```

Ces coefficients sont des valeurs initiales. Le premier sweep reste limité et chaque loss doit disposer d'une ablation.

## 12. Optimisation

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
weight_decay_start: 0.04
weight_decay_end: 0.20
schedule: cosine
warmup_epochs: 10
pilot_epochs: 50
full_epochs: 200
teacher_ema_start: 0.996
teacher_ema_end: 0.9999
precision: bf16
grad_clip: 1.0
```

Le learning rate est redimensionné selon le nombre effectif de polyèdres et de cellules de la grille surfacique, pas uniquement selon le nombre de scans.

## 13. Curriculum vers le modèle de fondation

| Phase | Données | Objectif | Critère |
|---|---|---|---|
| F0 | polyèdres d'un dataset | fidélité / remeshing | tokenizer valide |
| F1 | SemanticKITTI | supervisé | backbone apprenable |
| F2 | SemanticKITTI non annoté | Surface-JEPA | probing positif |
| F3 | vues simulées | Cross-Range PolyJEPA | robustesse densité |
| F4 | séquences réelles | temporel | retrieval cross-range |
| F5 | plusieurs LiDAR | multi-capteurs | transfert sans retokeniser |
| F6 | LiDAR + images | distillation 2D | sémantique et low-shot |
| F7 | multi-tâches | adaptation | statut fondation défendable |

## 14. Journalisation obligatoire

Pour chaque run :

- hash du code, du schéma HGP et des mesures et grilles surfaciques ;
- représentation, ancre, échelle, `M`, `B` et canaux ;
- distributions de radialité et de multiplicité ;
- nombre de polyèdres, facettes, événements et arêtes ;
- taux de matching teacher–student ;
- pertes par masse, portée, classe diagnostique et niveau ;
- variance des sous-espaces latents ;
- probes sémantique, portée, anneau, capteur et thinning ;
- mIoU, frontières, détection/instance selon la tâche ;
- latence et mémoire de chaque étage.
