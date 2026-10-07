
## 6. La tour

### 6.1 Arbre de fusion

Soit $1 \leq k \leq n$ et $\Lambda_k = \lbrace \beta(G) : \lvert G \rvert \in \lbrace k, k+1 \rbrace \rbrace = \lbrace a_1 < \cdots < a_N \rbrace$ les **niveaux d'événement** de l'ordre $k$. $\Gamma_k(a)$ ne change qu'en ces niveaux : il est constant sur $\left[ a_j, a_{j+1} \right)$, et $\Gamma_k^{<}(a_j) = \Gamma_k(a_{j-1})$ (vide pour $j = 1$).

**Définition.** Pour chaque niveau $a_j$ et chaque composante $C$ de $\Gamma_k(a_j)$, soit $o(C)$ le nombre de composantes de $\Gamma_k^{<}(a_j)$ contenues dans $C$. Le nœud $\nu(C)$ est défini par récurrence sur $j$ :

- $o(C) = 0$ : **naissance** ; $\nu(C)$ est une feuille nouvelle, de niveau $a_j$ ;
- $o(C) = 1$ : **continuation** ; $\nu(C) = \nu(C')$ pour l'unique composante $C' \subseteq C$ de $\Gamma_k^{<}(a_j)$ : aucun nœud n'est créé ;
- $o(C) \geq 2$ : **multifusion** ; $\nu(C)$ est un nœud nouveau, de niveau $a_j$, dont les enfants sont les $\nu(C')$ des $o(C)$ composantes $C' \subseteq C$ de $\Gamma_k^{<}(a_j)$.

L'**arbre de fusion** $\mathcal{T}_k$ est l'ensemble de ces nœuds. Un nœud $v$ de niveau $a_v$ est **vivant à la coupe fermée** $a$ si $a_v \leq a$ et si $v$ est la racine ou $a < a_{\mathrm{parent}(v)}$ ; il est **vivant à la coupe ouverte** $a$ si $a_v < a$ et si $v$ est la racine ou $a \leq a_{\mathrm{parent}(v)}$. Pour $a \geq a_v$, $\mathrm{anc}_a(v)$ est l'ancêtre de $v$ (au sens large) vivant à la coupe fermée $a$. La **tour** est la famille $(\mathcal{T}_k)_{1 \leq k \leq K}$ munie des applications verticales de NERF-3.

**TOUR-A (arbre de fusion).** [démontré] (i) $C \mapsto \nu(C)$ est une bijection des composantes de $\Gamma_k(a)$, donc de $L_k(a)$ par NERF-2, sur les nœuds vivants à la coupe fermée $a$ ; de même des composantes de $\Gamma_k^{<}(a)$, donc de $L_k^{<}(a)$, sur les nœuds vivants à la coupe ouverte $a$. (ii) Le niveau croît **strictement** d'un enfant à son parent, et toute multifusion a au moins deux enfants. (iii) $\mathcal{T}_k$ a une racine unique. (iv) À l'ordre 1 les feuilles sont les $n$ sites, au niveau 0 ; pour $k \geq 2$ tous les niveaux sont $> 0$ ; à l'ordre $n$, $\mathcal{T}_n$ est une seule feuille de niveau $\beta(X)$.

*Preuve.* (i) et (ii), par récurrence sur $j$. Supposons que $\nu$ soit une bijection des composantes de $\Gamma_k(a_{j-1})$ sur les nœuds de niveau $\leq a_{j-1}$ sans parent de niveau $\leq a_{j-1}$ (pour $j = 1$ il n'y a ni composante ni nœud). Les composantes de $\Gamma_k^{<}(a_j)$ sont celles de $\Gamma_k(a_{j-1})$, et chacune est contenue dans une seule composante de $\Gamma_k(a_j)$. Une composante $C$ de $\Gamma_k(a_j)$ reçoit une feuille nouvelle si $o(C) = 0$, le nœud de son unique sous-composante si $o(C) = 1$, un nœud nouveau qui devient le parent des nœuds de ses $o(C) \geq 2$ sous-composantes sinon : $\nu$ est une bijection des composantes de $\Gamma_k(a_j)$ sur les nœuds de niveau $\leq a_j$ sans parent de niveau $\leq a_j$, c'est-à-dire sur les nœuds vivants à la coupe fermée $a$ pour tout $a \in \left[ a_j, a_{j+1} \right)$. La coupe ouverte en $a \in \left( a_{j-1}, a_j \right]$ est la coupe fermée $a_{j-1}$. Un parent, créé au niveau $a_j$, a des enfants créés à des niveaux $\leq a_{j-1} < a_j$. (iii) Pour $a \geq a_N$ toutes les $k$-parties sont des sommets et toutes les $(k+1)$-parties des arêtes : LOC-1 avec $H = X$. (iv) CAD-1 (iii). $\square$

### 6.2 Classification des événements d'un niveau

On fixe $k$ et un niveau $a > 0$. Un sommet ou une arête de $\Gamma_k(a)$ est **nouveau** s'il est de niveau exactement $a$, **ancien** sinon.

**TOUR-B (classification).** [démontré]

0. Tout sommet ou arête nouveau $\sigma$ a pour plus petite boule englobante une boule critique $b$ de niveau $a$, et $\sigma \subseteq P_b$. Deux boules distinctes de niveau $a$ n'ont ni sommet ni arête nouveau en commun. Les sommets nouveaux de $b$ sont les éléments de $V_k^{=}(b)$.
1. La partition de $\Gamma_k(a)$ en composantes est engendrée par celle de $\Gamma_k^{<}(a)$ et par les **blocs** $J_k(b)$, pour les boules critiques $b$ de niveau $a$ avec $p + m \geq k$ : toutes les $k$-parties de $P_b$ sont dans une même composante de $\Gamma_k(a)$. Une boule avec $p + m < k$ n'a aucun effet.
2. Pour une boule $b$ de niveau $a$ avec $p + m \geq k$, exactement un des trois cas se produit :
   - **naissance** : $p < k$ et $\mu_k(b) = 0$ (ce qui exige $t \geq q$, et a toujours lieu pour $k = p + m$). Alors $J_k(b)$ est une composante entière de $\Gamma_k(a)$, sans aucun sommet ancien, et aucun autre bloc ne la touche ;
   - **inerte** : $p \geq k$, ou $p < k \leq p + q - 2$. Alors $V_k^{<}(b)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(a)$, à laquelle $b$ attache ses sommets nouveaux ;
   - **fenêtre** : $p + q - 1 \leq k \leq p + m - 1$ et $\mu_k(b) \geq 1$. Les composantes de $\Gamma_k^{<}(a)$ rencontrées par $V_k^{<}(b)$ sont exactement celles des représentants des morceaux ; $b$ les réunit en une seule et lui attache ses sommets nouveaux.
3. Au niveau 0, seul l'ordre 1 a des sommets : les $n$ sites, qui sont $n$ naissances.

*Preuve.* 0. $\sigma$ a au moins deux sites puisque $\beta(\sigma) > 0$ ; CAT-2 et l'unicité de la plus petite boule englobante. 1. $\Gamma_k(a)$ s'obtient de $\Gamma_k^{<}(a)$ en ajoutant les sommets et arêtes nouveaux. Par LOC-1 appliqué à $H = P_b$ (de niveau $a$ par CAT-2), $J_k(b)$ est dans une seule composante de $\Gamma_k(a)$. Une arête nouvelle $G$ a ses faces dans $J_k(B(G))$, un sommet nouveau $F$ est dans $J_k(B(F))$, une arête ancienne relie deux sommets d'une même composante ancienne : la relation « même composante de $\Gamma_k(a)$ » est donc exactement celle qu'engendrent les composantes anciennes et les blocs. 2. Les cas s'excluent et couvrent tout : si $p < k$, alors $1 \leq t \leq m$ ; $t \leq q - 2$ est le cas inerte ; sinon $\mu_k(b) = 0$ est la naissance, et $\mu_k(b) \geq 1$ impose $t \leq m - 1$ (LOC-3 (1)). Naissance : $V_k^{<}(b) = \emptyset$ (LOC-3 (1)), donc tout élément de $J_k(b)$ est nouveau et a $b$ pour plus petite boule ; s'il appartenait à un autre bloc $J_k(b')$, CAT-2 lui donnerait $b'$ pour plus petite boule. $J_k(b)$ est disjoint des autres blocs et des sommets anciens : c'est une composante. Inerte : LOC-2 (a) et LOC-W. Fenêtre : LOC-3 (3). Dans ces deux cas les sommets nouveaux de $b$ n'appartiennent qu'au bloc $J_k(b)$, qui contient aussi $V_k^{<}(b) \neq \emptyset$. 3. $\beta(F) = 0$ équivaut à $\lvert F \rvert = 1$. $\square$

**Corollaires de TOUR-B.** [démontré]

- *Fenêtre.* Une boule n'est un événement de l'ordre $k$ (naissance ou réunion de plusieurs composantes) que si $p + q - 1 \leq k \leq p + m$ : c'est CAT-ADM.
- *Coquille régulière.* Naissance à $k = p + m$ ; jonction de $m$ représentants $I \cup U \setminus \lbrace u \rbrace$ à $k = p + m - 1$ ; inerte ailleurs (LOC-R).
- *Isolement des naissances.* Le nœud d'une naissance de niveau $a$ est vivant à la coupe fermée $a$ : il n'est jamais enfant d'une fusion de niveau $a$.
- *Non-injectivité.* Une cellule de fenêtre à plusieurs morceaux peut ne créer aucun nœud (fixture `morceaux_non_injectifs`) : les composantes des représentants sont **dédupliquées** avant toute réunion.

### 6.3 Forêt à plateaux

**TOUR-C (plateaux atomiques).** [démontré] Au niveau $a$, soit $H_a$ le graphe biparti dont les sommets sont, d'un côté, les composantes de $\Gamma_k^{<}(a)$ et, de l'autre, les boules de niveau $a$ des cas inerte et fenêtre, chaque boule étant reliée aux composantes que rencontre $V_k^{<}(b)$. Alors les composantes de $\Gamma_k(a)$ qui contiennent un sommet ancien correspondent bijectivement aux composantes connexes de $H_a$, et $o(C)$ est le nombre de composantes anciennes de la composante de $H_a$ correspondante. Les multifusions de niveau $a$ sont donc les composantes de $H_a$ qui contiennent au moins deux composantes anciennes, avec exactement celles-ci pour enfants ; les naissances de niveau $a$ sont les cellules de naissance de TOUR-B.

*Preuve.* Par TOUR-B (1) et (2) : les blocs de naissance sont des composantes à part ; identifier un bloc $J_k(b)$ inerte ou de fenêtre revient à réunir les composantes anciennes rencontrées par $V_k^{<}(b)$ et à leur attacher $V_k^{=}(b)$, dont les éléments ne sont dans aucun autre bloc. $\square$

Conséquence pour la construction : à un niveau donné, les classes des représentants sont lues **avant** toute réunion de ce niveau, et chaque groupe d'au moins deux classes devient **un** nœud dont les enfants sont exactement ces classes. Traiter les jonctions d'un même niveau une à une, en créant un nœud à chacune, produit une chaîne de fusions de même niveau : c'est faux (fixture `carre` à l'ordre 1 : une seule fusion de quatre enfants au niveau 1, portée par quatre boules de même niveau).

### 6.4 Descente

**TOUR-D (descente).** [démontré] Soit $k \geq 2$, $F$ une $k$-partie (donc $\beta(F) > 0$) et $b = B(F)$ ; alors $F \in V_k^{=}(b)$ et $k \leq p + m$. (À l'ordre 1, une 1-partie est la feuille de son site : il n'y a rien à descendre.) Un **pas valide** $F \to F'$ est :

- si $p \geq k$ : $F'$ est une $k$-partie **quelconque** de $I$ ;
- si $p < k$ et $\mu_k(b) = 0$ : arrêt ; $F$ est un sommet de la naissance $(b,k)$ ;
- si $p < k$ et $\mu_k(b) \geq 1$ : $F' = I \cup A$ pour une partie séparable $A \subseteq U$ de cardinal $t$ **quelconque**.

Alors $\beta(F') < \beta(F)$ strictement, et $F$ et $F'$ sont dans la même composante de $\Gamma_k(\beta(F))$. Toute suite de pas valides s'arrête donc, après un nombre fini de pas, sur une naissance $N(F)$ de niveau $\leq \beta(F)$, et pour tout $a \geq \beta(F)$ le nœud de la composante de $F$ dans $\Gamma_k(a)$ est $\mathrm{anc}_a(N(F))$. Si $\beta(F) < a'$, $F$ et $N(F)$ sont déjà dans la même composante de $\Gamma_k^{<}(a')$.

*Preuve.* $F'$ a une trace séparable sur $U$ (vide dans le premier cas), donc $\beta(F') < a_b = \beta(F)$ par CAT-2. $F \neq F'$ sont deux $k$-parties de $P_b$, dans la même composante de $\Gamma_k(a_b)$ par LOC-1. Chaque $k$-partie relève d'un seul des trois cas, deux d'entre eux font décroître strictement le niveau, et les niveaux forment un ensemble fini : le cas d'arrêt finit par se produire. Tous les niveaux traversés sont $\leq \beta(F)$, donc toutes les parties traversées sont dans la composante de $F$ à tout niveau $a \geq \beta(F)$. $\square$

**Corollaires de TOUR-D.** [démontré]

- *Ce qui dépend des choix.* Le terminal $N(F)$ peut dépendre des choix (fixture `ligne_024`, ordre 2 : la paire extrême, de niveau 4, descend vers la paire de gauche ou vers celle de droite, deux naissances de niveau 1). Sa **classe** aux coupes $a \geq \beta(F)$ n'en dépend pas : c'est tout ce que la construction utilise. « Fonction pure » ne se dit que d'une politique de choix déterministe fixée ; deux politiques valides donnent la même forêt.
- *Raccourcis.* Si $F = P_b$ est la population d'une naissance avec $\lvert P_b \rvert = k$, la descente s'arrête sans pas. Un terminal mémorisé pour une boule $b$ et un ordre $k$ peut être rendu pour toute $F \in V_k^{=}(b)$ : il est dans la classe de $F$ aux coupes $a \geq a_b$, les seules où $F$ existe. Il ne dit rien des représentants de $b$, de niveau $< a_b$, qui se résolvent chacun par sa propre descente.
- *Nombre de pas.* Aucune borne autre que le nombre de niveaux inférieurs à $\beta(F)$ n'est démontrée (OBL-5).

### 6.5 Exactitude relative au catalogue

**TOUR-E (exactitude relative au catalogue).** [démontré, sous les hypothèses (H1) à (H4)] Hypothèses : (H1) sites distincts ; (H2) le catalogue contient toute boule critique avec $p + q \leq K + 1$, avec $I$ et $U$ exacts, et le recensement d'une boule rencontrée hors catalogue est exact ; (H3) les niveaux sont comparés exactement ; (H4) pour chaque cellule de fenêtre, les représentants utilisés sont des éléments de $V_k^{<}(b)$ et toute composante de $\Gamma_k^{<}(a_b)$ rencontrée par $V_k^{<}(b)$ en contient au moins un (c'est le cas d'un représentant par morceau, et de tout raffinement exhaustif au sens de LOC-3 (5)). Alors, pour chaque $k \leq \min(K, n)$, la forêt obtenue en

1. créant une feuille par cellule de naissance (et, à l'ordre 1, par site),
2. envoyant chaque représentant sur la naissance où aboutit une descente valide,
3. appliquant TOUR-C, niveau par niveau, aux groupes de classes ainsi formés,

est l'arbre de fusion $\mathcal{T}_k$.

*Preuve.* Les cellules de naissance et de fenêtre d'un ordre $k \leq K$ ont $p + q \leq k + 1 \leq K + 1$ : leurs boules sont au catalogue. Récurrence sur les niveaux d'événement, avec l'invariant : après le niveau $a$, deux feuilles sont dans la même classe si et seulement si elles sont dans la même composante de $\Gamma_k(a)$, et toute composante de $\Gamma_k(a)$ contient une feuille (TOUR-D appliqué à l'un de ses sommets). Au niveau $a$ : un représentant $F$ d'une cellule de niveau $a$ a $\beta(F) < a$, sa descente aboutit à une feuille de sa composante dans $\Gamma_k^{<}(a)$ (TOUR-D), donc de la classe correspondante par l'invariant au niveau précédent. Par (H4) et TOUR-B, les classes des représentants d'une cellule sont exactement les composantes anciennes qu'elle rencontre ; les boules inertes, au catalogue ou non, n'en rencontrent qu'une et ne changent rien. TOUR-C donne les multifusions et l'invariant au niveau $a$. $\square$

**Portée.** La tour est exacte **relativement** à (H2). Une boule de fenêtre absente du catalogue déplace ou supprime une fusion sans rompre TOUR-A (fixture `triangle_scalene`, § 7.1). Deux protections, de natures différentes : (a) dans le produit, une descente qui rencontre une plus petite boule avec $p + q \leq K + 1$ absente du catalogue **refuse** (elle ne poursuit pas avec un représentant calculé à la volée) — cela ne dit rien d'une boule jamais rencontrée ; (b) INV-EULER comme porte d'échelle, qui est une condition nécessaire et non un certificat (§ 7.1). La complétude elle-même est le théorème GEN-G.

### 6.6 Verticales

**TOUR-F (verticales à la coupe fermée).** [démontré] Soit $k \geq 2$. (i) Si $v$ est une naissance de l'ordre $k$ portée par $b$, son image par $\phi_k^{a_b}$ est le nœud de la composante de $\Gamma_{k-1}(a_b)$ qui contient **toutes** les $(k-1)$-parties de $P_b$. (ii) Si $v$ est une multifusion de niveau $a$, son image par $\phi_k^{a}$ est $\mathrm{anc}_a$ de l'image, prise à son niveau de création, de n'importe lequel de ses enfants ; tous les enfants donnent le même nœud. (iii) Pour un nœud $v$ et $a \geq a_v$, $\phi_k^{a}(\mathrm{anc}_a(v)) = \mathrm{anc}_a(\phi_k^{a_v}(v))$.

*Preuve.* (i) $\lvert P_b \rvert \geq k = (k-1) + 1$ : LOC-1 à l'ordre $k - 1$, avec $\beta(P_b) = a_b$. (ii) et (iii) : NERF-3. $\square$

L'image d'une naissance se calcule par descente, à l'ordre $k - 1$, d'une $(k-1)$-partie quelconque de $P_b$, puis $\mathrm{anc}_{a_b}$. La coupe est **fermée** : la composante image peut être créée au niveau $a_b$ lui-même (jonction de la même boule à l'ordre $k - 1$) et n'existe alors pas à la coupe ouverte.

### 6.7 Couvertures et continuations

La **couverture** d'un nœud $v$ vivant à la coupe fermée $a$ est la réunion des sommets de sa composante ; par NERF-4 c'est l'amas discret de la composante de $L_k(a)$, le $K$-polyèdre de la thèse.

**TOUR-G (couvertures).** [démontré]

1. *Formule exacte.* La couverture de $v$ à la coupe fermée $a$ est la réunion des $P_b$ pour les boules $b$ (critiques, ou de rayon nul si $k = 1$) de niveau $a_b \leq a$ telles que $p + q \leq k \leq p + m$ et dont le bloc $J_k(b)$ est dans la composante de $v$. À la coupe ouverte : mêmes boules avec $a_b < a$.
2. *Cas régulier.* Si aucune boule critique à coquille étendue ne vérifie $p + q \leq k \leq p + m - 1$, la couverture de $v$ est la réunion des populations $P_b$ des naissances de son sous-arbre : ni continuation ni fusion n'apporte de site.
3. *Sinon c'est faux* [faux] : une cellule de fenêtre à un seul morceau peut ajouter un site à une composante sans créer de nœud. Fixture `gain_de_couverture` : sites $(8,9,0)$, $(5,10,0)$, $(2,9,0)$, $(5,0,0)$, sur le cercle de centre $(5,5,0)$ et de niveau 25 ; à l'ordre 3 l'arbre a un seul nœud, né au niveau 9 ; à la coupe ouverte 25 sa couverture a trois sites, à la coupe fermée 25 elle en a quatre.
4. *Contribution d'une cellule.* Pour une boule inerte, $\bigcup V_k^{<}(b) = P_b$. Pour une cellule de fenêtre, les sites de $P_b$ absents de $\bigcup V_k^{<}(b)$ sont exactement ceux de $U$ qui n'appartiennent à aucune partie séparable de cardinal $t$ (calculables par LOC-4 (c)) ; il n'y en a aucun si $t \leq q - 1$. Seule une cellule de coquille étendue avec $t \geq q$ peut donc apporter un site sans naissance.

*Preuve.* 1. Si $J_k(b)$ est dans la composante, $P_b = \bigcup J_k(b)$ est dans la couverture ($\lvert P_b \rvert \geq k$). Inversement soit $x$ dans la couverture, et $F \ni x$ un sommet de la composante de niveau minimal parmi ceux qui contiennent $x$ ; soit $b = B(F)$ (boule de rayon nul si $\beta(F) = 0$). Supposons $p + q \geq k + 1$. Si $x \in I$, on prend $A' \subseteq U$ de cardinal $\max(k - p, 0) \leq q - 1$ et $I' \subseteq I$ contenant $x$ de cardinal $k - \lvert A' \rvert$ ; si $x \in U$, on prend $A' \subseteq U$ contenant $x$ de cardinal $\max(k - p, 1) \leq q - 1$ et $I' \subseteq I$ de cardinal $k - \lvert A' \rvert$. Dans les deux cas $F' = I' \cup A'$ est une $k$-partie de $P_b$ qui contient $x$, de trace séparable (CAT-1 (v)), donc de niveau $< a_b$ (CAT-2), et dans la composante de $F$ (LOC-1) : contradiction avec la minimalité. Donc $p + q \leq k$ ; de plus $k = \lvert F \rvert \leq p + m$, $a_b = \beta(F) \leq a$, et $J_k(b)$ est dans la composante de $F$. 2. Une boule régulière avec $p + q \leq k \leq p + m$ a $k = p + m$ : c'est une naissance ; une boule étendue aussi, par hypothèse ; ces naissances sont dans le sous-arbre de $v$. 3. Valeurs lues dans l'oracle de définition. 4. Si $p \geq k$, tout site de $I$ est dans une $k$-partie de $I$ et tout site $x \in U$ dans $\lbrace x \rbrace$ complété par $k - 1$ sites de $I$. Si $p < k$, tout site de $I$ est dans un $I \cup A$, et un site de $U$ est dans une $F \in V_k^{<}(b)$ si et seulement s'il est dans une partie séparable de cardinal $t$ ; pour $t \leq q - 1$ toute partie de cardinal $t$ est séparable et $m \geq t$. $\square$

Conséquence : la forêt et les populations des naissances ne suffisent pas à restituer les amas discrets hors du cas régulier. L'objet à publier est la **relation de couverture** : pour chaque cellule avec $p + q \leq k \leq p + m$, le triplet (boule, niveau $a_b$, nœud $\mathrm{anc}_{a_b}$ du terminal d'une $k$-partie de $P_b$). Pour une coquille régulière il n'y a que la cellule de naissance.

### 6.8 Construction sans lots

Cadre abstrait : des feuilles ; des **hyperarêtes** $e$, chacune de rang entier $r(e)$ et portant un ensemble de feuilles ; $\mathcal{G}_s$ est l'hypergraphe des hyperarêtes de rang $\leq s$. Les nœuds à construire sont les couples $(C, s)$ où $C$ est une composante de $\mathcal{G}_s$ qui réunit au moins deux composantes de $\mathcal{G}_{s-1}$, ses enfants étant ces composantes (TOUR-C, avec pour hyperarêtes les cellules de fenêtre, pour rang celui de leur niveau, pour feuilles les terminaux de leurs représentants ; une feuille née au rang $s$ n'est touchée que par des hyperarêtes de rang $> s$).

**TOUR-J (règle cartésienne).** [démontré] On traite les hyperarêtes par rang croissant, dans un ordre quelconque à rang égal, avec une structure union-find dont chaque classe est une liste : une hyperarête de rang $s$ qui touche $j \geq 2$ classes distinctes concatène leurs listes dans un ordre quelconque et inscrit la valeur $s$ aux $j - 1$ jonctions. Soit $\pi$ l'ordre final et $J(i)$ la valeur inscrite entre les positions $i$ et $i + 1$ (infinie entre deux listes jamais réunies). Alors : (P) pour tout $s$, les composantes de $\mathcal{G}_s$ sont les intervalles maximaux de $\pi$ dont toutes les jonctions internes sont $\leq s$ ; les nœuds de rang $s$ sont les intervalles maximaux de jonctions $\leq s$ dont le maximum vaut $s$, et leurs enfants sont les morceaux séparés par les jonctions égales à $s$.

*Preuve.* Après les hyperarêtes de rang $\leq s$, chaque classe est une liste contiguë dont les jonctions internes sont $\leq s$, et les classes sont les composantes de $\mathcal{G}_s$ ; les concaténations ultérieures n'inscrivent que des valeurs $> s$, aux bords des listes : d'où (P). Un intervalle maximal de jonctions $\leq s$ est une composante de $\mathcal{G}_{s-1}$ si et seulement si toutes ses jonctions sont $\leq s - 1$ ; sinon ses sous-intervalles maximaux de jonctions $\leq s - 1$, séparés par les jonctions égales à $s$, sont les composantes de $\mathcal{G}_{s-1}$ qu'il réunit. $\square$

**TOUR-J2 (contraction).** [démontré] Variante : on traite les hyperarêtes dans le même ordre avec un union-find ordinaire, chaque réunion de deux classes créant un nœud binaire de rang $s$. En contractant chaque ensemble maximal de nœuds binaires de même rang reliés par des liens parent-enfant, on obtient exactement les nœuds de TOUR-C.

*Preuve.* À la fin du rang $s - 1$ les classes sont les composantes de $\mathcal{G}_{s-1}$. Une composante $C$ de $\mathcal{G}_s$ qui en réunit plusieurs est construite par des réunions de rang $s$ ; ces nœuds binaires forment un sous-arbre connexe (tout nœud de rang $s$ de $C$ a pour ancêtres, jusqu'au sommet de $C$ à la fin du rang $s$, des nœuds de rang $s$), dont les enfants extérieurs sont les sommets des composantes de $\mathcal{G}_{s-1}$ contenues dans $C$. $\square$
