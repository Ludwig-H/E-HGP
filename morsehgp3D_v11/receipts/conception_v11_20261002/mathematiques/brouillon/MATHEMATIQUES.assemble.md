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

## 3. Le générateur par boîtes de centres

Le générateur rend $\mathrm{Cat}_K(X)$ sans jamais énumérer les parties de $X$ : il découpe l'espace des **centres** en pavés, attache à chaque pavé une liste de sites qui suffit à recenser toute boule centrée dans le pavé, et énumère les supports dans les feuilles.

### 3.1 Listes certifiées et dominance

Un **pavé** est $Q = \prod_i \left[ l_i, h_i \right)$, demi-ouvert ; $\bar{Q} = \prod_i \left[ l_i, h_i \right]$ est son adhérence et $s_i = h_i - l_i > 0$. Pour $n \geq K$ et $y \in \mathbb{R}^{3}$, $N_K(y) = X \cap \bar{B}(y, D_K(y))$ est la boule fermée des $K$ plus proches voisins, ex æquo compris ; pour $n < K$ on pose $N_K(y) = X$.

- $L \subseteq X$ est **$K$-certifiée pour $Q$** si $N_K(y) \subseteq L$ pour tout $y \in \bar{Q}$ (pavé fermé).
- $z$ **domine** $x$ sur $Q$ si $\lVert z - y \rVert^{2} < \lVert x - y \rVert^{2}$ pour tout $y \in \bar{Q}$ : inégalité stricte, sur le pavé fermé. Un site à égalité de distance en un seul point de $\bar{Q}$ ne domine pas.

**GEN-D (dominateurs).** [démontré] Si au moins $K$ sites dominent $x$ sur $Q$, alors $x \notin N_K(y)$ pour tout $y \in \bar{Q}$.

*Preuve.* En $y$, ces $K$ sites sont strictement plus proches que $x$ : $D_K(y) < \lVert x - y \rVert^{2}$. $\square$

**GEN-DLOC (forme entière de la dominance).** [démontré] Avec $x' = x - l$ et $z' = z - l$ : $z$ domine $x$ sur $Q$ si et seulement si

$$\lVert x' \rVert^{2} - \lVert z' \rVert^{2} > \sum_{i=1}^{3} \max \left( 0, 2 s_i (x'_i - z'_i) \right) .$$

L'égalité des deux membres signifie qu'un coin de $\bar{Q}$ est équidistant de $x$ et de $z$ : pas de dominance.

*Preuve.* Pour $y' = y - l \in \prod_i \left[ 0, s_i \right]$, $\lVert z - y \rVert^{2} - \lVert x - y \rVert^{2} = \lVert z' \rVert^{2} - \lVert x' \rVert^{2} + 2 \langle y', x' - z' \rangle$. Cette fonction affine atteint son maximum sur le pavé fermé coordonnée par coordonnée, et il vaut $\lVert z' \rVert^{2} - \lVert x' \rVert^{2} + \sum_i \max(0, 2 s_i (x'_i - z'_i))$. La dominance est la stricte négativité de ce maximum. $\square$

**GEN-L (restriction des listes).** [démontré] $X$ est $K$-certifiée pour tout pavé. Si $L$ est $K$-certifiée pour $Q$ et si $\bar{Q}' \subseteq \bar{Q}$, alors pour toute partie $Y \subseteq X$ la liste $L' = \lbrace x \in L : \text{moins de } K \text{ sites de } Y \text{ dominent } x \text{ sur } Q' \rbrace$ est $K$-certifiée pour $Q'$.

*Preuve.* Pour $y \in \bar{Q}'$, tout $x \in N_K(y)$ est dans $L$, et par la contraposée de GEN-D moins de $K$ sites le dominent sur $Q'$. $\square$

Le choix de $Y$ ne touche que la force du filtre, jamais l'exactitude.

### 3.2 Recensement local

**GEN-C (recensement local).** [démontré] Soient $L$ une liste $K$-certifiée pour $Q$ et $(c,a)$ une boule avec $c \in \bar{Q}$. Notons $p^{*}$ et $U^{*}$ le nombre de sites strictement intérieurs et la coquille dans $X$, $\hat{p}$ et $\hat{U}$ les mêmes quantités comptées dans $L$. (a) Si $p^{*} \leq K - 1$ : $X \cap \bar{B}(c,a) \subseteq L$, donc $\hat{p} = p^{*}$, $\hat{U} = U^{*}$ et l'intérieur est le même. (b) Si $p^{*} \geq K$ : $\hat{p} \geq K$. Par conséquent $\hat{p} \leq K - 1$ si et seulement si $p^{*} \leq K - 1$, et le recensement fait sur $L$ est alors exact.

*Preuve.* (a) Si $n < K$, $L = X$. Sinon toute boule fermée $\bar{B}(c,a')$ avec $a' < a$ contient au plus $p^{*} < K$ sites, donc $D_K(c) \geq a$ et $X \cap \bar{B}(c,a) \subseteq N_K(c) \subseteq L$. (b) Au moins $K$ sites sont à distance carrée $< a$ de $c$, donc $D_K(c) < a$ : $N_K(c)$, qui compte au moins $K$ sites et est contenu dans $L$, est dans $B^{\circ}(c,a)$. $\square$

Égalités : un site exactement sur la sphère d'une boule avec $p^{*} = K - 1$ est conservé dans la liste (cas (a)) ; à $p^{*} = K$ le recensement n'est plus garanti exact, mais il rend $\hat{p} \geq K$ et la boule est écartée.

### 3.3 Pavé ajusté et partition

**GEN-A (ajustement).** [démontré] Soient $L$ une liste $K$-certifiée pour $Q$, non vide, $e^{-}$ et $e^{+}$ les coins de sa boîte englobante ($e^{-}_i = \min_{x \in L} x_i$, $e^{+}_i = \max_{x \in L} x_i$), et $b$ une boule critique avec $p \leq K - 1$ et $c \in \bar{Q}$. Alors $e^{-}_i \leq c_i \leq e^{+}_i$ pour chaque axe. Si $L$ est vide, aucune telle boule n'a son centre dans $\bar{Q}$.

*Preuve.* Par GEN-C (a), $U \subseteq L$ ; $c \in \mathrm{conv}(U)$ est dans la boîte englobante fermée de $L$. Si $L$ est vide, $U \subseteq L$ est impossible. $\square$

Le pavé ajusté est $Q \cap \prod_i \left[ e^{-}_i, e^{+}_i + \varepsilon \right)$ pour un $\varepsilon > 0$ quelconque : la borne haute $e^{+}_i$ est **atteinte** par des centres (tous les sites d'un support partagent cette coordonnée), d'où le pas $\varepsilon$ ; un pavé ajusté vide ne contient aucun centre admis.

**GEN-P (partition des centres).** [démontré] On part d'un pavé demi-ouvert qui contient $\mathrm{conv}(X)$ avec la liste $X$. À chaque nœud $(Q, L)$, $L$ étant $K$-certifiée pour $Q$, on remplace $Q$ par son pavé ajusté, puis soit on s'arrête (feuille), soit on le coupe par un plan perpendiculaire à un axe en deux moitiés demi-ouvertes dont les listes sont obtenues par GEN-L. Alors toute boule critique avec $p \leq K - 1$ a son centre dans **exactement une** feuille, et la liste de cette feuille est $K$-certifiée pour elle. Ni le choix de l'axe, ni la position de la coupe, ni la règle d'arrêt n'interviennent.

*Preuve.* Le centre est dans $\mathrm{conv}(U) \subseteq \mathrm{conv}(X)$, donc dans la racine. Par récurrence sur la profondeur : si $c \in Q$, GEN-A place $c$ dans le pavé ajusté, dont la liste reste certifiée (pavé plus petit) ; les deux moitiés demi-ouvertes le partitionnent, $c$ est dans une seule, dont la liste est certifiée par GEN-L. Les feuilles sont deux à deux disjointes. $\square$

Égalités : un centre exactement sur le plan de coupe appartient à la moitié haute ; un centre sur la face basse de la racine lui appartient ; la racine doit contenir strictement $\mathrm{conv}(X)$ du côté haut.

### 3.4 Filtres de feuille

Dans une feuille de pavé $Q$ et de liste $L$, on note $\mathrm{Dom}(x)$ l'ensemble des sites de $L$ qui dominent $x$ sur $Q$.

**GEN-M (masques de dominance).** [démontré] Soit $b$ une boule avec $c \in \bar{Q}$. (a) Si $x \in U(b)$, tout site qui domine $x$ sur $Q$ est dans $I(b)$ ; donc pour $A \subseteq U(b)$, $\lvert \bigcup_{x \in A} \mathrm{Dom}(x) \rvert \leq p$. (b) Deux sites de $U(b)$ ne se dominent pas l'un l'autre.

*Preuve.* En $y = c \in \bar{Q}$ : un dominateur $z$ de $x$ vérifie $\lVert z - c \rVert^{2} < \lVert x - c \rVert^{2} = a$ ; deux sites de $U$ sont équidistants de $c$. $\square$

**GEN-Z (droite des équidistants).** [démontré] Soient $a_1, a_2, a_3$ trois sites non alignés, $u = a_1 - a_2$, $v = a_1 - a_3$, $F(y) = \left( \lVert a_1 - y \rVert^{2} - \lVert a_2 - y \rVert^{2}, \lVert a_1 - y \rVert^{2} - \lVert a_3 - y \rVert^{2} \right)$, $y_0$ le centre de $\bar{Q}$ et $g_i = s_i (u_i, v_i) \in \mathbb{R}^{2}$. La droite des points équidistants des trois sites rencontre le pavé **fermé** $\bar{Q}$ si et seulement si, pour chaque axe $i$ avec $g_i \neq 0$ et $\nu_i = (v_i, -u_i)$ :

$$\lvert \langle \nu_i, F(y_0) \rangle \rvert \leq \sum_{j=1}^{3} \lvert \langle \nu_i, g_j \rangle \rvert .$$

L'égalité (droite passant par une arête ou un sommet du pavé) compte comme une rencontre.

*Preuve.* $F$ est affine : $F(y) = F(y_0) - 2 \left( \langle y - y_0, u \rangle, \langle y - y_0, v \rangle \right)$. Quand $y$ parcourt $\bar{Q}$, $y - y_0 = \frac{1}{2} \sum_i \tau_i s_i e_i$ avec $\tau \in \left[ -1, 1 \right]^{3}$, donc $F(\bar{Q}) = F(y_0) + Z$ où $Z = \lbrace \sum_i \tau_i g_i : \tau \in \left[ -1, 1 \right]^{3} \rbrace$ est symétrique par rapport à 0. La droite, lieu $F = 0$, rencontre $\bar{Q}$ si et seulement si $F(y_0) \in Z$. $Z$ est un convexe compact de fonction d'appui $h(\nu) = \sum_j \lvert \langle \nu, g_j \rangle \rvert$. Les $g_i$ engendrent $\mathbb{R}^{2}$ parce que $u \times v \neq 0$ : $Z$ est un polygone d'intérieur non vide. Sa face dans la direction $\nu$ est la somme des faces des segments $\left[ -g_j, g_j \right]$ ; c'est une arête exactement quand $\langle \nu, g_j \rangle = 0$ pour un $g_j \neq 0$, c'est-à-dire quand $\nu$ est proportionnel à un $\nu_j$. Un polygone convexe d'intérieur non vide est l'intersection des demi-plans de ses arêtes, soit ici $\lvert \langle \nu_i, z \rangle \rvert \leq h(\nu_i)$. $\square$

Sous forme entière, $\langle \nu_i, g_j \rangle = s_j (v_i u_j - u_i v_j)$ est, au signe près, $s_j$ fois une composante de $u \times v$, nulle pour $j = i$.

### 3.5 Survie du support canonique et émission unique

On pose $\theta_r = K + 1 - r$ pour $r = 2, 3, 4$ (donc $\theta_2 = K - 1 \geq \theta_3 \geq \theta_4$).

**GEN-S (survie du support canonique).** [démontré] Soit $b \in \mathrm{Cat}_K(X)$ de centre $c$ dans une feuille de pavé $Q$ dont la liste $L$ est $K$-certifiée pour $Q$. Alors :

1. $U \subseteq L$, en particulier $S^{*} \subseteq L$ ;
2. pour toute partie $A \subseteq S^{*}$ : aucun site de $A$ n'en domine un autre sur $Q$, et $\lvert \bigcup_{x \in A} \mathrm{Dom}(x) \rvert \leq p \leq \theta_q \leq \theta_{\lvert A \rvert}$ ;
3. tout triplet de $S^{*}$ est non aligné et sa droite des équidistants rencontre $\bar{Q}$ (elle passe par $c$) ; un tel triplet n'est pas nécessairement un triangle aigu ;
4. $S^{*}$ passe le test de support de son cardinal : $c$ est le milieu de la paire ; ou le triplet est non aligné, strictement aigu, et $c$ est son centre circonscrit ; ou le quadruplet est non coplanaire et $c$, son centre circonscrit, lui est strictement intérieur ;
5. le recensement de la présentation $S^{*}$ sur $L$, interrompu seulement quand le nombre de sites intérieurs dépasse $\theta_q$, va à son terme et rend $I$ et $U$ exacts.

Une énumération qui ne garde que les paires, triplets et quadruplets de $L$ passant ces tests présente donc $S^{*}$ dans la feuille de $c$.

*Preuve.* $p \leq K + 1 - q \leq K - 1$ puisque $q \geq 2$ ; GEN-C (a) donne 1 et l'exactitude du recensement. 2 est GEN-M et la décroissance de $\theta$. 3 : $S^{*}$ est affinement indépendant et $c$ est équidistant de $U$. 4 est la définition d'un support, le centre circonscrit étant intérieur à un triangle si et seulement si le triangle est strictement aigu. 5 : $\hat{p} = p \leq \theta_q$. $\square$

Égalités : $p = \theta_q$ passe (fixtures `ligne_024` à $K = 2$ pour $q = 2$, `triangle_aigu_interieur` à $K = 3$ pour $q = 3$, `tetra_centre` à $K = 4$ pour $q = 4$). Le point 3 interdit d'exiger l'acuité d'un triplet pour le garder comme partie d'un quadruplet : fixture `tetra_face_obtuse`, dont le support de quatre sites a deux faces rectangles.

**GEN-E (émission unique).** [démontré] (a) Une coquille régulière n'a qu'une présentation, $U = S^{*}$. (b) Si toute présentation jugée dont le recensement va à son terme calcule $S^{*}$ par énumération des parties de $U$ dans l'ordre (cardinal, ordre lexicographique) — le cardinal ne dépassant pas celui de la présentation —, une table des $S^{*}$ déjà émis dans la feuille suffit à émettre chaque boule une seule fois.

*Preuve.* (a) CAT-1 (v). (b) Toute présentation jugée est un support de la boule qu'elle définit (test 4), donc $q$ est au plus son cardinal ; le recensement mené à terme est exact (GEN-C), donc toutes les présentations d'une même boule calculent le même $S^{*}$ ; elles sont toutes dans la feuille unique du centre (GEN-P). $\square$

### 3.6 Théorème du générateur

**GEN-G (théorème du générateur).** [démontré] L'arbre de GEN-P, dont chaque feuille énumère selon GEN-S et GEN-E et recense selon GEN-C, puis n'émet que les boules avec $p + q \leq K + 1$, rend exactement $\mathrm{Cat}_K(X)$, chaque boule une fois, avec $I$, $U$, $q$ et $S^{*}$ exacts. Ni la taille de feuille, ni le choix de $Y$, ni la coupe, ni la règle d'arrêt n'interviennent dans l'exactitude.

*Preuve.* Correction : une boule émise vient d'une présentation qui est un support, elle est donc critique ; son recensement terminé donne $\hat{p} \leq \theta \leq K - 1$, donc est exact par GEN-C ; $q$ et $S^{*}$ sont calculés sur la coquille exacte ; l'admission est testée. Complétude : une boule de $\mathrm{Cat}_K$ a $p \leq K - 1$, son centre est dans une feuille unique (GEN-P), où $S^{*}$ est présenté et recensé jusqu'au bout (GEN-S). Unicité : GEN-E. $\square$

### 3.7 Listes inhérentes et coût

**GEN-F (listes inhérentes).** [démontré] Toute liste $K$-certifiée pour $Q$ contient $N_K(y)$ pour chaque $y \in \bar{Q}$. Une boule de centre $c$ avec $p \leq K - 1$ impose donc au moins $p + m$ sites à la liste de tout pavé dont l'adhérence contient $c$, à toute profondeur : une grande coquille donne une grande liste, quel que soit le filtre. C'est la raison d'être d'un refus `resource_exhausted` sur feuille trop large ; ce n'est pas une faiblesse de GEN-L.

**GEN-COUT (ce qui est prouvé du coût).** Ce qui est démontré :

1. La sortie n'est pas linéaire dans le pire cas [démontré] : pour tout $m \geq 2$ il existe $2m$ points de $\mathbb{R}^{3}$ avec $\lvert \mathrm{Cat}_1 \rvert \geq m^{2}$. *Preuve.* Soient $R > 0$, $0 < \varepsilon \leq 1$ avec $\varepsilon^{2} < 2 / (\pi (m-1))$, $\theta_i = i \varepsilon / (m-1)$ pour $0 \leq i < m$, $A_i = (R \cos \theta_i, R \sin \theta_i, 0)$ et $B_j = (R - R \cos \theta_j, 0, R \sin \theta_j)$ : deux arcs de cercles enlacés, chacun passant par le centre de l'autre. Un calcul direct donne $\langle A_i - A_{i'}, B_j - A_{i'} \rangle = R^{2} \left( (\cos \theta_i - \cos \theta_{i'})(1 - \cos \theta_j) + 1 - \cos(\theta_i - \theta_{i'}) \right)$. Si $\theta_i < \theta_{i'}$ les deux termes sont $\geq 0$ et le second est $> 0$. Si $\theta_i > \theta_{i'}$, l'expression vaut $2 R^{2} \sin \frac{\theta_i - \theta_{i'}}{2} \left( \sin \frac{\theta_i - \theta_{i'}}{2} - \sin \frac{\theta_i + \theta_{i'}}{2} (1 - \cos \theta_j) \right)$, et la parenthèse est $\geq \frac{\varepsilon}{\pi (m-1)} - \frac{\varepsilon^{3}}{2} > 0$. L'isométrie $(x,y,z) \mapsto (R - x, z, y)$ échange les deux arcs, d'où la même inégalité pour $\langle B_j - B_{j'}, A_i - B_{j'} \rangle$. Or $\langle x - y, x' - y \rangle > 0$ dit que $y$ est strictement hors de la boule fermée de diamètre $x x'$ : chacune des $m^{2}$ boules diamétrales $A_i B_j$ a $I = \emptyset$, $U = \lbrace A_i, B_j \rbrace$, donc $p + q = 2$. $\square$ Fixture entière `arcs_enlaces_16` (16 sites obtenus en arrondissant à la grille deux arcs de 8 points, $R = 130000$ ; vérifié exactement : 64 paires croisées, $\lvert \mathrm{Cat}_1 \rvert = 78$).
2. La terminaison dépend de la seule règle d'arrêt : avec des pavés à bornes entières, une coupe au milieu du plus long côté et l'arrêt quand ce côté vaut 1, la profondeur est bornée par trois fois le nombre de bits du côté de la racine.
3. GEN-F : la liste d'une feuille ne descend pas sous $\max_{y \in \bar{Q}} \lvert N_K(y) \rvert$.

Ce qui ne l'est pas [à prouver, OBL-4] : toute borne du nombre de nœuds, de feuilles ou de recensements en fonction de $n$ et de la sortie ; la loi du coût en fonction de la marge entre la taille de feuille et $K$ (mesuré sur la v10 : 14 points à $K = 10$, 1 nœud pour une feuille de 24 sites, plus de 3 millions pour une feuille de 13) ; la linéarité observée sur LiDAR, qui reste une cible expérimentale jugée aux tailles 8 000, 16 000, 32 000 et sur les trames.

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

## 7. Invariants globaux

Ces invariants se calculent en temps linéaire en la sortie (ou par un juge indépendant) et servent de portes d'échelle. Aucun n'est un certificat de complétude : la complétude du catalogue est GEN-G, l'exactitude de la tour TOUR-E.

### 7.1 Identité d'Euler

**INV-EULER.** [démontré] Pour une boule critique $b$ et un ordre $k$, posons $t = k - p$ et

$$e_k(b) = \sum_{B \subseteq U,\ c \in \mathrm{conv}(B),\ \lvert B \rvert \geq t} (-1)^{\lvert B \rvert - t} \binom{\lvert B \rvert - 1}{t - 1} \quad \text{si } 1 \leq t \leq m, \qquad e_k(b) = 0 \text{ sinon.}$$

Alors, pour tout $1 \leq k \leq n$, la somme portant sur **toutes** les boules critiques de $X$ :

$$n \, \left[ k = 1 \right] + \sum_{b} e_k(b) = 1 .$$

Pour une coquille régulière de $q$ sites, $e_k(b) = (-1)^{q-t} \binom{q-1}{t-1}$ pour $1 \leq t \leq q$ : une paire donne $(-1, +1)$, un triangle $(+1, -2, +1)$, un tétraèdre $(-1, +3, -3, +1)$ aux ordres $p + 1, \ldots, p + q$.

*Preuve.* (1) Pour des entiers $N \geq k \geq 1$ : $\sum_{s=k}^{N} (-1)^{s-k} \binom{N}{s} \binom{s-1}{k-1} = 1$. C'est vrai pour $N = k$ ; pour $N > k$, la relation de Pascal $\binom{N}{s} = \binom{N-1}{s} + \binom{N-1}{s-1}$ ramène la somme à celle de $N - 1$ plus $\sum_{s=k}^{N} (-1)^{s-k} \binom{N-1}{s-1} \binom{s-1}{k-1} = \binom{N-1}{k-1} \sum_{s=k}^{N} (-1)^{s-k} \binom{N-k}{s-k} = 0$. (2) Avec $N = n$ : $\sum_{G \subseteq X,\ \lvert G \rvert \geq k} (-1)^{\lvert G \rvert - k} \binom{\lvert G \rvert - 1}{k-1} = 1$. (3) On regroupe les parties $G$ par leur plus petite boule englobante. Les singletons ne comptent que pour $k = 1$, chacun pour 1. Par CAT-2, les parties d'au moins deux sites de plus petite boule $b$ sont exactement les $G = J \cup B$ avec $J \subseteq I$, $B \subseteq U$ et $c \in \mathrm{conv}(B)$. (4) À $B$ fixé de cardinal $s$, la somme sur $J$ vaut $(-1)^{s-k} \sum_{j=0}^{p} (-1)^{j} \binom{p}{j} \binom{j+s-1}{k-1}$, avec la convention $\binom{r}{j} = 0$ pour $j < 0$ ou $j > r$, ce qui écarte de lui-même les parties de moins de $k$ sites. Comme $g(x) = \binom{x+s-1}{k-1}$ vérifie $g(x+1) - g(x) = \binom{x+s-1}{k-2}$, sa différence finie $p$-ième en 0 est $\sum_j (-1)^{p-j} \binom{p}{j} g(j) = \binom{s-1}{k-1-p}$, nulle si $k - 1 - p < 0$. La somme sur $J$ vaut donc $(-1)^{s-t} \binom{s-1}{t-1}$, et la somme sur $B$ donne $e_k(b)$. $\square$

L'identité est le regroupement, par plus petite boule, d'une identité binomiale sur toutes les parties : elle ne suppose ni position générale ni théorie de Morse.

*Remarque (lecture topologique, non utilisée).* [externe] La caractéristique d'Euler est additive sur les réunions finies de convexes compacts ; avec (1) appliqué en chaque point au nombre de boules qui le contiennent, la somme partielle des termes de niveau $\leq a$ vaut $\chi(L_k(a))$, et $L_k(a)$ est étoilé pour $a \geq \beta(X)$. C'est la preuve de la lecture L02 ; elle explique le nom, la preuve ci-dessus s'en passe.

**Portée.**

- *Quelles sphères.* Une boule ne contribue qu'aux ordres $p + 1 \leq k \leq p + m$. L'identité à l'ordre $k$ exige donc **toutes les boules critiques avec $p \leq k - 1$**, quel que soit $q$. $\mathrm{Cat}_K$ ne suffit pas à l'ordre $K$ (il lui manque les boules avec $p \leq K - 1$ et $p + q \geq K + 2$) ; il suffit aux ordres $\leq K - 2$ ; $\mathrm{Cat}_{K+2}$, ou le catalogue d'admission $p \leq K - 1$, suffit aux ordres $\leq K$.
- *Condition nécessaire, pas certificat.* Une boule régulière manquante change la somme de $\pm \binom{q-1}{t-1} \neq 0$ à chacun des ordres $p + 1, \ldots, p + q$. Mais deux omissions peuvent se compenser : fixture `euler_compensation` (auditeur indépendant), sites $(0,5,0)$, $(8,9,0)$, $(8,1,0)$, $(35,5,0)$, $(45,5,0)$, ordre 1 ; la boule du triangle de centre $(5,5,0)$ ($e_1 = +1$) et celle de la paire de centre $(40,5,0)$ ($e_1 = -1$) ont le même niveau 25 ; les retirer toutes deux laisse la somme égale à 1 et supprime une fusion. Un succès d'Euler ne s'appelle donc jamais « catalogue certifié ».
- *Ce qu'il voit.* Fixture `triangle_scalene`, sites $(0,0,0)$, $(6,0,0)$, $(2,5,0)$, ordre 1 : trois paires ($-1$ chacune) et un triangle aigu ($+1$), somme $3 - 3 + 1 = 1$. Sans la boule de la paire $(0,0,0)$, $(6,0,0)$, de niveau 9, la tour publie une fusion au niveau $41/4$ au lieu de 9, avec une seule racine et sans violer TOUR-A ; la somme vaut alors 2.
- Fixture `carre` : la boule étendue de niveau 2 a $e_k = (1, -3, 1, 1)$ pour $k = 1, \ldots, 4$, chaque boule de côté $(-1, +1)$ ; les sommes valent $4 - 4 + 1$, $4 - 3$, $1$, $1$.

### 7.2 Ordre 1 et arbre couvrant minimal euclidien

**INV-EMST.** [démontré] Soit $M$ un arbre couvrant de poids minimal du graphe complet sur $X$ pondéré par $\lVert x - x' \rVert^{2}$ (un choix quelconque en cas d'ex æquo). Pour tout $a \geq 0$, les composantes de $\Gamma_1(a)$ sont celles du graphe $\left( X, \lbrace e \in M : \lVert e \rVert^{2} \leq 4a \rbrace \right)$. Donc $\mathcal{T}_1$ est le dendrogramme N-aire du lien simple : ses feuilles sont les sites, au niveau 0 ; un nœud de niveau $a$ et d'arité $r$ correspond à $r - 1$ arêtes de $M$ de longueur carrée exactement $4a$, et pour chaque niveau $\sum_{v : a_v = a} (\text{arité}(v) - 1) = \lvert \lbrace e \in M : \lVert e \rVert^{2} = 4a \rbrace \rvert$.

*Preuve.* $\Gamma_1(a)$ a pour sommets les sites et pour arêtes les paires de niveau $\lVert x - x' \rVert^{2} / 4 \leq a$. Si une telle paire n'est pas dans $M$, le chemin de $M$ entre ses extrémités n'a que des arêtes de poids $\leq \lVert x - x' \rVert^{2}$ (sinon un échange donnerait un arbre plus léger) : les deux graphes ont les mêmes composantes à tout seuil. Dans une composante $C$ de $\Gamma_1(a)$, $M$ induit un arbre sur $C$ et un arbre sur chaque composante de $\Gamma_1^{<}(a)$ contenue dans $C$ ; la différence des nombres d'arêtes est $o(C) - 1$. $\square$

La porte compare la **structure N-aire**, pas seulement le multiensemble des poids : une multifusion binarisée garde les mêmes poids.

### 7.3 Autres invariants

**INV-RESTR (restriction).** [démontré, par GEN-G] La sortie du générateur à l'ordre $K$ égale la restriction $\lbrace p + q \leq K + 1 \rbrace$ de sa sortie à l'ordre $K + 2$, enregistrement par enregistrement (rangs recalculés). Les deux membres sont $\mathrm{Cat}_K$ ; les deux exécutions n'ont ni les mêmes listes ni les mêmes seuils $\theta$ : c'est un juge du générateur, pas un théorème de plus.

**INV-RACINE (racine unique).** [démontré, TOUR-A (iii)] Chaque $\mathcal{T}_k$, $k \leq n$, a une seule racine ; de façon équivalente, le nombre de feuilles moins $\sum_{v \text{ fusion}} (\text{arité}(v) - 1)$ vaut 1.

**INV-PLATEAU (plateaux atomiques).** [démontré, TOUR-A (ii) et TOUR-C] Aucun nœud n'a un parent de même niveau ; toute fusion a au moins deux enfants ; deux nœuds de même niveau ont des sous-arbres disjoints.

**INV-VIVANT (attaches vivantes à la coupe fermée).** [démontré, TOUR-A (i), TOUR-F, PTS-CORE, PTS-COVER] Tout nœud $w$ publié comme attache à un niveau $a$ vérifie $a_w \leq a$ et, s'il a un parent, $a < a_{\mathrm{parent}(w)}$ (large à gauche, strict à droite) : l'image verticale d'un nœud $v$ ($a = a_v$), le nœud `core` d'un site ($a = D_k(x)$), chaque nœud de $E_k(x)$ ($a = A_k(x)$). Pour une fusion $v$, les $\mathrm{anc}_{a_v}$ des images de ses enfants sont tous égaux à l'image de $v$.

**INV-NEUTRE (neutralité des descentes).** [démontré, TOUR-D] Deux politiques de descente valides (choix du saut, du représentant, des raccourcis) donnent la même forêt, les mêmes verticales et les mêmes attaches.

## 8. Hiérarchies de points

On fixe un ordre $k \leq K$. Pour un nœud $v$ vivant à la coupe fermée $a$, $C_v(a)$ est la composante correspondante de $L_k(a)$.

### 8.1 Objets

- **Amas discret** (thèse, définition 8) : $\mathrm{cov}_a(v) = \lbrace x \in X : \exists y \in C_v(a), \lVert x - y \rVert^{2} \leq a \rbrace$, égal à la couverture de $v$ (NERF-4, TOUR-G).
- **Cœur** : $C_v(a) \cap X$.
- **Entrée `core`** d'un site $x$ : date $D_k(x)$ ; propriétaire, le nœud vivant à la coupe fermée $D_k(x)$ dont la composante contient $x$.
- **Entrée `cover`** d'un site $x$ : date $A_k(x) = \min \lbrace \beta(F) : x \in F, \lvert F \rvert = k \rbrace$, niveau de la plus petite boule fermée contenant $x$ et $k - 1$ autres sites ; propriétaires, l'**ensemble** $E_k(x)$ des nœuds vivants à la coupe fermée $A_k(x)$ dont l'amas discret contient $x$.
- **Règle ancrée** : la donnée, pour chaque site, d'une date $t(x)$ et d'un nœud $o(x)$ vivant à la coupe fermée $t(x)$. Ses **blocs** au niveau $a$ sont, pour chaque nœud $v$ vivant à la coupe fermée $a$, $\lbrace x : t(x) \leq a, \mathrm{anc}_a(o(x)) = v \rbrace$ ; un site avec $t(x) > a$ n'est dans aucun bloc.

### 8.2 Ce qui est démontré

**PTS-CORE.** [démontré] (i) $x \in L_k(a)$ si et seulement si $D_k(x) \leq a$ : le site entre à la coupe fermée $D_k(x)$. (ii) Le propriétaire est bien défini et ne dépend d'aucun départage des ex æquo : c'est le nœud de la composante qui contient toutes les $k$-parties de $X \cap \bar{B}(x, D_k(x))$ ; il se calcule par la descente de l'une d'elles suivie de $\mathrm{anc}_{D_k(x)}$. (iii) `core` est une règle ancrée dont le bloc de $v$ au niveau $a$ est $C_v(a) \cap X$.

*Preuve.* NERF-4 (b) et TOUR-D. $\square$

**PTS-COVER.** [démontré] (i) $A_k(x) = \min \lbrace a_b : x \in P_b, \lvert P_b \rvert \geq k \rbrace$, le minimum portant sur les boules critiques et, pour $k = 1$, sur la boule de rayon nul de $x$. Aucun amas discret ne contient $x$ aux niveaux $a < A_k(x)$ ; au moins un le contient à tout niveau $a \geq A_k(x)$ : $A_k(x)$ est la date de **première couverture**, égalité incluse. (ii) Toute boule $b$ de niveau $A_k(x)$ avec $x \in P_b$ et $\lvert P_b \rvert \geq k$ vérifie $p + q \leq k$ : pour $k \geq 2$ elle est dans $\mathrm{Cat}_{k-1} \subseteq \mathrm{Cat}_K$. (iii) $E_k(x)$ est l'ensemble des nœuds des blocs $J_k(b)$ de ces boules. Dans l'ordre du catalogue, la première boule avec $x \in P_b$ et $p + m \geq k$ a le niveau $A_k(x)$, et les boules de ce niveau qui ont la même propriété donnent $E_k(x)$.

*Preuve.* (i) Une boule avec $x \in P_b$, $\lvert P_b \rvert \geq k$ contient une $k$-partie $F \ni x$ de niveau $\leq a_b$ ; inversement $F$ est dans $P_{B(F)}$. La suite est NERF-4 (a). (ii) Une $k$-partie $F \ni x$ de $P_b$ vérifie $A_k(x) \leq \beta(F) \leq a_b = A_k(x)$, donc $B(F) = b$ et $F$ est de niveau minimal parmi toutes les $k$-parties contenant $x$ ; l'argument de TOUR-G (1) donne $p + q \leq k$. (iii) NERF-4 (a) et LOC-1. $\square$

**PTS-ENC (encadrement).** [démontré] $\frac{1}{4} D_k(x) \leq A_k(x) \leq D_k(x)$ ; en rayons, $d_k / 2 \leq \alpha_k \leq d_k$. L'égalité de gauche a toujours lieu pour $k = 2$ (boule diamétrale de $x$ et de son plus proche voisin). L'égalité de droite a lieu pour $k = 1$ ($0 = 0$) et peut avoir lieu dès $k = 3$ : fixture `ligne_012`, sites $(0,0,0)$, $(1,0,0)$, $(2,0,0)$, site du milieu, $A_3 = D_3 = 1$ et $A_2 = \frac{1}{4} D_2 = \frac{1}{4}$.

*Preuve.* $\bar{B}(x, D_k(x))$ contient $x$ et $k - 1$ autres sites : une $k$-partie contenant $x$ a un niveau $\leq D_k(x)$. Si une $k$-partie $F \ni x$ tient dans une boule de rayon $\rho$, ses sites sont à distance $\leq 2 \rho$ de $x$ : $D_k(x) \leq 4 \rho^{2}$. $\square$

**PTS-LAM (laminarité).** [démontré] Les blocs d'une règle ancrée sont emboîtés : pour $a \leq a'$, tout bloc du niveau $a$ est contenu dans un bloc du niveau $a'$, et deux blocs d'un même niveau sont disjoints. Cela vaut pour `core`, pour toute règle qui choisit un propriétaire unique $o(x) \in E_k(x)$ avec $t(x) = A_k(x)$, et pour la projection par ancêtre commun (PTS-ANC).

*Preuve.* Si $\mathrm{anc}_a(o(x)) = \mathrm{anc}_a(o(x'))$, alors $\mathrm{anc}_{a'}(o(x)) = \mathrm{anc}_{a'}(o(x'))$ puisque $\mathrm{anc}_{a'} \circ \mathrm{anc}_a = \mathrm{anc}_{a'}$, et $t(x), t(x') \leq a \leq a'$. $\square$

**PTS-ANC (projection par ancêtre commun).** [démontré] Soit $o(x)$ le plus petit ancêtre commun des nœuds de $E_k(x)$ et $t(x) = a_{o(x)}$ si $\lvert E_k(x) \rvert \geq 2$, $t(x) = A_k(x)$ sinon. C'est une règle ancrée, équivariante par isométrie et par renumérotation, sans départage. Elle est **distincte** de l'entrée `cover` : $t(x) > A_k(x)$ strictement dès que $\lvert E_k(x) \rvert \geq 2$ (les nœuds de $E_k(x)$ sont distincts et vivants à la même coupe, leur ancêtre commun est né strictement après).

**PTS-NP (les blocs sont dans les amas discrets).** [démontré] Pour `core`, pour toute règle à propriétaire $o(x) \in E_k(x)$ et pour PTS-ANC, le bloc de $v$ au niveau $a$ est contenu dans $\mathrm{cov}_a(v)$.

*Preuve.* Le propriétaire couvre $x$ à sa date (pour PTS-ANC, sa composante contient celles des nœuds de $E_k(x)$), et la couverture croît le long des ancêtres (NERF-3 (a), NERF-4). $\square$

**PTS-MONO (monotonies).** [démontré] (i) $A_k(x) \leq D_k(x)$ : `cover` n'entre jamais après `core` (PTS-ENC). (ii) $D_k(x) \leq D_{k+1}(x)$ et $A_k(x) \leq A_{k+1}(x)$. (iii) `core` est verticale : pour $a \geq D_k(x)$, $\phi_k^{a}$ envoie le nœud `core` de $x$ à l'ordre $k$ au niveau $a$ sur son nœud `core` à l'ordre $k - 1$ ; tout bloc `core` de l'ordre $k$ est contenu dans un bloc `core` de l'ordre $k - 1$ au même niveau.

*Preuve.* (ii) Une $(k+1)$-partie contenant $x$ contient une $k$-partie contenant $x$, de niveau inférieur ou égal. (iii) $x \in R_F(a) \subseteq R_{F \setminus \lbrace y \rbrace}(a)$ pour $F \subseteq X \cap \bar{B}(x,a)$. $\square$

**PTS-K1 (ordre 1).** [démontré] Pour $k = 1$ : $D_1 = A_1 = 0$, $E_1(x)$ est la feuille de $x$, l'amas discret d'une composante est son cœur, les amas discrets forment une partition à chaque niveau, les deux entrées coïncident, et $\mathcal{T}_1$ est le lien simple (INV-EMST).

**PTS-STAB (stabilité de `core`).** [démontré] Soient $(x_i)$ et $(y_i)$ deux nuages de sites distincts indexés par le même ensemble, avec $\lVert x_i - y_i \rVert \leq \varepsilon$. Notons $u^{X}(i,j)$ le plus petit **rayon** $r$ tel que $x_i$ et $x_j$ soient dans une même composante de $L_k^{X}(r^{2})$. Alors $\lvert u^{X}(i,j) - u^{Y}(i,j) \rvert \leq 2 \varepsilon$, et la constante 2 est atteinte.

*Preuve.* Si $y \in L_k^{X}(r^{2})$, ses $k$ témoins déplacés restent à distance $\leq r + \varepsilon$ : $L_k^{X}(r^{2}) \subseteq L_k^{Y}((r + \varepsilon)^{2})$. Si $x_i \in L_k^{X}(r^{2})$, tout point du segment $\left[ x_i, y_i \right]$ est à distance $\leq \varepsilon$ de $x_i$, donc à distance $\leq r + 2 \varepsilon$ des témoins déplacés : le segment est dans $L_k^{Y}((r + 2 \varepsilon)^{2})$. Pour $r = u^{X}(i,j)$, $y_i$, $x_i$, $x_j$, $y_j$ sont donc dans une même composante de $L_k^{Y}((r + 2\varepsilon)^{2})$ ; on échange les rôles. Égalité : $k = 2$, $\lbrace (1,0,0), (11,0,0) \rbrace$ et $\lbrace (0,0,0), (12,0,0) \rbrace$, $\varepsilon = 1$, rayons 10 et 12. $\square$

**PTS-MR (encadrement par l'atteignabilité mutuelle).** [démontré ; sert aux bancs, pas au moteur] Soit $\Pi_k(a)$ la partition `core` au niveau $a$ et $M_k(\rho)$ la partition en composantes du graphe sur $\lbrace x : D_k(x) \leq \rho^{2} \rbrace$ dont les arêtes sont les paires à distance $\leq \rho$ (atteignabilité mutuelle de HDBSCAN, la distance-cœur étant la distance au $k$-ième site, $x$ compté). Alors tout bloc de $\Pi_k(r^{2})$ est dans un bloc de $M_k(2r)$, et tout bloc de $M_k(\rho)$ est dans un bloc de $\Pi_k(\frac{9}{4} \rho^{2})$. Pour $k = 1$, $\Pi_1(r^{2}) = M_1(2r)$.

*Preuve.* Les sites d'une $k$-partie de niveau $\leq r^{2}$ sont deux à deux à distance $\leq 2r$ et ont donc chacun $k$ sites à distance $\leq 2r$ ; de même pour la réunion de deux $k$-parties adjacentes ; un site du cœur appartient à la $k$-partie de ses $k$ plus proches voisins. Inversement, si $D_k(x), D_k(x') \leq \rho^{2}$ et $\lVert x - x' \rVert \leq \rho$, tout point du segment est à distance $\leq \rho / 2$ d'une extrémité, donc à distance $\leq 3 \rho / 2$ de $k$ sites. $\square$

### 8.3 Ce qui n'est pas vrai, et ce qui dépend d'un départage

**PTS-DEP (ce qui dépend d'un départage).** [démontré] Sont canoniques, sans aucun choix : l'entrée `core` ; la date $A_k(x)$ et l'ensemble $E_k(x)$ ; PTS-ANC. Dépend d'un choix : le propriétaire unique d'un site avec $\lvert E_k(x) \rvert \geq 2$, et avec lui les blocs aux seuls niveaux $a$ tels que $A_k(x) \leq a < a_{\mathrm{lca}(E_k(x))}$ ; à tout autre niveau les blocs n'en dépendent pas. Il **n'existe pas** de choix d'un propriétaire unique dans $E_k(x)$ qui soit équivariant par toutes les isométries : fixture `ligne_024`, ordre 2, la réflexion $x \mapsto 4 - x$ fixe le site du milieu et échange ses deux propriétaires admissibles. L'objet publié est donc le couple $(A_k(x), E_k(x))$ ; toute règle à propriétaire unique est une convention nommée à part (PTS-ANC ; ou un ordre exact des centres, déterministe dans un repère donné mais non invariant par échange d'axes ; ou, pour le seul différentiel, l'ordre du catalogue de la v10, qui dépend du rang de Morton).

**Contre-exemples gravés** (valeurs lues dans l'oracle de définition).

| Id | Énoncé réfuté | Fixture, ordre | Valeurs exactes |
| --- | --- | --- | --- |
| PTS-X1 | « `cover` est stable » | `f2_gauche` $(0,0,0)$, $(999,0,0)$, $(2000,0,0)$ et `f2_droite` $(0,0,0)$, $(1001,0,0)$, $(2000,0,0)$, ordre 2 | le niveau de réunion des deux premiers sites passe de $998001/4$ à $1000000$ (rayons 499,5 et 1000) pour un déplacement de 2 ; `core` passe de $998001$ à $1002001$ (rayons 999 et 1001) |
| PTS-X2 | « `cover` est verticale » | `ligne_01269` $(0,0,0)$, $(1,0,0)$, $(2,0,0)$, $(6,0,0)$, $(9,0,0)$, ordres 2 et 3, niveau 9 | sites désignés par leur abscisse ; ordre 2 : blocs $\lbrace 0,1,2 \rbrace$ et $\lbrace 6,9 \rbrace$ ; ordre 3 : bloc $\lbrace 0,1,2,6 \rbrace$, contenu dans aucun des deux |
| PTS-X3 | « un site est affecté au nœud de la composante qui le contient » | `firstcov_k3_n6` $(2,4,4)$, $(2,8,5)$, $(2,9,1)$, $(3,7,0)$, $(6,9,6)$, $(8,10,7)$, ordre 3, niveau 18 | le site $(6,9,6)$ est le cœur du nœud né au niveau 11 ; son entrée `cover`, de date $41/4$, l'affecte à un autre nœud vivant |
| PTS-X4 | « un site entre à un niveau de nœud » | `internal_k3` $(15,4,0)$, $(5,4,0)$, $(7,8,0)$, $(7,0,0)$, $(1,4,0)$, $(0,4,1)$, ordre 3 | le site $(15,4,0)$ a pour date `cover` 25, niveau qui n'est ni une naissance ni une fusion ; son propriétaire est la racine, née au niveau $169/9$ |
| PTS-X5 | « les amas discrets forment une partition » ($k \geq 2$) | `ligne_024`, ordre 2, niveau 1 | le site $(2,0,0)$ est dans deux amas discrets ; aucun site n'est couvert avant le niveau 1 |

**Non démontré.** Aucune propriété de continuité de l'entrée `cover` ou d'une règle à propriétaire unique n'est démontrée, et PTS-X1 réfute la plus simple. Les règles de recherche (votes, dates à marge, existence mûre, condensation, sélection) ne sont pas dans ce document et ne sont citées par aucun module du moteur.

## 9. Ce que la thèse énonce, et ce qui en diffère

Références : manuscrit, Parties I et II (pages PDF 35 à 134). Ni la thèse ni la v10 ne sont des autorités ; chaque ligne dit ce que ce document reprend, redémontre ou réfute.

| Thèse | Ici | Différence |
| --- | --- | --- |
| niveau = rayon $r$, boules fermées (déf. 7, 20) | niveau $a = r^{2}$, coupes fermée et ouverte | changement de variable croissant ; la coupe ouverte est ajoutée |
| $\Gamma_K$ : sommets = $K$-parties du complexe de Čech, adjacence « la réunion est un simplexe », sans borne de taille (déf. 21) ; les adjacences élémentaires suffisent (prop. 5) | $\Gamma_k(a)$ à adjacences élémentaires | aucune : la prop. 5 est l'argument d'échange de NERF-2 |
| $K$-polyèdres = amas discrets (th. 2) | NERF-2 et NERF-4, redémontrés ; coupe ouverte et naturalité verticale en plus | aucune sur la coupe fermée |
| amas discret $= X \cap \delta_r(C)$ ; « poser $C \cap X$ aurait été une erreur » (déf. 8, remarque 1) | `cover` suit la définition 8 ; `core` est $C \cap X$, objet différent et déclaré | `core` n'est pas l'amas discret de la thèse |
| position générale exigée pour les th. 4 à 7 (déf. 26) | aucune hypothèse | sous la déf. 26 toute coquille est régulière (appliquer la déf. 26 à $\sigma = S^{*}$) ; ici les coquilles étendues sont traitées par le quotient local |
| un simplexe $K$-séparant est de Gabriel (th. 4) | cas régulier de LOC-W | vrai sous la déf. 26 : une $(k+1)$-partie $\sigma$ non de Gabriel a une boule $b = B(\sigma)$ avec $p + m \geq k + 2$ et $m = q$, donc $k \leq p + q - 2$ : inerte |
| le $K$-graphe de Gabriel contient toutes les fusions utiles (prop. 6) ; le $K$-arbre couvrant minimal élagué redonne les $K$-polyèdres (th. 5) | **faux en général** : THESE-5 | remplacé par TOUR-E |
| les simplexes de Gabriel sont portés par la mosaïque de Delaunay d'ordre $K$ (th. 6, 7) | non utilisé | invariant d'architecture : la mosaïque d'ordre supérieur n'est jamais matérialisée |
| $K \leq \lvert X \rvert - 1$ au chapitre 8 ; un seul $K$ à la fois | tous les ordres $k \leq \min(K, n)$, $k = n$ compris, et les applications verticales | la tour est un ajout du projet |
| partition stricte par vote pondéré (§ 9.1, prop. 7) | hors de ce document | une sélection fixée donne une partition, pas des partitions emboîtées |

**THESE-5 (le théorème 5 du manuscrit est faux en général).** [faux] Énoncé de la thèse (déf. 28 à 30, prop. 6, th. 5) : sous la position générale de la déf. 26, pour tout $r$, les ensembles de points des composantes non réduites à un sommet du $K$-graphe de Gabriel élagué à $r$ sont les $K$-polyèdres de Čech non réduits à une $K$-partie isolée ; ce graphe a pour sommets les $K$-parties qui sont facettes d'au moins un simplexe de Gabriel à $K + 1$ points (simplexe dont la plus petite boule ouverte ne contient aucun point hors de lui) et relie entre elles les facettes de chaque tel simplexe.

Contre-exemple exact, $K = 2$, cinq points du plan : $A = (0,100,0)$, $C = (200,100,0)$, $z = (101,10,0)$, $y = (130,15,0)$, $w = (103,400,0)$.

- La déf. 26 est satisfaite : aucun point hors de $\sigma$ n'est sur la frontière de $B(\sigma)$, pour chacune des parties $\sigma$ d'au moins deux points.
- Les triangles de Gabriel sont exactement $Czy$ (niveau $17901/4$), $Azy$ ($24125/4$) et $ACw$ ($10001440081/360000$). La paire $AC$ a pour plus petite boule sa boule diamétrale, de niveau 10000, qui contient $z$ et $y$ strictement : les triangles $ACz$ et $ACy$ ont cette même boule et ne sont pas de Gabriel.
- Le graphe de Gabriel a donc, à tout niveau $\geq 10001440081/360000$, deux composantes : $\lbrace Cz, Cy, zy, Az, Ay \rbrace$, de points $\lbrace A, C, z, y \rbrace$, et $\lbrace AC, Aw, Cw \rbrace$, de points $\lbrace A, C, w \rbrace$. Il n'est jamais connexe : le « $K$-arbre couvrant minimal » n'existe pas comme arbre.
- $\Gamma_2$ n'a qu'une composante, de points $\lbrace A, C, z, y, w \rbrace$, à partir du même niveau ; son arbre de fusion a huit nœuds, la racine étant la fusion ternaire de niveau $10001440081/360000$.

Le désaccord est permanent. Vérifié par l'oracle de définition (pièce `these_th5.py`). La fixture E5 du registre racine (`gabriel-point-set-counterexample-5-points-v1`, points $(0,0,7)$, $(0,9,6)$, $(1,4,0)$, $(0,0,1)$, $(4,1,2)$) donne un désaccord de même nature, limité à l'intervalle de niveaux $\left[ 83886/3563, 24 \right)$.

*Où la preuve de la proposition 6 échoue.* La récurrence porte sur les **ensembles de points** des composantes. Quand un simplexe non de Gabriel $\sigma$ naît au niveau $r$, la preuve constate que ses facettes nées au même niveau « ne changent pas l'ensemble de points de la composante » : c'est vrai au niveau $r$. Mais l'hypothèse de récurrence ne retient pas que ces facettes **appartiennent désormais à cette composante**. Quand l'une d'elles est plus tard la facette active d'un simplexe de Gabriel, $\Gamma_K$ réunit sa composante aux autres, alors que le graphe de Gabriel n'y voit qu'un sommet neuf. L'égalité des ensembles de points n'est pas un invariant de récurrence. Ici la facette est $AC$, née au niveau 10000 dans $ACz$ et $ACy$, puis active dans $ACw$.

*Ce qui est vrai à la place.* TOUR-E : les cellules de fenêtre (ici la jonction $ACw$, de représentants $AC$, $Aw$, $Cw$) et la **descente** de chaque représentant. Le représentant $AC$ a deux sites strictement intérieurs à sa plus petite boule : saut vers $\lbrace z, y \rbrace$, naissance de niveau $433/2$, dont l'ancêtre au niveau de $ACw$ est la bonne composante.

## 10. Multiplicités

Une entrée peut porter des positions répétées : un site $x$ a un poids $w(x) \geq 1$, $W = \sum_x w(x)$. Un **point** est une copie $(x, i)$, $1 \leq i \leq w(x)$. La sémantique visée est « un doublon est un point » : $D_k(y)$ compte les copies ; une $k$-partie est un ensemble de $k$ copies ; le niveau d'une partie, les enveloppes convexes et la séparabilité se lisent sur ses positions ; $p$ et $m$ deviennent des nombres de copies ; $q$ et $S^{*}$ restent comptés en positions.

**MULT-1 (ce qui reste vrai pour des copies).** [démontré par transfert] Avec ce dictionnaire, les énoncés suivants restent vrais et leurs preuves s'appliquent mot pour mot, parce qu'elles n'emploient que la plus petite boule d'un ensemble de positions et des échanges d'éléments d'une partie, jamais le fait que deux éléments d'une partie ont des positions distinctes : CAD-2, CAT-2, NERF-1 à NERF-4, LOC-1 à LOC-4, LOC-W (une partie portée par moins de $q$ positions est séparable), TOUR-A (i) à (iii), TOUR-B (0) à (2), TOUR-C, TOUR-D, TOUR-E, TOUR-F, TOUR-G (1) et (4), TOUR-J, TOUR-J2, INV-RACINE, INV-PLATEAU, PTS-CORE, PTS-COVER, PTS-ENC, PTS-LAM ; et, en remplaçant « nombre de sites » par « poids », GEN-D à GEN-G, avec l'admission $p + q \leq K + 1$ et les seuils $\theta_r$ en poids. CAT-1, CAT-3, GEN-Z et LOC-5 portent sur des positions et ne changent pas.

**MULT-2 (ce qui change).**

1. *Niveau nul.* Un site de poids $w$ est une boule de rayon nul ($I = \emptyset$, $U$ = ses $w$ copies, $q = 1$) : à chaque ordre $k \leq w$ elle porte une naissance de niveau 0, dont la composante est l'ensemble des $k$-parties de ses copies ; sa fenêtre est $\left[ 1, \min(K, w) \right]$. TOUR-A (iv) et TOUR-B (3) sont remplacés par cet énoncé ; dans TOUR-D une partie de niveau nul est un sommet de cette naissance.
2. *Coquilles régulières.* LOC-R et TOUR-G (2) ne valent que pour une coquille sans position répétée. Sinon la cellule se décide par le quotient sur les copies, et les ordres à événement d'une fenêtre ne forment **pas** un intervalle. Fixtures : `paire_31`, sites $(12,12,12)$ de poids 3 et $(14,12,12)$ de poids 1 : la boule diamétrale, de niveau 1, est une jonction à l'ordre 1, une continuation avec gain de couverture aux ordres 2 et 3, une naissance à l'ordre 4. `triangle_311`, sites $(0,0,0)$ de poids 3, $(6,0,0)$ et $(3,5,0)$ de poids 1 : la boule circonscrite, de niveau $289/25$, est sans événement à l'ordre 1 (inerte) et à l'ordre 3 (un seul morceau), une jonction aux ordres 2 et 4, une naissance à l'ordre 5.
3. *Euler.* Dans INV-EULER, $n \left[ k = 1 \right]$ devient le nombre de sites de poids $\geq k$, et $e_k(b)$ somme sur les parties $B$ de copies de $U$ ; la preuve est la même.
4. *Points.* Toutes les copies d'un site ont les mêmes dates et les mêmes propriétaires (échanger deux copies d'un site est un automorphisme de tous les objets).

**MULT-3 (ce qui reste à rédiger ou à décider).** Tant que ces points ne sont pas levés, la tour **refuse** une entrée pondérée (`unsupported_degeneracy`), et le produit applique ce refus au plus tôt.

1. Décision de l'utilisateur : « un doublon est un point », ou dédoublonnage déclaré à l'entrée.
2. Contre-lecture de MULT-1 énoncé par énoncé par un tiers. État : relecture de l'auteur de ce document ; force brute exacte sur 36 nuages à positions répétées (pièce `verif_enonces.py`, mode copies) ; l'oracle de `reference/` juge ses deux étages l'un contre l'autre sur des nuages à doublons ; aucun binaire ne confirme une tour pondérée (la v10 la refuse).
3. Quotient local et identité d'Euler en temps borné quand les poids sont grands : regroupement des parties de copies par ensemble de positions. Non rédigé ici.
4. Enregistrements du catalogue (poids, drapeau de coquille pondérée, boules de rayon nul) et contrat des entrées de points pour des copies : à écrire avec le module qui les sert.

## 11. Statuts, obligations, pièces

### 11.1 Table des statuts

Statuts au sens du registre racine. « Oracle » désigne la référence bornée de `reference/` (étage A, définition ; étage B, constructif ; porte `mhgp11_reference_fast` et ses mutants). « P1 » à « P4 » sont les pièces du § 11.3. « Porte à créer » : contrôle exigé du moteur, qui n'existe pas encore.

| Id | Énoncé | Statut | Preuve | Contrôle | Fixture d'égalité |
| --- | --- | --- | --- | --- | --- |
| CAD-S | séparation (Gordan) | `theorem_external` | classique | — | — |
| CAD-1 | plus petite boule englobante | `proved_here` | § 1.2 | P1 | — |
| CAD-2 | régions témoins | `proved_here` | § 1.3 | oracle A | — |
| CAD-3 | niveaux rationnels, ordre exact | `proved_here` | § 1.4, CAT-3 | porte à créer (module `num`) | `circle25_pair`, `double_collision` |
| CAT-1 | supports | `proved_here` | § 2.2 | P1 | `triangle_rectangle`, `carre` |
| CAT-2 | boule minimale d'une partie d'une boule | `proved_here` | § 2.2 | P1 | — |
| CAT-3 | centre et niveau exacts | `proved_here` | § 2.2 | P2 ; porte à créer (`num`) | `tetra_face_obtuse` |
| CAT-ADM | l'admission $p + q \leq K + 1$ suffit | `proved_here` | LOC-W, TOUR-B | oracle (B contre A) | `ligne_024` ; `triangle_rectangle` (non nécessaire) |
| GEN-D, GEN-DLOC | dominance | `proved_here` | § 3.1 | P2 | `dominance_face`, `dominance_coin` |
| GEN-L | restriction des listes | `proved_here` | § 3.1 | P2 | — |
| GEN-C | recensement local | `proved_here` | § 3.2 | P2 | `triangle_rectangle_interieur` |
| GEN-A, GEN-P | ajustement, partition des centres | `proved_here` | § 3.3 | P2 | `carre` (centres sur la face haute de la boîte englobante) |
| GEN-M | masques de dominance | `proved_here` | § 3.4 | P2 | — |
| GEN-Z | droite des équidistants | `proved_here` | § 3.4 | P2 | `droite_sommet`, `droite_arete` |
| GEN-S | survie du support canonique | `proved_here` | § 3.5 | P2 | `ligne_024`, `triangle_aigu_interieur`, `tetra_centre`, `tetra_face_obtuse` |
| GEN-E | émission unique | `proved_here` | § 3.5 | P2 | `carre`, `cube` |
| GEN-G | théorème du générateur | `proved_here` | § 3.6 | P2 ; porte à créer (catalogue contre oracle) ; INV-RESTR | toutes les précédentes |
| GEN-F | listes inhérentes | `proved_here` | § 3.7 | — | `sphere_24` |
| GEN-COUT (1) | sortie non linéaire dans le pire cas | `proved_here` | § 3.7 | oracle B (catalogue par force brute) | `arcs_enlaces_16` |
| GEN-COUT (coût) | linéarité en la sortie, loi de la marge de feuille | `experimental_target` | aucune | portes d'échelle à créer | `amas_coins_14` (v10) |
| NERF-1 à NERF-4 | nerf, coupes fermée et ouverte, naturalité, couverture | `proved_here` (coupe fermée : aussi `theorem_external`, thèse th. 2 et prop. 5) | § 4 | oracle A (par construction) ; P1 | — |
| LOC-1, LOC-2 | échanges, structure stricte | `proved_here` | § 5.2 | P1 | — |
| LOC-3 | morceaux (surjection, raffinement exhaustif) | `proved_here` | § 5.3 | P1 ; oracle B | `morceaux_non_injectifs` |
| LOC-4, LOC-5 | quotient par famille couvrante | `proved_here` | § 5.3 | porte à créer (quotient contre énumération brute) | `carre`, `cube`, `cercle_12` |
| LOC-W | fenêtre de rang | `proved_here` | § 5.4 | P1 | `triangle_scalene` (boule du triangle : inerte à l'ordre $p + q - 2 = 1$, jonction à l'ordre $p + q - 1 = 2$) |
| LOC-R | coquille régulière | `proved_here` | § 5.4 | P1 | — |
| LOC-GEO | lecture géométrique des morceaux | `proved_here` | § 5.4 | — | — |
| TOUR-A | arbre de fusion | `proved_here` | § 6.1 | oracle A | — |
| TOUR-B | classification des événements | `proved_here` | § 6.2 | P1 ; oracle (B contre A) | `triangle_rectangle`, `carre`, `morceaux_non_injectifs` |
| TOUR-C | plateaux atomiques | `proved_here` | § 6.3 | P1 ; mutants de la référence | `carre` (ordre 1), `ligne_024` (ordre 1) |
| TOUR-D | descente | `proved_here` | § 6.4 | P1 ; INV-NEUTRE | `ligne_024` (terminal non unique), `these_th5_plan` (saut) |
| TOUR-E | exactitude relative au catalogue | `conditional_theorem` (relatif à H2) | § 6.5 | oracle (B contre A) ; porte à créer (tour contre oracle, arbre étiqueté) | `triangle_scalene` (catalogue amputé) |
| TOUR-F | verticales à la coupe fermée | `proved_here` | § 6.6 | oracle (B contre A) | `carre` (ordres 2 et 3) |
| TOUR-G | couvertures | `proved_here` ; (3) `false_in_general` pour « union des naissances » | § 6.7 | P1 | `gain_de_couverture`, `internal_k3` |
| TOUR-J, TOUR-J2 | construction sans lots | `proved_here` | § 6.8 | P3 | `carre` (ordre 1) |
| INV-EULER | identité d'Euler | `proved_here` ; « Euler certifie le catalogue » : `false_in_general` | § 7.1 | P1 ; porte d'échelle à créer | `carre`, `triangle_scalene`, `euler_compensation` |
| INV-EMST | ordre 1 = lien simple | `proved_here` | § 7.2 | porte d'échelle à créer (scikit-learn) | `carre` (ordre 1) |
| INV-RESTR, INV-RACINE, INV-PLATEAU, INV-VIVANT, INV-NEUTRE | invariants linéaires | `proved_here` | § 7.3 | portes d'échelle à créer | — |
| PTS-CORE, PTS-COVER | entrées `core` et `cover` | `proved_here` | § 8.2 | P1 ; oracle | `ligne_024`, `cover_tie_n4` |
| PTS-ENC, PTS-LAM, PTS-ANC, PTS-NP, PTS-MONO, PTS-K1 | encadrement, laminarité, monotonies | `proved_here` | § 8.2 | P1 (PTS-ENC) | `ligne_012` |
| PTS-STAB | stabilité de `core`, constante 2 | `proved_here` | § 8.2 | — | `stab_x`, `stab_y` |
| PTS-MR | encadrement par l'atteignabilité mutuelle | `proved_here` | § 8.2 | — | — |
| PTS-DEP | pas de propriétaire unique équivariant | `proved_here` | § 8.3 | — | `ligne_024` |
| PTS-X1 à PTS-X5 | stabilité, verticalité, respect du cœur de `cover` ; partition des amas discrets | `false_in_general` | § 8.3 | oracle A | `f2_gauche`, `f2_droite`, `ligne_01269`, `firstcov_k3_n6`, `internal_k3`, `ligne_024` |
| THESE-5 | th. 5 et prop. 6 du manuscrit | `false_in_general` | § 9 | P4 ; oracle A | `these_th5_plan`, `e5` |
| MULT-1, MULT-2 | énoncés pour des copies | `proved_here` par transfert, contre-lecture exigée avant usage | § 10 | P1 (mode copies) ; oracle sur nuages à doublons | `paire_31`, `triangle_311` |

### 11.2 Obligations restantes

1. **OBL-1.** Contre-lecture par un tiers de toutes les preuves de ce document, en priorité LOC-3, LOC-5, TOUR-B, TOUR-D, TOUR-E, TOUR-G (1) et INV-EULER ; inscription au registre racine après contre-lecture.
2. **OBL-2.** Exactitude des prédicats : chaque expression de CAT-3, GEN-DLOC, GEN-Z et du test « centre dans le pavé » doit porter son budget de bits (architecture § 3) ; aucun énoncé d'ici ne vaut pour un prédicat approché.
3. **OBL-3.** Grandes coquilles : limite unique et déclarée du nombre de sites d'une coquille, commune au catalogue et à la tour ; borne du nombre de membres de la famille de LOC-5 et du coût du quotient ; juge indépendant au-delà de 14 sites (l'oracle de définition s'arrête là).
4. **OBL-4.** Coût du générateur : aucune borne du nombre de nœuds, de feuilles ni de recensements ; aucune loi en fonction de la marge entre taille de feuille et $K$. Cible expérimentale, sous budget de nœuds.
5. **OBL-5.** Longueur des descentes : aucune borne autre que le nombre de niveaux.
6. **OBL-6.** Taille de la sortie par site sur les familles du contrat : mesurée, non bornée (GEN-COUT (1) interdit toute borne linéaire générale).
7. **OBL-7.** Complétude à l'échelle : INV-EULER est nécessaire, pas suffisant ; restent à écrire le refus d'une boule de fenêtre absente (TOUR-E, portée) et un juge d'échantillon par recensement brut.
8. **OBL-8.** Hiérarchie de points : aucune continuité démontrée hors `core` ; la convention de propriétaire unique de `cover` est une décision de produit (PTS-DEP).
9. **OBL-9.** Multiplicités : MULT-3.
10. **OBL-10.** Signaler à l'auteur de la thèse l'énoncé à corriger (th. 5, prop. 6) et proposer TOUR-E comme énoncé de remplacement.

### 11.3 Pièces

Scripts exacts (fractions), hors dépôt, sous `build/v11-persist/mathematiques/pieces/`. Ils ne prouvent rien : ils ont cherché un contre-exemple à chaque énoncé, sans en trouver.

| Pièce | Ce qu'elle confronte à la définition | Volume du 2 octobre 2026 |
| --- | --- | --- |
| P1 `verif_enonces.py` | CAT-1, CAT-2, LOC-1 à LOC-3, LOC-W, LOC-R, TOUR-B, TOUR-C, TOUR-D, TOUR-G, PTS-CORE, PTS-COVER, PTS-ENC, INV-EULER, INV-RACINE, par force brute sur $\Gamma_k$, sans aucun import du dépôt | 67 nuages de sites distincts (13 fixtures, 54 nuages de neuf familles dégénérées ou génériques, 4 à 8 sites, tous les ordres) et 36 nuages à positions répétées : 0 écart |
| P2 `verif_generateur.py` | modèle exécutable du générateur (GEN-D à GEN-G, poids compris) contre le catalogue de la définition ; GEN-DLOC contre les coins ; GEN-Z contre l'intersection exacte | 33 nuages, 396 exécutions, 2 299 boules attendues : 0 écart |
| P3 `verif_cartesienne.py` | TOUR-J contre TOUR-C sur des hypergraphes à rangs répétés | 3 000 essais, 5 564 nœuds dont 3 893 à trois enfants ou plus : 0 écart |
| P4 `these_th5.py` | THESE-5 par l'oracle de définition | désaccord permanent confirmé ; E5 : désaccord à un niveau |
| `fixtures_oracle.py`, `fixtures_catalogue.py` | valeurs exactes des fixtures, lues dans la référence | — |

### 11.4 Fixtures citées

Coordonnées entières, dans le domaine du profil à 18 bits. La liste complète à graver dans les portes, avec l'attendu exact de chaque fixture, est tenue à part (`FIXTURES.md` des pièces).

| Fixture | Sites |
| --- | --- |
| `ligne_024`, `ligne_012`, `ligne_01269` | sur l'axe des $x$ : abscisses $(0, 2, 4)$ ; $(0, 1, 2)$ ; $(0, 1, 2, 6, 9)$ |
| `f2_gauche`, `f2_droite` | abscisses $(0, 999, 2000)$ ; $(0, 1001, 2000)$ |
| `stab_x`, `stab_y` | abscisses $(1, 11)$ ; $(0, 12)$ |
| `triangle_rectangle` ; `triangle_rectangle_interieur` | $(0,0,0)$, $(3,0,0)$, $(0,4,0)$ ; avec en plus $(1,1,0)$ |
| `triangle_scalene` | $(0,0,0)$, $(6,0,0)$, $(2,5,0)$ |
| `triangle_aigu_interieur` | $(0,0,0)$, $(6,0,0)$, $(3,5,0)$, $(3,2,0)$ |
| `carre` | $(0,0,0)$, $(2,0,0)$, $(0,2,0)$, $(2,2,0)$ |
| `cube` | les huit points de $\lbrace 0, 2 \rbrace^{3}$ |
| `tetra_centre` | $(0,0,0)$, $(2,2,0)$, $(2,0,2)$, $(0,2,2)$, $(1,1,1)$ |
| `tetra_face_obtuse` | $(0,4,4)$, $(1,2,0)$, $(1,2,4)$, $(4,4,0)$ |
| `circle25_pair` | $(15,10,3)$, $(7,14,3)$, $(7,6,3)$, $(40,40,40)$, $(50,40,40)$ |
| `double_collision` | $(0,0,0)$, $(78404,0,0)$, $(39202,55440,0)$, $(150000,150000,150000)$, $(198046,200290,195586)$ : niveaux exacts $1728896403$ et $1728896403 + 1/768398400$, de même valeur en binaire64 |
| `cercle_12`, `sphere_24` | les 12 points entiers de $x^{2} + y^{2} = 25$, translatés de $(5,5,0)$ ; les 24 points entiers de $x^{2} + y^{2} + z^{2} = 5$, translatés de $(2,2,2)$ |
| `amas_coins_14` | $(1,7,1)$, $(3,1,5)$, $(3,1,7)$, $(3,3,3)$, $(4,5,0)$, $(5,5,0)$, $(6,0,6)$, $(262137,262136,6)$, $(262138,262140,5)$, $(262138,262141,4)$, $(262139,262136,0)$, $(262140,262140,4)$, $(262141,262140,3)$, $(262143,262140,5)$ |
| `morceaux_non_injectifs` | $(0,0,0)$, $(2,0,0)$, $(4,0,0)$, $(2,3,0)$ |
| `gain_de_couverture` | $(8,9,0)$, $(5,10,0)$, $(2,9,0)$, $(5,0,0)$ |
| `euler_compensation` | $(0,5,0)$, $(8,9,0)$, $(8,1,0)$, $(35,5,0)$, $(45,5,0)$ |
| `firstcov_k3_n6` | $(2,4,4)$, $(2,8,5)$, $(2,9,1)$, $(3,7,0)$, $(6,9,6)$, $(8,10,7)$ |
| `internal_k3` | $(15,4,0)$, $(5,4,0)$, $(7,8,0)$, $(7,0,0)$, $(1,4,0)$, $(0,4,1)$ |
| `cover_tie_n4` | $(7,6,10)$, $(6,7,10)$, $(10,5,10)$, $(5,10,10)$ |
| `these_th5_plan` | $(0,100,0)$, $(200,100,0)$, $(101,10,0)$, $(130,15,0)$, $(103,400,0)$ |
| `e5` | $(0,0,7)$, $(0,9,6)$, $(1,4,0)$, $(0,0,1)$, $(4,1,2)$ |
| `arcs_enlaces_16` | $(130000,0,0)$, $(129970,2786,0)$, $(129881,5570,0)$, $(129731,8351,0)$, $(129523,11129,0)$, $(129255,13902,0)$, $(128927,16668,0)$, $(128540,19427,0)$, $(30,0,2786)$, $(119,0,5570)$, $(269,0,8351)$, $(477,0,11129)$, $(745,0,13902)$, $(1073,0,16668)$, $(1460,0,19427)$, $(1906,0,22177)$ |
| `paire_31`, `triangle_311` | poids entre crochets : $(12,12,12)$ [3], $(14,12,12)$ [1] ; $(0,0,0)$ [3], $(6,0,0)$ [1], $(3,5,0)$ [1] |
| `dominance_face`, `dominance_coin` | prédicat GEN-DLOC : pavé $\left[ 10, 14 \right] \times \left[ 0, 4 \right]^{2}$, $z = (6,2,2)$ ne domine pas $x = (22,2,2)$ (égalité sur une face) et domine $x = (23,2,2)$ ; pavé $\left[ 0, 4 \right]^{3}$, $z = (0,0,0)$ ne domine pas $x = (8,8,8)$ (égalité en un coin) et domine $x = (8,8,9)$ |
| `droite_sommet`, `droite_arete` | prédicat GEN-Z : sites $(0,0,1)$, $(4,0,1)$, $(2,2,3)$, la droite touche le pavé $\left[ 2, 3 \right] \times \left[ 1, 2 \right] \times \left[ 1, 2 \right]$ en son seul sommet $(2,1,1)$ et manque $\left[ 2, 3 \right] \times \left[ 1, 2 \right] \times \left[ 2, 3 \right]$ ; sites $(0,0,0)$, $(2,0,0)$, $(0,2,0)$, la droite longe une arête de $\left[ 1, 2 \right]^{2} \times \left[ 0, 1 \right]$ et manque $\left[ 1, 2 \right] \times \left[ 2, 3 \right] \times \left[ 0, 1 \right]$ |
