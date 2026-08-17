# Représentation polyédrique : décision scientifique

## 1. Objet représenté

Pour un nœud `v` de la hiérarchie HGP, on note

```math
\Sigma_v=\bigcup_{\tau\in F_v}\tau
```

le recollement de facettes associé. `Σ_v` est une **surface polyédrique observée** : elle peut être ouverte, non convexe, partiellement occultée, multicouche depuis certains points de vue et éventuellement non-manifold. Elle ne doit être confondue ni avec l'enveloppe convexe des retours ni avec un amas de points auquel on aurait simplement donné un nom plus géométrique.

Le réseau doit produire un code fixe

```math
z_v=E_{\mathrm{surf}}(\Sigma_v)\in\mathbb R^d
```

qui conserve la géométrie utile, varie peu sous rééchantillonnage LiDAR et reste exploitable pour plusieurs tâches.

La question prioritaire est donc :

> Existe-t-il un tokenizer de surfaces polyédriques qui offre un meilleur compromis fidélité–compression–stabilité–sémantique que les points, voxels et superpoints ?

La hiérarchie ne peut devenir le support d'un modèle de fondation qu'après une réponse positive à cette question.

## 2. Audit de l'idée radiale monocouche

Pour un centre `c` et une direction `u∈S²`, définissons

```math
\mathcal R_{\Sigma,c}(u)
=
\{r>0:c+ru\in\Sigma\},
\qquad
N_{\Sigma,c}(u)=|\mathcal R_{\Sigma,c}(u)|.
```

Une carte radiale ordinaire `ρ(u)` n'existe que lorsque `N(u)=1` sur le domaine considéré. Cette hypothèse peut être raisonnable pour une surface quasi étoilée, mais elle échoue pour :

- les concavités et replis ;
- les surfaces ouvertes vues de biais ;
- la végétation, les clôtures et les structures grillagées ;
- les surfaces possédant plusieurs couches dans une même direction ;
- les centres situés sur ou près d'une feuille mince ;
- les événements HGP où plusieurs morceaux se rejoignent.

Même lorsque `N(u)≤1`, une tangence peut faire apparaître ou disparaître brutalement une intersection sous une perturbation faible. La carte monocouche est donc une **baseline interprétable**, pas la définition générale du token polyédrique.

### 2.1 Mesures de radialité

Pour des directions `u_j`, rapporter :

```math
C(c)=\frac1M\sum_j\mathbf 1[N_c(u_j)\ge1]
```

pour la couverture, et

```math
U(c)=
\frac{\sum_j\mathbf 1[N_c(u_j)=1]}
{\sum_j\mathbf 1[N_c(u_j)\ge1]}
```

pour l'unicité conditionnelle.

Ajouter obligatoirement :

- l'histogramme complet de `N_c(u)` ;
- la masse surfacique portée par les directions `N=0`, `N=1`, `N≥2` ;
- les résultats par classe, portée, persistance et dimension locale ;
- la sensibilité à une perturbation du centre ;
- la stabilité des intersections sous thinning.

Une moyenne élevée de `U` ne suffit pas si les échecs se concentrent sur les petites classes, les bords ou les objets articulés.

## 3. Solutions possibles à la non-unicité

### 3.1 Carte radiale à `K` couches

On stocke les intersections ordonnées

```math
r_1(u)\le\cdots\le r_K(u)
```

avec masques, normales et attributs.

**Avantages**

- extension directe de l'intuition initiale ;
- tenseur fixe et reconstruction explicite ;
- coût faible lorsque `K` est petit.

**Limites**

- troncature lorsque la multiplicité dépasse `K` ;
- discontinuités lorsque deux couches apparaissent, disparaissent ou s'échangent ;
- dépendance forte au centre ;
- nombreuses cases vides pour les surfaces ouvertes.

**Décision**

Conserver `K=1,2,4` comme baselines. Cette représentation n'est retenue comme voie principale que si la multiplicité empirique est presque toujours bornée et stable, ce qu'il serait imprudent de décréter avant d'avoir regardé les données.

### 3.2 Atlas multi-cartes

On couvre `Σ` par des cartes locales

```math
f_a:D_a\subset\mathbb R^2\to\Sigma,
```

par exemple des height maps orientées par le capteur, la gravité ou une normale locale. Les cartes sont encodées par un réseau partagé puis agrégées comme un ensemble.

**Avantages**

- représente les surfaces ouvertes, concaves et de topologie générale ;
- conserve les détails locaux ;
- possède des précédents solides, d'AtlasNet aux atlas métriquement cohérents.

**Limites**

- choix des ancres, coutures et orientations ;
- nombre de cartes variable ;
- changement discontinu possible de l'atlas sous thinning ;
- correspondance inter-vues plus difficile ;
- le tokenizer devient lui-même un problème d'optimisation appris.

**Décision**

Voie de repli pertinente si la représentation globale compacte perd trop d'information. Ne pas ouvrir un atlas appris avant d'avoir quantifié le sous-ensemble de polyèdres réellement mal représentés.

### 3.3 Encodeur natif de la surface maillée

Les facettes, arêtes et sommets forment un graphe surfacique. Un GNN ou Transformer de faces produit un code global fixe.

**Avantages**

- aucune hypothèse de radialité ;
- connectivité, interfaces et bords conservés ;
- représentation fidèle de la surface discrète ;
- précédent direct avec PolyhedronNet et les autoencodeurs de graphes de maillages.

**Limites**

- coût variable avec le nombre de facettes ;
- sensibilité au remeshing si les opérateurs ne sont pas pondérés par l'aire ;
- invariance à la densité moins directe ;
- nouveauté architecturale limitée si cette branche est utilisée seule.

**Décision**

C'est le **référentiel riche** et la meilleure solution de repli. Une branche légère de connectivité complète la représentation principale ; un encodeur complet sert de plafond.

### 3.4 Champ implicite, UDF ou fonction de séparation

Les UDF et les fonctions de séparation de type GIFS peuvent représenter des surfaces ouvertes ou multicouches.

**Avantages**

- topologies générales ;
- représentation continue ;
- utile pour complétion, génération et interrogation spatiale.

**Limites**

- ajuster un champ neural à une surface polyédrique déjà connue est redondant ;
- coût d'apprentissage ou d'échantillonnage par polyèdre ;
- extraction de surface non triviale ;
- aucune invariance au capteur n'est obtenue gratuitement.

**Décision**

Pas de tokenizer principal. À réserver au décodeur génératif ou à une tâche de complétion.

### 3.5 Carte 2,5D orientée capteur

On projette la surface dans un plan tangent à la direction capteur–polyèdre et on encode profondeur résiduelle, normales et masque.

**Avantages**

- cohérent avec la surface telle qu'elle est vue ;
- souvent proche d'un régime monocouche ;
- encode naturellement occultation et qualité d'observation.

**Limites**

- dépend du point de vue ;
- instable aux incidences rasantes ;
- le champ angulaire change avec la portée ;
- ne représente pas à lui seul une forme intrinsèque.

**Décision**

Canal auxiliaire `view`, séparé du code de forme. Il peut améliorer la segmentation sans porter le claim de tokenizer universel.

## 4. Objet mathématique retenu : mesure surfacique attribuée normalisée

### 4.1 Définition

Choisir une ancre déterministe `c_v`, une échelle robuste `s_v>0` et

```math
T_{c_v,s_v}(x)=\frac{x-c_v}{s_v}.
```

À la surface munie de ses attributs, associer la mesure

```math
\mu_v
=
(T_{c_v,s_v})_\#
\left(
\frac{w_v(x)}{s_v^2}
\,d\mathcal H^2_{\mid\Sigma_v}(x)
\right).
```

`w_v(x)` peut contenir plusieurs canaux : masse, confiance, attribut de bord, rémission et information de normale. Pour une normale dont l'orientation n'est pas garantie, utiliser le projecteur non orienté `n(x)n(x)^T` ou des quantités absolues ; une normale signée n'est admise que si son orientation est contractuellement définie.

La **mesure surfacique attribuée normalisée** est l'objet de référence. Elle ne sélectionne pas une intersection par direction : toute la surface contribue.

### 4.2 Propriétés exactes

Sous une similitude

```math
\Sigma'=aR\Sigma+b,
\qquad
c'=aRc+b,
\qquad
s'=as,
```

on a

```math
T_{c',s'}(aRx+b)=R\,T_{c,s}(x)
```

et

```math
\frac{dA'}{(s')^2}=\frac{dA}{s^2}.
```

La mesure de forme est donc :

- invariante à la translation ;
- invariante à l'homothétie ;
- équivariante à la rotation.

L'aire physique `A(Σ_v)` et l'échelle `s_v` restent dans le canal métrique. L'invariance de forme ne les efface donc pas du modèle.

La mesure exacte détermine le support géométrique de la surface à des ensembles d'aire nulle près. Elle ne conserve pas nécessairement la combinatoire des facettes, les coutures de mesure nulle ni une multiplicité de feuilles exactement superposées. Ces informations restent dans le graphe d'incidence.

### 4.3 Remeshing

La définition dépend de la surface et de sa mesure d'aire, non du nombre de triangles. Deux remeshings convergents doivent produire le même descripteur à l'erreur de quadrature près.

C'est une propriété centrale à tester : un encodeur qui change fortement lorsqu'un triangle est subdivisé n'a pas appris la surface, seulement les habitudes du triangulateur.

## 5. Discrétisation principale : base sphéro-radiale douce

### 5.1 Passage en coordonnées polaires

Pour `y=T_{c,s}(x)`, écrire

```math
u(y)=\frac{y}{\|y\|},
\qquad
r(y)=\|y\|.
```

La mesure peut être projetée sur une icosphère et une base radiale :

```math
X_v[j,b]
=
\int_{\Sigma_v}
\kappa_j(u(x))
\,\phi_b(\log(r(x)+\varepsilon))
\,q_v(x)
\,\frac{dA(x)}{s_v^2}.
```

- `κ_j` : noyaux angulaires doux ;
- `φ_b` : fonctions radiales ou splines ;
- `q_v` : attributs locaux ;
- `X_v∈R^{M×B×C}` : tenseur fixe indépendant du nombre de retours et de facettes.

Cette **grille sphéro-radiale de mesure** encode naturellement zéro, une ou plusieurs couches dans une même direction. La carte radiale monocouche est un cas particulier beaucoup plus pauvre.

Utiliser des noyaux doux plutôt que des bins durs. Les histogrammes durs changent brutalement lorsqu'une facette traverse une frontière de case, détail numérique que les réseaux transforment ensuite avec une remarquable efficacité en faux signal scientifique.

### 5.2 Variante spectrale

Comparer la grille apprise à des moments

```math
a_{k\ell m}
=
\int_{\Sigma_v}
R_k(r(x))Y_{\ell m}(u(x))
q_v(x)
\frac{dA(x)}{s_v^2}.
```

Les descripteurs par harmoniques sphériques et moments de Zernike sont des antécédents importants. La nouveauté ne peut donc pas être revendiquée sur le simple fait de développer une surface dans une base sphéro-radiale. Ces moments constituent une baseline analytique et peuvent initialiser ou régulariser l'encodeur appris.

### 5.3 Variante par quadrature d'aire

Échantillonner ou intégrer un nombre fixe de sites selon l'aire, puis appliquer un Deep Sets ou Set Transformer :

```math
z_v=\operatorname{SetEncoder}
\left\{
\left(T_{c,s}(x_i),n_i,a_i\right)
\right\}_{i=1}^{N_q}.
```

Ces sites ne sont pas les retours LiDAR bruts : ce sont des points de quadrature de la surface reconstruite. Cette baseline vérifie si la grille sphéro-radiale apporte autre chose qu'une manière compliquée de faire un pooling de surface.

## 6. Ancre, repère et conditionnement

### 6.1 Ancre

Baselines déterministes :

- barycentre surfacique ;
- médiane géométrique surfacique ;
- centre du miniball ;
- centre régularisé par le parent.

Le barycentre d'une feuille presque plane peut appartenir à la surface. Cela n'invalide pas la mesure cartésienne normalisée, mais peut rendre sa projection polaire mal conditionnée et concentrer la masse sur une bande angulaire. Mesurer :

- distance de l'ancre à la surface ;
- entropie de l'occupation angulaire ;
- masse proche de `r=0` ;
- variation du code sous perturbation de l'ancre.

Si une ancre unique est instable, utiliser un petit ensemble déterministe d'ancres, encodé par poids partagés puis agrégé comme un ensemble. Une paire symétrique de part et d'autre d'un plan local fiable ou quelques offsets dans le repère gravité–capteur sont préférables à un atlas libre immédiatement appris.

### 6.2 Repère

Pour SemanticKITTI, le repère principal est :

```text
gravité
rayon horizontal capteur → centre
orthogonale tangentielle
```

Il conserve des orientations physiquement utiles. Comparer à :

- repère ego ;
- code invariant à la rotation ;
- encodeur `SO(3)`-équivariant ;
- augmentation de lacet.

La PCA orientée n'est qu'une ablation : ses axes changent de signe ou de rang près des symétries.

## 7. Connectivité résiduelle

La discrétisation compacte peut perdre des bords fins ou la combinatoire locale. Trois corrections sont comparées :

1. scalaires topologiques et spectraux : composantes, bords, degrés, premiers modes du Laplacien dual ;
2. petite branche `SurfaceGraph` sur un maillage simplifié avec poids d'aire ;
3. encodeur complet de surface-attributed graph, utilisé comme plafond.

La voie cible est :

```text
mesure surfacique normalisée
   + grille ou moments sphéro-radiaux
   + résidu léger de connectivité
   → vecteur fixe du polyèdre
```

Le résidu complet ne devient le backbone que si la courbe taux–distorsion réfute la compression compacte.

## 8. Encodeur local recommandé

Configuration pilote :

```yaml
surface_object: normalized_attributed_surface_measure
surface_discretization: soft_spherical_radial_grid
angular_cells: 162
radial_basis: 12
channels:
  - normalized_area_mass
  - normal_projector_6
  - incidence
  - boundary_mass
  - remission
  - confidence
summary_tokens: 4
surface_dim: 192
surface_blocks: 4
surface_heads: 6
anchor_mode: robust_surface_center
frame: gravity_sensor_tangent
connectivity_residual: light
```

Traitement :

1. mélange radial partagé dans chaque cellule angulaire ;
2. convolution ou attention locale sur le graphe icosaédrique ;
3. tokens de résumé ;
4. fusion gated avec le code de connectivité ;
5. projection vers `z_v^{shape}`.

Les résolutions `M∈{42,162,642}` et `B∈{4,8,12,16}` tracent la courbe taux–distorsion. Les grandes valeurs ne sont ouvertes qu'après preuve que les cellules supplémentaires portent autre chose qu'une collection onéreuse de zéros.

## 9. Batterie de comparaison du tokenizer

À budget latent, paramètres et données appariés :

| ID | Représentation | Rôle |
|---|---|---|
| P0 | moments, spectre, shape distributions | baseline analytique |
| P1 | radial monocouche | intuition initiale |
| P2 | radial `K` couches | multiplicité explicite |
| P3 | moments sphéro-radiaux / Zernike-like | baseline spectrale |
| P4 | grille de mesure sphéro-radiale | voie compacte principale |
| P5 | P4 multi-ancre | robustesse au centre |
| P6 | quadrature d'aire + Set Encoder | contrôle sans grille |
| P7 | atlas multi-cartes | solution locale générale |
| P8 | SurfaceGraph / PolyhedronNet-like | plafond riche |

Métriques :

- reconstruction surfacique et erreur de masse ;
- Chamfer et Hausdorff pondérées par l'aire ;
- cohérence des normales et rappel des bords ;
- invariance au remeshing ;
- sensibilité à l'ancre ;
- stabilité au thinning, à la portée et au changement de capteur ;
- linear probe sémantique ;
- probe capteur, anneau, portée et thinning ;
- octets, FLOPs et latence.

Le résultat central doit être une **frontière de Pareto**, pas seulement le meilleur mIoU d'une configuration trouvée après suffisamment de variations pour que le hasard finisse par collaborer.

## 10. Décision

La voie principale retenue est :

```text
surface polyédrique HGP
    → mesure surfacique attribuée normalisée
    → projection sphéro-radiale douce
    → encodeur de surface
    → résidu léger de connectivité
    → code fixe du polyèdre
```

La radialité monocouche reste une baseline et un diagnostic. L'atlas multi-cartes est le repli pour les surfaces localement mal conditionnées. L'encodeur natif de maillage est le référentiel riche et peut devenir le backbone si les données réfutent la compression proposée.

Le changement de paradigme ne dépend donc pas de l'existence universelle d'une fonction radiale. Il dépend de la possibilité de remplacer l'échantillon ponctuel par une **mesure surfacique hiérarchique, normalisée et transférable**.
