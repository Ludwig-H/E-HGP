
## 4. Reformulation propre : définitions, théorèmes, preuves, statuts

Cette section est écrite pour être portée telle quelle dans la note mathématique de la v11. Tout y est énoncé pour des **sites distincts** dans $\mathbb{R}^{d}$ ; rien ne dépend de $d$ sauf la borne $d + 1$ sur le cardinal d'un support ($4$ en dimension $3$). **Aucune hypothèse de position générale.**

### 4.1 Définitions

- $X \subset \mathbb{R}^{d}$ fini, $n = \lvert X \rvert$. Pour $F \subseteq X$ non vide, $B(F)$ est sa plus petite boule fermée englobante (unique), $c_F$ son centre, $\beta(F)$ son rayon carré.
- $D_k(y)$ est la $k$-ième plus petite distance carrée de $y$ aux sites ; $L_k(a) = \lbrace y : D_k(y) \leq a \rbrace$ et $L_k^{<}(a) = \lbrace y : D_k(y) < a \rbrace$.
- $\Gamma_k(a)$ : les sommets sont les $k$-parties $F$ avec $\beta(F) \leq a$ ; chaque $(k+1)$-partie $G$ avec $\beta(G) \leq a$ relie toutes ses faces $G \setminus \lbrace g \rbrace$. $\Gamma_k^{<}(a)$ : même définition avec des inégalités strictes.
- **Sphère critique** $b$ : centre $c$, niveau $\lambda > 0$, $I$ = sites strictement intérieurs, $U$ = sites sur la sphère, avec $c \in \mathrm{conv}(U)$. On note $p = \lvert I \rvert$, $m = \lvert U \rvert$, $P_b = I \cup U$, et $q$ le plus petit cardinal d'un **support**, c'est-à-dire d'une partie affinement indépendante $S \subseteq U$ avec $c \in \mathrm{relint}\,\mathrm{conv}(S)$. La coquille est **régulière** si $m = q$ (alors $U$ est l'unique support), **étendue** sinon.
- Une partie $A \subseteq U$ est **séparable** si $c \notin \mathrm{conv}(A)$, enveloppe fermée ; par le théorème de Gordan, c'est équivalent à l'existence de $v$ avec $\langle v, x - c \rangle > 0$ pour tout $x \in A$. Toute partie d'une partie séparable est séparable.
- **Arbre de fusion** $T_k$ : pour chaque niveau critique $\lambda$ et chaque composante $C$ de $\Gamma_k(\lambda)$, soit $o(C)$ le nombre de composantes de $\Gamma_k^{<}(\lambda)$ contenues dans $C$. Si $o(C) = 0$, $C$ est une **naissance** (feuille de niveau $\lambda$) ; si $o(C) \geq 2$, $C$ est une **multifusion** (nœud de niveau $\lambda$ dont les enfants sont les nœuds courants de ces $o(C)$ composantes) ; si $o(C) = 1$, c'est une **continuation** : aucun nœud.
- **Application verticale** : pour $k \geq 2$, $\phi_k^{a}$ envoie la composante de $F$ dans $\Gamma_k(a)$ sur celle de $F \setminus \lbrace x \rbrace$ dans $\Gamma_{k-1}(a)$.

### 4.2 Théorème A (nerf) — `theorem_external`, complété

**Énoncé.** Pour tout $k \leq n$ et tout $a \geq 0$, $F \mapsto W_F(a) = \bigcap_{x \in F} \bar{B}(x, \sqrt{a})$ induit une bijection entre les composantes de $\Gamma_k(a)$ et celles de $L_k(a)$, naturelle en $a$ et compatible avec les inclusions $L_k(a) \subseteq L_{k-1}(a)$ ; de même entre $\Gamma_k^{<}(a)$ et $L_k^{<}(a)$.

**Preuve.** C'est le Théorème 2 du manuscrit (p. 60, numérotation imprimée) composé avec sa Proposition 5 (p. 86, adjacences élémentaires) : $L_k(a)$ est l'union des convexes compacts $W_F(a)$, non vides si et seulement si $\beta(F) \leq a$ ; $W_F(a) \cap W_{F'}(a) \neq \emptyset$ si et seulement si $\beta(F \cup F') \leq a$, et alors les $k$-parties de $F \cup F'$ sont reliées de proche en proche par ses $(k+1)$-parties. La version ouverte s'obtient avec les boules ouvertes. La compatibilité verticale vient de $W_F(a) \subseteq W_{F \setminus \lbrace x \rbrace}(a)$ ; $\phi_k^{a}$ est bien définie parce que les $(k-1)$-parties de $F$ sont reliées par $F$ elle-même, et que deux faces d'une même $(k+1)$-partie partagent une $(k-1)$-partie. $\square$

Aucune position générale n'est utilisée. Statut : la preuve de la thèse est complète ; la version ouverte et la naturalité verticale, absentes de la thèse, sont immédiates.

### 4.3 Lemmes

**Lemme 1 (boule minimale dans une boule critique).** Soit $b$ une sphère critique et $F \subseteq P_b$ non vide. Alors $B(F) = b$ si et seulement si $F \cap U$ n'est pas séparable ; sinon $\beta(F) < \lambda$.

*Preuve.* Si $c \in \mathrm{conv}(F \cap U)$ et $c' \neq c$, il existe $x \in F \cap U$ avec $\langle x - c, c' - c \rangle \leq 0$, donc $\lVert x - c' \rVert^{2} \geq \lambda + \lVert c - c' \rVert^{2} > \lambda$ : aucune boule de rayon carré $\leq \lambda$ autre que $b$ ne contient $F$. Sinon, Gordan donne $v$ avec $\langle v, x - c \rangle > 0$ sur $F \cap U$ ; pour $\varepsilon > 0$ assez petit, le centre $c + \varepsilon v$ est strictement plus proche de chaque point de $F \cap U$ et les points de $F \cap I$ gardent leur marge : $\beta(F) < \lambda$. $\square$

**Lemme 2 (Johnson).** Si $Q \subseteq X$ et $\lvert Q \rvert \geq k + 1$, les $k$-parties de $Q$ sont dans une même composante de $\Gamma_k(\beta(Q))$. Si de plus toute $(k+1)$-partie de $Q$ vérifie $\beta < a$, elles sont dans une même composante de $\Gamma_k^{<}(a)$.

*Preuve.* Deux $k$-parties de $Q$ se rejoignent en échangeant un élément à la fois ; chaque échange passe par une $(k+1)$-partie de $Q$. $\square$

**Lemme 3 (taille d'une partie non séparable).** Toute partie non séparable de $U$ contient un support ; elle a donc au moins $q$ éléments.

*Preuve.* Une partie minimale de $A$ dont l'enveloppe contient $c$ est affinement indépendante et contient $c$ dans son intérieur relatif (Carathéodory). $\square$

**Lemme 4 (structure stricte locale).** Soit $b$ une sphère critique, $k \leq p + m$, $t = k - p$, et $V_{<}(b, k)$ l'ensemble des $k$-parties $F \subseteq P_b$ dont la trace $F \cap U$ est séparable (par le lemme 1, ce sont les $k$-parties de $P_b$ de niveau $< \lambda$).

- (a) Si $p \geq k$ : $V_{<}(b, k)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(\lambda)$.
- (b) Si $p < k$ : toute $F \in V_{<}(b, k)$ est reliée dans $\Gamma_k^{<}(\lambda)$ à une partie $I \cup A$ avec $A \subseteq F \cap U$, $\lvert A \rvert = t$.
- (c) Si $p < k$ et si $A$, $A'$ sont deux parties de $U$ de taille $t$ telles que $A \cup A'$ soit séparable : $I \cup A$ et $I \cup A'$ sont dans la même composante de $\Gamma_k^{<}(\lambda)$.
- (d) Si $p < k \leq p + q - 2$ : $V_{<}(b, k)$ est non vide et contenu dans une seule composante de $\Gamma_k^{<}(\lambda)$.

*Preuve.* (a) Soit $N \subseteq I$ de cardinal $k$. Pour $F \in V_{<}$, tant que $F \neq N$ : ajouter $x \in N \setminus F$ (la $(k+1)$-partie a la même trace sur $U$, donc un niveau $< \lambda$), puis retirer $y \in F \setminus N$ (la trace ne peut que diminuer). (b) Même échange : ajouter $x \in I \setminus F$, retirer un point de $F \cap U$, qui en compte au moins $t + 1$ tant que $F \not\supseteq I$. (c) Toutes les $k$-parties et $(k+1)$-parties de $I \cup A \cup A'$ ont une trace séparable ; lemme 2. (d) Par le lemme 3, toute partie de $U$ de taille $\leq q - 1$ est séparable ; les parties de taille $t$ et $t + 1 \leq q - 1$ le sont, et deux parties de taille $t$ se rejoignent par échanges ; conclure par (b) et (c). $\square$

### 4.4 Théorème B (classification des événements d'un niveau) — prouvé ici

**Énoncé.** Fixons $k$ et un niveau $\lambda > 0$. Les sommets et arêtes de $\Gamma_k(\lambda)$ absents de $\Gamma_k^{<}(\lambda)$ se répartissent selon leur boule minimale, qui est une sphère critique de niveau $\lambda$ ; ceux de la sphère $b$ sont contenus dans $P_b$. L'effet de $b$ sur les composantes est le suivant.

1. $p + m < k$ : aucun.
2. $p + m = k$ : **naissance** d'une composante réduite au sommet $P_b$, isolée dans $\Gamma_k(\lambda)$.
3. $p + m \geq k + 1$ et $V_{<}(b, k) = \emptyset$ : **naissance** d'une composante formée de toutes les $k$-parties de $P_b$. Ce cas se produit si et seulement si $p < k$ et aucune partie de $U$ de taille $t$ n'est séparable (ce qui force $t \geq q$).
4. $p + m \geq k + 1$ et $V_{<}(b, k) \neq \emptyset$ : toutes les composantes de $\Gamma_k^{<}(\lambda)$ qui rencontrent $V_{<}(b, k)$ sont réunies en une seule, à laquelle s'attachent les sommets nouveaux de $b$. De plus : si $p \geq k$ ou si $k \leq p + q - 2$, une seule composante est rencontrée (boule **inerte**) ; si $p + q - 1 \leq k \leq p + m - 1$, les composantes rencontrées sont exactement celles des représentants $I \cup A_j$, un par **morceau**, les morceaux étant les classes des parties séparables de taille $t$ pour la clôture transitive de « $A \cup A'$ est séparable ».

Deux sphères distinctes de même niveau n'ont ni sommet nouveau ni arête nouvelle en commun. Au niveau $0$, seul l'ordre $1$ a des sommets (les sites, tous naissances).

**Preuve.** Un simplexe $\sigma$ de niveau $\lambda$ a $B(\sigma) = b$ pour une sphère critique $b$ (son centre est dans l'enveloppe de ses points de bord), et $\sigma \subseteq P_b$ ; l'unicité de la boule minimale donne la répartition et la disjonction entre sphères. Les faces d'une arête nouvelle de $b$ sont des $k$-parties de $P_b$ : $b$ ne relie donc que des $k$-parties de $P_b$, et par le lemme 2 il les relie toutes dès que $p + m \geq k + 1$. Cas 2 : $P_b$ est de niveau $\lambda$ par le lemme 1 ; une $(k+1)$-partie de niveau $\leq \lambda$ qui la contient aurait $b$ pour boule minimale, donc serait dans $P_b$, de cardinal $k$ : impossible. Cas 3 : si toutes les $k$-parties de $P_b$ sont nouvelles, elles forment une composante que rien d'autre ne touche au niveau $\lambda$ (même argument) ; $V_{<} = \emptyset$ exclut $p \geq k$ (une partie de $I$ a une trace vide, séparable) et équivaut, en prenant $F \supseteq I$, à l'absence de partie séparable de taille $t$. Cas 4 : première phrase par le lemme 2 ; le reste par le lemme 4. $\square$

**Corollaires.**

- *Fenêtre de rang.* Une sphère n'est un événement de l'ordre $k$ que si $p + q - 1 \leq k \leq p + m$. C'est exactement la fenêtre du code (`tower.cpp:1231-1232`) ; l'admission $p + q \leq K + 1$ du catalogue est donc nécessaire et suffisante pour les ordres $\leq K$.
- *Coquille régulière.* Les seules parties non séparables de $U$ sont $U$ : naissance à $k = p + m$, jonction à $m$ morceaux $U \setminus \lbrace u \rbrace$ à $k = p + m - 1$, inertie ailleurs. C'est la règle analytique du code (l. 567-580, 1374).
- *Isolement d'une naissance.* Une naissance de niveau $\lambda$ n'est jamais enfant d'une fusion de niveau $\lambda$. C'est l'invariant de plateau (D) du § 7.
- *Le critère par paires du code est exact.* « $A \cup A'$ séparable » et « $A$, $A'$ faces d'une même partie séparable de taille $t + 1$ » ont la même clôture transitive : une union séparable contient un chemin d'échanges. Mieux, tout découpage **plus fin** que les morceaux reste correct : le théorème C ne demande à chaque représentant que d'appartenir à $V_{<}(b, k)$.

### 4.5 Théorème C (forêt à plateaux) — prouvé ici

**Énoncé.** Au niveau $\lambda$, soit $H$ le graphe biparti dont les sommets sont les composantes de $\Gamma_k^{<}(\lambda)$ et les sphères de niveau $\lambda$ du cas 4, une sphère étant reliée aux composantes qu'elle rencontre. Les multifusions de $T_k$ au niveau $\lambda$ sont exactement les composantes connexes de $H$ qui contiennent au moins deux composantes de $\Gamma_k^{<}(\lambda)$, avec ces composantes pour enfants ; les naissances sont les sphères des cas 2 et 3.

**Preuve.** Conséquence directe du théorème B : les sphères d'un même niveau n'interagissent que par les composantes anciennes qu'elles partagent. $\square$

C'est la sémantique « racines pré-lot lues avant toute union, un nœud par groupe » de `kruskal` (l. 1071-1111). Traiter les jonctions d'un même niveau une à une produit une chaîne de fusions de même niveau : faux, et le mutant `m_seq` le montre.

### 4.6 Théorème D (descente) — prouvé ici

**Énoncé.** Soit $F$ une $k$-partie, $k \geq 2$, et $b = B(F)$. Définissons un **pas valide** $F \to F'$ :

- si $p \geq k$ : $F'$ est une $k$-partie quelconque de $I$ (le code prend les $k$ plus proches du centre) ;
- sinon, si $V_{<}(b, k) = \emptyset$ : arrêt, $F$ est un sommet de la naissance $(b, k)$ ;
- sinon : $F' = I \cup A$ pour une partie séparable $A \subseteq U$ de taille $t$ quelconque.

Alors $\beta(F') < \beta(F)$, et $F$, $F'$ sont dans la même composante de $\Gamma_k(\beta(F))$. Toute suite de pas valides termine donc sur une naissance $N(F)$ de niveau $\leq \beta(F)$, et $F$ appartient à la composante de $N(F)$ dans $\Gamma_k(a)$ pour tout $a \geq \beta(F)$. Si $\beta(F) < \lambda$, c'est déjà vrai dans $\Gamma_k^{<}(\lambda)$.

**Preuve.** Décroissance : une partie de $I$ tient dans une boule concentrique strictement plus petite ; pour $I \cup A$, lemme 1. Liaison : $F \neq F'$ sont deux $k$-parties de $P_b$, donc $\lvert P_b \rvert \geq k + 1$ et le lemme 2 s'applique au niveau $\beta(P_b) = \lambda_b = \beta(F)$. Terminaison : les niveaux sont en nombre fini. $\square$

**Corollaire (indépendance des choix).** La composante de $N(F)$ à tout niveau $\geq \beta(F)$ ne dépend ni de la règle du saut, ni du représentant choisi, ni d'un éventuel raccourci (semis, mémo). C'est ce qui rend la descente « pure » et le mémo par cellule sûr (`tower.cpp:1448-1450`). Vérifié à l'échelle au § 7 (variantes `v_noseed` et `v_alt`).

Le nombre de pas n'a **aucune borne prouvée** autre que le nombre de niveaux ; mesuré ici : 3,6 millions de pas pour 3,0 millions de résolutions sur la trame 01 à $K = 5$.

### 4.7 Théorème E (exactitude de la construction, relativement au catalogue) — prouvé ici

**Hypothèses.** (H1) sites distincts ; (H2) le catalogue contient toute sphère critique avec $p + q \leq K + 1$, avec $I$ et $U$ exacts ; (H3) les niveaux sont comparés exactement ; (H4) les représentants d'une cellule en fenêtre sont des éléments de $V_{<}(b, k)$, au moins un par composante de $\Gamma_k^{<}(\lambda_b)$ rencontrée par $V_{<}(b, k)$ (c'est le cas des morceaux, théorème B).

**Énoncé.** Pour $k \leq K$, la forêt obtenue en (i) créant une feuille par cellule de naissance et, à l'ordre $1$, par site, (ii) envoyant chaque représentant sur la naissance où aboutit une descente valide, (iii) appliquant la règle des plateaux du théorème C aux hyperarêtes ainsi formées, est l'arbre de fusion $T_k$.

**Preuve.** Récurrence sur les niveaux critiques. Invariant : après le niveau $a$, deux naissances sont dans la même classe si et seulement si elles sont dans la même composante de $\Gamma_k(a)$, et toute composante contient une naissance (théorème D appliqué à l'un de ses sommets). Au niveau $\lambda$ : par le théorème B, les composantes anciennes réunies par $b$ sont celles de ses représentants ; par le théorème D, chacune contient la naissance où aboutit la descente de ce représentant, dans $\Gamma_k^{<}(\lambda)$, donc dans la classe pré-lot correspondante par l'invariant. Le théorème C conclut. $\square$

**Portée exacte.** La tour est exacte **relativement** à (H2). Rien dans le produit ne témoigne de (H2) : voir le § 7.5 et le constat 08.

### 4.8 Théorème F (verticales) — prouvé ici

**Énoncé.** (i) Si $v$ est une naissance de l'ordre $k \geq 2$ portée par $b$, son image par $\phi_k^{\lambda_b}$ est la composante de $\Gamma_{k-1}(\lambda_b)$ qui contient **toutes** les $(k-1)$-parties de $P_b$. (ii) Si $v$ est une fusion de niveau $a$, son image est l'ancêtre vivant à la coupe fermée $a$ de l'image de n'importe lequel de ses enfants. (iii) Pour un nœud $v$ vivant à la coupe $a$, $\phi_k^{a}(v)$ est l'ancêtre vivant à la coupe fermée $a$ de l'image de $v$ à son niveau de création.

**Preuve.** (i) $\lvert P_b \rvert \geq k = (k - 1) + 1$ : lemme 2 à l'ordre $k - 1$. (ii) et (iii) : fonctorialité du théorème A. $\square$

Le code calcule (i) par descente d'une $(k-1)$-partie de $P_b$ puis remontée au rang de $b$ (l. 1689-1702), avec le raccourci régulier « dernier représentant de la jonction de la même boule à l'ordre $k - 1$ » (l. 1678-1687), et vérifie (ii) pour tous les enfants. Jugé ici pour tous les nœuds (§ 6.2, contrôle V).

### 4.9 Proposition G (couvertures et continuations) — prouvé ici, mesuré

La **couverture** d'une composante est l'union de ses sommets ; c'est le K-polyèdre de la thèse.

- Si toutes les sphères en fenêtre sont régulières, la couverture d'un nœud est l'union des populations $P_b$ des naissances de son sous-arbre : aucune continuation ni aucune fusion n'apporte de point nouveau. *Preuve.* Aux boules inertes, tout point de $P_b$ appartient déjà à une $k$-partie de trace séparable de la composante (lemme 4) ; à une jonction régulière, chaque point de $P_b$ est dans un représentant.
- Sinon c'est faux : une sphère à coquille étendue peut ajouter un point à une composante sans fusion. **Fixture** : les quatre points $(8, 9, 0)$, $(5, 10, 0)$, $(2, 9, 0)$, $(5, 0, 0)$ sur le cercle de centre $(5, 5, 0)$ et de rayon $5$. À l'ordre $3$, la seule partie séparable de taille $3$ est formée des trois premiers ; au niveau $25$ la composante gagne le quatrième point, sans fusion ni naissance. Le juge compte ce cas (« gain de couverture sans fusion ») : 1 sur cette fixture, @@GROWTH@@ dans les campagnes ; la forêt C++ est conforme, mais l'événement n'y laisse aucune trace.

Conséquence : pour publier les K-polyèdres exacts hors régularité, la tour doit enregistrer une contribution datée par cellule à morceaux, ou la relation « boule couvrante $\to$ nœud » (que le code sait produire en entrée `cover`, `ball_node`).

### 4.10 Théorème H (Euler) — prouvé (TOWER_v2 § 9.3, vérifié ici), mesuré

**Énoncé.** Pour $1 \leq K \leq n$, avec $[K = 1]$ valant $1$ si $K = 1$ et $0$ sinon : $$n \cdot [K = 1] + \sum_{b} e_K(b) = 1, \qquad \sum_{K \geq 1} e_K(b)\, y^{K-1} = y^{p} \sum_{T \subseteq U,\ c \in \mathrm{conv}(T)} (y - 1)^{\lvert T \rvert - 1}.$$

La somme porte sur toutes les sphères critiques. Pour une coquille régulière, $e_K(b) = (-1)^{m - 1 - j} \binom{m - 1}{j}$ avec $j = K - 1 - p$, nul hors de $0 \leq j \leq m - 1$.

**Preuve.** Pour une famille finie de boules fermées, $\mathbf{1}[n(y) \geq K] = \sum_{\lvert A \rvert \geq K} (-1)^{\lvert A \rvert - K} \binom{\lvert A \rvert - 1}{K - 1} \mathbf{1}[y \in \bigcap A]$ (identité binomiale en $n(y)$, le nombre de boules contenant $y$) ; la caractéristique d'Euler est additive sur les unions finies de convexes compacts et vaut $1$ sur une intersection non vide, donc $\chi(L_K(a)) = \sum_{\lvert A \rvert \geq K,\ \beta(A) \leq a} (-1)^{\lvert A \rvert - K} \binom{\lvert A \rvert - 1}{K - 1}$. On regroupe les parties $A$ par leur boule minimale (lemme 1 : $A = I' \cup T$ avec $I' \subseteq I$ et $c \in \mathrm{conv}(T)$), d'où la série génératrice en $y$ après la substitution $x^{j} \mapsto (y - 1)^{j - 1}$. Enfin, pour $a \geq \beta(X)$, tous les $W_F(a)$ contiennent $c_X$ : $L_K(a)$ est étoilé et $\chi = 1$. $\square$

**Portée.** Une sphère contribue aux ordres $p + 1 \leq K \leq p + m$ ; l'identité à l'ordre $K$ exige donc **toutes les sphères avec $p \leq K - 1$**, quel que soit $q$. Le catalogue du produit ($p + q \leq K + 1$) ne suffit pas ; un catalogue à $K + 2$ suffit (il contient ces sphères puisque $q \leq 4$), et l'admission $p \leq K - 1$ suffirait à moindre coût (constat 05). Une sphère régulière manquante change la somme d'un terme $\pm 1$ dès l'ordre $p + 1$ (pour une coquille étendue, au premier ordre où son coefficient est non nul) : l'identité est un témoin de complétude sensible, mais pas une preuve (des erreurs peuvent se compenser).

### 4.11 Contre-exemple I : le Théorème 5 du manuscrit est faux en général — exécuté

**Énoncé de la thèse** (Déf. 29 et 30, Prop. 6, Th. 5, p. 89 à 91, numérotation imprimée) : en position générale (Déf. 26), les ensembles de points des composantes non triviales du K-graphe de Gabriel élagué à $r$ sont les K-polyèdres non triviaux de Čech.

**Contre-exemple exact** ($K = 2$, cinq points du plan, en position générale au sens de la Déf. 26, vérifié en `Fraction`) : $A = (-100, 0)$, $C = (100, 0)$, $z = (1, -90)$, $y = (30, -85)$, $w = (3, 300)$. La paire $AC$ a deux intrus $z$ et $y$ dans sa boule diamétrale : les triangles $ACz$ et $ACy$ ne sont pas de Gabriel, et $AC$ rejoint silencieusement la composante de $Az$, $Cz$ au niveau $r^{2} = 10000$. Les seuls triangles de Gabriel sont $Azy$, $Czy$ et $ACw$. Le graphe de Gabriel a donc deux composantes à jamais, de points $\lbrace A, C, z, y \rbrace$ et $\lbrace A, C, w \rbrace$, alors que $\Gamma_2$ n'en a plus qu'une, $\lbrace A, C, z, y, w \rbrace$, dès $r^{2} = 10001440081/360000$. Le « K-arbre couvrant minimal » n'existe même pas comme arbre. La fixture E5 du dépôt donne un désaccord de même nature sur un intervalle de niveaux ($r^{2} = 83886/3563$ : Čech $\lbrace A, B, C, D, E \rbrace$, Gabriel $\lbrace A, B, C \rbrace$ et $\lbrace A, C, D, E \rbrace$). Sur des nuages aléatoires en position générale, 0 à 4 nuages sur 300 sont en désaccord à au moins un niveau selon la configuration (cinq configurations, $n$ de 5 à 7, $K$ de 2 à 3 ; 11 désaccords sur 1 500 nuages).

**Où la preuve de la Prop. 6 échoue.** L'invariant de récurrence porte sur les ensembles de points ; il ne retient pas qu'une facette née dans une coface non-Gabriel appartient désormais à une composante. Quand cette facette devient plus tard la facette active d'un simplexe de Gabriel, le graphe de Gabriel la traite comme un sommet neuf.

**Ce que la v10 fait à la place.** Sur ce nuage, le représentant $\lbrace A, C \rbrace$ de la jonction $ACw$ descend : sa boule minimale contient deux sites strictement intérieurs, saut vers $\lbrace z, y \rbrace$, qui est une naissance. La fusion est alors correcte ; le juge de cet audit le confirme (fixture `contre_exemple_th5_plan`, conforme aux cinq ordres). Le registre racine connaît le phénomène (« le flot des seules cofaces Gabriel […] reconstruit FULL » : `false_in_general`, E5) ; il n'y est pas dit que l'énoncé de la thèse lui-même tombe, ni donné de contre-exemple permanent.

### 4.12 Théorème J (règle cartésienne de TOWER_v2, non implémentée) — prouvé dans TOWER_v2 § 9.2, recoupé ici

**Énoncé (PO-T18).** Si l'on traite les hyperarêtes par rang croissant, dans un ordre quelconque à rang égal, par union-find et concaténation de listes, et si $J[i]$ est le rang de l'arête qui a collé les positions $i$ et $i + 1$ de l'ordre final, alors les multifusions sont exactement les intervalles maximaux de jonctions $\leq t$ dont le maximum vaut $t$, et leurs enfants sont les morceaux séparés par les jonctions égales à $t$.

La preuve écrite dans `TOWER_v2.md:836-870` est correcte (elle n'utilise que la contiguïté des composantes dans l'ordre de concaténation). Recoupement indépendant : 2 000 hypergraphes aléatoires à poids très répétés, 5 003 nœuds dont 3 839 à trois enfants ou plus, 0 écart avec le balayage par lots atomiques (`l02_cartesian_rule.py`). La règle est donc disponible pour la v11 si le noyau séquentiel par lots devient le plafond.

### 4.13 Tableau des statuts et obligations restantes

| Id | Énoncé | Statut après cet audit | Trace écrite en v10 | Contrôle |
| --- | --- | --- | --- | --- |
| A | nerf, coupes fermée et ouverte, naturalité verticale | `theorem_external` (thèse Th. 2 + Prop. 5) complété | thèse ; `SPEC_V10.md:25-31` | — |
| B | classification des événements, fenêtre, quotient local | prouvé ici, à contre-lire | énoncé seul (`SPEC_V10.md:49-56`, `tower.hpp:3-9`) ; esquisses hors dépôt (`build/v10-persist/design/TOWER_v1.md:632-648`) ; antécédents v7, dans un autre vocabulaire et pour un autre résolveur : bloc inerte (`docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md`, th. 4.2), fenêtre et ancres (`morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md`, hors de l'arbre de lecture) | juge L02, toutes familles |
| C | plateaux atomiques | prouvé ici | énoncé seul | juge L02 ; mutants `m_bin`, `m_seq` |
| D | descente, indépendance des choix | prouvé ici | énoncé seul ; `TOWER_v2.md:809-834` prouve un autre résolveur (saut depuis le centre dans tous les cas) | juge L02 ; variantes neutres à l'échelle |
| E | exactitude relative au catalogue | prouvé ici | absent | juge L02 |
| F | verticales à la coupe fermée | prouvé ici | énoncé en commentaire (`tower.cpp:1658-1662`) | juge L02 (tous les nœuds) ; mutants `m_vopen`, `m_vnoclimb` |
| G | couvertures : union des feuilles en régulier seulement | prouvé ici ; fixture du gain de couverture | absent | juge L02 (compteur) |
| H | Euler, sites distincts | prouvé (`TOWER_v2.md:872-893`), vérifié ici | conception seulement ; **aucun code** | petits nuages et échelle (§ 7) |
| I | Th. 5 de la thèse | `false_in_general`, contre-exemple gravé ici | registre racine : formulation voisine, sans contre-exemple à l'énoncé | `l02_these_th5.py` |
| J | règle cartésienne | prouvé (conception), recoupé ici | conception seulement ; non implémenté | `l02_cartesian_rule.py` |
| — | première boule couvrante dans le catalogue (entrée `cover`) | prouvé ici en passant : si $F \ni x$ minimise $\beta$, sa boule vérifie $p + q \leq K + 1$ | commentaire (`tower.cpp:1531-1534`) | lentille L03 |

**Obligations restantes pour la v11** (rien de ce qui suit n'est prouvé ni jugé en v10) :

1. **Complétude du catalogue** (H2) : théorème du générateur, hors de cette lentille ; seul témoin indépendant disponible à l'échelle : Euler.
2. **Multiplicités.** L'extension aux multiensembles (fenêtre en poids, Euler pondéré, quotient pondéré) n'existe que dans `TOWER_v2` § 3.9, 9.3 à 9.5 ; le code refuse. À décider avant tout : une trame avec doublons est aujourd'hui refusée.
3. **Coquilles étendues.** L'énumération est exponentielle en $m$ et bornée par des budgets ; aucune borne de $m$ n'est prouvée pour les entrées du contrat (mesuré : $m \leq 5$ sur les trois trames). Les quotients circulaire et pondéré de la conception ne sont ni codés ni jugés.
4. **Longueur des descentes** : aucune borne.
5. **Arithmétique** : la garde de décroissance et les sélections en double reposent sur une marge (`kApproxMargin`, `tower.cpp:27`) justifiée par un commentaire ; hors de cette lentille.
6. **Inscription au registre** : aucune de ces lignes n'y figure.
