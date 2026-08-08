# MorseHGP3D v2 — conception

> Statut : document de conception. Il fixe le modèle de calcul, les théorèmes
> employés, les bornes et le budget. Il ne revendique aucun statut public tant
> que les portes de validation du §10 ne sont pas franchies.

## 0. Ce qu'il faut calculer, en une page

Soit $X\subset\mathbb{R}^{3}$ fini, $n=\lvert X\rvert$, et $K\in\mathbb{N}^{*}$.
L'estimateur de densité des $K$ plus proches voisins de HARTIGAN est gouverné par

$$d_K(y)=\min\left\lbrace \rho\geq0\ :\ \left\lvert X\cap\bar B(y,\rho)\right\rvert\geq K\right\rbrace,$$

et les *amas de forte densité* au niveau $r$ sont les composantes connexes du
sur-niveau $L_K(r)=\left\lbrace y\in\mathbb{R}^{3}:d_K(y)\leq r\right\rbrace$.
Le théorème 2 du manuscrit identifie ces composantes aux $K$-polyèdres du
complexe de ČECH, et l'amas *discret* associé est
$C^{\mathrm{discret}}=X\cap\delta_r(C)$.

**La sortie contractuelle est donc la forêt de fusion de la famille
$r\mapsto\pi_0\left(L_K(r)\right)$, plus la couverture de chaque composante.**

Or $L_K(r)$ est exactement le sous-niveau $\left\lbrace d_K\leq r\right\rbrace$
d'une fonction $d_K:\mathbb{R}^{3}\to\mathbb{R}_{+}$. **La forêt demandée est
donc le *merge tree* de $d_K$.** C'est la reformulation qui structure toute
cette version : on ne calcule ni le complexe de ČECH, ni le graphe $\Gamma_K$
des facettes, ni un $K$-arbre couvrant sur les $(K-1)$-simplexes. On calcule les
points critiques d'indice $0$ et $1$ d'une fonction scalaire de $\mathbb{R}^{3}$,
et on les recolle.

## 1. Diagnostic de la v1

La v1 suit littéralement la chaîne du manuscrit :
complexe de ČECH $\to$ simplexes $K$-séparants $\to$ $K$-graphe de GABRIEL $\to$
$K$-arbre couvrant. Ses objets de base sont donc des **facettes de cardinal $K$**
et des **cofaces de cardinal $K+1$**. Trois conséquences, toutes mesurées dans le
dépôt :

1. **L'univers combinatoire est faux d'échelle.** À $n=50\,000$ et $K=10$, le
   nombre de cofaces candidates est $\binom{n}{3}+\binom{n}{4}=2{,}604\cdot10^{17}$
   (`CONTRAT_50K_BILAN.md` §3). Toutes les optimisations documentées réduisent ce
   chiffre à $4{,}4\cdot10^{8}$ candidats pour $1{,}8\cdot10^{7}$ records utiles,
   soit encore **24,4 candidats examinés par record émis**.
2. **La réduction sparse promise par le manuscrit est fausse en général.** La
   fixture `gabriel-point-set-counterexample-5-points-v1` réfute la proposition 6
   et la forme élaguée du théorème 5 : le graphe de GABRIEL oublie des *sommets*
   (facettes nées au niveau exact d'une coface non-GABRIEL) qui appartiennent
   pourtant à la composante de $\Gamma_K$, et cet oubli devient visible plus tard.
   La v1 a donc dû se rabattre sur `gamma_exhaustive_reference`, c'est-à-dire sur
   l'univers complet.
3. **Aucune source locale certifiée n'existe pour les supports.** Le théorème 1 de
   `RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md` montre qu'aucun RNG d'ordre fixé,
   même épaissi par JUNG, ne contient les arêtes des supports ; et
   `hartigan_triangle_all_side_ranks_above_k.json` montre qu'un support de rang
   trois peut avoir ses trois côtés de rang arbitrairement grand. La germination
   par paires est donc close.

Le point commun de ces trois murs : **ils sont des propriétés du graphe des
facettes, pas de l'objet géométrique.** Le graphe des facettes est un mauvais
support de calcul parce que le nombre de facettes de cardinal $K$ contenues dans
une boule de rayon $r$ est $\binom{\lvert X\cap\bar B\rvert}{K}$, alors que
l'information topologique portée par cette boule tient en un seul objet.

La v2 change donc d'objet de base. Elle ne manipule jamais de facette.

## 2. L'objet de base : la sphère critique

> **Définition 1 (sphère critique).** Un couple $(U,I)$ avec $U,I\subseteq X$
> disjoints est une *sphère critique* lorsque :
> - $U$ est affinement indépendant, $1\leq\lvert U\rvert\leq4$ ;
> - la plus petite boule fermée $\bar B(c,r)$ contenant $U$ vérifie
>   $U\subseteq\partial B(c,r)$ et $c\in\mathrm{relint}\,\mathrm{conv}(U)$
>   (autrement dit $U$ est le *support minimal* de sa propre miniboule) ;
> - $I=X\cap \mathring B(c,r)$ et $X\cap\partial B(c,r)=U$.
>
> Son *rang fermé* est $s=\lvert I\rvert+\lvert U\rvert=\left\lvert X\cap\bar B(c,r)\right\rvert$,
> et son *niveau* est $\beta=r^{2}$.

Autrement dit : une boule fermée dont le contenu est exactement $I\sqcup U$ et
qui est la plus petite boule contenant ce contenu. La condition
$c\in\mathrm{relint}\,\mathrm{conv}(U)$ est la condition de minimalité (FAIT 12
du manuscrit : le centre de la miniboule appartient à l'enveloppe convexe de son
ensemble de support).

Le lien avec la topologie est le théorème de Morse de REANI–BOBROWSKI pour la
fonction $d_k$ :

> **Fait 1 (points critiques de $d_k$).** Les points critiques de $d_k$ sont
> exactement les centres des sphères critiques $(U,I)$ vérifiant
> $\lvert I\rvert<k\leq\lvert I\rvert+\lvert U\rvert$. Une sphère critique de rang
> fermé $s$ est, pour $\mu=s-k$, un point critique d'indice $\mu$ de $d_k$, de
> multiplicité locale $\binom{\lvert U\rvert-1}{\mu}$.

Deux corollaires, qui sont tout le programme :

- **$\mu=0$, soit $k=s$.** Une sphère critique de rang fermé $k$ est un
  **minimum local** de $d_k$ : une *naissance* de composante, de multiplicité 1.
- **$\mu=1$, soit $k=s-1$.** Une sphère critique de rang fermé $k+1$ est un point
  critique **d'indice un** de $d_k$, de multiplicité $\lvert U\rvert-1$ : une
  *fusion* potentielle, qui peut tuer jusqu'à $\lvert U\rvert-1$ classes.

> **Conséquence.** Pour calculer la forêt de fusion à l'ordre $K$, **il suffit de
> connaître les sphères critiques de rang fermé $K$ et $K+1$.** Pour la tour
> complète $k=1,\dots,K$, il suffit de connaître celles de rang fermé
> $\leq K+1$ — un catalogue unique sert tous les ordres.

C'est là que la v2 gagne son premier ordre de grandeur : le catalogue est
un objet de taille $\Theta(\text{sortie})$, pas $\Theta\left(\binom{n}{K}\right)$.

### 2.1 Ce que le contre-exemple à cinq points ne touche pas

Le contre-exemple réfute une réduction *du graphe des facettes* : il montre que
le sous-graphe de GABRIEL n'a pas les mêmes composantes que $\Gamma_K$. Il ne dit
rien de la théorie de Morse de $d_K$, qui est une propriété de la fonction
elle-même et non d'un encodage combinatoire. La v2 n'hérite donc pas de cette
obstruction : elle ne construit ni $\Gamma_K$, ni son sous-graphe de GABRIEL.

Les « incidences silencieuses » ont d'ailleurs une lecture immédiate dans le
nouveau modèle : une coface non-GABRIEL est un point **non critique** de $d_K$,
donc un point où la topologie des sous-niveaux ne change pas. Elle ne peut ni
créer ni tuer de composante ; le seul effet qu'elle avait dans la v1 était de
faire entrer un sommet dans un graphe qui n'aurait pas dû avoir de sommets.

## 3. Trouver le catalogue : la dualité inversive

Le problème est maintenant : énumérer toutes les sphères critiques de rang fermé
$\leq s_{\max}=K+1$, exactement, et vite.

Fixons $p\in X$ et cherchons les sphères critiques dont $p$ est un point du
support ($p\in U$). Posons l'**inversion de centre $p$**

$$\iota_p(z)=\frac{z-p}{\left\Vert z-p\right\Vert^{2}},\qquad z\in X\setminus\lbrace p\rbrace .$$

> **Théorème 2 (dualité).** L'application $\bar B(c,r)\mapsto\nu=2\,(c-p)$
> est une bijection entre les boules fermées dont la frontière passe par $p$ et
> les vecteurs $\nu\neq0$, avec $r=\left\Vert\nu\right\Vert/2$. Sous cette
> bijection, pour tout $z\neq p$ :
>
> $$z\in\mathring B(c,r)\iff\left\langle \iota_p(z),\nu\right\rangle>1,\qquad z\in\partial B(c,r)\iff\left\langle \iota_p(z),\nu\right\rangle=1 .$$

*Preuve.* $\left\Vert z-c\right\Vert^{2}-r^{2}=\left\Vert z-p\right\Vert^{2}-2\left\langle z-p,c-p\right\rangle$
puisque $\left\Vert c-p\right\Vert=r$. En divisant par $\left\Vert z-p\right\Vert^{2}>0$
on obtient $1-\left\langle \iota_p(z),\nu\right\rangle$. $\square$

Notons $H_z=\left\lbrace \nu:\left\langle \iota_p(z),\nu\right\rangle=1\right\rbrace$
le **plan dual** de $z$, et
$\Lambda(\nu)=\#\left\lbrace z:\left\langle \iota_p(z),\nu\right\rangle>1\right\rbrace$
le **niveau** de $\nu$ dans l'arrangement $\left\lbrace H_z\right\rbrace_{z\in X\setminus p}$.

> **Théorème 3 (caractérisation duale des sphères critiques).** Les sphères
> critiques $(U,I)$ avec $p\in U$ et de rang fermé $s$ correspondent
> bijectivement aux $\nu\neq0$ tels que :
> 1. $\Lambda(\nu)=s-\lvert U\rvert$, où $\lvert U\rvert-1$ est le nombre de plans
>    duaux contenant $\nu$ ;
> 2. la projection orthogonale de l'origine sur l'enveloppe affine de
>    $A_U=\left\lbrace \iota_p(u):u\in U\setminus\lbrace p\rbrace\right\rbrace$
>    appartient à l'intérieur relatif de son enveloppe convexe, avec des
>    coefficients convexes $\left(\nu_u\right)$ ;
> 3. **et de plus** $\dfrac{\left\Vert\nu\right\Vert^{2}}{2}\sum_u\nu_u\left\Vert \iota_p(u)\right\Vert^{2}<1$.
>
> Le rayon vaut $r=\left\Vert\nu\right\Vert/2$.

La condition 3 est **indispensable** et son omission est l'objet du §3 de
`WARNING_AUDIT_COMPLETUDE.md`. Elle exprime exactement $\lambda_p>0$, où
$\lambda_p$ est la coordonnée barycentrique de $p$ dans
$c=\sum_{u\in U}\lambda_u u$. En effet la preuve ci-dessus donne
$\lambda_u=\nu_u\left\Vert\nu\right\Vert^{2}\left\Vert \iota_p(u)\right\Vert^{2}/2$
pour $u\neq p$, et $\lambda_p=1-\sum_{u\neq p}\lambda_u$. Sur le contre-exemple
$p=(0,0,0)$, $u=(1,0,0)$, $v=(0,1,0)$, on trouve $\nu=(1,1,0)$ et
$\frac{\left\Vert\nu\right\Vert^{2}}{2}\sum\nu_u\left\Vert \iota_p(u)\right\Vert^{2}=1$ :
la condition 3 est violée, $\lambda_p=0$, et le support minimal est bien
$\lbrace u,v\rbrace$ et non $\lbrace p,u,v\rbrace$.

**L'implémentation ne dépend pas de cette caractérisation duale** : elle teste
directement les coordonnées barycentriques dans le primal
(`well_centered3` / `well_centered4`), qui incluent $\lambda_p$ par
construction. Le théorème 3 sert de guide géométrique et de justification du
schéma de peeling, pas de prédicat.

*Preuve.* Le point 1 est le théorème 2. Pour le point 2, écrivons
$c=\sum_{u\in U}\lambda_u u$ avec $\lambda_u>0$ et $\sum\lambda_u=1$. Alors
$\nu=2(c-p)=2\sum_{u\neq p}\lambda_u(u-p)=\sum_{u\neq p}\mu_u\,\iota_p(u)$ avec
$\mu_u=2\lambda_u\left\Vert u-p\right\Vert^{2}>0$, puisque
$u-p=\iota_p(u)/\left\Vert \iota_p(u)\right\Vert^{2}$. En notant
$t=\left\langle \iota_p(u),\nu\right\rangle=1$ pour tout $u\in U\setminus\lbrace p\rbrace$,
on a $\left\Vert\nu\right\Vert^{2}=\sum_u\mu_u$, donc les coefficients
$\nu_u=\mu_u/\left\Vert\nu\right\Vert^{2}$ sont positifs de somme 1 et
$\nu/\left\Vert\nu\right\Vert^{2}=\sum_u\nu_u\,\iota_p(u)$ : le pied de la
perpendiculaire abaissée de l'origine sur le plan est bien une combinaison
convexe stricte des $\iota_p(u)$. La réciproque se lit dans le même calcul. Enfin
$\nu/\left\Vert\nu\right\Vert^{2}$ est le point du plan le plus proche de
l'origine, donc $\left\Vert\nu\right\Vert=1/\mathrm{dist}(0,H)$ est minimal
exactement quand le plan est le plus loin possible, ce qui est la condition de
minimalité locale. $\square$

**Lecture.** Pour chaque point $p$, le catalogue local est le **$\leq(s_{\max}-1)$-level
de l'arrangement de $n-1$ plans duaux**, avec pour chaque cellule le point le plus
proche de l'origine. Pour $\lvert U\rvert=4$ ce sont les **sommets** du niveau,
pour $\lvert U\rvert=3$ les **arêtes**, pour $\lvert U\rvert=2$ les **faces**, et
$\lvert U\rvert=1$ est le cas dégénéré $r=0$ traité à part.

Le niveau $0$ est l'enveloppe convexe polaire : ses sommets sont exactement les
tétraèdres de DELAUNAY incidents à $p$. Monter d'un niveau, c'est **peler** le
polytope. La v2 est donc un *peeling convexe local dans le dual inversif*.

### 3.1 Le critère d'arrêt

> [!CAUTION]
> **Version réfutée.** Une première rédaction affirmait qu'il suffisait de
> constater $2R_\rho\leq\rho$, où $R_\rho$ est le plus grand rayon **trouvé**
> dans $W_\rho$. C'est faux : le §1 de `WARNING_AUDIT_COMPLETUDE.md` construit un
> nuage où la croissance s'arrête avant d'avoir vu une paire de GABRIEL de très
> grand rayon. L'argument était circulaire — la maximalité dans le catalogue
> tronqué ne borne pas les rayons du catalogue global. Le critère ci-dessous le
> remplace ; il est *a priori*.

Le bon objet est une majoration du rayon **avant** de l'avoir observé. Pour une
direction unitaire $\hat e$ et un point $z\neq p$, posons
$\pi_{\hat e}(z)=\left\Vert z-p\right\Vert^{2}/\left(2\left\langle z-p,\hat e\right\rangle\right)$
lorsque $\left\langle z-p,\hat e\right\rangle>0$, et $+\infty$ sinon. Par le
théorème 2, $z$ appartient à la boule fermée tangente en $p$ de direction
$\hat e$ et de rayon $r$ **si et seulement si** $\pi_{\hat e}(z)\leq r$.

> **Théorème 4 (majoration a priori).** Soit $(U,I)$ une sphère critique de rang
> fermé au plus $s_{\max}$ avec $p\in U$, de direction $\hat e$ et de rayon $r$.
> Alors $r$ est strictement inférieur à la $s_{\max}$-ième plus petite valeur de
> $\pi_{\hat e}$ sur $X\setminus\lbrace p\rbrace$.

*Preuve.* Sinon $s_{\max}$ points distincts de $p$ vérifieraient
$\pi_{\hat e}\leq r$, appartiendraient donc à $\bar B(c,r)$, et le rang fermé
vaudrait au moins $s_{\max}+1$. $\square$

Pour rendre la borne calculable sur un cône, on minore le dénominateur : si
$\angle(\hat e,\hat u)\leq\theta$ alors
$\left\langle z-p,\hat e\right\rangle\geq\left\langle z-p,\hat u\right\rangle\cos\theta-\left\Vert z-p\right\Vert\sin\theta$,
d'où $\pi_{\hat e}(z)\leq\tilde\pi_{\hat u}(z)$ avec ce dénominateur minoré. On
note $\tau_{\hat u}$ la $s_{\max}$-ième plus petite valeur de $\tilde\pi_{\hat u}$
et $\tau=\max_{\hat u}\tau_{\hat u}$ sur un recouvrement de $\mathbb{S}^{2}$ par
des cônes de demi-angle $\theta$.

> **Corollaire (procédure complète et terminante).** Comme
> $\tilde\pi_{\hat u}(z)\geq\left\Vert z-p\right\Vert/2$, calculer $\tau$ sur
> $W_\rho$ au lieu de $X$ ne peut que **l'augmenter** — moins de candidats, donc
> statistique d'ordre plus grande, éventuellement $+\infty$. Si le $\tau$ ainsi
> obtenu vérifie $2\tau\leq\rho$, alors toute sphère critique tangente en $p$ a un
> rayon $r<\tau\leq\rho/2$, donc
> $\bar B(c,r)\subseteq\bar B(p,2r)\subseteq\bar B(p,\rho)$ : le catalogue local
> calculé sur $W_\rho$ est complet et son comptage de rang exact. Sinon on pose
> $\rho\leftarrow\max(2\rho,\lceil2\tau\rceil)$ et l'on recommence ; la suite
> termine au plus tard lorsque $\rho$ atteint le diamètre, cas où $W_\rho=X$ et le
> calcul est exact par épuisement.

Sur le contre-exemple de l'audit, le cône orienté vers $z$ ne contient aucun
point de dénominateur positif dans $W_{\rho_0}$ : $\tau=+\infty$, la croissance
se poursuit, et la paire $\lbrace p,z\rbrace$ est retrouvée.

**Ce que ce corollaire ne dit pas.** Il ne borne pas $\lvert W_p\rvert$. Le §2 de
l'audit a raison : dans le pire cas $\tau$ peut rester infini jusqu'à
$\rho=\mathrm{diam}(X)$ pour beaucoup de points, et le coût redevient
$\Theta(n^{2})$ avant même l'énumération. **Aucune borne de parcimonie n'est
revendiquée.** Le pipeline publie par point $\lvert W_p\rvert$, le nombre de
doublements et le drapeau de certification ; c'est cette distribution mesurée,
et non une hypothèse, qui décide du budget (§7).


### 3.1 bis Un élagage réfuté

Une version intermédiaire de `catalogue.cpp` filtrait les tétraèdres candidats
par un graphe de « faces admissibles », en supposant que la boule circonscrite à
une face — centrée dans le plan de la face — est incluse dans la circumboule du
tétraèdre. **C'est faux.** L'intersection de la circumboule avec le plan est bien
le *disque* circonscrit de la face, mais la boule tridimensionnelle de même bord
déborde : si $h$ est la distance du centre au plan et $R$ le circumrayon, le
point le plus éloigné de cette boule est à distance
$h+\sqrt{R^{2}-h^{2}}>R$ du centre dès que $h>0$. L'erreur consistait à écrire
$\left\Vert z-c\right\Vert^{2}=\left\Vert z-c'\right\Vert^{2}+\left\Vert c-c'\right\Vert^{2}$,
qui ne vaut que pour $z$ **dans** le plan.

Le §2 de `WARNING_AUDIT_IMPLEMENTATION_2.md` en donne une fixture entière : un
tétraèdre régulier bien centré de rang fermé 4 dont les quatre faces ont chacune
un rang $\geq12$. L'élagage l'éliminait. Il est supprimé ; la régression R3 le
vérifie.

Le seul élagage conservé est prouvé : $R\leq\tau$ (théorème 4) et les sommets
d'un support bien centré sont sur une sphère de rayon $R$, donc deux à deux à
distance au plus $2\tau$. Le coût de l'énumération redevient $\Theta(m_p^{3})$
par point, et c'est bien pourquoi le *peeling* du §3 — qui ne visite que les
cellules réellement présentes — n'est pas une optimisation mais la seule voie
vers un budget.

### 3.2 Redondance et propriétaire canonique

Une sphère critique $(U,I)$ est découverte $\lvert U\rvert$ fois, une fois depuis
chaque $p\in U$. On la conserve une seule fois, chez
$p^{\star}=\min U$ pour l'ordre des identifiants. Le facteur de redondance est
donc borné par 4 et, pour le travail, il est en réalité utile : chaque copie
fournit une vue locale différente utilisée par la descente du §4.

## 4. La forêt de fusion

Fixons un ordre $k\leq K$. Le catalogue fournit :

- **naissances** : les sphères critiques de rang fermé exactement $k$ ;
- **fusions** : les sphères critiques de rang fermé exactement $k+1$, chacune
  portant $\lvert U\rvert$ *bras*.

Le bras $u\in U$ d'une fusion $S=I\sqcup U$ est la facette $F_u=S\setminus\lbrace u\rbrace$,
de cardinal $k$, dont le niveau $\beta(F_u)$ est **strictement** inférieur à
$\beta(S)$ (FAIT 12 : retirer un point de support diminue strictement la
miniboule). Il faut savoir dans quelle composante de $L_k$ vit $F_u$ juste avant
$\beta(S)$.

> **Descente (preuve du théorème 4 du manuscrit, relue).** Soit $F$ un ensemble
> de $k$ points, $\bar B_F$ sa miniboule, $S_F$ son support. Si
> $\bar B_F\cap X=F$, alors $(S_F,F\setminus S_F)$ est une sphère critique de rang
> fermé $k$ : c'est un minimum, on s'arrête. Sinon, il existe un intrus
> $z\in \mathring B_F\cap(X\setminus F)$ ; pour tout $u\in S_F$, l'ensemble
> $F'=(F\setminus\lbrace u\rbrace)\cup\lbrace z\rbrace$ vérifie
> $\beta(F')<\beta(F)$, et $F\cup\lbrace z\rbrace$ est une coface de niveau
> $\beta(F)$ qui relie $F$ à $F'$. La descente reste donc dans la même composante
> de $L_k$, décroît strictement et se termine sur un minimum.

Le minimum atteint ne dépend pas des choix : tous les chemins restent dans la
même composante au niveau courant, ce qui suffit pour le *merge tree*.

L'algorithme est alors un balayage classique :

1. trier naissances et fusions par niveau croissant ;
2. à une naissance : créer un nœud (racine d'une nouvelle classe DSU) ;
3. à une fusion : résoudre les $\lvert U\rvert$ bras par descente, puis unir les
   classes distinctes obtenues ; si $q\geq2$ classes distinctes sont réunies,
   créer un nœud de **multifusion** à $q$ enfants — la forêt n'est pas
   artificiellement binarisée, conformément à `DEFINITION_HGP_3D.md` §2 ;
4. les niveaux égaux sont traités **par lot** : toutes les fusions de même niveau
   exact sont résolues avant d'appliquer les unions.

### 4.1 Flèches verticales

`DEFINITION_HGP_3D.md` §10 exige les morphismes
$v_{k}:\pi_0(L_{k+1})\to\pi_0(L_k)$. Le modèle les rend **naturels** : une sphère
critique de rang fermé $s$ est la même donnée vue comme naissance à l'ordre $s$
et comme point critique d'indice un à l'ordre $s-1$. La composante née à l'ordre
$s$ en $c$ doit donc s'envoyer sur la composante de l'ordre $s-1$ qui contient
$c$ après traitement du lot de niveau $\beta$.

> **Obligation de preuve V.1.** Que cette flèche soit bien définie (indépendante
> du représentant), qu'elle commute avec les flèches horizontales et qu'elle
> coïncide avec le morphisme de `DEFINITION_HGP_3D.md` §10 **n'est pas démontré
> ici**. Une première rédaction affirmait que c'était « vrai par construction » ;
> c'est plus fort que ce que la théorie citée fournit. Tant que V.1 n'est pas
> close, la sortie publie les flèches comme *proposées*, et l'implémentation ne
> les émet pas.

## 5. La couverture

Le cluster discret est $C^{\mathrm{discret}}=X\cap\delta_r(C)$. Pour un point
$x\in X$ et un niveau $r$, $x$ est couvert par $C$ si et seulement s'il existe
$y\in C$ avec $\left\Vert x-y\right\Vert\leq r$, c'est-à-dire si la fonction
$g_x(y)=\max\left(\left\Vert y-x\right\Vert,d_K(y)\right)$ atteint une valeur
$\leq r$ sur $C$.

> **Obligation de preuve C.1.** La caractérisation des minima locaux de $g_x$ —
> et en particulier le fait qu'ils soient déjà dans le catalogue, ou obtenus par
> la même machinerie locale avec $x$ imposé — **demande une preuve et un oracle
> séparés**. Une première rédaction la donnait pour acquise. L'implémentation
> actuelle ne produit donc pas encore le `coverage_log`, et la sortie le déclare
> absent plutôt que de le fabriquer.


## 6. Prédicats, largeurs et domaine contractuel

### 6.1 Le domaine contractuel est le nuage quantifié

Les coordonnées d'entrée sont quantifiées sur la grille entière
$\left[0,2^{16}\right)$ par axe. **Ce n'est pas un repli exact vis-à-vis des
binary64 d'origine** : la quantification peut fusionner deux points, déplacer une
coquille et changer le signe d'un prédicat. Le §4 de
`WARNING_AUDIT_COMPLETUDE.md` a raison sur ce point. Le domaine contractuel de
MorseHGP3D v2 est donc **explicitement le nuage quantifié**, et l'échelle,
l'origine et le nombre de collisions sont publiés dans le reçu de sortie. Toute
affirmation d'exactitude porte sur ce nuage-là et sur lui seul.

### 6.2 Largeurs, recalculées

Les degrés annoncés dans une première rédaction étaient sous-estimés (audit §4).
Voici les largeurs réelles, avec $b=16$ donc des différences bornées par
$2^{b+1}=2^{17}$ :

| quantité | expression | degré | borne | type |
| --- | --- | ---: | --- | --- |
| `det3` | 3 colonnes linéaires | 3 | $6\cdot2^{51}=2^{53{,}6}$ | `i128` |
| `insphere` | 3 linéaires + 1 quadratique | **5** | $24\cdot2^{85}=2^{89{,}6}$ | `i128` |
| `den` (support 4) | $2\det$ | 3 | $2^{54{,}6}$ | `i128` |
| `num` (support 4) | 1 quadratique + 2 linéaires | 4 | $2^{72{,}2}$ | `i128` |
| `den` (support 3) | $2\lVert B_1\times B_2\rVert^{2}$ | 4 | $2^{72{,}6}$ | `i128` |
| `num` (support 3) | — | 5 | $2^{89{,}6}$ | `i128` |
| `sphere_side` | $\lVert w\rVert^{2}\,\mathrm{den}-2\langle w,\mathrm{num}\rangle$ | 6 | $2^{108{,}2}$ | `i128` |
| `well_centered4` | Gram–Cramer | **6** | $2^{110}$ | `i128` |
| $\lVert \mathrm{num}\rVert^{2}$ | — | 10 | $2^{180{,}8}$ | `BigInt<4>` |
| comparaison de niveaux | $N_1\Delta_2^{2}-N_2\Delta_1^{2}$ | **14** | $2^{326}$ | `BigInt<6>` |

Le choix $b=16$ est précisément dicté par la dernière ligne : avec $b=21$, la
comparaison de niveaux réclamerait plus de 384 bits et le `sphere_side` de
support 3 dépasserait `i128`. Les bornes ci-dessus sont vérifiées dans le code
par des commentaires de largeur à chaque fonction et par les tests unitaires.

### 6.3 Deux pièges arithmétiques, corrigés

- `big_cmp` comparait par soustraction en complément à deux : si la différence
  mathématique déborde la largeur signée, le signe obtenu n'est pas l'ordre
  (« maximum positif moins $-1$ » devient négatif). La comparaison se fait
  désormais par le signe puis lexicographiquement sur les mots non signés, et un
  test unitaire couvre le cas.
- La magnitude d'un `i128` était formée par `-a`, non défini pour la valeur
  minimale. Elle est désormais construite en `u128` par `~u + 1`.

### 6.4 Position générale : assumée, détectée, jamais supposée silencieusement

La définition 1 impose $X\cap\partial B(c,r)=U$ avec $\lvert U\rvert\leq4$. Une
coquille cosphérique de plus de quatre points n'est donc **pas représentable**
dans cette version, et le théorème de REANI–BOBROWSKI invoqué au §2 est lui-même
énoncé sous position générale. La v2 **assume la position générale pour la
filtration de ČECH** et, plutôt que de le supposer en silence :

- le classificateur détecte tout point situé exactement sur une sphère sans
  appartenir au support proposé, et rejette la configuration ;
- l'oracle compte ces rejets et les publie (`degenerate`) ;
- la descente du §4 hérite de la même hypothèse : le §4 de l'audit exhibe
  $F=\lbrace(-1,0,0),(1,0,0)\rbrace$ avec $X=F\cup\lbrace(0,1,0)\rbrace$, où la
  miniboule fermée rencontre $X\setminus F$ **sans intrus strict**. La descente
  renvoie alors un échec explicite au lieu d'un résultat.

Le traitement complet des coquilles (supports minimaux multiples, transitions de
plateau) est une **obligation de preuve ouverte**, pas une fonctionnalité
présente.


## 7. Budget et architecture GPU

Cibles mesurées du dépôt : $\approx1{,}8\cdot10^{7}$ objets utiles à $n=50\,000$,
$K=10$, tous rangs $\leq11$ confondus. Pour l'ordre seul $k=K$, seuls les rangs
$K$ et $K+1$ sont nécessaires.

| étage | travail | cible |
| --- | --- | --- |
| normalisation + Morton + grille | $\Theta(n)$ | < 2 ms |
| kNN local ($s_{\max}$ voisins, rayon de départ) | $\Theta(n\,s_{\max})$ | < 3 ms |
| peeling local par point (bloc/warp par point) | $\Theta\left(\sum_p (\lvert W_p\rvert\log\lvert W_p\rvert+Z_p)\right)$ | 20–60 ms |
| compaction + propriétaire canonique | $\Theta(Z)$ tri par clé | < 10 ms |
| descente des bras | $\Theta(\text{fusions}\times\lvert U\rvert\times d_T)$ | 10–30 ms |
| tri par niveau + DSU par lots | $\Theta(Z\log Z)$ | 10–20 ms |
| couverture | $\Theta(n\,\bar\deg)$ | < 10 ms |

Chaque point est un **sous-problème indépendant** : c'est la propriété qui rend
l'ensemble massivement parallèle. Un bloc CUDA traite un point, charge son
voisinage $W_p$ en mémoire partagée (quelques centaines de points, soit quelques
kilo-octets), et n'écrit que ses sphères critiques. Aucune structure globale —
ni mosaïque de DELAUNAY d'ordre supérieur, ni graphe de facettes, ni catalogue de
cofaces — n'est matérialisée, ce qui respecte l'invariant d'architecture
d'`AGENTS.md`.

Le pire cas reste dense : si $\rho$ doit croître jusqu'au diamètre pour beaucoup
de points, $\lvert W_p\rvert=\Theta(n)$. Le pipeline le publie (histogramme de
$\lvert W_p\rvert$ et du nombre de doublements) au lieu de le supposer absent.

## 8. Ce que la v2 ne fait pas

- Elle ne construit pas $\Gamma_K$, ni son sous-graphe de GABRIEL, ni un
  $K$-arbre couvrant. Les théorèmes 4–7 du manuscrit restent vrais et utiles
  comme éclairage géométrique (tout simplexe séparant est de GABRIEL), mais ils
  ne sont plus le chemin de calcul.
- Elle ne matérialise aucune mosaïque de DELAUNAY d'ordre supérieur. Le
  $\leq(K)$-level local en est la trace, mais il est consommé en flux et jamais
  assemblé globalement.
- Elle ne suppose pas la position générale.

## 9. Correspondance avec le vocabulaire de la v1

| v1 | v2 |
| --- | --- |
| coface $K$-séparante | sphère critique de rang fermé $K+1$ |
| facette active $\tau$ | bras d'une fusion |
| naissance isolée de rang fermé $k$ | sphère critique de rang fermé $k$ (minimum) |
| incidence silencieuse | point **non critique** : sans effet, jamais énuméré |
| descente miniball | descente du §4, inchangée |
| ancre paire diamètre + arrangement de cordes | cas $\lvert U\rvert=2$ du peeling dual |
| $\tau(p)\geq2R(p)$ non certifié | théorème 4 : $2R_\rho\leq\rho$, certifié a posteriori |

## 10. Portes de validation

1. **P0 — oracle.** Un oracle force brute rationnel énumère tous les
   $(U,I)$ pour $n\leq14$ et tout $K$ ; le catalogue v2 doit lui être identique
   bit à bit, y compris sur les fixtures dégénérées du dépôt.
2. **P1 — $k=1$.** La forêt d'ordre 1 doit être exactement la forêt de fusion de
   l'EMST euclidien, lots d'égalités compris (`DEFINITION_HGP_3D.md` §7).
3. **P2 — topologie.** Sur des nuages de taille moyenne, $\#\pi_0(L_k(a))$ lu sur
   la forêt doit coïncider avec un comptage indépendant par grille dense.
4. **P3 — auto-certification.** Aucun point ne doit terminer avec
   $2R_\rho>\rho$ ; le compteur de doublements et l'histogramme de
   $\lvert W_p\rvert$ sont publiés.
5. **P4 — contrat.** $n=50\,000$, $K=10$, p95 `warm_e2e` mesurée dans un service
   résident, pic VRAM instrumenté.

Aucune de ces portes ne peut être franchie par un accord moyen ou un benchmark :
P0 à P2 sont des égalités exactes.

### 10.1 Obligations ouvertes après le second audit

`WARNING_AUDIT_IMPLEMENTATION_2.md` laisse ouvertes des obligations que cette
version **ne ferme pas** ; elles sont reprises ici pour qu'aucune promotion ne
puisse les contourner :

1. **Le recouvrement sphérique n'est pas certifié.** `make_cover` estime son
   rayon de couverture par échantillonnage puis applique une marge heuristique.
   Le drapeau `Catalogue::certified` décrit donc une réussite *heuristique*, pas
   une autorité d'exactitude. Il faut des bornes dirigées, un arrondi sortant et
   un refus explicite des configurations non couvertes. Ce qui est acquis : un
   recouvrement vide ou de demi-angle $\geq\pi/4$ rend désormais $\tau=+\infty$
   au lieu de certifier à tort (régression R4).
2. **L'oracle ne voit pas la structure.** P2 ne compare que des *nombres* de
   composantes : il a laissé passer une chaîne de fusions binaires de même
   niveau là où la définition normative impose une multifusion contractée
   (`DEFINITION_HGP_3D.md` §6). Il faut comparer la forêt elle-même — arité des
   nœuds, niveaux, généalogie — et non son seul $\pi_0$.
3. **L'oracle partage des primitives avec le chemin testé.** `brute_catalogue`
   réutilise `sphere4`, `well_centered4`, `sphere_side` et `miniball_of` ; une
   erreur commune passerait les deux. Il faut un oracle rationnel indépendant,
   et la comparaison P0 doit porter sur le rang, les membres, le shell et le
   niveau, pas seulement sur les identifiants du support.
4. **Les coquilles cosphériques restent hors du modèle** (rejetées et comptées).
5. **La complexité reste combinatoire** : $\Theta\left(\sum_p m_p^{3}\right)$
   après suppression de l'élagage faux, sans borne prouvée sur $m_p$.
6. **Il n'existe aucun backend CUDA**, aucune couverture discrète et aucune
   flèche verticale émise.

## Références

- L. HAUSEUX, *manuscrit de thèse*, Parties I et II (chap. 6 et 8 en
  particulier) — `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`.
- Y. REANI, O. BOBROWSKI, *Morse theory for the k-NN distance function*, 2024.
- H. EDELSBRUNNER, G. OSANG, *A simple algorithm for higher-order Delaunay
  mosaics and alpha shapes*, 2020 — `docs/references/pdfs/`.
- `docs/math/DEFINITION_HGP_3D.md`, `docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`,
  `docs/research/CONTRAT_50K_BILAN.md`.
