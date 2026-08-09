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
3. **l'équivariance** — renuméroter le nuage ne doit rien changer ;
4. **l'index contre la référence** — le chemin indexé doit rendre exactement le même catalogue que le balayage complet : mêmes supports, mêmes rangs, mêmes membres, même ordre, même statut. Sans cette porte, un index qui rate un point ne se verrait qu'au désaccord avec la force brute, c'est-à-dire beaucoup plus tard et beaucoup plus mal.

Le census exact par sommet est actif pendant toute la campagne : le transport
n'est jamais autorité : une contradiction positionne `kInvariantViolated` et
arrête le parcours, elle n'incrémente plus seulement un compteur que ce binaire
serait seul à lire.

Ce qui est comparé au-delà des couples support–rang : doublons publiés, tranche
de membres et contiguïté de `members_begin`, appartenance exacte des membres à
la boule fermée publiée, queue de `support` remplie de $-1$, ordre
lexicographique strict de sérialisation. Ce qui ne l'est **pas** : le centre
rationnel, le rayon et $\beta$ ne sont pas confrontés à une vérité distincte, et
les forêts ne sont pas construites. Ce n'est donc pas « le payload entier ».

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
| générique | 700 | 11 | $[0,24)$ | 2 à 6 | **4 761** | **0** |
| grille saturée | 600 | 10 | $[0,5)$ | 2 à 8 | **5 611** | **0** |

Couverture réellement exercée, publiée par le juge et exigée par CTest :

| campagne | nuages navigués | sommets | coquilles $>4$ | triplets quotientés | lots $>1$ | équivariances |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique | 4 510 | 510 154 | 2 055 | 9 324 | 28 835 | 201 |
| grille saturée | 5 383 | 530 197 | 23 823 | 51 274 | 303 598 | 176 |

Le premier juge local du parent est exercé sur ces mêmes campagnes : **510 154**
et **530 197** sommets ont reçu un parent, avec une racine par nuage et zéro
violation parmi les contrôles actuellement gravés — inclusion des intérieurs et
absence de cycle. Ce n'est pas encore Gate D entière : la porte doit devenir
fail-closed si le second parcours échoue, puis graver rang de la base, variation
stricte du potentiel, fermeture du flat et identité du lot suivant.

Ces deux campagnes n'ont pas encore de commande, graine, log brut et sidecar
versionnés dans le dépôt. Elles sont des diagnostics rapportés, pas un reçu de
qualification reproductible.

Les totaux diffèrent de ceux publiés précédemment : les campagnes ont été
rejouées après la fermeture du P0 d'élagage, et le compte de cas a changé avec
l'ajout des portes d'index et de domaine.

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
- Les forêts ne sont pas construites, et le centre rationnel, le rayon et $\beta$ ne sont pas confrontés à une vérité distincte. Ce qui est comparé est listé au §4 ; « payload entier » serait faux.
- Pas de forêts, pas de reverse search, pas de propriétaire calculé : la récolte déduplique encore par une table globale de coquilles.

---

## 4 bis. L'index : ce qu'il a coûté, et ce qu'il n'a pas donné

Les deux $O(n)$ que l'audit compte — la requête de pinceau et le census de
chaque tentative d'émission — sont remplacés par un **arbre k-d** interrogé
exactement. Le chemin de référence, qui balaie le nuage, est conservé : c'est
contre lui que l'index est jugé, et c'est la quatrième porte du différentiel.

### Le prédicat est exact, parce que la marge flottante a été réfutée

J'avais élagué un nœud par une boule flottante élargie d'un demi, en arguant que
« coordonnées et rayons restent sous $2^{17}$ sur la grille déclarée ». C'est
faux, et la note
[`NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md`](audits/NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md)
§1.3 le montre sur quatre points u16 distincts :

```text
(32767,32767,0)  (57863,57862,0)  (7672,7673,0)  (60104,30135,1)
```

Le *centre* d'une sphère portée par un quadruplet presque coplanaire sort
arbitrairement loin de la grille : ici $\mathrm{den}=2$ et le rayon vaut environ
$10^{18}$. Reproduit : l'élagage supprimait **la racine** et la requête rendait
zéro point au lieu des quatre supports.

Le prédicat est donc exact, en entiers. Avec $C_j=\mathrm{base}_j d+n_j$ et
$g_j=\max\lbrace\ell_jd-C_j,\ 0,\ C_j-h_jd\rbrace$, on a
$d^2\,\mathrm{dist}(c,Q)^2=g_x^2+g_y^2+g_z^2$, et le nœud n'est élagué que si
cette somme dépasse **strictement** $N=\lVert\mathrm{num}\rVert^2$ — l'égalité est
conservée, la boule est fermée et les points de coquille sont contractuels.
Largeurs : $C_j$ et $g_j$ sous $2^{91}$, les carrés sous $2^{182}$, la somme sous
$2^{184}$, donc `BigInt<4>`. Un chemin rapide flottant subsiste, mais **gardé** :
il n'est autorisé que si centre et rayon tiennent sous $2^{20}$, où l'erreur
absolue reste sous $2^{-30}$ et la marge d'un demi la domine de plus de $2^{28}$.

### La requête de pinceau teste un désaccord de signe, pas une différence de boules

Entre deux paramètres du pinceau la puissance d'un point est affine, donc un
événement non constant a des signes **strictement opposés** aux deux extrémités,
et un événement situé à une extrémité y a un signe nul et un signe non nul à
l'autre. La requête est donc le désaccord **ternaire** de `sphere_side`, et non
la différence symétrique des deux boules fermées : cette dernière perdrait
précisément le cas contractuel « sur la coquille d'un côté, strictement intérieur
de l'autre ». Un point du cercle du flat a le même signe nul aux deux extrémités
— il est déjà dans la fermeture et n'est pas redécouvert.

Balayer une boule entière serait correct mais ruineux : la sphère d'un sliver de
surface est géométriquement énorme bien qu'elle ne contienne qu'une poignée de
points, et sa **frontière** traverse une grande partie du nuage.

### L'ensemble intérieur est transporté, plus jamais recensé

$B(v)$ n'est plus un simple cardinal : c'est un ensemble, transporté par
$B_e=B(v)\cup D_-(d)$ puis $B(w)=B_e\setminus\lbrace i\in A:\ i\in B_e\rbrace$, et
le census de contrôle compare désormais l'**ensemble**, pas sa taille. Trois
conséquences : les événements « sortants » d'une requête sont exactement les
membres de $B(v)$, donc gratuits ; la récolte dispose d'un **préfiltre nécessaire
de propriété** — un sommet propriétaire doit avoir tout son intérieur dans la
boule de ce support, mais ce test ne l'unicise pas — qui écarte **88,6 %** des tentatives à
$s_{\max}=11$ sans payer de census — c'est un préfiltre **nécessaire**, pas une
reconnaissance du propriétaire canonique, et la table globale `emitted` reste
donc indispensable ; et les singletons se publient en temps
constant, ce qui supprime les $2{,}5\cdot10^9$ classifications que l'audit
comptait avant le germe.

### Ce que la mesure dit

**[mesuré]** profil LiDAR, un cœur, `g++ -O3 -march=native`, codespace 2 vCPU,
même binaire pour les deux colonnes :

| $s_{\max}$ | $n$ | référence | indexé | facteur | candidats/sommet |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 5 | 200 | 14,3 s | **1,3 s** | 10,7 | 768 → 160 |
| 5 | 400 | 54,4 s | **4,5 s** | 12,2 | — |
| 5 | 800 | — | **12,5 s** | — | — |
| 11 | 100 | 30,9 s | **11,0 s** | 2,8 | 768 → 122 |
| 11 | 200 | — | **20,6 s** | — | 155 |

Il faut lire la dernière ligne et pas la première. **À l'ordre du contrat le
facteur tombe à 2,8**, parce que les sphères d'un niveau profond sont grandes :
l'arbre élague moins, et le travail se déplace des prédicats exacts — divisés par
6,3 — vers le parcours de l'arbre. Extrapolé sans prudence depuis $n=200$,
$s_{\max}=11$ : 110 µs par sommet, environ $5{,}8\cdot10^7$ sommets à 50 000
points, soit près de deux heures sur un cœur et environ 130 s sur 48. **Le
contrat reste à deux ordres de grandeur.**

L'index est donc une brique positive et un P0 fermé, pas l'architecture. Ce qu'il
ne touche pas est inchangé : `seen`, `frontier` et les sommets visités résident
tous ; les flats sont énumérés depuis les triplets ; le propriétaire n'est pas
encore implémenté et il n'y a ni parcours reverse-search, ni streaming aval, ni
forêts.

---

## 4 ter. Gate D — le parent est local, et c'est ce qui ouvre la voie GPU

[`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](audits/NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md)
démontre qu'un parent unique se choisit **au sommet**, sans `seen`, sans mosaïque
globale, et sans énumérer les voisins pour décider lequel est le parent. Le cône
tangent de la chambre en $v$ est $K_v=\lbrace d:a_s\cdot d\ge0\ \forall s\in S(v)\rbrace$
et ses rayons extrêmes sont exactement les orientations des flats incidents.
Le premier juge réutilise ceux que le parcours énumère déjà : il filtre, puis
choisit canoniquement. La variante par petit programme linéaire exact évite aussi
d'énumérer tous ces flats pour calculer le parent.

**Le signe tangent ne coûte rien.** Pour une base planaire $(a,b,c)$ et
$u=(b-a)\times(c-a)$, un rayon entier du pinceau est $d=(u,2u\cdot a)$, et avec
$a_i=(-2p_i,1)$ on a l'identité $a_i\cdot d=-2\,\mathrm{orient3d}(a,b,c,p_i)$.
J'avais d'abord dérivé la direction depuis le circumcentre, ce qui frôlait
$2^{127}$ et m'obligeait à passer en `BigInt<4>` ; les deux formes coïncident,
puisque $(c_0-a)\cdot u=0$. Le filtre se lit donc avec le prédicat entier que le
pinceau évalue déjà.

Deux filtres, et la preuve n'exige rien d'autre : l'orientation doit rester dans
la chambre — aucun membre de coquille ne devient intérieur — et elle doit faire
croître $L_h$ pour $h=\min B(v)$, ou décroître $Q_r$ au niveau zéro.

**Un P0 trouvé et corrigé, et c'est le même que l'audit a levé.** La base du
potentiel du germe doit être **affinement indépendante**. Prendre les quatre
premiers membres de la coquille ne suffit pas : sur

```text
(0,0,1) (0,1,0) (0,1,1) (1,0,0) (1,1,0) (2,0,0)
```

la coquille du germe est $\lbrace0,2,3,4,5\rbrace$ et ses quatre premiers membres
sont coplanaires dans $x+z=1$ ; $Q_r$ perd le germe pour unique zéro et un
second sommet de niveau zéro devient une **seconde racine**. Mesuré avant
correction : un nuage sur six cents, mais à tous les ordres. L'extraction est
maintenant gloutonne et vérifiée, et un échec rend `kInvariantViolated`.

**[mesuré]** un rejeu exact externe sur le snapshot du parent couvre 5 623
nuages, 146 729 sommets et 15 258 sommets multiples. Il vérifie le rang quatre
de la base, une racine unique, $B(\pi(v))\subseteq B(v)$, 123 240 hausses
rationnelles strictes de $L_h$, 17 866 baisses rationnelles strictes de $Q_r$ et
l'absence de cycle : **zéro échec**. La porte permanente en vérifie actuellement
un sous-ensemble. Les quatre témoins de la note sont des fixtures permanentes —
`germe_base_non_independante`, `lex_admissible_cycle`, `lp_optimum_tie`,
`level_zero_lex_cycle`.

### Pourquoi c'est la route du GPU, et pas les 48 cœurs

Le parent local transforme l'énumération en test **sans état global de
visitation** : on descend de $v$ vers $w$ si et seulement si $\pi(w)=v$. Il n'y
a plus de `seen` à partager, donc plus de déduplication atomique des sommets ni
de table résidente proportionnelle au nombre visité. Des sous-arbres deviennent
traitables indépendamment, ce qui est une condition favorable au GPU.

Ce résultat ne chiffre aucun facteur d'accélération et ne supprime pas les
écritures globales de sortie. Les workers doivent encore produire des runs, le
merge doit fermer les ex æquo en $\beta$, et le réducteur horizontal puis les
verticales doivent communiquer. Le parent ouvre la voie device; seul un kernel
mesuré sur la G4 pourra en établir le débit.

**Ce que cela ne ferme pas**, et la note le dit avant moi : les **enfants**
exigent toujours tous les flats incidents réels, une grande coquille peut en
avoir un nombre combinatoire, et le parent local ne borne aucun temps. Le
prototype garde `seen`, `frontier` et `visited` : la porte juge le parent, elle
ne l'a pas encore substitué au parcours. Avant de le faire il faut encore, selon
la note, vérifier le rang trois de $C(d)$, l'identité
$S(\mathrm{next})=C(d)\cup A$, la finitude de l'extrémité et la stricte variation
du potentiel. Même après cette substitution resteront la source complète des
incidences silencieuses, le tri et les lots exacts, la partition horizontale,
`coverage_log` et la jointure verticale. Leur factorisation est dans
[`NOTE_GATE_D_GLOBALITES_RESIDUELLES.md`](audits/NOTE_GATE_D_GLOBALITES_RESIDUELLES.md).

---

## 4 quater. La source silencieuse n'est plus une inconnue mathématique

[`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](audits/NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md)
retire du dossier le verrou que je venais de désigner comme prioritaire. Pour une
facette $F$ du cœur, de miniboule fermée $B_F$ et de niveau $b_F$, en posant
$E_F=(B_F\cap X)\setminus F$, la première incidence se décide **sans aucune
recherche de voisinage** :

- **branche fermée**, $E_F\neq\varnothing$ : alors $\lambda(F)=b_F$ et $M(F)=\lbrace F\cup\lbrace x\rbrace:x\in E_F\rbrace$. La preuve tient en deux lignes et n'exige **aucune** hypothèse de régularité ;
- **branche vide**, $E_F=\varnothing$ : alors $\lambda(F)$ est le minimum des niveaux des cofaces **directes** contenant $F$, et $M(F)$ en est le groupe d'ex æquo — tout minimiseur est de Gabriel au sens ouvert, sinon un intrus strict fournirait une incidence strictement moins chère.

`mhgp3v_first_incidence` mesure cette dichotomie et la **juge** contre une vérité
exhaustive écrite dans le même fichier : pour chaque facette, le minimum de
$\beta$ sur tous les points extérieurs et l'ensemble complet de ses ex æquo.

**[mesuré]** 60 nuages de 11 points par ordre, grille $[0,22)$, coordonnées
distinctes :

| $k$ | cofaces directes | records/coface | facettes du cœur | branche fermée | co-minimiseurs (moy./max) | points touchés/facette | désaccords |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 | 1 457 | 3,00 | 1 952 | 40,5 % | 1,02 / 3 | 11,0 | **0** |
| 3 | 1 543 | 4,00 | 3 447 | 57,7 % | 1,02 / 3 | 11,0 | **0** |
| 4 | 1 438 | 5,00 | 4 597 | 66,4 % | 1,02 / 3 | 11,0 | **0** |
| 5 | 1 222 | 6,00 | 5 101 | 71,8 % | 1,02 / 3 | 11,0 | **0** |

Quatre lectures, et elles vont toutes dans le même sens :

- l'**identité de masse** est exacte à tous les ordres — exactement $k+1$ records de suppression par coface directe, ce qui rend le flux dimensionnable ;
- la **branche fermée** domine et croît avec $k$ ; elle ne coûte qu'une requête de boule fermée ;
- les **co-minimiseurs** sont minuscules — moyenne 1,02, maximum 3 — donc les lots atomiques de première incidence sont petits ;
- la requête certifiée ne touche que **onze points par facette**, et ce nombre ne bouge pas avec $k$.

**Ce que cela ne ferme pas.** Ce binaire ne remplace pas l'oracle général, qui
reste le juge hostile et le repli hors porte. Le regroupement est en mémoire :
ce sont les **volumes** qui sont publiés, pas un tri externe. Et la dichotomie
produit $M(F)$ ; elle ne produit ni l'autorité de régularité qui autorise la
rétraction vers la forêt $H_0$ normalisée, ni le réducteur, ni les verticales,
ni l'identité de sortie.

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

**[mesuré]** profil LiDAR à densité fixe, emprise $\propto\sqrt{n}$, $s_{\max}=11$
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

1. **La requête de pinceau était en $O(n)$.** L'index exact de `1a0a1f8` retire ce scan systématique, sans promettre une visite sous-linéaire au pire. À l'ordre du contrat, le facteur CPU mesuré reste seulement 2,8.
2. **La récolte payait un census en $O(n)$ par candidat, et 43 % de ses tentatives étaient des doublons.** Le census fermé indexé est écrit. Le préfiltre de propriété écarte une part réelle du travail, mais seul le couple « support canonique puis propriétaire exact » supprimera les duplications restantes et la table `emitted`.

**Ce que je ne dis pas :** que le contrat est atteignable. Les deux ratios
croissent encore à $n=300$, la sortie à 50 000 points serait de l'ordre de
$3\cdot10^6$ sphères et $3\cdot10^7$ identifiants de membres, et aucun de ces
deux nombres n'est mesuré — ils sont extrapolés d'une croissance non stabilisée,
et je les donne comme tels.

### La taille du terrain n'est pas garantie

Le parcours énumère les sommets géométriques de l'arrangement relevé dont le
niveau strict est sous le plafond. Sous position générale, le pavage de
rhomboïdes est dual de cet arrangement et sa tranche de profondeur $k$ est la
mosaïque de Delaunay d'ordre $k$. La dualité puis la tranche changent les
dimensions : en dimension trois, un sommet de l'arrangement est dual d'un
rhomboïde de dimension quatre, dont les tranches non triviales donnent des
cellules Delaunay tridimensionnelles à plusieurs ordres. Ce n'est pas un sommet
de mosaïque en bijection un-à-un.

La fixture u16 la plus petite suffit à écarter toute bijection naïve : pour les
quatre sommets d'un tétraèdre, l'arrangement relevé possède un sommet de niveau
zéro, tandis que la mosaïque de Delaunay d'ordre un possède quatre sommets et un
tétraèdre. Avec les coquilles multiples, un même record `(niveau, coquille)`
peut en outre contribuer à plusieurs ordres et cellules.

| objet | quantité effectivement portée |
| --- | --- |
| parcours actuel | tous les sommets d'arrangement shallow, leur coquille et leur niveau |
| mosaïques d'ordre supérieur | sommets, cellules de toutes dimensions, incidences et étiquettes par ordre |
| structures évitées par le parcours | cellules Delaunay/Voronoï, incidences et dual matérialisés |
| résident simultanément dans le prototype | `seen`, `frontier` et tous les sommets déjà visités |

Le gain acquis est donc réel — les cellules, incidences et étiquettes par ordre
ne sont jamais formées — mais aucun facteur mémoire cinq à dix ne découle d'une
identité de nombres de sommets. Le gain qui compterait, ne pas tout retenir,
n'est pas obtenu : `seen` et le vecteur des sommets visités retiennent
l'ensemble du terrain parcouru.

La borne classique de Clarkson--Shor pour le $\leq k$-niveau de $n$
hyperplans de $\mathbb{R}^4$ est $O(n^2k^2)$ sous ses hypothèses asymptotiques
et de position générale; pour $K=10$ fixé, elle est quadratique en $n$. C'est
un théorème utilisable pour situer le pire cas, mais pas une borne compatible
avec le SLO 50 k. Les mesures ci-dessus sont plusieurs ordres de grandeur en
dessous parce qu'un relevé est localement une surface : c'est une propriété du
**régime**, pas un théorème. Le §14 de [`PROPOSITION.md`](PROPOSITION.md) a déjà
retiré l'énoncé voisin « surface $\Rightarrow$ faible profondeur presque
partout » comme census et non comme théorème. Voir
[Edelsbrunner--Osang](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf)
pour la dualité et [Clarkson--Shor, corollaire 3.3](https://link.springer.com/content/pdf/10.1007/BF02187740.pdf)
pour la borne.

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

## 5 bis. Pour la navigation, mémoire et GPU ont le même verrou

C'est le point qui commande l'ordre des travaux, et il n'était écrit nulle part.

La structure qui coûte la mémoire de navigation est `seen`, la table des
coquilles déjà visitées. C'est aussi celle qui interdit une expansion GPU sans
communication : une table de hachage globale, à clefs de longueur variable,
écrite par tous les fils. Pour le **parcours de l'arrangement**, c'est un verrou
unique. Le pipeline HGP aval conserve d'autres globalités indépendantes.

La technique qui l'élimine est connue — la *reverse search* d'Avis et Fukuda.
Une règle locale et déterministe désigne, depuis tout sommet, l'unique voisin
par lequel on y serait arrivé ; l'arbre de parcours devient implicite et un
sommet n'est publié que par son parent. Trois conséquences d'un seul coup :

1. la mémoire privée de navigation tombe à $O(\text{profondeur})$ au lieu du nombre de sommets ;
2. deux sous-arbres n'échangent plus d'état de visitation, ce qui permet leur expansion parallèle ;
3. l'arbre devient déterministe sous une clef canonique, tandis que le déterminisme byte-à-byte de sortie reste celui du tri secondaire et des lots.

Le dépôt en possède déjà une preuve constructive,
[`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](audits/AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md),
pour l'arrangement simple. Son extension au vrai graphe multiplicitaire et une
règle qui choisit directement un rayon du parent sont maintenant prouvées dans
[`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](audits/NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md).
L'implémentation de reverse search reste ouverte : le live ne fait encore que
juger le parent depuis le BFS avec `seen`.

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
| état horizontal | non écrit | locator et partition actifs, couverture append-only, stockage externe autorisé |
| couture verticale | non écrite | requêtes vers l'ordre adjacent, tri puis sweep à coupe fermée |
| identités du contrat v2 | non produites | facettes, cofaces et provenances streamées, ou migration quotientée explicitement versionnée |

**Ce qu'un GPU n'achètera pas.** Il multiplie le débit ; il ne réduit pas le
nombre de sommets d'arrangement visités. La question de fond n'est donc pas
« aller plus vite sur ce terrain » mais « peut-on éviter d'en traverser la plus
grande part ». Aucun portage n'y répond.

---

## 5 ter. Commencer sous hypothèse de non-cosphéricité

**Décision de séquencement.** Le premier algorithme complet sera écrit pour un
arrangement **simple** — aucune cosphéricité, aucune coplanarité portante — et
le traitement des dégénérescences est reporté à l'une des **dernières phases**
du projet.

La raison n'est pas une impossibilité mathématique du parent multiplicitaire :
celui-ci est désormais prouvé sous coquille, intérieur et oracle `next` exacts.
Le cas simple reste un séquencement d'implémentation parce que l'énumérateur de
flats multiples, le propriétaire des basses arités et leur oracle indépendant
ne sont pas encore qualifiés. On y règle l'architecture — index, propriétaire,
streaming, tri, forme GPU — avant de rouvrir ces coûts et contrats
multiplicataires.

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
code non nul avec son diagnostic ; **neuf** tests négatifs le vérifient — argument
inconnu, `--smax` hors contrat, plancher de cas, plancher de couverture, suffixe
entier, coordonnée hors grille, troncature de `--clouds`, troncature de
`--coord`, et campagne combinatoirement impossible.

---

## 7. Ce qui reste ouvert, sans ordre de facilité

| # | question | statut |
| --- | --- | --- |
| 1 | index spatial *fail-open* pour la requête de pinceau | écrit et différencié au commit `1a0a1f8`; propriété immuable de l'index, compteurs d'élagage et preuve complète du petit fast-path flottant restent ouverts |
| 2 | règle de propriétaire pour les arités 2 et 3, et census local | existence et propriétaire canonique prouvés; le live n'a qu'un préfiltre nécessaire, puis conserve `emitted` |
| 3 | reverse search, pour supprimer `seen` et `frontier` | parent multiplicitaire prouvé; premier juge live en cours, parcours reverse-search et streaming non écrits |
| 4 | référence de l'oracle M1 tolérante aux multiplicités | non écrite ; sans elle le sujet n'a pas de juge indépendant en arithmétique rationnelle |
| 5 | source active/silencieuse, tri et lots, état horizontal, `coverage_log`, verticales et contrat d'identité | non écrits; globalités intrinsèques mais externalisables, factorisées dans la note Gate D aval |
| 6 | invariance topologique du support canonique quand plusieurs supports minimaux portent la même miniboule | ouverte ; la convention par coordonnées est *une* convention, pas un théorème |
| 6 bis | sémantique quotientée des observations confondues | ouverte ; le prototype les **refuse** explicitement plutôt que de publier un support dépendant de la numérotation |
| 7 | `sphere.hpp` au bord produit : paire de points confondus acceptée comme support d'arité deux, sentinelle `den==0` sans garde | ouverts, hors de ce fichier |
| 8 | le contrat 50 k / $K=10$ / 1 s | **non atteint, non mesuré, et les deux ratios qui le décident croissent encore à $n=300$** |
| 9 | publication directe des $n$ singletons | fermée dans le chemin indexé de `1a0a1f8`; le différentiel conserve le census du chemin lent comme oracle relatif |
| 10 | la taille $V$ du $\leq k$-niveau en général | **non bornée utilement** : Clarkson--Shor est quadratique en $n$ et en $k$, et les mesures ne valent que pour le régime de surface (§5) |
| 11 | le régime multi-captation | mesuré **moins peu profond** que la reconstruction fusionnée ; c'est la branche no-go de Gate D |

Le détail, les budgets et le journal des affirmations retirées sont dans
[`PROPOSITION.md`](PROPOSITION.md).
