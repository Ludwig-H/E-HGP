# Guide du projet

## 1. L'idée en une phrase

Au lieu de demander à un réseau de reconstruire mentalement des objets à partir d'un échantillon LiDAR irrégulier, on lui fournit directement une hiérarchie de **surfaces polyédriques observées**, puis on apprend comment ces surfaces se ressemblent, croissent et fusionnent.

Le changement proposé est donc celui de l'unité de calcul :

```text
points / voxels / superpoints
          ↓
polyèdres HGP porteurs d'une surface explicite
```

Une facette est un élément de discrétisation. Un polyèdre HGP est le recollement de facettes qui forme l'objet géométrique appris. La hiérarchie décrit l'évolution de ces objets lorsque le niveau de densité varie.

## 2. Pourquoi cela peut être supérieur aux points

À grande portée, une voiture fournit moins de retours, des trous différents et des facettes plus grossières. Un modèle point-wise doit apprendre simultanément :

- la géométrie de la voiture ;
- le motif du capteur ;
- la densité d'échantillonnage ;
- les occultations ;
- la sémantique.

La construction HGP vise à factoriser une partie de ce travail :

```text
échantillonnage variable
      → surface polyédrique
      → code de taille fixe
```

Le code de forme est normalisé ; la taille physique, la pose et la qualité d'acquisition restent disponibles dans des canaux séparés. Le modèle n'est donc pas aveugle au monde métrique, simplement moins dépendant du nombre accidentel de retours.

## 3. Ce qu'un polyèdre doit contenir

Pour un nœud `v`, le réseau reçoit conceptuellement :

```text
Σ_v       surface = recollement de facettes
λ_v       niveau de densité
m_v       masse / aire / nombre de retours
p(v)      parent dans la hiérarchie
ch(v)     enfants lors de la fusion
N(v)      voisins spatiaux latéraux
A_v       attributs capteur et confiance
```

Le polyèdre peut être ouvert, non convexe et partiellement occulté. Rien n'autorise à le remplacer silencieusement par son enveloppe convexe.

## 4. Pourquoi une simple fonction radiale ne suffit pas toujours

Depuis un centre `c`, une direction peut :

- ne rencontrer aucune facette ;
- rencontrer une seule couche ;
- rencontrer plusieurs couches.

Une fonction `ρ(u)` ne décrit que le deuxième cas. Elle reste une bonne baseline lorsque la surface est quasi étoilée, mais pas une définition universelle.

La représentation principale commence par la **mesure surfacique attribuée normalisée** dans un repère propre au polyèdre. Elle est ensuite projetée dans une base douce direction–rayon, c'est-à-dire sur

```math
\mathbb S^2\times\mathbb R.
```

La mesure conserve toute la surface, sans sélectionner une couche par direction. Sa discrétisation accepte donc naturellement les trous et les intersections multiples, puis devient un tenseur fixe encodé par un réseau sphérique/radial.

Voir [REPRESENTATION_POLYEDRIQUE.md](REPRESENTATION_POLYEDRIQUE.md).

## 5. Les cinq informations à ne pas confondre

### Forme normalisée

Ce qui doit rester stable sous translation, changement d'échelle et rééchantillonnage :

- mesure surfacique normalisée et sa projection sphéro-radiale ;
- normales et bords normalisés ;
- connectivité ou résumé spectral ;
- confiance géométrique.

### Grandeurs physiques

Ce qui ne doit pas être normalisé hors d'existence :

- dimensions métriques ;
- centre, hauteur et orientation ;
- aire et épaisseur ;
- position dans la scène.

### Filtration

Ce qui décrit l'espace d'échelle :

- naissance, mort et persistance ;
- écarts de niveaux logarithmiques ;
- rang ou quantile du niveau ;
- gain de masse ;
- degré et type de fusion.

### Acquisition

Ce qui décrit la qualité de la mesure :

- portée et incidence ;
- anneaux et couverture angulaire ;
- rémission ;
- trous, occultation et confiance.

### Sémantique apprise

Ce qui doit transférer entre observations :

- identité géométrique ;
- partie d'objet ;
- classe ;
- frontière et relation fonctionnelle.

Le pré-entraînement aligne principalement le sous-espace sémantique et de forme. Il ne force pas la taille, la pose et les attributs capteur à devenir identiques.

## 6. Architecture en trois échelles

### À l'intérieur d'un polyèdre

Un `SurfaceEncoder` lit la projection sphéro-radiale de la mesure de surface et produit un ou quelques tokens. Une petite branche de graphe de facettes conserve explicitement la connectivité et les bords ; son poids est audité à budget égal.

### Le long d'une branche

Un même objet HGP évolue avec le niveau de densité. Le `BranchEncoder` traite la suite de ses états en utilisant les vrais écarts de niveau, et non de simples indices `0,1,2,3`.

### Aux fusions et dans la scène

Un `MergeEventEncoder` combine les enfants comme un ensemble. Des arêtes latérales relient les polyèdres voisins qui ne sont pas encore réunis dans l'arbre. Quelques latents globaux peuvent apporter le contexte de scène si l'arbre seul crée un goulot d'étranglement.

```text
mesure + grille de chaque surface
        │
        ▼
SurfaceEncoder partagé
        │
        ├─ BranchEncoder : évolution en densité
        ├─ MergeEventEncoder : fusions
        └─ LateralEncoder : voisinage spatial
                    │
                    ▼
             tokens polyédriques
                    │
                    ▼
       décodeur par facette → points
```

## 7. Pourquoi les facettes restent utiles

Les facettes ne sont plus l'unité principale du backbone, mais elles restent :

- le support exact de la surface ;
- le support des attributs locaux ;
- la résolution de décodage ;
- l'interface de reprojection vers les points.

Pour une facette `τ`, le décodeur lit son embedding local et les embeddings des polyèdres ancêtres qui la contiennent. Il produit des logits par facette, puis

```math
\ell_x=\sum_{\tau\ni x}w_{x\tau}\ell_\tau.
```

Aucun token point n'est nécessaire dans le modèle principal.

## 8. Comment le modèle apprend sans labels

### Masquage surfacique

Masquer des secteurs contigus dans la grille `S²×R` ou des groupes de facettes, puis prédire l'embedding teacher de la partie cachée.

### Changement de densité

Construire indépendamment :

```text
teacher : scan dense
student : même scan avec capteur simulé plus pauvre
```

Apparier uniquement les polyèdres réellement correspondants et aligner leur code de forme.

### Temps

Les séquences LiDAR observent la même géométrie depuis plusieurs distances. Après compensation du mouvement, elles fournissent la supervision naturelle la plus forte pour l'invariance à la portée.

### Images

Dans une phase ultérieure, projeter les features d'un modèle visuel sur les facettes et les distiller vers les polyèdres. Les caméras restent absentes à l'inférence LiDAR-only.

## 9. Comment savoir si l'idée est vraie

Il faut répondre positivement à cinq questions distinctes :

1. **Radialité.** Quelle part des surfaces est monocouche ?
2. **Fidélité.** Peut-on reconstruire la surface depuis le code à budget raisonnable ?
3. **Stabilité.** Le code change-t-il moins que les points et superpoints sous thinning et remeshing ?
4. **Sémantique.** Un probe simple sépare-t-il les classes et les parties ?
5. **Hiérarchie.** La trajectoire HGP apporte-t-elle plus que le meilleur polyèdre isolé ?

Ces questions sont testées avant les campagnes coûteuses. La taille du modèle n'est pas une sixième métrique scientifique, malgré l'enthousiasme administratif qu'elle semble parfois susciter.

## 10. Ce qui constituerait un modèle de fondation

Un modèle entraîné sur SemanticKITTI seul reste un backbone spécialisé. Le statut fondation exige :

- pré-entraînement sur plusieurs datasets et capteurs ;
- linear probing et faible supervision ;
- transfert sans modifier le tokenizer ;
- plusieurs tâches : sémantique, instance, détection, complétion ou recherche ;
- amélioration avec l'échelle des données ;
- comparaison à des pré-entraînements point/voxel modernes.

La nouveauté visée est alors précise :

> apprendre des représentations générales sur un espace d'échelle de surfaces polyédriques explicites, plutôt que sur l'échantillon ponctuel fourni par un capteur particulier.
