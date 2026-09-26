# Variables de nœud

Ce qu'un nœud de l'échelle porte en entrée du réseau, comment on le normalise,
et comment on décide expérimentalement de ce qui mérite d'y rester.

L'hypothèse HGP-UNet privilégie l'apport structurel — échelle, voisinage,
biais de position, axe des ordres — sans avoir encore mesuré son poids face
aux variables de nœud. Ce document propose un jeu riche et la procédure pour
en retirer ce qui ne se paie pas. Le [protocole de guidage FULL](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md)
distingue ces entrées du tokenizer des variables enseignantes réservées à la
perte de pré-entraînement.

## 1. Cinq familles, et la règle qui les sépare

> Une seule famille est normalisée. Les quatre autres gardent leurs unités.

C'est la règle de la présentation du 16 septembre, § « Ce que le code ne
remplace pas ». L'hypothèse du poster porte sur la **forme** ; elle n'a jamais
prétendu que la taille, la portée ou la rémission étaient sans intérêt. Les
normaliser hors d'existence rendrait le modèle aveugle au monde métrique.

Cette règle conserve l'information physique ; elle n'interdit pas une
standardisation numérique réversible des canaux avec unités et constantes
déclarées. Pour les logarithmes, utiliser un rapport à une unité fixe,
par exemple $\log(r/r_0)$.

| famille | normalisée ? | ce qu'elle capture |
| --- | --- | --- |
| 1. forme | **oui** (translation, échelle) | ce qui doit rester stable quand le même objet est vu de plus loin |
| 2. physique | non | dimensions, pose, position dans la scène |
| 3. filtration | non | la trajectoire d'échelle : naissance, mort, fusion, ordre |
| 4. acquisition | non | la qualité de la mesure |
| 5. sémantique | — | c'est la sortie, jamais l'entrée |

### Équivariance plutôt qu'invariance

Conserver la taille physique tout en normalisant la forme est le choix à
tester. Donner $\log(r/r_0)$ permet au modèle d'utiliser la métrique ; cela ne
rend pas à lui seul le réseau équivariant. Distinguer la covariance de la
tour, la transformation déclarée des variables et le comportement appris.
Un changement de capteur modifie aussi le balayage et la visibilité : il
n'est pas une homothétie. Ablater le canal métrique à acquisition appariée,
puis mesurer le transfert avec réglages gelés.

## 2. Famille 1 — forme

La géométrie d'un nœud est $P_v = \bigcup_b \mathrm{conv}(S_b)$, réunion des
enveloppes convexes des supports des boules qui le composent. $P_v$ est
**une union finie de supports convexes fermés de dimensions mêlées** : ce
squelette de témoins n'est pas automatiquement une surface physique fermée,
et on ne le remplace pas par $\mathrm{conv}(P_v)$.

**Signature d'arité** — les proportions de supports d'arité $2$, $3$ et $4$,
brutes et pondérées par la mesure. C'est une **signature exacte des supports
critiques**, pas une mesure automatique de la dimension physique de l'objet :
arêtes, triangles et tétraèdres peuvent contribuer au même nœud. La tour publie les supports
nécessaires à cette signature, sous réserve de leur export.
Trois flottants : c'est le premier canal à tester.

**Moments des supports par dimension** jusqu'à l'ordre 3 — longueurs des
arêtes, aires des triangles et volume des tétraèdres en canaux distincts.
Une aire de bord pour les tétraèdres exige de définir et calculer ce bord.
Ces moments peuvent donner un repère propre. Comparer leur stabilité sous
décimation à celle d'une boîte englobante, qui dépend des points extrêmes.

**Histogramme sphéro-radial** des supports bidimensionnels ou des surfels
observés, explicitement distingués, sur $\mathbb{S}^2 \times \mathbb{R}$.
Il résume les couches présentes dans la mesure choisie, y compris plusieurs
intersections par direction. Il ne reconstitue pas la surface physique cachée.

**Grille de distances à sondes fixes** — $8^3 = 512$ sondes,
$D_P[j] = d(c + s\,b_j, P_v)/s$, la distance étant prise aux primitives
entières. La distance continue vérifie
$\left\Vert d_P-d_Q\right\Vert_\infty=d_H(P,Q)$ ; pour un repère
$(c,s)$ **commun**, 512 sondes ne donnent que
$\max_j|D_P[j]-D_Q[j]|\le d_H(P,Q)/s$,
avec des collisions possibles. L'identité d'union
$d_{P\cup Q}=\min(d_P,d_Q)$ s'applique aux **mêmes requêtes physiques** :
des grilles de parents recentrées ou renormalisées ne se combinent pas case
à case, et une fusion peut ajouter de nouveaux supports. Une grille finie peut
manquer un petit élément. Garder donc les primitives et les niveaux fins,
et comparer la grille aux descripteurs moins coûteux.

## 3. Famille 2 — physique

Rayon de naissance en mètres ; dimensions de la boîte ; aires des supports
bidimensionnels ou des surfels observés, identifiées séparément ; épaisseur
(plus petite valeur propre des moments d'ordre 2) ;
centre en repère capteur ; hauteur au-dessus du capteur ; **portée** et azimut ;
nombre de retours couverts, séparé en intérieur et coquille.

La portée doit être testée comme canal explicite : elle peut aider à séparer
effet de l'acquisition et structure locale. Son apport propre est à ablater
avec profils de rayons K-NN, comptes multi-rayons et anisotropie. Le score
$K/(n\omega_3r_K^3)$ est une concentration ambiante, pas automatiquement une
densité physique de surface ; le témoin T2 de [MESURE](MESURE.md) explicite
ces comparaisons conditionnelles.

## 4. Famille 3 — filtration

Ce que la tour donne et que rien d'autre ne donne. Ce sont aussi les cibles des
tâches de pré-entraînement, ce qui impose une précaution : **une variable
utilisée comme cible en pré-entraînement ne doit pas être donnée en entrée au
même moment.**

La restriction porte aussi sur les parents, degrés, choix des voisins et
biais d'attention qui révèlent la réponse. Dans le bras tokenizer, ces objets
viennent de la vue élève recalculée ; les objets de la tour enseignante restent
dans la perte. Élaguer cette dernière après masquage ne suffit pas.

Naissance, mort, persistance et écart logarithmique ; rang et quantile du
niveau dans la trame — utile parce que la densité absolue dépend du capteur
alors que le rang, moins ; gain de masse à la fusion ; **degré de multifusion** ;
nombre d'enfants dans l'arbre condensé ; **ordre $K$** et image verticale — son
identité, son niveau, sa taille ; excès de masse du nœud.

## 5. Famille 4 — acquisition

Rattachée par identifiant de point, hors moteur : rémission (moyenne,
écart-type, quantiles) ; anneaux couverts et couverture angulaire ; incidence
estimée depuis les moments ; proxy d'occultation, part du secteur angulaire
sans retour. Un secteur sans retour n'établit ni absence de surface ni espace
libre ; conserver cette incertitude dans les comparaisons entre vues.

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
