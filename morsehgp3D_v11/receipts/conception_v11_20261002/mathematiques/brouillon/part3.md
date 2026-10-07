
## 4. Le nerf : des composantes de $L_k(a)$ au graphe $\Gamma_k(a)$

### 4.1 Les graphes

Pour $1 \leq k \leq n$ et $a \geq 0$, $\Gamma_k(a)$ a pour **sommets** les $k$-parties $F$ avec $\beta(F) \leq a$ ; chaque $(k+1)$-partie $G$ avec $\beta(G) \leq a$ est une **arête** qui relie deux à deux ses $k + 1$ faces $G \setminus \lbrace g \rbrace$ (toutes sommets, par CAD-1 (iii)). $\Gamma_k^{<}(a)$ est défini de même avec les inégalités strictes. À l'ordre $n$ il n'y a pas d'arête et un seul sommet possible, $X$.

### 4.2 Lemme topologique

**NERF-1 (réunion finie de connexes).** [démontré] Soit $\mathcal{F}$ une famille finie de parties connexes non vides de $\mathbb{R}^{3}$, toutes fermées ou toutes ouvertes, et $N$ son graphe d'intersection (deux membres sont reliés quand ils se rencontrent). Les composantes connexes de la réunion sont exactement les réunions $L_{\mathcal{C}} = \bigcup_{T \in \mathcal{C}} T$, $\mathcal{C}$ parcourant les composantes de $N$.

*Preuve.* $L_{\mathcal{C}}$ est connexe : on range les membres de $\mathcal{C}$ de sorte que chacun rencontre l'un des précédents, et la réunion de deux connexes qui se rencontrent est connexe. Deux $L_{\mathcal{C}}$ distincts sont disjoints, sinon une arête de $N$ relierait deux composantes. Les $L_{\mathcal{C}}$ sont en nombre fini, deux à deux disjoints, tous fermés (ou tous ouverts) dans la réunion ; chacun est donc aussi ouvert (ou fermé) dans la réunion, comme complémentaire de la réunion finie des autres. Une partie connexe, ouverte et fermée est une composante connexe. $\square$

### 4.3 Théorème du nerf

**NERF-2 (nerf, coupes fermée et ouverte).** [démontré] Pour $1 \leq k \leq n$ et $a \geq 0$, l'application qui envoie un sommet $F$ sur la composante de $L_k(a)$ contenant $R_F(a)$ induit une bijection de $\pi_0(\Gamma_k(a))$ sur $\pi_0(L_k(a))$. Pour $a > 0$, l'application $F \mapsto R_F^{<}(a)$ induit de même une bijection de $\pi_0(\Gamma_k^{<}(a))$ sur $\pi_0(L_k^{<}(a))$.

*Preuve.* Par CAD-2, $L_k(a)$ est la réunion des $R_F(a)$, $F$ sommet, convexes compacts non vides. Deux d'entre eux se rencontrent si et seulement si $R_{F \cup F'}(a) \neq \emptyset$, c'est-à-dire $\beta(F \cup F') \leq a$. Une arête de $\Gamma_k(a)$ est donc une arête du graphe d'intersection. Inversement, si $H = F \cup F'$ vérifie $\beta(H) \leq a$ et $F \neq F'$, on passe de $F$ à $F'$ en remplaçant à chaque pas un site de $F \setminus F'$ par un site de $F' \setminus F$ : chaque partie intermédiaire est un sommet (elle est dans $H$), et deux parties consécutives ont pour réunion une $(k+1)$-partie de $H$, de niveau $\leq \beta(H) \leq a$, donc une arête de $\Gamma_k(a)$. Les deux graphes ont les mêmes composantes et NERF-1 conclut. La coupe ouverte se traite mot pour mot avec $R^{<}$, convexes ouverts, et des inégalités strictes. $\square$

C'est le théorème 2 du manuscrit complété par sa proposition 5 pour la coupe fermée ; la coupe ouverte et NERF-3 n'y figurent pas.

**NERF-3 (naturalité).** [démontré] (a) Horizontale : pour $a \leq a'$, $\Gamma_k(a)$ est un sous-graphe de $\Gamma_k(a')$, $\Gamma_k^{<}(a) \subseteq \Gamma_k(a)$, et $\Gamma_k(a) \subseteq \Gamma_k^{<}(a')$ si $a < a'$ ; les bijections de NERF-2 commutent aux inclusions. (b) Verticale : pour $k \geq 2$, l'application $\phi_k^{a}$ qui envoie la composante de $F$ dans $\Gamma_k(a)$ sur la composante de $F \setminus \lbrace x \rbrace$ dans $\Gamma_{k-1}(a)$ est bien définie (elle ne dépend ni de $x \in F$ ni du représentant $F$), commute aux inclusions horizontales, et correspond par NERF-2 à l'application induite par $L_k(a) \subseteq L_{k-1}(a)$.

*Preuve.* (a) Les conditions $\beta \leq a$, $\beta < a$, $\beta \leq a'$ s'impliquent dans cet ordre, et $R_F(a) \subseteq R_F(a')$ : la composante de $L_k(a')$ qui contient $R_F(a')$ contient celle de $L_k(a)$ qui contient $R_F(a)$. (b) $F \setminus \lbrace x \rbrace$ est un sommet de $\Gamma_{k-1}(a)$ (CAD-1 (iii)). Deux $(k-1)$-parties de $F$ ont pour réunion $F$, de niveau $\leq a$ : elles sont reliées dans $\Gamma_{k-1}(a)$. Si $F$ et $F'$ sont reliés par $G$, alors $F \cap F'$ est une $(k-1)$-partie commune aux deux. Enfin $R_F(a) \subseteq R_{F \setminus \lbrace x \rbrace}(a)$ : la composante de $L_{k-1}(a)$ qui contient la composante de $F$ est celle de $F \setminus \lbrace x \rbrace$. $\square$

**NERF-4 (couverture et cœur).** [démontré] Soit $C$ une composante de $L_k(a)$ et $\mathcal{C}$ la composante correspondante de $\Gamma_k(a)$. (a) $\lbrace x \in X : \exists y \in C, \lVert x - y \rVert^{2} \leq a \rbrace = \bigcup_{F \in \mathcal{C}} F$ : l'**amas discret** de $C$ (thèse, définition 8) est la **couverture** de $\mathcal{C}$. (b) $x \in C$ si et seulement si $D_k(x) \leq a$ et les $k$-parties de $X \cap \bar{B}(x,a)$ sont dans $\mathcal{C}$ ; ces $k$-parties sont toutes dans une même composante.

*Preuve.* (a) Si $x \in F \in \mathcal{C}$, tout $y \in R_F(a) \subseteq C$ convient. Si $y \in C$ et $\lVert x - y \rVert^{2} \leq a$, alors $\bar{B}(y,a)$ contient au moins $k$ sites dont $x$ ; une $k$-partie $F \ni x$ de ces sites vérifie $y \in R_F(a)$, donc $R_F(a)$ rencontre $C$, y est contenu, et $F \in \mathcal{C}$. (b) $x \in L_k(a)$ équivaut à $D_k(x) \leq a$, et alors $x \in R_F(a)$ exactement pour les $k$-parties $F$ de $X \cap \bar{B}(x,a)$ : leurs régions témoins se rencontrent toutes en $x$. $\square$

## 5. Structure locale d'une boule critique

Dans tout ce paragraphe $b = (c, a)$ est une boule critique, $I$, $U$, $p$, $m$, $q$ ses données, $k$ un ordre, $t = k - p$.

### 5.1 Parties séparables

Une partie $A \subseteq U$ est **séparable** si $c \notin \mathrm{conv}(A)$ (enveloppe fermée) ; par CAD-S cela équivaut à l'existence de $v$ avec $\langle v, x - c \rangle > 0$ pour tout $x \in A$. La partie vide est séparable ; toute partie d'une partie séparable est séparable ; $U$ ne l'est pas ; toute partie de moins de $q$ sites l'est (CAT-1 (v)) ; une partie non séparable contient un support, donc au moins $q$ sites (CAT-1 (i)).

On note $J_k(b)$ l'ensemble des $k$-parties de $P_b$, $V_k^{<}(b)$ celles dont la trace $F \cap U$ est séparable, $V_k^{=}(b)$ les autres. Par CAT-2 : $V_k^{<}(b)$ est l'ensemble des $k$-parties de $P_b$ de niveau $< a$, et $V_k^{=}(b)$ l'ensemble des $k$-parties dont la plus petite boule englobante est $b$.

### 5.2 Lemmes d'échange

**LOC-1 (Johnson).** [démontré] Soit $H \subseteq X$ avec $\lvert H \rvert \geq k$. Les $k$-parties de $H$ sont dans une même composante de $\Gamma_k(\beta(H))$ ; si $\beta(H) < a'$, elles sont dans une même composante de $\Gamma_k^{<}(a')$.

*Preuve.* Deux $k$-parties de $H$ se rejoignent en échangeant un site à la fois ; deux parties consécutives ont pour réunion une $(k+1)$-partie de $H$, de niveau $\leq \beta(H)$. $\square$

**LOC-2 (structure stricte).** [démontré] Si $H \subseteq P_b$, $\lvert H \rvert \geq k$ et $H \cap U$ est séparable, alors $\beta(H) < a$ et toutes les $k$-parties de $H$ sont dans une même composante de $\Gamma_k^{<}(a)$. En conséquence :

- (a) si $p \geq k$ : $V_k^{<}(b)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(a)$ ;
- (b) si $p < k$ : toute $F \in V_k^{<}(b)$ est dans la composante, dans $\Gamma_k^{<}(a)$, de $I \cup A$ pour chaque $A \subseteq F \cap U$ de cardinal $t$ (il en existe : $\lvert F \cap U \rvert \geq t$) ;
- (c) si $p < k$ et si $A$, $A'$ sont deux parties de $U$ de cardinal $t$ dont la réunion est séparable : $I \cup A$ et $I \cup A'$ sont dans la même composante de $\Gamma_k^{<}(a)$.

*Preuve.* CAT-2 donne $\beta(H) < a$, et LOC-1 le reste. (a) Pour $F \in V_k^{<}(b)$ et $N \subseteq I$ de cardinal $k$, prendre $H = F \cup I$, dont la trace sur $U$ est celle de $F$. (b) Même $H = F \cup I$. (c) $H = I \cup A \cup A'$. $\square$

### 5.3 Morceaux

Pour $p < k \leq p + m$, le **graphe local** $\mathcal{S}_t(b)$ a pour sommets les parties séparables de $U$ de cardinal $t$, deux sommets étant reliés quand leur réunion est séparable. Les **morceaux** de $b$ à l'ordre $k$ sont ses composantes connexes ; $\mu_k(b)$ est leur nombre. Un **représentant** d'un morceau est une $k$-partie $I \cup A$, $A$ sommet du morceau.

**LOC-3 (morceaux).** [démontré] Pour $p < k \leq p + m$ :

1. $\mu_k(b) = 0$ si et seulement si $V_k^{<}(b) = \emptyset$ ; cela exige $t \geq q$, et c'est toujours le cas pour $t = m$.
2. Les morceaux sont en bijection avec les composantes du graphe **induit sur $P_b$** par $\Gamma_k^{<}(a)$ (sommets $V_k^{<}(b)$, arêtes les $(k+1)$-parties de $P_b$ à trace séparable) : la composante de $F$ correspond au morceau des parties de cardinal $t$ de $F \cap U$.
3. Les composantes de $\Gamma_k^{<}(a)$ **rencontrées** par $V_k^{<}(b)$ sont exactement celles des représentants, un par morceau. L'application des morceaux vers ces composantes est surjective ; elle n'est **pas injective** en général, deux morceaux pouvant être reliés hors de $P_b$.
4. Le critère « $A$ et $A'$ sont deux faces d'une même partie séparable de cardinal $t + 1$ » a la même clôture transitive que « $A \cup A'$ est séparable ».
5. Raffinement : si l'on remplace les morceaux par une partition **exhaustive** plus fine de l'ensemble des sommets de $\mathcal{S}_t(b)$ (chaque bloc contenu dans un morceau, tous les sommets couverts) et que l'on prend un représentant par bloc non vide, l'ensemble des composantes de $\Gamma_k^{<}(a)$ des représentants est inchangé. Un sous-échantillon de représentants qui laisse un morceau sans représentant ne bénéficie pas de cet énoncé.

*Preuve.* 1. Si $A$ est un sommet, $I \cup A \in V_k^{<}(b)$ ; si $F \in V_k^{<}(b)$, toute partie de cardinal $t$ de $F \cap U$ est un sommet. Une partie de cardinal $t < q$ est séparable, et $U$ ne l'est pas. 2. Les parties de cardinal $t$ d'une même trace séparable sont deux à deux reliées dans $\mathcal{S}_t(b)$ ; une arête induite $G$ entre $F$ et $F'$ a une trace séparable qui contient celles de $F$ et de $F'$ ; donc $F \mapsto$ (morceau des parties de $F \cap U$) est constante sur les composantes induites. Elle est surjective par $A \mapsto I \cup A$, et injective car LOC-2 (b) et (c) se démontrent avec des arêtes de $P_b$. 3. LOC-2 (b) et (c) ; la non-injectivité est la fixture ci-dessous. 4. Si $A \cup A'$ est séparable et $A \neq A'$, on échange un site à la fois dans $A \cup A'$ ; deux parties consécutives sont des faces d'une partie de cardinal $t + 1$ de $A \cup A'$, séparable. Inversement deux faces distinctes d'une partie séparable de cardinal $t + 1$ ont cette partie pour réunion. 5. Chaque composante rencontrée contient un $I \cup A$ (LOC-2 (b)) ; $A$ est dans un bloc, dont le représentant est dans le même morceau, donc dans la même composante (LOC-2 (c)). $\square$

Fixture `morceaux_non_injectifs` (auditeur indépendant, 2 octobre 2026) : sites $(0,0,0)$, $(2,0,0)$, $(4,0,0)$, $(2,3,0)$, ordre 2. La boule de centre $(2,0,0)$ et de niveau 4 a $I = \lbrace (2,0,0) \rbrace$, $U = \lbrace (0,0,0), (4,0,0) \rbrace$, deux morceaux ; leurs représentants sont déjà dans la même composante depuis le niveau $13/4$ (fusion ternaire par les deux triangles rectangles) : aucun nœud n'est créé au niveau 4.

**LOC-4 (quotient par une famille couvrante).** [démontré] Soit $\mathfrak{S}$ une famille de parties séparables de $U$ telle que toute partie séparable de $U$ soit contenue dans un membre de $\mathfrak{S}$, et $\mathfrak{S}_t = \lbrace M \in \mathfrak{S} : \lvert M \rvert \geq t \rbrace$. Alors : (a) $\mu_k(b) = 0$ si et seulement si $\mathfrak{S}_t = \emptyset$ ; (b) les morceaux sont en bijection avec les composantes du graphe sur $\mathfrak{S}_t$ où $M$ et $M'$ sont reliés quand $\lvert M \cap M' \rvert \geq t$, le morceau de $A$ correspondant à la composante de tout $M \supseteq A$ ; (c) les sites de $U$ qui n'appartiennent à aucune partie séparable de cardinal $t$ sont ceux de $U \setminus \bigcup \mathfrak{S}_t$.

*Preuve.* (a) et (c) : une partie séparable de cardinal $t$ est dans un $M \in \mathfrak{S}_t$, et toute partie de cardinal $t$ d'un tel $M$ est séparable. (b) Si $A \subseteq M$ et $A \subseteq M'$, alors $\lvert M \cap M' \rvert \geq t$ : l'application est bien définie. Si $A \cup A'$ est séparable, un même $M$ contient $A$ et $A'$ : elle est constante sur les morceaux. Elle est surjective. Si $M_0, \ldots, M_r$ est une chaîne avec $\lvert M_i \cap M_{i+1} \rvert \geq t$, $A \subseteq M_0$ et $A' \subseteq M_r$, on choisit $B_i \subseteq M_i \cap M_{i+1}$ de cardinal $t$ : $A$, $B_0$, $B_1$, …, $A'$ sont consécutivement dans un même membre, donc reliés ou égaux dans $\mathcal{S}_t(b)$ : elle est injective. $\square$

**LOC-5 (une famille couvrante finie).** [démontré] Notons $u_x = x - c$ pour $x \in U$ ; pour $v \neq 0$, $U^{+}(v) = \lbrace x \in U : \langle v, u_x \rangle > 0 \rbrace$ et $U^{0}(v) = \lbrace x \in U : \langle v, u_x \rangle = 0 \rbrace$ ; pour $l \in U^{0}(v)$, $W(v,l) = \lbrace l \rbrace \cup \lbrace y \in U^{0}(v) : \langle v, u_l \times u_y \rangle > 0 \rbrace$ (les sites du grand cercle $v^{\perp}$ situés à moins d'un demi-tour après $l$ dans le sens direct autour de $v$). Si $m \geq 3$, la famille des $U^{+}(v_0) \cup W(v_0, l)$, où $v_0 = \pm (u_i \times u_j)$ pour $i, j \in U$ avec $u_i \times u_j \neq 0$ et $l \in U^{0}(v_0)$, est une famille couvrante au sens de LOC-4. Si $m = 2$ ($U$ est une paire antipodale), les parties séparables non vides sont les deux singletons.

*Preuve.* Chaque membre est séparable : dans le plan $v_0^{\perp}$, $e = v_0 \times u_l$ vérifie $\langle e, u_y \rangle = \langle v_0, u_l \times u_y \rangle > 0$ pour $y \in W(v_0,l) \setminus \lbrace l \rbrace$ et $\langle e, u_l \rangle = 0$ ; pour $\eta > 0$ puis $\varepsilon > 0$ assez petits, $v = v_0 + \varepsilon (e + \eta u_l)$ sépare $U^{+}(v_0) \cup W(v_0,l)$. Couverture : soit $A$ séparable maximale et $C = \lbrace v : \langle v, u_x \rangle > 0 \ \forall x \in A \rbrace$, cône ouvert non vide. Pour $v \in C$, $U^{+}(v) \supseteq A$ est séparable, donc $U^{+}(v) = A$ par maximalité : ainsi $\langle v, u_y \rangle \leq 0$ pour $y \notin A$, sur $C$ et sur son adhérence $\bar{C} = \lbrace v : \langle v, u_x \rangle \geq 0 \ \forall x \in A \rbrace$. Si les $u_x$, $x \in A$, engendrent $\mathbb{R}^{3}$, $\bar{C}$ est un cône polyédral pointé d'intérieur non vide ; une de ses génératrices extrêmes est portée par un $v_0 \neq 0$ orthogonal à deux $u_i$, $u_j$ non colinéaires, $i, j \in A$, donc $v_0$ est un multiple positif de $u_i \times u_j$ ou de son opposé. Comme $v_0 \in \bar{C}$ : $U^{+}(v_0) \subseteq A \subseteq U^{+}(v_0) \cup U^{0}(v_0)$. La projection $v'$ sur $v_0^{\perp}$ d'un $v \in C$ vérifie $\langle v', u_y \rangle = \langle v, u_y \rangle > 0$ sur $A \cap U^{0}(v_0)$ : ces directions, deux à deux distinctes, sont dans un demi-plan ouvert du plan orienté par $v_0$, et si $l$ est la première dans le sens direct, $A \cap U^{0}(v_0) \subseteq W(v_0, l)$. Si les $u_x$, $x \in A$, engendrent un plan de normale $\nu$, alors $v + \lambda \nu \in C$ pour tout $\lambda$, ce qui force $\langle \nu, u_y \rangle = 0$ pour $y \notin A$ : $U$ est dans ce plan, et l'argument précédent s'applique avec $v_0 = u_i \times u_j$ pour deux sites $i, j \in A$ non colinéaires avec $c$, $U^{+}(v_0) = \emptyset$ et $U^{0}(v_0) = U$. Si enfin $A$ est un singleton $\lbrace x \rbrace$ maximal, tout autre site de $U$ est antipodal à $x$, donc $m = 2$. $\square$

LOC-4 et LOC-5 donnent un quotient local en temps polynomial en $m$ ; l'énumération brute des parties de cardinal $t$ reste la définition, et l'oracle de `reference/` s'y tient.

### 5.4 Fenêtre de rang, coquilles régulières et étendues

**LOC-W (fenêtre de rang).** [démontré] Si $p < k \leq p + q - 2$, alors $\mu_k(b) = 1$ : $V_k^{<}(b)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(a)$.

*Preuve.* Ici $1 \leq t \leq q - 2$. Toute partie de $U$ de cardinal $\leq q - 1$ est séparable ; deux parties de cardinal $t$ qui diffèrent d'un site ont une réunion de cardinal $t + 1 \leq q - 1$, séparable, et l'on passe d'une partie de cardinal $t$ à une autre par de tels échanges : un seul morceau. LOC-3 (3) conclut. $\square$

**LOC-R (coquille régulière).** [démontré] Si $m = q$ : à l'ordre $k = p + m$, $\mu = 0$ ; à l'ordre $k = p + m - 1$, il y a exactement $m$ morceaux, de représentants $I \cup U \setminus \lbrace u \rbrace$, $u \in U$ ; aux ordres $p < k \leq p + m - 2$, un seul morceau.

*Preuve.* $U$ est la seule partie non séparable de $U$. Pour $t = m - 1$, les $m$ parties $U \setminus \lbrace u \rbrace$ sont séparables et la réunion de deux d'entre elles est $U$. Le reste est LOC-W. $\square$

Pour une coquille **étendue**, les ordres $p + q - 1 \leq k \leq p + m$ se décident par le quotient (LOC-3 ou LOC-4) ; tout peut s'y produire. Fixtures : `triangle_rectangle`, sites $(0,0,0)$, $(3,0,0)$, $(0,4,0)$ : la boule de l'hypoténuse (niveau $25/4$, $p = 0$, $q = 2$, $m = 3$) a un morceau à l'ordre 1 (aucun événement), deux à l'ordre 2 (jonction), aucun à l'ordre 3 (naissance). `carre`, sites $(0,0,0)$, $(2,0,0)$, $(0,2,0)$, $(2,2,0)$ : la boule de niveau 2 ($p = 0$, $q = 2$, $m = 4$) a un morceau à l'ordre 1, quatre à l'ordre 2, aucun aux ordres 3 et 4 (deux naissances, celle de l'ordre 3 ayant une population de quatre sites).

**LOC-GEO (lecture géométrique des morceaux).** [démontré ; n'est utilisé par aucune autre preuve] Pour $p < k \leq p + m$, il existe $\delta > 0$ tel que, en posant $C_A = \bigcap_{x \in A} B^{\circ}(x,a)$, on ait $L_k^{<}(a) \cap B^{\circ}(c,\delta) = \bigcup_{\lvert A \rvert = t} C_A \cap B^{\circ}(c,\delta)$, la réunion portant sur les parties de $U$ de cardinal $t$. $C_A$ est non vide si et seulement si $A$ est séparable, et alors $c$ lui est adhérent ; $C_A \cap C_{A'} = C_{A \cup A'}$. Les morceaux sont donc les composantes connexes de $L_k^{<}(a)$ au voisinage de $c$, et $\mu_k(b) = 0$ si et seulement si $c$ est un minimum local strict de $D_k$.

*Preuve.* Pour $y$ assez proche de $c$, les sites de $I$ restent à distance carrée $< a$ et les sites hors de la boule fermée à distance carrée $> a$ : $D_k(y) < a$ équivaut à l'existence de $t$ sites de $U$ à distance carrée $< a$ de $y$. Pour $x \in U$, $\lVert y - x \rVert^{2} < a$ équivaut à $2 \langle x - c, y - c \rangle > \lVert y - c \rVert^{2}$ : si $y \in C_A$, $v = y - c$ sépare $A$ ; si $v$ sépare $A$, $c + \varepsilon v \in C_A$ pour $\varepsilon > 0$ petit. Les $C_A \cap B^{\circ}(c,\delta)$ non vides sont des convexes ouverts et NERF-1 s'applique. Si aucun $A$ n'est séparable, $D_k(y) \leq a$ avec $y \neq c$ proche de $c$ fournirait $t$ sites de $U$ avec $\langle x - c, y - c \rangle > 0$, donc une partie séparable. $\square$
