# Résultat A — Lentilles entropiques homogènes et hiérarchie ordre–échelle

> **Statut.** Ce document établit les propriétés de représentation de la lentille homogène sur l'atlas complet. Il distingue ces théorèmes finis et déterministes du programme algorithmique sparse, qui demeure conditionnel.

## 1. Objet et conventions

Soit $X=(x_1,\ldots,x_n)$ une famille finie de points de $\mathbb{R}^{d}$. Les répétitions éventuelles sont autorisées. Fixons $1\leq k\leq n$ et posons

$$\mathcal{Q}_k=\left\lbrace Q\subseteq\lbrace 1,\ldots,n\rbrace:\lvert Q\rvert=k\right\rbrace.$$

Pour $Q\in\mathcal{Q}_k$, $q\in[1,\infty)$ et $y\in\mathbb{R}^{d}$, on définit l'atome homogène

$$\Phi_{Q,q}(y)=\left(\frac{1}{k}\sum_{i\in Q}\left\Vert y-x_i\right\Vert^{2q}\right)^{1/q},$$

et sa limite dure

$$\Phi_{Q,\infty}(y)=\max_{i\in Q}\left\Vert y-x_i\right\Vert^{2}.$$

Le champ d'ordre $k$ est l'enveloppe inférieure de ces atomes :

$$F_{k,q}(y)=\min_{Q\in\mathcal{Q}_k}\Phi_{Q,q}(y).$$

Toutes les valeurs sont des distances carrées. Pour $t\geq0$, l'atome au niveau $t$, son niveau de naissance et le niveau de rencontre de deux atomes sont

$$A_{Q,q}(t)=\left\lbrace y\in\mathbb{R}^{d}:\Phi_{Q,q}(y)\leq t\right\rbrace,$$

$$\beta_q(Q)=\min_{y\in\mathbb{R}^{d}}\Phi_{Q,q}(y),$$

$$w_q(Q,R)=\min_{y\in\mathbb{R}^{d}}\max\left\lbrace\Phi_{Q,q}(y),\Phi_{R,q}(y)\right\rbrace.$$

La normalisation par $1/k$ est essentielle : elle fait de $q=1$ la DTM empirique carrée et rend comparables les ordres successifs.

## 2. Théorème principal

> **Théorème A — représentation homogène de la hiérarchie.** Pour tout nuage fini $X$, sans hypothèse de position générale, les assertions suivantes sont vraies.
>
> 1. Si $a_1(y)\leq\cdots\leq a_n(y)$ sont les distances carrées $\left\Vert y-x_i\right\Vert^2$ rangées avec multiplicité, alors
>
> $$F_{k,q}(y)=\left(\frac{1}{k}\sum_{j=1}^{k}a_j(y)^q\right)^{1/q},\qquad F_{k,\infty}(y)=a_k(y).$$
>
> 2. Chaque $\Phi_{Q,q}$ est convexe, continue et coercive. Pour $q<\infty$, son centre $c_q(Q)$ est unique. Pour $q=\infty$, le centre de la plus petite boule englobante est également unique. Les ensembles $A_{Q,q}(t)$ sont donc convexes et compacts lorsqu'ils sont non vides.
>
> 3. Le sous-niveau global est exactement une union finie d'atomes convexes :
>
> $$L(k,q,t)=\left\lbrace F_{k,q}\leq t\right\rbrace=\bigcup_{Q\in\mathcal{Q}_k}A_{Q,q}(t).$$
>
> 4. La famille est croissante en échelle et décroissante en ordre et en dureté : si $t\leq t'$, $k\leq k'$ et $q\leq q'$, alors
>
> $$L(k,q,t)\subseteq L(k,q,t'),\qquad L(k',q,t)\subseteq L(k,q,t),\qquad L(k,q',t)\subseteq L(k,q,t).$$
>
> 5. Elle est homogène de degré deux. Pour $\lambda>0$, si $\lambda X=(\lambda x_i)_i$, alors
>
> $$F^{\lambda X}_{k,q}(\lambda y)=\lambda^2F^{X}_{k,q}(y).$$
>
> 6. Les deux extrémités sont exactement la DTM empirique carrée et le champ HGP :
>
> $$F_{k,1}(y)=\frac{1}{k}\sum_{j=1}^{k}a_j(y),\qquad \lim_{q\to\infty}F_{k,q}(y)=D_k(y)=a_k(y).$$
>
> 7. Pour $1\leq q<\infty$, on dispose des encadrements multiplicatifs
>
> $$k^{-1/q}D_k(y)\leq F_{k,q}(y)\leq D_k(y),$$
>
> $$k^{-1/q}\beta_\infty(Q)\leq\beta_q(Q)\leq\beta_\infty(Q),$$
>
> $$k^{-1/q}w_\infty(Q,R)\leq w_q(Q,R)\leq w_\infty(Q,R).$$
>
> 8. Sur l'atlas complet $\mathcal{Q}_k$, les swaps élémentaires suffisent à représenter les composantes de $L(k,q,t)$. Toute forêt couvrante minimale du graphe de swaps pondéré par $w_q$ préserve ces composantes à tous les niveaux, à condition de conserver séparément les naissances $\beta_q$.

Les assertions 1 à 8 sont des résultats de représentation exacts. Elles ne disent pas que $\mathcal{Q}_k$ peut être matérialisé en grande dimension.

## 3. La régularisation est une pénalisation KL à température sans dimension

Fixons une longueur de référence $\ell_0>0$ et posons $\varepsilon=1/q$. Si toutes les distances aux points de $Q$ sont strictement positives, la formule de Gibbs donne

$$\log\frac{\Phi_{Q,1/\varepsilon}(y)}{\ell_0^2}=\max_{\alpha\in\Delta_Q}\left\lbrace\sum_{i\in Q}\alpha_i\log\frac{\left\Vert y-x_i\right\Vert^2}{\ell_0^2}-\varepsilon\,\mathrm{KL}(\alpha\Vert u_Q)\right\rbrace,$$

où

$$\Delta_Q=\left\lbrace\alpha\in\mathbb{R}_{+}^{Q}:\sum_{i\in Q}\alpha_i=1\right\rbrace,\qquad (u_Q)_i=\frac{1}{k}.$$

Le maximiseur est

$$\alpha_i^\star(y)=\frac{\left\Vert y-x_i\right\Vert^{2q}}{\sum_{j\in Q}\left\Vert y-x_j\right\Vert^{2q}}.$$

**Preuve.** Pour $b_i=\log(\left\Vert y-x_i\right\Vert^2/\ell_0^2)$, l'identité élémentaire

$$\varepsilon\log\left(\frac{1}{k}\sum_{i\in Q}e^{b_i/\varepsilon}\right)=\max_{\alpha\in\Delta_Q}\left\lbrace\sum_{i\in Q}\alpha_i b_i-\varepsilon\,\mathrm{KL}(\alpha\Vert u_Q)\right\rbrace$$

s'obtient en écrivant le membre de droite comme sa valeur au Gibbs $\alpha^\star$ moins $\varepsilon\,\mathrm{KL}(\alpha\Vert\alpha^\star)$. Le membre de gauche est précisément $\log(\Phi_{Q,1/\varepsilon}(y)/\ell_0^2)$. Si certaines distances sont nulles, on pose $\log0=-\infty$ et l'on adopte les conventions $0\cdot(-\infty)=0$ et $\alpha_i\log0=-\infty$ pour $\alpha_i>0$. Le maximum se restreint alors à la face portée par les distances positives, et la formule de Gibbs reste valable en donnant une masse nulle aux indices de distance nulle. Si toutes les distances sont nulles, les deux membres valent $-\infty$ et le quotient affiché pour $\alpha^\star$ n'est pas défini. $\square$

Cette écriture fixe la place de l'entropie : elle régularise le choix du point contraignant **à l'intérieur d'une facette**. Le minimum extérieur sur $Q$ reste dur. Le coefficient $\varepsilon$ est sans dimension, et la référence $\ell_0$ rend les logarithmes eux-mêmes sans dimension. À $\ell_0$ fixé, une homothétie ajoute $2\log\lambda$ à tous les gains, sans changer $\alpha^\star$.

À $\varepsilon=1$, le Gibbs est proportionnel aux distances carrées et l'atome est la moyenne des distances carrées de la DTM. Lorsque $\varepsilon\downarrow0$, la masse se concentre sur les points les plus éloignés et l'atome tend vers la lentille HGP dure.

## 4. Convexité, centres et union d'atomes

Pour $q<\infty$, posons

$$H_{Q,q}(y)=\Phi_{Q,q}(y)^{1/2}=k^{-1/(2q)}\left(\sum_{i\in Q}\left\Vert y-x_i\right\Vert^{2q}\right)^{1/(2q)}.$$

L'application $y\mapsto(y-x_i)_{i\in Q}$ est affine et la dernière expression est sa norme mixte $\ell_{2q}(\ell_2)$. Ainsi $H_{Q,q}$ est convexe. Comme $s\mapsto s^2$ est convexe et croissante sur $\mathbb{R}_{+}$, $\Phi_{Q,q}=H_{Q,q}^2$ est convexe.

Une preuve directe de l'unicité du centre est utile. Minimiser $\Phi_{Q,q}$ équivaut à minimiser

$$S_{Q,q}(y)=\sum_{i\in Q}\left\Vert y-x_i\right\Vert^{2q}.$$

La fonction $z\mapsto\left\Vert z\right\Vert^{2q}$ est strictement convexe pour $q\geq1$ ; sa somme après translations l'est donc aussi. Comme elle est coercive, elle possède un unique minimiseur. À $q=\infty$, l'unicité du centre de la plus petite boule euclidienne est classique et découle aussi de l'identité du parallélogramme.

La convexité et la coercivité donnent immédiatement la convexité et la compacité des sous-niveaux atomiques. Enfin,

$$F_{k,q}(y)\leq t\quad\Longleftrightarrow\quad\text{il existe }Q\in\mathcal{Q}_k\text{ tel que }\Phi_{Q,q}(y)\leq t,$$

ce qui prouve l'union d'atomes. Cette égalité, et non une approximation du champ sur une grille, est l'invariant topologique fondamental.

Au pôle DTM, l'atome prend la forme d'une boule de puissance. Si $b_Q=k^{-1}\sum_{i\in Q}x_i$ et $v_Q=k^{-1}\sum_{i\in Q}\left\Vert x_i-b_Q\right\Vert^2$, alors

$$\Phi_{Q,1}(y)=\left\Vert y-b_Q\right\Vert^2+v_Q.$$

Au pôle HGP, $A_{Q,\infty}(t)$ est l'intersection des $k$ boules fermées de rayon $\sqrt{t}$ centrées aux points de $Q$. Les valeurs intermédiaires déforment continûment ces atomes sans perdre leur convexité.

## 5. Identité de rang et monotonies

À $y$ fixé, minimiser $\Phi_{Q,q}(y)$ revient à choisir les $k$ plus petites valeurs parmi les $a_i(y)^q$. Cela prouve l'identité de rang du théorème, y compris en présence d'ex aequo.

La monotonie en $q$ est l'inégalité des moyennes de puissance :

$$1\leq q\leq q'\leq\infty\quad\Longrightarrow\quad F_{k,q}(y)\leq F_{k,q'}(y).$$

Pour la monotonie en ordre, la moyenne des $k$ premières valeurs ne peut diminuer lorsqu'on lui ajoute $a_{k+1}(y)^q$, qui est supérieur ou égal à chacun de ses termes :

$$F_{k,q}(y)\leq F_{k+1,q}(y).$$

Ces inégalités ponctuelles donnent les inclusions de sous-niveaux. La sortie naturelle est donc une famille de forêts de fusion $T_{k,q}$ munie de morphismes canoniques

$$T_{k+1,q}\longrightarrow T_{k,q},\qquad T_{k,q'}\longrightarrow T_{k,q}\quad\text{pour }q\leq q'.$$

Le paramètre $q$ peut servir seulement de continuation interne ; il n'est pas nécessaire de publier une trifiltration. En revanche, aucune continuation ne doit casser les morphismes ordre–échelle.

Les naissances et rencontres héritent aussi de la monotonie :

$$q\leq q'\quad\Longrightarrow\quad\beta_q(Q)\leq\beta_{q'}(Q),\qquad w_q(Q,R)\leq w_{q'}(Q,R).$$

Cette monotonie n'empêche pas deux événements distincts d'échanger leur ordre relatif lorsque $q$ varie.

## 6. Limites DTM et HGP

Pour des nombres non négatifs $z_1,\ldots,z_k$, on a

$$k^{-1/q}\max_i z_i\leq\left(\frac{1}{k}\sum_{i=1}^{k}z_i^q\right)^{1/q}\leq\max_i z_i.$$

Appliquée aux distances carrées, cette inégalité donne

$$k^{-1/q}\Phi_{Q,\infty}(y)\leq\Phi_{Q,q}(y)\leq\Phi_{Q,\infty}(y).$$

Prendre le minimum sur les facettes prouve l'encadrement de $F_{k,q}$. Prendre le minimum sur $y$ prouve celui de $\beta_q$. Prendre le minimum sur $y$ après le maximum des deux atomes prouve celui de $w_q$.

Il en résulte les inclusions hiérarchiques

$$\left\lbrace D_k\leq t\right\rbrace\subseteq\left\lbrace F_{k,q}\leq t\right\rbrace\subseteq\left\lbrace D_k\leq k^{1/q}t\right\rbrace.$$

En paramètre rayon, la dilatation est $k^{1/(2q)}$. Cette borne est uniforme et multiplicative ; elle reste pertinente sur un espace non borné. Elle ne prouve pas que les arbres de fusion ont la même combinatoire pour un $q$ fini.

Pour la mesure empirique uniforme $\mu_X=n^{-1}\sum_i\delta_{x_i}$ et la masse $m=k/n$, la définition standard de la distance à une mesure donne, avec la convention empirique usuelle,

$$d_{\mu_X,m}(y)^2=F_{k,1}(y).$$

À l'autre extrémité, $F_{k,\infty}=D_k$ et

$$\left\lbrace D_k\leq t\right\rbrace=\left\lbrace y:\#\bigl(X\cap B(y,\sqrt{t})\bigr)\geq k\right\rbrace,$$

qui est exactement le niveau de multicouverture utilisé par HGP.

## 7. Théorème des swaps

Deux facettes $Q,R\in\mathcal{Q}_k$ diffèrent par un swap si $\lvert Q\setminus R\rvert=\lvert R\setminus Q\rvert=1$. Pour un niveau $t$, soit $G^{\mathrm{swap}}_{k,q}(t)$ le graphe dont les sommets sont les $Q$ tels que $\beta_q(Q)\leq t$, et dont les arêtes sont les swaps satisfaisant $w_q(Q,R)\leq t$.

> **Proposition A.1 — suffisance des swaps.** Pour tout $t$, les composantes connexes de $G^{\mathrm{swap}}_{k,q}(t)$ sont en bijection avec celles de $L(k,q,t)$.

**Preuve.** Considérons d'abord le graphe complet d'intersection des atomes non vides au niveau $t$. Comme chaque atome est connexe et que la famille est finie, ses composantes sont exactement celles de leur union.

Il suffit donc de remplacer toute arête d'intersection par un chemin de swaps. Soient $Q$ et $R$ deux atomes contenant un même point $y$. Posons $z_i=\left\Vert y-x_i\right\Vert^{2q}$ pour $q<\infty$, et $z_i=\mathbf{1}_{\left\Vert y-x_i\right\Vert^2>t}$ pour $q=\infty$. Dans le cas fini,

$$\sum_{i\in Q}z_i\leq kt^q,\qquad\sum_{i\in R}z_i\leq kt^q.$$

Apparions arbitrairement $Q\setminus R$ et $R\setminus Q$. Chaque remplacement produit un incrément $\delta=z_{\mathrm{entrant}}-z_{\mathrm{sortant}}$. Effectuons d'abord tous les swaps de $\delta\leq0$, puis ceux de $\delta>0$. Les sommes intermédiaires ne dépassent jamais le maximum des deux sommes terminales. Tous les atomes intermédiaires contiennent donc $y$ au niveau $t$.

Pour $q=\infty$, $Q$ et $R$ ne contiennent que des indices dont la distance carrée à $y$ est au plus $t$ ; tout sous-ensemble intermédiaire possède la même propriété. Le chemin de swaps existe donc aussi à la limite dure. $\square$

> **Corollaire A.2 — compression par forêt couvrante.** Pondérons chaque arête du graphe de swaps complet par $w_q(Q,R)$. Une forêt couvrante minimale préserve ses composantes après seuillage à tout niveau $t$.

Ce corollaire est la propriété minimax usuelle des arbres couvrants : pour deux sommets, le poids maximal sur leur chemin dans la forêt est le plus petit goulot d'étranglement possible. Les poids de naissance doivent rester attachés aux sommets. On a toujours

$$w_q(Q,R)\geq\max\left\lbrace\beta_q(Q),\beta_q(R)\right\rbrace.$$

La forêt compresse les **rencontres** ; elle ne compresse pas par elle-même l'ensemble des $\binom{n}{k}$ facettes.

## 8. Le cas fondamental $k=1$

Pour une facette singleton, il n'y a aucun choix entropique :

$$\Phi_{\lbrace i\rbrace,q}(y)=\left\Vert y-x_i\right\Vert^2$$

pour tout $q$. Deux atomes se rencontrent au niveau

$$w_q(\lbrace i\rbrace,\lbrace j\rbrace)=\frac{1}{4}\left\Vert x_i-x_j\right\Vert^2.$$

La forêt de fusion est donc, pour tout $q$, l'arbre minimum couvrant euclidien avec une transformation monotone de ses poids. La régularisation est exactement neutre à l'ordre $1$ et devient non triviale seulement pour $k\geq2$.

## 9. Atlas incomplet : ce qui reste exact

Soit $\mathcal{A}_k\subseteq\mathcal{Q}_k$ un atlas déclaré, et posons

$$F^{\mathcal{A}}_{k,q}(y)=\min_{Q\in\mathcal{A}_k}\Phi_{Q,q}(y).$$

Les résultats de convexité, d'union d'atomes, de monotonie en $q$ et de compression du **graphe complet des rencontres dans $\mathcal{A}_k$** restent exacts. En revanche :

- le chemin de swaps entre deux facettes de $\mathcal{A}_k$ peut sortir de $\mathcal{A}_k$ ;
- la fermeture par sous-facettes ne répare pas cette obstruction à ordre fixé ;
- une forêt de swaps restreinte n'est exacte que si elle possède un certificat minimax pour toutes les rencontres omises ;
- enrichir l'atlas change le champ $F^{\mathcal{A}}_{k,q}$ et impose de mettre à jour sa hiérarchie.

L'inclusion

$$\left\lbrace F^{\mathcal{A}}_{k,q}\leq t\right\rbrace\subseteq\left\lbrace F_{k,q}\leq t\right\rbrace$$

montre qu'un atlas incomplet ne crée pas de chemin absent de l'union complète. Il peut toutefois manquer des naissances, retarder des fusions et créer des branches artificiellement persistantes.

## 10. Théorèmes acquis et programme restant

| Énoncé | Statut |
|---|---|
| Formule KL logarithmique et homogénéité | théorème exact |
| Convexité, compacité et centre unique de chaque atome | théorème exact |
| Union d'atomes et hiérarchie ordre–échelle–$q$ | théorème exact |
| Limites DTM/HGP et encadrements multiplicatifs | théorème exact |
| Suffisance des swaps sur $\mathcal{Q}_k$ | théorème exact |
| Préservation des composantes par une forêt couvrante minimale | théorème exact |
| Exactitude d'un atlas arbitraire par ses seuls swaps internes | faux en général |
| Existence d'un atlas universellement exact et de taille quasi linéaire en grande dimension | impossible à attendre sans hypothèses supplémentaires |
| Récupération des seules branches de persistance supérieure à un seuil par enrichissement adaptatif | programme conditionnel à des marges de complétude et de coupe |
| Continuité combinatoire des événements le long de $q$ | faux sans marges ; programme de continuation avec détection des bifurcations |

Le résultat A justifie donc `HomogeneousLensTower` comme **objet mathématique** : il conserve la hiérarchie, les interactions d'ordre supérieur et le point d'ancrage $k=1$, tout en remplaçant le maximum local par une réduction régulière de taille $k$. Il ne justifie pas à lui seul une prétention de sparsité globale.

## Références

- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6 et 8 : régions témoins, graphe élémentaire des facettes et réduction par arbre couvrant.
- F. Chazal, D. Cohen-Steiner et Q. Mérigot, [*Geometric Inference for Probability Measures*](https://doi.org/10.1007/s10208-011-9098-0), *Foundations of Computational Mathematics*, 2011 : distance à une mesure.
- L. J. Guibas, Q. Mérigot et D. Morozov, [*Witnessed k-Distance*](https://arxiv.org/abs/1102.4972), 2011 : distance-$k$, sites de puissance et approximation witnessed.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024 : structure critique du champ K-NN dur.
