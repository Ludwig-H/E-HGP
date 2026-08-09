# MorseHGP3D v3

État : **`exploration_v3`**. Aucun statut public, aucun SLO, aucune phase
ouverte au registre. La promotion « M3 » annoncée au commit `2e3fa7b` est
**retirée** : l'audit [`AUDIT_PROMOTION_M3_2E3FA7B.md`](audits/AUDIT_PROMOTION_M3_2E3FA7B.md)
la déclare invalide, et j'ai reproduit ses quatre P0 moi-même, sur le header
committé, contre ma propre force brute. Ils sont réels.

L'autorité mathématique reste `docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`. Les audits de
[`audits/`](audits/) motivent les corrections ; ils ne certifient rien.

---

## 0. Ce qui a changé, et pourquoi ce n'était pas un détail

Le parcours `order_k_bfs.hpp` coupait le graphe sur le **rang fermé**
$\rho(v)=\ell(v)+\lvert S(v)\rvert$ et forçait le niveau du germe à zéro. Les
deux sont faux hors position simple. Reproduit ici, sans rien modifier au
header committé :

| fixture | ce que le parcours committé rend | la vérité exhaustive |
| --- | --- | --- |
| cube cosphérique, $s_{\max}=2$ | 8 sphères (les singletons) | **20** — les 12 boules diamétrales d'arêtes manquent |
| coquille constante, $s_{\max}=2$ | 6 | **15** |
| témoin coplanaire, tout $s_{\max}$ | 0, nuage déclaré hors domaine | 12 à 23 selon l'ordre |
| $n=2$ / $n=3$ | 2 / 3 | **3** / **6** |

Le cube dit tout en une ligne : coquille 8, niveau 0, rang fermé 8. Le sommet
est de **niveau zéro** — il ne peut pas être plus superficiel — et la coupe par
rang fermé le supprimait avant toute navigation, emportant avec lui les douze
arêtes du cube. Une grande coquille ne peut jamais être coupée : elle porte la
connectivité.

Le témoin coplanaire dit l'autre moitié : un point coplanaire à la face
d'enveloppe et strictement **intérieur** à son cercle circonscrit est intérieur
à toutes les sphères du pinceau. Le germe stockait 0 là où le census exact donne
1 ; le niveau transporté finissait par passer sous zéro, et le nuage entier
partait « hors domaine ». C'est l'explication locale et permanente des sorties
G4 à 8 000 et 20 000 points.

**Le nouveau fichier est [`prototype/order_k_flats.hpp`](prototype/order_k_flats.hpp).**
`order_k_bfs.hpp` est conservé tel quel : les audits le référencent par
empreinte, et le réécrire effacerait leurs constats.

---

## 1. Les quatre grandeurs, et ce que chacune a le droit de décider

Pour $x=(c,t)\in\mathbb{R}^4$ et le relèvement $\varphi(p)=(p,\lVert p\rVert^2)$, posons $L_i(x)=t-2c\cdot p_i+\lVert p_i\rVert^2$.

| grandeur | définition | seul usage autorisé |
| --- | --- | --- |
| niveau strict $\ell(v)$ | $\lvert\lbrace i:L_i(v)<0\rbrace\rvert$ | coupe du graphe, potentiel de parcours |
| coquille $S(v)$ | $\lbrace i:L_i(v)=0\rbrace$, taille variable | clef géométrique du sommet, fermetures de flats |
| rang fermé $\rho(v)$ | $\ell(v)+\lvert S(v)\rvert$ | **filtre de publication seulement** |
| support HGP $U^\star$ | $\subseteq S(v)$, $1\le\lvert U^\star\rvert\le4$ | arité, forêt, sérialisation |

### Les arêtes sont des flats fermés de rang trois, pas des triplets

Une arête de l'arrangement est une droite $F$, intersection de trois hyperplans
indépendants ; sa fermeture $C(F)=\lbrace i:F\subseteq H_i\rbrace$ est, dans la
géométrie des points, l'ensemble des points de la coquille situés dans un même
**plan** — donc sur un même cercle de la sphère. Les arêtes incidentes à $v$
sont en bijection avec les **plans distincts** engendrés par au moins trois
points non alignés de $S(v)$, et non avec les $\binom{m}{3}$ triplets.

La transition est $S(w)=C(F)\cup A$, où $A$ est le lot des points atteignant
simultanément le paramètre suivant. Le transport du niveau se fait par lots et
ne suppose jamais qu'un seul point change d'état :

$$B_e=B(v)\cup\lbrace i\in S(v)\setminus C:\ i\ \text{intérieur sur l'arête}\rbrace,\qquad B(w)=B_e\setminus\lbrace i\in A:\ i\ \text{intérieur sur l'arête}\rbrace.$$

La variation du niveau d'un voisin **n'est pas bornée par un**. Elle vaut
exactement $\lvert D_-\rvert-\lvert A_{\text{int}}\rvert$, et chacun des deux
termes peut être grand dès que plusieurs hyperplans coïncident sur l'événement.
L'énoncé « $\pm1$ » vient du cas simple et n'a jamais été vrai ailleurs ; je
l'avais d'abord corrigé en « $-1$, $0$ ou $+1$ », ce qui était encore faux.

### Le plafond est le niveau STRICT $s_{\max}-2$

C'est le **théorème de propriétaire** de
[`AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`](audits/AUDIT_VOIE_MULTIPLICITES_ORDER_K.md) §6 :
en dimension affine trois, tout support indépendant $U$ d'arité $q$ est contenu
dans un sommet $o(U)$ avec $B(o(U))\subseteq B_U$, donc $\ell(o(U))\le d_U\le s_{\max}-q$.
La chaîne $s_{\max}-q\le s_{\max}-2$ ne vaut **que pour** $q\ge2$ : les singletons ne
relèvent pas de la navigation et sont publiés à part, directement.
Naviguer selon $\ell\le s_{\max}-2$ et récolter les sous-ensembles de taille 2 et
3 des coquilles visitées suffit donc — un propriétaire peut porter une coquille
de taille huit ou cinquante, il est traversé quel que soit son rang fermé.

Les 4-sous-ensembles ne sont **pas** récoltés, et c'est démontrable : si le
support canonique a quatre points il est affinement indépendant, sa sphère est
le sommet lui-même et sa coquille est $S(v)$, que `try_emit(v.shell)` publie ;
si quatre points de la coquille sont coplanaires, leur miniboule a un support
d'au plus trois points et la récolte d'arité trois la publie.

### La connexité de $\lbrace\ell\le k\rbrace$ est acquise, et pas par ce dépôt

Elle est démontrée dans
[`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](audits/AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md),
pour un arrangement fini d'hyperplans non verticaux possédant au moins un
sommet — ni le relèvement parabolique, ni la simplicité ne sont requis. Le
parcours en tire le droit de partir d'un **seul** germe de niveau zéro.
L'ancien fichier disait explicitement ne pas la démontrer ; ce n'est plus
l'oracle qui porte cette charge.

---

## 2. Le germe est certifié, il ne l'est plus par décret

La correction n'est pas de compter les points coplanaires intérieurs : c'est de
les rendre **impossibles**. On prend une face support de l'enveloppe, puis dans
son plan un triangle **de Delaunay** du sous-nuage coplanaire — cercle
circonscrit vide dans ce plan. Toute sphère du pinceau coupe ce plan selon ce
cercle : aucun point coplanaire n'est jamais intérieur, et le premier lot
rencontré depuis le demi-espace vide est de niveau zéro **par construction**.

Le prédicat qui le permet est entier et exact : la **cocircularité coplanaire**,
déterminant $4\times4$ dont les lignes sont $(b-a,\lVert b-a\rVert^2)$,
$(c-a,\lVert c-a\rVert^2)$, $(u,0)$ et $(d-a,\lVert d-a\rVert^2)$ avec
$u=(b-a)\times(c-a)$. La ligne $(u,0)$ force le centre dans le plan ; le signe
est invariant par échange de $b$ et $c$, et négatif signifie strictement
intérieur au cercle. Borne $2^{108,8}$, donc `i128`.

### La première construction du triangle était fausse, et l'audit l'a réfutée

J'avais écrit une **descente de rayon** : si $d$ est intérieur au cercle de
$(a,b,c)$, l'un des trois triangles obtenus en remplaçant un sommet aurait un
cercle strictement plus petit. C'est faux.
[`AUDIT_ORDER_K_FLATS_9C587E6.md`](audits/AUDIT_ORDER_K_FLATS_9C587E6.md) §2 le
montre sur cinq points de la grille u16 :

```text
A=(0,0,0)  B=(0,3,0)  C=(2,1,0)  P=(1,1,0)  Q=(1,1,2)
```

$P$ est strictement intérieur au cercle de $ABC$ — le prédicat entier rend $-72$
— et pourtant les quatre rayons carrés valent exactement $5/2$. Aucune descente
n'existe : le germe rendait `germe_non_certifie` étape 6 et le catalogue sortait
vide, et 30 des 120 permutations échouaient là où 90 réussissaient. Le bon
potentiel de Delaunay n'a jamais été le rayon, c'est le vecteur des angles.

**La correction supprime la boucle.** Sur une **arête de l'enveloppe** du
sous-nuage coplanaire, le troisième point de Delaunay est celui qui maximise
l'angle inscrit, et « $d$ strictement intérieur au cercle de $(a,b,c)$ » équivaut
à « l'angle en $d$ dépasse l'angle en $c$ » : c'est un ordre **total** sur les
points d'un même côté de la droite, donc une seule passe suffit et il n'y a
aucune terminaison à prouver. Le cercle obtenu est vide — un intrus du même côté
contredirait la maximalité, il n'y a personne de l'autre côté puisque l'arête est
sur l'enveloppe, et un point de la droite hors du segment est extérieur à tout
cercle passant par ses extrémités.

L'arête d'enveloppe s'obtient elle aussi en une passe : le point lex-min du
sous-nuage coplanaire est extrême pour la forme « $x$ puis $y$ puis $z$ », donc
sommet de son enveloppe, et depuis lui aucune paire de directions n'est
antipodale — l'ordre angulaire est total. À angle égal on prend le point le plus
**proche**, sans quoi un point du segment resterait entre les deux extrémités et
serait intérieur à tout cercle passant par elles.

Les 120 permutations de cette fixture sont un test permanent : zéro refus,
signature unique. Le garde quadratique `q*q+8` de la boucle a disparu avec elle,
et avec lui son propre P0 — `q` converti en `int` débordait dès 46 341 points
coplanaires, ce qu'un nuage de 50 000 points peut atteindre.

**Un piège trouvé en chemin, et il n'était pas prévu par les audits.** La
rotation d'emballage autour de l'axe $(p_0,p_1)$ ne peut pas se décider au seul
signe de `orient3d` : ce prédicat ne voit un plan qu'à $\pi$ près, donc deux
candidats situés **dans** le plan vertical support mais de part et d'autre de
l'axe sont déclarés à égalité alors que leurs angles valent $0$ et $\pi$. La
passe partait du mauvais côté. Le nuage qui l'a exhibé, sur 3 000 tirés :

```text
(26,30,33) (27,30,34) (27,30,26) (34,30,33) (30,33,26) (25,30,25) (35,31,30)
```

La correction classe l'angle explicitement : avec $e=p_1-p_0$, $g=(-e_y,e_x,0)$
la normale intérieure du plan vertical support et $f=g\times e$, l'angle vaut
$0$ si $(w\cdot g=0,\ w\cdot f>0)$, $\pi$ si $(w\cdot g=0,\ w\cdot f<0)$, et il
est dans l'intervalle ouvert sinon, où `orient3d` redevient un ordre total. La
fixture est permanente sous le nom `germe_demi_tour`.

Un échec de germe rend `germe_non_certifie` avec son **étape**, jamais un germe
faux. `CloudStatus` remplace le booléen `out_of_domain` : dimension affine
inférieure à trois et moins de quatre points ne sont plus des erreurs mais une
**voie directe exhaustive**, exacte et déclarée.

---

## 3. Le support canonique n'est pas unique, et la convention naturelle est fausse

Une miniboule peut avoir **plusieurs** supports minimaux : le cube cosphérique
en a quatre, ses quatre paires antipodales. Trois conventions, mesurées :

| convention | résultat |
| --- | --- |
| lire le support sur le candidat qui a servi à découvrir la sphère | la force brute annonce $\lbrace2,5\rbrace$, la navigation $\lbrace0,7\rbrace$, pour la même sphère |
| le lire sur la coquille triée par **identifiant** | équivariant mais **pas invariant** : une seule permutation suffit à changer la sortie sur `cube`, `constant_shell_members`, `coplanaire_pur` et `germe_demi_tour` |
| le lire sur la coquille triée par **coordonnées** | invariant ; c'est la convention retenue |

C'est la porte ouverte n°10 du contrat d'audit. La convention retenue ne dépend
plus que de l'ensemble de points ; reste hors contrat le cas de deux points de
coordonnées identiques, dégénérescence déclarée à part.

L'ordre de sérialisation est désormais **lexicographique sur les quatre cases**
de `support`, queue remplie de $-1$ — jamais par arité d'abord. Deux générateurs
qui trient différemment produisent des catalogues sémantiquement égaux mais
d'indices différents, et `ForestNode::source` est un indice.

---

## 4. Ce qui est jugé, et par quoi

`mhgp3v_flats_differential` compare le sujet à une vérité écrite dans le même
fichier, qui n'appelle ni le germe, ni les prédicats de pinceau, ni le transport.

**Portée exacte de son autorité, et elle est plus étroite que ce que j'avais
écrit.** La vérité partage avec le sujet trois primitives de la v2 :
`mhgp::sphere_side`, `mhgp::sphere4` pour construire les sommets exhaustifs, et
`mhgp::miniball_of` pour décider les candidats et relire le support canonique.
Une faute commune de miniboule, de bon centrage ou de convention de support
serait donc **invisible** ici. Ce juge établit « portée de navigation et
catalogue concordants **relativement à ces primitives** », pas « catalogue
critique exact ». L'autorité indépendante manquante est une référence
rationnelle multiplicitaire dans l'oracle M1, qui n'existe pas.

Trois portes, et il faut les trois :

1. **le sommet** — tous les sommets d'arrangement énumérés en force brute, groupés par coquille, filtrés au niveau strict, comparés sur le couple (coquille, **niveau**) ;
2. **le catalogue** — tous les sous-ensembles de taille au plus quatre, miniboule, census exact, déduplication par coquille, support canonique ;
3. **l'équivariance** — renuméroter le nuage ne doit rien changer.

Le census exact par sommet est actif pendant toute la campagne : le transport
n'est jamais autorité, il est confirmé ou réfuté à chaque sommet. Le **payload
entier** est comparé — doublons publiés, tranche de membres et contiguïté de
`members_begin`, appartenance exacte des membres à la boule fermée publiée,
queue de `support` remplie de $-1$, ordre lexicographique strict de
sérialisation — et non plus le seul ensemble des couples support–rang.

**Planchers de couverture.** Chaque campagne exige un minimum de nuages
réellement navigués, de sommets, de coquilles multiples et de triplets
quotientés. Sans eux, une régression qui classerait tous les nuages en dimension
affine inférieure ferait comparer l'exhaustif du sujet à l'exhaustif de la
vérité et garderait toute la porte verte sans jamais exercer la navigation.

**Domaine déclaré, garde symétrique.** Deux observations confondues sont hors
contrat : le sujet doit refuser, et refuser exactement dans ce cas. La convention
de support canonique par ordre des coordonnées ne sépare pas deux points de même
coordonnée, et échanger leurs identifiants changeait quatre supports publiés.

**[mesuré]** deux campagnes après correction du germe, `-O2`, coordonnées
entières distinctes :

| campagne | nuages | points | grille | $s_{\max}$ | cas | désaccords |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique | 2 500 | 12 | $[0,26)$ | 2 à 7 | **19 544** | **0** |
| grille saturée | 1 500 | 10 | $[0,5)$ | 2 à 8 | **13 669** | **0** |

Couverture réellement exercée, publiée par le juge et exigée par CTest :

| campagne | nuages navigués | sommets | coquilles $>4$ | triplets quotientés | lots $>1$ | équivariances |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique | 18 819 | 3 359 381 | 11 863 | 51 578 | 171 246 | 646 |
| grille saturée | 13 209 | 1 331 410 | 58 393 | 125 574 | 751 805 | 396 |

La grille saturée est le régime qui compte : dix points dans une boîte de côté
cinq, donc presque tous les nuages portent des cosphéricités, des coplanarités
et des alignements. C'est ce que l'ancien parcours censurait.

**Fixtures permanentes**, aux coordonnées exactes publiées par les audits :
`coplanar_constant_witness`, `cube`, `constant_shell_members`, `bridge_shell5`,
`unreachable_extra_shell`, `noncritical_shell_tie`, `unit_increment_refutation`,
`non_well_centred_vertex`, `regular_tetrahedron`, `giant_centre_det1`,
`radius2_of_P0`, `well_centred_not_small`, `Q1_decisive`,
`partial_catalogue_on_reject`, `base_n2`, `base_n3`, `coplanaire_pur`,
`germe_demi_tour`, `germe_arete_traversee`, `descente_rayon_refutee`,
`coordonnees_dupliquees` — chacune à tous les ordres 2 à 8, plus son
équivariance ; `descente_rayon_refutee` est en outre rejouée sur ses **120
permutations**, avec exigence de zéro refus et de signature unique.

### Ce qui n'est PAS jugé

- L'oracle M1 n'a **pas** été étendu à ce sujet. Sa référence déclare hors domaine tout nuage portant un point surnuméraire sur une coquille — précisément le régime que ce parcours traite. L'étendre aux multiplicités est un travail à part, et il doit être audité.
- Aucun accélérateur spatial n'est branché : la requête de pinceau balaie le nuage entier. Le contrat *fail-open* de [`AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md`](audits/AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md) s'appliquera quand il le sera ; les P0 de `Grid::ball` restent donc **ouverts et non contournés**, simplement hors du chemin.
- Pas de forêts, pas de reverse search, pas de propriétaire calculé : la récolte déduplique encore par une table globale de coquilles.

---

## 5. Le contrat 50 000 points, $K=10$, une seconde

### Une correction qui change l'arithmétique de la question

**Le rapport 100:1 entre travail et sortie était un artefact.** Il comparait les
sommets visités à un compteur de sphères critiques produit par la récolte
défaillante, qui omettait l'essentiel des arités deux et trois. Mesuré sur le
catalogue complet et vérifié contre la force brute, le rapport réel est
d'environ **17:1**.

Ce n'est pas une bonne nouvelle déguisée : la sortie est six fois plus grosse
qu'annoncé. Le travail total, lui, n'a pas bougé.

**[mesuré]** profil LiDAR à densité fixe, emprise $\propto\sqrt n$, $s_{\max}=11$
(donc $K=10$, plafond de niveau strict 9), un cœur, `g++ -O3 -march=native`,
codespace 2 vCPU, **sans aucune accélération spatiale** :

| $n$ | sommets | sommets/point | critiques | critiques/point | travail/sortie | candidats/sommet |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 77 692 | 776,9 | 4 940 | 49,4 | 15,7 | 768 |
| 200 | 187 095 | 935,5 | 11 144 | 55,7 | 16,8 | 1 568 |
| 300 | 308 151 | 1 027,2 | 18 207 | 60,7 | 16,9 | 2 368 |

Arités à $n=300$ : 300 / 5 242 / 11 593 / 1 072. Les **triangles dominent** —
c'est la moitié du catalogue, et c'est exactement ce que l'ancienne récolte
perdait.

Les 777 sommets par point à $n=100$ retrouvent la mesure publiée précédemment,
ce qui rend les deux profils comparables. Les deux colonnes de droite disent
ensuite l'essentiel :

- **`candidats/sommet` vaut exactement $8(n-4)$**. Quatre flats, deux directions, un balayage complet du nuage à chaque fois. Tout le temps est là, et c'est une absence d'index, pas une propriété du problème.
- **`sommets/point` et `critiques/point` croissent encore** — 777 → 1 027 et 49,4 → 60,7 entre $n=100$ et $n=300$. **Rien n'est extrapolé à 50 000 points**, et l'extrapolation naïve de l'ancien README était déjà de cette nature.

### Ce que cela laisse comme question

Le mur n'est plus « le parcours jette 98,9 % de son travail » : il en jette 94 %,
et le facteur est de 17, pas de 100. Le mur est ailleurs, et il est double.

1. **La requête de pinceau est en $O(n)$.** À 50 000 points, un index qui la ramène à une vingtaine de candidats vaut un facteur de l'ordre de $10^3$. C'est le seul endroit où un accélérateur peut encore rendre autant — et il devra le faire *fail-open*, sous le contrat de l'audit numérique, sinon il rendra un faux vert.
2. **La récolte paie un census en $O(n)$ par candidat, et 43 % de ses tentatives sont des doublons.** C'est la règle de propriétaire qui les supprime, et un census local qui supprime le $O(n)$. Aucune des deux n'est écrite.

**Ce que je ne dis pas :** que le contrat est atteignable. Les deux ratios
croissent encore à $n=300$, la sortie à 50 000 points serait de l'ordre de
$3\cdot10^6$ sphères et $3\cdot10^7$ identifiants de membres, et aucun de ces
deux nombres n'est mesuré — ils sont extrapolés d'une croissance non stabilisée,
et je les donne comme tels.

### La taille du terrain n'est pas garantie

Le $\leq k$-niveau parcouru **est** le squelette de la mosaïque de Delaunay
d'ordre supérieur : c'est un théorème classique — le diagramme de Voronoï
d'ordre $k$ est la projection du $k$-niveau de l'arrangement relevé. Le parcours
ne visite donc pas un objet plus petit qu'elle, il visite **le même**. Il faut
dire précisément ce qui est gagné et ce qui ne l'est pas :

| | mosaïque d'ordre $\leq k$ matérialisée | parcours |
| --- | --- | --- |
| nombre de sommets | $V$ | **$V$, identique** |
| étiquette par sommet | le $k$-sous-ensemble, $O(k)$ entiers | coquille et compteur de niveau |
| cellules de dimension 1, 2, 3 ; incidences ; dual | construites et stockées | jamais formées |
| résident simultanément | tout | **tout aussi, aujourd'hui** |

Le gain acquis est donc la charge utile par sommet et l'absence des cellules de
dimension supérieure — de l'ordre d'un facteur cinq à dix en octets, **pas un
ordre de grandeur**. Le gain qui compterait, ne pas tout retenir, n'est pas
obtenu : `seen` et le vecteur des sommets visités retiennent l'ensemble.

**Et $V$ lui-même n'est borné par aucun théorème utilisable ici.** La borne
classique de Clarkson--Shor pour le $\leq k$-niveau de $n$ hyperplans de
$\mathbb{R}^4$ est **quadratique en $n$ et en $k$**. Les mesures ci-dessus sont
plusieurs ordres de grandeur en dessous, parce qu'un relevé est localement une
surface, donc très loin d'une configuration adverse. C'est une propriété du
**régime**, pas un théorème — et le §14 de [`PROPOSITION.md`](PROPOSITION.md) a
déjà retiré l'énoncé voisin « surface $\Rightarrow$ faible profondeur presque
partout » comme census et non comme théorème.

**Le multi-captation est le contre-exemple attendu, et il est déjà mesuré.** Le
census du 8 août, publié au §1.5 de `PROPOSITION.md`, donne le nuage à dix
captations recalées **moins peu profond** que la reconstruction fusionnée. Deux
relevés superposés créent des couches quasi dupliquées et la profondeur y monte
localement. C'est exactement ce que la porte Gate D doit trancher, et sa branche
« sortie sparse avec intermédiaires denses » est un **no-go d'architecture**
déclaré à l'avance.

**[obligation]** avant toute mesure à l'échelle : l'index *fail-open*, la règle
de propriétaire, le census local, puis un reçu séquentiel avec pic mémoire. Et
la mesure n'ira **pas** sur la G4 : c'est une charge CPU, elle n'a rien à faire
sur un GPU.

---

## 5 bis. Mémoire et GPU sont le même problème, pas deux

C'est le point qui commande l'ordre des travaux, et il n'était écrit nulle part.

La structure qui coûte la mémoire est `seen`, la table des coquilles déjà
visitées. C'est **exactement** celle qui interdit le GPU : une table de hachage
globale, à clefs de longueur variable, écrite par tous les fils. Il n'y a pas un
verrou mémoire et un verrou GPU ; il y en a **un seul**.

La technique qui l'élimine est connue — la *reverse search* d'Avis et Fukuda.
Une règle locale et déterministe désigne, depuis tout sommet, l'unique voisin
par lequel on y serait arrivé ; l'arbre de parcours devient implicite et un
sommet n'est publié que par son parent. Trois conséquences d'un seul coup :

1. la mémoire de navigation tombe à $O(\text{profondeur})$ au lieu du nombre de sommets ;
2. le parcours devient **parallèle sans communication** — deux fils sur deux sous-arbres n'ont rien à échanger, ce qui est la forme que réclame un GPU ;
3. le déterminisme devient gratuit, ce que la porte de publication exige de toute façon.

Le dépôt en possède déjà une preuve constructive,
[`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](audits/AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md),
**sous hypothèse d'arrangement simple**. Le parent multiplicitaire n'est
qu'esquissé.

### Ce qui portera, et ce qui devra être restructuré

Les prédicats sont déjà de la forme idéale : entiers, bornés, sans division,
sans rationnel, sans branchement sur la précision. Cette partie ne demande
aucune recherche. Le reste demande une restructuration explicite :

| point dur | forme actuelle | forme visée |
| --- | --- | --- |
| travail par sommet très irrégulier | une seule boucle | classes de charge : coquille de taille quatre en régime dense, coquilles plus grandes routées vers une file séparée |
| requête de pinceau | balayage $8(n-4)$ | requête indexée bornée, cas non concluants **différés dans une file de seconde passe** au lieu d'un repli en $O(n)$ dans le fil |
| census de la récolte | $O(n)$ par candidat | requête de portée sur la boule connue |
| tri global par $\beta$ exact | non écrit | étage **barrière** : runs triés puis fusion déterministe, clefs rationnellement égales réunies avant toute mutation |

**Ce qu'un GPU n'achètera pas.** Il multiplie le débit ; il ne réduit pas le
nombre de sommets visités, qui est celui de la mosaïque. La question de fond
n'est donc pas « aller plus vite sur ce terrain » mais « peut-on éviter d'en
traverser la plus grande part ». Aucun portage n'y répond.

---

## 5 ter. Commencer sous hypothèse de non-cosphéricité

**Décision de séquencement.** Le premier algorithme complet sera écrit pour un
arrangement **simple** — aucune cosphéricité, aucune coplanarité portante — et
le traitement des dégénérescences est reporté à l'une des **dernières phases**
du projet.

La raison n'est pas le confort. C'est que la *reverse search* n'est démontrée
que dans ce cas : c'est donc le seul domaine où la mémoire de navigation et la
forme parallèle peuvent être obtenues **et prouvées** aujourd'hui. On y règle
l'architecture — index, propriétaire, streaming, tri, forme GPU — sur un terrain
où chaque pièce a sa preuve, puis on rouvre les multiplicités en dernier.

Trois conditions, sans lesquelles cette décision serait une régression :

1. **Le domaine est déclaré et gardé fail-closed.** Un nuage portant une cosphéricité doit être **refusé avec sa raison nommée**, jamais traité comme s'il était simple. La règle du dépôt tient : un refus nomme sa cause, un compteur muet est une branche morte.
2. **Le travail multiplicitaire déjà fait est conservé, pas jeté.** `order_k_flats.hpp`, ses fixtures `cube`, `constant_shell_members`, `bridge_shell5`, `coplanaire_pur` et la campagne à grille saturée restent au dépôt et restent vertes ; elles deviennent la porte d'entrée de la phase finale.
3. **Aucune mesure à l'échelle sur donnée réelle ne peut être revendiquée depuis cette version.** La cible u16 quantifiée **n'est pas** dans ce domaine : un nuage LiDAR de cinq cents points porte déjà une cosphéricité à cinq points, et la campagne à grille saturée du §4 est précisément le régime qui compte. Les chiffres obtenus sous hypothèse simple qualifient l'architecture, jamais le contrat.

---

## 6. Construire et exécuter

```sh
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
cd build/v3 && ctest --output-on-failure
```

Le juge multiplicitaire seul, avec ses planchers :

```sh
./mhgp3v_flats_differential --clouds 0 --min-cases 150
./mhgp3v_flats_differential --clouds 1500 --points 10 --coord 5 --smax 8 --seed 31337 --min-cases 10000
```

Une campagne vide, un argument inconnu ou un plancher non atteint rendent un
code non nul avec son diagnostic ; trois tests négatifs le vérifient.

---

## 7. Ce qui reste ouvert, sans ordre de facilité

| # | question | statut |
| --- | --- | --- |
| 1 | index spatial *fail-open* pour la requête de pinceau | non écrit ; les P0 de `Grid::ball` restent ouverts |
| 2 | règle de propriétaire pour les arités 2 et 3, et census local | non écrite ; 43 % de la récolte est redondante |
| 3 | reverse search, pour supprimer `seen` et `frontier` | non écrite ; **c'est le même verrou que le GPU** (§5 bis), et elle n'est démontrée que sous arrangement simple — d'où la décision de séquencement du §5 ter |
| 4 | référence de l'oracle M1 tolérante aux multiplicités | non écrite ; sans elle le sujet n'a pas de juge indépendant en arithmétique rationnelle |
| 5 | forêts, tri global par $\beta$ exact, lots atomiques | non écrits |
| 6 | invariance topologique du support canonique quand plusieurs supports minimaux portent la même miniboule | ouverte ; la convention par coordonnées est *une* convention, pas un théorème |
| 6 bis | sémantique quotientée des observations confondues | ouverte ; le prototype les **refuse** explicitement plutôt que de publier un support dépendant de la numérotation |
| 7 | `sphere.hpp` au bord produit : paire de points confondus acceptée comme support d'arité deux, sentinelle `den==0` sans garde | ouverts, hors de ce fichier |
| 8 | le contrat 50 k / $K=10$ / 1 s | **non atteint, non mesuré, et les deux ratios qui le décident croissent encore à $n=300$** |
| 9 | les $n$ singletons passent par `try_emit` avant le germe, soit $2{,}5\cdot10^9$ appels à `sphere_side` à 50 k | ouverte ; c'est une porte d'architecture, pas une constante |
| 9 | la taille $V$ du $\leq k$-niveau en général | **non bornée utilement** : Clarkson--Shor est quadratique en $n$ et en $k$, et les mesures ne valent que pour le régime de surface (§5) |
| 10 | le régime multi-captation | mesuré **moins peu profond** que la reconstruction fusionnée ; c'est la branche no-go de Gate D |

Le détail, les budgets et le journal des affirmations retirées sont dans
[`PROPOSITION.md`](PROPOSITION.md).
