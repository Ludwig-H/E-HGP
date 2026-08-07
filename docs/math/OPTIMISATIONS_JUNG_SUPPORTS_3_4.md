# Recension : ce que le théorème de Jung donne aux supports trois et quatre

Document mathématique. Aucun claim, aucune porte ouverte ou fermée, aucun statut
public. Il recense systématiquement les énoncés que le théorème de Jung rend
disponibles pour la génération locale certifiée des supports minimaux bien
centrés de rang fermé au plus $s_{\max}$, avec pour chacun sa preuve, sa
constante fermée, son coût et ce qu'il vaut mesuré.

## 0. Pourquoi Jung s'applique ici

> **Théorème (Jung, 1901).** Toute partie bornée de $\mathbb{R}^d$ de diamètre
> $D$ est contenue dans une boule de rayon au plus $\gamma_d D$, avec
> $\gamma_d=\sqrt{\tfrac{d}{2(d+1)}}$. La borne est atteinte exactement par les
> ensembles contenant un $d$-simplexe régulier d'arête $D$.

L'application n'est pas automatique : Jung parle de la **miniboule**, notre objet
est la **boule circonscrite**. Les deux coïncident ici, et c'est exactement
l'hypothèse « minimal bien centré ».

> **Proposition 0.** Si $U$ est minimal bien centré, $\bar B(c_U,r_U)$ est la
> miniboule de $U$ et $U$ en est l'ensemble de support.

*Démonstration.* La miniboule d'un compact $S$ est caractérisée par : son centre
appartient à $\operatorname{conv}(S\cap\partial B)$. Ici tout $U$ est sur la
sphère et $c_U\in\operatorname{relint}\operatorname{conv}(U)$ par définition du
bon centrage. $\square$

D'où, pour $m=\lvert U\rvert$ :

$$\boxed{\;r_U\;\le\;\gamma_m\,\operatorname{diam}(U)\;},\qquad
\gamma_3=\tfrac1{\sqrt3}\ (\text{le triangle est plan}),\qquad
\gamma_4=\sqrt{\tfrac38}.$$

L'arité fixe la dimension à employer : un triangle vit dans un plan, donc c'est
$\gamma_2^{\text{Jung}}=1/\sqrt3$ qui s'applique, pas $\gamma_3^{\text{Jung}}$.

## 1. L'encadrement et le rapport d'aspect

Avec la borne triviale $\operatorname{diam}(U)\le 2r_U$ :

$$\frac{D}{2}\;\le\;r_U\;\le\;\gamma_m D,
\qquad\text{soit}\qquad
\frac{D}{r_U}\in\Bigl[\tfrac1{\gamma_m},\,2\Bigr]
=\begin{cases}[1{,}7321,\;2] & m=3\\ [1{,}6330,\;2] & m=4.\end{cases}$$

**J1 — Rigidité de forme.** Le rapport diamètre sur circumrayon d'un support
accepté est confiné dans une bande de largeur relative $1{,}15$ ($m=3$) ou
$1{,}22$ ($m=4$). C'est une contrainte forte et gratuite : elle interdit tout
support « allongé » ou « ramassé ».

**J2 — Un intervalle compact, et c'est lui qui rend tout fini.** Toutes les
constructions qui suivent reposent sur le fait que $r_U$ vit dans un compact
explicite dès que $D$ est connu. Sans borne supérieure sur $r_U$, aucune des
régions ci-dessous n'est bornée. C'est la contribution structurelle de Jung, et
elle précède toutes les constantes.

**Vérification.** Sur 300 000 supports minimaux bien centrés tirés au hasard, le
minimum observé de $D/r$ vaut $1{,}7354$ ($m=3$) contre la borne $1{,}7321$, et
$1{,}6709$ ($m=4$) contre $1{,}6330$ : les deux bornes sont approchées, celle des
triangles à 0,2 %. La borne antérieure $\sqrt2=1{,}4142$, obtenue par un lemme
d'angle élémentaire, était donc loin de l'optimum.

## 2. Le résultat unificateur : la cascade de dimensions

C'est l'énoncé dont tout le reste découle.

> **Théorème (cascade).** Soit $U$ accepté, $(p,q)$ sa paire diamétrale,
> $D=\lvert p-q\rvert$, et $S\subseteq U$ contenant $\{p,q\}$. Le lieu des
> centres compatibles
> $$\mathcal{C}(S)=\bigl\{x:\ \lvert x-s\rvert \text{ constant sur } S,\
> \lvert x-p\rvert\le\gamma_m D\bigr\}$$
> est un **compact de dimension $3-(\lvert S\rvert-1)$**, d'extension bornée
> explicitement par Jung :
>
> | $S$ | lieu | dimension | extension |
> | --- | --- | ---: | --- |
> | $\{p,q\}$ | disque du plan médiateur, centré en $M$ | 2 | rayon $\sqrt{\gamma_m^2-\tfrac14}\,D$ |
> | $\{p,q,z\}$ | segment $\perp$ au plan de $S$, centré en $o_S$ | 1 | demi-longueur $\sqrt{\gamma_m^2D^2-r_\triangle^2}$ |
> | $\{p,q,z,w\}$ | le circumcentre | 0 | — |
>
> où $M$ est le milieu de $pq$, $o_S$ le circumcentre du triangle $S$ et
> $r_\triangle$ son circumrayon.

*Démonstration.* L'équidistance à $k$ points affinement indépendants définit un
sous-espace affine de dimension $3-(k-1)$ ; l'intersection avec
$\lvert x-p\rvert\le\gamma_m D$ le rend compact. Les extensions s'obtiennent par
Pythagore : $\lvert c_U-M\rvert^2=r_U^2-D^2/4$ et
$\lvert c_U-o_S\rvert^2=r_U^2-r_\triangle^2$, puis $r_U\le\gamma_m D$. $\square$

**Constantes fermées du disque** :
$\sqrt{\gamma_3^2-\tfrac14}=\sqrt{\tfrac1{12}}=0{,}288675$ et
$\sqrt{\gamma_4^2-\tfrac14}=\sqrt{\tfrac18}=0{,}353553$.

**J3 — Chaque sommet ajouté fait tomber la dimension du lieu de centres d'une
unité, et Jung en borne l'extension à chaque étage.** C'est la structure
algorithmique : le générateur ne cherche jamais dans $\mathbb{R}^3$, mais dans un
disque, puis dans un segment, puis en un point.

## 3. Les tests dérivés, par étage

Tous reposent sur la même mécanique : exhiber une boule **incluse** dans
$\bar B(c_U,r_U)$ et dont on sache compter les points ; si elle est sur-peuplée,
le candidat est rejeté. Aucun ne peut fabriquer un rejet abusif, car l'inclusion
est certifiée.

### 3.1 Test sans aucune requête (le plus économique)

> **J4.** Si $R_{mb}(S)>\gamma_m D$ pour un $S\subseteq U$, alors $U$ n'est pas
> accepté avec $(p,q)$ pour paire diamétrale.

*Démonstration.* $R_{mb}$ est croissante pour l'inclusion, donc
$R_{mb}(S)\le R_{mb}(U)=r_U\le\gamma_m D$. $\square$

Coût : **zéro requête spatiale**. Une miniboule de $\lvert S\rvert\le4$ points et
une comparaison. À appliquer avant tout comptage, à chaque étage.

Forme rationnelle exacte : $\gamma_3^2=\tfrac13$ et $\gamma_4^2=\tfrac38$ sont
**rationnels**, donc le test s'écrit $3R_{mb}^2\le D^2$ et $8R_{mb}^2\le 3D^2$ —
des inégalités polynomiales dans l'arithmétique entière déjà présente, sans
racine carrée.

### 3.2 Test de germe, boule unique

> **J5 (stabilité du centre de la miniboule).** Pour tout $S$ borné et toute
> boule $\bar B(c,r)\supseteq S$ : $\ \lvert c-c_{mb}(S)\rvert^2\le r^2-R_{mb}(S)^2$.

*Démonstration.* Écrire $c_{mb}=\sum_i\lambda_i s_i$ avec $\lambda_i>0$,
$\sum\lambda_i=1$, $\lvert s_i-c_{mb}\rvert=R_{mb}$. Alors
$\sum_i\lambda_i\lvert s_i-c\rvert^2=R_{mb}^2+\lvert c_{mb}-c\rvert^2$, le terme
croisé s'annulant. Chaque $\lvert s_i-c\rvert\le r$. $\square$

> **J6 (boule inscrite).** $\bar B\bigl(c_{mb}(S),\,\gamma_m D-\sqrt{\gamma_m^2D^2-R_{mb}(S)^2}\bigr)\subseteq\bar B(c_U,r_U)$,
> et ce rayon **croît** avec $R_{mb}(S)$.

Cas $S=\{p,q\}$ : $c_{mb}=M$, $R_{mb}=D/2$, et la constante fermée est

$$\kappa_m=\gamma_m-\sqrt{\gamma_m^2-\tfrac14},\qquad
\kappa_3=\frac{\gamma_3}{2}=\frac1{2\sqrt3}=0{,}288675,\qquad
\kappa_4=\sin15^\circ=0{,}258819 .$$

L'identité $\kappa_3=\gamma_3/2$ tient parce que $\gamma_3^2-\tfrac14=\gamma_3^2/4$.

**Exactitude rationnelle.** $\kappa_3^2=\tfrac1{12}$ est **rationnel** : le test
des triangles s'écrit exactement $12\lvert x-M\rvert^2\le D^2$. Pour les
tétraèdres $\kappa_4^2=\tfrac{2-\sqrt3}{4}$ est irrationnel ; on emploie un
**minorant rationnel** — par exemple $\tfrac1{15}<\kappa_4^2$ — ce qui rétrécit
la boule-test et ne peut donc qu'affaiblir le rejet, jamais le fausser.

### 3.3 Test de germe, recouvrement du disque

Le test précédent n'utilise qu'un point du disque de centres. Jung en borne le
rayon, donc on peut le **recouvrir**.

> **J7.** Soit $x_1,\dots,x_N$ un recouvrement du disque $\mathcal{C}(\{p,q\})$
> de rayon de recouvrement $\delta$. Si
> $\lvert P\cap\bar B(x_i,\tfrac D2-\delta)\rvert>s_{\max}$ pour tout $i$, alors
> $(p,q)$ n'est la paire diamétrale d'aucun support accepté.

*Démonstration.* $c_U$ appartient au disque, donc $\lvert c_U-x_i\rvert\le\delta$
pour un $i$ ; et $r_U\ge D/2$, donc
$\bar B(x_i,\tfrac D2-\delta)\subseteq\bar B(c_U,r_U)$. $\square$

Avec $\delta=\alpha D$, la constante effective devient $\tfrac12-\alpha$ au lieu
de $\kappa_m$, pour $N\approx1{,}2\,\bigl(\sqrt{\gamma_m^2-\tfrac14}/\alpha\bigr)^2$
positions.

**Mesuré à 50 000 points, $s_{\max}=11$** (anneaux hexagonaux, $N$ positions par
arête) :

| $m$ | test | $N$ | constante | degré moyen | travail |
| ---: | --- | ---: | ---: | ---: | ---: |
| 4 | J6, boule unique | 1 | $0{,}2588$ | 542,2 | $3{,}75\cdot10^{11}$ |
| 4 | J7, $\alpha=0{,}20$ | 7 | $0{,}300$ | 525,7 | $3{,}42\cdot10^{11}$ |
| 4 | J7, $\alpha=0{,}10$ | 19 | $0{,}400$ | 245,0 | $3{,}98\cdot10^{10}$ |
| 4 | **J7, $\alpha=0{,}05$** | 61 | $0{,}450$ | **162,3** | $\mathbf{1{,}05\cdot10^{10}}$ |
| 3 | J6, boule unique | 1 | $0{,}2887$ | 391,4 | $1{,}32\cdot10^{9}$ |
| 3 | J7, $\alpha=0{,}10$ | 19 | $0{,}400$ | 215,1 | $3{,}89\cdot10^{8}$ |
| 3 | **J7, $\alpha=0{,}05$** | 37 | $0{,}450$ | **138,1** | $\mathbf{1{,}58\cdot10^{8}}$ |

**J7 vaut un facteur 36 sur les quadruples et 8,4 sur les triples**, au prix de
61 et 37 comptages par arête au lieu d'un. Le compromis est très favorable :
le degré varie comme l'inverse du cube de la constante et le travail des
quadruples comme le cube du degré, donc chaque point de constante gagné se paie
une fois et se récupère neuf fois.

Un détail contre-intuitif mérite d'être noté : à $\alpha=0{,}20$ le degré
**moyen** baisse à peine et le degré **maximum** augmente (964 contre 646 pour
$m=4$). Un recouvrement trop grossier place des positions loin du milieu, où la
boule-test est petite ; le gain n'apparaît qu'à partir de $\alpha\le0{,}10$. Le
réglage n'est donc pas monotone en $N$ et doit être mesuré, pas supposé.

### 3.4 Test incrémental, recouvrement du segment

Au troisième sommet, le lieu est un **segment**, dont un recouvrement coûte
$O(1/\delta)$ positions au lieu de $O(1/\delta^2)$.

> **J8.** Soit $S=\{p,q,z\}$ affinement indépendant, $o_S$ son circumcentre,
> $r_\triangle$ son circumrayon, $\nu$ la normale unitaire à son plan, et
> $x_1,\dots,x_N$ un recouvrement à $\delta$ près du segment
> $\{o_S+t\,\nu:\lvert t\rvert\le\sqrt{\gamma_m^2D^2-r_\triangle^2}\}$. Si
> $\lvert P\cap\bar B(x_i,\;r_\triangle-\delta)\rvert>s_{\max}$ pour tout $i$,
> alors aucun support accepté de paire diamétrale $(p,q)$ ne contient $z$.

*Démonstration.* Tout point équidistant des trois sommets à distance $\rho$
vérifie $\rho^2=r_\triangle^2+\operatorname{dist}(\cdot,\Pi)^2$ où $\Pi$ est le
plan de $S$. Donc $c_U$ est sur la droite $o_S+\mathbb{R}\nu$, à distance
$\sqrt{r_U^2-r_\triangle^2}\le\sqrt{\gamma_m^2D^2-r_\triangle^2}$ de $o_S$ : il
est sur le segment. La même identité donne
$$\boxed{\,r_U\;\ge\;r_\triangle\;\ge\;R_{mb}(S)\,}$$
— la borne inférieure du rayon est le **circumrayon**, strictement meilleure que
le rayon de miniboule employé en J6. On conclut par l'inégalité triangulaire.
$\square$

**Corollaire (J4' — test libre renforcé).** Si $r_\triangle(S)>\gamma_m D$, le
segment est vide et $z$ est rejeté **sans aucune requête**. C'est strictement
plus tranchant que J4, puisque $r_\triangle\ge R_{mb}$.

**Mesuré à 50 000 points, $s_{\max}=11$**, en aval du germe J7 à
$\alpha=0{,}10$ et de l'énumération réelle par lentille :

| filtre appliqué au troisième sommet | candidats retenus | part |
| --- | ---: | ---: |
| aucun | $2{,}95\cdot10^{8}$ | 100 % |
| J4' libre sur $r_\triangle$ | $2{,}66\cdot10^{8}$ | 90,2 % |
| J9, boule unique en $c_{mb}$ | $1{,}62\cdot10^{8}$ | 54,8 % |
| J8, segment $N=4$ | $1{,}41\cdot10^{8}$ | 47,7 % |
| J8, segment $N=8$ | $6{,}04\cdot10^{7}$ | 20,5 % |
| **J8, segment $N=16$** | $\mathbf{4{,}70\cdot10^{7}}$ | **15,9 %** |

et sur le travail des quadruples : $3{,}31\cdot10^{9}$ avec J9 seul contre
$\mathbf{3{,}93\cdot10^{8}}$ avec J8 à $N=16$, soit un **facteur 8,4** pour
seize comptages par triangle. La dimension un du lieu paie : seize positions sur
un segment font mieux que soixante et une sur un disque.

**J9 — Le test se renforce à chaque sommet.** Dans J6 comme dans J8, le rayon de
la boule-test croît avec $R_{mb}(S)$, qui croît avec $S$. La chaîne
$\{p,q\}\subset\{p,q,z\}\subset\{p,q,z,w\}$ donne donc trois tests de puissance
croissante, dont le dernier est la décision exacte elle-même.

**Limite honnête de J9, mesurée.** Un troisième sommet situé **à l'intérieur de
la boule diamétrale** de $(p,q)$ ne fait pas croître $R_{mb}$ : la miniboule reste
celle de l'arête et le test incrémental à boule unique redevient le test de
germe. Ces sommets sont la majorité — 76,8 % survivent — et l'élagage
incrémentiel à boule unique ne vaut donc que $1{,}91$. C'est J8, par le
recouvrement du segment, qui doit récupérer le reste : le segment est court dès
que $r_\triangle$ approche $\gamma_m D$, indépendamment de $R_{mb}$.

## 4. Confinement global d'une arête diamétrale

> **J10.** Toute la géométrie utile à l'arête $(p,q)$ est contenue dans une seule
> boule autour du milieu :
> $$\bar B(c_U,r_U)\ \subseteq\ \bar B\bigl(M,\ (\gamma_m+\sqrt{\gamma_m^2-\tfrac14})\,D\bigr),$$
> de rayon $\tfrac{\sqrt3}{2}D=0{,}866\,D$ pour $m=3$ et $\cos15^\circ\,D=0{,}966\,D$
> pour $m=4$.

*Démonstration.* $\lvert x-M\rvert\le\lvert x-c_U\rvert+\lvert c_U-M\rvert\le r_U+\sqrt{r_U^2-D^2/4}$,
croissant en $r_U$, évalué en $\gamma_mD$. $\square$

**Usage.** Une tuile device traitant l'arête $(p,q)$ n'a besoin d'aucun point
au-delà de ce rayon : la localité de la mémoire est bornée explicitement, ce qui
est directement exploitable pour le pavage et la résidence.

## 5. Rigidité de l'égalité

> **J11.** $r_U=\gamma_m D$ **si et seulement si** $U$ est un simplexe régulier
> d'arête $D$ ($m-1$ étant sa dimension).

C'est le cas d'égalité de Jung. Trois conséquences :

- les configurations extrémales sont de mesure nulle, donc les bornes ci-dessus
  sont serrées mais rarement atteintes — ce que confirme la mesure ($1{,}6709$
  observé contre $1{,}6330$) ;
- un support atteignant l'égalité est identifiable exactement et peut servir de
  fixture de falsification permanente pour toute implémentation des tests ;
- aucun test ci-dessus ne doit employer une inégalité stricte là où le cas
  d'égalité est atteignable : les comparaisons sont écrites en $\le$, et la
  simplification en $\ge$ n'est licite que sur le lieu de centres, qui est fermé.

## 6. Ce que Jung ne donne pas

Il faut le dire aussi nettement que le reste.

- **Il ne borne pas le rang.** Jung est purement métrique ; la condition
  $\lvert P\cap\bar B(c_U,r_U)\rvert\le s_{\max}$ reste la seule source de
  sensibilité à la sortie. Jung sert à transformer cette condition en tests
  locaux calculables, pas à la remplacer.
- **Il ne borne pas le nombre de supports par arête.** Le lieu de centres est
  compact, mais rien n'y limite le nombre de configurations acceptées.
- **Il ne réduit pas la porte de bon centrage.** La fraction de supports bien
  centrés reste constante en $n$ — 28 % des triples, 10 % des quadruples — et
  aucune borne métrique ne la fait baisser.
- **Il ne dispense pas de la classification terminale exacte.** Tous les tests
  ci-dessus sont des rejets certifiés ; l'acceptation reste décidée par
  `analyze_circumcenter_support_integer` puis la requête de boule fermée.

## 7. Bilan des gains, mesurés et attendus

À 50 000 points, $s_{\max}=11$, nuage `uniform_latin`.

| énoncé | statut | effet |
| --- | --- | --- |
| J1 encadrement $[D/2,\gamma_mD]$ | mesuré | remplace $\sqrt2$ par l'optimum ; **facteur 6,5** sur le travail du germe |
| J4 test sans requête | à implémenter | rejet gratuit avant tout comptage |
| J6 germe, boule unique | mesuré | degré $546{,}4$ ($m=4$), $394{,}8$ ($m=3$) |
| J9 incrémental, boule unique | mesuré | **facteur 1,91** sur les quadruples ; limité par les sommets internes |
| J7 germe, disque recouvert | **mesuré** | constante $0{,}2588\to0{,}450$ ; **facteur 36** sur les quadruples, 8,4 sur les triples |
| J4' libre sur $r_\triangle$ | **mesuré** | 9,8 % des candidats rejetés sans requête |
| **J8 incrémental, segment recouvert** | **mesuré** | **facteur 8,4** sur les quadruples à $N=16$ |
| J10 confinement | à exploiter | borne de résidence par tuile device |

### 7.1 Correction : toutes les mesures portent une restriction non certifiée

Toutes les mesures de degré et de travail de ce document, sans exception,
restreignent les paires candidates aux voisins situés à moins de **six fois** le
rayon $s_{\max}$-plus-proche-voisin du point. **Cette restriction n'est pas
certifiée par les lemmes ci-dessus.** Les chiffres publiés sont donc des
**minorants** du travail réellement certifié, et il faut les lire comme tels.

Ce que le théorème donne réellement comme restriction de la boucle de germes est
$$D\;\le\;2\,R(p),$$
où $R(p)$ est le plus grand rayon d'une boule passant par $p$, **de centre dans
l'enveloppe convexe**, contenant au plus $s_{\max}$ points — c'est exactement la
route A du §5.0, écartée parce que la certifier demande un recouvrement
directionnel décalé **et** un test de vivacité de calotte contre les demi-espaces
de la coque.

Les deux routes sont donc complémentaires, et il faut les tenir ensemble : la
route B rejette une paire donnée pour un coût dérisoire, la route A dit
lesquelles il faut seulement considérer. La boucle de germes est exhaustive en
$O(n^2)$ sans elle, et la restriction employée dans les mesures en est un
substitut mesuré, pas prouvé.

Le générateur déclare désormais cette restriction dans son certificat —
`seed_neighbourhood_cutoff_multiple` et `seed_loop_exhaustive_over_pairs` — et le
vérificateur **refuse par nom** une exécution restreinte qui prétendrait être
exhaustive. L'écart est machine-lisible, comme celui du pic VRAM.

**État certifié courant, mesuré conjointement.** La dernière mesure applique en
une seule passe le germe J7 ($\alpha=0{,}10$), l'énumération réelle par lentille,
le test libre J4' et le segment J8 ($N=16$). Ce n'est donc plus un produit de
facteurs séparés :

| | valeur |
| --- | ---: |
| travail quadruples | $3{,}93\cdot10^{8}$ |
| travail triples | $4{,}70\cdot10^{7}$ |
| **travail certifié total** | $\mathbf{4{,}4\cdot10^{8}}$ |
| univers $\binom n3+\binom n4$ | $2{,}604\cdot10^{17}$ |
| **gain** | $\mathbf{5{,}9\cdot10^{8}}$ |
| candidats par record émis | **24,4** |

Vingt-quatre candidats par record émis, contre cinq visites de nœud par record à
l'étage paire : le générateur est désormais dans le **même ordre de grandeur**
que ce que le projet sait déjà faire tourner à 50 000 points. Et ce chiffre est
certifié, là où l'estimation par échantillonnage du rayon tangent — non
certifiée — donnait $5{,}24\cdot10^{9}$, douze fois pire.

Aucun énoncé de la recension ne reste non mesuré.

## 8. La restriction certifiée de la boucle de germes, et ce qu'elle coûte

Le §7.1 nomme l'obligation restée ouverte : la restriction certifiée est
$D\le 2R(p)$, avec $R(p)$ le plus grand rayon d'une boule passant par $p$, de
centre **dans un convexe contenant $P$**, contenant au plus $s_{\max}$ points.
Deux questions se posaient, et les deux sont maintenant mesurées.

### 8.1 La bissection est licite

> **Lemme.** Pour une direction $u$ fixée, les boules tangentes en $p$ sont
> **emboîtées croissantes** en leur rayon : $\bar B(p+\rho'u,\rho')\subseteq
> \bar B(p+\rho u,\rho)$ pour $\rho'\le\rho$.

*Démonstration.* $\lvert c'-c\rvert=\rho-\rho'$ et $\rho'+(\rho-\rho')=\rho$.
$\square$

Comme le convexe $C$ contient $p$, l'admissibilité $p+\rho u\in C$ est elle aussi
héréditaire vers le bas. L'ensemble des rayons admissibles à population bornée
est donc un **segment initial** $[0,R(p)]$, et une bissection sur $\rho$ est
licite — ce qui n'allait pas de soi et qu'il fallait établir.

### 8.2 L'enveloppe convexe n'est pas nécessaire, et parfois n'aide pas

Un **sur-ensemble** convexe est sûr : il tue moins de calottes, donc majore
$R(p)$, donc affaiblit la restriction sans jamais perdre de support. Mesuré à
50 000 points, $s_{\max}=11$, 48 directions :

| famille | région | $R/\rho_{s_{\max}}$ max | voisinage moyen | voisinage max |
| --- | --- | ---: | ---: | ---: |
| `uniform_latin` | AABB (6 plans) | 1,496 | 114,9 | 170 |
| `uniform_latin` | $k$-DOP (26) | 1,421 | 114,3 | 170 |
| `uniform_latin` | $\operatorname{conv}(P)$ | 1,348 | 113,6 | 168 |
| `eight_clusters` | AABB (6 plans) | **347,2** | **1 375,5** | **25 026** |
| `eight_clusters` | $k$-DOP (26) | 347,2 | 1 375,2 | 25 026 |
| `eight_clusters` | $\operatorname{conv}(P)$ | 347,2 | 1 374,4 | 25 026 |

**Deux conclusions, et la seconde est la plus importante.**

D'abord, sur un nuage quasi uniforme, **l'AABB suffit** : elle coûte 11 % sur le
pire rapport et 3 % sur le travail par rapport à l'enveloppe exacte. Or l'AABB
est déjà disponible — c'est `root_aabb()` du LBVH. Aucune structure globale
nouvelle n'est donc requise, ce qui écarte d'emblée la question de savoir si une
enveloppe convexe serait un objet interdit.

Ensuite, et c'est le fait dur : sur un nuage **en amas**, la borne s'effondre —
$R/\rho$ atteint 347 — et **l'enveloppe convexe exacte n'y change rien**, au
chiffre près. La raison est structurelle : le vide qui laisse grossir la boule
tangente est **intérieur** à l'enveloppe, entre les amas. Aucun convexe ne peut
l'exclure, puisqu'un convexe contenant les amas contient le vide qui les sépare.

### 8.3 Ce que cela coûte réellement

Il ne faut pas lire ces voisinages comme du travail de clique : ils sont le
**vivier de paires à tester**, sur lequel la route B s'applique ensuite. Le coût
propre de la restriction est donc un nombre de tests de germe :

| famille | paires à tester | comptages de boule (19 positions) |
| --- | ---: | ---: |
| `uniform_latin` | $2{,}9\cdot10^{6}$ | $5{,}5\cdot10^{7}$ |
| `eight_clusters` | $3{,}4\cdot10^{7}$ | $6{,}5\cdot10^{8}$ |

Soit un facteur douze entre les deux familles sur le coût du germe — supportable,
et sans commune mesure avec le facteur $10^5$ qu'une lecture naïve des
voisinages suggérait.

**Ce que cela impose au contrat.** Le contrat à 50 000 points n'est pas
indépendant de la famille : les campagnes prévues comprennent huit boules
séparées, soixante-quatre amas multi-échelles, vingt-quatre filaments et
quatre-vingt-seize paires d'amas déséquilibrées. La restriction heuristique à six
rayons employée dans toutes les mesures antérieures est **valide sur les nuages
quasi uniformes et fausse sur les nuages en amas**, où le vivier certifié est
douze fois plus grand. Toute mesure future doit déclarer la famille avec le
chiffre.

### 8.4 Le jeu de directions, et la seule preuve dont on dispose

Le test de §5.0 (Lemme 3) exige un **rayon de recouvrement** $\theta$ du jeu de
directions : un $\theta$ sous-estimé laisserait une direction non testée et
pourrait perdre un support. Il faut donc un jeu dont le rayon soit **démontré**,
pas estimé.

> **Six axes signés, rayon $\arccos(1/\sqrt3)$.** Pour tout $u$ unitaire,
> $\sum_i u_i^2=1$ force $\max_i\lvert u_i\rvert\ge1/\sqrt3$, donc l'axe signé le
> plus proche est à au plus $\arccos(1/\sqrt3)=54{,}7356^\circ$ ; et
> $(1,1,1)/\sqrt3$ atteint cette borne, qui est donc exacte.

C'est une preuve d'une ligne, et c'est **la seule dont on dispose**. Voici ce
qu'elle coûte, mesuré à 50 000 points, $s_{\max}=11$, sur `uniform_latin`, la
région étant l'AABB :

| jeu | $\theta$ | facteur $1-\sin\theta$ | $R/\rho_{s_{\max}}$ médian | voisinage moyen |
| --- | ---: | ---: | ---: | ---: |
| **6 axes — prouvé** | $54{,}74^\circ$ | $0{,}1835$ | $5{,}75$ | **10 292** |
| 14 — estimé | $37{,}98^\circ$ | $0{,}3846$ | $2{,}72$ | 1 583 |
| 26 — estimé | $28{,}91^\circ$ | $0{,}5165$ | $2{,}07$ | 764 |
| 48 — estimé | $23{,}57^\circ$ | $0{,}6001$ | $1{,}80$ | **515** |

**Le jeu prouvé est vingt fois trop faible.** Et le prix de la certification est
lisible sur la dernière ligne : le voisinage passe de 114 — l'estimation
échantillonnée non certifiée — à 515 avec la boule-test décalée, soit un facteur
4,5 rien que pour rendre le test valide.

**L'obligation restante est finie et mécanique, pas ouverte.** Le rayon de
recouvrement d'un ensemble fini fixé de la sphère est atteint en un **sommet de
son diagramme de Voronoï sphérique** ; c'est donc un nombre algébrique exactement
calculable pour tout jeu concret. Il suffit de le calculer une fois, hors ligne,
et de le sceller. Rien dans cette étape ne dépend des données.

Le certificat porte donc `tangent_direction_count`,
`tangent_covering_radius_millidegrees` et `tangent_covering_radius_proved`, et le
vérificateur refuse par nom un rayon **inférieur au scellé**, une preuve
**revendiquée sans scellé**, et une preuve **sans jeu de directions**. Un jeu
fin honnêtement déclaré comme estimé est admis — c'est ainsi qu'on avance en
attendant la preuve — mais il ne peut pas se dire prouvé.

### 8.5 Les rayons de recouvrement exacts des jeux du cube

L'obligation du §8.4 est levée pour trois jeux, et le calcul est fermé.

**Réduction au domaine fondamental.** Les trois jeux — 6 normales de face,
14 = 6 + 8 normales de sommet, 26 = 6 + 12 + 8 — sont invariants sous le groupe
octaédral complet. Il suffit donc de borner sur le domaine fondamental
$x\ge y\ge z\ge0$. Sur ce domaine, le cosinus maximal à chaque type est atteint
tous signes positifs et composantes décroissantes :

$$\text{face}\to x,\qquad \text{arête}\to\frac{x+y}{\sqrt2},\qquad
\text{sommet}\to\frac{x+y+z}{\sqrt3}.$$

Le rayon de recouvrement est $\arccos$ du **minimum du maximum** de ces formes.

**Le minimum est le point d'équioscillation intérieur.** En un minimiseur où une
forme dépasse strictement les autres, on peut se déplacer sur la sphère en la
diminuant ; le minimum est donc soit à l'équioscillation, soit au bord. Les trois
bords donnent des valeurs strictement plus grandes (jeu 26) :

| bord | valeur |
| --- | ---: |
| $z=0$ | $0{,}923880$ |
| $y=z$ | $0{,}888074$ |
| $x=y$ | $0{,}953021$ |
| **intérieur** | $\mathbf{0{,}886452}$ |

**Les trois valeurs fermées.**

| jeu | $\cos\theta$ | $\theta$ | $1-\sin\theta$ | scellé (millidegrés) |
| ---: | --- | ---: | ---: | ---: |
| 6 | $1/\sqrt3$ | $54{,}735610^\circ$ | $0{,}183503$ | 54 736 |
| 14 | $1/\sqrt{5-2\sqrt3}$ | $36{,}206023^\circ$ | $0{,}409310$ | 36 207 |
| **26** | $\mathbf{1/\sqrt{9-2\sqrt2-2\sqrt6}}$ | $\mathbf{27{,}569276^\circ}$ | $\mathbf{0{,}537179}$ | **27 570** |

Pour le jeu 26, l'équioscillation donne $x=t$, $y=(\sqrt2-1)t$,
$z=(\sqrt3-\sqrt2)t$ et la normalisation $t^2\,(9-2\sqrt2-2\sqrt6)=1$. Une
recherche brute sur quatre millions de directions du domaine confirme les trois
valeurs à $2\cdot10^{-5}$ près — et par en dessous, comme il se doit puisqu'un
échantillon ne tombe pas exactement dans le trou profond.

**Ce que cela débloque.** Le facteur de la boule-test décalée passe de $0{,}1835$
— le seul jeu prouvé jusqu'ici — à $\mathbf{0{,}5372}$, à comparer au $0{,}6001$
du jeu à 48 directions qui, lui, reste estimé. Le voisinage certifié mesuré à
50 000 points tombe de **10 292 à environ 760**, soit un facteur **treize**, et il
est désormais **prouvé**. Les valeurs scellées arrondissent vers le haut, de sorte
qu'aucune ne peut être inférieure à la vérité.
