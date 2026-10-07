
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
