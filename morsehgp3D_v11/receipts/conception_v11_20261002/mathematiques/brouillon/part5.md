
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
