# Résultat B — Descente facette–centre et attaches hiérarchiques

> **Statut.** La descente est un théorème fini sous des hypothèses explicites de sélection stricte. Sa limite miniball est également un théorème sous essentialité des supports visités. Le traitement exact et peu coûteux des plateaux dégénérés, ainsi que la découverte exhaustive des selles, restent deux problèmes séparés.

## 1. Pourquoi une descente est nécessaire

Une selle locale du champ K-NN ne donne pas à elle seule une fusion dans la hiérarchie. Chacun de ses bras doit être rattaché à une composante de sous-niveau **strictement antérieure**. Marcher dans une mosaïque de Delaunay d'ordre supérieur serait une manière de calculer cette attache, mais ce n'est pas une nécessité mathématique.

La descente facette–centre utilise seulement deux opérations :

1. minimiser un atome convexe indexé par une facette de $k$ points ;
2. sélectionner une facette K-NN exacte au centre obtenu.

La trajectoire géométrique produite par ces alternances constitue un certificat de connexité sous-niveau. Elle est indépendante de toute grille et de toute mosaïque.

Dans tout le document, $X=(x_1,\ldots,x_n)\subset\mathbb{R}^{d}$ est fini, $a_1(y)\leq\cdots\leq a_n(y)$ désignent les distances carrées aux observations rangées avec multiplicité, et

$$D_k(y)=a_k(y).$$

## 2. Théorème abstrait pour une enveloppe d'atomes

Soit $\mathcal{Q}$ un ensemble fini d'indices. À chaque $Q\in\mathcal{Q}$ associons une fonction continue, convexe et coercive

$$\psi_Q:\mathbb{R}^{d}\longrightarrow\mathbb{R}.$$

Supposons que chaque $\psi_Q$ possède un unique minimiseur et posons

$$c(Q)=\mathop{\mathrm{argmin}}_{y\in\mathbb{R}^{d}}\psi_Q(y),\qquad\beta(Q)=\psi_Q(c(Q)),$$

$$F(y)=\min_{Q\in\mathcal{Q}}\psi_Q(y),\qquad\mathcal{B}(y)=\mathop{\mathrm{argmin}}_{Q\in\mathcal{Q}}\psi_Q(y).$$

Une règle de sélection $S$ choisit $S(y)\in\mathcal{B}(y)$. Considérons

$$Q_{m+1}=S(c(Q_m)).$$

> **Théorème B.1 — descente atomique stricte.** Supposons que, pour chaque facette visitée, $Q_m\neq Q_{m+1}$ implique
>
> $$\psi_{Q_{m+1}}(c(Q_m))<\beta(Q_m).$$
>
> Alors $\beta(Q_{m+1})<\beta(Q_m)$ à chaque transition non stationnaire. L'itération termine après au plus $\lvert\mathcal{Q}\rvert-1$ transitions sur un point fixe $Q_\star=S(c(Q_\star))$. De plus, pour
>
> $$\gamma_m(s)=(1-s)c(Q_m)+s c(Q_{m+1}),\qquad 0\leq s\leq1,$$
>
> on a
>
> $$F(\gamma_m(s))<\beta(Q_m)$$
>
> pour tout $s\in[0,1]$ et pour chaque transition non stationnaire. Si $S$ est localement constant au voisinage de $c(Q_\star)$, alors $c(Q_\star)$ est un minimum local de $F$.

**Preuve.** La minimisation du nouvel atome donne

$$\beta(Q_{m+1})\leq\psi_{Q_{m+1}}(c(Q_m))<\beta(Q_m).$$

La convexité donne ensuite

$$\psi_{Q_{m+1}}(\gamma_m(s))\leq(1-s)\psi_{Q_{m+1}}(c(Q_m))+s\beta(Q_{m+1})<\beta(Q_m).$$

Comme $F\leq\psi_{Q_{m+1}}$, le segment entier reste dans le sous-niveau strict annoncé. La suite ne peut revisiter une facette, car sa valeur $\beta$ décroît strictement ; la finitude donne la terminaison. Au point fixe, si $S$ reste constant dans un voisinage, $F=\psi_{Q_\star}$ dans ce voisinage et $c(Q_\star)$ minimise cet atome. $\square$

Ce théorème est volontairement abstrait : il sépare la géométrie du chemin de la manière dont les meilleurs atomes sont obtenus.

## 3. Spécialisation aux lentilles homogènes, $1\leq q<\infty$

Reprenons les notations du [résultat A](RESULTAT_A_LENTILLES_HOMOGENES.md). Pour une facette $Q$ de cardinal $k$,

$$\Phi_{Q,q}(y)=\left(\frac{1}{k}\sum_{i\in Q}\left\Vert y-x_i\right\Vert^{2q}\right)^{1/q},\qquad c_q(Q)=\mathop{\mathrm{argmin}}_y\Phi_{Q,q}(y),\qquad\beta_q(Q)=\Phi_{Q,q}(c_q(Q)).$$

Le meilleur atome à $y$ est une facette formée de $k$ plus proches voisins de $y$. Notons $N_k(y)$ cette facette lorsque l'écart de shell est strict :

$$a_k(y)<a_{k+1}(y),$$

avec la convention $a_{n+1}=+\infty$ lorsque $k=n$.

> **Théorème B.2 — descente homogène finie.** Fixons $1\leq q<\infty$. Supposons que l'écart $a_k(c_q(Q_m))<a_{k+1}(c_q(Q_m))$ soit strict pour toute facette visitée. Alors l'itération
>
> $$Q_{m+1}=N_k(c_q(Q_m))$$
>
> termine après au plus $\binom{n}{k}-1$ transitions. À chaque transition non stationnaire,
>
> $$\beta_q(Q_{m+1})<\beta_q(Q_m),$$
>
> et le segment entre les deux centres appartient à $\left\lbrace F_{k,q}<\beta_q(Q_m)\right\rbrace$. Le centre terminal est un minimum local de $F_{k,q}$.

**Preuve.** Sous l'écart strict, $N_k(c_q(Q_m))$ est l'unique facette minimisant la somme des puissances des distances. Si elle diffère de $Q_m$, alors

$$\Phi_{Q_{m+1},q}(c_q(Q_m))<\Phi_{Q_m,q}(c_q(Q_m))=\beta_q(Q_m).$$

Le théorème B.1 s'applique. L'écart strict persiste dans un voisinage du centre terminal, donc la facette K-NN y est localement constante. $\square$

Le centre peut être caractérisé sans logarithme. Pour $q=1$,

$$c_1(Q)=\frac{1}{k}\sum_{i\in Q}x_i.$$

Pour $q>1$, il est l'unique solution de

$$\sum_{i\in Q}\left\Vert c_q(Q)-x_i\right\Vert^{2q-2}(c_q(Q)-x_i)=0.$$

La descente à $q=1$ est donc une version finie du mean-shift K-NN sur les barycentres. Pour $q>1$, elle remplace le barycentre par un centre de Fréchet de puissance $2q$.

### 3.1 Ce que prouve la terminaison

La borne $\binom{n}{k}-1$ est seulement une borne finie de pire cas. Le théorème ne donne ni une borne logarithmique, ni une borne indépendante de $n$, ni même une longueur moyenne faible. Il prouve en revanche trois propriétés directement hiérarchiques :

- aucune trajectoire stricte ne cycle ;
- chaque segment fournit un chemin explicite dans le bon sous-niveau ;
- le minimum terminal appartient à la même composante de sous-niveau que le point de départ de la descente.

## 4. Passage à la limite miniball

Pour une facette $Q$, posons

$$g_Q(y)=\max_{i\in Q}\left\Vert y-x_i\right\Vert^2,$$

$$c_\infty(Q)=\mathop{\mathrm{argmin}}_y g_Q(y),\qquad\beta_\infty(Q)=g_Q(c_\infty(Q)).$$

Le couple $(c_\infty(Q),\sqrt{\beta_\infty(Q)})$ est la plus petite boule englobante de $Q$. Son ensemble frontière complet est

$$U(Q)=\left\lbrace x_i:i\in Q,\ \left\Vert c_\infty(Q)-x_i\right\Vert^2=\beta_\infty(Q)\right\rbrace.$$

Nous utiliserons l'hypothèse locale suivante sur chaque facette effectivement visitée.

> **Hypothèse $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$.** L'ensemble $U(Q)$ est affinement indépendant, il constitue l'unique support minimal de la miniball, le centre appartient à $\mathrm{relint}\,\mathrm{conv}(U(Q))$, et aucun point de $X\setminus Q$ n'appartient à la sphère $\partial B(c_\infty(Q),\sqrt{\beta_\infty(Q)})$.

La condition de `relint` implique que tous les points de $U(Q)$ ont une coordonnée barycentrique strictement positive. Ils sont donc tous essentiels.

> **Lemme B.3 — retrait d'un support essentiel.** Supposons $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ pour $Q$. Soit $R$ un ensemble de même cardinal obtenu en retirant au moins un point de $U(Q)$ et en ajoutant seulement des points strictement intérieurs à la miniball de $Q$. Alors
>
> $$\beta_\infty(R)<\beta_\infty(Q).$$

**Preuve.** L'ensemble $R$ est contenu dans la miniball fermée de $Q$. Si son rayon minimal était égal, cette boule serait encore minimale pour $R$. Le critère de minimalité d'une boule euclidienne imposerait alors que son centre appartienne à l'enveloppe convexe des points de $R$ situés sur sa frontière. Ceux-ci forment un sous-ensemble strict de $U(Q)$ ne contenant pas au moins un support essentiel, ce qui contredit l'unicité et l'essentialité du support. $\square$

Pour éviter toute ambiguïté entre un atome qui atteint seulement le même maximum et une vraie facette top-$k$, définissons, pour $r_k^2=a_k(y)$,

$$E_{<}(y)=\left\lbrace i:\left\Vert y-x_i\right\Vert^2<r_k^2\right\rbrace,\qquad E_{=}(y)=\left\lbrace i:\left\Vert y-x_i\right\Vert^2=r_k^2\right\rbrace,$$

$$\mathcal{N}_k(y)=\left\lbrace E_{<}(y)\cup S:S\subseteq E_{=}(y),\ \lvert S\rvert=k-\lvert E_{<}(y)\rvert\right\rbrace.$$

Ainsi, une facette de $\mathcal{N}_k(y)$ contient tous les points strictement plus proches que le shell de rang $k$. En présence d'ex aequo, cette condition est plus forte que la seule égalité $g_Q(y)=D_k(y)$.

> **Théorème B.4 — descente K-NN–miniball.** Supposons $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ pour chaque facette visitée et supposons exacts les K-NN et les miniballs. Choisissons
>
> $$Q_{m+1}\in\mathcal{N}_k(c_\infty(Q_m)).$$
>
> Si $Q_m\notin\mathcal{N}_k(c_\infty(Q_m))$, alors
>
> $$\beta_\infty(Q_{m+1})<\beta_\infty(Q_m).$$
>
> L'itération termine en un nombre fini d'étapes sur une facette $Q_\star$ active à son propre centre. Pour $0\leq s\leq1$, posons
>
> $$\gamma_m(s)=(1-s)c_\infty(Q_m)+s c_\infty(Q_{m+1}).$$
>
> Alors
>
> $$D_k(\gamma_m(0))\leq\beta_\infty(Q_m),\qquad D_k(\gamma_m(s))<\beta_\infty(Q_m)\quad\text{pour }0<s\leq1.$$
>
> Le centre terminal est un minimum local non dégénéré d'indice $0$ du champ K-NN dur, au sens des hypothèses précédentes.

**Preuve.** Si $Q_m$ n'est pas K-NN à son centre, un point extérieur à $Q_m$ se trouve strictement à l'intérieur de sa miniball ; l'absence de point extérieur sur la sphère exclut le cas d'égalité. Comme $Q_m$ fournit déjà $k$ points dans la boule fermée, toute facette K-NN au centre est encore contenue dans cette boule, ajoute seulement des points intérieurs et retire au moins un des points frontière les plus éloignés. Le lemme B.3 donne la décroissance stricte.

Au centre courant,

$$g_{Q_{m+1}}(c_\infty(Q_m))\leq\beta_\infty(Q_m),$$

tandis que $\beta_\infty(Q_{m+1})<\beta_\infty(Q_m)$. Par convexité de $g_{Q_{m+1}}$,

$$g_{Q_{m+1}}(\gamma_m(s))\leq(1-s)g_{Q_{m+1}}(c_\infty(Q_m))+s\beta_\infty(Q_{m+1})<\beta_\infty(Q_m)$$

pour $s>0$. Comme $D_k\leq g_{Q_{m+1}}$, le chemin est strict après son point de départ. La décroissance de $\beta_\infty$ interdit les cycles.

À la terminaison, aucun point extérieur à $Q_\star$ n'est dans la miniball fermée. L'écart entre la $k$-ième et la $(k+1)$-ième distance est donc strict ; $D_k=g_{Q_\star}$ dans un voisinage du centre, qui est un minimum local. En notant $N_{\mathrm{closed}}$ le nombre de points dans la boule fermée terminale, on a $N_{\mathrm{closed}}=k$, donc l'indice de Reani–Bobrowski vaut $\mu=N_{\mathrm{closed}}-k=0$. L'indépendance affine du support, la condition de `relint` et la hessienne positive des distances carrées donnent sa non-dégénérescence locale. $\square$

La précision sur $s=0$ est nécessaire : plusieurs points frontière peuvent être ex aequo, de sorte que $D_k(c_\infty(Q_m))$ peut être égal au niveau précédent. Le segment entre **immédiatement** dans le sous-niveau strict.

## 5. Cohérence entre les centres finis et la miniball

> **Proposition B.5 — convergence des centres.** Pour toute facette fixée $Q$,
>
> $$\lim_{q\to\infty}\beta_q(Q)=\beta_\infty(Q),\qquad\lim_{q\to\infty}c_q(Q)=c_\infty(Q).$$

**Preuve.** L'encadrement du résultat A donne

$$k^{-1/q}g_Q(y)\leq\Phi_{Q,q}(y)\leq g_Q(y).$$

Il implique immédiatement la convergence des minima. De plus,

$$g_Q(c_q(Q))\leq k^{1/q}\Phi_{Q,q}(c_q(Q))\leq k^{1/q}\beta_\infty(Q).$$

La suite des centres est donc bornée. Toute valeur d'adhérence minimise $g_Q$ ; l'unicité du centre de miniball impose qu'elle soit $c_\infty(Q)$. $\square$

Cette convergence ne garantit pas à elle seule la convergence des **transitions** $N_k(c_q(Q))$. Une marge de shell positive au centre limite est nécessaire. Sans elle, la limite dure possède un plateau de choix que les valeurs finies de $q$ peuvent résoudre de façons différentes.

## 6. Ex aequo et plateaux

Les ex aequo ne doivent pas être masqués par un ordre arbitraire des identifiants si la sortie visée est la hiérarchie exacte du nuage non perturbé.

À un point $y$, l'ensemble exact des facettes K-NN est la famille $\mathcal{N}_k(y)$ définie à la section précédente à partir de $E_{<}(y)$ et $E_{=}(y)$.

Deux stratégies ont des statuts différents.

1. **Perturbation symbolique.** Elle donne une facette unique et permet d'appliquer les théorèmes stricts, mais calcule d'abord la hiérarchie perturbée. Recoller ensuite les niveaux égaux exige un argument séparé.
2. **Traitement par lot.** À $y$ fixé, toutes les facettes de $\mathcal{N}_k(y)$ sont reliées par swaps dans $E_{=}(y)$. Cette connexité n'implique toutefois pas que leurs valeurs de naissance $\beta_\infty$ soient égales. Un graphe candidat pour formaliser la dynamique de plateau oriente une transition $Q\to R$ lorsque $R\in\mathcal{N}_k(c_\infty(Q))$, distingue les transitions à valeur constante $\beta_\infty(R)=\beta_\infty(Q)$ des sorties strictement descendantes, puis contracte les composantes fortement connexes à valeur constante.

Le regroupement simultané est nécessaire pour respecter la hiérarchie non perturbée, mais il reste à démontrer que le quotient précédent fournit la sémantique exacte de tout plateau. Sa construction implicite et l'obtention de ses composantes fortement connexes sans énumération sont également des problèmes ouverts. Le théorème B.4 ne doit pas être cité tel quel sur un plateau où $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ échoue.

Les événements critiques de même niveau posent un second problème, distinct des K-NN ex aequo : toutes leurs incidences doivent être évaluées par rapport à $\left\lbrace D_k<t\right\rbrace$, puis contractées simultanément. Un ordre séquentiel de traitement ne doit pas binariser artificiellement un nœud multifurqué.

## 7. Attache globale d'un bras : principe abstrait

> **Proposition B.6 — certificat d'attache par chemin descendant.** Soit $h$ un niveau et $C$ une composante de $\left\lbrace F<h\right\rbrace$. Supposons qu'il existe un point $z$ du germe considéré et un chemin $\rho:[0,1]\to C$ tel que $\rho(0)=z$ et $\rho(1)=c(Q_0)$, avec $\beta(Q_0)<h$. Considérons une itération finie $Q_0,\ldots,Q_\star$ telle que, à chaque transition non stationnaire, $\beta(Q_{m+1})<\beta(Q_m)$ et les centres successifs soient reliés par un chemin $\gamma_m$ satisfaisant
>
> $$F(\gamma_m(s))\leq\beta(Q_m)\qquad\text{pour tout }s\in[0,1].$$
>
> Alors $c(Q_\star)\in C$.

**Preuve.** Le chemin initial est dans $C$. La décroissance donne $\beta(Q_m)\leq\beta(Q_0)<h$ pour tout $m$, et chaque chemin $\gamma_m$ est donc contenu dans $\left\lbrace F<h\right\rbrace$. La concaténation relie $z$ au centre terminal dans ce sous-niveau ; elle reste dans la composante $C$. $\square$

Ce résultat paraît élémentaire, mais il isole exactement l'information requise par une forêt de fusion : le minimum terminal est un représentant de la composante globale du bras. Il n'est pas nécessaire de reconstruire toute la cellule d'ordre supérieur traversée.

## 8. Attaches des selles HGP dures

Considérons une selle non dégénérée d'indice $1$ du champ $D_k$. Écrivons sa boule critique $B(c,r)$ sous la forme

$$I=X\cap B^{\circ}(c,r),\qquad U=X\cap\partial B(c,r),$$

et supposons

$$\lvert I\rvert+\lvert U\rvert=k+1,$$

avec $U$ affinement indépendant, support minimal de la boule, et $c\in\mathrm{relint}\,\mathrm{conv}(U)$. Ici le rang fermé est $N_{\mathrm{closed}}=\lvert I\rvert+\lvert U\rvert$, de sorte que la convention de Reani–Bobrowski donne bien $\mu=N_{\mathrm{closed}}-k=1$. Comme $r^2=D_k(c)$, on a $\lvert I\rvert<k$ et donc $\lvert U\rvert\geq2$. Pour $u\in U$, la facette du bras local est

$$Q_u=I\cup\bigl(U\setminus\lbrace u\rbrace\bigr).$$

> **Théorème B.7 — correction des attaches par descente miniball.** Supposons $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ le long de toutes les descentes issues des $Q_u$. Alors chaque segment
>
> $$\eta_u(s)=(1-s)c+s c_\infty(Q_u),\qquad0<s\leq1,$$
>
> appartient à $\left\lbrace D_k<r^2\right\rbrace$ et part dans le germe local indexé par $Q_u$. La concaténation de $\eta_u$ avec la descente K-NN–miniball issue de $Q_u$ reste dans ce sous-niveau strict et termine dans la composante globale attachée à ce bras.

**Preuve.** Tous les points de $U$ sont essentiels. Retirer $u$ donne donc

$$\beta_\infty(Q_u)<r^2.$$

Comme $g_{Q_u}(c)=r^2$, la convexité donne, pour $s>0$,

$$D_k(\eta_u(s))\leq g_{Q_u}(\eta_u(s))\leq(1-s)r^2+s\beta_\infty(Q_u)<r^2.$$

Pour identifier le germe, posons $v=c_\infty(Q_u)-c$. Comme $\beta_\infty(Q_u)<r^2=g_{Q_u}(c)$, on a $v\neq0$. Pour tout $x\in U\setminus\lbrace u\rbrace$ et tout $0<s\leq1$, la majoration stricte précédente donne

$$r^2-2s\langle x-c,v\rangle+s^2\left\Vert v\right\Vert^2<r^2,$$

donc $2\langle x-c,v\rangle>s\left\Vert v\right\Vert^2>0$ et

$$\left.\frac{\mathrm{d}}{\mathrm{d}s}\left\Vert c+sv-x\right\Vert^2\right\rvert_{s=0}=-2\langle x-c,v\rangle<0.$$

Écrivons maintenant la relation barycentrique stricte

$$\sum_{x\in U}\lambda_x(x-c)=0,\qquad\lambda_x>0.$$

En prenant le produit scalaire avec $v$, on obtient

$$-2\langle u-c,v\rangle=\frac{2}{\lambda_u}\sum_{x\in U\setminus\lbrace u\rbrace}\lambda_x\langle x-c,v\rangle>0.$$

La distance au point exclu $u$ croît donc au premier ordre. La trajectoire quitte $c$ dans le secteur local où les $k$ points de $Q_u$ sont sous le niveau critique, c'est-à-dire le bras indexé par $Q_u$. Pour tout $\delta>0$ assez petit, $\eta_u(\delta)$ appartient à ce germe et $\eta_u([\delta,1])$ le relie à $c_\infty(Q_u)$ dans $\left\lbrace D_k<r^2\right\rbrace$. Les chemins du théorème B.4 vérifient l'hypothèse faible de la proposition B.6, avec des potentiels strictement décroissants à partir de $\beta_\infty(Q_u)<r^2$ ; cette proposition achève donc la preuve. $\square$

Si deux bras terminent en des minima distincts déjà reliés sous $r^2$, ils ont la même attache globale. Une union–find construite sur les événements strictement antérieurs identifie alors leurs deux représentants. Pour que cette lecture soit exacte, il faut :

- un catalogue complet des événements antérieurs pertinents, ou un oracle équivalent de composantes ;
- l'insertion du minimum terminal s'il n'était pas encore catalogué ;
- un traitement simultané de toutes les selles au niveau $r^2$ ;
- des K-NN et miniballs exacts, ainsi que les hypothèses de support le long du chemin.

Le théorème attache correctement **une selle déjà connue**. Il ne prouve pas que toutes les selles ont été découvertes.

## 9. Version finie en $q$ des attaches

La proposition B.6 s'applique sans changement à $F_{k,q}$ pour $q<\infty$ : dès qu'un bras possède une facette germe $Q_0$ et un chemin initial strictement sous son niveau, la descente homogène fournit son attache globale.

Ce qui manque encore est une classification de Morse complète des singularités de l'enveloppe inférieure $F_{k,q}$ et une construction canonique de leurs facettes germes pour tout $q$ fini. Les points suivants ne doivent donc pas être présentés comme acquis :

- une bijection entre les selles de $F_{k,q}$ et celles de $D_k$ ;
- la conservation de l'indice lors de la continuation en $q$ ;
- l'absence de création ou d'annulation de paires critiques ;
- la préservation automatique des attaches lorsque des marges s'annulent.

La continuation DTM $\to$ HGP est un excellent mécanisme de proposition et de suivi tant que les marges de shell, de valeur et d'incidence restent positives. Elle n'est pas un certificat de complétude.

## 10. Portée GPU-friendly du résultat

La descente possède une forme mathématique compatible avec un traitement massivement parallèle :

- son état est une facette de taille fixe $k$ ;
- le centre fini est une réduction convexe locale, et le centre dur une miniball locale ;
- la transition est une requête top-$k$ au centre ;
- le potentiel $\beta$ fournit un invariant de décroissance vérifiable indépendamment pour chaque trajectoire ;
- plusieurs bras et plusieurs valeurs de $k\leq K$ peuvent être traités par lots ;
- aucune triangulation, mosaïque ou grille n'entre dans la preuve.

Ces propriétés ne constituent pas une spécification de noyaux GPU et ne donnent aucun temps d'exécution. Les trois coûts non résolus sont la requête K-NN exacte, la longueur moyenne des trajectoires et le nombre de selles à attacher.

## 11. Théorèmes acquis et programme restant

| Énoncé | Statut |
|---|---|
| Descente stricte d'une enveloppe finie d'atomes convexes | théorème exact |
| Descente homogène sous marges K-NN strictes | théorème exact |
| Convergence des centres de Fréchet vers la miniball | théorème exact |
| Descente miniball sous supports essentiels et vacuité de frontière | théorème exact conditionnel à $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ |
| Chemins stricts et attaches d'une selle HGP connue | théorème exact sous les hypothèses énoncées |
| Terminaison arbitraire en présence d'ex aequo non traités | non démontrée et fausse pour certaines règles de sélection cycliques |
| Traitement exact des plateaux par hypergraphe implicite sparse | programme |
| Découverte exhaustive des selles | hors du résultat B ; relève du résultat D |
| Classification des selles pour tout $q<\infty$ | programme |
| Borne faible sur le nombre moyen d'étapes en 3D | programme probabiliste |

Le résultat B ferme donc le problème des attaches sous position générale sans imposer de mosaïque d'ordre supérieur. Il ne ferme pas le problème d'énumération : **découvrir les événements** et **attacher les événements connus** sont deux tâches mathématiques différentes.

## Références

- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6 à 8 : régions témoins, simplexes séparants, supports de miniballs et K-arbre minimum couvrant.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024 : indices critiques, description locale et supports bien centrés du champ K-NN.
- E. Welzl, [*Smallest Enclosing Disks (Balls and Ellipsoids)*](https://doi.org/10.1007/BFb0038202), 1991 : plus petite boule englobante et calcul à dimension fixée.
