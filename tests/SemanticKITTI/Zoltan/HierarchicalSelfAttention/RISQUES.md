# Risques, réfutations et solutions de repli

## 1. Carte des risques

La hiérarchie HGP est supposée calculable et peu coûteuse. Les risques portent donc sur ce qui vient après : la nature des polyèdres, leur représentation, l'apprentissage et la portée du claim.

| ID | Risque | Gravité | Test précoce | Réponse |
|---|---|---:|---|---|
| R1 | les polyèdres ne correspondent pas à des unités sémantiques utiles | critique | G2 | conserver facettes/deltas, revoir le décodage |
| R2 | la radialité monocouche échoue | élevé, non fatal | G1 | mesure surfacique, `K` couches ou atlas |
| R3 | la grille compacte perd bords, couches ou connectivité | critique pour le code compact | G1/G4 | résidu SurfaceGraph ou backbone mesh-native |
| R4 | l'ancre et le repère sont instables | élevé | G1/G3 | ancre robuste, multi-ancre, équivariance |
| R5 | l'invariance d'échelle ne devient pas robustesse à la portée | critique pour le claim | G3 | réduire le claim, améliorer le SSL capteur |
| R6 | les états hiérarchiques dupliquent la même surface | élevé | G5 | deltas de facettes / innovations |
| R7 | l'arbre HGP n'apporte rien au polyèdre isolé | critique pour la hiérarchie | G5 | papier tokenizer ou arrêt du claim HGP |
| R8 | les arbres de deux acquisitions ne sont pas appariables | élevé | G3/G6 | matching partiel, temps réel, intra-arbre |
| R9 | le matching sélectionne seulement les grandes surfaces faciles | élevé | G6 | stratification, plafonnement, OT sparse |
| R10 | le modèle apprend le capteur plutôt que la forme | élevé | G3/G6 | sous-espaces, dropout, probes, transfert |
| R11 | le décodeur par facette détruit les petites classes | critique | G2 | décodeur de bord ou raffinement sans labels |
| R12 | la complexité architecturale masque l'apport du tokenizer | élevé | plan factoriel | ordre strict des ablations |
| R13 | le nombre de polyèdres ou cellules de surface explose | élevé | profiling | budget, deltas, compression, échantillonnage |
| R14 | la représentation ne transfère qu'entre scènes routières proches | critique pour fondation | G7 | élargir les données ou réduire le claim |
| R15 | les gains viennent de la caméra teacher | élevé | S5/S6 | preuve géométrie-only préalable |

## 2. R1 — Polyèdre géométrique, mais pas unité sémantique

Un polyèdre HGP peut suivre une surface physiquement cohérente sans coïncider avec un objet ou une partie sémantique. Une façade et un panneau coplanaires peuvent fusionner ; une voiture peut être répartie sur plusieurs branches.

### Tests

- oracle par polyèdre, facette et branche ;
- pureté sémantique contre persistance et niveau ;
- rappel de frontières ;
- comparaison aux superpoints à nombre de tokens égal ;
- mesure de la fragmentation d'une instance sur l'arbre.

### Réponse

Le backbone reste polyédrique, mais la sortie ne doit pas être un label rigide par nœud. Conserver :

- les facettes comme support de décodage ;
- les embeddings de plusieurs ancêtres ;
- les deltas de surface ;
- une tête de frontière ;
- une tête d'affiliation d'instance.

La hiérarchie peut fournir un contexte utile sans que chaque nœud soit lui-même une instance parfaite.

## 3. R2 — Non-unicité radiale

Le risque est réel mais ne réfute pas le paradigme. Il réfute seulement la fonction `ρ(u)` comme représentation universelle.

### Diagnostics

- histogramme de multiplicité par direction ;
- masse d'aire en régime multicouche ;
- stabilité de l'ordre des couches ;
- résultats par classe et portée ;
- sensibilité au centre.

### Réponses ordonnées

1. mesure surfacique attribuée normalisée ;
2. grille sphéro-radiale douce conservant toutes les couches ;
3. multi-ancre si la projection est mal conditionnée ;
4. atlas multi-cartes pour les surfaces localement pathologiques ;
5. encodeur mesh-native si la compression reste insuffisante.

Le projet ne doit jamais transformer un échec de radialité en suppression silencieuse des facettes problématiques.

## 4. R3 — Perte due à la discrétisation compacte

La mesure continue peut être fidèle tandis que sa grille `M×B` lisse :

- deux couches radiales proches ;
- une tige fine ;
- un bord aigu ;
- une petite composante ;
- une couture ou une structure grillagée.

### Tests

- courbes `M×B` contre Chamfer, Hausdorff, bords et mIoU ;
- cas synthétiques de fréquence contrôlée ;
- comparaison à quadrature d'aire et SurfaceGraph ;
- erreur pondérée par classe et taille ;
- remeshing à géométrie constante.

### Réponse

Le modèle cible inclut toujours des scalaires de connectivité. Une branche `SurfaceGraph` légère est ouverte si elle améliore le taux–distorsion à budget apparié. Si elle domine systématiquement, elle devient le backbone local et la grille reste un canal global de forme.

La nouveauté porte alors sur le token polyédrique hiérarchique, non sur une projection particulière.

## 5. R4 — Ancre et repère instables

Le barycentre d'une surface ouverte peut être proche ou appartenir à la surface. Une petite perturbation peut alors redistribuer fortement la masse angulaire. La PCA ajoute ses retournements de signe et permutations d'axes.

### Tests

- distance ancre–surface ;
- entropie angulaire ;
- masse près de `r=0` ;
- jacobien numérique du descripteur par rapport à l'ancre ;
- perturbations et remeshing ;
- symétries synthétiques.

### Réponses

- ancre robuste choisie sans labels ;
- centre régularisé par la branche ;
- petit ensemble d'ancres déterministes ;
- agrégation permutation-invariante des ancres ;
- repère gravité–capteur–tangent ;
- variante `SO(3)`-équivariante pour transfert général.

Une ancre apprise librement n'est envisagée qu'après ces baselines : sinon le réseau peut déplacer le repère pour cacher les défauts du tokenizer.

## 6. R5 — Homothétie et portée ne sont pas la même chose

La normalisation

```math
x\mapsto\frac{x-c}{s}
```

retire une similitude géométrique. Éloigner un objet dans un nuage métrique calibré ne l'homothétise pas : cela change le nombre de retours, l'angle d'incidence, les trous et les occultations.

### Test décisif

Comparer séparément :

1. similitudes appliquées à une même surface ;
2. remeshing d'une même surface ;
3. thinning synthétique ;
4. observation réelle du même support à plusieurs portées ;
5. changement de capteur.

### Décision

- invariance seulement aux similitudes : propriété géométrique, claim limité ;
- robustesse au thinning mais pas aux occultations : claim de rééchantillonnage ;
- robustesse réelle cross-range/cross-capteur : claim perceptif fort.

Les trois ne sont pas interchangeables, même si une seule figure logarithmique pourrait commodément les superposer.

## 7. R6 — Duplication multi-échelle

Une même facette peut contribuer à de nombreux ancêtres. Traiter chaque état comme une observation indépendante :

- surcompte la surface ;
- augmente le coût ;
- facilite des raccourcis par masse ou profondeur ;
- favorise le sur-lissage.

### Réponse principale

Lorsque possible, encoder les deltas exacts :

```math
\Delta F_t=F_{t+1}\setminus F_t.
```

Le token d'état et le token d'innovation sont distincts. À défaut, calculer une innovation latente et contrôler qu'elle apporte plus qu'une différence de masse.

### Null tests

- états complets répétés ;
- deltas seuls ;
- état + delta ;
- facettes activées à des niveaux permutés ;
- même budget de tokens.

## 8. R7 — Hiérarchie inutile

Le polyèdre isolé et le graphe latéral peuvent déjà suffire. L'arbre peut n'ajouter qu'une manière coûteuse de diffuser du contexte.

### Contrôles

- modèle plat ;
- graphe latéral ;
- arbre aléatoire apparié ;
- octree ;
- HDBSCAN/RSL ;
- HGP sans niveaux ;
- HGP avec niveaux permutés ;
- HGP complet.

### Décision

Si l'arbre réel n'améliore ni sémantique, ni robustesse, ni faible supervision, le papier peut rester centré sur le tokenizer de surfaces. Il ne faut pas garder « HGP » dans le claim principal par attachement familial.

## 9. R8/R9 — Matching inter-arbres rare ou biaisé

Deux acquisitions peuvent produire des polyèdres de granularités différentes. Un matching basé uniquement sur le meilleur recouvrement sélectionne les routes, bâtiments et grands objets proches.

### Mesures obligatoires

- couverture par classe, portée, aire, persistance et multiplicité ;
- précision estimée des matches ;
- collisions un-à-plusieurs ;
- taux de rejet ;
- fraction de la loss par strate ;
- performance lorsque les poids de matching sont uniformisés.

### Réponses

1. matching partiel many-to-many par masse surfacique ;
2. transport optimal sparse entre facettes ou mesures ;
3. cohérence de branche plutôt que nœud exact ;
4. teacher temporel agrégé ;
5. Surface-JEPA intra-polyèdre lorsque le matching est impossible ;
6. curriculum du thinning faible vers fort.

Une loss inter-arbres ne contribue jamais sur un match incertain sans pondération ni masque explicite.

## 10. R10 — Raccourci capteur

Portée, anneau, rémission et couverture sont corrélés aux classes. Le réseau peut améliorer la validation tout en devenant plus dépendant du capteur.

### Défenses

- sous-espaces `shape`, `metric`, `hier`, `sensor` ;
- channel dropout ;
- permutation des attributs capteur ;
- probes linéaires ;
- gradient reversal en ablation ;
- alignement cross-range appliqué seulement au sous-espace `shape` ;
- transfert vers un capteur inconnu.

Le but n'est pas de rendre le modèle amnésique : l'incertitude d'une observation est utile. Le but est d'empêcher le canal capteur d'expliquer seul la sémantique.

## 11. R11 — Résolution de sortie insuffisante

Un token polyédrique global peut reconnaître une voiture tout en oubliant où se trouve sa frontière avec le trottoir.

### Réponse

- prédiction par facette ;
- contexte de plusieurs ancêtres ;
- skip des features locales de surface ;
- tête de frontière sur le graphe dual ;
- raffinement de facettes selon une règle géométrique sans labels ;
- variante hybride point-wise uniquement comme plafond diagnostique.

L'oracle détermine si le problème vient du décodeur ou de la structure. Aucun nombre de couches ne recrée une frontière supprimée dans le contrat d'entrée.

## 12. R12 — Trop de nouveautés simultanées

Changer en même temps :

- la primitive ;
- le descripteur ;
- l'opérateur ;
- la loss ;
- les données ;
- le teacher 2D ;

rend tout résultat causalement illisible.

### Ordre imposé

1. mesure et fidélité ;
2. modèle plat ;
3. graphe latéral ;
4. branches et fusions ;
5. Surface-JEPA ;
6. Cross-Range PolyJEPA ;
7. temps ;
8. 2D et langage.

SPT, HSA et Sequoia sont des baselines, non des obligations architecturales.

## 13. R13 — Explosion du nombre de tokens

Même avec HGP rapide, tous les états, facettes et cellules de grille peuvent dépasser le nuage brut.

### Réponses

- références partagées aux facettes ;
- deltas entre niveaux ;
- sous-échantillonnage d'états par persistance, pas par profondeur arbitraire ;
- bases de surface multi-résolution ;
- batching par budgets multiples ;
- quantification et cache des tokens locaux ;
- inducing tokens pour les fusions de grand degré.

### Mesures

```text
N_points, N_facets, N_polyhedra, N_events,
N_surface_cells, bytes, VRAM, latency.
```

Le modèle doit être comparé à paramètres, FLOPs, octets et tokens. Choisir uniquement la métrique où il paraît léger ne constitue pas une méthode de compression.

## 14. R14 — Généralisation limitée au monde routier

Le repère gravité–capteur et la nature des surfaces LiDAR sont adaptés à la conduite. Ils peuvent transférer mal vers indoor, robotique rapprochée, CAD ou télédétection.

### Stratégie

Deux niveaux de claim :

1. **foundation model LiDAR outdoor** : plusieurs capteurs et jeux routiers ;
2. **foundation model 3D général** : domaines et orientations variés, encodeur équivariant et tâches plus larges.

Le premier est déjà ambitieux et scientifiquement défendable. Le second exige des données et des baselines d'une autre échelle.

## 15. R15 — Teacher 2D dominant

Une distillation visuelle peut améliorer fortement le score alors que le tokenizer polyédrique n'apporte rien.

### Contrôle

- géométrie seule ;
- même teacher 2D distillé vers points/voxels ;
- teacher 2D distillé vers superpoints ;
- teacher 2D distillé vers polyèdres ;
- évaluation sans caméra à l'inférence.

La caméra n'est ouverte qu'après preuve de valeur du tokenizer et de la hiérarchie en LiDAR seul.

## 16. Critères de repli

### Repli A — radialité réfutée

Passer à la mesure surfacique complète. Aucun changement de paradigme n'est perdu.

### Repli B — grille compacte réfutée

Utiliser `SurfaceGraph` comme encodeur local, avec la grille comme canal global.

### Repli C — hiérarchie réfutée

Publier ou poursuivre le tokenizer polyédrique plat ; abandonner le claim d'espace d'échelle.

### Repli D — cross-range réfuté

Conserver le modèle supervisé ; réduire le claim d'invariance et chercher la cause géométrique avant un nouveau SSL.

### Repli E — fondation réfutée

Nommer le résultat backbone spécialisé. La valeur scientifique d'un bon modèle de segmentation ne dépend pas de l'adoption d'un titre plus impérial.
