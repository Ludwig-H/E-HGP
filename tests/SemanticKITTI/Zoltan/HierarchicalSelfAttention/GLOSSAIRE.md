# Glossaire normatif

## Objets géométriques

**Facette**  
Cellule surfacique élémentaire de la construction HGP. Elle porte géométrie, aire, normale ou projecteur normal, attributs et incidences. Elle sert de discrétisation et de résolution de sortie, pas de token principal du backbone.

**Polyèdre HGP**  
Recollement de facettes associé à un nœud ou un état de branche de la filtration. Il représente une surface effectivement observée, éventuellement ouverte, non convexe, multicouche ou non-manifold. Dans ce dossier, il ne désigne ni une enveloppe convexe ni un simple cluster de points.

**Surface observée**  
Support géométrique fourni par les facettes pour une acquisition donnée. Elle peut différer de la surface physique complète de l'objet à cause des occultations et de la géométrie du capteur.

**Carrier**  
Réalisation géométrique exacte portée par une cellule, un ensemble de facettes ou un nœud. Toujours préciser le carrier utilisé dans une métrique.

**Bord**  
Arête de facette incidente à une seule face dans le sous-complexe considéré, ou frontière explicitement déclarée. Les bords peuvent être géométriques, d'occultation ou de troncature.

**Remeshing**  
Modification de la triangulation conservant approximativement la même surface géométrique. L'invariance au remeshing est une propriété centrale du tokenizer de surface.

**Surface non-manifold**  
Surface dont certaines arêtes ou certains sommets ne possèdent pas un voisinage de variété 2D. Elle n'est pas automatiquement rejetée, mais reçoit des indicateurs de validité et d'incertitude.

## Hiérarchie HGP

**Filtration**  
Famille emboîtée de surfaces ou sous-complexes indexée par un niveau de densité `λ`.

**Niveau de densité**  
Valeur physique ou statistique de la filtration. Elle est distincte d'un numéro de couche ou d'un nombre de clusters.

**Nœud HGP**  
État persistant de la hiérarchie portant sa propre surface polyédrique, ses attributs et ses relations. Un nœud interne n'est pas seulement un objet auxiliaire de pooling.

**Branche**  
Suite ordonnée d'états d'une même composante entre sa naissance et sa fusion. Elle décrit une trajectoire de surface en fonction de la densité.

**Événement de fusion**  
Événement où plusieurs branches deviennent un parent commun. Les enfants forment un ensemble non ordonné.

**Persistance**  
Étendue de vie d'une branche dans le paramètre de filtration. Le dossier distingue persistance en rang, en quantile et en différence logarithmique de niveau.

**Delta de facettes**  
Ensemble

```math
\Delta F_t=F_{t+1}\setminus F_t
```

des facettes activées entre deux états emboîtés. Il représente une innovation géométrique exacte lorsqu'il est disponible.

**Innovation latente**  
Différence apprise entre le code directement observé d'un parent et sa prédiction depuis les enfants. Elle n'est pas appelée quantité géométrique exacte.

**Arête latérale**  
Relation spatiale entre polyèdres de branches différentes à des niveaux compatibles.

**Arbre aléatoire apparié**  
Null test conservant approximativement profondeur, degrés et masses de l'arbre réel tout en randomisant les associations.

## Représentation de surface

**Ancre**  
Point `c_v` servant à centrer la surface. Son choix est déterministe, sans labels, et sa sensibilité est mesurée.

**Échelle robuste**  
Scalaire `s_v>0` utilisé pour normaliser la surface. La taille physique reste disponible dans un canal séparé.

**Mesure surfacique attribuée normalisée**  
Mesure de référence

```math
\mu_v
=
(T_{c_v,s_v})_\#
\left(
\frac{w_v(x)}{s_v^2}
\,d\mathcal H^2_{\mid\Sigma_v}(x)
\right),
\qquad
T_{c,s}(x)=\frac{x-c}{s}.
```

Elle encode toute la masse de surface et ses attributs, sans choisir une intersection par direction. Elle est invariante aux translations et homothéties, et équivariante aux rotations.

**Carte radiale monocouche**  
Fonction `ρ(u)` donnant l'unique rayon d'intersection d'une surface avec une direction. Elle n'est définie que lorsque la multiplicité radiale vaut un.

**Multiplicité radiale**  
Nombre

```math
N_{\Sigma,c}(u)
=
|\{r>0:c+ru\in\Sigma\}|.
```

Il peut valoir zéro, un ou davantage.

**Radialité**  
Propriété empirique d'une surface pour laquelle une grande part des directions utiles possède une intersection unique et stable. Elle est mesurée, jamais supposée universelle.

**Radial-`K`**  
Représentation stockant les `K` premières intersections ordonnées par direction, avec masques.

**Grille sphéro-radiale de mesure**  
Discrétisation douce de la mesure surfacique sur des cellules angulaires et une base radiale. Elle peut contenir plusieurs couches dans une même direction.

**Moments sphéro-radiaux**  
Coefficients obtenus en projetant la mesure sur des bases radiales et des harmoniques sphériques. Les moments de Zernike et descripteurs harmoniques sont des antécédents importants.

**Quadrature d'aire**  
Ensemble pondéré de sites échantillonnés ou intégrés selon l'aire de la surface. Ces sites proviennent du polyèdre, pas des retours LiDAR bruts.

**Atlas multi-cartes**  
Collection de paramétrisations locales 2D couvrant la surface. C'est une solution générale mais plus difficile à stabiliser entre acquisitions.

**SurfaceGraph**  
Encodeur du graphe des facettes, arêtes et sommets, pondéré par l'aire et conçu pour conserver connectivité et bords.

**Projecteur normal**  
Matrice `nnᵀ` représentant une normale sans choisir son signe. Utilisée lorsque l'orientation des faces n'est pas globalement fiable.

**Taux–distorsion**  
Courbe reliant erreur géométrique ou sémantique au nombre d'octets, de coefficients, de tokens ou de FLOPs.

## Canaux du token

**Canal `shape`**  
Forme normalisée issue de la mesure surfacique, de sa grille et du résidu de connectivité.

**Canal `metric`**  
Taille, aire physique, centre, hauteur, pose et localisation.

**Canal `filter`**  
Niveau, rang, quantile, persistance, croissance et type d'événement.

**Canal `sensor`**  
Portée, anneaux, rémission, incidence, couverture, occultation et confiance.

**Canal `topo`**  
Résumé de la connectivité : composantes, bords, degrés, indicateurs non-manifold et spectre du graphe dual.

**Repère gravité–capteur–tangent**  
Repère local utilisant la gravité, la direction horizontale du capteur vers le polyèdre et leur orthogonale. Il conserve une orientation physiquement utile pour le LiDAR outdoor.

## Architecture

**`HGP-PolyFM`**  
Nom de travail du modèle natif des surfaces polyédriques et de leur espace d'échelle. Le nom ne constitue pas un claim de nouveauté ni de statut fondation.

**`SurfaceEncoder`**  
Encodeur local partagé produisant un code fixe depuis la mesure de surface, sa grille et les informations de connectivité.

**`BranchEncoder`**  
Encodeur de la trajectoire de surface le long d'une branche, positionné par les vrais écarts de niveau.

**`MergeEventEncoder`**  
Agrégateur permutation-invariant des enfants, relations et innovations d'un événement de fusion.

**`LateralGraph`**  
Message passing spatial entre polyèdres voisins de branches différentes.

**Décodeur par facette**  
Tête combinant une feature locale de facette et les tokens de ses ancêtres pour produire des logits ensuite reprojetés vers les points.

**SPT-nano-poly**  
Adaptation de Superpoint Transformer servant de baseline de hiérarchie U-Net sur la structure polyédrique.

**Sequoia-fixed**  
Baseline parent–enfants–frères utilisant la hiérarchie HGP fixée hors gradient.

**HSA**  
Hierarchical Self-Attention. Baseline théorique sous contrainte hiérarchique ; son théorème publié ne couvre pas automatiquement des features surfaciques propres à tous les nœuds internes.

**MeanTree**  
Baseline sans attention utilisant moyenne ou somme pondérée et MLP.

## Apprentissage

**`Surface-JEPA`**  
Prédiction latente de parties ou secteurs masqués d'une même surface polyédrique.

**`Cross-Range PolyJEPA`**  
Pré-entraînement teacher–student entre deux acquisitions dont les surfaces et hiérarchies HGP sont reconstruites indépendamment.

**Observable-only**  
Règle selon laquelle une cible inter-vues contribue seulement si une correspondance géométrique suffisamment fiable existe.

**Teacher EMA**  
Encodeur cible mis à jour par moyenne exponentielle des paramètres du student.

**Sous-espace `shape`**  
Partie du latent fortement alignée entre acquisitions.

**Sous-espace `sensor`**  
Partie conservant qualité et conditions de mesure ; elle n'est pas forcée à être invariante.

**Probe sémantique**  
Tête simple entraînée sur des embeddings gelés pour mesurer l'information sémantique.

**Probe capteur**  
Tête diagnostique prédisant portée, anneau, capteur ou thinning. Elle révèle les raccourcis d'acquisition.

**Frozen probing**  
Évaluation où le backbone est gelé et seules des têtes légères sont entraînées.

**Faible supervision**  
Fine-tuning avec une faible fraction des labels, typiquement `0.1 %`, `1 %` ou `10 %`.

## Évaluation

**Oracle de facettes**  
Meilleure sortie possible lorsque chaque facette connaît sa distribution GT.

**Oracle de polyèdres**  
Meilleure sortie possible lorsque chaque polyèdre connaît sa distribution GT.

**Oracle multi-ancêtres**  
Plafond utilisant plusieurs nœuds de la branche pour chaque facette.

**Retrieval cross-range**  
Recherche d'une même surface ou partie observée à une autre portée ou avec un autre capteur.

**Invariance au remeshing**  
Stabilité du token lorsque la triangulation change mais que la surface reste la même.

**Robustesse à la portée**  
Stabilité sous la modification réelle de l'acquisition liée à la distance : densité, incidence, trous et occultations. Elle est distincte de l'invariance à l'homothétie.

**Frontière de Pareto**  
Ensemble des représentations non dominées simultanément en fidélité, invariance, sémantique et coût.

**Track strict**  
SemanticKITTI mono-scan, LiDAR seul, sans TTA, ensemble ni accumulation temporelle à l'inférence.

**Backbone spécialisé**  
Encodeur entraîné et évalué principalement sur un dataset ou une tâche.

**Foundation model LiDAR outdoor**  
Encodeur pré-entraîné sur plusieurs datasets et capteurs LiDAR, transférable à plusieurs tâches avec peu de labels et sans modifier le tokenizer.

**Foundation model 3D général**  
Claim plus large exigeant plusieurs domaines 3D, orientations, densités et modalités. Il n'est pas ouvert par SemanticKITTI seul.
