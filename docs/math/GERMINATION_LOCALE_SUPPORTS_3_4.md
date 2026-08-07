# Germination locale certifiée des supports trois et quatre

Objet : remplacer la subdivision globale de produits — dont le coût est mesuré
proportionnel au travail exploré, c'est-à-dire à $\binom n3+\binom n4$ — par une
génération **locale à complétude prouvée**. Le document énonce ce qui est
démontré, ce qui est mesuré, et deux résultats négatifs qui ferment
définitivement deux familles d'approches.

Aucun claim, aucune porte ouverte ou fermée, aucun statut public.

## 1. Cadre et notations

$P\subset\mathbb{R}^3$ fini, $s_{\max}=\min(K+1,n)$ le rang fermé maximal
pertinent. Pour $U\subseteq P$ de cardinal $m\in\{3,4\}$ affinement indépendant,
$c_U$ est le circumcentre, $r_U$ le circumrayon, $\beta(U)=r_U^2$ le niveau
exact déjà porté par le pipeline.

$U$ est **minimal bien centré** ssi toutes les coordonnées barycentriques de
$c_U$ dans $U$ sont strictement positives, c'est-à-dire
$c_U\in\operatorname{relint}\operatorname{conv}(U)$ — de façon équivalente,
$\bar B(c_U,r_U)$ est la miniboule de $U$ et $U$ en est le support minimal.

$U$ est **accepté à l'ordre $K$** ssi il est minimal bien centré, son shell vaut
exactement $U$, et $\lvert P\cap\bar B(c_U,r_U)\rvert\le s_{\max}$.

## 2. Lemme d'angle

> **Lemme 1.** Si $U$ est minimal bien centré, il existe $p,q\in U$ distincts
> avec $\lvert p-q\rvert^2 > 2\,r_U^2$.

*Démonstration.* Écrivons $c_U=\sum_i\lambda_i p_i$ avec $\lambda_i>0$ et
$\sum_i\lambda_i=1$, et posons $v_i=p_i-c_U$, de sorte que $\lvert v_i\rvert=r_U$
et $\sum_i\lambda_i v_i=0$. Fixons $j$. Alors
$0=\bigl\langle\sum_i\lambda_i v_i,\;v_j\bigr\rangle
 =\lambda_j r_U^2+\sum_{i\ne j}\lambda_i\langle v_i,v_j\rangle$.
Comme $\lambda_j r_U^2>0$, il existe $i\ne j$ avec $\langle v_i,v_j\rangle<0$, et
alors $\lvert p_i-p_j\rvert^2=2r_U^2-2\langle v_i,v_j\rangle>2r_U^2$. $\square$

Comme par ailleurs deux points d'une sphère de rayon $r_U$ sont à distance au
plus $2r_U$, on obtient l'**encadrement à deux côtés**

$$r_U\sqrt2\;<\;\operatorname{diam}(U)\;\le\;2\,r_U .$$

Pour $m=3$ la borne inférieure se resserre en $r_U\sqrt3$ : dans un triangle
d'angles tous aigus, le plus grand angle $A$ vérifie $\pi/3\le A<\pi/2$ et le
côté opposé vaut $2r_U\sin A\ge r_U\sqrt3$. La vérification numérique sur
93 837 supports minimaux bien centrés tirés au hasard donne un rapport
$\operatorname{diam}/r$ observé dans $[1{,}691,\;2{,}000]$, compatible avec les
deux bornes et suggérant $\sqrt{8/3}$ comme infimum pour $m=4$ (tétraèdre
régulier) — non utilisé ci-dessous, non démontré.

Ces comparaisons sont exactes sans arithmétique nouvelle : $\operatorname{diam}^2$
et $\beta(U)$ sont déjà des rationnels canoniques du pipeline.

## 3. Premier résultat négatif : la localité seule ne réduit rien

Il est tentant d'attribuer chaque support à un **propriétaire dyadique** : le
niveau $h$ tel que $h/2<\operatorname{diam}(U)\le h$, et la cellule de la grille
de côté $h$ contenant $c_U$. Le Lemme 1 donne alors $h/4<r_U<h/\sqrt2$, donc tout
sommet est à distance $<h$ de $c_U$ : $U$ tient dans le bloc $3\times3\times3$
autour de sa cellule. La complétude est gratuite, puisque le propriétaire est une
fonction de $U$.

**Et c'est exactement pourquoi cela ne peut rien gagner.** Le propriétaire
définit une **partition** de $\binom P3\sqcup\binom P4$ ; énumérer, pour chaque
propriétaire, tous les sous-ensembles qu'il possède refait donc l'univers, terme
pour terme. Un schéma qui organise les sous-ensembles sans en **rejeter des
groupes entiers sans les énumérer** ne peut pas réduire le travail total.

C'est la lecture structurelle de T1 et T2 : le partitionnement de la frontière
est précisément cette réorganisation, et la mesure du 7 août — coût invariant
sous le découpage — est ce théorème vu de l'extérieur.

Mesuré sur le nuage `uniform_latin` de 50 000 points : en n'admettant que les
niveaux compatibles avec le rang ($\bar B(c_U,h/4)\subseteq\bar B(c_U,r_U)$ doit
contenir au plus $s_{\max}$ points, ce qui exclut $h>0{,}15$), le travail du
schéma par propriétaire reste $\approx 1{,}1\cdot10^{15}$ contre un univers de
$2{,}604\cdot10^{17}$ : un gain de **240 fois**, contre les $10^{10}$ nécessaires.

## 4. Second résultat négatif : la germination par paires est fermée

L'idée symétrique — engendrer les supports en étendant les paires d'un catalogue
de rang — est **définitivement réfutée**, et la réfutation est déjà dans le
dépôt. La fixture `hartigan_triangle_all_side_ranks_above_k.json` porte un
support trois de rang fermé trois dont les trois côtés ont rang quatre ; et
`FRONTIERE_DIRECTE_SUPPORTS_3_4.md` §1.1 précise que l'on peut, en plaçant des
points rationnels dans les calottes des boules diamétrales situées hors de la
miniboule, rendre ces trois rangs **arbitrairement grands** sans changer le rang
trois du support.

Aucun élargissement fini $f(K)$ du catalogue de paires ne suffit donc. Ce n'est
pas une conjecture ouverte : c'est clos. La même non-hérédité vaut entre arités
trois et quatre (`tetrahedron_face_filter_counterexamples.json`).

## 5. Le théorème de germination locale

Ce qui reste doit donc être une contrainte portée par **chaque sommet**, dérivée
du rang et non de la longueur. La voici, et elle est immédiate.

> **Théorème.** Soit $U$ accepté à l'ordre $K$ et $p\in U$. Alors
> $\bar B(c_U,r_U)$ est une boule dont le bord passe par $p$, dont le centre
> appartient à $\operatorname{conv}(U)\subseteq\operatorname{conv}(P)$, et qui
> contient au plus $s_{\max}$ points de $P$.
>
> Par conséquent, en posant
> $$R(p)\;=\;\sup\bigl\{\rho>0\;:\;\exists\,c,\;\lvert c-p\rvert=\rho,\;
> c\in\operatorname{conv}(P),\;\lvert P\cap\bar B(c,\rho)\rvert\le s_{\max}\bigr\},$$
> on a $r_U\le R(p)$ pour **tout** sommet $p$ de $U$, donc
> $$\operatorname{diam}(U)\;\le\;2\,r_U\;\le\;2\min_{p\in U}R(p).$$

*Démonstration.* Le circumball lui-même est un témoin admissible pour chacun de
ses points de bord. $\square$

> **Corollaire (générateur complet).** Soit $\tau$ une majoration certifiée,
> $\tau(p)\ge 2R(p)$. Le graphe $G_\tau$ sur $P$ défini par
> $p\sim q\iff\lvert p-q\rvert\le\min(\tau(p),\tau(q))$ contient **toutes** les
> arêtes de tout support accepté. Énumérer les $m$-cliques de $G_\tau$ produit
> donc tous les supports acceptés, chacun exactement une fois si l'on retient la
> clique au plus petit identifiant.

La complétude est **inconditionnelle** ; seul le coût dépend des données et de la
finesse de $\tau$. C'est exactement le partage que le projet assume déjà pour
l'étage paire, et que `FRONTIERE_DIRECTE_SUPPORTS_3_4.md` §7 énonce comme règle :
« sparse » désigne une propriété à mesurer, jamais une conséquence automatique.

### 5.0 La majoration certifiée : ce qui échoue, et ce qui marche

Le corollaire n'est un algorithme que si l'on sait produire un $\tau$ **certifié**.
Les mesures du §5.1 et du §6 utilisent une **minoration échantillonnée** de
$R(p)$ — 48 directions et une dichotomie — qui ne certifie rien. Voici ce que
l'on peut réellement prouver.

> **Lemme 2 (l'emboîtement directionnel n'existe pas).** Pour $u\ne u'$, aucune
> boule tangente en $p$ de direction $u'$ et de rayon $\rho'\le\rho$ n'est
> contenue dans la boule tangente de direction $u$ et de rayon $\rho$, sauf si
> $u=u'$.

*Démonstration.* La condition d'inclusion est
$\sqrt{\rho'^2+\rho^2-2\rho\rho'\cos\theta}\le\rho-\rho'$ avec
$\theta=\angle(u,u')$. En élevant au carré, elle se réduit à
$-2\rho\rho'\cos\theta\le-2\rho\rho'$, c'est-à-dire $\cos\theta\ge1$. $\square$

Un recouvrement directionnel naïf est donc impossible. Il faut décentrer.

> **Lemme 3 (boule-test décalée).** Si $\angle(u,u_i)\le\theta$, la plus grande
> boule incluse dans $\bar B(p+\rho u,\rho)$ et centrée sur le rayon
> $p+t\,u_i$ est obtenue en $t=\rho\cos\theta$ et a pour rayon
> $\rho\,(1-\sin\theta)$.

*Démonstration.* $|p+t u_i-(p+\rho u)|^2=t^2-2t\rho\cos\theta+\rho^2$ est
minimal en $t=\rho\cos\theta$, où il vaut $\rho^2\sin^2\theta$; le rayon
admissible est $\rho$ moins cette distance. $\square$

**Route A, mesurée et écartée.** Le Lemme 3 donne un test certifié : si, pour
toutes les directions d'un recouvrement, la boule-test est sur-peuplée, aucune
direction ne convient. Mais **sans la contrainte de coque il est vide**, et
l'imposer demande un test de vivacité de calotte contre les demi-espaces de
l'enveloppe convexe. Mesuré à 50 000 points sans ce test : voisinage moyen 4 432
à 6 958 et maximum 23 743 à 25 498 selon la finesse — les points de coque
repartent à l'infini, exactement comme au §5.1. Travail $3$ à $5\cdot10^{15}$.
Cette route est abandonnée au profit de la suivante, qui n'a besoin ni de
recouvrement, ni d'enveloppe convexe.

#### Jung donne la constante optimale, et le lemme d'angle devient un corollaire

Le lemme d'angle du §2 majore $r_U$ par $\operatorname{diam}(U)/\sqrt2$. Ce n'est
pas la meilleure borne : **le théorème de Jung** l'est.

> **Théorème (Jung).** Toute partie bornée de $\mathbb{R}^d$ de diamètre $D$ est
> contenue dans une boule de rayon au plus $\gamma_d D$ avec
> $\gamma_d=\sqrt{\tfrac{d}{2(d+1)}}$, soit $\gamma_2=1/\sqrt3$ et
> $\gamma_3=\sqrt{3/8}$. La borne est atteinte par le simplexe régulier.

Il s'applique ici parce qu'un support **minimal bien centré** a précisément pour
miniboule sa boule circonscrite : $c_U$ est dans $\operatorname{conv}(U)$ et tous
les points de $U$ sont à distance $r_U$ de $c_U$, ce qui est exactement la
caractérisation du centre de la miniboule et de son ensemble de support. Donc

$$r_U \;\le\; \gamma_m\,\operatorname{diam}(U),\qquad
\gamma_3=\tfrac1{\sqrt3}\ (\text{triangle, plan}),\quad
\gamma_4=\sqrt{\tfrac38}\ (\text{tétraèdre}).$$

Autrement dit $\operatorname{diam}(U)\ge r_U\sqrt3$ pour $m=3$ et
$\ge r_U\sqrt{8/3}$ pour $m=4$, contre $r_U\sqrt2$ par le lemme d'angle. Jung est
donc strictement meilleur, **et optimal** : la vérification numérique sur 300 000
tirages donne un minimum observé de $1{,}7354$ pour $m=3$ contre la borne
$\sqrt3=1{,}7321$, et $1{,}6709$ pour $m=4$ contre $\sqrt{8/3}=1{,}6330$. Le
lemme d'angle garde sa valeur — il est élémentaire, il vaut en toute dimension et
il ne suppose rien sur $m$ — mais il n'est plus la borne employée.

#### Le lemme d'arête diamétrale, dans sa forme générale

> **Lemme 4 (stabilité du centre de la miniboule).** Soit $S$ borné, de
> miniboule $\bar B(c_{mb},R_{mb})$. Pour toute boule $\bar B(c,r)\supseteq S$,
> $$\lvert c-c_{mb}\rvert^2\;\le\;r^2-R_{mb}^2 .$$

*Démonstration.* Le centre de la miniboule s'écrit $c_{mb}=\sum_i\lambda_i s_i$
avec $\lambda_i>0$, $\sum\lambda_i=1$ et $\lvert s_i-c_{mb}\rvert=R_{mb}$. Alors
$\sum_i\lambda_i\lvert s_i-c\rvert^2
 =\sum_i\lambda_i\bigl(R_{mb}^2+2\langle s_i-c_{mb},\,c_{mb}-c\rangle
   +\lvert c_{mb}-c\rvert^2\bigr)
 =R_{mb}^2+\lvert c_{mb}-c\rvert^2$,
le terme croisé s'annulant car $\sum_i\lambda_i(s_i-c_{mb})=0$. Chaque
$\lvert s_i-c\rvert\le r$, donc le membre de gauche est au plus $r^2$. $\square$

> **Corollaire (test certifié, forme générale).** Soit $U$ accepté, $(p,q)$ sa
> paire diamétrale, $D=\lvert p-q\rvert$, et $S\subseteq U$ **connu**. Alors
> $$\bar B\Bigl(c_{mb}(S),\;\gamma_m D-\sqrt{\gamma_m^2D^2-R_{mb}(S)^2}\Bigr)
>   \;\subseteq\;\bar B(c_U,r_U),$$
> et cette boule contient donc au plus $s_{\max}$ points. Le rayon est
> **croissant en $R_{mb}(S)$** : le test se renforce à chaque sommet ajouté.

En effet $r_U\le\gamma_m D$ par Jung et $r_U\ge R_{mb}(S)$ par monotonie de la
miniboule ; la fonction $r\mapsto r-\sqrt{r^2-R_{mb}^2}$ est décroissante, donc
son minimum sur l'intervalle admissible est atteint en $r=\gamma_m D$.

**Le cas $S=\{p,q\}$** — le germe — donne $c_{mb}=M$, $R_{mb}=D/2$ et la
constante fermée

$$\kappa_m=\gamma_m-\sqrt{\gamma_m^2-\tfrac14},\qquad
\kappa_3=\tfrac1{2\sqrt3}=0{,}288675,\qquad
\kappa_4=\sin 15^\circ=0{,}258819 .$$

> **Test de germe.** Si
> $\lvert P\cap\bar B(M,\;\kappa_m\lvert p-q\rvert)\rvert>s_{\max}$, alors
> $(p,q)$ n'est la paire diamétrale d'**aucun** support accepté de taille $m$.

Une seule requête de comptage par arête. Ni recouvrement directionnel, ni
enveloppe convexe, ni recherche non bornée : c'est Jung qui borne $r_U$ par le
haut, et la stabilité du centre qui transforme cette borne en une boule
explicitement inscrite.

**Mesuré à 50 000 points, $s_{\max}=11$** :

| constante | degré moyen | degré max | travail | gain |
| --- | ---: | ---: | ---: | ---: |
| lemme d'angle, $\kappa=0{,}2071$ | 1 010,4 | 1 252 | $2{,}49\cdot10^{12}$ | $1{,}05\cdot10^{5}$ |
| Jung $m=4$, $\kappa_4=0{,}2588$ | 546,4 | 646 | $3{,}81\cdot10^{11}$ | $6{,}83\cdot10^{5}$ |
| Jung $m=3$, $\kappa_3=0{,}2887$ | 394,8 | 456 | $1{,}41\cdot10^{11}$ | $1{,}85\cdot10^{6}$ |

Avec la constante propre à chaque arité, le travail certifié du germe vaut
$1{,}34\cdot10^{9}$ pour les triples et $3{,}79\cdot10^{11}$ pour les quadruples,
soit $\mathbf{3{,}80\cdot10^{11}}$ et un gain de $\mathbf{6{,}85\cdot10^{5}}$ —
contre $1{,}08\cdot10^{5}$ avec le lemme d'angle seul. **Jung vaut un facteur
6,5, gratuitement.**

**Correction honnête, maintenue.** Le chiffre de $5{,}24\cdot10^{9}$ candidats
annoncé au §6 repose sur la minoration échantillonnée de $R(p)$ et **n'est pas
certifié**. Le générateur certifié coûte aujourd'hui $3{,}80\cdot10^{11}$. Le
resserrement restant ne passe plus par une meilleure constante de germe — Jung
est optimale — mais par l'**élagage incrémentiel** que le corollaire général
autorise : à chaque sommet ajouté, $R_{mb}(S)$ croît et la boule-test grossit.
C'est là que se joue le travail des quadruples, qui domine tout le reste.

### 5.1 La contrainte de coque n'est pas cosmétique

Sans la condition $c\in\operatorname{conv}(P)$, $R(p)=+\infty$ pour tout point de
la coque : une boule tangente en $p$ peut croître indéfiniment vers l'extérieur
en restant vide. Mesuré sur `uniform_latin` à 50 000 points, $s_{\max}=11$, avec
48 directions et dichotomie sur le rayon :

| | sans contrainte | centre dans $\operatorname{conv}(P)$ |
| --- | ---: | ---: |
| $R(p)/\rho_{s_{\max}}(p)$ médian | 1,073 | 1,073 |
| p99 | 12,17 | **1,376** |
| max | 14,62 | **1,466** |
| $\lvert P\cap B(p,2R)\rvert$ moyen | 2 346 | **114** |
| max | 48 701 | **166** |
| travail total | $5{,}5\cdot10^{15}$ | $3{,}2\cdot10^{9}$ |

où $\rho_s(p)$ est le rayon de la boule **centrée** en $p$ contenant $s$ points.
Six ordres de grandeur séparent les deux lignes, et ils tiennent à une condition
que le théorème donnait gratuitement.

## 6. Coût mesuré du générateur à 50 000 points

Sur le même nuage, avec la majoration uniforme conservatrice
$\tau(p)=2\cdot1{,}466\cdot\rho_{s_{\max}}(p)$ — le facteur maximal observé,
appliqué à tous les points — le graphe $G_\tau$ a un degré moyen de 236 et

| grandeur | valeur |
| --- | ---: |
| triangles à examiner | $2{,}33\cdot10^{8}$ |
| quadruples à examiner | $5{,}00\cdot10^{9}$ |
| **travail total du générateur** | $\mathbf{5{,}24\cdot10^{9}}$ |
| univers $\binom n3+\binom n4$ | $2{,}604\cdot10^{17}$ |
| **gain** | $\mathbf{5{,}0\cdot10^{7}}$ |
| sortie utile estimée | $1{,}8\cdot10^{7}$ |
| candidats examinés par record émis | 291 |

Une majoration **par point** au lieu du facteur uniforme ramène le degré de 236 à
environ 114, donc les quadruples d'un facteur $\approx 8$ : le travail tombe vers
$6\cdot10^{8}$, soit environ 33 candidats par record. C'est l'écart entre une
borne grossière et une borne serrée, pas entre deux algorithmes.

**Honnêteté de la mesure.** Les 48 directions échantillonnées donnent une
**minoration** de $R(p)$, donc une **minoration** du travail : un $\tau$ certifié
sera un peu plus grand. L'application du facteur maximal observé à tous les
points compense partiellement, mais la construction d'une majoration certifiée de
$R(p)$ — sans laquelle la complétude n'est pas acquise en pratique — reste à
faire, et c'est le premier travail d'implémentation.

## 7. Compatibilité avec le registre des pistes abandonnées

| interdit | statut ici |
| --- | --- |
| fenêtre Morton fixe ou préfixe fini de voisins comme autorité | **respecté** : $\tau(p)$ est une quantité locale certifiée par le rang, pas un rayon universel ; aucune liste de voisins n'a de pouvoir d'exclusion propre |
| Delaunay, mosaïque d'ordre supérieur, catalogue cellulaire dans le chemin produit | **respecté** : aucune triangulation, aucune cellule, aucune incidence matérialisée |
| fermeture depuis les seules paires de rang utile | **respecté**, et §4 explique pourquoi cette voie est close |
| héritage de filtre entre arités trois et quatre | **respecté** : les $m$-cliques sont énumérées indépendamment par arité |
| tour globale de boules saturées énumérée exhaustivement | **respecté** : aucune énumération globale |

La règle de réouverture exige en outre un théorème de complétude neuf (§5), une
fixture falsifiant le motif d'abandon sans casser les contre-exemples existants
(les deux contre-exemples de §4 restent valides et sont **utilisés**, pas
contournés), aucune structure globale interdite, et une porte de performance
distincte (§8).

## 8. Ce que cela donne contre les contrats à 50 000 points

Le contrat de la porte de sortie de la phase 14 est : $n=50\,000$, $K_{\max}=10$,
p95 `warm_e2e` sous 100 ms, objectif secondaire sous 1 s.

Le coût unitaire d'un candidat est **mesuré**, et il ne faut pas l'emprunter à
l'étage paire. Le recensement exhaustif donne 34,5 à 96,9 µs par support sur un
cœur, et sa ligne `uniform_latin` arité quatre — 43,0 µs par support pour **une
seule** requête de boule fermée sur 10 668 000 supports — isole le coût de la
**porte géométrique seule** : circumcentre entier et signes barycentriques. La
requête de boule, quand elle a lieu, coûte 26,4 à 34,4 visites de nœud.

Avec $6\cdot10^{8}$ candidats (majoration serrée) et $4\cdot10^{-5}$ s par
candidat, l'étage higher coûte de l'ordre de $2{,}5\cdot10^{4}$ s sur un cœur.
L'énoncé honnête est donc :

- le passage de $2{,}604\cdot10^{17}$ à $\sim10^{9}$ est **acquis par un
  théorème**, et c'est huit ordres de grandeur ;
- il reste **deux à trois ordres de grandeur de coût unitaire** pour le contrat
  d'une seconde, davantage pour celui de 100 ms ;
- ce résidu est un problème de coût par opération, pas de complexité, et son
  levier principal est identifié.

### 8.1 Le filtre fp64 : hypothèse construite, mesurée, et réfutée

Le moteur higher device ne contenait **aucun flottant** — zéro occurrence de
`double` ou `float` dans les 2 159 lignes de
`phase15_higher_support_device_tiled_slot_engine.cuh` — alors que l'étage paire
filtre **97,8 %** de ses prédicats exacts en fp64. L'hypothèse naturelle était
que ce filtre manquant expliquait une grande part de l'écart de débit entre les
deux étages. Elle a été implémentée
(`phase15_higher_support_product_fp64_filter.cuh`, barreau fp64 devant l'échelle
entière aux deux portes de produit) et **elle est fausse**.

Le filtre est correct : il ne répond que lorsque son intervalle à arrondi
extérieur est strictement signé, ce qui prouve le même signe pour l'intervalle
exact, et défère partout ailleurs. Les treize suites `higher_support` passent
sans qu'un seul verdict bouge, fixture permanente des coins de boîte comprise.
Mais l'A/B sur la suite de frontière hôte, trois exécutions de chaque côté, donne
23,74 / 24,54 / 24,72 s avec le filtre contre 24,33 / 25,07 / 25,65 s sans :
**environ 2 %, c'est-à-dire du bruit.**

La raison est structurelle et vaut d'être écrite. Un filtre fp64 est rentable
contre une arithmétique **rationnelle non bornée** — exactement le repli de
l'étage paire (`prepared_witness_aabb_minimum_sign` en BigInt), où il filtre
97,8 %. Le repli du moteur higher est un intervalle **entier de largeur fixe à
quatre membres**, déjà bon marché ; et la discipline d'arrondi extérieur coûte un
`nextafter` par opération élémentaire, du même ordre. L'écart de débit de 894×
entre les deux étages **n'est donc pas expliqué** par l'absence de filtre fp64.

Le code est conservé, étiqueté pour ce qu'il est : sûr, neutre en verdict, et
**non justifié par une mesure**. Sa valeur éventuelle est propre au device — la
structure de coût de l'int256 sur GPU n'est pas celle de l'hôte — et seule une
session native peut la trancher.

### 8.2 Ce que la réfutation laisse

Deux leviers restent, tous deux jamais mesurés : le lancement est à un thread par
slot avec `kThreadsPerBlock = 128` et un nombre de slots plafonné à 1 024 par une
constante de schéma que ni T1 ni T2 ne touchent — aucune mesure du dépôt n'a fait
varier le nombre de threads explorateurs — et la coopération intra-warp sur le
plan de sonde (au plus 91 positions, 182 évaluations) est explicitement différée
dans l'en-tête du `.cu`.

Et surtout, l'A/B ci-dessus soutient une hypothèse plus forte : **les ~40 µs par
support ne seraient pas de l'arithmétique du tout.** Retirer du travail
arithmétique aux portes n'a rien déplacé. Si le coût est dans l'aller-retour de
lancement et de drainage plutôt que dans les portes, le correctif est entièrement
différent, et aucune mesure hôte ne peut le décider : il faut un profil natif,
sur une session G4, avec le nombre de lancements et le temps par lancement
publiés. C'est la première chose à instrumenter avant d'écrire quoi que ce soit
d'autre sur cet étage.

### 8.2 Pourquoi la porte géométrique ne peut pas suffire

Le recensement établit que la fraction de supports bien centrés est
**constante en $n$** : 28,0 à 28,9 % des triples et 9,7 à 11,3 % des quadruples,
à $n = 32$, 64 et 128. À 50 000 points cela ferait de l'ordre de
$5{,}8\cdot10^{12}$ triples et $2{,}5\cdot10^{16}$ quadruples bien centrés. La
porte de bon centrage — exactement ce que certifient $\Delta$ et les numérateurs
barycentriques — **ne peut donc jamais**, à elle seule, donner la sensibilité à
la sortie. Seul le rang fermé la donne.

C'est précisément ce que fait le théorème du §5 : $\tau$ dérive du rang, pas de
la géométrie, et le générateur énumère $5{,}2\cdot10^{9}$ cliques au lieu des
$2{,}5\cdot10^{16}$ supports bien centrés. La contrainte de rang est appliquée
**sans énumérer l'ensemble bien centré**, ce qui était l'exigence.

### 8.3 Ce que ces chiffres ne couvrent pas

L'aval entier — façade, journaux, `batch_plan`, reducer, forêt, pipeline
vertical, archive durable 15L, API publique — est absent de toute estimation. La
seule mesure existante vaut **18 923,7 ms à $n=32$ pour 171 événements**, et
l'entrée de l'aval passerait à $1{,}8\cdot10^{7}$ records. Aucune mesure n'existe
entre $n=4$ et $n=32$.

Le contrat de 100 ms exige en outre, indépendamment de cet étage, un service
résident chaud — la seule création du contexte CUDA coûte 1,242 s à froid — et le
portage device de la canonicalisation et du LBVH, qui consomment 18,5 ms sur CPU.
Enfin, aucun instrument du dépôt ne peut produire le nombre contractuel :
`warm_e2e_protocol_executed` est un littéral `false`, `p95_ms` un littéral
`null`, et le runner ne porte aucune instrumentation mémoire alors que la porte
exige un pic sous 80 % de la VRAM.

## 9. Route d'implémentation

1. **Majoration certifiée $\tau(p)\ge 2R(p)$.** C'est la seule pièce sans
   laquelle la complétude n'est pas acquise. Les cônes de Yao-48 et la structure
   d'ancres de l'étage paire calculent déjà des objets de cette nature ; la
   contrainte de coque se réduit à un jeu de demi-espaces exacts.
2. **Construction de $G_\tau$** par requêtes de boule sur le LBVH existant, sans
   arithmétique exacte : c'est une relation de distance.
3. **Énumération des $m$-cliques** par tuiles device, sur le modèle du chemin
   tuile-certifié déjà en place — la chaîne ancrée, ses invariants et sa clôture
   BigInt sont réutilisés tels quels, seule la source des candidats change.
4. **Classification terminale inchangée** :
   `analyze_circumcenter_support_integer` puis
   `ExactHigherSupportIndexedClosedBallQuery::classify`, les deux primitives que
   les deux bases de vérification partagent déjà.
5. **Différentiel obligatoire** contre le chemin exhaustif sur un petit nuage,
   et sur la famille `eight_clusters` — `uniform_latin` ne contient aucun
   quadruple minimal bien centré et n'exerce donc pas l'arité quatre.

## 10. La comptabilité, refondée sur la complétude (décision du 7 août 2026)

L'identité de clôture $R_j+C(F_j)=\binom n3+\binom n4$ ne survit pas à ce
changement : le générateur n'énumère plus l'univers, donc la masse non examinée
n'est plus « résolue par élagage » mais « exclue par le théorème ». Deux options
existaient — refonder la comptabilité, ou conserver l'identité en comptant
explicitement la masse exclue comme une troisième catégorie munie de son propre
certificat.

**La décision est prise : refonder.** La seconde option reconstruirait une
comptabilité d'univers pour un algorithme qui a précisément cessé de le
parcourir, et son certificat de complétude serait de toute façon le véritable
porteur de la preuve ; on paierait un objet de preuve supplémentaire pour une
identité devenue décorative.

Ce que la chaîne ancrée certifiera désormais n'est plus « toute la masse a été
partitionnée » mais **« tout support accepté a été produit »**, adossé à trois
choses et à elles seules :

1. le **théorème** du §5 — tout support accepté a ses sommets dans le graphe
   engendré par $\tau$ ;
2. la **certification de $\tau$** — la majoration employée est prouvée
   supérieure ou égale à $2R(p)$, ou, dans la route retenue au §5.0, le rejet
   d'arête est prouvé par le Lemme 4 ;
3. la **classification terminale exacte** inchangée, qui décide chaque candidat
   produit.

Trois conséquences de contrat, à traiter comme telles :

- le champ `verification_basis` du certificat gagne une valeur, distincte de
  `fresh_cpu_replay_every_commit` et de
  `tile_certified_engine_with_exact_closure`, qui nomme la base de complétude et
  la constante certifiée employée ;
- la validation croisée BigInt du pont ne peut plus vérifier une partition de
  masse ; elle vérifie que le graphe engendré et le rejet d'arête sont ceux que
  la constante déclarée impose, ce qui est une identité de **production**, pas de
  **couverture** ;
- l'induction de reprise demeure : le successeur reste une fonction déterministe
  du checkpoint de confiance, seule la quantité conservée change.

Cette refondation est le point de contrat le plus délicat de la route, et elle
doit être écrite **avant** le générateur, pas après.
