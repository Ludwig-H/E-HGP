# MorseHGP3D v3

État : **`exploration_v3_hors_registre`**. Backend courant : CPU de référence,
oracles bornés et microkernel GPU candidat sous audit. Profil exercé : **entrée
u16 quantifiée seulement**. Aucun statut public, aucun SLO et aucune phase ne
sont ouverts au registre.

L'état audité du snapshot committé `81f9210` est scellé dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

**Trois des quatre P0 de cet audit sont fonctionnellement fermés sur leur domaine
borné.** La troncature `i128` du chemin `use_owner` est réduite par
`sign_of` et les appels bruts `i128`/`long long` sont interdits; le minimum en
demi-plan du filtre de paires est calculé par un balayage exact au lieu des
seules directions vives, et ses masses sont republiées; le noyau F0 accepte
enfin la naissance tout $N_a$ que son contrat écrit autorisait, sous un
troisième oracle qui ne ferme rien transitivement. Pour le quatrième, le delta
replay ferme maintenant la vacuité et tue le mutant qui refuse tout, mais ne
rejoue encore aucun payload : il crédite seulement des masses scalaires.

L'audit conserve trois réserves de garde : les surcharges owner supprimées ne
forment pas encore un type fort fermé à toutes les petites conversions entières;
le validateur régulier F0 accepte un handle strict dupliqué; ses deux CTests ne
sont pas enregistrés si Python est absent. La fixture du cône signé ne protège
toujours pas l'identité du propriétaire. Les
fermetures constructives sont dans la
[`note des verrous mathématiques prioritaires`](audits/NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md).
Le passage GPU est spécifié séparément dans la
[`note des verrous mathématiques GPU`](audits/NOTE_VERROUS_MATHEMATIQUES_GPU.md) :
largeurs exactes 64/128/384 bits, voisin terminal, sous-arbres transactionnels,
owner/census, runs et porte 50 k/G4.

Le prototype `direct_source.cpp` est un résultat mathématique positif, pas une
source produit certifiée. Des oracles indépendants valident son cover--rayon et
ses voisinages bornés sans écart, et sa partie candidate évite arrangement et
mosaïque. Le palier `bb31b426...`, intégré à `81f9210`, conserve les membres,
tue les doubles
émissions, applique les fallbacks, publie les masses en `u128` et compare un
quotient sémantique de 30 forêts; 13 CTests Release et huit passages ciblés
ASan/UBSan/LSan sont verts. Bas ordre, borne $K+1\le s_{\max}$ et juge explicite
sont fermés. Il ne compare toutefois ni l'ordre canonique du catalogue, ni son
pool concaténé, ni les indices publics `ForestNode::source`; son nouveau contrôle
structurel boucle ou sort du tableau sur certaines forêts malformées. Le
coût/mémoire 50 k reste ouvert. Le
verdict épinglé est dans
[`AUDIT_SOURCE_DIRECTE_24AD3D37.md`](audits/AUDIT_SOURCE_DIRECTE_24AD3D37.md).

Un rebuild Release CPU complet du commit `81f9210` passe 73/73 CTests en
351,62 s avec GCC 13.3, GMP et Python actifs; 69 tests appartiennent à la v3 et
quatre sont des dépendances transitives v2. Ce reçu positif vérifie
l'intégration du snapshot, pas le contrat de débit, de mémoire ou de replay.

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

Une miniboule peut avoir **plusieurs** supports. Le cube cosphérique en a six au
sens inclusion-minimal : quatre paires antipodales et deux tétraèdres de parité.
Les quatre paires sont les seuls supports de cardinalité minimale; c'est parmi
elles que la convention canonique tranche d'abord. Trois conventions, mesurées :

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

Quatre portes, et il faut les quatre :

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
violation dans le snapshot de ces mesures. Le juge live a depuis ajouté le rang
de la base, la variation exacte du potentiel, la fermeture du flat et plusieurs
contradictions fail-closed. Ce n'est toujours pas Gate D entière : il ne prouve
pas que le sommet rendu est le premier événement orienté, et les mutations qui
suppriment ou tronquent la porte ne sont pas toutes gravées.

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
- Pas de forêts ni de reverse search **dans le catalogue**. Un propriétaire exact
  expérimental est calculé sur la récolte naviguée et rejoué sous permutation,
  mais cette signature omet membres et multiplicités. Il est désactivé par défaut
  et n'a pas encore de fixture permanente `owner_signed_cone`.

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
boule de ce support. Le live ajoute ensuite « support canonique puis
propriétaire » sur son chemin expérimental. `emitted` reste la référence du
différentiel, mais sa table est vide dans le chemin owner+index+navigable; les
voies sans index ou directes gardent le repli hybride. Les singletons se publient
en temps
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

L'index est donc une brique positive et un P0 fermé, pas l'architecture. Il ne
borne ni le nombre de flats ni le nombre de sommets. Un endpoint séparé de
reverse search n'utilise plus `seen` ni `frontier` pour décider le parcours; le
catalogue live passe toutefois encore par le BFS. L'API sink existe, et le
propriétaire d'émission est expérimenté sur le seul chemin indexé affine 3D,
mais ni l'un ni l'autre n'est intégré au flux transactionnel aval; il n'y a pas
encore de forêts.

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

### La reverse search est écrite, et elle visite le même ensemble

Le théorème de parent rend l'énumération **sans table globale de visitation** :
on descend de $v$ vers $w$ si et seulement si $\pi(w)=v$, donc aucune
déduplication n'est nécessaire pour décider le parcours.
`reverse_search_shallow` l'implémente, et le différentiel exige qu'elle rende
**exactement** le même ensemble de sommets que le BFS — mêmes coquilles, mêmes
ensembles intérieurs, même statut. C'est la porte qui autorise à retirer les
tables globales : un parent légèrement faux produirait un sous-arbre tronqué que
seule cette comparaison verrait.

L'état qui décide la navigation est désormais la **pile** : par niveau, le
sommet courant et l'indice du fils. Il ne contient ni `seen` ni `frontier`. Les
voisins sont énumérés dans un ordre déterministe — flats de la coquille, puis
les deux orientations — sans quoi l'indice du fils ne serait pas reproductible
au retour, ce qui est toute la mécanique d'Avis–Fukuda. L'enveloppe de
comparaison conserve encore un `std::vector<Vertex> visited` pour se confronter
au BFS; ce vecteur n'intervient dans aucune décision. Le delta live ajoute plus
bas un endpoint sink qui ne matérialise pas cette sortie, sans encore l'intégrer
au catalogue.

**[reproductible]** les quatre portes CTest positives du commit `969db5c`, tout
comparé au BFS :

| campagne | cas | désaccords | sommets par reverse search | profondeur max | fils testés / sommet |
| --- | ---: | ---: | ---: | ---: | ---: |
| fixtures | 211 | **0** | 1 578 | 7 | 5,3 |
| générique | 1 184 | **0** | 110 873 | **21** | 6,0 |
| grille saturée | 1 291 | **0** | 111 170 | 17 | 6,5 |
| cosphérique | 2 071 | **0** | 101 877 | 16 | 6,0 |

Au total : 4 757 cas, 325 498 sommets, 2 012 590 fils testés, profondeur maximale
21 et zéro désaccord. Les valeurs six et 21 sont des mesures de ces campagnes,
pas des bornes. Une grande coquille peut rendre le nombre de flats combinatoire.

**Ce que cela ne rend pas.** Aucune borne de temps. Au commit `aec7439`, chaque
fils testé paie encore la requête de voisin **aller**, puis un préfiltre en
$O(m)$; les survivants énumèrent le préfixe des couples du candidat jusqu'à la
clef retour, mais ne paient plus de seconde requête de voisin. Une grande
coquille peut avoir un nombre combinatoire de flats, et six fils par sommet est
une mesure de régime, pas un théorème. Au commit `969db5c`, le compteur
n'incluait ni les voisins non bornés ou hors coupe, ni la queue de triplets que
`for_each_flat` continuait à examiner après avoir trouvé le prochain enfant;
les compteurs actuels couvrent désormais ce travail.

Le travail réel est maintenant mesuré, et **il est cinq fois plus grand que ce que
« six flats par sommet » laissait croire**. `for_each_flat` est interruptible, et
deux compteurs comptent désormais tous les triplets balayés et toutes les
fermetures reconstruites, y compris celles des triplets écartés parce qu'ils ne
sont pas la base canonique de leur fermeture. Sur la campagne générique à 11
points : 10 626 sommets, 72 236 flats **livrés** — 6,8 par sommet — mais
**335 314 triplets balayés et autant de fermetures reconstruites**, soit **31,6
par sommet**, chacune coûtant un `orient3d` par point de la coquille.

Ces deux compteurs sont d'ailleurs toujours égaux, et ce n'est pas un hasard de
régime : trois points **distincts** d'une même sphère ne sont jamais alignés,
puisqu'une droite coupe une sphère en au plus deux points. Le garde de
colinéarité est donc inactif sur une coquille, et la porte exige leur égalité —
une divergence dénoncerait une coquille non cosphérique ou un point double.

Chaque requête alloue aussi un bitmap et une liste de candidats de taille $O(n)$
au pire; la pile recopie les coquilles des sommets sur le chemin. Le commit
`969db5c` n'appelait que le balayage exhaustif. Le rejeu indexé le double
désormais avec des feuilles de **quatre** — seize laissait une feuille unique sur
des nuages de treize points, donc aucun élagage interne —, une porte permanente à
vingt points, des compteurs de nœuds **visités** et non seulement construits, et
l'auto-test juge maintenant `box` et le désaccord ternaire contre l'exhaustif, pas
seulement `closed_ball`. Sur la porte à vingt points : 1 105 239 nœuds visités
dont 472 443 feuilles, profondeur 19. `box` avait de plus un chemin fail-open —
pile saturée, sous-arbre **omis en silence** là où les deux autres requêtes
retombaient sur une descente récursive ; il retombe désormais comme elles.

### Refuser un fils sans calculer son parent : un facteur 1,6 de plus

Le poste dominant restait les 5,9 requêtes de parent par sommet. La question n'est
pourtant pas « quel est le parent de $w$ » mais « $v$ est-il le parent de $w$ », et
cette question a une **condition nécessaire en $O(m)$**.

Si $w$ est notre fils, $\pi(w)$ emprunte le **même plan** en direction opposée.
Tester l'admissibilité de ce seul couple $(G,-\delta)$ coûte un `orient3d` par point
de la coquille de $w$ — aucune fermeture, aucune énumération. S'il échoue,
$\pi(w)\neq v$ est **certifié**, et le candidat est refusé pour rien.

**[mesuré]** grille cosphérique, 40 nuages : sur 107 114 fils testés, **65 044 —
61 % — sont refusés par ce seul test en $O(m)$**, et 24 580 seulement par la
comparaison de coquille qui vient après. Les requêtes de parent tombent de
105 172 à **42 070**, les fermetures reconstruites de 308 832 à **192 570**, et les
nœuds d'index visités de 3 322 180 à 2 905 996. Zéro désaccord sur 521 cas.

Cumulé avec la sortie précoce du parent, sur la campagne générique à onze points :
**335 314 → 171 856 → 108 856 fermetures**, soit 31,6 → 16,2 → **10,2 par
sommet**. Un facteur trois sur le poste de coût, en deux corrections dont aucune ne
change la sortie.

Le live emploie le corollaire plus fort : après un retour admissible,
l'adjacence du pinceau rend la seconde requête de voisin inutile.
`decide_child` cherche seulement un couple admissible antérieur à la clef
retour. Le différentiel rejoue chaque couple contre le chemin complet — parent
canonique, voisin retour, comparaison de coquille — et exige les identités de
comptage par nuage. Sur la porte générique courante, **269 918 requêtes de
retour** disparaissent.

La qualification a été renforcée : les directions hors
$\lbrace-1,+1\rbrace$ sont rejetées, les compteurs `pairs_judged`,
`pairs_accepted` et `pairs_prefiltered` ont des planchers, et les appels directs
couvrent un couple antérieur admissible, une clef cible dépassée et une base
divergente. Elle reste relative : le chemin complet et le décideur partagent
`pair_admissible`, et aucune mutation n'altère encore une vraie étape
d'énumération pour vérifier que le juge la détecte.

### Le curseur courant reprend exactement après le fils

Le frame courant conserve `(i,j,k,dir)`. Après un fils en direction négative, il
reprend la direction positive du même flat; après la positive, il avance au
triplet suivant. Sur la porte fixtures, à sorties et décisions identiques, le
curseur réduit les flats livrés de **10 753 à 8 060** et les fermetures de
**15 667 à 12 736**. Les cinq portes différentielles restent vertes. Ce gain
mesuré ne donne toujours aucune borne sur une grande coquille; des mutations
ciblées premier/dernier triplet, reprise après chaque slot et fermeture non
canonique restent utiles.

### La sortie est bornée, et le high-water est mesuré

C'était la dette la plus lourde : tant que l'endpoint rendait un
`std::vector<Vertex>`, la sortie était $\Omega(V)$ et **aucun** gain mémoire
n'était démontré, quelle que fût l'absence de table de visitation.

`reverse_search_stream` rend maintenant les sommets **un à un** à un sink, et
publie le high-water des identifiants portés par les sommets du chemin, sortie
exclue. Cette métrique ne compte ni la capacité de la pile, ni le candidat et son
parent temporaires, ni les fermetures, bitmaps et listes de scratch d'une requête.
L'enveloppe qui matérialise reste le sujet du différentiel; le catalogue ne
consomme pas encore le sink.

La porte le juge avec un consommateur qui ne tient **rien** : il compte et replie
un hachage 64 bits indépendant de l'ordre. C'est un falsificateur compact, pas
une égalité exacte en raison des collisions possibles; un curseur comparant
chaque callback à l'oracle matérialisé fermerait cette dette sans accumuler la
sortie côté sink. L'interruption est une porte, pas une
politesse : un sink qui s'arrête doit stopper le parcours et rendre un statut non
`kOk`, sans quoi une sortie tronquée passerait pour complète. Le test actuel
refuse toutefois le **germe** dès le premier callback : il a attrapé ce trou,
mais n'exerce ni l'arrêt après un préfixe ni la branche d'interruption depuis un
enfant.

**[mesuré]** porte à vingt points, 5 nuages : 5 400 sommets rendus au sink,
**85 identifiants de sommets actifs au maximum**, 109 interruptions vérifiées,
zéro désaccord détecté avec la sortie matérialisée. Le rapport $5400/85$ ne peut
pas être appelé facteur mémoire : le numérateur compte des records, le
dénominateur des identifiants, et le scratch est omis. Une coquille cosphérique
peut être de taille $\Theta(n)$ et la profondeur n'est bornée par aucun théorème.
Le gain acquis est l'absence de table de déduplication **dans la décision** et une
API de sortie streamée. Le replay du sink est en outre lancé sans lui passer le
`CertifiedIndex`; la composition streamée-indexée n'est pas encore qualifiée.
L'intégration devra écrire dans un segment non committé : une erreur peut
survenir après plusieurs callbacks. `kSinkStopped` distingue désormais l'arrêt
volontaire d'une violation d'invariant; le test interrompt au germe puis après
un préfixe de trois sommets. Le high-water
mémoire complet, l'intégration transactionnelle au catalogue et l'absence
d'écriture partagée d'un kernel réel restent à obtenir.

### La règle owner est constructive, mais le chemin u16 reste NO-GO

Streamer les sommets ne suffit pas tant que le **catalogue** garde `emitted`, une
table de déduplication proportionnelle à la sortie. La note d'audit prouve la
règle candidate, et le cube u16 dit pourquoi un propriétaire par support ne
suffit pas : $\lbrace0,2\rbrace^3$ a **six** supports inclusion-minimaux pour une
seule boule — quatre diagonales, seules de cardinalité minimale, et deux
tétraèdres de parité. Il faut donc canoniser
**d'abord**, posséder **ensuite** :

1. recenser exactement la boule fermée et refuser son rang s'il dépasse le contrat ;
2. calculer sur sa coquille le support minimal canonique $U_{\mathrm{can}}$ ;
3. rejeter tout candidat $U\neq U_{\mathrm{can}}$ — c'est la déduplication **dans** un sommet ;
4. tester que le sommet courant est $o(U_{\mathrm{can}})$ — c'est la déduplication **entre** sommets ;
5. émettre une fois.

Le test de propriété est local et exact : avec $\varepsilon_s=-1$ sur
$B_U\cap S(v)$ et $+1$ ailleurs, $v=o(U)$ si et seulement si aucun rayon extrême du
cône tangent **signé** n'a $(g_U\cdot d,d_0,d_1,d_2,d_3)$ lexicographiquement
négatif. Le gradient ne coûte pas $O(n)$ : $A_X=\sum_i a_i$ est précalculé une
fois, les termes de $U$ s'annulent sur le cône, et $|B_U|\leq s_{\max}-|U|$. Tout se
lit avec le même prédicat entier que le pinceau, par l'identité
$a_i\cdot d=-2\,\mathrm{orient3d}$.

#### Le P0 de troncature est fermé, et il ne pouvait pas être fermé par une campagne

Cette identité exacte était rompue à son appel : `owner_rays_ok` passait le
déterminant `i128` brut à `tangent_sign(int, ...)`. Ce n'était pas une perte de
précision mais une perte de **signe**, donc un cône tangent faux, donc un
propriétaire faux, donc des sphères supprimées du catalogue.

Les deux échelles qui le révèlent ne sont pas choisies : ce sont les deux
frontières arithmétiques du prédicat sur la grille déclarée.

| nuage | déterminant | frontière | catalogue normal | catalogue owner avant |
| --- | --- | --- | ---: | ---: |
| tétraèdre axial d'arête $s$ | $s^3$ | franchit $2^{31}$ entre 1290 et 1291 | 7 / 10 / 11 | 7 puis **4** |
| tétraèdre alterné d'arête $s$ | $-2s^3$ | vaut exactement `INT_MIN` à 1024 | 10 / 14 / 15 | 10 puis **4** |

À 1024 le mal n'était même pas un signe faux : `tangent_sign` niait `INT_MIN`,
comportement indéfini, et Release rendait le bon catalogue par accident pendant
qu'UBSan criait. C'est la raison pour laquelle **aucune campagne ne pouvait
fermer cette porte** : toutes les portes permanentes tenaient dans une boîte de
côté au plus quarante, où le déterminant reste très en dessous de $2^{31}$.

La correction réduit le prédicat par `sign_of` **avant** l'appel. Pour que la
faute ne puisse pas revenir par inadvertance, les surcharges `i128` et
`long long` de `tangent_sign` sont maintenant **supprimées** : passer un
déterminant là où un signe est attendu est devenu une erreur de compilation.

Deux portes permanentes la gardent. La fixture `[frontière u16 owner]` grave les
sept échelles 1290/1291/2048 et 1023/1024/1025/1626, aux ordres 2 à 4 et dans
les deux modes d'index, et exige en plus un **témoin** : au moins un déterminant
réellement évalué doit changer de signe par troncature 32 bits — sans ce second
temps, la fixture pourrait devenir muette sans jamais rougir. Elle en compte 84.
Le mutant de troncature réintroduit rend exactement les nombres de l'audit :
**18 désaccords**, 7 contre 4 à l'échelle 1291 et 10 contre 4 à 1025.

La campagne `mhgp3v_flats_u16_owner` navigue enfin sur la grille entière —
60 nuages, `coord=65536` — là où le contrat u16 est réellement exercé.

**[mesuré]** les 54 CTests Release passent en 756,75 s de temps mur avec deux
tests en parallèle, planchers de couverture compris. La campagne u16 déterministe
que l'audit citait en contre-exemple, `--clouds 1 --points 8 --coord 65536
--smax 2 --seed 20260809`, rendait `OWNER != EMITTED : 16 sphères contre 19`;
elle rend maintenant 216 cas et zéro désaccord.

La frontière est explicite : **le propriétaire couvre seulement la récolte
naviguée**. Les singletons sans index et la voie directe n'ont aucun sommet de
$P_U$ et conservent le repli exact `emitted`. Une sonde indépendante sur les
quatre combinaisons index × propriétaire, couvrant 174 444 exécutions et
41 430 permutations, trouve les mêmes statuts, supports, rangs, membres et
niveaux exacts. Le tétraèdre rend 11 sphères partout, le triangle direct 7, et
`owner_signed_cone` 22 avec exactement un propriétaire parmi deux candidats.

La porte permanente de domaine compare maintenant statut, support, arité, rang,
niveau et membres dans les quatre quadrants. Elle impose 11 sphères au
tétraèdre, 7 au triangle et ajoute un nuage coplanaire de cinq points; la vérité
de cardinalité 19 de ce dernier n'est toutefois pas gravée. La signature de
permutation owner transporte aussi membres et multiplicités, et le cône signé
ainsi que le cube multi-support sont devenus permanents.

Une dette mathématique subsiste dans cette nouvelle fixture. Sur le segment des
centres du cône signé, remplacer $\varepsilon=-1$ par $+1$ échange seulement le
propriétaire de $z=0$ vers $z=4$. Le catalogue reste identique et la porte passe.
Il faut comparer directement l'identité des deux sommets candidats ou tuer le
mutant du signe; l'égalité de sortie ne protège pas le théorème local.

La disparition de table est acquise uniquement dans le quadrant
owner+index+navigable. Sans index il reste $O(n)$ clefs singleton; sur la voie
directe, le repli peut rester en $\Theta(\text{sortie})$. Le sujet publie
maintenant le high-water du **nombre d'entrées** de `emitted`, relevé à chaque
insertion; ce n'est toujours ni le nombre d'octets, ni le RSS, ni la mémoire de
`kept`, `members_pool` et des sommets. Les scans owner live peuvent encore coûter
$\Theta(m^4+m^3\lvert B_U\rvert)$ par sommet. Un réducteur exact à six états est
désormais prouvé en $O(m+\lvert B_U\rvert)$ par support d'arité deux, mais il
reste à l'intégrer et le coût total du harvest d'arité trois reste ouvert.

### Le premier `.cu` est un microkernel de couples, pas encore une wavefront qualifiée

Le snapshot `04555bd` ajoute une cible CUDA optionnelle, un lanceur `.cu` et un
corps `host/device`. Sur CPU, ses quatre CTests passent; nombre de flats et
masque admissible concordent avec la référence pour les sommets `kOk`. Le commit
`78583f1` rapporte ensuite quatre lancements G4 `sm_120` sans écart sur le même
payload borné. Aucun stdout brut, binaire, PTX/cubin ou rapport `ptxas` n'est
versionné; ce diagnostic ne certifie ni l'identité ni l'ordre des flats quand le
masque est nul.

Sa portée est plus étroite que son nom : `navigate_shallow` matérialise d'abord
tous les sommets sur CPU, puis un thread calcule seulement le masque des couples
`(flat,direction)` admissibles. Le kernel n'appelle ni `neighbour_along`, ni
`decide_child`; il ne produit aucun voisin, parent, enfant, curseur, sous-arbre,
run ou transaction. C'est un microkernel borné concordant hôte/device sur les
entrées acceptées, pas encore le parcours.

#### Le refus n'est plus vacuable; le replay structurel reste à fermer

Une coquille cosphérique peut avoir $\Theta(n)$ points : aucune capacité fixe
n'est universellement suffisante, et prétendre le contraire serait faux. Le refus
est donc une pièce du contrat. Mais le contrat n'était pas tenu.

La boucle de référence exécutait `continue` sur chaque statut refusé, et
`summarise` comptait le refus puis écartait ses flats et son masque. Le plancher
`--min-refused` prouvait que la **branche** avait été prise, jamais que son
résultat avait été conservé — et les refus d'admission, dont `shell>32`,
n'entraient même pas dans le compteur. Conséquence directe : un mutant qui refuse
**tous** les sommets restait vert, avec zéro flat, zéro couple, zéro désaccord.

Le delta post-`8481b67` met en place quatre contrôles utiles :

1. **Des planchers séparés** : nuages décidés, sommets acceptés, flats, couples,
   noyaux lancés et rejeux. Un plancher agrégé se satisfait d'un seul chiffre.
2. **Un oracle non borné exécuté pour chaque statut**, refus d'admission compris.
3. **Le ledger $\text{refusés}=\text{rejoués}+\text{en attente}$**, publié par
   raison. Une raison sans rejeu est un trou, pas une statistique.
4. **Une identité de masse agrégée** entre les comptes du CPU complet et la
   somme des comptes dits committés/rejoués.

Le mutant vit maintenant **dans le binaire**, sous `--force-refuse-all`, et une
porte négative exige qu'il rougisse. Il est instructif : sous mutation le ledger
balance encore et la masse est encore conservée — rejouer tout est *correct*,
simplement inutile — et ce sont les planchers d'acceptation, de flats et de
couples qui le tuent. Aucune des deux moitiés ne suffisait seule.

**[audit]** Cela ferme la vacuité, pas encore le replay exact. L'oracle non borné
est pré-calculé pour chaque sommet avant l'admission; un refus ne rejoue ensuite
aucun objet et ne conserve aucun suffixe, il crédite seulement les deux compteurs
`reference[index].flats/couples`. L'identité ne compare donc ni séquence, ni
multiplicité, ni absence de perte/duplication compensée. Les noms
`committed_*`/`replayed_*` désignent ici des masses, pas un flux transactionnel.
La porte accepte en outre `pending>0` alors que ce champ signifie un trou, et ne
porte aucune identité du nombre de sommets; une sortie zéro-flat peut disparaître
sans changer les deux masses.

**[mesuré]** sur la campagne de refus `--clouds 4 --points 20 --coord 3
--smax 12 --seed 11` : 2 542 sommets présentés, 2 515 acceptés, 27 refusés pour
capacité de flats, 27 rejoués, zéro en attente. La masse committée vaut 15 346
flats et 7 109 couples, la masse rejouée 1 403 flats et 224 couples, et la
référence complète exactement 16 749 et 7 333. Les 27 refusés portent donc en
moyenne 52 flats, pas 32 : additionner les préfixes aurait sous-compté de 539.

**Ce que cela ne ferme pas.** Le refus est désormais jugé, mais le rejeu ne
transporte encore que deux masses scalaires : le suffixe au-delà du trente-
deuxième flat n'est comparé à rien d'indépendant, faute d'une seconde écriture
du chemin non borné. Ce qui est établi est donc l'égalité bit à bit du préfixe
committé, l'identité de masse de l'union, et rien de plus sur le suffixe. La
voie produit devra le matérialiser — pagination de la réduction parent, ou
rejeu du sommet entier avec multiplicité et clef exactes.

#### Compte et masque ne sont pas une signature

Un masque nul ne certifie pas l'ordre : sur six points cosphériques un sommet
porte vingt flats et un masque nul, et **toute** permutation des vingt conserve
`(flat_count, mask)`. Chaque verdict porte maintenant un digest ordonné de la
position, de la base, de la taille de fermeture et des bits d'admissibilité.
C'est un détecteur de régression positif, pas une signature exacte : les
identifiants de fermeture sont absents et FNV-1a 64 bits peut collisionner. Le
payload complet `(closure,base,slot,verdict)` doit encore être comparé avec sa
multiplicité, notamment pour le suffixe refusé.

#### Le lot est authentifié avant lancement

`WavefrontJob` transportait des pointeurs et des tailles bruts. Un job
`root_size = 0, root_base = nullptr` obtenait encore `kOk`, `point_count`
n'était jamais lu, et une entrée malformée devenait un accès hors limites
**device** au lieu d'un refus avant lancement. `validate_job` vérifie maintenant
les principaux champs structurels et rend une raison nommée : nuage non nul et dans la grille u16,
digest déclaré recalculé, base racine de taille quatre et affinement
indépendante, identifiants dans le nuage, coquille et intérieur strictement
triés et disjoints, $\ell(v)=\lvert B(v)\rvert$, capacités, et exactitude des
multiplications de tailles avant allocation. La fixture exerce un lot conforme
et dix formes malformées; plusieurs raisons de `JobVerdict` restent sans témoin.

Deux gardes restent à fermer. Le lanceur CUDA ne rappelle pas lui-même
`validate_job`, donc la sûreté dépend encore de son appelant et de l'absence de
mutation entre validation et copie. Et un `VertexVerdict::status` hors enum est
traité comme `kOk` par `summarise_into`; la sonde `status=777` est acceptée. Les
deux frontières doivent échouer fermées avant de parler de lot authentifié. Le
job ne porte pas non plus `smax` et ne peut donc vérifier que le plafond global
30, pas $\lvert B(v)\rvert\leq s_{\max}-2$ pour la campagne courante. Enfin un
nuage contenant une coordonnée dupliquée, avec digest recalculé, est déclaré
`valid` alors que le chemin produit refuse ce domaine : le digest authentifie
les octets, pas le profil. Cosphéricité de la coquille et classification exacte
de l'intérieur ne sont pas revérifiées non plus. Cette fonction valide la
structure d'un payload CPU déjà certifié; elle n'authentifie pas seule sa
géométrie.

Les queues des deux tableaux de `BoundedVertex` n'étaient jamais initialisées
avant la copie : la structure n'avait donc aucune représentation stable en
octets. Elles portent maintenant `-1`, qui n'est jamais un PointId, et deux
`static_assert` interdisent tout remplissage.

#### L'intérieur 31 et 32 était accepté alors que le contrat le rend impossible

`kMaxInterior` valait `kMaxRank = 32`. La coupe du parcours impose pourtant
$\lvert B(v)\rvert\leq s_{\max}-2\leq30$ : un intérieur de 31 ou 32 est une
**entrée malformée**, pas un dépassement de capacité rejouable. La capacité est
maintenant exactement la borne du contrat, et son dépassement porte le statut
`kInteriorAboveContract`. Symétriquement, `kClosureOverflow` est devenu
`kInvariantViolation` : toute fermeture est incluse dans une coquille déjà
bornée par l'admission, donc ce statut est impossible et il est traité comme
fatal au lieu d'être rejoué.

**[audit]** La qualification ne suit pas encore cette sémantique : elle classe
`kInteriorAboveContract` parmi les refus rejouables. Et `admit` teste d'abord la
coquille, si bien que `shell=33, interior=31` masque l'entrée malformée sous
`kShellOverflow`. La priorité des erreurs et le ledger doivent rendre ce cas
fatal.

Le cap de 32 flats est atteint bien avant le cap de coquille, et il a maintenant
sa fixture permanente. Sept points entiers cosphériques sans quadruplet
coplanaire — la sphère de centre `(100,100,100)` et de rayon 25 — portent 35
flats. Le noyau garde le préfixe 32 et le marque refusé; la porte vérifie son
masque `0x940800000009` et son digest incomplet contre le préfixe de référence,
puis crédite le compte total 35. Elle ne transporte pas encore les trois flats
du suffixe et ne peut donc pas certifier leur rejeu. À coquille 32, le
maximum générique est 4 960 : la voie produit devra paginer la réduction parent
ou rejouer le sommet entier, avec multiplicité et clef exactes.

Le verrou GPU principal reste de produire le minimum exact de
`neighbour_along` et **tout** son lot d'ex æquo, puis de décider le parent. La
[`note GPU`](audits/NOTE_VERROUS_MATHEMATIQUES_GPU.md) fournit une fixture
entière où un candidat plus lointain précède deux ex æquo minimaux, ainsi qu'une
partition de sous-arbres permettant de juger tâches, rollback et replay sans
construire de mosaïque d'ordre supérieur.

### Diagnostic G4 `sm_120` du microkernel borné

**[diagnostic déclaré, session G4 du 9 août 2026, RTX PRO 6000 Blackwell,
capacité 12.0]** Le même corps — `evaluate_vertex`, écrit une seule fois — a
été compilé sous le `nvcc` de `/usr/local/cuda-12.9` pour `sm_120-real` et
exécuté dans un kernel. La comparaison porte sur le `VertexVerdict` borné hôte
et device; elle n'appelle la référence non bornée que pour les sommets `kOk`.
Le dépôt ne contient ni stdout brut, ni version patch du toolkit, ni hash du
binaire, PTX/cubin, rapport `ptxas`, digest d'entrée ou répétitions :

| campagne | sommets | temps kernel | débit | désaccords |
| --- | ---: | ---: | ---: | ---: |
| 120 points, grille 4000, $s_{\max}=8$ | 128 955 | 0,224 ms | **575 M sommets/s** | **0** |
| 200 points, grille 8000, $s_{\max}=6$ | 71 084 | 0,170 ms | 417 M sommets/s | **0** |
| 24 points, grille 4000, $s_{\max}=8$ | 19 019 | 0,323 ms | 59 M sommets/s | **0** |
| 20 points, grille 3, $s_{\max}=12$ | 2 542 | 2,020 ms | 1,3 M sommets/s | **0** |

À quatre flats par sommet, la première ligne représente 1 031 640 appels
directionnels, soit environ **4,61 milliards d'appels par seconde**; les 340 781
résultats admissibles représentent 1,52 milliard par seconde. La dernière ligne
atteint 32 flats au maximum, pas en moyenne. Ses 27 `kFlatOverflow` sont comptés
mais **jamais rejoués** : la boucle oracle les saute. Son faible nombre de blocs,
la charge par sommet et la divergence sont confondus; le facteur 450 n'isole
aucune de ces causes.

**La cible primaire reste 100 ms pour toute la chaîne; moins d'une seconde est
une porte secondaire.** Cette session ne tranche aucune des deux. Les 1 096,8
sommets par point observés à `n=300` ne sont pas une borne inférieure à 50 k;
le profil cube concerné avait en outre une densité décroissante. Diviser
55 millions de sommets hypothétiques par 575 millions par seconde donne 95,7 ms,
mais seulement sous l'hypothèse que terrain, distribution de coquilles et débit
se transportent. Aucun parcours GPU complet ne mesure un facteur dix à trente
ni un retard de quinze fois.

**La source critique directe est donc une hypothèse prioritaire à tester, pas une
nécessité démontrée.** Le ratio 6,5 compare les sommets visités aux sommets bien
centrés d'arité quatre; à `n=300`, le rapport au catalogue complet publié est
environ 4,66 et les arités deux et trois dominent. Plus profondément, ce catalogue
est coupé par le rang fermé, tandis que la source Gabriel ouverte peut devoir
traiter un grand extra-shell. Une voie directe doit encore prouver la complétude
de son stream de supports, un census terminal et un coût output-sensitive; elle
ne se déduit pas de ce ratio.

Elle ne dit rien de plus que cela. Ce kernel n'exécute **pas** la descente :
`neighbour_along` n'est pas borné, l'index n'est pas porté, la sortie n'est pas
écrite, et rien n'est transféré au-delà du lot. Le terrain lui-même n'est pas
stabilisé — 772 → 999 → 1 097 sommets par point entre $n=100$ et $n=300$ — et une
extrapolation sur une croissance non stabilisée reste une extrapolation. **Le
NO-GO 50 k tient.** Exactitude owner/F0, replay, taille du terrain, source
critique, voisin terminal, parent, tâches et pipeline aval restent ouverts.

### Le profileur à densité fixe corrige le protocole, pas la borne 50 k

Le commit `f851374` remplace positivement le cube d'emprise `sqrt(n)` par un
cube de volume proportionnel à `n`, et fournit aussi une nappe synthétique
d'épaisseur bornée. En Release, la commande
`mhgp3v_scale_profile --points 100 --smax 11 --repeats 2 --seed 20260809`
reproduit 805,5 sommets par point, 159,28 sphères par point et les arités
`1,00/20,11/77,36/60,80`.

Ce point ne décide pas seul les 100 ms. La densité est codée en dur, la nappe
n'est pas un nuage LiDAR enregistré, les statuts non `kOk` sont retirés de la
moyenne, la déduplication du générateur est quadratique hors chrono, et les temps
n'incluent ni tout le pipeline ni ses octets. La porte d'échelle doit publier
toutes les graines et tous les statuts, des quantiles par famille sanctionnée,
les temps et high-waters par étage, puis un vrai point 50 k ou une borne prouvée.

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
GPU effectif, exécutant autre chose qu'un test CPU, pourra ensuite en établir le
débit sur la G4.

**Ce que cela ne ferme pas**, et la note le dit avant moi : les **enfants**
exigent toujours tous les flats incidents réels, une grande coquille peut en
avoir un nombre combinatoire, et le parent local ne borne aucun temps. Le
nouvel endpoint a bien substitué la reverse search au BFS pour le parcours
différentiel, mais pas pour le catalogue; son enveloppe d'oracle matérialise
encore la sortie. Le
delta live confronte aussi l'API indexée au balayage avec des feuilles de quatre
et une porte permanente à vingt points. Il prouve la traversée de nœuds internes;
la preuve d'élagage reste celle de l'auto-test synthétique, pas une métrique propre
aux requêtes de la reverse search. Le juge confronte les coquilles, les intérieurs
et les potentiels sur son domaine borné, sans constituer encore un reçu de
premier événement orienté ni une mesure de mémoire. Même avec un
sink, resteront la source complète des incidences silencieuses, le tri et les
lots exacts, la partition horizontale, `coverage_log` et la jointure verticale.
Leur factorisation est dans
[`NOTE_GATE_D_GLOBALITES_RESIDUELLES.md`](audits/NOTE_GATE_D_GLOBALITES_RESIDUELLES.md).

---

## 4 quater. La première incidence est décidée; sa source produit reste ouverte

[`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](audits/NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md)
retire le verrou mathématique de la décision locale, pas celui de sa production
terminale. Pour une facette $F$ du cœur, de miniboule fermée $B_F$ et de niveau
$b_F$, en posant $E_F=(B_F\cap X)\setminus F$, la première incidence se décide
par une requête `closed_ball` complète ou par le minimum d'une source directe;
aucune étoile de voisinage n'est nécessaire :

- **branche fermée**, $E_F\neq\varnothing$ : alors $\lambda(F)=b_F$ et $M(F)=\lbrace F\cup\lbrace x\rbrace:x\in E_F\rbrace$. La preuve tient en deux lignes et n'exige **aucune** hypothèse de régularité ;
- **branche vide**, $E_F=\varnothing$ : si la source contient toutes les cofaces de Gabriel ouvertes et développe ou refuse explicitement les égalités extérieures, alors $\lambda(F)$ est le minimum de leurs niveaux parmi celles qui contiennent $F$, et $M(F)$ en est le groupe d'ex æquo — tout minimiseur est de Gabriel au sens ouvert, sinon un intrus strict fournirait une incidence strictement moins chère.

### Ma première source était fausse, et le juge ne pouvait pas le voir

La première version filtrait les cofaces de rang fermé $k+1$, c'est-à-dire la
vacuité **fermée**, là où le théorème exige la vacuité **intérieure** avec une
politique explicite pour les points extérieurs exactement sur la coquille. Sur
cinq points

```text
(0,0,0) (0,2,2) (2,0,2) (2,2,0) (0,0,2)
```

les quatre premiers forment un tétraèdre régulier et le cinquième est sur sa
sphère : la vérité Gabriel ouverte compte **cinq** cofaces de taille quatre, la
fermée **une**. J'en gardais une, j'en omettais quatre — et j'affichais zéro
désaccord, parce que l'univers des facettes jugées était dérivé de ma propre
source. Une coface omise faisait disparaître aussi la facette qui l'aurait
révélée. **Le juge était circulaire.**

Deux corrections, et la seconde compte plus que la première.

Dans le falsificateur borné, la source **développe** les extra-shells. Toute coface de Gabriel ouverte $Q$ de
cardinal $k+1$ a pour miniboule une sphère critique $B$, avec
$I(B)\subseteq Q\subseteq I(B)\cup S(B)$ ; donc $Q=I\cup T$ pour un
$T\subseteq S$ de cardinal $k+1-\lvert I\rvert$. Énumérer les sphères critiques
puis ces $T$ est complet relativement à un catalogue critique complet. Le
prototype obtient cette prémisse par `flat_catalogue(pts,n)` : il matérialise le
catalogue entier et développe combinatoirement les coquilles. C'est une bonne
référence bornée, pas l'architecture produit 50 k.

La vérité **énumère son propre univers vis-à-vis de la source** : toutes les cofaces de cardinal $k+1$ à
vacuité ouverte, puis ses propres facettes. Les deux univers sont comparés
**avant** $\lambda$ et $M$, donc une coface omise est désormais un désaccord. Le
niveau $\lambda(F)$ est comparé lui aussi, pas seulement l'ensemble $M(F)$. Le
parseur lit les entiers en entier et un statut non `kOk` fait échouer au lieu de
censurer. Cette énumération partage encore `miniball_of`, `sphere_side` et
`sphere_cmp_beta` avec le sujet : elle est indépendante de la source, pas des
primitives arithmétiques. Les planchers couvrent les deux branches. Le compteur
dit « nœuds internes » mesure actuellement les nœuds construits par l'index, pas
ceux visités par les requêtes.

**[mesuré sur `dcd19178`]** trois régimes, coordonnées distinctes, tout jugé
contre l'univers énuméré séparément mais relativement aux primitives partagées :

| régime | $n$ | grille | $k$ | cofaces (manq./surn.) | facettes (manq./surn.) | branche fermée | co-min. moy./max | désaccords |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| cube cosphérique | 8 | $[0,2)$ | 3 | 2 800 (0/0) | 2 240 (0/0) | **100 %** | 2,71 / 5 | **0** |
| nœuds internes | 20 | $[0,20)$ | 3 | 2 496 (0/0) | 5 103 (0/0) | 62,4 % | 1,05 / 4 | **0** |
| ordre plus haut | 9 | $[0,3)$ | 4 | 593 (0/0) | 1 605 (0/0) | 81,1 % | 1,75 / 5 | **0** |

Les commandes utilisent respectivement 40, 30 et 25 nuages, avec les seeds 7,
11 et 13. La grille de côté deux exerce un régime cosphérique contenant des
extra-shells et une branche fermée totale; aucun compteur ne prouve
« extra-shell sur chaque facette ». Le run à vingt points construit 210 nœuds
internes et touche 9,3 points par facette contre $n=20$, ce qui montre au moins
un élagage sans mesurer la visite des nœuds. Les deux multiplicités de provenance
sont comptées séparément; aucune déduplication terminale ni aucun plancher ne les
certifie encore.

Et la reproduction hostile de l'audit,
`--clouds 1 --points 7 --coord 2 --k 2 --seed 1`, qui rendait six désaccords,
rend maintenant 31 cofaces et 21 facettes sans manquante ni surnuméraire.

### Une attache par facette, et la cible brute est réfutée en pratique aussi

[`NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`](audits/NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md)
montre que sous la porte régulière il n'est pas nécessaire de publier tous les
$M(F)$ : une **unique attache canonique** par facette ayant au moins deux intrus
**stricts** suffit à la forêt $H_0$ normalisée. Les intrus stricts $J_F$ sont la
boule **ouverte**, à ne pas confondre avec $E_F$ qui décide la branche. Avec
$z_F=\min J_F$ et $u_F=\min U_F$, la cible locale est
$T_F=(F\setminus\lbrace u_F\rbrace)\cup\lbrace z_F\rbrace$, et le lemme donne
$\beta(T_F)<a_F$.

La note réfute la cible **brute** : $T_F$ peut ne pas appartenir à $D_k$, et il
faut alors viser le carrier strict **résolu**. Sa fixture u16 renforcée possède
dix points et $F=289$, avec $J_F=\lbrace1,5,7\rbrace$. Pour $z_F=1$, chacun des
trois bras obtenus en retirant 2, 8 ou 9 a un niveau strictement plus petit mais
reste hors de $D_3$. Aucun choix local de $u_F$ ne rend donc la cible immédiate
valide : il faut la descente vers le cœur, puis le `find` pré-lot.

**[diagnostic hors autorité régulière]** 30 nuages de 20 points, grille $[0,20)$, $k=3$ : sur 5 103 facettes,
les intrus stricts se répartissent en 1 997 / 3 011 / **95** pour zéro, un et au
moins deux. Ce sont **95 candidats d'attache**; le run observe 210 co-minimiseurs
fermés sur ces facettes, aucune cible de niveau non strict et **6 cibles brutes
hors du cœur**. Comme il n'authentifie ni support unique essentiel ni absence
d'extra-shell, il ne peut pas encore conclure que 95 attaches remplacent ces 210
co-minimiseurs. Il confirme néanmoins que la cible brute hors cœur n'est pas un
cas limite.
La campagne compte 3 184 branches fermées mais seulement 3 106 facettes ayant
au moins un intrus strict : au moins 78 facettes portent donc une égalité
extérieure sans intrus. Le domaine régulier est effectivement violé, et non
simplement non vérifié.
Le CTest du snapshot `2c395d3` utilise 12 nuages : il exerce 39 candidats, porte 88
co-minimiseurs fermés et observe 5 cibles brutes hors du cœur. Son plancher
prouve l'exercice de la branche, pas l'autorité de l'attache.

### La porte locale est beaucoup plus forte; l'autorité globale reste ouverte

Le delta live épinglé par
`first_incidence_dichotomy.cpp=b0741d4edcc9839ad4ab12bb58867b8c125fc83f9ab127708dcd15a91e640c17`
ferme les deux réfutations précédentes. Il compte toute égalité extérieure,
intrus strict présent ou non; cherche les supports essentiels tous cardinaux
confondus; refuse un membre de coquille hors support; et réauthentifie chaque
facette de la descente. Les fixtures permanentes couvrent désormais l'égalité
mixte, les supports 2 contre 3, `E5` et le lookup brut hors cœur à sept points.

Le CTest à quatorze points observe 26 descentes, quatre pas, une longueur maximale
de deux, neuf terminaux sans intrus, dix-sept à un intrus et 26 reçus directs
sous le cutoff, sans désaccord. Des planchers exigent les deux branches et au
moins un pas. C'est une correction constructive importante.

Le fail-closed local est maintenant nettement plus fort. Une
panne de miniboule sur un sous-ensemble du contrôle de support est fail-closed —
un `continue` pouvait déclarer unique un support qui ne l'est pas ; une panne
pendant la descente est un désaccord ; le plancher ne **soustrait** plus les
refus ; et `--require-regular` exige zéro refus et zéro panne de descendant,
puisqu'une campagne qui se déclare régulière ne peut pas contenir un descendant où
le théorème n'est pas démontré. Le type de reçu porte le terminal, la longueur de
la chaîne, la branche, l'identité de la coface directe engagée et son niveau; les
fixtures lui passent effectivement une sortie et `E5` exige la coface
$\lbrace2,3,4\rbrace$ de niveau $162/25$. Les campagnes ordinaires appellent
encore la descente sans sortie de reçu et n'en conservent que les agrégats.

Deux limites empêchent encore d'appeler `--require-regular` une autorité de
quotient.

1. La porte globale du théorème d'attache concerne aussi les objets silencieux
   omis du quotient. Le binaire contrôle les facettes cœur et les chaînes
   choisies, pas tout ce plateau. Sa fixture u16 à dix points vérifie toujours le
   cœur et les trois bras, pas seule les 120 triplets et 210 quadruplets annoncés.
2. Le reçu est construit depuis `truth_direct`, carte exhaustive globale de la
   vérité : il **juge** le théorème, il ne l'implémente pas. Le dernier
   $\mathrm{find}_{<a_F}(R_F)$, la partition pré-lot et l'équivalence des
   quotients ne sont ni implémentés ni jugés.

La
[`note de descente locale`](audits/NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md)
ferme donc la partie géométrique sous ses hypothèses; ce prototype en est un bon
falsificateur borné, pas encore le resolver ni le fold produit.

La
[`note de source depuis les sphères certifiées`](audits/NOTE_GATE_D_SOURCE_DIRECTE_DEPUIS_SPHERES_CERTIFIEES.md)
ferme aussi une partie amont : toute sphère directe d'ordre $k$ possède un
propriétaire rencontré au niveau strict au plus $k-1$, puis son census $I,S$
détermine exactement ses cofaces. Sous zéro-extra-shell, un argument
d'échantillonnage borne même, pour une coquille propriétaire de taille $m$, les
sources possédées de rang deux et trois par $O(mK)$ et $O(mK^2)$. Une famille
universelle explicite donne maintenant un générateur déterministe complet de
candidates par coques échantillonnées, sans énumérer toutes les suppressions;
un halfspace-reporting déterministe filtre aussi leur profondeur locale. Ce
générateur n'est ni implémenté ni jugé. Le census global de chaque boule contre
$X$, la propriété et la déduplication inter-sommets restent sans borne produit;
leur recertification exacte en batch est le verrou suivant. Récolter seulement
les directions du parent est faux.

Deux mises en garde qui subsistent : les co-minimiseurs observés sont petits mais
**sans borne générale** — une facette peut en avoir $\Theta(n)$ — et le rapport de
$k+1$ records par coface est une identité de construction du flux, qui le
dimensionne sans certifier la terminalité de la source.

**Ce que cela ne ferme pas.** Il faut d'abord authentifier séparément la
source directe ouverte, l'univers de facettes, les extra-shells, les statuts et
les budgets. Le balayage exhaustif local partage encore `miniball_of` et
`sphere_cmp_beta` avec le sujet; l'oracle général doit encore être appelé comme
juge hostile et repli hors porte. Le regroupement est en mémoire : le prototype
publie des comptes logiques, sans tri externe, wire exact, budget ni high-water.
Enfin, la dichotomie produit $M(F)$ sous ses prémisses; elle
ne produit ni l'autorité de régularité qui autorise la rétraction vers la forêt
$H_0$ normalisée, ni le réducteur, ni les verticales, ni l'identité de sortie.

---

## 4 quinquies. La source directe donne un accord relatif borné et déplace le verrou

La partie candidate est la première voie de ce dépôt qui produit les records du
catalogue **sans énumérer de sommet d'arrangement**. Elle énumère directement
les supports, chacun exactement une fois sur les campagnes reçues, depuis son
ancre $p=\min U$, dans un voisinage dont la complétude est prouvée sous les
préconditions du cover. Le mode jugé appelle encore `flat_catalogue`; l'absence
d'arrangement décrit donc la partie candidate, pas l'exécutable entier.

### Le lemme de rayon, et l'ordre non circulaire qui le rend utilisable

Pour $q\in\{2,3,4\}$ poser $t_q=s_{\max}-q+1$. La boîte du nuage est subdivisée
en feuilles à bornes entières; chaque feuille authentifie $t_q$ PointId distincts
dont la distance carrée au coin le **plus éloigné** de la feuille est strictement
inférieure à $Q_q$. Si une miniboule propre de support $q$ vérifie
$q+\lvert I\rvert\le s_{\max}$, son centre est dans $\mathrm{conv}(U)$ donc dans
une feuille, et $\beta\ge Q_q$ rendrait les $t_q$ témoins tous strictement
intérieurs, donc $q+\lvert I\rvert\ge s_{\max}+1$. Donc $\beta<Q_q$, donc tout
membre de la boule fermée est à distance carrée $<4Q_q$ de $p$.

L'ordre compte, et c'est le point délicat : le lemme conclut **sous** l'hypothèse
$q+\lvert I\rvert\le s_{\max}$, or c'est $\lvert I\rvert$ que le census doit
calculer. La banque de témoins brise le cercle dans le bon sens — tous les
témoins strictement intérieurs donnent `AboveInteriorWindow` et un refus **sans
census**; un seul témoin non intérieur prouve $\beta<Q_q$ **avant** le census,
qui devient alors global et complet dans $N_q(p)$.

### Accord relatif complet avec le catalogue fermé, cosphéricités comprises

Le mot « certifiée » n'est pas employé : ce prototype partage avec la référence
`mhgp::sphere_*` et `mhgp::miniball_of`, donc ce qu'il établit est un **accord
relatif** à ces primitives sur les campagnes exercées.

**[mesuré]** clef par coquille : même ensemble de sphères, même support
canonique, même rang, même niveau rationnel exact et **même liste triée de
membres par sphère**. Ce test n'établit pas l'identité du pool global concaténé.

| campagne | nuages | sphères | candidats | émissions | doublons | désaccords |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| générique `40 pts / 160³ / s_max=8` | 6 | 12 124 | 612 300 | 12 124 | 0 | **0** |
| grille saturée `12 pts / 5³ / s_max=8` | 12 | 2 072 | 9 372 | 2 072 | 0 | **0** |
| petit nuage `5 pts / s_max=32` | 6 | 30+ | — | = | 0 | **0** |

La grille saturée est la campagne qui compte : c'est là que les cosphéricités
donnent plusieurs supports minimaux pour une même boule, et la source n'émet
qu'une fois parce qu'elle applique la **même** convention canonique que la
référence — coquille triée par coordonnées, puis `miniball_of`, puis rejet de
tout candidat qui n'est pas ce support.

### Quatre défauts que l'audit a trouvés, et qui étaient tous le même défaut

[`AUDIT_SOURCE_DIRECTE_24AD3D37.md`](audits/AUDIT_SOURCE_DIRECTE_24AD3D37.md) a
réfuté le premier palier sur quatre points, et ils avaient tous la même forme :
le programme **affirmait plus que ce qu'il vérifiait**.

`--judge 0` imprimait quand même « rend exactement le catalogue fermé ». Une
exactitude annoncée en l'absence d'oracle est pire qu'un silence. Les trois
modes sont maintenant exclusifs — `--cover-only 1 --judge 1` est **refusé** —, et
seul le mode jugé a le droit de conclure; les deux autres impriment
`AUCUNE EXACTITUDE N'EST AFFIRMÉE`.

La map de sortie était indexée par la coquille et l'affectation **écrasait** les
doublons. Un mutant retirant la restriction $z>p$ émettait 126 fois au lieu de
56 sans qu'aucun compteur ne bouge, et le différentiel restait vert. Les
émissions sont maintenant comptées à part de la taille de la map; le mutant vit
dans le binaire sous `--force-both-directions` et une porte négative exige qu'il
rougisse — il rend 651 doublons et sort 3.

Le payload jetait `members` après en avoir pris la taille : deux sorties de même
rang et d'intérieurs différents étaient indiscernables. Les listes ordonnées
complètes de membres sont maintenant construites **et** comparées par coquille.
En mode forêt, un pool global source est aussi assemblé, mais son ordre et ses
offsets ne sont pas égaux au payload canonique de la référence.

`n<t_q` et le plafond de cellules sortaient sur un message générique. Ce sont
maintenant des statuts typés — `petit_nuage_direct` et `plafond_cellules` — avec
repli racine réellement appliqué, et le petit nuage a sa porte permanente.

Deux bornes annoncées étaient fausses. La dernière cellule nominale dépasse le
maximum du nuage, donc la distance brute au coin n'est pas bornée par
$3\cdot65535^2$ mais par $3(2\cdot65535)^2<2^{36}$. Et `bound_t` en `u64` déborde
silencieusement dès $n\approx13\,500$ : les compteurs de preuve sont passés en
128 bits non signés. Enfin un clamp du locator est devenu une **violation
d'invariant** : sur un support bien centré le centre est dans l'enveloppe convexe
du support, donc dans la boîte, et clamper masquerait une faute.

Le réaudit suivant a trouvé trois défauts de plus, tous fermés. Les ordres bas
étaient un **domaine contradictoire** : la CLI acceptait $s_{\max}=2$ puis la lane
$q=3$ échouait sur $t_q<1$ avec un message générique. Une lane d'arité
$q>s_{\max}$ est vide **par contrat** — un support d'arité $q$ a un rang au moins
$q$ — et c'est maintenant ce qu'elle rend. `s_max=2` a sa porte; `s_max=3`
passe manuellement mais n'a pas encore de CTest dédié.
Une forêt d'ordre $k$ lit les rangs $k$ et $k+1$, donc $K+1\le s_{\max}$ est
exigé au lieu de qualifier des ordres tronqués. Et les portes positives passaient
`--judge 1` **implicitement** : elles l'exigent maintenant, sinon une mutation du
défaut CLI les ferait passer en mode mesure avec `reference=0`.

L'empreinte de forêt omettait aussi les nœuds **inaccessibles**, les liens
`parent` et `n_children` — une signature qui part des racines ne voit jamais un
nœud que personne n'atteint. Un parcours séparé compte maintenant ces anomalies
et son résultat entre dans l'empreinte. Ce n'est pas encore un validateur total :
un auto-cycle `next_sibling` ne termine pas, et un `first_child` hors plage
déclenche un heap-buffer-overflow sous ASan. Des fautes identiques des deux côtés
restent en outre compatibles avec un digest égal. La forêt valide de la gate est
créditée; toute promesse fail-closed sur une forêt malformée reste ouverte.

Trois réserves de mutation-résistance sont fermées. L'identité
$\text{candidats}=C_q$ est maintenant **exigée** par lane, pas observée : une
mutation qui sauterait ou dupliquerait seulement des candidats non émissibles
conserverait la sortie, les planchers et le différentiel, et ce compteur est le
seul témoin. L'unicité de la coquille côté **autorité** est assertée au lieu
d'être supposée. Et un mutant `--force-drop-member` retire un membre de chaque
sphère émise : sans lui, rien ne prouvait que le comparateur de listes compare
autre chose que des listes vides — il rend 412 désaccords et sa porte négative
l'exige.

Enfin la revendication est ramenée à ce qui est vérifié : **mêmes listes de
membres par coquille**, pas « mêmes pools ». L'ordre global du catalogue, les
offsets publics et les indices `ForestNode::source` diffèrent de la référence, et
l'empreinte de forêt est explicitement un **quotient sémantique** invariant à la
renumérotation — pas une égalité de sérialisation. Le chrono étiqueté source
couvre maintenant l'assemblage et les deux folds, source **et référence**, ainsi
que leurs empreintes; il ne sépare donc toujours pas ces coûts.

### Un quotient sémantique de la forêt est produit et comparé

Le catalogue n'est que la moitié du contrat : ce que le projet doit produire est
la **forêt des arbres de niveaux de densité**, pour $k=1..K$. Le prototype la
construit maintenant depuis les deux catalogues avec le même `build_forest` et
compare un quotient invariant à la renumérotation. C'est une intégration
positive, pas un oracle indépendant de la correction mathématique du fold.

La comparaison ne porte surtout pas sur des indices. `ForestNode::source` d'une
multifusion est, par contrat, « la plus petite **par index** des sphères de rang
$k+1$ du lot » : cet indice dépend du générateur, et
[`AUDIT_CONTRAT_CATALOGUE_FORET_ORDER_K_CF9374`](audits/AUDIT_CONTRAT_CATALOGUE_FORET_ORDER_K_CF9374.md)
l'avait déjà dit. La première version de cette signature a d'ailleurs signalé une
« divergence » qui n'en était pas une : deux forêts identiques nommant un
représentant différent pour le même événement de fusion.

L'empreinte récursive du quotient identifie une **naissance** par
l'ensemble de membres de son minimum de rang $k$, une **multifusion** par le
**rang exact** de son niveau dans l'ordre `sphere_cmp_beta` — pas par un double,
pas par un indice —, et un nœud par le multiensemble trié des signatures de ses
enfants. Elle est pertinente après accord exact des records et sous précondition
de forêts valides. Les nouveaux compteurs observent accessibilité, parents et
nombre d'enfants, mais ne valident pas totalement la structure; l'empreinte
n'authentifie toujours pas `beta` ni le représentant public `source` des
multifusions.

**[mesuré]** `--clouds 6 --points 14 --coord 12 --smax 6 --forest 5` : 30 forêts
comparées, 1 647 nœuds, 32 racines, **zéro empreinte quotient différente**. Les
cumulés $K=1..5$ valent 154, 419, 774, 1 201 et 1 647 nœuds : chaque ordre de la
gate contribue réellement.

La gate ne reçoit toutefois que la masse agrégée. Un mutant temporaire qui
saute entièrement l'ordre maximal ne compare plus que 24 forêts, conserve les
1 201 nœuds des ordres un à quatre, annonce encore `k=1..5` et laisse les treize
CTests directs verts. Il faut exiger le nombre exact de forêts et la contribution
de chaque ordre.

Une sonde indépendante trouve toutefois, sur 24 nuages, 3 062 positions de
catalogue déplacées et 4 016 champs publics `ForestNode::source` différents,
malgré 120/120 empreintes quotient égales. La source itère une map triée par
coquille, tandis que le contrat public trie les quatre slots du support. Le
résultat prouve donc l'équivalence abstraite modulo renumérotation, pas
l'identité du `Catalogue`, du pool, des offsets, de la `Forest` publique ou de
leur sérialisation. Il faut trier le catalogue source selon le support canonique
et reconstruire le pool avant de revendiquer la chaîne publique bout en bout.

La qualification exige en outre $K+1\le s_{\max}$. Le palier `1c3948c3...`
acceptait pourtant `--smax 4 --forest 5` et même `--forest 32`, puis annonçait
un accord complet : les ordres hauts étaient vides et l'ordre quatre tronqué
faute de rang cinq. `81f9210` refuse maintenant ces commandes et possède une
porte négative; cette dette de domaine est fermée.

### Le verrou candidat et le verrou de payload sont maintenant séparés

Sur le domaine effectivement jugé, la complétude des records par coquille n'a
produit aucun écart. Restent distincts la complétude bas ordre, le payload public,
la source Gabriel ouverte et le nombre de **candidats**; ce dernier se mesure.

**[mesuré, densité fixe $10^{-3}$, $s_{\max}=11$, côté de feuille 8]**

| $n$ | emprise | $Q_4$ | rayon $2\sqrt{Q_4}$ | degré moyen | degré max |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 400 | $74^3$ | 1 198 | 69,2 | 353,7 | 399 |
| 1 600 | $117^3$ | 1 231 | 70,2 | 674,0 | 1 363 |

$Q_q$ vaut 1 198 puis 1 231 sur ces deux tailles. Cette stabilité finie est
compatible avec un rayon local à densité fixe, mais deux points ne prouvent ni
convergence ni asymptote. Le scénario extrapolé emploie la constante
$\rho\cdot\tfrac43\pi(2\sqrt{Q})^3\approx1\,450$, et non les 8 à 24 que la note
GPU emploie dans ses plafonds. Les degrés mesurés 354 et 674 sont encore sous
l'asymptote uniquement parce que le rayon 70 déborde la boîte.

Avec $d^{+}\approx d/2$, l'énumération combinadique donne alors, à 50 000 points :

$$C_2\approx3{,}6\cdot10^{7},\qquad C_3\approx1{,}3\cdot10^{10},\qquad C_4\approx3{,}2\cdot10^{12}.$$

Le budget primaire est 100 ms. Le balayage aveugle des $(q-1)$-sous-ensembles du
voisinage est quartique en degré et le scénario à 50 000 points donne
$3{,}2\cdot10^{12}$ candidats. C'est une obstruction de dimensionnement forte,
mais encore une extrapolation conditionnelle à la densité, au cover et au degré,
pas un reçu chronométrique 50 k ni une preuve d'impossibilité de toute variante.

Un gain réel existe et il est chiffré. $Q_q$ est le **maximum** sur les feuilles :
il est fixé par la région la plus vide du nuage, pas par la région typique. Les
$Q$ **effectifs**, sur toutes les feuilles de tous les nuages, valent

| $n$ | feuilles | min | médiane | max | rayon min | rayon médian | rayon max |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 400 | 1 000 | 202 | 406 | 1 198 | 28,4 | 40,3 | 69,2 |
| 1 600 | 3 375 | 194 | 370 | 1 231 | 27,9 | 38,5 | 70,2 |

Le lemme de rayon vaut **feuille par feuille** : un cover adaptatif diviserait le
rayon typique par 1,8, donc le degré par environ six, donc $C_4$ par $6^3$. Cela
ramènerait $C_4$ à $1{,}4\cdot10^{10}$ — un facteur 230 gagné, et toujours deux
ordres de grandeur au-dessus du budget. L'adaptativité est nécessaire; elle n'est
pas suffisante.

### Les supports sont des cliques, et le lemme de triple est exact

J'ai posé la question à l'auditeur, puis je l'ai mesurée moi-même.

Le lemme s'applique à **chaque** paire d'une coquille critique. Un support d'arité
trois a donc ses trois paires admissibles, un support d'arité quatre ses six :

$$\text{support d'arité }3=\text{TRIANGLE de }G,\qquad \text{support d'arité }4=\text{K4 de }G.$$

C'est une réduction structurelle exacte, pas une heuristique. Et il existe en plus
un **lemme de triple**, plus fort que celui de paire parce que le plan frontière
n'est plus libre. Pour un triple $T$ non aligné de plan $\pi$, de circumcentre $m$
— qui est la projection orthogonale du centre critique $c_0$ sur $\pi$ — et de
circumrayon $\rho$, en coordonnées $\pi=\{y_3=0\}$, $m=0$, $c_0=(0,0,d)$ :

$$\lVert y-c_0\rVert^2=\lVert y\rVert^2-2y_3d+d^2\leq\rho^2+d^2=R^2\quad\text{dès que } y\in D_T \text{ et } y_3\geq0 .$$

Donc $A_3(T)=\min\bigl(\lvert X\cap D_T\cap H^{+}\rvert,\lvert X\cap D_T\cap H^{-}\rvert\bigr)\leq s_{\max}$,
et il n'y a que **deux** demi-espaces à tester au lieu d'une famille continue.
Aucune hypothèse de bon centrage : $m$ est le circumcentre, pas la miniboule.

**[mesuré, densité fixe $10^{-3}$, $s_{\max}=11$, deux nuages par ligne]** zéro
triple vrai réfuté — le lemme est nécessaire, vérifié. Le compteur de quadruples
réfutés est aussi à zéro, mais c'est **impliqué** : les quatre faces d'un vrai
quadruple sont quatre vrais triples, que le lemme conserve tous. C'est une garde
de cohérence, pas un témoin indépendant, et le code le dit.

| $n$ | degré de $G$ | triangles/pt | K4/pt | facteur brut | une face | **quatre faces** |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 50 | 47,6 | 359,6 | 3 888,7 | 99,8 | 81,6 | **53,1** |
| 100 | 80,8 | 913,7 | 13 444,9 | 221,1 | 144,4 | **69,5** |
| 200 | 113,1 | 1 500,5 | 23 773,0 | 266,7 | 162,2 | **69,3** |

Le lemme de triple retient 82 %, 63 % puis 57 % des triangles.

**La colonne qui compte est la dernière, et elle répond à la question 2 de
l'audit.** Ma première version n'appliquait le lemme qu'à **une** face de chaque
K4 — le triple des trois plus petits identifiants — et publiait 82, 144, 162,
donc une hausse. Les quatre faces d'un support d'arité quatre sont toutes sur sa
coquille, donc toutes nécessaires : les tester donne 53,1, 69,5, 69,3. Le gain
est de 2,3 fois, et la hausse **disparaît** entre $n=100$ et $n=200$.

Trois tailles ne prouvent pas plus un plateau qu'elles ne prouvaient une hausse.
La formulation recevable reste : *sur ces campagnes bornées, cette variante par
cliques reste très surproductive — environ soixante-dix fois — et ne ferme pas le
budget.* Le travail évité quand un triple échoue est maintenant réel : le
développement du K4 s'arrête à la première face refusée.

### Ce que ces diagnostics permettent réellement de comparer

| générateur | ratio diagnostique publié | tendance observée |
| --- | ---: | --- |
| sous-ensembles du voisinage du cover | $C_4\approx3{,}2\cdot10^{12}$ à 50 k | catastrophique |
| cliques du graphe admissible, quatre faces | 53,1 → 69,5 → 69,3 | environ 70 sur les deux dernières tailles; aucune tendance asymptotique reçue |
| **parcours de l'arrangement** | environ 4,7 à 6,5 | à peu près stable |

Sur ces campagnes, le ratio publié du parcours est beaucoup plus petit. Cela
ne suffit pas à le déclarer meilleur générateur : le ratio K4 divise des K4
candidats par les seules sphères de support canonique quatre, tandis que le ratio
du parcours divise des sommets par toutes les sphères critiques. Le coût d'une
unité diffère aussi. Son débit absolu reste de toute façon ouvert — le scénario
vaut $V\approx5{,}5\cdot10^{7}$ sommets à 50 k pour un budget de 100 ms.

C'est une alerte de direction utile : les deux voies directes essayées restent
très surproductives sur les campagnes reçues. Même la division directe de 53--70 par
4,7--6,5 ne prouve toutefois pas un ralentissement de vingt-cinq à trente-cinq,
car unités et dénominateurs diffèrent. Une décision d'architecture exige le même
payload, les mêmes nuages, des unités de travail communes et les chronos/high-
waters complets.

**[audit `ee5ee51`, réponse intégrée à `81f9210`]** Le comptage 28/56/70 sur le
graphe complet et le lemme de triple sont crédités. Les cinq réserves de forme
sont closes : un mutant `--force-triple-accept` vit dans le binaire et une porte
négative exige qu'il rougisse — c'est le **plancher de triples rejetés** qui le
tue, pas `triple_missing == 0`, qu'un filtre acceptant tout satisfait ; quatre
planchers reçoivent triangles, K4, rejets et supports d'arité quatre vrais ; le
vrai degré **non orienté** est publié à côté de $d^{+}$, qu'une étoile centrée
sur le dernier identifiant aurait fait afficher à 1 ; un dénominateur nul imprime
`N/A` et non `0.0`, parce que zéro y signifie « quotient non défini » et non
« aucun gaspillage » ; et le chrono composite est nommé
`cliques+verite+lemme`.

Ces corrections de forme sont réelles, mais la réception reste incomplète.
Aucune gate ne reçoit le nombre de K4 survivant aux **quatre** faces ni le gain
sur une seule face. Surtout, `truth_triples` et `truth_quads` ne sont consultés
que pour les candidats visités : un itérateur qui omet une partie des vraies
cliques tout en dépassant les planchers peut encore rendre zéro faux rejet.
Deux mutants temporaires gardent effectivement les deux CTests verts : l'un
remplace les trois faces supplémentaires par `true`; l'autre omet 202 vrais
triples et 97 vrais quadruples dont l'ancre vaut zéro. Comparer explicitement
les ensembles visités à la vérité est la prochaine porte.
Le détail reproductible est dans
[`AUDIT_CLIQUES_ET_TRIPLE_EE5EE51.md`](audits/AUDIT_CLIQUES_ET_TRIPLE_EE5EE51.md).

Une faute d'affichage a été trouvée en la corrigeant : le formateur de ratio
employait un `static char[]`, si bien que les deux `%s` d'un même `printf`
pointaient sur la dernière valeur écrite et affichaient deux facteurs
**identiques**. C'est ce qui m'a d'abord fait lire « quatre faces = 81,6 ».

Ce n'est pas un argument pour abandonner la source directe : son cover et sa
fenêtre restent un mécanisme de complétude **sans arrangement** du dépôt, et un
quotient sémantique de forêt en sort sans écart sur les petits runs. Ce n'est pas
non plus une preuve qu'aucun générateur direct peut battre le parcours. Le débit
du voisin terminal, la partition en tâches et la résidence de la sortie restent
des verrous utiles indépendamment de cette comparaison.

La structure output-sensitive qui manque à la voie directe est d'ailleurs
exactement celle du parcours. Fixons un triple admissible $T$ : les sphères
d'arité quatre le contenant ont leur centre sur **l'axe du pinceau** de $T$, une
droite, et le contenu de la boule varie monotonement de chaque côté du plan. Les
quatrièmes points admissibles forment donc un préfixe dans chaque direction —
c'est `neighbour_along`, mot pour mot. Chercher à l'éviter revient à le
réinventer.

### Question ouverte à l'auditeur

Le cover et la fenêtre ferment la **complétude**, et le prototype le prouve à
zéro désaccord y compris sur cosphéricités. Ce qu'ils ne ferment pas est le
**générateur de candidats** : $C_q=\sum_p\binom{d_q^{+}(p)}{q-1}$ avec
$d\approx1\,450$ extrapolé sous le scénario de densité fixe, contre $D=8$ à $24$ supposé dans les
plafonds de la note.

Trois questions précises, dans l'ordre où elles bloquent :

1. le cover adaptatif par feuille est-il déjà couvert par la capability
   `center-cover + degree` telle qu'elle est écrite, ou faut-il en versionner une
   variante ? Le lemme est local, mais le `Q_q` publié est global ;
2. ~~quel gain apporte le test des quatre faces ?~~ **Répondu et mesuré** : 2,3
   fois, et la hausse du facteur disparaît — 53,1, 69,5, 69,3. Le développement
   d'un K4 s'arrête désormais à la première face refusée. La question qui reste
   est la suivante : existe-t-il une condition nécessaire sur les **quadruples**
   qui descendrait ce facteur de 70 d'un ordre de grandeur, comme le passage
   d'une à quatre faces l'a fait d'un facteur 2,3 ?
3. avant d'abandonner une famille, comment comparer parcours et source avec le
   même payload, des unités de travail homogènes, les mêmes nuages et des caps
   reçus ? En parallèle, le voisin terminal borné, les tâches transactionnelles
   et la résidence device restent les priorités du parcours.

Une quatrième question, plus petite mais bloquante pour le coût : la
construction du cover est aujourd'hui $F\cdot n$ tests, $F$ étant le nombre de
feuilles — donc quasi quadratique au côté par défaut. L'audit le note. Une
construction par grille à deux niveaux, ou une recherche des $t_q$ témoins par
anneaux croissants autour de chaque feuille, ramènerait ce terme à
$O(n+F\cdot t_q)$ ; faut-il la sceller dans la capability, ou le cover
est-il destiné à être calculé une fois hors chrono et scellé par digest ?

---

## 5. Le contrat 50 000 points, $K=10$ : 100 ms primaire, une seconde secondaire

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
2. **La récolte payait un census en $O(n)$ par candidat, et 43 % de ses tentatives étaient des doublons.** Le census fermé indexé est écrit. Le couple « support canonique puis propriétaire exact » est exercé sur les non-singletons du chemin indexé affine 3D, mais son domaine direct est faux, sa fixture signée n'est pas permanente et `emitted` conserve les singletons.

**Ce que je ne dis pas :** que le contrat est atteignable. Les deux ratios
croissent encore à $n=300$, la sortie à 50 000 points serait de l'ordre de
$3\cdot10^6$ sphères et $3\cdot10^7$ identifiants de membres, et aucun de ces
deux nombres n'est mesuré — ils sont extrapolés d'une croissance non stabilisée,
et je les donne comme tels.

### Où passe vraiment le travail, mesuré sur une seule fenêtre

J'ai d'abord déduit un rapport de 285 entre sommets visités et sphères d'arité
quatre, en rapprochant deux colonnes du tableau ci-dessus. C'était faux deux fois :
les sommets sont comptés sur le niveau strict $\le s_{\max}-2$ et les critiques sur
le rang fermé $\le s_{\max}$, et les deux profils de nuage n'étaient pas le même.
Mesuré sur **une seule** fenêtre et **un seul** profil — cube uniforme, emprise
$\propto\sqrt{n}$ en chaque coordonnée, $s_{\max}=11$, index actif :

| $n$ | sommets/pt | dont boule = miniboule de leur coquille | catalogue/pt | arités 2/3/4 par point |
| ---: | ---: | ---: | ---: | --- |
| 100 | 771,8 | 113,7 (1 sur 6,8) | 171,2 | 21,0 / 82,3 / 67,0 |
| 200 | 999,2 | 145,7 (1 sur 6,9) | 209,8 | 24,0 / 101,1 / 83,7 |
| 300 | 1 096,8 | 167,5 (1 sur 6,5) | 235,4 | 25,8 / 112,5 / 96,1 |

Un sommet d'arrangement porte une sphère critique d'arité quatre si et seulement si
sa boule est la miniboule de sa coquille — son centre dans l'enveloppe de sa
coquille. C'est **un sommet sur 6,5**, pas un sur 285. Le filtre de criticité vaut
donc un facteur sept à onze, pas deux ordres de grandeur, et adresser
l'énumération par plan plutôt que par sommet — ce que je croyais être le levier —
n'attaquerait que ce facteur-là.

### Le noyau de parcours ressemble à un problème de débit; le contrat complet non

Les deux colonnes qui comptent croissent encore : 772 → 999 → 1 097 sommets par
point, et 171 → 210 → 235 sphères par point, entre $n=100$ et $n=300$. **Rien
n'est stabilisé, donc rien n'est extrapolable proprement** — et la borne de
Clarkson--Shor pour le $\le k$-niveau de $n$ hyperplans de $\mathbb{R}^4$,
$O(n^2k^2)$, n'interdit pas que cette croissance continue.

Ce que la mesure permet en revanche de dire, c'est la **forme du noyau reverse**. Le
travail par sommet est maintenant instrumenté — **[mesuré]** sur la campagne
générique à onze points, 10,2 fermetures reconstruites par sommet, chacune un
`orient3d` par point de coquille, soit de l'ordre de 40 prédicats exacts par
sommet, et la structure de ce compte ne dépend que de la taille des coquilles,
qui reste quatre en position générale. Le contrat n'est donc pas un problème de structure de
données de visitation résidente pour **cette décision** : son coût dominant est
un débit de prédicats entiers. Le nombre de sommets 50 k, lui, n'est pas mesuré;
$10^8$ serait une extrapolation d'une croissance encore non stabilisée.

Trois propriétés en font un **candidat** de front d'onde device : pas de table de
visitation dans la décision, API de sortie streamée et prédicats entiers. Elles
ne qualifient pas encore ce chemin : le premier `.cu` ne calcule que les masques
de couples sur un batch produit par le parcours CPU, le high-water publié ne
compte qu'une partie des identifiants de pile, le sink n'est pas composé à
l'index et la transaction de sortie manque. Aucun facteur CPU/GPU n'est donc
revendiqué.

**Ce que je ne dis pas** : que le compte y est. Le microkernel a été exécuté
sur G4, mais seulement sur un batch produit par CPU et sans reçu brut versionné;
la croissance par point n'est pas stabilisée. La sortie streamée doit encore
alimenter le harvest certifié des supports, la source directe, le fold pré-lot,
la couverture et les verticales, qui n'existent pas comme pipeline. Le NO-GO
tient.

### Le lemme de demi-boule et son sweep sont exacts; le probe reste diagnostique

Le lemme du demi-boule donne une condition **nécessaire, exacte et entière**, sans
aucune hypothèse de régularité : si $p$ et $u$ sont sur la coquille d'une sphère
critique de rang $\le s_{\max}$, alors il existe un demi-espace fermé $H$ dont le
plan contient la droite $(p,u)$ avec $\lvert X\cap D_{pu}\cap H\rvert\le s_{\max}$,
où $D_{pu}$ est la boule diamétrale. La preuve tient en trois lignes : le centre est
équidistant de $p$ et $u$, donc $(c-m)\perp(u-p)$ et $R^2=\rho^2+d^2$ ; pour tout
$y$ du demi-boule côté centre,
$\lVert y-c\rVert^2=\lVert y-m\rVert^2-2(y-m)\cdot(c-m)+d^2\le R^2$.

La réduction au plan perpendiculaire à $(u-p)$ est exacte. En revanche, le
`minimum_halfplane_count` du commit `40ad152` ne calculait pas le minimum d'un
demi-plan fermé : il testait seulement les directions portées par les points et
comptait les points de frontière. Le vrai minimum peut être atteint **entre**
deux rayons.

#### Le sweep est réparé, et par une identité plutôt que par une heuristique

Un demi-plan fermé et le demi-plan **ouvert** opposé partitionnent le plan privé
de l'origine, donc

$$\min_H\lvert P\cap H_{\text{fermé}}\rvert=\lvert P\rvert-\max_H\lvert P\cap H_{\text{ouvert}}\rvert.$$

Et le maximum sur les demi-plans ouverts, lui, **est** atteint sur un arc
semi-ouvert porté par un point. Si l'arc ouvert optimal contient des points,
soit $\theta_i$ l'angle du premier ; tous les autres sont dans
$[\theta_i,\theta_i+\pi)$, et cet arc semi-ouvert est réalisable en reculant la
frontière d'un $\varepsilon$ sans point. Un balayage à deux pointeurs sur les
angles triés le calcule exactement, **sans jamais évaluer un angle** :
l'appartenance à $[\theta_i,\theta_i+\pi)$ s'écrit
`cross(d,y) > 0 ou (cross(d,y) == 0 et dot(d,y) > 0)`.

La projection a été resserrée en même temps, et ce n'était pas cosmétique.
L'ancienne base entière multipliait $d\times e$ par $d\times(d\times e)$ et
produisait des coordonnées jusqu'à $2^{73}$ : leur produit croisé atteint
$2^{147}$ et **déborde** `i128`. Ce n'était invisible que parce que l'emprise
mesurée est minuscule devant la grille déclarée. On prend maintenant directement
$r=d\times e=2(u-p)\times(z-p)$, déjà dans $d^{\perp}$, avec
$\lvert r_i\rvert<2^{34}$ sur u16 ; $r=0$ caractérise exactement les points de
la droite. On garde les deux composantes autres que l'axe canonique le plus
petit tel que $d_k\neq0$ : la restriction de cette projection à $d^{\perp}$ est
un isomorphisme linéaire, donc elle envoie demi-plans sur demi-plans et le
minimum est inchangé. Les déterminants restent sous $2^{69}$.

Six fixtures permanentes gardent le sweep, et le mutant par directions vives est
conservé dans le binaire **pour qu'elles puissent prouver qu'elles mordent** :

| fixture | exact | par directions vives |
| --- | ---: | ---: |
| contre-exemple entier de l'audit | **2** | 5 |
| trois contrats : ouvert / fermé / vif | 3 | 4 puis 5 |
| antipodes exacts | 3 | — |
| cinq rayons confondus | **2** | 7 |
| trois points de la droite | 5 | — |
| ordre sur un nuage de 24 points | 225 écarts stricts, zéro inversion | — |

#### Les deux lemmes ne bornent pas la même chose

Le lemme **fermé** borne $\lvert X\cap D_{pu}\cap H\rvert$ par le rang fermé
$\lvert S\rvert+\lvert I\rvert$. Le lemme **ouvert** borne
$2+\lvert X\cap D_{pu}^{\circ}\cap H\rvert$ par $q+\lvert I\rvert$, où $q$ est
l'arité du support. Comme $D^{\circ}\subset D$, on a
$A_{\text{ouvert}}\le A_{\text{fermé}}$ : sur le catalogue de rang fermé,
l'ouvert admet donc **plus** de paires et le fermé est le filtre le plus
sélectif. Ce n'est pas un défaut de l'ouvert : sur la source Gabriel ouverte,
qui n'exige que $q+\lvert I\rvert\le s_{\max}$ et laisse l'extra-shell libre, le
lemme fermé n'est pas valide du tout et seul l'ouvert survit. Les deux sont
mesurés ici contre la vérité du catalogue **fermé**, la seule dont ce programme
dispose ; le nombre `ADMISES ouvert` ne dimensionne donc pas la source ouverte.

**[mesuré, sweep exact, protocole homogène — cube à densité fixe $10^{-3}$,
$s_{\max}=11$, graine 20260809, deux nuages par ligne]**

| $n$ | paires totales | admises (fermé) | % du total | vraies | admises/pt | vraies/pt | admises/vraies |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 50 | 1 225 | 1 190 | 97,1 % | 828 | 23,8 | 16,6 | 1,44 |
| 100 | 4 950 | 4 042 | 81,6 % | 2 133 | 40,4 | 21,3 | 1,89 |
| 200 | 19 900 | 11 312 | 56,8 % | 5 156 | 56,6 | 25,8 | 2,19 |
| 400 | 79 800 | 28 718 | 36,0 % | 11 768 | 71,8 | 29,4 | 2,44 |
| 800 | 319 600 | 67 690 | 21,2 % | 25 734 | 84,6 | 32,2 | 2,63 |

Le sujet fautif rendait 10 697 admises à $n=200$ contre 11 312 : la correction
augmente bien `ADMIS`, d'environ 6 %, exactement comme l'audit l'avait prédit.
Sur ces nuages aléatoires, le mutant ne réfute d'ailleurs **aucune** paire
vraie : le contre-exemple est adversarial, pas typique — ce qui est précisément
pourquoi une campagne aléatoire ne pouvait pas le trouver.

**Ces cinq lignes ne prouvent toujours aucun `Big-O`.** Les incréments de
`admises/pt` par doublement valent 16,6 puis 16,2 puis 15,2 puis 12,8, et ceux
de `vraies/pt` 4,7 puis 4,5 puis 3,6 puis 2,8 : les deux suites **décroissent**,
ce qui est compatible avec $n\log n$, avec une puissance lente, et aussi avec
une saturation. Une graine, une densité et cinq tailles ne choisissent pas entre
ces familles, et `RelevantGP` n'impose aucune borne de degré : le cas général
peut rester quadratique dès l'arité deux. Le rapport `admises/vraies` croît
lentement et régulièrement — 1,44 à 2,63 — donc le filtre ne devient pas plus
sélectif avec la taille : c'est le fait le plus contraignant de cette table.

L'audit a reproduit exactement les lignes 50 et 100; les tailles supérieures ne
possèdent toujours ni sidecar brut, ni coordonnées, ni digests attendus publiés.
Le CTest impose des planchers anti-vacuité, mais aucune valeur de cette table,
aucun rang et aucun histogramme. `secondes filtre` inclut en outre par défaut le
mutant par directions vives : à `n=200`, `--ranks 0`, le même nuage passe de
0,70 s avec `--mutant 1` à 0,16 s avec `--mutant 0`. Ce champ est un chrono
diagnostique combiné, pas le coût isolé du sweep exact.

Le zéro aléatoire annoncé au commit `40ad152` ne protégeait pas la faute. La
fixture entière suivante possède une sphère critique `RelevantGP` de centre
`(100,100,100)`, rayon carré 194 et support
`{(113,100,95),(113,100,105),(87,105,100),(87,95,100)}`. En ajoutant
`(114,100,100),(115,100,100),(116,100,100)`, le demi-espace `x<=113`
contient la paire des deux premiers supports et aucun extra : le minimum exact
vaut 2. Le sujet rendait 5 et réfutait cette paire vraie à `s_max=4`.

Le lemme ne prouve pas non plus qu'une paire admise est courte : il borne un
minimum sur les deux demi-boules, tandis que l'autre côté peut contenir
arbitrairement beaucoup de points. Aucun rayon de voisinage ne s'en déduit.
Le binaire est un oracle exhaustif de diagnostic, pas un générateur : il visite
tous les points pour chaque paire. À 50 k, cela représente
`62 498 750 000 000` tests point--boule avant même le sweep angulaire.

La voie produit doit construire implicitement des voisinages avec un certificat
de couverture, publier degrés et masses combinadiques, et conserver toute
frontière non résolue pour replay. La porte `center-cover + degree` formulée
dans la note mathématique GPU donne maintenant un énoncé exact et falsifiable;
son SLO reste conditionnel aux caps reçus, jamais déduit de cette table. Son
terminal `AboveInteriorWindow` par arité doit aussi être versionné dans le
contrat avant conformité : la norme active exige encore un shell complet sur
une fenêtre uniforme plus large.

**[audit du prototype `bb31b426...`, intégré à `81f9210`]** Le résultat est
scindé et largement
positif. Le lemme, la localisation rationnelle et les `9^3` cellules passent
des oracles indépendants : 33 914 voisinages et 15 360 supports propres, zéro
écart. La partie candidate ne construit ni sommet d'arrangement ni mosaïque.
Elle transporte maintenant les listes triées de membres par coquille, compare
leur identité, refuse toute double émission et tue le mutant intégré. Les modes
jugé, mesure et cover sont exclusifs; seul le premier conclut. Petit nuage et
cap de cellules replient exactement à la racine; la dispersion porte sur tous
les `Q` effectifs; `C_q/T_q/H_q` sont calculés avant énumération et stockés en
`u128`. Treize CTests Release et huit ciblés ASan/UBSan/LSan passent. La gate
compare 30 quotients arborescents, 1 647 nœuds et 32 racines; chacun des ordres
un à cinq contribue à la masse.

Trois findings sont fermés au palier courant. CMake passe `--judge 1`
explicitement dans les gates positives; les lanes `q>s_max` sont vides au lieu
d'échouer; et une forêt complète est refusée sauf si $K+1\le s_{\max}$. Le
mutant de membre rougit, sa combinaison structurellement incohérente avec la
forêt est refusée, l'unicité des coquilles de la référence est reçue et
`candidats==C_q` est maintenant une obligation. Une porte `s_max=3` manque
encore, bien que le cas passe manuellement.

La nouvelle comparaison forêt ne reçoit pas encore le payload public. Les deux
catalogues sont ordonnés différemment; une sonde compte 4 016 indices publics
`ForestNode::source` différents avec 120/120 empreintes sémantiques égales.
L'empreinte quotientte précisément cette renumérotation et les deux côtés
appellent le même `build_forest`. Elle garde donc utilement l'invariance du fold,
mais ne certifie ni l'ordre canonique, ni le pool global et ses offsets, ni la
correction indépendante ou la sérialisation de la forêt.

Le coût produit reste NO-GO. Le cover rescane tous les points dans chaque
feuille, le CSR peut être dense, les high-waters omettent plusieurs buffers,
sorties et vérité, et une allocation sous `cell-cap` n'a aucun statut de reprise.
Le target s'arrête à 20 000 points. Les campagnes positives ont des voisinages
complets et ne reçoivent ni la frontière sélective, ni un digest attendu.
L'identité `candidates==C_q` est bien exigée par lane depuis `81f9210`, sans
valeurs attendues permanentes de $C_q/T_q/H_q$. Les chronos et high-waters
restent incomplets : le temps nommé
source inclut en réalité les folds source **et référence** et leurs empreintes,
et le pire cas récursif peut recopier des chaînes quadratiquement. Le commentaire
qui borne certains agrégats `long long` oublie jusqu'à 2 000 nuages; la borne
cumulée dépasse `i64`. Le nouveau contrôle structurel de forêt n'est pas total :
cycle de frères et enfant hors plage donnent respectivement boucle et overflow
ASan. Le label correct reste
donc **prototype CPU candidat, accord relatif au catalogue fermé partagé**; la
source Gabriel ouverte streamée et la porte 50 k ne sont pas implémentées.

Pour la source Gabriel **ouverte**, le filtre utile emploie l'intérieur
$D_{pu}^{\circ}$ de la boule diamétrale, ajoute manuellement les deux extrémités
et cherche le même demi-espace fermé. Toute paire d'un support `U` vérifiant
$\lvert U\rvert+\lvert I\rvert\leq s_{\max}$ passe alors, même avec un
extra-shell arbitraire. Ce renforcement exact a été vérifié sans écart sur
59 154 inégalités bornées; il ne fournit toujours ni rayon ni borne de degré.

Le diagnostic k-NN du commit `5d9159a` était lui aussi cassé de quatre façons,
et les quatre sont corrigées. La matrice $n^2$ et les $n$ tris étaient construits
**hors chrono** : ils ont maintenant leur propre horloge, publiée à côté de celle
du filtre. Les rangs étaient d'insertion, donc départagés par `PointId` ; ils
sont maintenant de **compétition**, ex æquo géométriques groupés, ce qui les rend
indépendants de la numérotation. `rank_max_true` n'était mis à jour qu'après
admission, donc aveugle à toute paire vraie que le filtre avait déjà supprimée ;
il balaye maintenant la vérité entière, indépendamment du filtre. Et la classe
imprimée `128` contenait les rangs au plus 127 : les bornes affichées sont
désormais `[2^i, 2^{i+1}-1]`. Enfin la matrice est plafonnée à 2 000 points et
son absence est **dite**, jamais silencieuse.

Rien de tout cela ne sauve le k-NN comme énumérateur. Une paire Gabriel vide
peut avoir un rang croisé
arbitrairement grand avec la taille du nuage. À la taille produit, prendre
`p=(0,0,0)`, `u=(65535,0,0)` et, pour `1<=i<=24999`, les points
`(0,0,i)` et `(65535,0,i)` donne exactement 50 000 points u16. Tous les extras
sont strictement hors de la boule diamétrale ouverte, donc `A=2`, tandis que
chaque extrémité a 24 999 points strictement plus proches que l'autre : le rang
croisé vaut 25 000. Aucun petit `k` contractuel ne suit donc du lemme. Le filtre
exact, un complément certifié et le replay viennent avant ce diagnostic.

### Un k-NN borné sans complément est réfuté; l'histogramme ne qualifie rien

**[mesuré, sweep exact et rangs de compétition]** rang croisé maximum, sur la
vérité entière et non plus sur le seul `ADMIS` :

| $n$ | rang max admises | rang max **vraies** | rang max vraies / $n$ |
| ---: | ---: | ---: | ---: |
| 50 | 48 | 38 | 0,76 |
| 100 | 90 | 63 | 0,63 |
| 200 | 180 | 88 | 0,44 |
| 400 | 362 | 109 | 0,27 |
| 800 | 682 | 154 | 0,19 |

Le rang maximum des paires **vraies** croît donc encore à $n=800$, et il vaut
presque le cinquième du nuage. Le rang maximum des paires **admises** reste, lui,
entre $0{,}85\,n$ et $0{,}96\,n$ sur ces cinq tailles : aucun petit cap de
voisinage n'apparaît dans cette campagne. Sur les
seules paires admises, la queue reste épaisse — 79,6 % des rangs sont sous 128 à
$n=800$, donc plus d'un cinquième au-delà.

Ces nombres sont des diagnostics, pas une loi. Ils réfutent les caps inférieurs
aux maxima observés sur ces seules entrées; la construction adversariale à
50 000 points réfute inconditionnellement tout `k<=24999` sur son entrée u16,
pas tout entier borné imaginable. La « vérité » partage en outre les
primitives de `flat_catalogue` : ce n'est pas un oracle indépendant. Un k-NN peut
rester une priorité ou un filtre, mais toute masse omise exige un complément
exact certifié et rejouable.

### Le terrain à densité fixe : quatre diagnostics, aucune asymptote encore

Le protocole précédent tirait les points dans un cube d'emprise
$\propto\sqrt{n}$ **par coordonnée**, donc de volume $\propto n^{3/2}$ : la densité
décroissait en $n^{-1/2}$ et son extrapolation à 50 000 points n'était pas
licite. `mhgp3v_scale_profile` corrige ce point avec une emprise
$\propto n^{1/3}$ pour le cube, et une aire $\propto n$ à épaisseur bornée pour
la nappe synthétique.

**[diagnostic publié, protocole encore hétérogène]** $s_{\max}=11$, densité
$10^{-3}$, un cœur :

| $n$ | sommets/pt | catalogue/pt | répétitions reçues |
| ---: | ---: | ---: | ---: |
| 100 | 805,5 | 159,3 | 2 |
| 200 | 1 011,5 | 219,8 | 1 |
| 400 | 1 171,9 | 266,3 | 1 |
| 800 | 1 271,9 | 299,9 | 1 |

Les commandes `--points 400/800 --smax 11 --repeats 1 --seed 20260809` ont
été reproduites au binaire Release et retrouvent exactement ces masses; leurs
sorties brutes ne sont toutefois pas versionnées.

La ligne cube `n=100` agrège deux nuages, mais `n=200` un seul; au même binaire
et avec deux répétitions, `n=200` donne plutôt `1 013,5/216,80`. Les incréments
publiés comparent donc des estimateurs différents. La nappe synthétique donne
806,7 / 1 013,6 / 1 162,1 / 1 250,2 sommets par point, sans dispersion scellée.
Son catalogue vaut 240,47 puis 257,00 par point à `n=400/800`, contre
266,28 puis 299,94 pour le cube : l'écart de sortie atteint 9,7 % puis 14,3 % et
augmente sur cette fenêtre.
Même avec un protocole homogène, trois incréments n'identifieraient ni une loi
géométrique ni une fonction bornée. Les valeurs 1 430 sommets/point et 390
sphères/point obtenues en prolongeant les derniers incréments sont un scénario
de modèle, pas des asymptotes.

### Ce que ce scénario ferait au contrat de 100 ms

Sous les hypothèses `1 430 sommets/point` et `575 M sommets/s`, la seule passe
d'admissibilité de 71,5 millions de sommets prendrait environ 0,124 s. Sous
l'hypothèse supplémentaire de 19 millions de sphères, 100 ms donneraient 5,3 ns
par sortie. Ces divisions sont correctes; les masses, le transport du débit et
le facteur dix à trente du pipeline ne sont ni bornés ni reçus. Elles ne donnent
donc pas un écart mesuré de quinze à quarante.

Le générateur local de la note actuelle présuppose bien un propriétaire shallow;
il ne peut pas, à lui seul, retirer le terrain. Cela n'exclut pas une **autre**
source directe. La note d'audit GPU formule un premier jalon falsifiable pour
les supports d'arités deux à quatre : une couverture certifiée de l'espace des
centres borne le rayon de toute sortie admissible, puis un voisinage exact de
rayon prouvé contient support, intérieur et coquille complets. Cette voie ne
construit aucun sommet d'arrangement ni mosaïque. Son exactitude est démontrable;
son SLO reste conditionné aux degrés complets, masses combinadiques et replays
publiés dans le reçu.

Le P0 des probes historiques `40ad152`/`5d9159a` est corrigé à `180975e` par un
sweep circulaire exact, des fixtures et un mutant permanent. Cela n'en fait pas
encore un juge de cette voie : sa vérité partage `flat_catalogue`, décrit le
catalogue fermé et non la source Gabriel ouverte, tandis que l'énumération reste
exhaustive en paires et points. Il falsifie et mesure le lemme borné; il ne
construit ni le voisinage certifié ni le stream produit.

**Conclusion actuelle :** aucune route démontrée n'atteint le contrat, et aucune
de ces quatre tailles ne prouve qu'il est impossible. Le prochain résultat
décisionnel est soit un théorème de borne, soit une campagne multi-graines avec
quantiles jusqu'à 50 k et pipeline complet, pas l'extrapolation d'une asymptote.

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

1. la mémoire de décision n'a plus de table en $O(V)$ et peut tomber à la pile plus un scratch $O(n)$, à condition de streamer la sortie ;
2. deux sous-arbres n'échangent plus d'état de visitation, ce qui permet leur expansion parallèle ;
3. l'arbre devient déterministe sous une clef canonique, tandis que le déterminisme byte-à-byte de sortie reste celui du tri secondaire et des lots.

Le dépôt en possède déjà une preuve constructive,
[`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](audits/AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md),
pour l'arrangement simple. Son extension au vrai graphe multiplicitaire et une
règle qui choisit directement un rayon du parent sont maintenant prouvées dans
[`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](audits/NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md).
Le commit `969db5c` implémente la reverse search dans un endpoint différentiel et
la confronte au BFS. Le delta live ajoute une porte indexée à nœuds internes et
une API sink avec high-water partiel des payloads du chemin. Restent ouverts la
mesure de l'élagage propre aux requêtes reverse, le high-water mémoire complet,
l'intégration du sink au catalogue et sa forme device.

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

Le sous-ensemble scellé par l'audit courant et le noyau F0 :

```sh
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_flats_(fixtures|generic|indexed_tree|degenerate|cospherical|u16_owner)$' -j2
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_(gate_d_fold_f0|gate_d_fold_f0_optimised|admissible_pair_sweep)$'
```

Quand Python est trouvé, le noyau F0 est maintenant un CTest en mode normal
**et** sous `python3 -O` : c'est ce drapeau qui effaçait les vingt-sept
obligations portées par `assert` et laissait imprimer un `PASS` vide. Elles
passent désormais par des contrôles explicites. La configuration reste toutefois
fail-open sans interpréteur, car `find_package(Python3 ...)` n'est pas
`REQUIRED` : dans ce cas les deux CTests disparaissent sans faire échouer CMake.

Trois choses ont été intégrées depuis l'audit. Le carré géométrique tout $N_a$
d'arité quatre est une fixture permanente, et il **est** une naissance : la garde
de carrier qui le refusait est retirée de la vérité comme du sujet, et la
refuser est devenue le mutant `reject_carrierless_birth`. L'invariant régulier
« au moins deux facettes strictes » possède un vérificateur séparé par record
brut avant projection; son mutant par composante est
tué par le lot de contrebande `bad_all_new` / `good_with_strict`, où un record
parfaitement régulier masque un record sans aucune facette stricte. Enfin un
troisième oracle énumère les partitions de Bell et exige que la partition
respectant les records et raffinant toutes les autres soit unique : Warshall et
le DSU ferment tous deux transitivement et peuvent partager une erreur de
fermeture, celui-ci ne ferme rien. Il juge les 2 168 hypergraphes du domaine
exhaustif, mais partage encore la classification des composantes.

Le vérificateur régulier n'est pas encore autonome : il compte les occurrences
de handles stricts sans en vérifier l'unicité. Un record qui répète deux fois le
même handle strict est accepté, alors que `resolve_batch` le refuse comme
`duplicate raw endpoint`. Son contrat doit donc imposer un lot déjà validé ou
réutiliser les contrôles structurels communs, avec une fixture négative.

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
| 2 | règle de propriétaire pour les arités 2 et 3, et census local | P0 de troncature `i128` fermé sur u16; garde de type encore partielle et identité signée non protégée; table nulle seulement pour owner+index+navigable; réducteur linéaire prouvé mais non intégré, census ramené à un halfspace-report 4D exact |
| 3 | reverse search, pour supprimer `seen` et `frontier` | parent multiplicitaire prouvé, **parcours et sink écrits et différenciés contre le BFS**; le catalogue passe encore par le BFS, et le high-water complet n'est pas mesuré |
| 4 | référence de l'oracle M1 tolérante aux multiplicités | non écrite ; sans elle le sujet n'a pas de juge indépendant en arithmétique rationnelle |
| 5 | source active/silencieuse, tri et lots, état horizontal, `coverage_log`, verticales et contrat d'identité | non écrits; globalités intrinsèques mais externalisables, factorisées dans la note Gate D aval |
| 6 | invariance topologique du support canonique quand plusieurs supports minimaux portent la même miniboule | ouverte ; la convention par coordonnées est *une* convention, pas un théorème |
| 6 bis | sémantique quotientée des observations confondues | ouverte ; le prototype les **refuse** explicitement plutôt que de publier un support dépendant de la numérotation |
| 7 | `sphere.hpp` au bord produit : paire de points confondus acceptée comme support d'arité deux, sentinelle `den==0` sans garde | ouverts, hors de ce fichier |
| 8 | le contrat 50 k / $K=10$ / 100 ms primaire, 1 s secondaire | **non atteint, non mesuré; les ratios observés ne bornent pas 50 k et le pipeline complet n'existe pas** |
| 9 | publication directe des $n$ singletons | fermée dans le chemin indexé de `1a0a1f8`; le différentiel conserve le census du chemin lent comme oracle relatif |
| 10 | la taille $V$ du $\leq k$-niveau en général | **non bornée utilement** : Clarkson--Shor est quadratique en $n$ et en $k$, et les mesures ne valent que pour le régime de surface (§5) |
| 11 | le régime multi-captation | mesuré **moins peu profond** que la reconstruction fusionnée ; c'est la branche no-go de Gate D |

Le détail, les budgets et le journal des affirmations retirées sont dans
[`PROPOSITION.md`](PROPOSITION.md).
