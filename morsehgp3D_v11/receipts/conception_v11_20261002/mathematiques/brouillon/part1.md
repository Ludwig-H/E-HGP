# Mathématiques de la v11

Document normatif des énoncés mathématiques de `morsehgp3D_v11/`. Le code cite un énoncé par son identifiant (`CAT-1`, `GEN-D`, `TOUR-B`, `INV-EULER`…) : un élagage, une étape de la tour ou un invariant de porte sans identifiant ici n'a pas de justification. L'architecture (`ARCHITECTURE.md`) fixe le profil numérique et les contrats ; ce document ne les répète pas.

```text
phase=exploration_v11_hors_registre
mode=mathematiques
public_status=not_claimed
```

**Conventions de lecture.**

- Aucune hypothèse de position générale, sauf mention explicite : égalités de niveaux, sites alignés, coplanaires, cocycliques ou cosphériques sont dans le champ de chaque énoncé.
- Les §§ 1 à 9 supposent des sites distincts, chacun de poids 1. Les positions répétées sont traitées au § 10 ; tant que ce paragraphe n'est pas clos, la tour refuse une entrée pondérée.
- Chaque énoncé porte un statut : **[démontré]** (preuve complète écrite ici), **[externe]** (résultat classique cité sans preuve), **[à prouver]**, **[faux]** (contre-exemple exact donné), **[mesuré]**. Le § 11 les traduit dans l'échelle du registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`.
- Chaque inégalité dit de quel côté tombe l'égalité. « Strict » veut dire strict ; « fermé » inclut l'égalité.
- Les preuves ont été écrites le 2 octobre 2026 à partir des lectures L01 à L03 de l'audit de la v10, refaites une à une, puis confrontées à la définition par force brute exacte (§ 11.3). Elles n'ont pas encore été contre-lues par un tiers (obligation OBL-1).

## 1. Cadre et notations

### 1.1 Sites, distances, niveaux

- $X \subset \mathbb{Z}^{3}$ est un ensemble fini non vide de **sites** (positions distinctes), $n = \lvert X \rvert$. Tout est écrit en dimension 3 ; les §§ 4 à 8 (hors LOC-5) valent en toute dimension $d$, la borne 4 de CAT-1 devenant $d + 1$.
- Une **$k$-partie** est une partie de $X$ à $k$ éléments ; $\lVert \cdot \rVert$ est la norme euclidienne, $\langle \cdot , \cdot \rangle$ le produit scalaire.
- Un **niveau** est un carré de rayon : aucune racine carrée n'apparaît. Pour $c \in \mathbb{R}^{3}$ et $a \geq 0$, la boule fermée, la boule ouverte et la sphère de centre $c$ et de niveau $a$ sont $\bar{B}(c,a) = \lbrace y : \lVert y - c \rVert^{2} \leq a \rbrace$, $B^{\circ}(c,a) = \lbrace y : \lVert y - c \rVert^{2} < a \rbrace$ et $\partial B(c,a) = \lbrace y : \lVert y - c \rVert^{2} = a \rbrace$.
- Pour $y \in \mathbb{R}^{3}$ et $1 \leq k \leq n$, $D_k(y)$ est la $k$-ième plus petite valeur de $\lVert y - x \rVert^{2}$, $x \in X$. Donc $D_k(y) \leq a$ si et seulement si $\lvert X \cap \bar{B}(y,a) \rvert \geq k$, et $D_k(y) < a$ si et seulement si $\lvert X \cap B^{\circ}(y,a) \rvert \geq k$.
- Coupe fermée et coupe ouverte de l'ordre $k$ au niveau $a$ :

$$L_k(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) \leq a \rbrace, \qquad L_k^{<}(a) = \lbrace y \in \mathbb{R}^{3} : D_k(y) < a \rbrace .$$

- Monotonie : $L_k(a) \subseteq L_k(a')$ si $a \leq a'$ ; $L_k(a) \subseteq L_{k-1}(a)$ ; $L_k^{<}(a) \subseteq L_k(a) \subseteq L_k^{<}(a')$ si $a < a'$.

### 1.2 Plus petite boule englobante

**CAD-S (séparation).** [externe] Pour une partie finie $A \subset \mathbb{R}^{3}$ et un point $c$ : $c \notin \mathrm{conv}(A)$ si et seulement s'il existe $v$ avec $\langle v, x - c \rangle > 0$ pour tout $x \in A$. (Séparation stricte d'un point et d'un convexe compact ; pour $A = \emptyset$ les deux membres sont vrais. C'est la forme du théorème de Gordan utilisée partout.)

**CAD-1 (plus petite boule englobante).** [démontré] Soit $G \subset \mathbb{R}^{3}$ fini non vide. (i) Il existe une unique boule fermée de niveau minimal qui contient $G$ ; on la note $B(G)$, son centre $c_G$, son niveau $\beta(G)$. (ii) Une boule fermée $\bar{B}(c,a)$ qui contient $G$ est $B(G)$ si et seulement si $c \in \mathrm{conv}(G \cap \partial B(c,a))$. (iii) $G' \subseteq G$ entraîne $\beta(G') \leq \beta(G)$ ; $\beta(G) = 0$ si et seulement si $G$ est un singleton.

*Preuve.* (i) $y \mapsto \max_{x \in G} \lVert x - y \rVert^{2}$ est continue et tend vers l'infini à l'infini : elle atteint son minimum. Si deux centres $c \neq c'$ donnaient le même niveau minimal $a$, leur milieu $m$ vérifierait, pour tout $x \in G$, $\lVert x - m \rVert^{2} = \frac{1}{2} \lVert x - c \rVert^{2} + \frac{1}{2} \lVert x - c' \rVert^{2} - \frac{1}{4} \lVert c - c' \rVert^{2} < a$ : contradiction. (ii) Sens réciproque : si $c = \sum_i \lambda_i x_i$ avec $x_i \in G \cap \partial B(c,a)$, $\lambda_i > 0$, $\sum_i \lambda_i = 1$, alors pour tout $c' \neq c$ on a $\sum_i \lambda_i \langle x_i - c, c' - c \rangle = 0$, donc un $x_i$ vérifie $\langle x_i - c, c' - c \rangle \leq 0$ et $\lVert x_i - c' \rVert^{2} = a - 2 \langle x_i - c, c' - c \rangle + \lVert c - c' \rVert^{2} > a$. Aucune boule de niveau $\leq a$ centrée ailleurs qu'en $c$ ne contient $G$, et une boule centrée en $c$ qui contient les $x_i$ a un niveau $\geq a$. Sens direct : si $a = 0$, $G = \lbrace c \rbrace$. Si $a > 0$ et $c \notin \mathrm{conv}(G \cap \partial B(c,a))$, CAD-S donne $v$ avec $\langle v, x - c \rangle > 0$ sur $G \cap \partial B(c,a)$ ; pour $\varepsilon > 0$ assez petit, $c + \varepsilon v$ est à distance carrée $a - 2 \varepsilon \langle v, x - c \rangle + \varepsilon^{2} \lVert v \rVert^{2} < a$ de ces points, et les points de $G$ strictement intérieurs le restent par continuité : $\bar{B}(c,a)$ n'est pas de niveau minimal. (iii) La boule $B(G)$ contient $G'$. $\square$

### 1.3 Régions témoins

Pour $G \subseteq X$ non vide : $R_G(a) = \bigcap_{x \in G} \bar{B}(x,a)$ et $R_G^{<}(a) = \bigcap_{x \in G} B^{\circ}(x,a)$.

**CAD-2 (régions témoins).** [démontré] $R_G(a)$ est convexe et compact, non vide si et seulement si $\beta(G) \leq a$ (égalité : $R_G(\beta(G)) = \lbrace c_G \rbrace$). $R_G^{<}(a)$ est convexe et ouvert, non vide si et seulement si $\beta(G) < a$. De plus

$$L_k(a) = \bigcup_{\lvert F \rvert = k} R_F(a), \qquad L_k^{<}(a) = \bigcup_{\lvert F \rvert = k} R_F^{<}(a) .$$

*Preuve.* $y \in R_G(a)$ équivaut à $G \subseteq \bar{B}(y,a)$, possible si et seulement si $\beta(G) \leq a$ ; pour $a = \beta(G)$, l'unicité de CAD-1 donne $y = c_G$. $y \in R_G^{<}(a)$ équivaut à $\max_{x \in G} \lVert x - y \rVert^{2} < a$, possible si et seulement si le minimum $\beta(G)$ de ce maximum est $< a$. Enfin $D_k(y) \leq a$ équivaut à l'existence de $k$ sites dans $\bar{B}(y,a)$, c'est-à-dire d'une $k$-partie $F$ avec $y \in R_F(a)$ ; de même avec les inégalités strictes. $\square$

### 1.4 Niveaux rationnels exacts

**CAD-3 (niveaux).** [démontré] Pour $G \subseteq X$, $c_G \in \mathbb{Q}^{3}$ et $\beta(G) \in \mathbb{Q}_{\geq 0}$ (formules explicites : CAT-3). Un niveau est manipulé comme une fraction $N/D$, $D > 0$, non réduite ; $N/D < N'/D'$ équivaut à $N D' < N' D$, et l'ordre publié des niveaux est cet ordre exact (les moyens de le décider relèvent de l'architecture, § 3 et § 4). L'ensemble $\Lambda = \lbrace \beta(G) : \emptyset \neq G \subseteq X \rbrace$ est fini ; le **rang** d'un niveau est son rang dense parmi les niveaux exacts distincts publiés. Deux écritures différentes d'un même niveau ont le même rang.

### 1.5 Table des notations

| Symbole | Sens |
| --- | --- |
| $X$, $n$ | sites, nombre de sites |
| $k$, $K$ | ordre courant ; plus grand ordre servi |
| $a$, $a_b$, $a_v$ | niveau (carré de rayon) ; niveau d'une boule $b$, d'un nœud $v$ |
| $B(G)$, $c_G$, $\beta(G)$ | plus petite boule englobante de $G$, son centre, son niveau |
| $R_G(a)$, $R_G^{<}(a)$ | régions témoins fermée et ouverte |
| $L_k(a)$, $L_k^{<}(a)$ | coupes fermée et ouverte de l'ordre $k$ |
| $\Gamma_k(a)$, $\Gamma_k^{<}(a)$ | graphes des $k$-parties (§ 4) |
| $b = (c, a_b)$, $I$, $U$, $P_b$, $p$, $m$ | boule critique, intérieur strict, coquille, $P_b = I \cup U$, $p = \lvert I \rvert$, $m = \lvert U \rvert$ |
| $q$, $S^{*}$ | plus petit cardinal d'un support (noté `qmin` dans le code), support canonique |
| $t = k - p$ | nombre de sites de coquille d'une $k$-partie qui contient $I$ |
| $J_k(b)$, $V_k^{<}(b)$, $V_k^{=}(b)$ | $k$-parties de $P_b$ ; celles de niveau $< a_b$ ; celles de niveau $a_b$ |
| $\mu_k(b)$ | nombre de morceaux de $b$ à l'ordre $k$ |
| $\mathcal{T}_k$, $\mathrm{anc}_a(v)$ | arbre de fusion de l'ordre $k$ ; ancêtre de $v$ vivant à la coupe fermée $a$ |
| $Q$, $\bar{Q}$ | pavé demi-ouvert, son adhérence (§ 3) |
| $A_k(x)$, $E_k(x)$ | date et propriétaires de l'entrée `cover` d'un site (§ 8) |

## 2. Boules critiques, supports, catalogue

### 2.1 Définitions

Une **boule** est un couple $b = (c, a)$ avec $a > 0$. On note $I(b) = X \cap B^{\circ}(c,a)$ son **intérieur strict**, $U(b) = X \cap \partial B(c,a)$ sa **coquille**, $P_b = I \cup U$ sa population fermée, $p = \lvert I \rvert$, $m = \lvert U \rvert$. Un site à distance carrée exactement $a$ du centre est dans $U$, jamais dans $I$.

- $b$ est **critique** si $c \in \mathrm{conv}(U)$.
- Un **support** de $b$ est une partie $S \subseteq U$ affinement indépendante avec $c$ dans l'intérieur relatif de $\mathrm{conv}(S)$ (coordonnées barycentriques toutes strictement positives).
- $q(b)$ est le plus petit cardinal d'un support ; $S^{*}(b)$, le **support canonique**, est le plus petit support de cardinal $q$ pour l'ordre lexicographique des indices de sites triés (l'indice d'un site est son rang de Morton, convention du module `cloud`). $S^{*}$ dépend de cette convention ; la boule, $I$, $U$, $p$, $m$ et $q$ n'en dépendent pas.
- La coquille est **régulière** si $m = q$ (alors $U = S^{*}$ est l'unique support), **étendue** si $m > q$.
- La **boule de rayon nul** d'un site $x$ est $(x, 0)$, avec par convention $I = \emptyset$, $U = \lbrace x \rbrace$, $q = 1$. Elle n'est pas une boule critique au sens ci-dessus ; elle porte les naissances de l'ordre 1 et vit dans la table des sites.

### 2.2 Supports

**CAT-1 (supports).** [démontré] Soit $b = (c,a)$ une boule, $a > 0$. (i) Toute partie $A \subseteq U$ avec $c \in \mathrm{conv}(A)$ contient un support. En particulier $b$ est critique si et seulement si elle a un support. (ii) Un support a 2, 3 ou 4 sites. (iii) Si $S$ est un support, $c$ est l'unique point de l'enveloppe affine de $S$ équidistant des sites de $S$ : $S$ détermine $b$. De même $U$ détermine $b$ quand $b$ est critique. (iv) Si $S$ est un support, aucune partie stricte de $S$ ne contient $c$ dans son enveloppe convexe. (v) Toute partie de $U$ de moins de $q$ sites a une enveloppe convexe qui ne contient pas $c$.

*Preuve.* (i) et (ii). Soit $S \subseteq A$ minimale pour l'inclusion avec $c \in \mathrm{conv}(S)$, et $c = \sum_{s \in S} \lambda_s s$, $\lambda_s \geq 0$, $\sum_s \lambda_s = 1$. Par minimalité tous les $\lambda_s$ sont $> 0$. Si $S$ était affinement liée, il existerait $\nu \neq 0$ avec $\sum_s \nu_s s = 0$ et $\sum_s \nu_s = 0$ ; avec $\tau = \min \lbrace \lambda_s / \nu_s : \nu_s > 0 \rbrace$, les coefficients $\lambda - \tau \nu$ sont $\geq 0$, de somme 1, représentent $c$ et s'annulent en un site : contradiction avec la minimalité. Donc $S$ est affinement indépendante, d'au plus 4 sites, les coordonnées barycentriques de $c$ y sont uniques, égales aux $\lambda_s > 0$. Enfin $\lvert S \rvert \geq 2$ car $a > 0$ interdit $c \in U$. (iii) Si $c' \in \mathrm{aff}(S)$ est équidistant de $S$, alors pour $s, s' \in S$ la quantité $\lVert s - c \rVert^{2} - \lVert s - c' \rVert^{2} = 2 \langle s, c' - c \rangle + \lVert c \rVert^{2} - \lVert c' \rVert^{2}$ ne dépend pas de $s$, donc $\langle s - s', c' - c \rangle = 0$ : $c' - c$ est orthogonal à la direction de $\mathrm{aff}(S)$ et lui appartient, d'où $c' = c$, puis $a = \lVert s - c \rVert^{2}$. Le même calcul vaut pour $U$ puisque $c \in \mathrm{conv}(U) \subseteq \mathrm{aff}(U)$. (iv) Une telle partie donnerait à $c$ des coordonnées barycentriques dans $S$ dont l'une est nulle. (v) Sinon (i) fournirait un support de moins de $q$ sites. $\square$

**CAT-2 (boule minimale d'une partie d'une boule).** [démontré] Soit $b = (c,a)$ une boule, $a > 0$, et $F \subseteq P_b$ non vide. Alors $B(F) = b$ si et seulement si $c \in \mathrm{conv}(F \cap U)$ ; sinon $\beta(F) < a$, strictement. Réciproquement, pour toute partie $G \subseteq X$ d'au moins deux sites, $B(G)$ est une boule critique $b$, $G \subseteq P_b$ et $c \in \mathrm{conv}(G \cap U)$. Les boules critiques sont donc exactement les plus petites boules englobantes des parties d'au moins deux sites, et $b = B(P_b) = B(U) = B(S)$ pour tout support $S$.

*Preuve.* Si $c \in \mathrm{conv}(F \cap U)$, CAD-1 (ii) donne $B(F) = b$. Sinon $b$ n'est pas $B(F)$ par CAD-1 (ii) ; comme elle contient $F$, $\beta(F) \leq a$, et $\beta(F) = a$ ferait de $b$ une boule de niveau minimal contenant $F$, donc $B(F)$ par unicité. La réciproque est CAD-1 (ii) appliqué à $B(G)$, de niveau $> 0$. $\square$

**CAT-3 (centre et niveau exacts).** [démontré] Soit $S = \lbrace s_0, \ldots \rbrace$ un support, $u = s_1 - s_0$, $v = s_2 - s_0$, $z = s_3 - s_0$. Le centre s'écrit $c = s_0 + N/D$ et le niveau $a = \lVert N \rVert^{2} / D^{2}$, avec $N \in \mathbb{Z}^{3}$, $D \in \mathbb{Z}_{>0}$ :

- $q = 2$ : $N = u$, $D = 2$ ;
- $q = 3$ : $w = u \times v$, $N = (\lVert u \rVert^{2} v - \lVert v \rVert^{2} u) \times w$, $D = 2 \lVert w \rVert^{2}$ ;
- $q = 4$ : $N = \lVert u \rVert^{2} (v \times z) + \lVert v \rVert^{2} (z \times u) + \lVert z \rVert^{2} (u \times v)$, $D = 2 \det(u, v, z)$, les deux signes étant changés si $D < 0$.

Pour tout site $y$, le signe de $D \lVert y - s_0 \rVert^{2} - 2 \langle N, y - s_0 \rangle$ est celui de $\lVert y - c \rVert^{2} - a$ : négatif dans $I$, nul sur $U$, positif dehors.

*Preuve.* $N/D$ est dans la direction de $\mathrm{aff}(S)$ et vérifie $2 \langle N/D, d \rangle = \lVert d \rVert^{2}$ pour $d \in \lbrace u, v, z \rbrace$ (calcul direct : pour $q = 3$, $\langle N, u \rangle = \lVert u \rVert^{2} \lVert w \rVert^{2}$ et $\langle N, v \rangle = \lVert v \rVert^{2} \lVert w \rVert^{2}$ ; pour $q = 4$, $\langle N, u \rangle = \lVert u \rVert^{2} \det(u,v,z)$ et de même pour $v$ et $z$) : c'est l'équidistance à $s_0$ et aux autres sites. CAT-1 (iii) conclut. Enfin $\lVert y - c \rVert^{2} - a = \lVert y - s_0 \rVert^{2} - 2 \langle y - s_0, N/D \rangle$ puisque $\lVert c - s_0 \rVert^{2} = a$. $\square$

### 2.3 Catalogue et règle d'admission

Pour une boule critique $b$ et un ordre $k$, on pose $t = k - p$. La **cellule** $(b,k)$ existe pour $p < k \leq p + m$, c'est-à-dire $1 \leq t \leq m$.

**CAT-D (catalogue).** Le catalogue d'ordre $K$ est $\mathrm{Cat}_K(X) = \lbrace b \text{ critique} : p + q \leq K + 1 \rbrace$. Chaque boule y porte $S^{*}$, $q$, $p$, $m$, $I$, $U$ et son niveau exact ; l'ordre canonique est (niveau exact, $S^{*}$). L'écriture canonique du niveau est celle de CAT-3 appliquée à $S^{*}$ : elle ne dépend d'aucun ordre d'énumération. La **fenêtre** de $b$ est l'intervalle d'ordres $\left[ \max(1, p + q - 1), \min(K, p + m) \right]$. Égalité : $p + q = K + 1$ est admise, $p + q = K + 2$ ne l'est pas.

**CAT-ADM (portée de l'admission).** [démontré, par LOC-W et TOUR-B] Une boule critique ne crée ni naissance ni fusion à un ordre $k < p + q - 1$ ou $k > p + m$. Toute boule qui porte un événement d'un ordre $\leq K$ est donc dans $\mathrm{Cat}_K$. La règle est **suffisante, pas nécessaire** : une boule admise peut n'avoir aucun événement dans sa fenêtre (fixture `triangle_rectangle`, § 5.4 : la boule de l'hypoténuse est admise à $K = 1$ et n'y porte aucun événement, sa cellule d'ordre 1 n'ayant qu'un morceau).

**CAT-R (restriction).** Par définition, pour $K \leq K'$ : $\mathrm{Cat}_K = \lbrace b \in \mathrm{Cat}_{K'} : p + q \leq K + 1 \rbrace$, avec les mêmes enregistrements (les rangs sont recalculés). C'est l'invariant INV-RESTR du § 7.

Fixtures d'égalité. `ligne_024` : sites $(0,0,0)$, $(2,0,0)$, $(4,0,0)$ ; la boule de la paire extrême (niveau 4, $p = 1$, $q = 2$) est absente de $\mathrm{Cat}_1$ ($p + q = K + 2$) et présente dans $\mathrm{Cat}_2$ ($p + q = K + 1$) ; $\lvert \mathrm{Cat}_1 \rvert = 2$, $\lvert \mathrm{Cat}_2 \rvert = 3$. `circle25_pair` : le niveau 25 est celui d'une paire (écriture $100/4$) et d'un triangle (autre écriture) : même rang.
