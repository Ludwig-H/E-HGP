# Variables de nœud

Ce qu'un nœud de l'échelle porte en entrée du réseau, comment on le normalise,
et comment on décide expérimentalement de ce qui mérite d'y rester.

Avertissement de cadrage, qui vient de l'architecture et non d'une intuition :
dans HGP-UNet, **l'essentiel de l'apport est structurel** — l'échelle, le
voisinage, le biais de position, l'axe des ordres. Les variables de nœud sont
le levier le plus faible. Ce document propose donc un jeu riche **et** la
procédure pour en retirer sans état d'âme tout ce qui ne se paie pas.

## 1. Cinq familles, et la règle qui les sépare

> Une seule famille est normalisée. Les quatre autres gardent leurs unités.

C'est la règle de la présentation du 16 septembre, § « Ce que le code ne
remplace pas ». L'hypothèse du poster porte sur la **forme** ; elle n'a jamais
prétendu que la taille, la portée ou la rémission étaient sans intérêt. Les
normaliser hors d'existence rendrait le modèle aveugle au monde métrique.

| famille | normalisée ? | ce qu'elle capture |
| --- | --- | --- |
| 1. forme | **oui** (translation, échelle) | ce qui doit rester stable quand le même objet est vu de plus loin |
| 2. physique | non | dimensions, pose, position dans la scène |
| 3. filtration | non | la trajectoire d'échelle : naissance, mort, fusion, ordre |
| 4. acquisition | non | la qualité de la mesure |
| 5. sémantique | — | c'est la sortie, jamais l'entrée |

### Équivariance plutôt qu'invariance

Pour le transfert inter-capteurs on veut que la **structure** soit sans échelle
mais que le **modèle** sache l'échelle : une voiture mesure quatre mètres, et
c'est une information réelle. On donne donc $\log r$ comme canal explicite,
tandis que le pooling, le voisinage et le biais de position restent sans
échelle. L'ablation de ce canal dit lequel des deux régimes domine — c'est une
mesure de la phase 2.

## 2. Famille 1 — forme

La géométrie d'un nœud est $P_v = \bigcup_b \mathrm{conv}(S_b)$, réunion des
enveloppes convexes des supports des boules qui le composent. $P_v$ est
**ouvert, non convexe et possiblement multicouche** : on ne le remplace jamais
par $\mathrm{conv}(P_v)$.

**Signature d'arité** — les proportions de supports d'arité $2$, $3$ et $4$,
brutes et pondérées par la mesure. C'est une **signature de dimension locale,
exacte et gratuite** : une structure filiforme est portée surtout par des
arêtes diamétrales, une surface par des triangles aigus, un volume par des
tétraèdres. Aucun modèle par points n'y a accès et la tour la publie déjà.
Trois flottants : c'est le premier canal à tester.

**Moments de la mesure surfacique** jusqu'à l'ordre 3 — longueurs pour les
arités 2, aires pour 3 et 4, en forme close par simplexe. Donne aussi un repère
propre plus stable sous décimation qu'une boîte englobante, laquelle dépend de
deux points extrêmes et donc du hasard de l'échantillonnage.

**Histogramme sphéro-radial** de la mesure surfacique sur
$\mathbb{S}^2 \times \mathbb{R}$ — conserve toute la surface sans sélectionner
une couche par direction, donc accepte les trous et les intersections
multiples. C'est ce qu'une fonction radiale $\rho(u)$ ne sait pas faire : depuis
un centre, une direction peut ne rencontrer aucune couche, ou plusieurs.

**Grille de distances à sondes fixes** — $8^3 = 512$ sondes,
$D_P[j] = d(c + s\,b_j, P_v)/s$, la distance étant prise aux primitives
entières. Deux propriétés la recommandent : la **stabilité**,
$\left\Vert d_P - d_Q \right\Vert_\infty = d_H(P, Q)$, et l'**union**,
$d_{P \cup Q} = \min(d_P, d_Q)$, qui permet de calculer le code d'une fusion
depuis ceux des parents. Son défaut est connu et documenté par la présentation
elle-même : une grille peut manquer un petit élément. D'où les deux
descripteurs précédents, et la règle de garder les primitives et les niveaux
fins.

## 3. Famille 2 — physique

Rayon de naissance en mètres ; dimensions de la boîte ; aire de la mesure
surfacique ; épaisseur (plus petite valeur propre des moments d'ordre 2) ;
centre en repère capteur ; hauteur au-dessus du capteur ; **portée** et azimut ;
nombre de retours couverts, séparé en intérieur et coquille.

La portée doit être un canal explicite : c'est le paramètre de nuisance de
l'hypothèse d'invariance, et un modèle qui ne l'a pas ne peut pas apprendre à
s'en défaire.

## 4. Famille 3 — filtration

Ce que la tour donne et que rien d'autre ne donne. Ce sont aussi les cibles des
tâches de pré-entraînement, ce qui impose une précaution : **une variable
utilisée comme cible en pré-entraînement ne doit pas être donnée en entrée au
même moment.**

Naissance, mort, persistance et écart logarithmique ; rang et quantile du
niveau dans la trame — utile parce que la densité absolue dépend du capteur
alors que le rang, moins ; gain de masse à la fusion ; **degré de multifusion** ;
nombre d'enfants dans l'arbre condensé ; **ordre $K$** et image verticale — son
identité, son niveau, sa taille ; excès de masse du nœud.

## 5. Famille 4 — acquisition

Rattachée par identifiant de point, hors moteur : rémission (moyenne,
écart-type, quantiles) ; anneaux couverts et couverture angulaire ; incidence
estimée depuis les moments ; proxy d'occultation, part du secteur angulaire
sans retour.

## 6. Taille et format

| famille | dimensions |
| --- | --- |
| grille de distances $8^3$ | 512 |
| moments jusqu'à l'ordre 3 | 19 |
| histogramme sphéro-radial | 96 à 128 |
| signature d'arité | 6 |
| physique | ≈ 20 |
| filtration | ≈ 20 |
| acquisition | ≈ 15 |
| **total** | **≈ 690 à 720** |

Stockage : float16 pour la famille 1, entiers exacts pour ce qui l'est déjà
(arités, comptes, ordre $K$, rangs). Le flottant est une sortie du moteur,
jamais un maillon de sa chaîne d'exactitude.

**La grille de distances à 512 dimensions est le poste à interroger en
premier.** Elle vaut à elle seule les trois quarts du descripteur ; si la
signature d'arité et les moments la remplacent sans perte, on divise le cache
par quatre.

## 7. Comment on décide ce qui reste

Par ablation XGBoost, porte 0.4 de [`MESURE.md`](MESURE.md) : chaque famille
seule ; toutes sauf une ; contre une base triviale (hauteur, taille, portée) ;
contre une base « points » sans hiérarchie ; **avec et sans la famille
filtration**. Un canal qui ne bat pas la base triviale d'une marge annoncée
d'avance est retiré, quelle que soit son élégance.

## 8. Références du codage

- [BPS 19] Prokudin, Lassner, Romero, *Efficient Learning on Point Clouds with
  Basis Point Sets*, ICCV 2019 — le cadrage et les sondes fixes.
- [Schneider 13] Schneider, *Convex Bodies : The Brunn–Minkowski Theory*,
  2e éd., 2013 — la fonction support, et la raison de ne pas l'employer seule :
  $h_P = h_{\mathrm{conv}(P)}$.
- [PolyhedronNet 25] Yu, Zhang, Zhao, *Representation Learning for Polyhedra
  with Surface-Attributed Graph*, ICLR 2025 — l'antécédent le plus proche pour
  un encodeur natif sur polyèdres.
- Manuscrit, parties I–II et § 9.1 — l'objet, les supports de Gabriel, le vote
  pondéré.
