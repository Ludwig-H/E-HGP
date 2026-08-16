# Guide du projet

## 1. Idée en une phrase

Le réseau ne segmente pas directement un nuage de points. Il segmente une **structure polyédrique multi-échelle** construite hors gradient, puis reporte ses prédictions vers les points par une application linéaire conservant la masse.

```text
scan LiDAR
   │
   ├─ prétraitement géométrique
   │      ├─ facettes élémentaires
   │      ├─ graphe dual / incidences
   │      ├─ arbre complet de fusion
   │      └─ descripteurs fusionnables
   │
   └─ PolyTreeFormer
          ├─ attention locale entre facettes
          ├─ attention parent–enfants
          ├─ contexte descendant
          └─ logits par facette
                    │
                    └─ reprojection pondérée → logits par point
```

Les coordonnées des points ne sont pas introduites sous forme de tokens. Elles ne servent qu'à calculer les objets géométriques, leurs attributs et la reprojection finale.

## 2. Pourquoi ce choix peut être utile au LiDAR

L'échantillonnage d'une même surface change fortement avec la portée. Un réseau point-wise doit apprendre à reconnaître qu'un objet dense à courte distance et le même objet très clairsemé au loin représentent la même structure.

Dans le modèle idéal où la densité observée est multipliée par un facteur positif `q`, les ensembles de niveau vérifient

```math
\{\rho_q \geq \lambda\}=\{\rho \geq \lambda/q\}.
```

L'arbre des composantes reste donc identique et seuls les niveaux sont reparamétrés. En coordonnées logarithmiques, cette reparamétrisation devient une translation. Les quantités

```math
\Delta\log\lambda,
\qquad
\log\lambda_{\mathrm{mort}}-\log\lambda_{\mathrm{naissance}},
\qquad
\log(m_p/m_v)
```

sont alors naturelles.

Le LiDAR réel ne suit pas exactement ce modèle : la dilution varie avec la portée, l'angle d'incidence, l'occultation, les anneaux et la réflectivité. Le projet ne suppose donc pas l'invariance ; il la **mesure** et entraîne explicitement le modèle à la conserver lorsque la sémantique n'a pas changé.

## 3. Ce que signifie « polyèdre-only »

### Autorisé

- utiliser les points hors réseau pour construire les facettes et les descripteurs ;
- calculer des statistiques de rémission ou de géométrie sur les sommets d'une facette ;
- calculer la loss officielle après reprojection vers les points ;
- utiliser les identifiants de points survivants pour apparier deux vues synthétiques en pré-entraînement.

### Interdit dans le modèle principal

- un backbone PointNet, sparse convolution ou Point Transformer ;
- un token par point ;
- un voisinage point-wise appris en parallèle ;
- des features point-wise cachées dans les tokens de facettes ;
- une correction finale par un réseau sur les points.

Cette séparation doit rester testable dans le code. Une variante hybride pourra servir de plafond, mais elle ne doit pas être confondue avec la proposition principale.

## 4. Unités apprises

### 4.1 Feuilles

Les feuilles sont les facettes élémentaires effectivement sérialisées. Elles forment l'unité minimale de prédiction. Chaque feuille reçoit :

- une géométrie intrinsèque ;
- une pose dans le repère capteur et le repère gravitaire ;
- des statistiques d'acquisition ;
- un niveau de filtration ;
- ses incidences et voisins dans le graphe dual.

### 4.2 Nœuds internes

Chaque nœud interne représente l'union des feuilles de sa branche avant un événement de fusion. Ses attributs ne sont pas recalculés par une boucle sur les points pendant l'entraînement. Ils sont obtenus par agrégation exacte ou contrôlée :

- `max` pour les fonctions support ;
- somme pour masses, moments et histogrammes ;
- composition pour les statistiques de filtration ;
- union d'incidences sous forme sparse.

### 4.3 Points de sortie

Une facette `τ` prédit une distribution `p_τ`. Pour un point `x`,

```math
p_x=\sum_{\tau\ni x} w_{x\tau}p_\tau,
\qquad
w_{x\tau}\geq0,
\qquad
\sum_{\tau\ni x}w_{x\tau}=1.
```

La sortie demeure point-wise, sans réintroduire de token point.

## 5. Les quatre familles de canaux

Le modèle ne doit pas mélanger une invariance souhaitée avec une suppression aveugle de l'information.

### A. Forme normalisée

But : reconnaître une structure malgré une variation globale d'échelle ou de densité.

- Gram normalisé de la cellule ;
- rapports de valeurs propres ;
- support directionnel normalisé ;
- quantiles ou CDF de projections ;
- moments centraux normalisés ;
- masques de dégénérescence.

Le support seul ne suffit pas : il ne voit que l'enveloppe convexe. Les statistiques de masse projetée et les incidences conservent l'information intérieure et non convexe.

### B. Grandeurs physiques

But : distinguer des objets de même forme mais de taille ou de position différentes.

- dimensions métriques ;
- hauteur absolue et hauteur au sol ;
- centre dans le repère ego ;
- orientation par rapport à la gravité ;
- portée et direction radiale.

Ces canaux ne sont pas normalisés avec la forme.

### C. Filtration et croissance

But : encoder la trajectoire multi-échelle.

- écarts de niveaux logarithmiques ;
- persistance ;
- rang ou quantile du niveau dans le scan ;
- rapports de masse parent–enfant ;
- type et degré de fusion ;
- âge relatif dans la branche.

Le niveau brut peut être conservé comme canal diagnostique, mais il ne doit pas être le seul encodage.

### D. Acquisition

But : exploiter sans subir le capteur.

- rémission : moyenne, dispersion et quantiles ;
- nombre et étendue des anneaux ;
- couverture angulaire ;
- fraction de sommets ou cellules manquants ;
- angle d'incidence estimé ;
- masque de disponibilité de chaque statistique.

Ces canaux sont soumis à dropout et à des tests de raccourci. Un modèle qui prédit surtout la portée ou l'anneau n'a pas appris l'invariance recherchée.

## 6. Quel Transformer utiliser

### Premier porteur : SPT-nano adapté

Superpoint Transformer fournit déjà :

- un encodeur-décodeur multi-niveaux ;
- des graphes d'adjacence à chaque niveau ;
- des arêtes verticales parent–enfant ;
- des encodages relatifs sur clés, requêtes et valeurs ;
- un mode `nano` sans étage point-wise.

C'est le meilleur point de départ pour tester rapidement la faisabilité. Le partitionnement SPT est remplacé par la hiérarchie fournie ; les descripteurs SPT sont remplacés par les canaux définis ici ; la sortie est portée par les facettes.

### Modèle cible : attention de famille sur l'arbre complet

Une fois le prototype validé, le modèle doit consommer tous les événements de fusion, pas seulement trois coupes arbitraires. Chaque nœud échange avec :

- ses enfants ;
- son parent ;
- éventuellement ses frères par une attention de set ;
- quelques voisins géométriques latéraux au même niveau.

Ce schéma reprend l'idée sparse de Sequoia. Il est linéaire dans le nombre d'arêtes si l'attention entre frères n'est pas quadratique sur les gros événements.

### HSA : opérateur comparatif

HSA offre une dérivation mathématique propre de l'attention sous contrainte hiérarchique et un algorithme dynamique. Il devient pertinent une fois établis :

1. une bonne tokenisation ;
2. des canaux utiles ;
3. un gain provenant réellement de l'arbre.

Il n'est pas la baseline initiale, car il n'existe pas encore de validation 3D comparable et son coût dépend du carré du branchement maximal.

### Encodeur d'incidences complet : extension

Si le graphe dual perd une information mesurable, une couche de type AllSetTransformer peut traiter les incidences sommet–facette–coface comme des applications multiensemble. Cette extension est reportée : elle augmente fortement le coût et complique l'attribution du gain.

## 7. Comment entraîner le modèle

### Supervision directe

Le réseau prédit des logits par facette. La loss principale est calculée après reprojection vers les points. Des cibles molles par facette et par nœud fournissent une supervision auxiliaire sans imposer un label majoritaire faux aux régions mixtes.

### Pré-entraînement de portée

Une vue enseignante utilise le scan complet. Une vue étudiante est produite par une dégradation LiDAR réaliste : réduction d'anneaux, thinning angulaire et radial, occultation par secteurs, point drop et bruit radiométrique. La hiérarchie est reconstruite indépendamment sur les deux vues.

Le réseau étudiant prédit les représentations des branches appariées de l'enseignant. La supervision ne porte que sur des éléments observables par l'étudiant, selon le principe qui s'est montré décisif dans DOS ; aucun token masqué ne révèle sa position.

Les objectifs initiaux sont :

- prédiction latente des branches appariées ;
- prédiction des écarts de fusion et de la persistance ;
- cohérence parent–enfants ;
- régularisation anti-effondrement.

Le masquage porte sur des sous-arbres ou secteurs angulaires, pas sur des cellules indépendantes dispersées au hasard.

## 8. Ce qui doit être démontré avant de viser le classement

1. **Résolution.** La représentation par facettes doit avoir un oracle point-wise très supérieur au score visé.
2. **Stabilité.** La hiérarchie doit mieux résister au thinning que les contrôles.
3. **Apprenabilité.** Un modèle sans tokens points doit atteindre une performance raisonnablement proche d'un backbone point-wise.
4. **Spécificité.** La hiérarchie réelle doit battre un arbre aléatoire, un octree et HDBSCAN à budget égal.
5. **Utilité du niveau.** Les écarts de filtration doivent apporter plus qu'une simple topologie d'arbre.
6. **Transfert.** L'effet doit survivre à un second capteur ou dataset.

Une réussite sur le seul score SemanticKITTI ne suffit pas à démontrer l'invariance. Inversement, une belle stabilité d'arbre sans performance aval ne suffit pas à publier une méthode de segmentation. Il faut les deux, cette exigence absurde que les idées soient à la fois vraies et utiles.

## 9. Configuration minimale recommandée

```yaml
model:
  leaf_dim: 128
  hidden_dim: 192
  num_levels: 4
  local_blocks: 2
  hierarchy_blocks_per_level: 2
  heads: 6
  ffn_ratio: 4
  drop_path: 0.10

channels:
  gram: true
  eig_ratios: true
  support_directions: 42
  projection_quantiles: 9
  physical_scale: true
  filtration_relative: true
  acquisition: true

training:
  batch_by_token_budget: true
  optimizer: adamw
  base_lr: 0.0002
  weight_decay: 0.05
  warmup_epochs: 10
  precision: bf16
```

Cette configuration est un point de départ. Les premières ablations doivent réduire le modèle, pas l'agrandir.
