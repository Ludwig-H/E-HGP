# Le jeton polyédrique

Ce que porte un jeton, comment il est normalisé, et comment on décide
expérimentalement de ce qui mérite d'y rester.

Un avertissement hérité du corpus précédent, repris ici parce qu'il est
probablement juste : **le descripteur de nœud est le levier le plus faible du
projet**. La tokenisation et la hiérarchie apportent davantage que le choix
exact du codage de forme. C'est pourquoi ce document propose un descripteur
riche *et* une procédure (porte G3) pour en retirer sans état d'âme tout ce qui
ne se paie pas.

## 1. Les cinq familles, et la règle qui les sépare

> Une seule famille est normalisée. Les quatre autres gardent leurs unités.

C'est la règle de la présentation du 16 septembre, § « Ce que le code ne
remplace pas ». Le poster fait l'hypothèse que la **forme** varie moins que
l'échantillonnage ; il n'a jamais dit que la taille, la portée ou la rémission
étaient sans intérêt. Les normaliser hors d'existence rendrait le modèle
aveugle au monde métrique.

| famille | normalisée ? | ce qu'elle doit capturer |
| --- | --- | --- |
| 1. forme | **oui** (translation, échelle) | ce qui doit rester stable quand le même objet est vu de plus loin |
| 2. physique | non | dimensions, pose, position dans la scène |
| 3. filtration | non | la trajectoire d'échelle : naissance, mort, fusion, ordre |
| 4. acquisition | non | la qualité de la mesure |
| 5. sémantique | — | c'est la sortie, jamais l'entrée |

## 2. Famille 1 — le code de forme

Le nœud $v$ a pour géométrie $P_v = \bigcup_b \mathrm{conv}(S_b)$, réunion des
enveloppes convexes des supports des boules qui le composent. $P_v$ est
**ouvert, non convexe et possiblement multicouche** : rien n'autorise à le
remplacer par $\mathrm{conv}(P_v)$.

### 2.1 Repère propre

Centre $c$ = centre de la boîte englobante de $P_v$ ; $s$ = moitié du plus grand
côté ; $\widehat{P} = (P_v - c)/s \subset [-1, 1]^3$. C'est le cadrage de
[BPS 19] repris par la présentation.

Variante à comparer (G3) : le repère des moments d'ordre 2 de la mesure
surfacique, qui est plus stable sous décimation qu'une boîte englobante —
laquelle dépend de deux points extrêmes et donc du hasard de l'échantillonnage.
C'est un point faible réel du cadrage par boîte, à mesurer, pas à trancher ici.

### 2.2 Grille de distances à sondes fixes

$8^3 = 512$ sondes fixes $b_j$, et
$D_P[j] = d(c + s\, b_j,\ P_v) / s$.

La distance est prise aux **primitives entières**, pas seulement aux sommets :
$d_P(b) = \min_{\sigma \in F} d(b, \sigma)$. Deux propriétés la recommandent :

- **stabilité** : $\left\Vert d_P - d_Q \right\Vert_\infty = d_H(P, Q)$, la
  distance de Hausdorff. Une petite perturbation de la surface produit une
  petite perturbation du code ;
- **union** : $d_{P \cup Q} = \min(d_P, d_Q)$, donc le code d'une fusion se
  calcule depuis ceux des parents sans repartir des points.

Son défaut est connu et documenté par la présentation elle-même : **une grille
peut manquer un petit élément**. D'où les deux descripteurs complémentaires
ci-dessous, et la règle « garder les primitives et les niveaux fins ».

### 2.3 Mesure surfacique, moments et histogramme sphéro-radial

La représentation *principale* n'est pas une fonction radiale $\rho(u)$ : depuis
un centre, une direction peut ne rencontrer aucune couche, ou plusieurs.
$\rho(u)$ ne décrit que le cas d'une couche unique.

On part donc de la **mesure surfacique attribuée normalisée** de $P_v$ —
longueurs pour les supports d'arité 2, aires pour les arités 3 et 4 — dont on
tire :

- les **moments** jusqu'à l'ordre 3, en forme close par simplexe, donc
  $O(\#\text{supports})$ et exacts en rationnels avant arrondi ;
- un **histogramme sphéro-radial** sur $\mathbb{S}^2 \times \mathbb{R}$ : la
  mesure est conservée en entier, sans sélection d'une couche par direction, ce
  qui accepte naturellement les trous et les intersections multiples.

### 2.4 La signature d'arité — le canal le plus spécifique, et le moins cher

Les proportions de supports d'arité 2, 3 et 4 dans le nœud :
$(n_2, n_3, n_4)/(n_2 + n_3 + n_4)$, plus les mêmes proportions pondérées par
la mesure.

C'est une **signature de dimension locale, gratuite et exacte** : une structure
filiforme est portée surtout par des arêtes diamétrales ; une surface, par des
triangles aigus ; un volume, par des tétraèdres. Aucun modèle point à point
n'y a accès, et la v9 la publie déjà (`arity`, `balls_by_qmin`). C'est le
premier canal à tester en G3 et, si l'intuition se vérifie, il coûte trois
flottants.

## 3. Famille 2 — grandeurs physiques

- rayon de naissance $r_v$ en mètres (depuis le niveau exact $r^2$, converti à
  la sortie seulement) ;
- dimensions de la boîte englobante, aire de la mesure surfacique, épaisseur
  (plus petite valeur propre des moments d'ordre 2) ;
- centre en repère capteur, hauteur au-dessus du capteur, **portée** (distance
  au capteur) et azimut ;
- nombre de retours couverts $|S_v|$, séparé en intérieur et coquille.

La **portée** doit être un canal explicite : elle est le paramètre de nuisance
de l'hypothèse du poster, et un modèle qui ne l'a pas ne peut pas apprendre à
s'en défaire.

## 4. Famille 3 — filtration

Ce que la tour donne et que rien d'autre ne donne.

- niveau de naissance $r_b(v)$ et niveau de mort $r_d(v)$ (le niveau de la
  fusion qui absorbe $v$), persistance $r_d - r_b$ et écart logarithmique
  $\log r_d - \log r_b$ ;
- rang et quantile du niveau dans la trame — utile parce que la densité absolue
  dépend du capteur alors que le rang, moins ;
- **gain de masse** à la fusion, et **degré de multifusion** (nombre de
  parents : la v9 publie explicitement les événements à deux parents ou plus) ;
- nombre d'enfants dans l'arbre condensé ;
- **ordre $K$** et l'image verticale dans $K-1$ : son identité, son niveau, sa
  taille. C'est la coordonnée de bifiltration ;
- excès de masse du nœud, tel que `select_excess_of_mass` le calcule.

## 5. Famille 4 — acquisition

Rattachée par `PointId`, hors moteur, puisque la quantification 1 mm ne
transporte pas les attributs :

- rémission : moyenne, écart-type, quantiles ;
- anneaux couverts (nombre, indices minimal et maximal) et couverture
  angulaire ;
- incidence estimée : angle entre la normale des moments et la direction du
  capteur ;
- proxy d'occultation : part du secteur angulaire du nœud sans retour.

## 6. Taille et format

Ordre de grandeur, à confirmer par la porte G0.5 :

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

Stockage : **float16 pour la famille 1**, entiers exacts pour les canaux qui le
sont déjà (arités, comptes, ordre $K$, rangs). Le flottant est une sortie du
moteur, jamais un maillon de sa chaîne d'exactitude — c'est la doctrine du
dépôt et il n'y a pas de raison d'y déroger ici.

## 7. Comment on décide ce qui reste

Aucune de ces familles n'est justifiée par l'intuition. La porte **G3** du
[protocole](PROTOCOLE.md) tranche par ablation XGBoost :

1. entraîner sur **chaque famille seule** ;
2. entraîner sur **toutes sauf une** ;
3. comparer à une base triviale (hauteur du centre, taille de boîte, portée) ;
4. lire l'importance par gain et les valeurs de Shapley.

Un canal qui ne bat pas la base triviale d'une marge annoncée à l'avance est
**retiré**, quelle que soit son élégance mathématique. Écrire la marge avant de
lancer l'expérience fait partie de la porte.

## 8. Références du codage

- [BPS 19] S. Prokudin, C. Lassner, J. Romero, *Efficient Learning on Point
  Clouds with Basis Point Sets*, ICCV 2019, p. 4332–4341 — le cadrage et les
  sondes fixes.
- [Schneider 13] R. Schneider, *Convex Bodies : The Brunn–Minkowski Theory*,
  2e éd., Cambridge University Press, 2013 — la fonction support, et la raison
  de ne pas l'employer seule : $h_P = h_{\mathrm{conv}(P)}$.
- [PolyhedronNet 25] D. Yu, G. Zhang, L. Zhao, *Representation Learning for
  Polyhedra with Surface-Attributed Graph*, ICLR 2025 — l'antécédent le plus
  proche pour un encodeur natif sur polyèdres, à citer et à ne pas réinventer.
- Manuscrit, parties I–II et § 9.1 — l'objet, les supports de Gabriel et le
  vote pondéré.
