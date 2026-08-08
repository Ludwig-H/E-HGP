# RNG, Jung, cliques et niveaux peu profonds pour les supports 3D

> [!IMPORTANT]
> L'autorité mathématique reste en Phase 15, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=exact_rational_obstruction_and_shallow_arrangement_theorem`. La sous-porte `P15-HOCUDA-P0` autorise uniquement un prototype GPU de proposition et de mesure; elle n'ouvre ni source complète, ni décision scientifique, ni réducteur, ni SLO. Ce document ne ferme aucune phase et maintient `public_status=not_claimed`.

## 1. Verdict

Il faut distinguer deux graphes. Le RNG brut n'est pas complet, et la cascade bornée arité deux vers trois puis quatre, avec les facteurs de Jung $\alpha_2$ puis $\alpha_3$ appliqués à la plus grande arête incidente, ne le rend pas complet non plus : la fixture régulière ci-dessous le réfute exactement au rang fermé 11, même avec la symétrisation au maximum des deux extrémités. En revanche, le graphe local $G_\tau$ déjà défini dans le dépôt est un surgraphe complet dès que son rayon par sommet $\tau(p)$ majore une quantité de rang certifiée. Cet objet reste sparse dans le régime favorable et peut être construit sans mosaïque de Delaunay d'ordre supérieur; le verrou est de certifier $\tau$, pas de multiplier une échelle RNG.

Jung fournit néanmoins le bon changement de dimension. Une paire diamètre fixe un disque dans son plan médiateur. Chaque troisième point y définit une droite orientée, coupée en une corde. Un tétraèdre de support quatre correspond à l'intersection de deux de ces droites. Surtout, le nombre de points strictement intérieurs à sa sphère est exactement la profondeur de cette intersection dans l'arrangement des demi-plans orientés. Au rang fermé maximal $s_{\max}=11$, il suffit donc d'énumérer les sommets de profondeurs 0 à 7, et non toutes les paires de cordes.

Ce résultat est localement sensible à $K$. Il ne donne pas encore une complexité globale en $O(n\,\mathrm{poly}(K))$ : il faut aussi borner ou mesurer le nombre de paires diamètre candidates et la somme des voisinages locaux. Le pire cas de ces deux quantités reste polynomialement dense.

Dans les formules ci-dessous, $s_{\max}$ désigne le rang fermé maximal. La convention du dépôt est $s_{\max}=K_{\mathrm{eff}}+1$ : le rang fermé 11 mesuré historiquement correspond donc à `requested_order=10`. Si « $K=11$ » désigne littéralement `requested_order=11`, les constantes huit et sept ci-dessous deviennent respectivement neuf et huit.

## 2. Obstruction exacte au RNG de points

### 2.1 Convention

Pour un entier $q\geq0$, le RNG d'ordre $q$ conserve l'arête $xy$ lorsque sa lune ouverte contient au plus $q$ autres points :

$$L^{\circ}(x,y)=\left\lbrace z:\lVert z-x\rVert<\lVert x-y\rVert\ \text{et}\ \lVert z-y\rVert<\lVert x-y\rVert\right\rbrace.$$

Le RNG classique est le cas $q=0$. Une convention employant les inégalités fermées changerait les cas d'égalité, pas la construction strictement séparée ci-dessous.

### 2.2 Théorème négatif

> **Théorème 1.** Pour tout entier fini $q$, il existe un nuage rationnel de $\mathbb{R}^{3}$ contenant un support $U$ de cardinal quatre, affinement indépendant, minimal bien centré et de rang fermé 11, tel que le RNG d'ordre $q$ induit sur les sommets de $U$ n'est pas une clique. Par conséquent, aucune fonction finie $q=f(11)$ ne rend la génération « cliques du RNG d'ordre $q$ » complète pour les supports de rang fermé 11.

*Preuve.* Prenons les quatre points

$$A=(0,0,0),\quad B=(-2,-2,-1),\quad C=(-2,1,0),\quad D=(0,-1,-1).$$

Le déterminant affine vaut 4. Leur circumball a pour centre $c=(-3/2,-1/2,-1/2)$, rayon carré $r^{2}=11/4$ et coordonnées barycentriques strictement positives $(1/8,3/8,3/8,1/8)$. Le tétraèdre est donc un support minimal bien centré. Ajoutons $c$ et les six points $c\pm(1/16,0,0)$, $c\pm(0,1/16,0)$, $c\pm(0,0,1/16)$. Ils sont strictement intérieurs et donnent un rang fermé égal à $4+7=11$.

L'arête $AD$ a une longueur carrée égale à 2. Soit $L$ une puissance de deux telle que $L\geq16(q+2)$ et, pour $1\leq i\leq q+1$, posons $t_i=1+i/L$ et $z_i=(t_i,-1/2,-1/2)$. Alors

$$\lVert z_i-A\rVert^{2}=\lVert z_i-D\rVert^{2}=t_i^{2}+\frac{1}{2}<2,$$

car $t_i<17/16$. Les $q+1$ points $z_i$ appartiennent donc strictement à $L^{\circ}(A,D)$. En revanche,

$$\lVert z_i-c\rVert^{2}=(t_i+3/2)^{2}>25/4>11/4,$$

donc ils restent tous strictement hors de la circumball de $U$. Ils ne changent pas son rang fermé, mais la lune contient plus de $q$ points et l'arête $AD$ disparaît. Le support n'est plus une clique. Toutes les coordonnées sont dyadiques. $\square$

La fixture permanente [`rng_order_q_rank11_support4_counterexample.json`](../../tests/fixtures/regressions/rng_order_q_rank11_support4_counterexample.json) fixe le cas produit $q=11$. Le checker [`check_rng_jung_clique_obstruction.py`](../../tools/check_rng_jung_clique_obstruction.py) recalcule toute la géométrie avec `Fraction` et instancie la famille pour $q=0,1,7,11,1000$.

Une variante symétrique part du tétraèdre régulier $(1,1,1)$, $(1,-1,-1)$, $(-1,1,-1)$, $(-1,-1,1)$. Chaque arête a longueur carrée 8 et son milieu $m$ vérifie $\lVert m\rVert^{2}=1$. Pour $L$ puissance de deux avec $L\geq16(q+2)$, les $q+1$ points $z_i=(2+i/L)m$ vérifient $\lVert z_i\rVert^{2}>4>3$ et $\lVert z_i-x\rVert^{2}=\lVert z_i-y\rVert^{2}=(1+i/L)^{2}+2<8$ pour les extrémités $x,y$. Ils sont donc hors de la circumball du tétraèdre, dans la lune ouverte de l'arête et même dans sa boule diamétrale. En appliquant la construction aux six arêtes, on les supprime toutes du RNG et on rend leurs rangs diamétraux arbitrairement grands, tout en gardant sept témoins strictement intérieurs et le rang fermé 11 du support quatre. Le checker exécute exactement cette famille pour les cinq ordres annoncés.

Cette variante ferme deux échappatoires : ni une 4-clique, ni une paire diamètre tirée seulement du catalogue des supports deux de rang utile ne fournit une source complète. L'arrangement construit plus bas est exact une fois son ancre $pq$ connue; il ne résout pas à lui seul l'énumération complète et sparse des ancres.

### 2.3 L'obstruction apparaît déjà à l'arité trois

Dans un triangle dont la plus longue arête est unique, le troisième sommet appartient à la lune ouverte de cette arête, puisque les deux autres côtés sont strictement plus courts. Le RNG classique ne contient donc pas le triangle comme 3-clique. Il ne faut pas étendre cet énoncé aux triangles équilatéraux sous la convention ouverte.

Cette obstruction est différente de la non-hérédité des rangs déjà enregistrée : un triangle de rang faible peut avoir tous ses côtés de rang arbitrairement élevé. Les deux résultats interdisent respectivement le critère de lune d'ordre fixé et le catalogue de paires de rang fixé.

### 2.4 Le graphe RNG épaissi par Jung

Formalisons la version plus forte proposée. Soit $R_q$ le RNG d'ordre $q$ et $\lambda_q(x)$ la longueur de sa plus grande arête incidente à $x$. La règle one-shot la plus généreuse ajoute $xy$ dès que

$$\lVert x-y\rVert\leq\alpha\max(\lambda_q(x),\lambda_q(y)).$$

Une règle mutuelle employant le minimum produit un sous-graphe et ne peut donc pas mieux couvrir. Dans une cascade arité deux vers trois puis quatre, on recalcule la plus grande arête après le premier épaississement. Avec la règle au maximum, il faut construire le graphe entier à chaque snapshot : une grande échelle portée par l'autre extrémité peut se transmettre, et une multiplication portant seulement sur l'échelle du sommet étudié ne constitue pas une preuve.

Le nuage régulier généré par le checker au cas $q=11$ possède, pour chacun des quatre sommets du support, la valeur exacte

$$\lambda_{11}^{2}=\frac{801}{256},\qquad D^{2}=8.$$

Le checker construit les 1 003 arêtes du RNG d'ordre 11 sur tout le nuage, puis applique simultanément la règle au maximum des extrémités. Un étage tétraédrique ne produit aucune arête du support. La cascade produit exacte $\alpha_2^{2}=4/3$ puis $\alpha_3^{2}=3/2$ passe successivement à 2 053 puis 2 965 arêtes globales, mais laisse encore absentes les six arêtes du support de carré 8. Cette conclusion est donc un calcul global exact, pas l'extrapolation de la seule valeur $\lambda_{11}$.

La portée de ce contre-exemple est bornée et le checker la rend explicite. Si l'on répète indéfiniment la règle au maximum avec $\alpha_3$, les grandes échelles des témoins extérieurs se transmettent : cette fixture récupère les six arêtes au deuxième étage tétraédrique et atteint ensuite un point fixe qui contient la 4-clique. À l'inverse, la règle mutuelle au minimum atteint ici un point fixe incomplet. Le dépôt ne transforme donc ni l'un ni l'autre fait en théorème universel : la cascade produit à deux étages est réfutée; une fermeture au maximum jusqu'au point fixe n'est pas réfutée par cette fixture, mais elle n'a aucune borne de parcimonie ni preuve de complétude enregistrée et peut tendre vers le graphe complet.

Jung borne le rayon d'une miniboule à partir du diamètre d'un ensemble déjà connu. Il ne fournit pas la minoration inverse $\lambda_q(x)\geq D/\alpha$ nécessaire pour que l'échelle incidente du RNG atteigne une arête d'un support inconnu. C'est précisément l'implication manquante.

Répéter artificiellement la multiplication jusqu'à couvrir toute distance ne répare pas cette preuve gratuitement : si l'échelle grandit même sans nouvelle arête, tout sommet non isolé finit par couvrir le diamètre du nuage et le graphe devient complet. Si elle se propage par la règle au maximum des extrémités réellement ajoutées, la fixture montre précisément que la portée peut grossir après la cascade utile et récupérer le support; ce comportement ne fournit cependant aucune borne sparse en $n$ ou en $K$. Si elle emploie la règle mutuelle au minimum, la fixture se stabilise avant le support. Le choix oppose donc couverture empirique et maîtrise de la densité; aucun des deux n'est aujourd'hui une autorité exacte.

### 2.5 Le surgraphe local qui possède une preuve

La bonne abstraction de graphe est celle de [`GERMINATION_LOCALE_SUPPORTS_3_4.md`](GERMINATION_LOCALE_SUPPORTS_3_4.md). Pour chaque point $p$, soit $R(p)$ le plus grand rayon d'une boule de rang utile, tangente en $p$ et dont le centre respecte le domaine admissible. Si $\tau(p)\geq2R(p)$ est une majoration certifiée, alors

$$p\sim q\quad\Longleftrightarrow\quad\lVert p-q\rVert\leq\min(\tau(p),\tau(q))$$

contient toutes les arêtes de tout support accepté. Ce $G_\tau$ donne exactement l'objet sparse sauf au pire cas recherché par l'utilisateur : un CSR de points et des voisinages locaux, sans cellule, coface, incidence ou mosaïque de Delaunay d'ordre supérieur. Mais la plus grande arête RNG incidente n'est pas une majoration de $2R(p)$; la fixture ci-dessus donne l'inégalité opposée. Le RNG épaissi peut donc proposer les premières arêtes de $G_\tau$, jamais certifier à lui seul que les autres sont inutiles.

## 3. Ce que Jung autorise, et ce qu'il n'autorise pas

Le théorème de Jung donne, en dimension trois, la constante optimale

$$r_{\mathrm{mb}}\leq\sqrt{\frac{3}{8}}D.$$

Pour un support minimal bien centré, la miniboule est sa circumball, donc cette borne s'applique à son rayon. Elle est atteinte par le tétraèdre régulier. Elle ne borne ni le nombre de points dans une lune, ni le rang d'une sphère : les témoins du Théorème 1 sont précisément hors de la circumball et échappent à toute information fournie par Jung.

Une clique métrique au niveau des arêtes est un simplexe de Vietoris--Rips. Le sandwich euclidien issu de Jung donne une approximation de Čech, pas l'égalité des niveaux. Sur le tétraèdre régulier ci-dessus, chaque arête a longueur carrée 8, la naissance de Čech des arêtes vaut 2, celle des faces vaut $8/3$ et celle du tétraèdre vaut 3. La clique apparaît donc au niveau carré 2 alors que le tétraèdre naît au niveau carré 3 : le facteur optimal vaut

$$\alpha_{3}^{2}=\frac{3}{2},\qquad \alpha_{3}=\sqrt{\frac{3}{2}}.$$

Il ne faut pas confondre $\alpha_{3}^{2}=3/2$, facteur Rips--Čech sur les niveaux carrés, avec $3/8$, carré de la constante rayon--diamètre de Jung.

## 4. Le substitut exact : arrangement ancré par une paire diamètre

### 4.1 Disque de centres

Fixons une paire distincte $p,q$, posons $D=\lVert p-q\rVert$, $M=(p+q)/2$ et $e=(q-p)/D$. Tout centre d'une sphère passant par $p$ et $q$ s'écrit $c=M+v$, avec $v\cdot e=0$, et son rayon vérifie $r^{2}=D^{2}/4+\lVert v\rVert^{2}$.

Si $p,q$ est une paire diamètre d'un support tétraédrique minimal bien centré, Jung impose $r^{2}\leq3D^{2}/8$. Le centre appartient donc au disque fermé

$$J_{pq}=\left\lbrace M+v:v\cdot e=0\ \text{et}\ \lVert v\rVert^{2}\leq\frac{D^{2}}{8}\right\rbrace.$$

### 4.2 Une droite orientée par point

Pour chaque $x\notin\{p,q\}$, définissons dans le plan vectoriel $e^{\perp}$

$$h_x(v)=2v\cdot(x-M)-\left(\lVert x-M\rVert^{2}-\frac{D^{2}}{4}\right).$$

Une expansion directe donne l'identité de puissance

$$h_x(v)=r^{2}-\lVert x-(M+v)\rVert^{2}.$$

Ainsi $h_x(v)>0$, $h_x(v)=0$ et $h_x(v)<0$ signifient respectivement que $x$ est strictement dans la sphère, sur son bord et strictement dehors. L'équation $h_x(v)=0$ est une droite; son intersection avec $J_{pq}$ est la corde de Jung de $x$ lorsqu'elle est non vide.

### 4.3 Des cordes au support quatre

> **Théorème 2.** Supposons $p,q,z,w$ affinement indépendants. Leur centre circonscrit existe de façon unique. Il appartient à $J_{pq}$ si et seulement si les droites $h_z=0$ et $h_w=0$ s'intersectent dans ce disque. Si, de plus, les six distances du quadruplet sont au plus $D$, alors $p,q$ en est une paire diamètre. Le quadruplet est un support utile exactement lorsque les prédicats terminaux de bon centrage, de shell et de rang l'acceptent.

*Preuve.* Les centres équidistants de $p,q,z$ forment la droite $h_z=0$ dans le plan médiateur de $p,q$; la même affirmation vaut pour $w$. L'indépendance affine rend leurs deux droites non parallèles et leur intersection est l'unique centre équidistant des quatre points. L'appartenance au disque est équivalente à $r^{2}\leq3D^{2}/8$. Les tests de diamètre, de bon centrage et de rang portent ensuite exactement leurs définitions. $\square$

Le graphe pertinent n'est donc pas un graphe global sur les points. C'est, pour chaque ancre $pq$, le graphe local d'intersection des cordes. L'énumération de toutes ses arêtes resterait quadratique. La condition de rang donne la réduction supplémentaire décisive.

### 4.4 Le rang est une profondeur de demi-plans

Orientons chaque droite par le demi-plan $H_x^{+}=\{v:h_x(v)>0\}$. Pour un point $v$ qui n'est sur aucune droite autre que celles de $z$ et $w$, posons

$$\delta_{pq}(v)=\#\left\lbrace x\in P\setminus\{p,q,z,w\}:h_x(v)>0\right\rbrace.$$

> **Théorème 3.** En position générale, au sommet $v=(h_z=0)\cap(h_w=0)$, le rang fermé de la sphère passant par $p,q,z,w$ vaut $4+\delta_{pq}(v)$. Par conséquent, tout support quatre minimal bien centré de rang fermé au plus $s_{\max}$ apparaît parmi les sommets de l'arrangement de profondeur stricte au plus $\kappa=s_{\max}-4$ situés dans $J_{pq}$ pour au moins une de ses paires diamètre.

*Preuve.* Les quatre points $p,q,z,w$ sont sur le bord. Par l'identité de puissance, chaque autre point est strictement intérieur si et seulement si son demi-plan orienté contient $v$. La formule de rang et la borne de profondeur suivent. Tout ensemble fini possède une paire diamètre et Jung place son centre dans le disque correspondant. $\square$

Pour $s_{\max}=11$, la profondeur maximale vaut seulement

$$\kappa=11-4=7.$$

La fixture permanente recalcule également ce fait : son ancre diamètre unique est $BC$, son centre appartient à $J_{BC}$, sa profondeur stricte vaut 7 et son rang fermé vaut 11.

Sans position générale, plusieurs droites peuvent être concourantes. Si $t(v)$ droites passent par le même centre, $c_e$ points sont intérieurs dans tout le disque et $d(v)$ autres demi-plans le contiennent strictement, alors

$$\mathrm{rang}_{\text{fermé}}(v)=2+c_e+d(v)+t(v).$$

Le rang fermé 11 impose donc $t(v)\leq9-c_e$. La preuve par droite reste utile : en comptant une position concurrente une seule fois sur chaque droite, $\sum_v t(v)\leq2m_e(\kappa_e+1)$. Comme $\binom{t}{2}\leq t(T-1)/2$ pour $t\leq T=9-c_e$, le nombre total de quadruplets développables vérifie

$$\sum_v\binom{t(v)}{2}\leq(T-1)m_e(\kappa_e+1).$$

Il reste donc linéaire en $m_e$ à rang fixé, à condition de grouper chaque concurrence en un batch exact et de rejeter avant développement lorsque son shell dépasse le rang. Une implémentation ne peut ni choisir arbitrairement deux droites, ni perturber la concurrence en intersections artificielles; elle doit appliquer la règle de carrier déjà enregistrée ou rendre `unsupported_degeneracy` pour les formes non couvertes.

## 5. Complexité en $n$ et en $K$

### 5.1 Variables qui gouvernent réellement le coût

Soit $\mathcal{A}$ l'ensemble des paires diamètre candidates produites par une autorité exacte, $a=\lvert\mathcal{A}\rvert$. Pour $e=pq$, soit $W_e$ l'ensemble local des points qui peuvent être intérieurs ou sur le shell d'une sphère dont le centre appartient à $J_{pq}$. Parmi eux, $c_e$ points sont strictement intérieurs pour tout le disque, tandis que $m_e$ autres ont une droite frontière qui rencontre le disque et participent à son arrangement. Les points dont le demi-plan positif manque tout le disque sont sans effet. Posons

$$M=\sum_{e\in\mathcal{A}}m_e,\qquad C=\sum_{e\in\mathcal{A}}c_e.$$

Jung borne aussi le range-report. Toute sphère utile ancrée par $pq$ est contenue dans la boule centrée en $M$ de rayon $\cos(15^{\circ})D$. Un point plus éloigné ne peut être ni intérieur ni sur le shell d'un centre de $J_{pq}$. Le LBVH peut donc proposer $W_e$ sans matrice paire--point globale.

Les grandeurs $a$, $M$ et $C$, pas seulement $n$, décident la scalabilité. Elles doivent être publiées par famille, avec histogrammes et maxima; une moyenne seule masque les ancres lourdes. Si $c_e>s_{\max}-4$, l'ancre est rejetée avant tout arrangement.

### 5.2 Borne combinatoire locale

Pour l'ancre $e$, le budget de profondeur variable est $\kappa_e=s_{\max}-4-c_e$. La complexité cumulative $\Theta(m\kappa)$ des premiers niveaux d'un arrangement de demi-plans est connue par la méthode de Clarkson--Shor et rappelée par Chan. La constante utile ici possède une preuve élémentaire que nous reproduisons. Sur une droite frontière $\ell$, soient $u,v$ les intersections extrêmes de profondeur au plus $\kappa_e$. Toute autre droite coupant strictement l'intervalle $[u,v]$ place exactement l'un de ses deux bouts dans son demi-plan positif. Si cet intervalle contenait plus de $2\kappa_e$ intersections strictement entre ses extrémités, les profondeurs de $u$ et $v$ auraient donc une somme supérieure à $2\kappa_e$, ce qui est impossible. Chaque droite porte au plus $2\kappa_e+2$ sommets de profondeur au plus $\kappa_e$. Chaque sommet appartenant à deux droites en position générale, l'arrangement entier en possède au plus $m_e(\kappa_e+1)$. Le disque de Jung ne peut qu'en retirer. Ainsi, pour $\kappa_e\geq0$,

$$Z_e\leq m_e(\kappa_e+1)=m_e(s_{\max}-3-c_e).$$

Pour $s_{\max}=11$ et $c_e=0$, donc $\kappa_e=7$, une ancre possède au plus

$$Z_e\leq8m_e$$

sommets admissibles par leur profondeur, avant même les rejets diamètre et bon centrage. C'est la vraie rupture asymptotique locale face aux $\binom{m_e}{2}$ paires de cordes de la boucle actuelle. La borne est serrée au pire cas; la constante huit ne doit donc pas être présentée comme un simple artefact de preuve.

La borne cumulative $m_e(\kappa_e+1)$ est celle qui correspond directement au produit : tous les rangs fermés jusqu'à $s_{\max}$ sont requis. Dans la convention `requested_order`, elle vaut $m_e(K_{\mathrm{eff}}-2-c_e)$ lorsqu'elle est positive. Elle est `proved_here`; la littérature citée fournit le contexte asymptotique, pas une autorité logicielle.

Les résultats de construction des premiers niveaux donnent aussi une faisabilité algorithmique théorique : sous position générale, une construction incrémentale randomisée de type Las Vegas atteint un coût espéré $O(m_e\log m_e+m_e(\kappa_e+1))$ pour les faces peu profondes, à un facteur de fonction d'Ackermann près selon la variante déterministe. Aronov--Har-Peled et Agarwal--de Berg--Matoušek--Schwarzkopf donnent les constructions de référence. Leur transfert exact aux demi-plans arbitrairement orientés, au clipping par $J_{pq}$ et aux égalités du produit doit encore être formalisé; il ne s'agit pas d'un kernel livré.

Un constructeur réel doit en plus payer le range-report, la préparation des demi-plans, les prédicats exacts, les égalités et la compaction. Un prototype qui obtient les $Z_e$ sommets en testant d'abord toutes les $\binom{m_e}{2}$ intersections n'acquiert donc aucun gain de complexité. Le premier prototype doit annoncer séparément travail proposé, décisions certifiées et sorties terminales.

### 5.3 Somme globale

En sommant les bornes locales, on obtient immédiatement

$$\sum_{e\in\mathcal{A}}Z_e\leq\sum_{e\in\mathcal{A}:\kappa_e\geq0}m_e(\kappa_e+1)\leq(s_{\max}-3)M.$$

À rang fermé 11, le nombre de sommets peu profonds est donc au plus $8M$. Cette borne porte sur les sommets proposés avec répétitions entre ancres; un ownership canonique peut réduire la sortie, jamais le travail déjà dépensé pour les trouver.

En séparant les sources, la complexité constructive conditionnelle s'écrit

$$T=T_{\mathcal{A}}+T_{\mathrm{range}}+O\!\left(\sum_{e\in\mathcal{A}}\left[m_e\log m_e+m_e(\kappa_e+1)\right]\right)+T_{\mathrm{exact}}+T_{\mathrm{sink}},$$

où les termes avec $\kappa_e<0$ sont nuls. Un scan de tout le nuage par ancre donne à lui seul $T_{\mathrm{range}}=\Theta(an)$; un LBVH doit remplacer ce scan, mais sa sortie et son comportement sur les amas restent à mesurer.

Cela ne prouve pas une complexité linéaire en $n$. Dans le pire cas, $a=\Theta(n^{2})$, $m_e=\Theta(n)$ pour beaucoup d'ancres et $M=\Theta(n^{3})$. À $K$ fixé, le cœur d'arrangement reste alors en $O(n^{3}\log n)$; lorsque $K$ croît jusqu'à être comparable à $m_e$, le préfixe peu profond devient l'arrangement entier et le pire cas remonte à $O(n^{4})$. Aucun RNG d'ordre fixé, ni le catalogue de paires de rang utile, ne borne $a$ à cause de la variante régulière du Théorème 1.

Le régime favorable à démontrer ou mesurer est au contraire

$$a=O(n\,p(K)),\qquad M=O(n\,p(K)).$$

Sous ces deux hypothèses et un range-report proportionnel à sa sortie, le cœur devient $\widetilde O(n\,p(K)K)$, donc quasi linéaire en $n$ pour $K$ fixé. Aucune preuve actuelle ne fournit la fonction $p$; cet énoncé est une cible falsifiable, pas une complexité acquise.

Les mesures `uniform_latin` existantes montrent seulement environ 90,5 paires retenues par point pour l'arité trois à 50 000 points, sur un run censuré. Elles ne certifient ni l'arité quatre, ni $M$, ni `eight_clusters`; sur cette dernière famille, la restriction tangente conserve 76 % des premières paires parcourues. Il n'existe donc encore aucune extrapolation légitime à dix millions de points.

### 5.4 Coût propre du graphe et des cliques

Supposons qu'un surgraphe certifié $G=(P,E)$ soit disponible, qu'il provienne de $G_\tau$ ou d'une autre preuve. Soit $d$ sa dégénérescence, grandeur plus sûre que le degré moyen en présence de hubs. Une orientation de dégénérescence permet d'énumérer les triangles en $O(\lvert E\rvert d)$ et les 4-cliques en $O(\lvert E\rvert d^{2})$, à sorties incluses. Avec un degré maximal $\Delta$, les bornes plus grossières sont $O(n\Delta^{2})$ et $O(n\Delta^{3})$. Le stockage du graphe reste $O(n+\lvert E\rvert)$ et les cliques peuvent être streamées; aucune mosaïque de Delaunay d'ordre supérieur n'apparaît.

Dans le régime favorable $\lvert E\rvert=O(n\,p(K))$ et $d=O(p(K))$, le parcours des 4-cliques vaut $O(n\,p(K)^{3})$, donc il est linéaire en $n$ pour $K$ fixé. Au pire, $\lvert E\rvert=\Theta(n^{2})$, $d=\Theta(n)$ et le nombre de 4-cliques atteint $\Theta(n^{4})$. Le mécanisme est bien « sparse sauf peut-être au pire cas », mais la parcimonie doit être certifiée ou mesurée par famille; elle ne découle pas de Jung.

Le pire cas dense est réalisable même en partant du RNG classique. Choisissons $m$ directions unitaires rationnelles distinctes $u_i$, des points $p_i=\varepsilon u_i$ et des satellites $s_i=Ru_i$, avec $R>3\varepsilon$. La lune de $p_is_i$ est vide : pour $j\neq i$, $p_j$ est plus loin de $s_i$ que $p_i$, et $s_j$ est plus loin de $p_i$ que $s_i$. Donc $p_is_i$ appartient au RNG et $\lambda(p_i)\geq R-\varepsilon$. Si $2\varepsilon\leq\alpha(R-\varepsilon)$, les $m$ points $p_i$ deviennent une clique du graphe épaissi, y compris avec la règle mutuelle au minimum. Pour $n=2m$, cela donne exactement les ordres de grandeur $\Theta(n^{2})$ arêtes, $\Theta(n^{3})$ triangles et $\Theta(n^{4})$ 4-cliques.

Une architecture hybride est plus forte que l'énumération brute des 4-cliques : employer $G_\tau$ comme source sparse d'ancres et de voisinages, puis énumérer seulement les sommets de faible profondeur dans chaque disque de Jung. Elle conserve le coût graphe $O(n+\lvert E\rvert)$, évite le facteur $d^{2}$ lorsque beaucoup de cliques ont un rang trop élevé et ne matérialise toujours ni cellules, ni cofaces globales. Le RNG épaissi peut accélérer la proposition des arêtes, à condition qu'un complément fail-open certifie les arêtes de $G_\tau$ qu'il n'a pas proposées.

La construction exacte du RNG initial ne doit pas elle-même passer par Delaunay : elle doit devenir une requête LBVH de lunes et un flux CSR borné. Son coût produit sera $T_{\mathrm{RNG}}+T_{\mathrm{growth}}$ et non gratuitement $O(n)$; un RNG de petite sortie peut encore être cher à certifier si la recherche visite des couples quadratiques.

### 5.5 Complément exact des ancres par blocs de centres

Le graphe RNG--Jung peut rester un excellent producteur de propositions, mais le Théorème 1 interdit de lui déléguer la complétude. La cible GPU complète donc ses arêtes par un self-join dual-tree du LBVH, sans développer d'abord les paires de points. Un état de la file contient deux nœuds d'extrémités disjoints $A,B$ et représente implicitement toutes leurs paires. Les blocs diagonaux sont divisés canoniquement; un bloc hors diagonale est soit rejeté par certificat, soit subdivisé, soit développé seulement lorsqu'il est devenu assez petit.

Soit $H$ la plus grande séparation coordonnée possible entre les boîtes de $A$ et $B$. Pour toute paire $p\in A,q\in B$, on a $D\leq\sqrt{3}H$ et le déplacement du centre par rapport au milieu est au plus $D/\sqrt{8}$. Comme $\sqrt{3/8}<5/8$, tous les centres de Jung possibles du bloc sont contenus dans la boîte des milieux, élargie de $5H/8$ sur chaque axe. Cette boîte est divisée en un nombre fixe $R$ de patches dyadiques; ces patches sont un recouvrement transitoire de centres, pas des cellules géométriques persistantes.

Pour un patch $C$ et un nœud témoin $W$ disjoint de $A\cup B$, la borne dirigée suivante suffit :

$$\min_{c\in C,\ p\in A,\ x\in W}\left(\lVert c-p\rVert^{2}-\lVert c-x\rVert^{2}\right)>0.$$

Elle certifie que tous les points de $W$ sont strictement intérieurs à toute sphère pertinente dont le centre est dans $C$. Les nœuds témoins comptés pour un même patch forment une antichaîne disjointe. Si chaque patch possède au moins $s_{\max}-3$ témoins pour un support quatre — huit au rang fermé 11 — le bloc entier est impossible, car deux extrémités, deux futurs carriers et ces témoins imposeraient un rang au moins égal à 12. Le seuil correspondant pour un support trois est $s_{\max}-2$, donc neuf. Toute borne ambiguë subdivise le témoin ou le bloc d'extrémités et ne rejette jamais.

Avec $Q$ blocs d'extrémités visités, $V_c$ visites de nœuds témoins, $R$ fixe, $a$ ancres résiduelles, $M=\sum_em_e$ lignes actives et $Z=\sum_eZ_e$, le coût cible est

$$O\!\left(n\log n+E_{\mathrm{prop}}+Q+RV_c+\sum_eV_e+\sum_e\left[m_e\log m_e+m_e(\kappa_e+1)\right]+Z+T_{\mathrm{exact}}+T_{\mathrm{sink}}\right).$$

Dans le régime favorable $E_{\mathrm{prop}},Q,M=O(n\,p(K))$, ce coût est quasi linéaire en $n$ à $K$ fixé. Le pire cas reste explicitement dense : $Q=\Theta(n^{2})$, $V_c=\Theta(n^{3})$ et $M=\Theta(n^{3})$ sont possibles; le cœur shallow vaut alors $O(n^{3}\log n)$ à $K$ fixé et peut redevenir quartique lorsque $K=\Theta(n)$. Une capacité épuisée doit donc publier un résidu ou un arrêt budgétaire honnête, jamais basculer vers une mosaïque, une matrice de paires ou un catalogue global de cofaces.

### 5.6 Conséquence pour le contrat

Même un générateur d'arité quatre parfait ne suffit pas au chemin actuel. Le diagnostic direct frais sur G4 mesure la seule frontière paire à 50 000 points et rang fermé 11 à 2,395883 s dans sa fenêtre `frontier_ns`, et 3,927585 s depuis le début froid du processus, avant supports trois--quatre, classification exacte et réduction. La sonde de germination lancée sur cette VM n'est pas un kernel : elle exécute le prototype CPU séquentiel et ne parcourt que 0,363834 % des paires `uniform_latin` en 120 s. Le contrat sous la seconde exige donc un pipeline fusionné qui évite la recertification et la matérialisation intermédiaires actuelles, pas seulement un remplacement de la boucle quadratique d'arité quatre.

La nouvelle réduction peut néanmoins enlever la cause principale de cette explosion : la profondeur incorpore le rang pendant la génération, au lieu d'envoyer des milliards de quadruplets vers un classifieur de boule fermée. Sa valeur doit être jugée sur $a$, $M$, $\sum Z_e$, les records uniques acceptés et le temps exact bout en bout.

## 6. Décomposition GPU proposée

Le mécanisme peut rester sparse et streamé. Il ne construit ni mosaïque de Delaunay d'ordre supérieur, ni Gamma global, ni population globale de cellules, cofaces ou incidences.

| étage | forme GPU plausible | risque principal |
|---|---|---|
| graphe de proposition | RNG et épaississement incident par requêtes LBVH, CSR trié, intersections d'adjacences | la fixture prouve que ce graphe seul n'est pas une autorité; degré et hubs peuvent exploser |
| ancres | arêtes de $G_\tau$ certifiées, segmentées en chunks bornés | $a$ peut rester quadratique et le calcul de $\tau$ reste ouvert |
| voisinage $W_e$ | range-report LBVH dans la boule de confinement J10, count--scan--emit | longueur variable et ancres lourdes |
| demi-plans | un thread par couple ancre--point; coefficients binary64 proposés et transcript exact | base 2D, orientations et cas presque parallèles |
| niveaux peu profonds | warp brute pour une petite ancre, constructeur de niveaux par lots pour une ancre moyenne, file persistante pour les ancres lourdes | l'algorithme incrémental théorique porte un graphe de conflits dynamique, des tris, du ray-shooting et de la divergence; il n'est pas une primitive GPU prête |
| décision | filtres diamètre et bon centrage, puis échelle exacte `int128/int256/int512/int1024` et fallback hôte | égalités, dépassements et coût du fallback |
| sortie | compaction segmentée, ownership par paire diamètre canonique, sink direct vers la réduction | doublons de diamètres et backpressure |

Les calculs binary64 ou tensoriels ne peuvent produire que des propositions. Toute comparaison capable d'omettre une corde, un sommet peu profond ou une égalité doit être dirigée ou rejouée exactement. Les coefficients peuvent être portés dans une base rationnelle non normalisée du plan médiateur afin d'éviter que la normalisation par $D$ devienne une autorité flottante; le disque devient alors une ellipse sous une forme quadratique exacte.

Sans fusion, la mémoire maximale d'un chunk est

$$O\!\left(B+\sum_{e\in\mathrm{chunk}}\lvert W_e\rvert+\sum_{e\in\mathrm{chunk}}m_e+\sum_{e\in\mathrm{chunk}}Z_e\right),$$

où $B$ est le nombre d'ancres du chunk. Le chemin cible fusionne le range-report et la classification en demi-plans, sous-tuile $W_e$, compte $c_e$ et n'émet que les $m_e$ lignes actives; son scratch remplace alors la somme complète des $\lvert W_e\rvert$ par la capacité bornée de la sous-tuile courante. Il ne faut jamais conserver tous les voisinages de toutes les ancres. Les distributions de $\lvert W_e\rvert$, $m_e$ et $Z_e$ imposent un ordonnancement par classes de taille et une file lourde; sans cela, quelques ancres agglomérées déterminent toute la latence de bloc.

Le point le moins GPU-friendly est l'énumération exacte des niveaux peu profonds. Les étapes `sort`, histogramme, `count--scan--emit`, classification indépendante et compaction sont régulières; le maintien dynamique des niveaux, les concurrences et les reprises exactes ne le sont pas. Un prototype qui teste toutes les paires de cordes dans un bloc peut servir d'oracle ou de falsificateur pour petits $m_e$, mais il conserve le coût quadratique et ne qualifie pas l'architecture. La porte d'implémentation demande un algorithme de niveaux dont le travail est lié à $m_e$, $\kappa_e$ et $Z_e$, avec un transcript de complétude indépendant.

Le prototype CPU actuel de germination n'est pas encore cette implémentation exacte. Plusieurs rejets de `local_germination.cpp` reposent directement sur des normes et comparaisons binary64 pour la borne tangente, la porte triangulaire de Jung et la distance entre les deux tiers. Aucun différentiel n'a encore exhibé une perte à ces endroits, mais l'absence d'intervalle extérieur ou de repli exact laisse un trou de preuve. Ces décisions doivent devenir des propositions fail-open, avec fixtures `nextafter` et reprise entière ou rationnelle, avant qu'un graphe ou un arrangement puisse porter `public_status=exact`.

### 6.1 Première tranche CUDA : cordes résidentes, pas encore supports

`P15-HOCUDA-P0` implémente une première primitive bornée `JungChordCsrTile`. Elle consomme une tuile explicite d'ancres et une lease LBVH authentifiée, puis produit sur le device un CSR de lignes actives. Pour $d=q-p$, $A=2x-p-q$, $D^{2}=d\cdot d$, $B=A\cdot A-D^{2}$ et $G=\lVert d\times A\rVert^{2}$, les filtres sont $Q_3=4G-3B^{2}$ pour l'arité trois et $Q_4=2G-B^{2}$ pour l'arité quatre. Un intervalle certifiant $Q<0,B<0$ compte un témoin intérieur constant; $Q<0,B>0$ certifie un extérieur constant; toute autre incertitude descend le LBVH puis émet la feuille `fail_open`.

Le range-prune rationnel emploie $\lVert x-M\rVert^{2}>3D^{2}/4$ pour l'arité trois et la borne sûre $\lVert x-M\rVert^{2}>15D^{2}/16$ pour l'arité quatre. Toutes les lignes actives restent dans le CSR parce qu'un point hors de la lentille diamètre peut encore changer la profondeur. Un bit séparé `support_eligible` autorise comme carrier seulement un point vérifiant $\lVert x-p\rVert\leq D$ et $\lVert x-q\rVert\leq D$, avec égalité ou ambiguïté conservée. Le chemin `count--scan--emit` ne parcourt jamais les $\binom{m_e}{2}$ intersections; il publie cette masse uniquement comme baseline analytique non exécutée.

Pour $a$ ancres, $V$ visites de nœuds, $W$ feuilles testées et $M$ lignes émises, cette tranche coûte $\Theta(V+W+M)$ par passe, avec deux parcours LBVH, et retient $O(a+M)$ mots en plus du LBVH résident. Dans un régime spatial favorable, $V=O(a\log n+M)$; au pire $V=\Theta(an)$. La dépendance directe à $K$ se limite ici au cutoff $c_e>s_{\max}-4$ ou $c_e>s_{\max}-3$, donc à un compteur borné par $O(K)$; la source d'ancres et le futur constructeur shallow portent les dépendances globales restantes.

Cette tranche ne calcule encore aucun $Z_e$, aucun support trois ou quatre, aucun rang terminal et aucune réduction. Sa source de qualification `morton_window_proposal_only` est volontairement incomplète; une capacité de CSR peut retourner `capacity_exhausted`. Elle mesure donc le coût du range-report et la distribution des lignes, mais ne peut qualifier ni le contrat 50 k, ni une complexité globale, même si son kernel isolé est rapide. La prochaine tranche produit le complément d'ancres par blocs décrit en 5.5, puis construit les niveaux peu profonds sans travail en $\sum_em_e^{2}$.

## 7. Ne pas confondre les deux RNG

Le RNG du graphe pondéré des facettes déjà découvertes est une réduction de connectivité valide : remplacer une arête par un chemin strictement plus léger préserve les composantes à tous les seuils. Il peut donc réduire un flux après génération.

Il ne découvre pas ses propres sommets ni arêtes. Construire naïvement le graphe des facettes revient à matérialiser les structures combinatoires interdites, et les incidences silencieuses déjà enregistrées empêchent de lire la seule réduction Gabriel comme une autorité de complétude. Le Théorème 1 réfute le RNG de points comme source par cliques; il ne réfute pas le RNG de facettes comme compression postérieure.

## 8. Portes falsifiables avant un nouveau test G4

1. Étendre l'oracle rationnel borné pour comparer, sur toutes les fixtures $n\leq14$, les supports acceptés exhaustivement et ceux retrouvés par disque, demi-plans et profondeurs; inclure permutations, égalités de diamètre, parallélisme et shell supplémentaire.
2. Implémenter un census CPU séparant les degrés et la dégénérescence du RNG, du graphe épaissi et de $G_\tau$, leurs arêtes manquantes contre l'oracle, puis $a$, $m_e$, $M$, profondeurs par niveau, $Z_e$, intersections brutes évitées, doublons, largeurs exactes et fallbacks, sur les trois familles de la porte P0 à $n=32$.
3. Refuser comme autorité tout graphe épaissi qui omet une arête de support; refuser aussi la route shallow si le travail du constructeur de niveaux reste proportionnel à $\sum_e m_e^{2}$ ou si sa sortie n'est pas bit-à-bit identique à l'oracle.
4. Fermer les rejets binary64 du prototype par intervalles extérieurs et repli exact, avec corpus ULP/`nextafter`; aucune comparaison flottante ne peut omettre un candidat.
5. La sous-porte `P15-HOCUDA-P0` autorise avant cette promotion un backend device strictement `proposal_only`, destiné à mesurer le range-report, les demi-plans et le travail shallow avec résidu explicite. Après différentiel, builds Release/audit et compute-sanitizer, seulement, il peut devenir candidat à une autorité. La prochaine session G4 du nouveau composant commence à $n=32$, puis va directement à 50 000 si les compteurs de croissance sont favorables.
6. Les dizaines de millions restent interdites tant que le pipeline exact complet à 50 000 points n'a pas terminé sous le contrat, avec reprise, mémoire et sortie aval réelles.

La preuve seule ne justifie toujours aucun kernel autoritaire de niveaux peu profonds. `P15-HOCUDA-P0` autorise uniquement un falsificateur de travail CUDA dont les sorties ne sont pas consommables scientifiquement. La campagne directe précédente confirme la frontière paire à 2,396 s, révèle que la germination de 120 s est CPU et archive l'arrêt ciblé des VM dans [`phase15_rng_jung_g4_20260808/RESULTATS.md`](../validation/phase15_rng_jung_g4_20260808/RESULTATS.md). Ni cette campagne ni le prototype ne remplacent le différentiel exact et aucun SLO n'est ouvert.

## 9. Références externes

- G. T. Toussaint, [*The relative neighbourhood graph of a finite planar set*](https://doi.org/10.1016/0031-3203(80)90066-7), 1980.
- T. Lim et R. J. McCann, [*Isodiametry, variance, and regular simplices*](https://www.math.toronto.edu/mccann/papers/LimMcCann19.pdf), annexe A pour une formulation moderne de Jung et son cas d'égalité.
- D. Attali, A. Lieutier et D. Salinas, [*Vietoris--Rips complexes also provide topologically correct reconstructions of sampled shapes*](https://dattali.github.io/Publications/2012-cgta-Rips.pdf), §2 pour le sandwich Rips--Čech issu de Jung.
- T. M. Chan, [*On the Bichromatic k-Set Problem*](https://doi.org/10.1145/1824777.1824782) ([copie auteur](https://tmc.web.engr.illinois.edu/bi.pdf)), pour les niveaux d'arrangements de demi-plans et la borne cumulative $\Theta(m\kappa)$. Le préprint ultérieur arXiv:1609.07709 est retiré au profit de ce résultat antérieur et n'est pas employé comme autorité.
- B. Aronov et S. Har-Peled, [*On Approximating the Depth and Related Problems*](https://doi.org/10.1137/060669474) ([copie auteur](https://sarielhp.org/p/04/depth/depth.pdf)), §4.2 pour la construction randomisée des faces de profondeur au plus $k$ en dimension deux.
- P. K. Agarwal, M. de Berg, J. Matoušek et O. Schwarzkopf, [*Constructing Levels in Arrangements and Higher Order Voronoi Diagrams*](https://doi.org/10.1137/S0097539795281840) ([copie auteur](https://pure.tue.nl/ws/portalfiles/portal/2495316/Metis201003.pdf)), pour les algorithmes de construction des premiers niveaux de droites.
- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf), comme comparaison avec la matérialisation globale que le chemin produit MorseHGP3D évite.
