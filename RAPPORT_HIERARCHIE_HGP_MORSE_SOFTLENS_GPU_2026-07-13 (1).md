# Repenser HGP à partir de la hiérarchie : complexe de Morse en 3D et lentilles entropiques en grande dimension

> Rapport mathématique — 13 juillet 2026  
> Dépôt examiné : [`Ludwig-H/E-HGP`](https://github.com/Ludwig-H/E-HGP), branche `main`, révision `f4c61bde13fb2a0c847f31e46e235eaf2dae7c3a`  
> Manuscrit relu : `Manuscrit_de_thèse(7).pdf`, en particulier les parties I et II (chapitres 2 à 9)

> [!IMPORTANT]
> **Verdict.** L'objet à régulariser n'est pas d'abord le champ K-NN, et encore moins une grille spatiale : c'est la représentation implicite de la filtration de ses composantes. La bonne voie est de conserver un **minimum externe dur**, qui porte l'union des régions témoins et donc la hiérarchie, puis d'utiliser l'entropie dans le **maximum interne** qui définit chaque région témoin. Cette construction fournit une famille continue allant exactement de HGP à la DTM. En 3D, le point d'arrivée recommandé reste toutefois la hiérarchie K-NN dure : la théorie de Morse montre que son $H_0$ ne dépend que de minima et de selles supportés par au plus quatre points, même pour $K=10$. En grande dimension, une exactitude sparse universelle est impossible ; il faut un atlas adaptatif de facettes, une continuation entropique et une notion explicite de complétude des événements critiques.

## 1. Ce que les deux premières parties de la thèse imposent

Soit $X=\{x_1,\ldots,x_n\}\subset\mathbb{R}^{p}$, et soit $d_{(K)}(y)$ la distance de $y$ à son $K$-ième plus proche voisin. En variable rayon, l'ensemble de forte densité K-NN est

$$
L_K(r)=\left\{y\in\mathbb{R}^{p}:\left|B(y,r)\cap X\right|\ge K\right\}=\left\{y:d_{(K)}(y)\le r\right\}.
$$

La hiérarchie de Hartigan estimée est la filtration de composantes

$$
\mathcal{H}_K=\left\{\pi_0\bigl(L_K(r)\bigr)\right\}_{r\ge0}.
$$

Le manuscrit établit trois faits qui doivent rester des invariants de toute nouvelle proposition.

### 1.1 La sortie fondamentale est une hiérarchie de composantes

Le rayon $r$ augmente, les ensembles $L_K(r)$ croissent, et leurs composantes naissent puis fusionnent. Une bonne structure de données doit donc restituer **toutes les coupes** de cette filtration, pas seulement une partition choisie après coup.

À une composante continue $C\subset L_K(r)$, la thèse associe le cluster discret

$$
C^{\mathrm{disc}}=\left\{x\in X:\mathrm{dist}(x,C)\le r\right\}.
$$

Pour $K\ge2$, les $C^{\mathrm{disc}}$ peuvent se recouvrir. Une sortie fidèle est donc un arbre de composantes continues enrichi d'une relation d'incidence avec les observations, ou, de manière équivalente, une **hiérarchie de recouvrements**. Imposer trop tôt une partition efface précisément l'information d'ordre supérieur que HGP introduit (définition 7 et discussion, PDF p. 45–47 / manuscrit p. 23–25).

### 1.2 Les régions témoins sont l'objet géométrique élémentaire

Pour chaque $K$-ensemble $Q\subset X$, la région témoin dure est la lentille

$$
T_Q(r)=\bigcap_{x_i\in Q}B(x_i,r).
$$

Elle est convexe, donc connexe, et

$$
L_K(r)=\bigcup_{\substack{Q\subset X\\|Q|=K}}T_Q(r).
$$

En prenant pour sommets les seules facettes actives $\{Q:|Q|=K,\ T_Q(r)\neq\varnothing\}$, leur graphe d'intersection est exactement le graphe $\Gamma_K(X,r)$ des $(K-1)$-simplexes de Čech du chapitre 6. Ses composantes sont en bijection avec celles de $L_K(r)$, et l'union des identifiants présents dans une composante donne exactement l'amas discret de la thèse (définitions 20–21 et théorème 2, PDF p. 81–85 / manuscrit p. 59–63).

### 1.3 Les fusions tiennent dans une forêt ; les naissances restent des poids de sommet

Le chapitre 8 réduit les adjacences à des swaps élémentaires : deux $K$-facettes $Q$ et $R$ sont reliées par une coface de cardinal $K+1$ lorsqu'elles diffèrent d'un seul point. Le poids de cette coface est le rayon de sa plus petite boule englobante,

$$
\rho(Q\cup R)=\min_{y\in\mathbb{R}^{p}}\max_{x_i\in Q\cup R}\|y-x_i\|.
$$

Sous l'hypothèse de position générale de la définition 26, après restriction aux cofaces de Gabriel, une forêt minimum couvrante restitue, à chaque seuil, les $K$-polyèdres non triviaux du graphe complet. Les facettes isolées ne sont pas toutes garanties par le graphe de Gabriel ; la hiérarchie complète exige donc de conserver séparément leur poids de naissance $\rho(Q)$, ou, de façon minimale, les minima critiques qui engendrent effectivement une composante. Avec cette convention, le $K$-MST de la thèse n'est pas seulement une analogie : il compresse exactement toutes les fusions utiles (définitions 26–30 et théorèmes 4–5, PDF p. 108–115 / manuscrit p. 86–93).

La conclusion à tirer est plus précise que « calculer une mosaïque de Delaunay d'ordre $K$ » :

> **Il faut trouver les naissances et les fusions qui entrent dans la forêt couvrante sans matérialiser le graphe de facettes, ni a fortiori la mosaïque complète.**

## 2. Où placer l'entropie sans perdre la hiérarchie

L'analogie avec Sinkhorn doit être maniée avec précaution. En transport optimal, la régularisation entropique agit dans un problème convexe dont la représentation matricielle reste compacte. Ici, un soft-min global sur toutes les facettes donnerait un mélange dense : toute facette recevrait un poids positif, et la notion de support ne porterait plus la connectivité. La régularisation doit laisser intacte l'opération extérieure qui construit une union.

### 2.1 Écriture min–max du K-NN

Le carré de la distance K-NN s'écrit

$$
d_{(K)}(y)^2=\min_{\substack{Q\subset X\\|Q|=K}}\max_{x_i\in Q}\|y-x_i\|^2.
$$

Le minimum choisit les $K$ plus proches voisins ; le maximum désigne, dans cette facette, le voisin qui contraint la lentille. C'est ce maximum interne qu'il est naturel de régulariser par KL.

### 2.2 Définition proposée : les lentilles entropiques

Pour $\tau>0$, de dimension physique « distance au carré », et pour $|Q|=K$, posons

$$
\phi_{Q,\tau}(y)=\tau\log\left(\frac{1}{K}\sum_{x_i\in Q}\exp\left(\frac{\|y-x_i\|^2}{\tau}\right)\right).
$$

Cette fonction possède la représentation variationnelle

$$
\phi_{Q,\tau}(y)=\max_{\alpha\in\Delta_Q}\left\{\sum_{i\in Q}\alpha_i\|y-x_i\|^2-\tau\,\mathrm{KL}(\alpha\|u_Q)\right\},
$$

où $u_Q$ est la loi uniforme sur $Q$. À basse température, l'adversaire $\alpha$ se concentre sur le point le plus éloigné ; à haute température, il devient uniforme.

Définissons ensuite le champ global en gardant le minimum extérieur **dur** :

$$
F_{K,\tau}(y)=\min_{\substack{Q\subset X\\|Q|=K}}\phi_{Q,\tau}(y).
$$

Comme l'exponentielle est croissante, le minimiseur est toujours un ensemble de $K$ plus proches voisins. Ainsi,

$$
F_{K,\tau}(y)=\tau\log\left(\frac{1}{K}\sum_{j=1}^{K}\exp\left(\frac{d_{(j)}(y)^2}{\tau}\right)\right).
$$

Cette famille a deux limites exactes :

$$
F_{K,0}(y)=d_{(K)}(y)^2,
$$

$$
F_{K,\infty}(y)=\frac{1}{K}\sum_{j=1}^{K}d_{(j)}(y)^2.
$$

La première est le champ K-NN de HGP. La seconde est le carré de la DTM empirique de masse $K/n$. La DTM n'est donc pas une idée étrangère à HGP : elle est la limite à haute température de la régularisation entropique naturelle du pire voisin.

De plus, pour $0<\tau_1<\tau_2$,

$$
F_{K,\infty}\le F_{K,\tau_2}\le F_{K,\tau_1}\le F_{K,0}.
$$

L'augmentation de la température dilate les sous-niveaux et peut avancer les fusions ; le retour vers $\tau=0$ resserre progressivement la filtration vers HGP.

### 2.3 Théorème de représentation hiérarchique

Pour un seuil carré $t$, posons

$$
A_{Q,\tau}(t)=\left\{y\in\mathbb{R}^{p}:\phi_{Q,\tau}(y)\le t\right\}.
$$

Chaque $\phi_{Q,\tau}$ est $2$-fortement convexe ; chaque atome $A_{Q,\tau}(t)$ est donc convexe et

$$
\left\{F_{K,\tau}\le t\right\}=\bigcup_{|Q|=K}A_{Q,\tau}(t).
$$

Définissons la naissance d'une facette et la rencontre de deux facettes par

$$
\beta_\tau(Q)=\min_y\phi_{Q,\tau}(y),
$$

$$
w_\tau(Q,R)=\min_y\max\left\{\phi_{Q,\tau}(y),\phi_{R,\tau}(y)\right\}.
$$

Considérons le graphe filtré dont les sommets sont les $K$-ensembles, présents à partir de $\beta_\tau(Q)$, et dont les arêtes relient deux ensembles qui diffèrent d'un seul point, avec poids $w_\tau(Q,R)$.

**Proposition.** À tout seuil $t$, les composantes de ce graphe sont exactement les composantes de $\{F_{K,\tau}\le t\}$. Par conséquent, une forêt minimum couvrante pondérée par $w_\tau$, accompagnée des poids de naissance $\beta_\tau$, représente toute la hiérarchie.

**Esquisse de preuve.** Le graphe d'intersection complet des convexes $A_{Q,\tau}(t)$ a les mêmes composantes que leur union. Il reste à montrer que les swaps élémentaires suffisent. Soit $y$ commun à deux atomes actifs $Q$ et $R$, et posons $z_i=\exp(\|y-x_i\|^2/\tau)$. L'activité de $Q$ équivaut à une borne sur $\sum_{i\in Q}z_i$. Pour transformer $Q$ en $R$, on apparie les retraits et les ajouts, puis on effectue d'abord les swaps dont l'incrément de somme est négatif et ensuite ceux dont l'incrément est positif. Les sommes intermédiaires ne dépassent jamais le maximum des deux sommes terminales. Chaque facette intermédiaire est donc active au même point $y$. C'est l'analogue entropique de la proposition 6 de la thèse.

Aux deux extrémités,

$$
w_0(Q,R)=\rho(Q\cup R)^2
$$

pour un swap élémentaire, tandis qu'à haute température

$$
\phi_{Q,\infty}(y)=\|y-b_Q\|^2+v_Q,
$$

avec $b_Q=K^{-1}\sum_{i\in Q}x_i$ et $v_Q=K^{-1}\sum_{i\in Q}\|x_i-b_Q\|^2$. On retrouve donc respectivement le graphe de facettes de Čech et une union de boules de puissance.

### 2.4 Le cas $K=1$ : le MST est un point fixe

Pour une facette réduite à $Q=\{x_i\}$, le simplexe des poids $\Delta_Q$ est lui-même réduit à un point. Pour toute température,

$$
\phi_{\{x_i\},\tau}(y)=\|y-x_i\|^2.
$$

Pour deux points,

$$
w_\tau(\{x_i\},\{x_j\})=\frac{1}{4}\|x_i-x_j\|^2.
$$

La forêt couvrante est donc exactement l'EMST, indépendamment de $\tau$. La réponse mathématique à la question « quelle est la structure du 1-NN régularisé ? » est ainsi très nette :

> **Dans cette régularisation entropique, choisie pour conserver la représentation par régions témoins et par MST, le 1-NN ne se régularise pas : son arbre reste l'EMST.**

Dans ce cadre min–max, une modification non triviale à $K=1$ devrait agir sur le minimum externe sur les points, ou changer la métrique. Un soft-min externe perdrait l'union exacte de boules et la compression par MST. Ce serait un autre objet, non une continuation fidèle du Single-Linkage de la thèse.

### 2.5 Un petit problème dual, adapté au GPU

La naissance possède déjà un dual de taille $K$ :

$$
\beta_\tau(Q)=\max_{\alpha\in\Delta_Q}\left\{\sum_{i\in Q}\alpha_i\|x_i\|^2-\left\|\sum_{i\in Q}\alpha_i x_i\right\|^2-\tau\,\mathrm{KL}(\alpha\|u_Q)\right\}.
$$

Son centre est $y_Q^\star=\sum_i\alpha_i^\star x_i$. À $\tau=0$, on retrouve le dual de la miniball ; à $\tau=\infty$, $\alpha^\star=u_Q$ et $\beta_\infty(Q)=v_Q$. Le poids de rencontre $w_\tau(Q,R)$ se calcule de même sans optimiser directement dans $\mathbb{R}^{p}$. Introduisons $u_i\ge0$ pour $i\in Q$, $v_j\ge0$ pour $j\in R$, avec $\sum_i u_i+\sum_jv_j=1$, $\gamma=\sum_i u_i$, et agrégeons les doublons dans $\eta_s=u_s+v_s$. Alors

$$
w_\tau(Q,R)=\max_{\substack{u,v\ge0\\\sum_i u_i+\sum_jv_j=1}}\left\{\sum_s\eta_s\|x_s\|^2-\left\|\sum_s\eta_sx_s\right\|^2-\tau\left[\sum_{i\in Q}u_i\log\left(\frac{Ku_i}{\gamma}\right)+\sum_{j\in R}v_j\log\left(\frac{Kv_j}{1-\gamma}\right)\right]\right\}.
$$

Les coordonnées de $u$ ou $v$ absentes d'un des deux ensembles sont prises nulles dans $\eta_s=u_s+v_s$. La dérivation introduit un poids $\gamma\in[0,1]$ pour écrire le maximum de deux fonctions comme un maximum de combinaisons convexes, applique deux fois la représentation variationnelle de $\phi$, puis élimine $y$ analytiquement ; le théorème minimax s'applique par coercivité en $y$ et compacité des simplexes. Les termes entropiques aux faces $\gamma=0$ et $\gamma=1$ sont compris par prolongement continu. Il s'agit d'une maximisation concave sur un simplexe de dimension au plus $2K-1$. Le centre de rencontre est

$$
y^\star=\sum_s\eta_sx_s.
$$

Pour un swap élémentaire, l'union ne contient que $K+1$ points ; pour $K=10$, le noyau dur $\tau=0$ est donc un problème de miniball sur onze points, et le noyau général reste un petit problème à Gram. La dimension ambiante intervient dans le calcul initial des produits scalaires, mais pas dans la taille du solveur. Un active-set, Newton projeté ou mirror-ascent batché est naturellement compatible avec le GPU. Un nombre fixe d'itérations ne fournit toutefois qu'un poids approché : le statut `atlas_exact` exige des bornes primale–duale certifiées, une tolérance inférieure à l'écart entre événements concurrents et un chemin spécifique pour les quasi-égalités.

À haute température, le poids de rencontre de deux sites de puissance possède même une forme fermée. Pour $d=\|b_Q-b_R\|>0$,

$$
s=\mathrm{clip}\left(\frac{d^2+v_R-v_Q}{2d},0,d\right),
$$

$$
w_\infty(Q,R)=\max\left\{v_Q+s^2,\ v_R+(d-s)^2\right\}.
$$

Si $d=0$, $w_\infty(Q,R)=\max\{v_Q,v_R\}$. Enfin,

$$
w_\infty(Q,R)\le w_\tau(Q,R)\le w_0(Q,R).
$$

Le poids power est donc un minorant très bon marché pour un Borůvka branch-and-bound. Celui-ci n'est exact que si chaque paire ou bloc non exploré possède un minorant certifié ; une shortlist ANN suivie du petit dual reste une heuristique. Sous cette condition, seuls les candidats encore susceptibles d'améliorer l'arête sortante courante passent dans le solveur certifié.

## 3. Le bon objet en 3D pour $K=10$ : le complexe de Morse de $d_{(K)}$

La mosaïque d'ordre $K$ contient toute l'information, mais elle en contient beaucoup trop pour calculer seulement $H_0$. La théorie de Morse récente de Reani et Bobrowski fournit la réduction qui manquait au manuscrit.

Pour $r=d_{(K)}(c)$, posons

$$
I(c)=X\cap B^\circ(c,r),\qquad U(c)=X\cap\partial B(c,r).
$$

Sous une hypothèse de position générale, $c$ est critique si et seulement si

$$
c\in\mathrm{relint}\,\mathrm{conv}\,U(c),
$$

et son indice vaut

$$
\mu(c)=|I(c)|+|U(c)|-K.
$$

De plus, $|U(c)|\le p+1$. Ces résultats sont établis dans [Reani–Bobrowski, *Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792).

### 3.1 Pour $H_0$, seuls les indices 0 et 1 comptent

- $\mu=0$ : naissance d'une composante sous les hypothèses génériques ;
- $\mu=1$ : fusion possible de composantes ;
- $\mu\ge2$ : aucun changement de $H_0$.

En dimension 3, $|U|\le4$. Ainsi, l'ordre $K=10$ n'impose jamais un **support géométrique frontière** à dix ou onze points : le centre et le rayon sont déterminés par deux, trois ou quatre points frontière. L'événement complet transporte néanmoins 10 ou 11 identifiants, les autres étant les points strictement intérieurs.

| événement pour $K=10$ | $\vert U\vert =2$ | $\vert U\vert =3$ | $\vert U\vert =4$ |
|---|---:|---:|---:|
| naissance $\mu=0$ | $\vert I\vert =8$ | $\vert I\vert =7$ | $\vert I\vert =6$ |
| selle $\mu=1$ | $\vert I\vert =9$ | $\vert I\vert =8$ | $\vert I\vert =7$ |

Géométriquement, les supports sont respectivement une paire dont le centre est le milieu, un triangle bien centré dans son cercle, ou un tétraèdre bien centré dans sa sphère.

### 3.2 Les selles d'indice 1 sont exactement les cofaces de Gabriel

Soit $\sigma$ une coface de cardinal $K+1$, de miniball $B_\sigma=B(c,\rho)$, et supposons $\sigma$ de Gabriel au sens du chapitre 8. On utilise ici la position générale de la définition 26 : aucun point extérieur à $\sigma$ sur $\partial B_\sigma$, indépendance affine du support frontière et unicité des niveaux critiques distincts. Écrivons $I=\sigma\cap B^\circ_\sigma$ et $U=\sigma\cap\partial B_\sigma$. La vacuité de Gabriel donne

$$
|I|+|U|=K+1,
$$

Comme $|U|\ge2$, on a $|I|\le K-1$ ; par conséquent $d_{(K)}(c)=\rho$. La propriété caractéristique d'une miniball donne $c\in\mathrm{relint}\,\mathrm{conv}\,U$. Le centre est donc un point critique d'indice 1. Réciproquement, un point critique d'indice 1 définit $\sigma=I\cup U$, de cardinal $K+1$ ; la condition $c\in\mathrm{relint}\,\mathrm{conv}\,U$ montre que $B(c,r)$ est la miniball de $\sigma$, et les définitions de $I,U$ avec la position générale excluent tout point extérieur dans cette boule. C'est une coface de Gabriel.

On obtient donc, sous les mêmes hypothèses génériques, la correspondance

$$
\left\{\text{cofaces de Gabriel de cardinal }K+1\right\}\longleftrightarrow\left\{\text{points critiques de }d_{(K)}\text{ d'indice }1\right\}.
$$

Le théorème 4 de la thèse affirme ensuite que seules certaines de ces cofaces sont $K$-séparantes. Dans le langage de Morse, une selle d'indice 1 peut simultanément fusionner des composantes de $H_0$ et créer des classes de $H_1$. Une union–find ordonnée par rayon extrait exactement sa partie $H_0$.

De même, les configurations critiques d'indice 0 sont les facettes de cardinal $K$ qui donnent naissance aux composantes.

### 3.3 Le « K-MST » minimal devient un hyperarbre critique

Pour une selle $c$ d'indice 1, les bras locaux juste sous le niveau critique sont portés par les $K$-ensembles

$$
Q_u=I\cup\bigl(U\setminus\{u\}\bigr),\qquad u\in U.
$$

Notons $j=|U|\le4$. Lorsque le rayon atteint $r(c)$, les $j$ bras sont attachés simultanément. S'ils appartiennent juste avant $r(c)$ à $m$ composantes globales distinctes, l'événement tue $m-1$ classes de $H_0$ et peut créer simultanément $j-m$ classes de $H_1$ ; l'hyper-Kruskal ne retient que le premier nombre.

La structure minimale à produire est donc :

- des feuilles, portées par les minima d'indice 0 ;
- des hyperarêtes d'arité 2, 3 ou 4, portées par les selles d'indice 1 ;
- un rayon critique par événement ;
- une union–find qui ne conserve que les attaches réunissant des composantes distinctes ;
- les facettes locales $Q_u$ et les attaches de branche nécessaires à la merge forest.

Cette structure suffit à la **merge forest continue**. Elle ne suffit pas, à elle seule, à énumérer toutes les facettes de Čech ni toutes les incidences observation–cluster. Le recouvrement discret HGP doit être reconstruit à la demande sur une coupe, à partir des facettes actives ou d'un index géométrique certifié. De plus, l'affectation d'un bras à sa composante globale par une descente de $d_{(K)}$ doit encore faire l'objet d'un théorème de terminaison et de maintien dans le sous-niveau strict ; tant que ce résultat manque, c'est une conjecture algorithmique bien motivée.

Pour $K=1$, les minima sont les observations à rayon nul. Une selle d'indice 1 a $I=\varnothing$ et $|U|=2$ ; sa boule diamétrale est vide, donc elle définit une arête de Gabriel. Kruskal ne conserve que les selles qui fusionnent deux composantes : sous unicité des longueurs, on retrouve l'EMST unique ; avec des égalités, un EMST portant la même hiérarchie. Le complexe de Morse généralise ainsi le rôle hiérarchique du MST.

L'hypothèse « un seul événement par niveau » est commode pour les preuves, mais ne doit pas devenir une hypothèse cachée du code. Des événements de même rayon doivent être traités par lot : on fige les composantes strictement sous le niveau, on construit le graphe ou l'hypergraphe des attaches du lot, puis on crée un nœud de fusion multifurqué par composante de ce graphe. Un ordre arbitraire entre threads produirait un dendrogramme binaire artificiel et non la hiérarchie mathématique.

### 3.4 Ce qu'il faut calculer — et ce qu'il ne faut plus calculer

L'objet de sortie n'est ni une grille, ni le complexe alpha complet, ni la mosaïque complète. Le pipeline mathématique cible est

$$
\boxed{\text{minima de rang }K\ +\ \text{selles de rang }K+1\ +\ \text{attaches descendantes}\ +\ \text{hyper-Kruskal}.}
$$

La mosaïque de Delaunay d'ordre supérieur et la rhomboid tiling restent utiles comme **oracles de référence ou de complétude**, pas comme format obligatoire de calcul ni de sortie. Les constructions exactes du multicover sont décrites par [Corbet–Kerber–Lesnick–Osang](https://arxiv.org/abs/2103.07823) et l'algorithme incrémental des mosaïques par [Edelsbrunner–Osang](https://arxiv.org/abs/2011.03617).

### 3.5 Génération GPU des événements

Une architecture 3D crédible comporte les étages suivants.

1. **Index spatial.** Tri Morton, LBVH ou grille de hachage uniquement pour les requêtes de voisinage et les certificats de comptage ; aucun voxel n'est un sommet de la hiérarchie.
2. **Proposition de supports.** Générer des paires, triangles et tétraèdres locaux susceptibles de porter une boule de rang $K$ ou $K+1$. La marche dans les cellules d'ordre $k$, l'algorithme incrémental $k=1,\ldots,K$, ou la continuation SoftLens peuvent alimenter cette étape.
3. **Prédicats géométriques batchés.** Calculer centre, rayon et coordonnées barycentriques des supports de taille au plus quatre ; rejeter les centres hors de $\mathrm{relint}\,\mathrm{conv}(U)$.
4. **Certification de rang d'un candidat.** Compter par BVH les points strictement dans la boule et ceux sur la frontière. Pour $K=10$, seuls les totaux 10 et 11 sont utiles à $H_0$. Ce comptage valide une boule proposée ; il ne prouve pas que la liste des supports est complète.
5. **Attaches descendantes.** Pour chaque bras $Q_u$, tenter une descente de $d_{(K)}$ vers une composante plus ancienne, par swaps de voisins et miniballs locaux. Les bras et les selles sont indépendants et se traitent en parallèle, mais l'exactitude requiert encore le certificat de descente mentionné en section 3.3.
6. **Radix sort et hyper-union–find.** Trier les événements par rayon, traiter atomiquement les lots de même niveau, appliquer les naissances puis les selles, et stocker uniquement les fusions effectives.

L'arithmétique rapide peut proposer les événements. Seules les configurations proches d'une égalité de rang, d'une coordonnée barycentrique nulle ou d'un prédicat in-sphere nul doivent passer dans un chemin adaptatif plus précis.

Le véritable verrou n'est ni le calcul de miniball ni l'union–find : c'est la **complétude de la génération des supports critiques et de leurs attaches**. Une BVH certifie le rang d'un candidat, pas l'absence d'un support jamais proposé. Un résultat déclaré exact doit donc reposer sur un énumérateur exhaustif de cellules ou de rhomboïdes, un branch-and-bound couvrant muni de bornes géométriques, ou un certificat explicite de fermeture de l'atlas ; sinon le statut reste conditionnel.

### 3.6 Complexité : pourquoi le sous-seconde est plausible mais non universel

Construire explicitement les $K$ premières mosaïques possède, en dimension 3, une borne de pire cas quadratique en $n$ pour $K$ fixé. Même une Delaunay 3D ordinaire peut être quasi quadratique sur un bon échantillonnage d'une surface lisse ; voir [Erickson, *Nice Point Sets Can Have Nasty Delaunay Triangulations*](https://arxiv.org/abs/cs/0103017). Il est donc impossible de garantir « exact, 50 000 points, moins d'une seconde » pour toute entrée en construisant la mosaïque.

Pour un processus de Poisson stationnaire et $K,p$ fixés, le nombre attendu de cellules par unité de volume est en revanche linéaire dans l'intensité ; [Edelsbrunner–Nikitenko](https://arxiv.org/abs/1709.09380) donnent des formules explicites. Les événements d'indice 0 et 1 n'en sont qu'un sous-ensemble. Un régime quasi linéaire est donc mathématiquement crédible pour des nuages **volumétriques** non pathologiques. Ce modèle ne prédit pas le comportement d'un LiDAR essentiellement surfacique, et aucune borne quasi linéaire de pire cas n'est ici obtenue pour le nombre de candidats, le nombre d'événements $H_0$ ni la longueur totale des descentes.

Le squelette algorithmique est compatible avec des débits GPU très élevés : un EMST Borůvka a été calculé sur 37 millions de points 3D en moins de 0,5 s sur A100 dans [Prokopenko–Sao–Lebrun-Grandié](https://arxiv.org/abs/2207.00514). Ce résultat ne prédit pas le coût de $K=10$, mais montre que Borůvka, la BVH et l'union–find ne sont pas en eux-mêmes incompatibles avec la cible.

Une voie exacte de référence itérative $k=1,\ldots,10$ devient également plus crédible depuis l'apparition d'algorithmes GPU généraux pour les diagrammes de Voronoï et de puissance 3D ; voir [Taveira et al., 2026](https://arxiv.org/abs/2605.06408). Il reste toutefois à mesurer les constantes de la construction d'ordre supérieur. Pour tenir la latence, le backend final doit aussi pouvoir viser directement les événements de rang 10 et 11 ; les dix passes ne sont pas une nécessité mathématique.

### 3.7 Mode à latence bornée : multicover sparse plutôt que grille uniforme

Si un nuage surfacique ou dégénéré produit trop de candidats, la meilleure approximation hiérarchique connue n'est pas une grille : c'est une sparsification directe de la multicouverture.

[Alonso, *A Sparse Multicover Bifiltration of Linear Size*](https://arxiv.org/abs/2411.06986) construit, pour dimension et précision fixées, une approximation de taille $O(n)$, indépendante de $K$. L'idée utilise des filets persistants, des boules dont la croissance ralentit puis elles disparaissent, une application de couverture et des multiplicités entières. Elle vise directement la filtration $L_K(r)$, donc la hiérarchie, et non une norme fonctionnelle intermédiaire.

Pour un backend GPU 3D à budget strict, la hiérarchie de modes recommandée est donc :

1. **Morse exact certifié** lorsque la génération des événements se ferme ;
2. **Morse conditionnel** avec diagnostics de frontière d'atlas ;
3. **multicover sparse certifié à précision choisie** lorsque le budget d'événements est dépassé.

Les filets persistants peuvent être approchés par des MIS parallèles sur des graphes de voisinage dyadiques, ou par une hiérarchie Morton avec résolution des conflits entre cellules voisines. Cette traduction GPU reste à démontrer soigneusement, mais elle est structurellement plus adaptée que l'augmentation d'une grille uniforme.

## 4. Grande dimension : pourquoi l'exact sparse est impossible sans hypothèse

La faible taille des miniballs ne suffit pas à rendre l'objet global sparse. Considérons

$$
X=\{e_1,\ldots,e_n\}\subset\mathbb{R}^{n},
$$

où les $e_i$ sont les vecteurs de la base canonique. Pour chaque $K$-ensemble $Q$, son barycentre est

$$
b_Q=\frac{1}{K}\sum_{i\in Q}e_i.
$$

On a

$$
\|b_Q-e_i\|^2=1-\frac{1}{K}\quad\text{si }i\in Q,
$$

$$
\|b_Q-e_i\|^2=1+\frac{1}{K}\quad\text{si }i\notin Q.
$$

Le $K$-ensemble $Q$ est donc l'unique ensemble de $K$ plus proches voisins dans un voisinage de $b_Q$, et $b_Q$ est un minimum local de $d_{(K)}$. La hiérarchie exacte possède ainsi au moins

$$
\binom{n}{K}
$$

points de naissance spatialement distincts, tous au même niveau $1-1/K$. Toute représentation explicite et étiquetée de la merge forest possède donc au moins ce nombre de feuilles. Une représentation symbolique peut naturellement exploiter la symétrie exceptionnelle de cet exemple, mais une perturbation générique élimine cette compression sans éliminer les minima. L'explosion combinatoire appartient ici au **merge tree lui-même**, pas seulement à une mosaïque inutilement riche. Elle subsiste à haute température : la moyenne quadratique associée à $Q$ est également minimale en $b_Q$.

Il n'existe donc aucun analogue universel de Sinkhorn qui donnerait une représentation exacte toujours sparse ou quasi linéaire en $n$. Lorsque $K$ fait partie de l'entrée, $\binom{n}{K}$ peut être exponentiel ; pour $K=10$ fixé, il est polynomial mais de degré 10, ce qui suffit à exclure l'algorithme pratique recherché. Il faut assumer au moins l'un des choix suivants :

- une faible dimension intrinsèque ou un nombre de doublement borné ;
- des marges critiques et un nombre d'événements pertinents bornés ;
- une condensation par persistance ou par masse ;
- une exactitude limitée à un atlas adaptatif explicitement déclaré.

## 5. SoftLens-HGP sur un atlas adaptatif

Soit

$$
\mathcal{A}\subseteq\binom{X}{K}
$$

un ensemble fini de facettes. On définit

$$
F_{\mathcal{A},\tau}(y)=\min_{Q\in\mathcal{A}}\phi_{Q,\tau}(y).
$$

La hiérarchie de $F_{\mathcal{A},\tau}$ peut être calculée exactement sans grille. Pour un atlas arbitraire, il faut initialement considérer le graphe complet des rencontres $w_\tau(Q,R)$, car le chemin de swaps reliant deux atomes peut sortir de $\mathcal{A}$. Une arête non élémentaire sélectionnée par la forêt peut, à son centre $y^\star$, proposer un chemin d'au plus $K$ swaps actifs. Cette factorisation n'est pas sémantiquement neutre : pour représenter exactement l'atlas courant, on conserve l'arête directe ; si les facettes intermédiaires sont ajoutées, elles deviennent de vraies nouvelles colonnes, susceptibles de créer ailleurs une naissance ou une fusion plus précoce, et toute la forêt du nouvel atlas doit être recalculée.

### 5.1 Initialisation witnessed

En adoptant explicitement la convention où le témoin appartient à son propre voisinage, posons

$$
Q_i=\{x_i\}\cup N_{K-1}\left(x_i;X\setminus\{x_i\}\right).
$$

L'initialisation naturelle est

$$
\mathcal{A}_0=\mathrm{unique}\left\{Q_i:i=1,\ldots,n\right\}.
$$

Elle contient au plus $n$ facettes. À $\tau=\infty$, cette construction est exactement le *witnessed k-distance* de [Guibas–Mérigot–Morozov](https://arxiv.org/abs/1102.4972), donc une union de $O(n)$ boules de puissance. À $\tau=0$, les mêmes identifiants portent de vraies lentilles de Čech : la filtration obtenue est une sous-filtration de HGP, exacte sur l'atlas.

La DTM witnessed est ainsi le bon **état initial** : elle donne des power-sites, des poids de rencontre fermés et d'excellents minorants. Elle ne doit pas être confondue avec la hiérarchie finale si l'objectif reste HGP.

### 5.2 Génération de colonnes aux endroits qui changent l'arbre

Le raffinement doit être piloté par les événements de la forêt, pas par une erreur uniforme du champ.

1. Calculer la forêt de l'atlas courant par Borůvka implicite.
2. Récupérer les centres des minima et, surtout, les centres $y^\star$ des arêtes de fusion retenues ou presque retenues.
3. À chaque centre, calculer le vrai $K$-NN dur

$$
Q^\star(y)=N_K(y).
$$

À $y$ fixé, $Q^\star(y)$ est une colonne minimisante exacte de $F_{K,\tau}(y)$, pour toute température.
4. Conserver les arêtes directes comme certificat de l'atlas courant, ajouter les facettes intermédiaires comme nouvelles colonnes, puis recalculer la forêt du nouvel atlas.
5. Continuer jusqu'à stabilisation de l'arbre, épuisement du budget, ou réussite d'un certificat de complétude.

L'oracle de proposition peut utiliser un soft-top-$K$ capé de type Fermi–Dirac sur le shell de rangs $K,\ldots,K+L$ :

$$
q_i(y;\varepsilon)=\frac{1}{1+\exp\left((\|y-x_i\|^2-\lambda)/\varepsilon\right)},\qquad\sum_iq_i=K.
$$

Il propose des swaps concurrents de forte probabilité, entièrement par primitives GPU. Mais la projection finale sur $K$ identifiants, le calcul de $w_\tau$ et les décisions d'union–find restent durs.

Il faut distinguer deux températures :

- $\tau$, qui définit éventuellement une autre hiérarchie mathématique ;
- $\varepsilon$, qui ne sert qu'à explorer des candidats et ne doit pas changer la hiérarchie déclarée.

### 5.3 Continuation en température et en ordre

La stratégie recommandée est une homotopie

$$
\tau=\infty\longrightarrow\tau_m\longrightarrow\cdots\longrightarrow\tau_1\longrightarrow0.
$$

La forêt, les selles et les colonnes trouvées à haute température initialisent la température suivante. La sortie fidèle à la thèse est $\tau=0$. Une sortie à $\tau>0$ reste une hiérarchie mathématiquement bien définie, mais elle doit être nommée hiérarchie SoftLens et non HGP exact.

L'itération $k=1,\ldots,K$ proposée dans la question est également naturelle :

1. $k=1$ fournit l'EMST et ses coupes ;
2. pour passer de $k-1$ à $k$, étendre les facettes témoins par les points du shell local aux minima et aux selles ;
3. dédupliquer par hachage des identifiants triés ;
4. reconstruire la forêt d'ordre $k$, puis utiliser ses événements pour enrichir l'ordre suivant.

En faible dimension, cette continuation reflète l'algorithme exact d'Edelsbrunner–Osang, qui sélectionne les sommets d'ordre $k$ depuis les mosaïques d'ordres inférieurs. En grande dimension, elle est une heuristique structurée, non un certificat universel.

### 5.4 Borůvka implicite plutôt que graphe explicite

Pour chaque composante courante $C$ de l'atlas, Borůvka demande seulement l'arête sortante de poids minimal :

$$
\mathrm{Sep}_{K,\tau}(C)=\mathop{\mathrm{argmin}}_{\substack{Q\in C\\R\in\mathcal{A}\setminus C}}w_\tau(Q,R).
$$

Un oracle exact de $\mathrm{Sep}_{K,\tau}$ donne une forêt exacte sur l'atlas en $O(\log|\mathcal{A}|)$ rondes. Le poids power $w_\infty$, les volumes englobants des barycentres et les bornes de variance permettent d'élaguer les paires. L'index doit toutefois couvrir tout $\mathcal{A}$ et fournir un minorant certifié pour chaque bloc élagué ; les candidats survivants passent alors dans le petit dual concave de la section 2.5.

Cette formulation est le véritable analogue de l'accélération de l'OT : l'entropie ne transforme pas magiquement toute la hiérarchie en un produit matrice–vecteur, mais elle fournit un problème local lisse et un excellent état chaud pour un **oracle de coupe parallèle**.

### 5.5 Exactitude et certificat hiérarchique

Trois statuts doivent être séparés.

- **Exact sur atlas.** La forêt restitue toutes les composantes de $F_{\mathcal{A},\tau}$ à tous les seuils.
- **HGP interne.** À $\tau=0$, un atlas incomplet donne une union de lentilles incluse dans $L_K(r)$. Il ne crée pas de chemin qui n'existerait pas dans HGP, mais il peut manquer des naissances ou retarder des fusions.
- **HGP global.** Il exige qu'à chaque seuil l'inclusion de l'union des atomes de l'atlas dans $L_K(r)$ induise une bijection sur les composantes :

$$
\pi_0\left(\bigcup_{Q\in\mathcal{A}}T_Q(r)\right)\xrightarrow{\ \sim\ }\pi_0\left(L_K(r)\right)\qquad\text{pour tout }r.
$$

Un certificat opérationnel doit couvrir les naissances et garantir, pour chaque coupe de Borůvka rencontrée, qu'une arête globale de poids minimal traversant la coupe est présente dans l'atlas ; une simple saturation sans nouvelle colonne ne suffit pas.

Le bon théorème de récupération ne doit pas être formulé en Wasserstein. Il doit porter sur les événements critiques. Pour un point critique $c$, de rayon $r$, avec intérieur $I$ et support frontière $U$, définissons la marge de shell

$$
\delta_{\mathrm{shell}}(c)=\min\left\{r^2-\max_{x_i\in I}\|c-x_i\|^2,\ \min_{x_j\notin I\cup U}\|c-x_j\|^2-r^2\right\},
$$

en omettant le premier terme si $I=\varnothing$. Sous position générale, cette marge est positive. Il faut lui adjoindre une marge barycentrique — distance de $c$ au bord relatif de $\mathrm{conv}(U)$ — et une marge entre niveaux de fusion pertinents.

Un objectif théorique réaliste est alors : si l'oracle localise chaque support d'indice 0 ou 1 dont la branche a une persistance supérieure à $\pi$, avec une erreur inférieure à une fraction de ces marges, la forêt condensée calculée est isomorphe à la forêt HGP condensée au-dessus de $\pi$. Cette garantie concerne directement les naissances, les fusions et l'ordre des branches.

> [!WARNING]
> La marge $d_{(K+1)}(c)^2-d_{(K)}(c)^2$ n'est **pas** le bon critère aux selles : elle est nulle, puisque plusieurs points frontière sont ex æquo. Il faut mesurer l'écart entre le shell critique $U$ et les points strictement intérieurs/extérieurs.

## 6. Où se placent la DTM, UMAP et les complexes sparse

### 6.1 DTM

La DTM est la limite $\tau=\infty$ de SoftLens-HGP. Elle possède trois rôles particulièrement utiles :

- initialisation witnessed de taille linéaire ;
- minorant power pour Borůvka et le branch-and-bound ;
- hiérarchie de secours très lisse lorsque l'utilisateur accepte explicitement de remplacer le pire voisin par une moyenne quadratique.

Elle ne doit pas être vendue comme HGP régularisé exact : elle avance certaines naissances et certaines fusions. L'intérêt du chemin SoftLens est précisément de pouvoir revenir vers $\tau=0$ sans changer de langage mathématique ni de noyau d'événements.

### 6.2 UMAP

UMAP est utile pour ses primitives d'ANN, son adaptation d'échelle locale et son optimisation GPU. Son graphe flou reste néanmoins gouverné par des relations binaires. Il ne restitue pas les composantes de l'union des lentilles $K$-aires et reproduirait, comme objet final, la faute conceptuelle reprochée dans la thèse à RSL/HDBSCAN : estimer à l'ordre $K$, connecter à l'ordre 1.

La partie réutilisable d'UMAP est donc l'**infrastructure de proposition** : index ANN, shells locaux et éventuellement apprentissage d'une métrique. L'objet hiérarchique doit rester le graphe de facettes ou le complexe critique. Une réduction de dimension ne peut préserver la hiérarchie qu'avec un contrat supplémentaire sur les événements critiques ; elle ne constitue pas en elle-même la solution.

### 6.3 Multicover sparse

Les travaux de Buchet–Dornelas–Kerber sur les filtrations de Čech d'ordre supérieur et, surtout, la construction linéaire d'Alonso sont les alternatives les plus proches de la thèse : elles sparsifient directement la multicouverture. Pour la 3D à latence bornée, elles sont mathématiquement préférables à une discrétisation uniforme. En grande dimension, leur taille dépend d'une hypothèse de dimension fixe ou de doublement borné ; elles ne contredisent donc pas l'exemple combinatoire de la section 4.

## 7. Comparaison avec l'implémentation courante de `perg_hgp`

La version GitHub examinée expose deux objets distincts dans [`perg_hgp/README.md`](https://github.com/Ludwig-H/E-HGP/blob/f4c61bde13fb2a0c847f31e46e235eaf2dae7c3a/perg_hgp/README.md).

### 7.1 `PowerCover3D`

Pour chaque observation $x_i$, l'implémentation construit une loi locale $q_i$, puis un site de puissance

$$
z_i=\sum_jq_{ij}x_j,\qquad a_i=\sum_jq_{ij}\|x_j-z_i\|^2.
$$

Elle définit ensuite

$$
g(y)=K\text{-ième plus petite valeur de }\left\{\sqrt{\|y-z_i\|^2+a_i}\right\}_{i=1}^{n},
$$

échantillonne $g$ aux centres d'une grille uniforme anisotrope, et calcule une forêt cubique 26-connexe. Le contrat est documenté dans [`POWER_COVER_3D.md`](https://github.com/Ludwig-H/E-HGP/blob/f4c61bde13fb2a0c847f31e46e235eaf2dae7c3a/perg_hgp/POWER_COVER_3D.md).

Cette architecture est techniquement sérieuse : noyau Gibbs fusionné, diagnostics de voisinage, enveloppes inner/outer, MSF exact du champ cubique stocké, sérialisation cohérente et nombreux garde-fous numériques. Elle a fourni une base GPU réelle et des mesures utiles.

Mais sa régularisation n'est pas SoftLens-HGP. Elle effectue

$$
\text{rang }K\text{ après lissage local de chaque site},
$$

alors que la construction proposée effectue

$$
\text{minimum dur sur les facettes après lissage entropique du maximum interne}.
$$

Ces deux opérations ne commutent pas. La première conserve une fonction de rang sur des sites déplacés ; la seconde conserve l'union de régions témoins et le graphe de facettes de la thèse.

La seconde différence est la grille. Même en mode dur $q_i=\delta_i$, `PowerCover3D` reconstruit le $H_0$ d'un champ cubique, non les événements critiques exacts de $d_{(K)}$. La grille garantit une enveloppe spatiale, mais son nombre de cellules est lié à l'étendue globale et à la plus petite échelle à résoudre.

### 7.2 Ce que disent les benchmarks les plus récents

La [campagne G4 du 12 juillet 2026](https://github.com/Ludwig-H/E-HGP/blob/f4c61bde13fb2a0c847f31e46e235eaf2dae7c3a/perg_hgp/benchmarks/results/G4_20260712_REPORT.md) rapporte notamment :

- après compilation, sur $N=100\,000$ et une grille $48^3$, environ 1,13 s en mode dur et 1,38 s en mode local ;
- 159,23 s pour 10 millions de points, grille $96^3$, en mode local ;
- 962,95 s pour 30 millions de points, grille $128^3$, en mode local ;
- 33,56 s pour 30 millions en mode dur $\kappa=1$ ;
- le MSF cubique lui-même ne prend que 0,31 à 0,56 s sur les grands runs ; le coût est dominé par le voisinage et la régularisation ;
- sur un test anisotrope $K=10$, passer de $24^3$ à $48^3$ améliore fortement la coupe mesurée, mais augmente le temps CPU de 1,238 s à 6,961 s.

La cible « 50 000 points 3D en moins d'une seconde » paraît donc compatible avec le budget matériel en régime chaud, mais elle n'est pas encore démontrée de bout en bout, et surtout la résolution nécessaire à une bonne hiérarchie dépend encore de la grille.

### 7.3 `PERGHGPClusterer` historique

Le prototype historique est plus proche de la bonne combinatoire : atlas de témoins, cofaces, filtres de Gabriel et MST dual. Mais [`estimator.py`](https://github.com/Ludwig-H/E-HGP/blob/f4c61bde13fb2a0c847f31e46e235eaf2dae7c3a/perg_hgp/perg_hgp/estimator.py) impose encore une entrée $(N,3)$, une grille spatiale pour les candidats, et des budgets fixes de témoins/cofaces/facettes. Son statut `atlas_exact` signifie exact pour l'atlas construit, non exact global pour HGP.

### 7.4 Évolution recommandée du code

Il ne faut pas jeter le travail actuel. Les briques réutilisables sont nombreuses :

- index de voisinage et audit des identifiants ;
- soft-top-$K$/Gibbs fusionné ;
- primitives de tri, hachage, déduplication et union–find ;
- diagnostics fail-closed ;
- sérialisation des forêts et rapports de précision ;
- harnais Blackwell et jeux de benchmarks.

Le changement d'architecture proposé est le suivant.

| étage actuel | remplacement hiérarchique proposé |
|---|---|
| sites locaux $q_i\mapsto(z_i,a_i)$ | facettes $Q$, atomes $\phi_{Q,\tau}$, minima et selles |
| K-ième rang de sites de puissance | minimum dur sur les facettes |
| centres d'une grille uniforme | centres critiques continus calculés par petit dual/miniball |
| MSF cubique | Borůvka sur atlas ou hyper-Kruskal Morse |
| erreur de cellule globale | complétude des événements, marge de shell, persistance de branche |
| sortie plate conservatrice | merge forest + relation de couverture, partition seulement en post-traitement |

## 8. Architecture recommandée par régime

### 8.1 Données massives 3D, $K=10$

**Objet mathématique final :** merge forest dure de $d_{(10)}$, donc hiérarchie continue HGP exacte lorsqu'elle est certifiée ; le recouvrement discret d'une coupe est une requête géométrique séparée.

**Backend principal :** `MorseHGP3D`.

1. Générer et certifier uniquement les configurations d'indice 0 et 1, dont le support géométrique frontière vérifie $|U|\le4$, tout en transportant les 10 ou 11 identifiants de l'événement.
2. Tracer leurs bras $Q_u$ par descentes locales certifiées ; conserver un statut conditionnel tant que le théorème d'attache manque.
3. Construire l'hyperforêt par radix sort et union–find.
4. Conserver les porteurs critiques et un index des facettes actives ; reconstruire le recouvrement observation–branche à la demande, et une partition seulement en post-traitement.
5. Utiliser $k=1,\ldots,10$ comme voie de référence et d'initialisation, tout en développant une génération directe des rangs 10 et 11 pour la latence.
6. Si le budget de candidats est dépassé, basculer vers une multicouverture sparse à précision fixée, pas vers une grille plus grossière silencieuse.

**Rôle de l'entropie :** proposer des supports, initialiser les selles et ordonner le branch-and-bound ; les rayons finaux restent durs.

**Critère de succès sous-seconde :** temps chaud end-to-end, données déjà résidentes GPU, incluant KNN/index, génération, certification, tri et forêt. Il faut publier séparément le taux de fermeture exacte, le nombre de candidats par point, le nombre d'événements critiques et le coût des prédicats adaptatifs.

### 8.2 Très grande dimension

**Objet mathématique calculable :** hiérarchie SoftLens exacte sur un atlas adaptatif, avec sortie finale $\tau=0$ lorsque la fidélité HGP est exigée.

**Backend principal :** `SoftLensBoruvka`.

1. Initialiser avec les facettes witnessed $Q_i=\{x_i\}\cup N_{K-1}(x_i;X\setminus\{x_i\})$.
2. Construire la forêt minimum des sites de puissance à $\tau=\infty$ (PMST).
3. Enrichir l'atlas aux minima, selles de la forêt et arêtes concurrentes.
4. Annealer $\tau$ vers zéro ; résoudre les poids par petits duaux batchés avec intervalles de certification.
5. Utiliser les arêtes non élémentaires pour proposer des colonnes de swap, conserver l'arête directe pour l'atlas courant, puis recalculer après enrichissement.
6. Répéter pour $k=1,\ldots,K$ si le coût le permet.
7. Condenser les branches sous un seuil de persistance ou de masse explicitement choisi.
8. Déclarer `atlas_exact`, `critical_complete` ou `conditional` ; ne jamais appeler « exact global » une simple saturation heuristique.

**Rôle de l'entropie :** elle définit une homotopie hiérarchique exploitable et un oracle GPU lisse. Elle ne supprime pas le pire cas $\binom{n}{K}$, exponentiel lorsque $K$ varie et déjà impraticable à $K=10$.

## 9. Résultats mathématiques à viser en priorité

### Résultat A — théorème SoftLens

Formaliser complètement la proposition de la section 2.3 : convexité des atomes, suffisance des swaps, équivalence des composantes, préservation par MSF, limites HGP et DTM. Ce résultat semble accessible sans hypothèse statistique.

### Résultat B — équivalence Gabriel–Morse en indice 1

Énoncer sous les conventions exactes de position générale de la thèse que les cofaces de Gabriel de cardinal $K+1$ sont en bijection avec les points critiques d'indice 1 de $d_{(K)}$. En déduire que le $K$-MST du chapitre 8 est la réduction par union–find du complexe de Morse $H_0$.

### Résultat C — récupération condensée sous marges critiques

Prouver qu'un atlas contenant tous les supports d'indice 0 et 1 au-dessus d'une persistance $\pi$, et dont les erreurs sont dominées par les marges de shell et barycentriques, restitue exactement la hiérarchie condensée. C'est la garantie pertinente pour l'algorithme pratique.

### Résultat D — complexité moyenne 3D

Sous un modèle de Poisson, ou sous des hypothèses explicites de densité bornée et de dimension intrinsèque 3, borner le nombre attendu de minima/selles $H_0$, le nombre de supports proposés et le travail des descentes. L'objectif est une complexité attendue $O(n)$ ou $O(n\log n)$ pour $K$ fixé.

### Résultat E — oracle certifié de supports et d'attaches

Construire un branch-and-bound couvrant ou un énumérateur équivalent qui certifie qu'aucun support de rang $K$ ou $K+1$ n'a été omis dans la plage de rayons pertinente, puis prouver que chaque descente de bras termine dans la bonne composante en restant sous son niveau de selle. La BVH sert aux bornes et aux comptages, mais ne constitue pas seule le certificat. C'est la pièce qui sépare une heuristique rapide d'un backend exact.

## 10. Décision finale

La voie recommandée n'est ni « DTM à la place de HGP », ni « UMAP d'ordre supérieur », ni « grille adaptative plus fine ». C'est une architecture à deux niveaux partageant le même invariant : une forêt couvrante des événements de la filtration.

$$
\boxed{\text{3D : complexe de Morse }H_0\text{ de }d_{(10)}\ \longrightarrow\ \text{hyperforêt exacte, sinon multicover sparse à précision certifiée}.}
$$

$$
\boxed{\text{grande dimension : SoftLens-HGP}\ \longrightarrow\ \text{atlas critique adaptatif}\ \longrightarrow\ \text{Borůvka implicite}.}
$$

Le point conceptuel le plus important est que l'entropie doit régulariser **le choix du point contraignant à l'intérieur d'une facette**, jamais la décision extérieure qui assemble les facettes. Cette position conserve simultanément :

- les régions témoins ;
- la filtration par sous-niveaux ;
- les interactions d'ordre supérieur ;
- la compression de toute la hiérarchie par une forêt ;
- le cas $K=1$, qui reste un EMST portant exactement la hiérarchie Single-Linkage ;
- la DTM, qui apparaît comme limite chaude et non comme changement arbitraire de problème.

Pour le cas 3D à 50 000 points, la priorité de recherche doit être un prototype `MorseHGP3D` sans grille, avec référence exacte par mosaïque/rhomboid sur petites tailles, puis mesure sur LiDAR du taux de fermeture des supports et des attaches. Le sous-seconde est un objectif expérimental crédible, pas une conséquence déjà démontrée de la réduction de Morse. Pour la grande dimension, la priorité est le noyau dual SoftLens $K\le10$, le Borůvka d'atlas et une campagne mesurant non seulement le temps, mais la stabilisation des naissances et des fusions au cours de l'enrichissement.

## Références principales

- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), 2024.
- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](https://arxiv.org/abs/2011.03617), 2020.
- R. Corbet, M. Kerber, M. Lesnick et G. Osang, [*Computing the Multicover Bifiltration*](https://arxiv.org/abs/2103.07823), 2021–2022.
- Á. J. Alonso, [*A Sparse Multicover Bifiltration of Linear Size*](https://arxiv.org/abs/2411.06986), version 2025.
- M. Buchet, B. B. Dornelas et M. Kerber, [*Sparse Higher Order Čech Filtrations*](https://arxiv.org/abs/2303.06666), 2023–2024.
- L. J. Guibas, Q. Mérigot et D. Morozov, [*Witnessed k-Distance*](https://arxiv.org/abs/1102.4972), 2011.
- H. Edelsbrunner et A. Nikitenko, [*Poisson-Delaunay Mosaics of Order k*](https://arxiv.org/abs/1709.09380), 2017–2018.
- A. Prokopenko, P. Sao et D. Lebrun-Grandié, [*A single-tree algorithm to compute the Euclidean minimum spanning tree on GPUs*](https://arxiv.org/abs/2207.00514), 2022.
- B. Taveira et al., [*Scalable GPU Construction of 3D Voronoi and Power Diagrams*](https://arxiv.org/abs/2605.06408), 2026.
- J. Erickson, [*Nice Point Sets Can Have Nasty Delaunay Triangulations*](https://arxiv.org/abs/cs/0103017), 2001.
