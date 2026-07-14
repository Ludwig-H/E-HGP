# HGP ordre–échelle : échafaudage de Morse classé, descente facette–centre et réalisation GPU

> [!NOTE]
> **Statut éditorial.** Ce rapport reste la synthèse qui motive l'architecture à deux backends. Les résultats A–E annoncés en section 13 sont désormais développés dans [`docs/math/`](docs/math/README.md). Pour le résultat D, la direction active renonce à matérialiser le rhomboid tiling ou une mosaïque de Delaunay d'ordre supérieur : elle adapte leur récurrence à une cascade de diagrammes de puissance 3D ordinaires, avec certificat explicite de fermeture.

> Suite mathématique et algorithmique — 14 juillet 2026  
> Dépôt examiné : [`Ludwig-H/E-HGP`](https://github.com/Ludwig-H/E-HGP), branche `main`, révision [`5a8d5445d0772efcc77082165dd558b2dfb82ea8`](https://github.com/Ludwig-H/E-HGP/tree/5a8d5445d0772efcc77082165dd558b2dfb82ea8)  
> Point de départ : [`RAPPORT_HIERARCHIE_HGP_MORSE_SOFTLENS_GPU_2026-07-13 (1).md`](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/RAPPORT_HIERARCHIE_HGP_MORSE_SOFTLENS_GPU_2026-07-13%20%281%29.md), y compris son addendum normatif  
> Cibles : tous les ordres $1\leq k\leq K_{\max}=10$ ; nuages 3D de l'ordre de $50\,000$ points ; très grande dimension

> [!IMPORTANT]
> **Verdict révisé.** La bonne structure n'est pas un arbre calculé dix fois, mais une **bifiltration ordre–échelle**. En 3D, son squelette $H_0$ exact est porté par un catalogue unique de sphères critiques classées, muni des attaches globales et des incidences de lots : une sphère de rang fermé $s$ porte une naissance à l'ordre $s$ et une selle à l'ordre $s-1$. Le problème des attaches globales peut être résolu sans mosaïque par une descente alternée « facette K-NN $\leftrightarrow$ centre de miniball », monotone et finie sous l'hypothèse de frontière essentielle $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ précisée ci-dessous. En grande dimension, la meilleure relaxation semble être une lentille entropique **homogène**, reliant la DTM à HGP sans température dimensionnée, sur un atlas de blocs de taille $K_{\max}$ fermé par sous-ensembles. L'entropie rend les noyaux locaux réguliers et GPU-friendly ; la sparsité vient de l'atlas d'événements et de la forêt, jamais de l'entropie seule.

## 1. Ce que change la version GitHub modifiée

La version modifiée ne se contente pas de corriger la forme du premier rapport. Son addendum fixe plusieurs contraintes qui deviennent ici des invariants.

1. Tous les seuils sont exprimés en distance au carré.
2. La représentation par swaps est exacte sur l'atlas complet. Sur un atlas arbitraire, elle exige soit le graphe complet implicite des rencontres, soit un certificat que les chemins minimax restent dans l'atlas.
3. Une liste de candidats validés n'est pas une preuve de complétude. En 3D, le rang d'une sphère peut être certifié par BVH, mais l'absence d'une sphère jamais proposée ne l'est pas.
4. Les bras locaux d'une selle doivent être distingués de leurs attaches globales ; les événements de même niveau doivent être traités par lots.
5. En grande dimension, l'exemple de la base canonique exclut une représentation exacte universellement sparse, y compris après passage à la DTM.
6. La sortie doit rester une hiérarchie de composantes continues, enrichie si nécessaire d'une relation de couverture ; une partition plate n'est qu'un post-traitement.

La présente suite conserve ces corrections. Elle apporte surtout trois éléments nouveaux :

- un objet commun à tous les ordres $k\leq K_{\max}$ ;
- une régularisation entropique sans unité, mieux adaptée à une filtration multi-échelle ;
- un théorème de descente qui ferme le problème des attaches laissé ouvert dans le premier rapport sous les hypothèses explicites de support, d'oracles exacts, de catalogue terminal et de traitement des lots données dans l'annexe B.

## 2. L'objet central : la bifiltration ordre–échelle

Soient $a_1(y)\leq\cdots\leq a_n(y)$ les distances euclidiennes au carré de $y$ aux observations, rangées par ordre croissant. Le champ HGP d'ordre $k$ est

$$D_k(y)=a_k(y).$$

Pour un seuil carré $t$, posons

$$L(k,t)=\left\lbrace y:D_k(y)\leq t\right\rbrace.$$

Deux monotonies coexistent :

$$t\leq t'\quad\Longrightarrow\quad L(k,t)\subseteq L(k,t'),$$

$$k\leq k'\quad\Longrightarrow\quad L(k',t)\subseteq L(k,t).$$

La famille $L(k,t)$ est donc une bifiltration, croissante en échelle et décroissante en ordre. Chaque inclusion induit une application canonique entre composantes connexes. À échelle fixée, toute composante d'ordre $k+1$ est contenue dans une unique composante d'ordre $k$.

### 2.1 La sortie n'est pas un arbre unique

Pour chaque $k$, la variation de $t$ donne une forêt de fusion ordinaire $T_k$. La variation de $k$ ajoute des morphismes compatibles

$$T_{k+1}\longrightarrow T_k.$$

En général, ce système à deux paramètres ne se réduit pas à un dendrogramme unique ni à un barcode à une dimension. La structure de sortie recommandée est une **forêt de fusion graduée** :

- une forêt de fusion $T_k$ à chaque ordre ;
- des identifiants d'événements partagés entre ordres ;
- des flèches verticales décrivant l'inclusion des composantes de $k+1$ dans celles de $k$ ;
- une politique explicite pour les événements simultanés.

Ce format répond à l'exigence « construire une seule structure jusqu'à $K=10$ ». Les dix forêts restent visibles, car elles représentent dix estimateurs différents, mais la géométrie, les candidats, les événements et les relations entre ordres ne sont plus recalculés indépendamment.

### 2.2 Pourquoi cette bifiltration est la bonne lecture de la thèse

La filtration de multicoverture du manuscrit contient déjà le paramètre de rayon et le paramètre de multiplicité. Les rhomboïdes de Corbet–Kerber–Lesnick–Osang réalisent précisément ce couplage ordre–échelle. La proposition présente consiste à ne conserver que son squelette nécessaire à $H_0$ : naissances, selles d'indice 1, attaches et inclusions verticales.

Cette réduction est différente d'une mosaïque d'ordre $10$ seule. Elle explique également pourquoi l'itération $k=1,\ldots,10$ est mathématiquement naturelle sans imposer dix constructions isolées : les événements d'un ordre deviennent les événements adjacents du suivant.

## 3. Une régularisation préférable : la lentille entropique homogène

La SoftLens additive du premier rapport est correcte du point de vue des composantes, mais sa température $\tau$ a l'unité d'une distance au carré. Un même $\tau$ ne régularise donc pas de la même manière les petites et les grandes branches d'une hiérarchie. Un changement d'unité $x\mapsto\lambda x$ exige de remplacer $\tau$ par $\lambda^2\tau$.

Il existe une variante plus naturelle pour un problème intrinsèquement multi-échelle. Pour $q\in[1,\infty)$, pour une facette $Q$ de cardinal $k$, définissons

$$\Phi_{Q,q}(y)=\left(\frac{1}{k}\sum_{x_i\in Q}\left\Vert y-x_i\right\Vert^{2q}\right)^{1/q}.$$

On pose par continuité

$$\Phi_{Q,\infty}(y)=\max_{x_i\in Q}\left\Vert y-x_i\right\Vert^2.$$

Le champ global garde le minimum extérieur dur :

$$F_{k,q}(y)=\min_{\substack{Q\subset X\\\lvert Q\rvert=k}}\Phi_{Q,q}(y)=\left(\frac{1}{k}\sum_{j=1}^{k}a_j(y)^q\right)^{1/q}.$$

Ainsi,

$$F_{k,1}(y)=\frac{1}{k}\sum_{j=1}^{k}a_j(y),\qquad F_{k,\infty}(y)=a_k(y).$$

L'extrémité $q=1$ est exactement la DTM empirique au carré ; l'extrémité $q=\infty$ est exactement HGP.

### 3.1 Il s'agit bien d'une régularisation KL

Fixons une longueur de référence $\ell_0>0$ et posons $\varepsilon=1/q\in(0,1]$. Pour les distances non nulles, la formule variationnelle du log-sum-exp donne

$$\log\frac{\Phi_{Q,1/\varepsilon}(y)}{\ell_0^2}=\max_{\alpha\in\Delta_Q}\left\lbrace\sum_{i\in Q}\alpha_i\log\frac{\left\Vert y-x_i\right\Vert^2}{\ell_0^2}-\varepsilon\,\mathrm{KL}(\alpha\Vert u_Q)\right\rbrace.$$

Avec $\log0=-\infty$, la convention étendue restreint le simplexe aux distances positives ; si elles sont toutes nulles, les deux membres valent $-\infty$. L'entropie régularise ici le maximum des **log-distances**. Cette position possède quatre avantages.

1. Le minimum extérieur sur les facettes reste dur ; la représentation par union d'atomes est conservée.
2. $\varepsilon$ est sans unité.
3. Pour tout $\lambda>0$,

$$\Phi_{\lambda Q,q}(\lambda y)=\lambda^2\Phi_{Q,q}(y).$$

4. Le chemin de continuation est compact : $\varepsilon=1$ donne la DTM et $\varepsilon=0$ donne HGP.

Le nom `HomogeneousSoftLens`, ou « lentille entropique homogène », évite de la confondre avec les sites de puissance de `PowerCover3D`.

### 3.2 Convexité des atomes et hiérarchie exacte

La racine carrée de $\Phi_{Q,q}$ s'écrit

$$\Phi_{Q,q}(y)^{1/2}=k^{-1/(2q)}\left(\sum_{x_i\in Q}\left\Vert y-x_i\right\Vert^{2q}\right)^{1/(2q)}.$$

C'est une norme mixte appliquée à l'application affine $y\mapsto(y-x_i)_{i\in Q}$. Elle est convexe ; son carré l'est également. Les sous-niveaux

$$A_{Q,q}(t)=\left\lbrace y:\Phi_{Q,q}(y)\leq t\right\rbrace$$

sont donc convexes, compacts lorsqu'ils sont non vides, et

$$\left\lbrace F_{k,q}\leq t\right\rbrace=\bigcup_{\lvert Q\rvert=k}A_{Q,q}(t).$$

Le théorème de swaps du premier rapport demeure valable. À $y$ fixé, il suffit de remplacer chaque coût $a_i$ par $z_i=a_i^q$. L'activité d'une facette est une borne sur la somme des $z_i$ ; les échanges négatifs puis positifs relient deux facettes actives sans dépasser la plus grande somme terminale. Sur l'atlas complet, les swaps élémentaires suffisent donc à représenter les composantes. Sur un atlas incomplet, la restriction formulée dans l'addendum GitHub demeure indispensable.

### 3.3 Monotonies et contrôle hiérarchique

Les moyennes de puissance donnent

$$F_{k,q}(y)\leq F_{k,q'}(y)\leq D_k(y)\qquad\text{pour }1\leq q\leq q'\leq\infty.$$

Ajouter le terme $a_{k+1}(y)^q$, supérieur ou égal à tous les précédents, augmente la moyenne. Par conséquent,

$$F_{k,q}(y)\leq F_{k+1,q}(y).$$

La bifiltration ordre–échelle est donc conservée à toute température entropique. Si $q$ est lui-même retenu comme paramètre, les sous-niveaux forment même une trifiltration : ils croissent avec $t$ et décroissent avec $k$ et $q$. En production, $q$ peut rester un paramètre d'homotopie interne plutôt qu'une troisième dimension de sortie. De plus,

$$k^{-1/q}D_k(y)\leq F_{k,q}(y)\leq D_k(y).$$

Cette relation donne une dilatation multiplicative, et non additive, des niveaux :

$$\left\lbrace D_k\leq t\right\rbrace\subseteq\left\lbrace F_{k,q}\leq t\right\rbrace\subseteq\left\lbrace D_k\leq k^{1/q}t\right\rbrace.$$

En variable rayon, le facteur maximal est $k^{1/(2q)}$. Pour $k=10$, il vaut environ $1{,}155$ à $q=8$, $1{,}075$ à $q=16$ et $1{,}037$ à $q=32$. Ces facteurs ne prouvent pas l'isomorphisme des arbres ; ils servent à choisir une continuation cohérente en échelle.

Pour $k=1$, le simplexe entropique est réduit à un point :

$$\Phi_{\left\lbrace x_i\right\rbrace,q}(y)=\left\Vert y-x_i\right\Vert^2,\qquad w_q(i,j)=\frac{1}{4}\left\Vert x_i-x_j\right\Vert^2.$$

La structure régularisée du 1-NN reste donc exactement l'EMST pour tout $q$. La régularisation devient non triviale à partir de $k=2$, sans casser ce point d'ancrage.

### 3.4 Comparaison des trois lissages envisagés

| construction | opération régularisée | DTM | conserve l'union de facettes | équivariant par changement d'échelle | rôle recommandé |
|---|---|---:|---:|---:|---|
| KL capé sur les voisins | sélection extérieure des voisins | oui | non à température positive | non | proposition ANN ou substitut déclaré |
| SoftLens additive | maximum interne des distances carrées | limite $\tau\to\infty$ | oui | si $\tau$ est redimensionné | référence analytique, bornes additives |
| lentille homogène | maximum interne des log-distances | exactement $q=1$ | oui | oui | continuation principale DTM $\to$ HGP |
| HGP dur | aucun | non | oui | oui | sortie exacte 3D |

La lentille homogène ne rend pas le problème à la fois exact et sparse en grande dimension. Elle fournit en revanche le meilleur compromis identifié jusqu'ici entre sens hiérarchique, DTM, absence de paramètre dimensionné et noyau GPU simple.

## 4. Théorème de descente facette–centre

Le premier rapport identifiait correctement un verrou : connaître les bras locaux d'une selle ne suffit pas ; il faut déterminer à quelles composantes globales ils appartiennent juste sous le niveau critique. Il envisageait une marche dans les cellules d'ordre $k$. Une construction plus simple suffit.

Pour une facette $Q$, notons

$$c_q(Q)=\mathop{\mathrm{argmin}}_y\Phi_{Q,q}(y),\qquad\beta_q(Q)=\Phi_{Q,q}\bigl(c_q(Q)\bigr).$$

À $q=\infty$, $c_\infty(Q)$ est le centre de la plus petite boule englobante de $Q$ et $\beta_\infty(Q)$ est son rayon au carré. Pour $q=1$, $c_1(Q)$ est le barycentre de $Q$ et $\beta_1(Q)$ sa variance empirique.

Considérons l'itération

$$Q_{m+1}=N_k\bigl(c_q(Q_m)\bigr),$$

où $N_k(y)$ désigne une facette de $k$ plus proches voisins de $y$, avec traitement déterministe ou par lot des ex æquo.

### 4.1 Version lisse, $1\leq q<\infty$

> **Théorème — descente atomique finie.** Supposons les écarts K-NN stricts aux centres visités. Si $Q_m$ n'est pas actif en son propre centre, alors
>
> $$\beta_q(Q_{m+1})<\beta_q(Q_m).$$
>
> Le segment de $c_q(Q_m)$ vers $c_q(Q_{m+1})$ reste dans un sous-niveau strictement inférieur à $\beta_q(Q_m)$. Comme il n'existe qu'un nombre fini de facettes, l'itération termine sur une facette $Q_\star$ active en son centre. Sous un écart strict, $c_q(Q_\star)$ est un minimum local du champ $F_{k,q}$.

**Preuve.** Au centre $c_q(Q_m)$, remplacer un point de $Q_m$ par un point strictement plus proche diminue strictement la somme des puissances. Par conséquent,

$$\Phi_{Q_{m+1},q}\bigl(c_q(Q_m)\bigr)<\Phi_{Q_m,q}\bigl(c_q(Q_m)\bigr)=\beta_q(Q_m).$$

La minimisation sur le nouvel atome donne

$$\beta_q(Q_{m+1})\leq\Phi_{Q_{m+1},q}\bigl(c_q(Q_m)\bigr)<\beta_q(Q_m).$$

Enfin, la convexité de $\Phi_{Q_{m+1},q}$ montre que tout son segment vers son minimiseur reste sous sa valeur au point de départ. Le champ global étant le minimum des atomes, il est lui aussi inférieur sur ce segment. La décroissance stricte interdit tout cycle. Au point fixe, l'atome minimisé est également l'atome globalement actif dans un voisinage, ce qui donne un minimum local. $\square$

Ce théorème est plus général que la lentille homogène : il vaut pour toute famille d'atomes convexes dont le meilleur atome à $y$ est la facette des $k$ plus proches voisins. Il s'applique notamment à la SoftLens additive.

### 4.2 Limite dure : descente K-NN–miniball

À $q=\infty$, posons

$$g_Q(y)=\max_{x_i\in Q}\left\Vert y-x_i\right\Vert^2.$$

Le remplacement d'un point frontière par un point strictement intérieur peut laisser $g_Q$ inchangé au centre courant, car plusieurs points frontière sont ex æquo. La valeur de miniball diminue néanmoins strictement sous l'hypothèse générique appropriée.

Pour chaque facette visitée, introduisons l'hypothèse $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ suivante : l'ensemble frontière complet de sa miniball est affine indépendant, constitue son unique support minimal, le centre appartient à son enveloppe convexe relative intérieure, et aucun point de $X$ extérieur à la facette n'est sur la sphère. Tous les points frontière sont alors essentiels.

> **Lemme — retrait d'un support essentiel.** Soit $U$ l'ensemble frontière complet de la miniball de $Q$ sous $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$. Si une facette $R$ de même cardinal est obtenue en retirant au moins un élément de $U$ et en n'ajoutant que des points strictement dans cette miniball, alors
>
> $$\beta_\infty(R)<\beta_\infty(Q).$$

En effet, chaque élément de $U$ possède une coordonnée barycentrique strictement positive et est essentiel à l'unique miniball. Après son retrait, l'ensemble restant est contenu dans la boule fermée, tous les nouveaux points sont strictement à l'intérieur, et l'égalité de rayon contredirait l'essentialité du support.

Si $Q$ n'est pas une facette K-NN en $c_\infty(Q)$, un point extérieur à $Q$ est strictement dans sa miniball. Une facette K-NN au centre retire nécessairement au moins un des points frontière les plus éloignés ; sous $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$, il est essentiel. Le lemme donne donc la décroissance stricte. On obtient :

> **Théorème — descente K-NN–miniball.** Si $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$ vaut pour chaque facette visitée et si les requêtes K-NN et miniballs sont exactes, l'itération
>
> $$Q_{m+1}=N_k\bigl(c_\infty(Q_m)\bigr)$$
>
> termine en un nombre fini d'étapes sur une configuration critique d'indice $0$. Chaque segment de descente reste sous le niveau précédent.

Le dernier point vient de la convexité de $g_{Q_{m+1}}$. Même si sa valeur au centre précédent est égale au niveau précédent, elle devient strictement plus petite dès que l'on avance vers son minimiseur, puisque sa valeur terminale est strictement inférieure.

### 4.3 Pourquoi cela calcule l'attache globale d'un bras

Soit $c$ une selle dure d'ordre $k$, de rayon carré $r^2$, d'intérieur $I$ et de support frontière $U$. Ses bras locaux sont

$$Q_u=I\cup\bigl(U\setminus\left\lbrace u\right\rbrace\bigr),\qquad u\in U.$$

Le support $U$ est minimal et bien centré. Retirer $u$ diminue strictement le rayon de miniball :

$$\beta_\infty(Q_u)<r^2.$$

Le segment du centre critique $c$ vers $c_\infty(Q_u)$ entre immédiatement dans le sous-niveau strict. Plus précisément, les dérivées directionnelles des distances aux points de $U\setminus\left\lbrace u\right\rbrace$ sont négatives. Comme $c$ est une combinaison convexe stricte de $U$, la dérivée de la distance au point exclu $u$ est positive ; le segment appartient donc bien au germe du bras $Q_u$.

On lance ensuite la descente K-NN–miniball depuis $Q_u$. Toute la trajectoire reste dans $\left\lbrace D_k<r^2\right\rbrace$ et finit sur un minimum. Si plusieurs minima ont déjà fusionné sous $r^2$, l'union–find des événements précédents les identifie à la même composante. Le représentant final donne donc exactement l'attache globale du bras.

La concaténation du segment initial et de tous les segments de descente est un chemin explicite dans le sous-niveau strict. Avec $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$, une règle top-$k$ cohérente, un catalogue complet et un minimum terminal présent dans ce catalogue, cette construction ferme le point laissé conditionnel dans la section 3.3 du premier rapport. Sans l'une de ces conditions, elle reste un oracle d'attache vérifiable, mais son statut global est conditionnel. Elle ne résout pas la complétude de l'énumération des selles ; elle résout l'autre moitié du problème : **une fois une selle connue, ses attaches ne requièrent ni mosaïque complète ni marche explicite dans les cellules d'ordre supérieur**.

### 4.4 Ex æquo et production

Les données réelles peuvent contenir des duplications, des cosphéricités et des niveaux égaux. Trois mécanismes sont requis.

- Les points sur la frontière d'une même boule sont traités comme un support complet, sans choisir arbitrairement quatre identifiants si la position générale échoue.
- Une transition avec plusieurs facettes K-NN ex æquo est un petit hypergraphe de plateau. On le résout en lot ou par perturbation symbolique documentée ; le théorème de décroissance stricte précédent ne s'applique pas tel quel à ce plateau.
- Les selles de même rayon sont attachées aux composantes strictement sous le niveau, puis contractées simultanément. L'ordre des threads CUDA ne doit jamais créer une binarisation artificielle.

## 5. Cas 3D : un unique échafaudage de Morse classé jusqu'à $K_{\max}$

Pour une sphère critique de centre $c$ et de rayon $r$, posons

$$I=X\cap B^\circ(c,r),\qquad U=X\cap\partial B(c,r),\qquad s=\lvert I\rvert+\lvert U\rvert.$$

Le nombre $s$ est le **rang fermé** de la sphère. La théorie de Morse du K-NN donne, à l'ordre $k$,

$$\mu_k(c)=s-k.$$

Sous position générale en dimension 3, $1\leq\lvert U\rvert\leq4$, et $c\in\mathrm{relint}\,\mathrm{conv}(U)$.

### 5.1 Théorème de réutilisation entre ordres

> **Proposition — échafaudage classé.** Une même sphère critique de rang fermé $s$ porte :
>
> - une naissance $H_0$ à l'ordre $k=s$ ;
> - une selle d'indice $1$ à l'ordre $k=s-1$, si $s\geq2$.
>
> Pour calculer toutes les hiérarchies $H_0$ jusqu'à $K_{\max}$, il suffit donc d'énumérer une fois les sphères critiques de rang $s\leq K_{\max}+1$.

La preuve est immédiate par $\mu_k=s-k$. Les mêmes sphères peuvent être critiques d'indice supérieur aux ordres plus bas, mais ces événements ne modifient pas $H_0$.

Pour $K_{\max}=10$, un enregistrement de rang $s\leq11$ contient au plus onze identifiants, alors que son support géométrique contient au plus quatre points. Il alimente les ordres adjacents comme suit.

| rang fermé $s$ | rôle à l'ordre $s$ | rôle à l'ordre $s-1$ | taille géométrique du support |
|---:|---|---|---:|
| $1$ | naissance de $T_1$ | aucun | $1$ |
| $2,\ldots,10$ | naissance de $T_s$ | selle de $T_{s-1}$ | $2$ à $4$ |
| $11$ | hors sortie si $K_{\max}=10$ | selle de $T_{10}$ | $2$ à $4$ |

Le catalogue `RankedMorseScaffold(K_max)` contient donc la géométrie commune ; les forêts $T_k$ ne sont que des vues filtrées de ce catalogue.

### 5.2 Les mêmes événements ancrent les morphismes verticaux

Juste après son niveau $r^2$, la petite composante qui naît à l'ordre $s$ est incluse dans la composante d'ordre $s-1$ contenant les bras de la même sphère. Si ces bras appartenaient à plusieurs composantes, la selle les fusionne ; s'ils appartenaient déjà à la même composante, l'événement ne change pas $H_0$ mais fournit tout de même le porteur de l'inclusion verticale.

L'identifiant partagé de la sphère ne suffit toutefois pas à décrire tout le morphisme de forêts de fusion. Dans le lot de niveau $r^2$, on attache la feuille qui naît à l'ordre $s$ à la composante **post-lot** d'ordre $s-1$ contenant les bras. On étend ensuite cette application aux ancêtres par compatibilité avec les inclusions en échelle. Si plusieurs événements sont simultanés, l'hypergraphe d'incidence complet du lot détermine la composante post-lot.

Il faut donc conserver dans l'échafaudage toutes les selles d'indice $1$, y compris celles que l'hyper-Kruskal élimine de la forêt comprimée parce qu'elles ne fusionnent rien. Elles fournissent des ancrages et subdivisions utiles aux morphismes $T_s\to T_{s-1}$, sans remplacer le calcul d'incidence du lot.

### 5.3 Pipeline mathématique exact après énumération et prédicats certifiés

Une fois le catalogue complet disponible, avec K-NN, miniballs et comparaisons de niveaux certifiés, la représentation restante est exacte et sparse en la taille de ce catalogue. Le temps de calcul dépend encore du coût des requêtes K-NN, des certificats et de la longueur des descentes ; aucune borne linéaire n'est implicite dans cette phrase.

1. Pour chaque sphère de rang $s\leq K_{\max}$, créer la naissance de l'ordre $s$.
2. Pour chaque sphère de rang $2\leq s\leq K_{\max}+1$, créer la selle de l'ordre $s-1$ et ses bras $Q_u$.
3. Attacher chaque bras par descente K-NN–miniball.
4. Trier les événements par couple `(ordre, rayon carré)` et traiter chaque niveau comme un hypergraphe simultané.
5. Construire les $T_k$ par union–find ; utiliser l'identifiant partagé comme ancrage, puis calculer l'attache post-lot et propager le morphisme par les relations d'ancêtre.
6. Conserver les points intérieurs/frontière comme porteurs critiques et garder l'index spatial pour reconstruire les recouvrements HGP à la demande. Les seuls événements critiques suffisent à la forêt de fusion continue, mais pas à énumérer toutes les facettes actives d'une coupe.

Pour $k=1$, les naissances de rang $1$ sont les observations. Les selles de rang $2$ sont les paires à boule diamétrale vide ; Kruskal conserve exactement un EMST. Le cas fondamental demandé dans la question initiale apparaît donc comme la première tranche de l'échafaudage classé, et non comme un cas spécial ajouté après coup.

### 5.4 Fermeture exacte par diagrammes de puissance ordinaires

Une sphère utile possède un rang fermé au plus $K_{\max}+1$, mais cela ne signifie pas que ses points frontière figurent dans une liste de $L$ voisins de chacun d'eux. Aucun $L=O(K)$ universel ne donne une énumération exacte. La solution retenue n'est donc pas une liste locale plus large : c'est une génération de colonnes avec certificat global.

Pour tout $Q\subseteq X$ de cardinal $k$, posons

$$b_Q=\frac{1}{k}\sum_{x\in Q}x,\qquad \omega_Q=\left\Vert b_Q\right\Vert^2-\frac{1}{k}\sum_{x\in Q}\left\Vert x\right\Vert^2.$$

La puissance du site $(b_Q,\omega_Q)$ vérifie

$$\pi_Q(y)=\left\Vert y-b_Q\right\Vert^2-\omega_Q=\frac{1}{k}\sum_{x\in Q}\left\Vert y-x\right\Vert^2.$$

Ses minimiseurs globaux sont exactement les labels top-$k$ en $y$. Partons d'un ensemble de colonnes $\mathcal S_k$ et construisons leurs cellules de puissance dans un polytope borné $\Omega\supset\mathrm{conv}(X)$. Pour chaque sommet $v$ de la cellule courante d'un label $Q$, une requête top-$k$ exacte fournit le shell complet des minimiseurs globaux. Un label strictement meilleur est inséré et redécoupe la cellule ; un label de même puissance est inséré pour reconstruire l'incidence non perturbée.

La différence $\pi_R-\pi_Q$ est affine. Par conséquent, si elle est positive ou nulle à tous les sommets de la cellule courante, elle l'est dans toute la cellule. L'absence de violation aux sommets est donc un **certificat exact de fermeture** contre les $\binom{n}{k}$ sites implicites. Chaque ronde non fermée ajoute une colonne ; la procédure termine en temps fini, même si sa borne générale reste combinatoire.

La récurrence d'Edelsbrunner–Osang fournit les colonnes initiales des ordres suivants. Elle est utilisée comme initialisation chaude, pas comme objet de stockage ni comme unique certificat. Une sphère ayant $i$ points strictement intérieurs et un support $U$ de taille $2$, $3$ ou $4$ apparaît déjà dans le diagramme fermé d'ordre $i+1$, sur une face de codimension $\lvert U\rvert-1$. Cet **ordre de détection** ne doit pas être confondu avec l'ordre $s-1$ où la même sphère est une selle d'indice $1$, avec $s=i+\lvert U\rvert$.

Deux modes internes seulement sont scientifiquement honnêtes : `power_closed`, lorsque toutes les cellules et incidences des ordres $1$ à $K_{\max}$ portent leur certificat, et `power_budgeted`, lorsque chaque événement retourné est validé mais que la complétude du catalogue reste inconnue. Le premier ne justifie le statut public `exact` qu'après fermeture des prédicats, attaches et lots ; le second impose `conditional`. Le résultat D donne le théorème et le contrat détaillés.

### 5.5 Complexité honnête

Même la Delaunay 3D ordinaire peut avoir une taille quadratique. Il n'existe donc pas de garantie universelle « exact, $50\,000$ points, moins d'une seconde ». Cette cible doit être testée sur des nuages volumétriques réguliers et rester conditionnée à des diagnostics mesurés ; la proximité du linéaire du certificat ne peut pas être postulée :

- nombre de supports proposés et acceptés par point ;
- profondeur et taux d'élagage du BVH ;
- nombre moyen d'étapes de descente par bras ;
- proportion de prédicats réévalués en précision adaptée ;
- taux de fermeture exacte de l'énumérateur ;
- taille finale du catalogue et des forêts.

## 6. Très grande dimension : une tour de blocs plutôt qu'un graphe UMAP

En grande dimension, l'énumération complète des facettes reste impossible dans le pire cas. Il faut néanmoins éviter deux pertes d'information : construire séparément les ordres et réduire trop tôt les interactions à des arêtes entre observations.

### 6.1 Atlas de blocs maximaux

On stocke une famille de blocs de cardinal $K_{\max}$,

$$\mathcal B\subseteq\binom{X}{K_{\max}},$$

et on lui associe sémantiquement sa fermeture descendante

$$\Downarrow\mathcal B=\left\lbrace Q\subseteq X:\exists B\in\mathcal B,\ Q\subseteq B,\ 1\leq\lvert Q\rvert\leq K_{\max}\right\rbrace.$$

À l'ordre $k$, l'atlas de facettes est

$$\mathcal A_k(\mathcal B)=\left\lbrace Q\in\Downarrow\mathcal B:\lvert Q\rvert=k\right\rbrace,$$

et le champ restreint est

$$F^{\mathcal B}_{k,q}(y)=\min_{Q\in\mathcal A_k(\mathcal B)}\Phi_{Q,q}(y).$$

Cette fermeture descendante est indispensable. Un unique préfixe arbitraire de chaque bloc ne garantit pas l'inclusion entre ordres lorsque le point d'évaluation change.

> **Proposition — bifiltration d'un atlas incomplet.** Pour tout atlas de blocs $\mathcal B$, même incomplet,
>
> $$F^{\mathcal B}_{k,q}(y)\leq F^{\mathcal B}_{k+1,q}(y).$$

En effet, soit $Q$ une face active de taille $k+1$. Retirons un point qui maximise $\left\Vert y-x_i\right\Vert^{2q}$. La moyenne des $k$ termes restants ne dépasse pas la moyenne initiale, et cette face appartient automatiquement à $\Downarrow\mathcal B$.

La hiérarchie restreinte est donc une vraie tour

$$T^{\mathcal B}_{1,q}\longleftarrow T^{\mathcal B}_{2,q}\longleftarrow\cdots\longleftarrow T^{\mathcal B}_{K_{\max},q}.$$

Si chaque observation appartient à au moins un bloc, la tranche $k=1$ contient tous les singletons et son champ restreint est exactement $D_1$. Sa hiérarchie de composantes admet donc l'EMST comme représentation mince. Le calcul retrouve effectivement cet EMST seulement si toutes les rencontres de paires sont couvertes par le graphe complet implicite ou par un oracle de coupe global certifié. Aux ordres supérieurs, la tour reste une sous-filtration de l'objet complet et peut manquer des naissances ou retarder des fusions.

### 6.2 Fermeture exacte, matérialisation paresseuse

Un bloc de dix points possède $2^{10}-1=1023$ faces. Il ne faut pas les stocker systématiquement. Pour un bloc $B$ et un point $y$, on trie les dix distances. Les préfixes de cet ordre local sont simultanément les meilleures faces de chaque cardinal $k$ **parmi les faces de $B$**, pour tout $q$.

Le contrat de données recommandé est donc :

- le bloc maximal est stocké comme dix identifiants ;
- une face éventuelle est un masque de dix bits ;
- le treillis booléen complet existe sémantiquement ;
- seuls les masques signalés comme minimum, événement de fusion, concurrent de coupe ou porteur vertical sont matérialisés ;
- une nouvelle requête critique ajoute le bloc $N_{K_{\max}}(y)$ et enrichit tous les ordres en une seule opération.

Cette dernière propriété remplace avantageusement la boucle indépendante $k=1,\ldots,10$. Un événement détecté à n'importe quel ordre appelle une seule requête top-$10$ ; ses faces deviennent immédiatement disponibles à tous les ordres inférieurs.

Cette paresse est une représentation, pas un certificat automatique. Un préfixe trié évalue le meilleur masque d'un bloc en un point donné ; il ne prouve pas à lui seul que toutes les naissances et rencontres de $\Downarrow\mathcal B$ ont été trouvées. Pour déclarer `atlas_exact` relativement à la fermeture sémantique, un oracle implicite doit séparer toutes les faces non matérialisées encore susceptibles de modifier une coupe. Sinon, l'atlas effectif exact est seulement le sous-atlas des faces matérialisées.

### 6.3 DTM, descente des minima et continuation

À $q=1$,

$$\Phi_{Q,1}(y)=\left\Vert y-b_Q\right\Vert^2+v_Q,$$

où $b_Q$ est le barycentre et $v_Q$ la variance de la facette. La descente facette–centre devient

$$Q_{m+1}=N_k(b_{Q_m}),$$

c'est-à-dire un mean-shift K-NN fini sur les facettes. À l'autre extrémité, $q=\infty$, elle devient la descente K-NN–miniball. Pour $1<q<\infty$, le centre est un centre de Fréchet de puissance $2q$.

Cette écriture concerne le champ complet. Sur un atlas fixé, l'étape choisit la meilleure face présente dans $\Downarrow\mathcal B$. Interroger le vrai $N_k$ global puis ajouter son top-$10$ est une étape de génération de colonne : elle améliore l'atlas, mais change l'objet restreint et impose de recalculer sa forêt.

Une stratégie de proposition cohérente est donc :

1. initialiser avec les blocs top-$10$ centrés aux observations et aux témoins disponibles ;
2. trouver en parallèle les minima stables à $q=1$ ;
3. dédupliquer leurs facettes et ajouter leurs top-$10$ comme blocs ;
4. continuer les centres selon $q=2,4,8,16,32,\infty$ ;
5. engendrer de nouveaux blocs aux changements de shell, aux selles et aux arêtes presque minimales de Borůvka.

Cette continuation est une heuristique de complétude, pas une preuve : des branches critiques peuvent apparaître entre deux valeurs de $q$ ou ne posséder aucun bassin atteint par les graines. Son intérêt est de concentrer les calculs sur les événements susceptibles de modifier la forêt.

### 6.4 Petit dual des naissances et rencontres

Pour $1<q<\infty$, posons $p=q/(q-1)$. La dualité des normes donne

$$\Phi_{Q,q}(y)=\max_{\substack{u\geq0\\\left\Vert u\right\Vert_p\leq k^{-1/q}}}\sum_{i\in Q}u_i\left\Vert y-x_i\right\Vert^2.$$

Définissons le poids de rencontre

$$w_q(Q,R)=\min_y\max\left\lbrace\Phi_{Q,q}(y),\Phi_{R,q}(y)\right\rbrace.$$

L'identité $\max\left\lbrace A,B\right\rbrace=\max_{\theta\in[0,1]}\left\lbrace\theta A+(1-\theta)B\right\rbrace$ et l'homogénéité du dual introduisent deux vecteurs $u,v\geq0$ satisfaisant

$$\left\Vert u\right\Vert_p+\left\Vert v\right\Vert_p\leq k^{-1/q}.$$

On prolonge $u$ par zéro hors de $Q$, $v$ par zéro hors de $R$, puis on pose $\eta_i=u_i+v_i$ sur $Q\cup R$, $s=\sum_i\eta_i$ et $m=\sum_i\eta_i x_i$. La compacité du domaine dual et la coercivité en $y$ justifient le minimax ; l'élimination analytique de $y$ donne

$$w_q(Q,R)=\max_{u,v}\left\lbrace\sum_i\eta_i\left\Vert x_i\right\Vert^2-\frac{\left\Vert m\right\Vert^2}{s}\right\rbrace,$$

sous les contraintes précédentes. La valeur est prolongée par $0$ en $s=0$ ; tout optimum non trivial vérifie $s>0$. Le problème possède au plus $2k\leq20$ variables, quelle que soit la dimension ambiante. Une solution duale faisable fournit une borne inférieure. Pour les mêmes coefficients avec $s>0$, le point $y=m/s$ donne un candidat ; c'est l'évaluation primale $\max\left\lbrace\Phi_{Q,q}(y),\Phi_{R,q}(y)\right\rbrace$ qui fournit la borne supérieure.

Les extrémités sont traitées séparément ou par limite continue : à $q=1$, on retrouve les sites de puissance et une formule fermée pour la rencontre ; à $q=\infty$, le dual de miniball. Une matrice de Gram de taille au plus $20\times20$ suffit à toutes les itérations ; la dimension ambiante intervient seulement dans sa construction.

### 6.5 L'usage le plus convaincant de l'entropie : certifier les coupes du problème dur

Pour toute facette et toute paire de facettes,

$$\Phi_{Q,q}\leq\Phi_{Q,\infty}\leq k^{1/q}\Phi_{Q,q},$$

$$\beta_q(Q)\leq\beta_\infty(Q)\leq k^{1/q}\beta_q(Q),$$

$$w_q(Q,R)\leq w_\infty(Q,R)\leq k^{1/q}w_q(Q,R).$$

Supposons qu'un solveur retourne un intervalle numérique $[L_q(e),U_q(e)]$ pour chaque arête candidate $e$ traversant une coupe de Borůvka. Si

$$k^{1/q}U_q(e)<\min_{f\neq e}L_q(f),$$

alors $e$ est l'unique arête minimale de cette coupe pour les poids HGP durs. On peut donc :

1. commencer à $q=1$ avec les poids de puissance très bon marché ;
2. raffiner seulement les coupes ambiguës à $q=2,4,8,16,32$ ;
3. résoudre le miniball dur uniquement lorsque les intervalles se recouvrent encore ;
4. accepter une arête seulement lorsque sa comparaison avec toutes les faces implicites et tous les blocs non visités de l'atlas déclaré est certifiée.

C'est l'analogue le plus proche de l'idée Sinkhorn : le problème entropique ne remplace pas la décision combinatoire finale, mais fournit des minorants, des majorants, des candidats et des états chauds permettant de certifier cette décision avec peu de passages au problème dur.

Le point crucial reste « toutes les faces et tous les blocs non visités ». CAGRA, IVF-PQ ou un graphe UMAP donnent d'excellents candidats, mais pas un minorant global sur les candidats absents. Si aucun index certifiable n'élague la coupe en grande dimension, Borůvka redevient quadratique. Pour passer de l'exactitude sur atlas à l'exactitude globale, il faudrait en outre certifier les blocs jamais ajoutés. Ce comportement est conforme à l'impossibilité mathématique du pire cas.

### 6.6 Exactitude sur atlas et exactitude globale

La fermeture descendante garantit la bifiltration entre ordres ; elle ne garantit pas que les swaps actifs restent dans l'atlas. Pour chaque $k$, un calcul exact sur l'atlas doit utiliser le graphe complet implicite des rencontres ou certifier les chemins minimax manquants.

Les trois statuts publics sont ceux du résultat E :

- `exact` : la tour globale est certifiée sur le périmètre déclaré, `full` ou `condensed(pi)` ;
- `atlas_exact` : les arbres et morphismes sont exacts pour l'atlas descendant identifié, sans conclusion globale automatique ;
- `conditional` : la conclusion globale dépend encore d'une hypothèse ou d'un oracle non fermé.

Les propriétés `hard_via_q`, `critical_complete`, `power_closed` et $\pi$-complétude sont des certificats ou des diagnostics internes. Elles justifient éventuellement `exact` dans un périmètre donné, mais ne créent pas de nouveaux statuts concurrents. Un rappel ANN élevé, l'absence de nouvelle colonne pendant quelques rondes ou la stabilité visuelle d'un dendrogramme ne changent jamais seuls `conditional` en `exact`.

### 6.7 Pourquoi UMAP n'est qu'une infrastructure de proposition

UMAP apporte trois briques utiles : une construction ANN, une échelle locale et un graphe navigable. Son objet final reste essentiellement un 1-squelette flou sur les observations. Ici, les interactions d'ordre supérieur sont portées explicitement par :

- une facette de $k$ points comme sommet de l'atlas d'ordre $k$ ;
- un bloc de dix points comme générateur de son treillis de faces ;
- une naissance dépendant conjointement des $k$ points ;
- une rencontre dépendant de jusqu'à $2k$ points ;
- un shell ex æquo donnant une hyperarête ;
- les morphismes de face entre ordres adjacents.

UMAP ou CAGRA peut donc proposer les blocs et apprendre éventuellement une métrique. La hiérarchie doit rester celle des atomes, des rencontres et de leur forêt ; sinon on reviendrait à estimer à l'ordre $K$ puis connecter à l'ordre $1$.

## 7. Architecture GPU commune

Les deux backends doivent partager l'orchestration et les primitives bas niveau, mais pas leur oracle géométrique global.

| primitive | 3D massive | grande dimension |
|---|---|---|
| candidats K-NN | index spatial ou cascade précédente, en proposition | cuVS CAGRA, IVF-PQ ou NN-descent en proposition |
| validation K-NN | oracle top-$k$/shell exact sur les observations originales | scan exhaustif sur petits cas ; sinon aucune certification globale générale |
| atome chaud | barycentre/DTM | barycentre/DTM |
| atome dur | miniball de $k\leq10$ points en 3D | miniball ou dual de dimension $\leq k$ dans un Gram |
| événement global | sphère critique de support $\leq4$ | rencontre de deux faces de l'atlas |
| complétude | cascade de puissance fermée par séparation top-$k$ aux sommets ; arrêt budgété conditionnel | impossible universellement ; atlas, certificats de coupes et statut explicites |
| sortie | échafaudage classé + dix forêts + morphismes | tour de blocs + dix forêts + morphismes |

### 7.1 Critère de compatibilité GPU

Une construction est retenue seulement si son travail dominant se décompose en lots parmi les familles suivantes : distances et produits scalaires, top-$k$ de petite taille, découpage indépendant de cellules convexes 3D, petits problèmes convexes sur au plus $2K_{\max}$ points, tris segmentés, réductions de coupes et union–find par lots de niveaux.

Cette liste est un contrat mathématique de granularité, pas un choix de bibliothèque. Les index ANN restent des producteurs de colonnes ; les parcours couvrants, les bornes primales–duales et les prédicats filtrés restent les décideurs. Le partage entre backends porte sur les lots, les intervalles et le format de sortie, tandis que leurs oracles globaux demeurent différents. CuPy, C++/CUDA, CUB, RMM ou cuVS seront évalués après validation des oracles, sans rétroagir sur les résultats A–E.

### 7.2 Arithmétique et statuts

La géométrie de proposition peut utiliser `float32`, voire `float16` ou BF16 pour l'index haute dimension. Les décisions de hiérarchie ne doivent pas hériter silencieusement de cette précision.

1. Conserver les rayons au carré ; aucune racine carrée dans le chemin combinatoire.
2. Retourner un intervalle $[w^-,w^+]$ pour chaque poids numériquement raffiné.
3. Utiliser `float64` pour les différences finales et un prédicat adaptatif lorsque l'intervalle contient zéro.
4. Regrouper les événements dont les intervalles restent superposés, au lieu de les ordonner arbitrairement.
5. Séparer les statuts `neighbor`, `candidate_completeness`, `predicate`, `attachment`, `event_order`, `meeting_graph` et `hierarchy`.

Un booléen global `certified=True` serait trop ambigu. Une arête peut avoir un poids certifié alors que l'atlas qui l'a proposée est incomplet.

## 8. Contrat mathématique GPU-friendly du backend 3D : `MorseHGP3D`

Cette section ne fige plus une architecture CUDA ni un budget en millisecondes. Elle identifie les primitives dont la sémantique est assez locale, régulière et batchable pour qu'une implémentation GPU soit plausible sans modifier l'objet mathématique.

### 8.1 `PowerCascadeH0`

À chaque ordre $k$, la cascade reçoit des labels $Q$, construit les cellules de puissance ordinaires de leurs sites $(b_Q,\omega_Q)$, interroge le top-$k$ exact aux sommets non fermés, insère les colonnes violatrices et répète. Les opérations d'un même lot de cellules sont indépendantes jusqu'aux tris de déduplication et d'incidence. L'ordre $k+1$ dépend des colonnes émises aux ordres précédents, mais tout le travail lourd à ordre fixé est parallèle.

Le contrat exact porte sur quatre objets : les cellules fermées, les classes complètes de minimiseurs ex æquo, les incidences internes appariées et la file vide des colonnes violatrices. La structure supérieure n'est jamais matérialisée. Les polyèdres d'un ordre peuvent être libérés dès que leurs incidences, leurs événements et leurs propositions inter-ordres ont été réduits.

### 8.2 Trois oracles globaux seulement

Le backend exact a besoin de trois requêtes sur les observations originales.

1. `top_k_shell(y,k)` renvoie les points strictement avant le rang $k$ et le shell complet au rang $k$ ; il sépare les cellules contre tous les labels implicites.
2. `closed_rank(c,r)` compte les points intérieurs et frontaux, avec arrêt dès que le rang dépasse $K_{\max}+1$ ; il valide une sphère proposée.
3. `interior_ids(c,r)` restitue les identifiants nécessaires aux bras, aux ancres verticales et à la relation de couverture.

Un BVH, une grille de hachage ou une autre structure spatiale peut réaliser ces requêtes. Ce choix appartient à l'implémentation future. Mathématiquement, le premier oracle décide la complétude géométrique et les deux autres décident le catalogue ; aucun audit échantillonné ne peut porter le même statut.

### 8.3 Supports critiques et descente

Les faces de codimensions $1$, $2$ et $3$ fournissent respectivement les supports de tailles $2$, $3$ et $4$. Leur traitement demande seulement de petits systèmes de Gram, des signes barycentriques, un test d'appartenance à la face et un comptage de rang. Pour $k\leq10$ en dimension $3$, une miniball de facette possède elle aussi un support d'au plus quatre points : la descente `AlternatingMEB` se ramène donc à des lots de petits problèmes indépendants suivis de requêtes top-$k$.

La valeur de miniball décroît strictement le long des transitions non stationnaires sous $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$. Cette monotonie donne un graphe d'états acyclique hors plateaux et autorise la mémoïsation, mais elle ne borne pas uniformément le nombre d'étapes. Les plateaux restent des lots, jamais des choix arbitraires de voisin.

### 8.4 Réduction hiérarchique

Les événements sont triés et réduits par la clé combinatoire de leur sphère, puis par lots de niveaux certifiés. Les attaches des bras sont ramenées aux composantes strictement sous le lot ; chaque composante connexe de l'hypergraphe du lot crée au plus une multifurcation. L'identifiant partagé d'une sphère relie sa naissance d'ordre $s$ à son ancre d'ordre $s-1$.

Cette dernière étape est composée de tris segmentés, de réductions et d'union–find. Elle est GPU-friendly, mais sa correction vient du résultat C : la concurrence d'exécution ne doit jamais imposer l'ordre mathématique de deux événements égaux.

### 8.5 Complexité et cible de latence

Pour l'ordre $k$, notons $M_k$ le nombre de cellules actives, $P_k$ le nombre d'incidences émises, $W_k$ le nombre de sommets testés par l'oracle, $J_k$ le nombre de nouvelles colonnes strictement violatrices ou co-minimisantes, $A_k$ le nombre de tests conservatifs colonne–cellule, $L_k$ le nombre de découpages et $Z_k$ le nombre d'événements acceptés. Le volume combinatoire pertinent est gouverné par

$$\sum_{k=1}^{K_{\max}}(M_k+P_k+W_k+J_k+A_k+L_k+Z_k),$$

et non par le seul nombre final de branches. Ce décompte n'est pas une borne de temps : il faut encore lui appliquer les coûts des top-$k$, découpages, tris, prédicats et comptages de rang. Rien n'empêche ces quantités d'être quadratiques dans le pire cas. La cible de moins d'une seconde pour $50\,000$ points en 3D est donc une hypothèse de régime volumétrique à tester, données déjà résidentes et décompte ci-dessus mesuré proche du linéaire ; elle n'est pas un corollaire du théorème de fermeture. Un arrêt par budget conserve la validité individuelle des événements, mais transforme obligatoirement le statut global en `conditional`.

## 9. Contrat mathématique GPU-friendly en grande dimension : `HomogeneousLensTower`

Le backend haute dimension n'essaie pas de reproduire la géométrie 3D. Il construit une tour exacte sur un atlas descendant, puis cherche à certifier le plus grand squelette condensé possible contre l'univers global. L'itération porte simultanément sur l'atlas et sur $q$, pas sur dix problèmes indépendants.

### 9.1 État sparse commun aux ordres

L'état primaire est une famille de blocs $B$ de cardinal $K_{\max}$, accompagnée sémantiquement de toutes leurs faces. Une face est matérialisée seulement si elle porte un minimum, une rencontre, une coupe concurrente ou une flèche verticale. Une requête top-$K_{\max}$ à un centre critique ajoute un bloc et enrichit ainsi tous les ordres en une seule opération.

Pour un couple `(point, bloc)`, le calcul des $K_{\max}$ distances, leur tri et les moyennes préfixes donnent en une passe les valeurs des meilleurs sous-blocs de chaque taille. Cette structure répétée, de taille fixée par $K_{\max}=10$ et non par la dimension ambiante, est la raison mathématique principale de la compatibilité GPU.

### 9.2 Petits problèmes convexes après les produits scalaires

Les naissances $\beta_q(Q)$ et les rencontres $w_q(Q,R)$ se calculent sur au plus $K_{\max}$ et $2K_{\max}$ observations. Après formation des produits scalaires, la dimension du problème convexe dépend de ces cardinalités, non de $d$. Les blocs partageant des observations réutilisent donc leurs matrices de Gram, et les valeurs à l'ordre $k$ ou à l'exposant $q$ précédent fournissent des initialisations cohérentes.

Le contrat du solveur n'est pas « converger rapidement » : il doit retourner des bornes inférieure et supérieure sur chaque niveau. Ces intervalles alimentent directement les lots, les coupes fondamentales et le statut de la hiérarchie. Le choix futur entre Newton projeté, miroir ou autre méthode n'a aucune portée topologique.

### 9.3 Boucle de colonnes hiérarchique

Une ronde conceptuelle suit cinq étapes.

1. Calculer exactement les dix forêts sur l'atlas courant et leurs flèches verticales.
2. Identifier les naissances, rencontres, attaches et coupes qui portent la condensation demandée.
3. Interroger en lot les familles absentes susceptibles de battre ces décisions.
4. Ajouter un bloc top-$K_{\max}$ à chaque témoin violateur et reprendre toutes les tranches affectées.
5. Arrêter seulement lorsque les certificats requis sont fermés, ou lorsque le budget impose le statut `conditional`.

Les dix union–find sont distinctes, puisque les dix tranches le sont, mais elles partagent blocs, produits scalaires, candidats et témoins de coupe. Une réduction de type Borůvka peut choisir les rencontres minimax à l'intérieur de l'atlas ; l'exactitude globale exige en plus les minorants couvrants de l'annexe E.

### 9.4 Rôle précis des méthodes ANN et de la continuation

UMAP, CAGRA, IVF-PQ ou NN-descent sont des générateurs de blocs et de concurrents. Un reranking exact corrige le poids des candidats trouvés, mais ne certifie pas l'absence d'un candidat meilleur. Ces méthodes ne changent donc jamais seules `conditional` en `exact`.

La continuation $q=1\to\infty$ joue un rôle différent : elle ordonne les cas difficiles, réutilise les solutions locales et fournit les intervalles multiplicatifs $[w_q,k^{1/q}w_q]$ pour le problème dur. Elle peut fermer tôt une coupe lorsque ces intervalles sont disjoints. Elle ne prouve pas qu'un événement absent de l'atlas n'existe pas.

### 9.5 Mesure de la difficulté réelle

La dimension ambiante commande essentiellement le coût des distances et des produits scalaires. La difficulté combinatoire se mesure plutôt par le nombre de blocs $M$, de faces matérialisées, de rencontres concurrentes, de rondes de colonnes et de coupes non fermées. Une complexité honnête doit publier ces quantités ainsi que la part du squelette condensé certifiée globalement.

Dans le pire cas, l'atlas nécessaire n'est pas sparse. La sortie correcte est alors une tour `atlas_exact` accompagnée d'une conclusion globale `conditional`, ou un refus d'atteindre le périmètre condensé demandé ; jamais une partition plate présentée comme substitut.

## 10. Comparaison avec l'implémentation GitHub la plus récente

La dernière modification du dépôt, `5a8d544`, ne change que le rapport. Le code GPU courant est celui de la série terminant à [`d67647`](https://github.com/Ludwig-H/E-HGP/commit/d67647). Le pipeline de `PowerCover3D` est orchestré par [`api.py`](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/api.py) :

1. normalisation isotrope en `float32` ;
2. index RBC 3D, ou brute force selon le chemin choisi ;
3. régularisation locale de chaque site $x_i\mapsto(z_i,a_i)$ ;
4. second index sur les sites déplacés $z_i$ ;
5. calcul du $K$-ième rayon de puissance aux centres d'une grille ;
6. Borůvka implicite sur la grille 26-connexe.

### 10.1 Ce qui est déjà bon

- [`cuda_runtime.py`](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/cuda_runtime.py) sur-échantillonne le RBC, recalcule les distances associées aux identifiants et possède un audit direct en `float64`.
- [`spatial_core.py`](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/spatial_core.py) fusionne les noyaux Gibbs et les valeurs de puissance, limite les allocations et expose des diagnostics.
- [`cubical_msf.py`](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/cubical_msf.py) évite la liste explicite des arêtes : il scanne treize voisins avant par voxel, compacte les parents et utilise des clés déterministes.
- La normalisation, RMM/CuPy, le budget VRAM, la sérialisation et les tests Blackwell sont des briques réutilisables.

L'audit RBC reste empirique et échantillonné ; il détecte des erreurs mais ne certifie pas chaque top-$k$. La fin du MSF copie encore les arêtes retenues sur CPU pour les trier, et chaque phase synchronise le compteur de sortie avec l'hôte. Ces points ne dominent pas les benchmarks actuels, mais ils devront disparaître du chemin événementiel visant moins d'une seconde. De même, la couverture des sites possède encore un chemin CPU : le nouveau contrat doit séparer la forêt continue de la reconstruction à la demande du recouvrement.

### 10.2 Ce qui doit rester dans le backend legacy

Le champ courant effectue un rang après lissage des sites. Il ne calcule ni la SoftLens du premier rapport, ni la lentille homogène, ni la multicouverture HGP dure. La grille transforme ensuite les événements continus en événements cubiques.

Avec la grille par défaut $128^3$, il existe $2\,097\,152$ voxels. Le balayage de treize voisins représente environ $27{,}3$ millions de tests d'arêtes par phase Borůvka, indépendamment du nombre réel d'événements critiques. Cette organisation peut être efficace comme approximation de champ, mais sa résolution et sa topologie restent liées au voxel.

La [campagne G4](https://github.com/Ludwig-H/E-HGP/blob/5a8d5445d0772efcc77082165dd558b2dfb82ea8/perg_hgp/benchmarks/results/G4_20260712_REPORT.md) confirme où se trouve le coût : après compilation, environ $1{,}13$ s en mode dur et $1{,}38$ s en mode local pour $100\,000$ points sur $48^3$ ; environ $159{,}23$ s pour $10$ millions en mode local ; $962{,}95$ s pour $30$ millions en mode local contre $33{,}56$ s en dur. Le MSF cubique lui-même ne prend qu'environ $0{,}31$ à $0{,}56$ s sur les grandes exécutions. Le changement proposé cible donc bien le voisinage, la régularisation et la représentation du champ, non une micro-optimisation de l'union–find.

Les éléments suivants doivent donc rester dans `power_cover_3d_cuda` :

- la régularisation `solve_entropy_gibbs` ;
- `GridSpec`, les centres de grille et l'évaluation cubique ;
- les scans 26-connexes ;
- la couverture boule–AABB spécifique aux voxels ;
- les sorties `base/inner/outer` de la forêt cubique.

### 10.3 Séparation mathématique recommandée

| étage actuel | `MorseHGP3D` | `HomogeneousLensTower` |
|---|---|---|
| RBC audité | proposition seulement ; oracle top-$k$ couvrant pour décider | générateur ANN de blocs, sans portée certifiante seul |
| Gibbs site par site | supprimé | remplacé par moyenne de puissance dans chaque face |
| champ de rang sur grille | cellules de puissance fermées puis sphères classées | atomes d'un atlas descendant |
| MSF cubique | lots d'attaches et hyper-Kruskal gradué | niveaux minimax et coupes certifiées |
| grille 26-connexe | aucune | aucune |
| `certified` global | statuts séparés | statuts séparés |

Le partage entre backends doit porter sur le format `GradedMergeForest`, les lots, les intervalles numériques et les statuts, pas sur une géométrie artificiellement commune. `PowerCover3D` constitue une référence cubique et une source de briques d'exécution ; il ne doit pas être renommé ni étendu silencieusement pour simuler l'un des nouveaux objets. L'arborescence de code et le choix des bibliothèques seront décidés après validation des résultats A–E.

## 11. Garanties portant directement sur la hiérarchie

Le critère de qualité ne doit pas être une distance Wasserstein entre champs. Il doit comparer les niveaux auxquels les points ou les branches deviennent connectés.

Pour un graphe pondéré $G$, notons $\beta(u)$ la naissance d'un sommet et $w(uv)$ le niveau brut d'une rencontre. Le poids effectif est $\omega(uv)=\max\left\lbrace w(uv),\beta(u),\beta(v)\right\rbrace$. Notons $\lambda_G(u,v)$ le plus petit niveau auquel $u$ et $v$ appartiennent à la même composante, c'est-à-dire la valeur minimax des $\omega$ le long d'un chemin. Soit $T$ un arbre couvrant candidat utilisant des arêtes de $G$ avec leurs poids effectifs hérités. Supposons que, pour chaque arête $e$ de $T$, la suppression de $e$ définisse une coupe et que

$$\omega_T(e)\leq(1+\delta)\min\left\lbrace\omega_G(f):f\text{ traverse cette coupe}\right\rbrace.$$

Alors

$$\lambda_G(u,v)\leq\lambda_T(u,v)\leq(1+\delta)\lambda_G(u,v)$$

pour tous $u,v$. La première inégalité vient de $T\subseteq G$. Pour la seconde, l'arête maximale du chemin de $T$ entre $u$ et $v$ définit une coupe que tout chemin de $G$ doit traverser.

Ce petit résultat fournit un contrat très utile, à condition de vérifier les **coupes fondamentales de l'arbre final**. Un simple oracle approché sur les composantes rencontrées pendant Borůvka ne suffit pas automatiquement ; il faut certifier ces coupes finales ou prouver séparément que la variante de Borůvka implique ce certificat. Combiné aux intervalles $[w_q,k^{1/q}w_q]$, ce test permet de publier une garantie multiplicative sur toute la hiérarchie, et pas seulement sur le poids total de l'arbre.

Pour l'objet continu global, un second niveau de garantie demeure nécessaire : l'atlas doit contenir les naissances et chemins pertinents. Une garantie condensée raisonnable est : tous les événements de persistance supérieure à $\pi$ sont présents, leurs attaches sont certifiées et les niveaux de leurs coupes satisfont l'inégalité précédente.

## 12. Programme expérimental qui peut invalider la proposition

Le prototype doit être conçu comme une expérience réfutable. Des mesures de temps sans oracle topologique ne suffiraient pas.

### 12.1 Oracle exact de petite taille

Construire d'abord un backend CPU lent qui :

- énumère toutes les facettes pour de petits $n$ ;
- calcule les naissances et toutes les rencontres du graphe complet ;
- énumère en 3D toutes les sphères supportées par un à quatre points et vérifie leur rang ;
- produit les dix forêts de fusion et leurs morphismes ;
- conserve les lots exacts d'événements égaux.

Cet oracle doit vérifier :

1. $k=1$ contre un EMST exact ;
2. $q=1$ contre les sites de puissance ;
3. $q=\infty$ contre HGP dur ;
4. monotonie en $k$ et en $q$ ;
5. bijection naissance/selle du catalogue classé ;
6. maintien sous-niveau et terminaison de chaque descente ;
7. commutation des morphismes entre ordres et échelles ;
8. nécessité éventuelle d'arêtes non-swap sur un atlas arbitraire.

### 12.2 Validation du backend 3D

| famille de données | but |
|---|---|
| Poisson volumétrique | régime moyen favorable attendu |
| mélanges 3D à densités et cols contrôlés | ordre des naissances et fusions |
| LiDAR surfacique | détecter l'explosion des supports peu profonds |
| moment curve, couches cosphériques, duplications | pire cas et dégénérescences |
| $n$ petit avec oracle complet | exactitude événement par événement |

Mesurer séparément le temps froid, le temps chaud, les copies, la construction de l'index, le nombre de propositions, le rappel des sphères critiques, le nombre de nœuds BVH visités, les itérations d'attache, le taux de fallback précis et la taille des forêts.

Le test décisif pour la cible $50\,000$ points n'est pas seulement « moins d'une seconde ». Il est : moins d'une seconde **avec le statut annoncé**, un taux de fermeture connu et une comparaison événement par événement à la référence sur des sous-échantillons.

### 12.3 Validation grande dimension

Comparer quatre sorties : DTM $q=1$, calendriers intermédiaires, HGP dur sur atlas et référence exhaustive lorsque possible. Les mesures principales sont :

- rappel des naissances et fusions persistantes ;
- distorsion des niveaux minimax $\lambda(u,v)$ ;
- nombre de blocs et de faces réellement matérialisées ;
- part des coupes certifiées à chaque $q$ ;
- nombre de miniballs durs finalement nécessaires ;
- divergence entre `atlas_exact` et la référence globale ;
- stabilité des morphismes $T_{k+1}\to T_k$ ;
- temps et mémoire de CAGRA séparés du temps de hiérarchie.

L'exemple perturbé de la base canonique doit figurer dans les tests. Il doit provoquer une croissance explosive ou un statut `conditional`, et non une fausse réussite sparse.

## 13. Résultats mathématiques désormais séparés

Les cinq objectifs précédents sont développés comme annexes autonomes dans [`docs/math/`](docs/math/README.md). Ce découpage distingue les théorèmes, les conséquences conditionnelles et les conjectures qui doivent encore être tranchées.

### [Résultat A — lentilles homogènes](docs/math/RESULTAT_A_LENTILLES_HOMOGENES.md)

L'annexe formalise l'écriture KL logarithmique, la convexité des atomes, la bifiltration, la suffisance des swaps sur l'atlas complet et les limites DTM/HGP.

### [Résultat B — descente atomique](docs/math/RESULTAT_B_DESCENTE_ATOMIQUE.md)

L'annexe sépare le théorème lisse, la limite miniball sous support essentiel, les dégénérescences et les conditions qui transforment une descente locale en certificat d'attache globale.

### [Résultat C — complexe de Morse bifiltré](docs/math/RESULTAT_C_COMPLEXE_MORSE_BIFILTRE.md)

L'annexe définit la sortie `GradedMergeForest`, les événements classés, les lots de niveaux égaux et les morphismes verticaux entre ordres.

### [Résultat D — énumération 3D par diagrammes de puissance](docs/math/RESULTAT_D_ENUMERATION_3D_POWER_GPU.md)

La piste unique retenue adapte la récurrence incrémentale des mosaïques à une cascade de diagrammes de puissance 3D ordinaires. Ni rhomboid tiling ni mosaïque d'ordre supérieur ne devient une structure du backend. Le lemme de séparation aux sommets donne désormais un certificat exact de fermeture de chaque cellule contre tous les labels top-$k$ implicites. La taille de ce certificat et l'exclusion directe des cellules inutiles à $H_0$ restent ouvertes ; un arrêt budgété demeure conditionnel.

### [Résultat E — récupération condensée par coupes](docs/math/RESULTAT_E_RECUPERATION_CONDENSEE_COUPES.md)

L'annexe formule une récupération directement hiérarchique en termes de niveaux minimax, de coupes fondamentales, de marges critiques et de persistance minimale, sans détour par une distance Wasserstein entre champs.

## 14. Feuille de route mathématique avant implémentation

### Étape 0 — objet et oracle

- figer `GradedMergeForest`, les lots de niveaux et les statuts ;
- écrire un oracle exhaustif de petite taille pour les multicovertures, les attaches et les morphismes ;
- vérifier que $k=1$ redonne exactement l'EMST et son arbre de fusion.

### Étape 1 — audit des preuves locales

- relire les preuves de A et B en incluant les zéros, ex æquo et supports non essentiels ;
- tester la descente contre l'oracle sur toutes les petites configurations ;
- formaliser les événements simultanés de C comme morphismes de composantes, et non comme ordre arbitraire d'arêtes.

### Étape 2 — éprouver et affiner le résultat D

- vérifier exhaustivement le lemme de fermeture et la reconstruction des incidences sur de petits nuages ;
- formaliser le traitement stratifié des cosphéricités et duplications au-delà de la position générale ;
- distinguer dans les tests l'ordre de détection $i+1$ de l'ordre de selle $s-1$ ;
- chercher un oracle d'exclusion qui prouve l'absence d'événement $H_0$ sans fermer toutes les cellules ;
- borner ou mesurer séparément la taille du certificat de cascade et celle du catalogue $H_0$.

### Étape 3 — rendre le résultat E effectif

- caractériser des familles de données où la $\pi$-complétude possède un certificat sparse ;
- construire un oracle d'exclusion global pour les branches absentes, et pas seulement pour les arêtes proposées ;
- vérifier les marges de niveau, de coupe, d'attache et d'ordre contre l'oracle exhaustif ;
- construire des contre-exemples qui forcent le statut `conditional` et mesurer la frontière de certification.

### Étape 4 — porte d'entrée vers le GPU

La spécification des noyaux ne commence qu'après les étapes précédentes. Elle devra montrer que les oracles retenus se ramènent à des lots de diagrammes de puissance 3D, de top-$k$, de petits problèmes convexes, de tris segmentés et de réductions de coupes. Les détails d'implémentation ne doivent pas modifier les objets définis par A–E.

## 15. Décision finale par régime

### Données massives en 3D, $K_{\max}=10$

La sortie scientifique doit rester la hiérarchie dure HGP. La bonne architecture est

$$\boxed{\text{sphères critiques de rang }s\leq11\ \longrightarrow\ \text{descente K-NN–miniball}\ \longrightarrow\ \text{hyper-Kruskal gradué}.}$$

L'entropie et la DTM servent à proposer et ordonner, pas à redéfinir la sortie. Le même catalogue calcule tous les ordres. La descente des attaches est démontrée sous $(\mathrm{GP}_{\partial}^{\mathrm{desc}})$, avec top-$k$, miniballs, catalogue terminal et lots d'ex æquo certifiés. La fermeture par diagrammes de puissance ordinaires est exacte sous les hypothèses du résultat D ; le grand problème restant est de la rendre sensible aux seuls événements $H_0$ utiles, sans fermer une sortie intermédiaire potentiellement quadratique.

Pour $50\,000$ points, un temps inférieur à une seconde est une cible de recherche à tester en régime volumétrique et données résidentes GPU, et seulement lorsque le certificat entier reste mesuré proche du linéaire. Lorsque le budget expire, le backend doit conserver les événements individuellement validés et rendre le statut global `conditional`, sans redéfinir silencieusement la hiérarchie.

### Très grande dimension

La bonne structure calculable est

$$\boxed{\text{blocs }K_{\max}\ \longrightarrow\ \text{fermeture descendante implicite}\ \longrightarrow\ q=1\to\infty\ \longrightarrow\ \text{Borůvka gradué}.}$$

La lentille homogène est préférable à la SoftLens additive comme continuation principale. Elle conserve la hiérarchie, possède la DTM comme extrémité exacte, partage une seule requête top-$10$ entre tous les ordres et permet de certifier de nombreuses coupes du problème dur avant de résoudre leurs miniballs.

Il ne faut cependant pas promettre une représentation globale exacte et sparse. La sortie honnête est une tour `atlas_exact`, accompagnée soit d'un périmètre global `condensed(pi)` entièrement certifié, soit d'un statut global `conditional`. UMAP et CAGRA restent dans la couche de proposition.

### Réponse synthétique à l'intuition initiale

L'analogie avec Sinkhorn est valide, mais à un endroit plus local que dans le transport optimal :

> **l'entropie ne remplace pas la hiérarchie combinatoire ; elle transforme chaque comparaison locale de pire voisin en un oracle lisse, homogène, batchable et muni de bornes. La hiérarchie globale reste une forêt dure d'événements.**

C'est précisément ce partage des rôles qui rend la proposition à la fois mathématiquement fidèle à la thèse et réaliste sur GPU.

## Références principales

- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024.
- R. Corbet, M. Kerber, M. Lesnick et G. Osang, [*Computing the Multicover Bifiltration*](https://arxiv.org/abs/2103.07823), 2021–2022.
- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](https://arxiv.org/abs/2011.03617), 2020–2022.
- Á. J. Alonso, [*A Sparse Multicover Bifiltration of Linear Size*](https://arxiv.org/abs/2411.06986), version 2025.
- M. Buchet, B. B. Dornelas et M. Kerber, [*Sparse Higher Order Čech Filtrations*](https://arxiv.org/abs/2303.06666), 2023–2024.
- F. Chazal, D. Cohen-Steiner et Q. Mérigot, [*Geometric Inference for Probability Measures*](https://doi.org/10.1007/s10208-011-9098-0), 2011.
- B. Taveira et al., [*Scalable GPU Construction of 3D Voronoi and Power Diagrams*](https://arxiv.org/abs/2605.06408), 2026.
- A. Prokopenko, P. Sao et D. Lebrun-Grandié, [*A single-tree algorithm to compute the Euclidean minimum spanning tree on GPUs*](https://arxiv.org/abs/2207.00514), 2022.
- L. McInnes, J. Healy et J. Melville, [*UMAP: Uniform Manifold Approximation and Projection for Dimension Reduction*](https://arxiv.org/abs/1802.03426), 2018–2020.
- NVIDIA RAPIDS, [documentation officielle cuVS CAGRA](https://docs.rapids.ai/api/cuvs/stable/neighbors/cagra/) et [brute force](https://docs.rapids.ai/api/cuvs/stable/neighbors/bruteforce/), version stable 26.06.
