# RNG, Jung, cliques et niveaux peu profonds pour les supports 3D

> [!IMPORTANT]
> L'autorité mathématique reste en Phase 15, `backend=reference_cpu`, `profile=hgp_reduced`, `mode=exact_rational_obstruction_and_shallow_arrangement_theorem`. La sous-porte `P15-HOCUDA-P0` autorise uniquement un prototype GPU de proposition et de mesure. La sous-porte documentaire `P15-HOCUDA-P1`, `backend=cuda_g4_plus_reference_cpu_oracle`, `profile=hgp_reduced`, `mode=complete_fail_open_center_cover_anchor_superset_v1`, spécifie le futur sur-ensemble complet d'ancres diamètre, mais aucun code P1 ne lui confère encore une autorité. Les deux restent `prototype_only`, sans décision terminale, réducteur ou SLO. Ce document ne ferme aucune phase et maintient `public_status=not_claimed`.

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

Ainsi $h_x(v)>0$, $h_x(v)=0$ et $h_x(v)<0$ signifient respectivement que $x$ est strictement dans la sphère, sur son bord et strictement dehors. Lorsque la projection de $x-M$ sur $e^{\perp}$ est non nulle, l'équation $h_x(v)=0$ est une droite et son intersection avec $J_{pq}$ est la corde de Jung de $x$ lorsqu'elle est non vide. Un site collinéaire à $pq$ induit au contraire une fonction constante : il est intérieur universel si $\lVert x-M\rVert<D/2$, extérieur universel si $\lVert x-M\rVert>D/2$, et shell universel en cas d'égalité. Ce dernier cas, notamment un identifiant distinct à la coordonnée d'une extrémité, appartient au census de plateau et non à l'arrangement de droites.

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

Le Théorème 3 compte tous les sites du nuage et porte donc sur une profondeur globale $\delta_e^{\mathrm{full}}$. Le chemin v5 historique emploie en q4 un cover de coefficient 3 qui contient les carriers utiles, mais peut omettre des intérieurs de leur sphère; `mhgp5_q4_cover_fixture` en donne un témoin permanent. Sa profondeur $\delta_e^{(3)}$ reproduit le filtre de génération et `digest_balls`, pas nécessairement le rang fermé. À l'inverse, le cover q3 de coefficient 3 contient tous les intérieurs. Une intégration compatible doit nommer ces deux profondeurs séparément; une route qui revendique le rang du Théorème 3 doit fournir le range-report global ou une recertification terminale équivalente.

Sans position générale, plusieurs droites peuvent être concourantes et des sites collinéaires peuvent être shell universel. Notons $S_e(v)$ le shell complet hors des deux extrémités, $c_e$ le nombre d'intérieurs universels et $d(v)$ le nombre des autres demi-plans qui contiennent $v$ strictement. Le rang fermé de la **boule complète** vaut alors

$$\mathrm{rang}_{\text{fermé}}(B_v)=2+c_e+d(v)+\lvert S_e(v)\rvert.$$

Cette quantité ne constitue pas une porte de rejet des carriers pertinents. Le contrat de plateau impose tous les intérieurs stricts, mais permet un sous-ensemble $T$ du shell complet $U_e(v)=\left\lbrace p,q\right\rbrace\cup S_e(v)$ dès que le centre appartient à $\mathrm{conv}(T)$ et que $\lvert I\rvert+\lvert T\rvert\leq s_{\max}$. Pour un carrier minimal de cardinalité quatre, la profondeur de génération reste donc $c_e+d(v)\leq s_{\max}-4$, indépendamment de $\lvert S_e(v)\rvert$. En particulier, la conclusion antérieure $t(v)\leq9-c_e$ et le rejet avant développement lorsque le shell dépassait le rang étaient faux hors `RelevantGP`.

La fixture permanente `mhgp5_plateau_shell_relevance` fixe douze points entiers sur une même sphère : sa coquille complète dépasse 11, mais elle contient un tétraèdre affinement indépendant dont le centre a les quatre barycentriques égales à $1/4$. Pour l'ancre canonique de ce tétraèdre, les dix autres sites donnent dix droites géométriques distinctes et concurrentes, toutes présentes dans le cover q4 historique de coefficient 3. Ce carrier reste pertinent et doit atteindre le census puis l'expansion de plateau.

Une implémentation exacte groupe chaque concurrence par centre ou `BallKey`, réalise le census complet puis laisse l'expansion de plateau décider les sous-supports. Elle ne perturbe pas la concurrence en intersections artificielles. Cependant, préserver le contrat historique de représentation peut encore exiger de retrouver le représentant `(BallKey, arity, ExactLevel)` minimal choisi par le RLE : `q4_level_raw` dépend du support même lorsque la boule sémantique est identique. Grouper seulement les `BallKey` ne prouve donc ni le digest ni la forêt. La borne locale ci-dessous est démontrée en position générale; le groupement évite les centres dupliqués mais ne fournit pas, à lui seul, une borne sur l'examen des paires incidentes d'une concurrence.

## 5. Complexité en $n$ et en $K$

### 5.1 Variables qui gouvernent réellement le coût

Soit $\mathcal{A}$ l'ensemble des paires diamètre candidates produites par une autorité exacte, $a=\lvert\mathcal{A}\rvert$. Pour $e=pq$, soit $W_e$ l'ensemble local des points qui peuvent être intérieurs ou sur le shell d'une sphère dont le centre appartient à $J_{pq}$. Parmi eux, $c_e$ points ont une puissance strictement positive sur tout le disque, tandis que $m_e$ autres ont une droite frontière qui rencontre le disque et participent à son arrangement. Les sites shell universels collinéaires sont conservés séparément pour le census. Les points dont la puissance reste strictement négative sur tout le disque sont sans effet. Posons

$$M=\sum_{e\in\mathcal{A}}m_e,\qquad C=\sum_{e\in\mathcal{A}}c_e.$$

Jung borne aussi le range-report. Toute sphère utile ancrée par $pq$ est contenue dans la boule centrée en $M$ de rayon $\cos(15^{\circ})D$. Un point plus éloigné ne peut être ni intérieur ni sur le shell d'un centre de $J_{pq}$. Le LBVH peut donc proposer $W_e$ sans matrice paire--point globale.

Les grandeurs $a$, $M$ et $C$, pas seulement $n$, décident la scalabilité. Elles doivent être publiées par famille, avec histogrammes et maxima; une moyenne seule masque les ancres lourdes. Si $c_e>s_{\max}-4$, l'ancre est rejetée avant tout arrangement.

### 5.2 Borne combinatoire locale

Pour l'ancre $e$, le budget de profondeur variable est $\kappa_e=s_{\max}-4-c_e$. En position générale, la complexité cumulative $\Theta(m\kappa)$ des premiers niveaux d'un arrangement de demi-plans est connue par la méthode de Clarkson--Shor et rappelée par Chan. La constante utile ici possède une preuve élémentaire que nous reproduisons. Sur une droite frontière $\ell$, soient $u,v$ les intersections extrêmes de profondeur au plus $\kappa_e$. Toute autre droite coupant strictement l'intervalle $[u,v]$ place exactement l'un de ses deux bouts dans son demi-plan positif. Si cet intervalle contenait plus de $2\kappa_e$ intersections strictement entre ses extrémités, les profondeurs de $u$ et $v$ auraient donc une somme supérieure à $2\kappa_e$, ce qui est impossible. Chaque droite porte au plus $2\kappa_e+2$ sommets de profondeur au plus $\kappa_e$. Chaque sommet appartenant à deux droites, l'arrangement entier en possède au plus $m_e(\kappa_e+1)$. Le disque de Jung ne peut qu'en retirer. Ainsi, pour $\kappa_e\geq0$,

$$Z_e\leq m_e(\kappa_e+1)=m_e(s_{\max}-3-c_e).$$

Pour $s_{\max}=11$ et $c_e=0$, donc $\kappa_e=7$, une ancre possède au plus

$$Z_e\leq8m_e$$

sommets admissibles par leur profondeur, avant même les rejets diamètre et bon centrage. C'est la vraie rupture asymptotique locale face aux $\binom{m_e}{2}$ paires de cordes de la boucle actuelle. La borne est serrée au pire cas; la constante huit ne doit donc pas être présentée comme un simple artefact de preuve.

La borne cumulative $m_e(\kappa_e+1)$ est celle qui correspond directement au produit : tous les rangs fermés jusqu'à $s_{\max}$ sont requis. Dans la convention `requested_order`, elle vaut $m_e(K_{\mathrm{eff}}-2-c_e)$ lorsqu'elle est positive. Elle est `proved_here`; la littérature citée fournit le contexte asymptotique, pas une autorité logicielle.

Les résultats de construction des premiers niveaux donnent aussi une faisabilité algorithmique théorique : sous position générale, une construction incrémentale randomisée de type Las Vegas atteint un coût espéré $O(m_e\log m_e+m_e(\kappa_e+1))$ pour les faces peu profondes, à un facteur de fonction d'Ackermann près selon la variante déterministe. Aronov--Har-Peled et Agarwal--de Berg--Matoušek--Schwarzkopf donnent les constructions de référence. Leur transfert exact aux demi-plans arbitrairement orientés, au clipping par $J_{pq}$ et aux égalités du produit doit encore être formalisé; il ne s'agit pas d'un kernel livré.

Un constructeur réel doit en plus payer le range-report, la préparation des demi-plans, les prédicats exacts, les égalités et la compaction. Un prototype qui obtient les $Z_e$ sommets en testant d'abord toutes les $\binom{m_e}{2}$ intersections n'acquiert donc aucun gain de complexité. Le premier prototype doit annoncer séparément travail proposé, décisions certifiées et sorties terminales.

### 5.3 Somme globale

En position générale, en sommant les bornes locales, on obtient immédiatement

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

### 5.5 Obstruction étroite à une décomposition ternaire exact-once

Une WSSD approximative peut avoir une taille linéaire en dimension fixée. Le résultat suivant ne l'interdit pas et ne doit donc pas être intitulé « la WSPD ne se généralise pas ». Il ferme seulement le remplacement de la source q3 par des blocs ternaires **symétriques**, fortement séparés et exact-once qui réaliseraient explicitement tous les supports aigus.

> **Théorème 4.** Soient $P_0,P_1\subset\mathbb{R}^{3}$, avec $\lvert P_0\rvert=\lvert P_1\rvert=m$, $\mathrm{diam}(P_0)\leq\Delta$, et toutes les distances entre $P_0$ et $P_1$ ainsi qu'entre deux points distincts de $P_1$ au moins égales à $L>\Delta/s$, pour $s>1$. Un bloc ternaire est un triplet non ordonné $\left\lbrace A,B,C\right\rbrace$ de facteurs non vides, deux à deux disjoints et inclus dans $P_0\cup P_1$. Posons $D_\tau=\max(\mathrm{diam}(A),\mathrm{diam}(B),\mathrm{diam}(C))$ et exigeons $\mathrm{dist}(X,Y)\geq sD_\tau$ pour toute paire de facteurs. Si chaque support non ordonné $\left\lbrace p,q,r\right\rbrace$, avec $p\neq q\in P_0$ et $r\in P_1$, appartient au produit d'interaction d'un unique bloc, alors la famille contient au moins $m(m-1)$ blocs.

*Preuve.* Dans le bloc qui couvre $\left\lbrace p,q,r\right\rbrace$, les trois points occupent trois facteurs distincts. Les facteurs de $p$ et $q$ donnent $sD_\tau\leq\lVert p-q\rVert\leq\Delta$, donc $D_\tau\leq\Delta/s<L$. Le facteur de $r$ ne peut contenir ni un autre point de $P_1$, ni un point de $P_0$; il vaut donc $\left\lbrace r\right\rbrace$, et les deux autres facteurs sont inclus dans $P_0$.

Fixons $r$. Les couples de facteurs restants partitionnent exactement toutes les paires de $P_0$. Choisissons une arête représentative entre un point de chaque facteur de chaque couple. Le graphe ainsi obtenu sur $P_0$ est connexe. Sinon, choisissons $p,q$ de composantes distinctes à distance minimale et regardons le couple qui les couvre. Les diamètres de ses deux facteurs sont strictement inférieurs à $\lVert p-q\rVert$ puisque $s>1$; par minimalité, chaque représentant reste dans la composante du point correspondant, alors que son arête relie les deux composantes, contradiction. Il faut donc au moins $m-1$ couples pour chaque $r$. Deux points $r$ distincts ne partagent aucun bloc puisque leur facteur est singleton, d'où $m(m-1)$. $\square$

Cette borne porte réellement sur des supports q3 aigus. Prenons $P_0$ sur un cercle de rayon $R$ dans le plan $z=0$ et $P_1=\left\lbrace(0,0,H_j)\right\rbrace$ sur son axe, avec $\min_j H_j>\max(R,2R/s)$ et des espacements supérieurs à $2R/s$. Pour $p\neq q\in P_0$ et $r=(0,0,H_j)$, les trois produits scalaires qui testent l'acuité sont strictement positifs :

$$ (q-p)\mathbin{\cdot}(r-p)=R^{2}-p\mathbin{\cdot}q>0,\qquad(p-r)\mathbin{\cdot}(q-r)=p\mathbin{\cdot}q+H_j^{2}>0. $$

Le premier calcul vaut symétriquement au sommet $q$. Les hypothèses du théorème sont satisfaites avec $\Delta=2R$ et un $L$ adéquat; toute réalisation symétrique fortement séparée et exact-once de ces supports aigus est donc quadratique.

La portée négative s'arrête là. Le théorème ne borne pas une source asymétrique ancre--tiers, une source déjà restreinte aux centres de profondeur au plus huit, un arrangement de centres, ni une WSSD approximative. Il n'établit donc pas que q3 doit être quadratique dans l'architecture fibrée proposée ici.

Enfin, l'acuité fournit précisément la localisation que perd une séparation métrique seule. Si $D$ est la plus longue arête d'un triangle aigu, $R_c$ son circumrayon et $m_D$ le milieu de cette arête, alors

$$ R_c^{2}\leq\frac{D^{2}}{3},\qquad\lVert c-m_D\rVert^{2}=R_c^{2}-\frac{D^{2}}{4}\leq\frac{D^{2}}{12}. $$

Un triangle proche de $89^{\circ}$--$89^{\circ}$--$2^{\circ}$ illustre donc une disparité de côtés, pas une délocalisation du centre q3 : son circumrayon tend vers $D/2$ et son centre vers le milieu de l'arête maximale. Dans le produit, le troisième sommet est le carrier proposé par le cover; les autres sites sont les témoins de profondeur.

### 5.6 Complément exact des ancres par blocs de centres

Le graphe RNG--Jung peut rester un excellent producteur de propositions, mais le Théorème 1 interdit de lui déléguer la complétude. `P15-HOCUDA-P1`, `backend=cuda_g4_plus_reference_cpu_oracle`, `profile=hgp_reduced`, `mode=complete_fail_open_center_cover_anchor_superset_v1`, confie cette complétude à un self-join dual-tree du LBVH. Un état contient deux nœuds d'extrémités disjoints $A,B$ et représente implicitement toutes leurs paires. La décomposition canonique $T(N)=T(L)\mathbin{\dot\cup}(L\times R)\mathbin{\dot\cup}T(R)$ et la subdivision d'un produit croisé sur un seul côté partitionnent chaque paire non ordonnée exactement une fois. Le RNG épaissi peut seulement ordonner ces états; il ne ferme aucun bloc et n'émet aucun second flux à dédupliquer.

Soit $H=\max_i\max(\lvert A_i^{-}-B_i^{+}\rvert,\lvert A_i^{+}-B_i^{-}\rvert)$, calculé vers le haut, et soit $M_{AB}$ la boîte extérieure de tous les milieux. Pour toute paire $p\in A,q\in B$, on a $D^{2}\leq3H^{2}$. Si $pq$ est une paire diamètre d'un support quatre bien centré, Jung impose $\lVert c-(p+q)/2\rVert^{2}\leq D^{2}/8$. Comme $3/8<25/64$, chaque coordonnée du déplacement est strictement inférieure à $5H/8$. Tous les centres possibles sont donc dans $C_{AB}=M_{AB}\mathbin{\oplus}[-5H/8,5H/8]^{3}$. Cette boîte est partagée en $4\times4\times4=64$ patches fermés. Les arrondis sont extérieurs, les frontières peuvent se recouvrir et les patches restent un scratch transitoire, jamais des cellules géométriques persistantes. Pour un support trois, la constante correspondante est $H/2$.

Exiger des témoins dans les 64 patches bruts serait sûr mais trop faible : la boîte contient de nombreux points qui ne sont sur aucun plan médiateur d'une paire du bloc. P1 doit d'abord certifier les patches infaisables. Pour des intervalles $C=[c_-,c_+]$, $P=[p_-,p_+]$ et $X=[x_-,x_+]$ d'une coordonnée, posons $g(t)=\mathrm{dist}(t,P)^{2}-\max((t-x_-)^{2},(t-x_+)^{2})$. Entre les bornes de $P$ et le milieu de $X$, cette fonction est affine ou concave; le milieu de $X$ est une coupure à dérivée descendante et ne peut être un minimum intérieur. Par conséquent son minimum exact sur $C$ est atteint dans $\mathcal{S}=\left\lbrace c_-,c_+\right\rbrace\cup(\left\lbrace p_-,p_+\right\rbrace\cap C)$. En sommant les trois minima avec arrondi inférieur, on obtient $L(C,P,X)$; la borne supérieure est $U(C,P,X)=-L(C,X,P)$.

Un patch $C$ est retiré seulement si l'égalité du plan médiateur est impossible, c'est-à-dire $L(C,A,B)>0$ ou $U(C,A,B)<0$, ou si la condition de Jung est impossible par la borne $8\,\mathrm{dist}(C,M_{AB})^{2}>D_{\max}^{2}(A,B)$. Le membre gauche emploie une distance inférieure de boîtes et le membre droit une distance maximale supérieure. Une égalité, une valeur non finie, un overflow, une sous-normale non maîtrisée ou tout intervalle traversant zéro conservent le patch.

Pour un patch encore faisable et un nœud témoin $W$ disjoint en plages de feuilles de $A\cup B$, les deux puissances coïncident sur tout centre pertinent. Les bornes renforcées sont donc $L_W=\max(L(C,A,W),L(C,B,W))$ et $U_W=\min(U(C,A,W),U(C,B,W))$. Si $L_W>0$, tous les points de $W$ sont strictement intérieurs et le parcours saute ses descendants; si $U_W\leq0$, aucun point du nœud n'est un témoin strict; sinon le LBVH descend et une feuille ambiguë ne compte pas. Les nœuds acceptés pour un patch forment ainsi une antichaîne de plages disjointes.

> **Théorème 5.** Si chaque patch non certifié infaisable possède une antichaîne témoin de masse au moins $s_{\max}-3$, le produit $A\times B$ ne contient aucune paire diamètre d'un support quatre de rang fermé au plus $s_{\max}$.

*Preuve.* Le centre d'un éventuel support appartient à au moins un patch du recouvrement. Ce patch ne peut avoir été certifié infaisable. Chaque point de son antichaîne est distinct des extrémités et strictement intérieur à la sphère réelle. Un point qui serait l'un des deux futurs carriers aurait au contraire une puissance nulle; la positivité stricte l'exclut donc automatiquement, sans disjonction préalable avec des carriers encore inconnus. Avec $s_{\max}-3$ intérieurs et les quatre points du support sur le shell, le rang fermé vaut au moins $s_{\max}+1$, contradiction. $\square$

Au rang fermé onze, huit témoins suffisent pour le support quatre. Le support trois demande $s_{\max}-2=9$ témoins. La disjonction entre témoins n'est requise qu'à l'intérieur d'un patch; les mêmes points peuvent certifier des patches différents. Une antichaîne doit en revanche être explicitement disjointe des deux blocs d'extrémités. Les doublons de coordonnées ne posent pas d'exception : les identifiants restent distincts, tandis qu'une coordonnée égale à un point de shell empêche la stricte positivité.

Avec $R=64$, $Q$ blocs d'extrémités visités, $V_W=\sum_{b,j}V_{b,j}$ visites totales de couples patch--nœud témoin, $a$ ancres résiduelles, $M=\sum_em_e$ lignes actives et $Z=\sum_eZ_e$, le coût cible est $O(n\log n+E_{\mathrm{prop}}+RQ+V_W+a+\sum_eV_e+\sum_e[m_e\log m_e+m_e(\kappa_e+1)]+Z+T_{\mathrm{exact}}+T_{\mathrm{sink}})$. Cette notation retire l'ambiguïté antérieure entre un nombre de visites déjà total et un facteur $R$ ajouté une seconde fois.

Dans le régime favorable $Q,V_W,a,M=O(n\,p(K))$, le coût est quasi linéaire en $n$ à $K$ fixé et le scratch témoin d'un bloc actif vaut $O(RK)$. Le pire cas reste explicitement dense : $Q=\Theta(n^{2})$, $V_W=\Theta(n^{3})$ pour $R$ fixe et $M=\Theta(n^{3})$ sont possibles; le cœur shallow vaut alors $O(n^{3}\log n)$ à $K$ fixé et peut redevenir $O(n^{4})$ lorsque $K=\Theta(n)$. La mémoire de travail tuilée vaut $O(n+S H_{\mathrm{LBVH}}+BRK+M_{\mathrm{tile}}+Z_{\mathrm{tile}})$ pour $S$ shards et $B$ blocs actifs, sans matrice de paires.

Une capacité prototype atteinte produit une continuation privée et rend ce profil `slo_eligible=false`. Elle n'autorise pas un « arrêt budgétaire » industriel. Le chemin industriel doit absorber le backpressure, reprendre la frontière en conservant $P_{\mathrm{prune}}+P_{\mathrm{emit}}+P_{\mathrm{pending}}=\binom{n}{2}$, puis rendre l'objet complet, ou échouer sur une ressource physique réelle. Il ne bascule jamais vers une mosaïque, une matrice de paires ou un catalogue global de cofaces et ne publie jamais `budget_exhausted`.

### 5.7 Conséquence pour le contrat

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

### 6.2 Deuxième tranche CUDA : frontière d'ancres complète, autorité bornée

La porte documentaire `P15-HOCUDA-P1` autorise l'implémentation de 5.5 sous `backend=cuda_g4_plus_reference_cpu_oracle`, `profile=hgp_reduced`, `mode=complete_fail_open_center_cover_anchor_superset_v1`. Son ABI minimale résidente relie une lease LBVH authentifiée à une lease move-only d'ancres contenant identités nuage--index, epoch, vues device, compteur device, contrôle et événement de disponibilité. `JungChordCsrTile` adopte cette lease en D2D; aucun upload d'ancres, callback hôte par niveau ou lecture de compteur intermédiaire n'est permis. Un ordonnanceur persistant multi-CTA traite des shards self-join disjoints, garde leur DFS privée et utilise une ring avec backpressure; une seule synchronisation terminale est admise.

La première tranche `P15-HOCUDA-P1a`, `backend=cuda_g4_plus_reference_cpu_oracle`, `profile=hgp_reduced`, `mode=proposal_only_center_cover_prune_mass_falsifier_v1`, est plus étroite : elle n'exécute que le center-cover support quatre et n'émet ni ancre ni paire. Deux compteurs device doivent certifier $P_{\mathrm{prune}}+P_{\mathrm{microtile}}=\binom{n}{2}$ sans tableau de paires et avec une seule synchronisation terminale. Les reçus par bloc sont bornés et réservés à $n=32$; le profil direct 50 k ne rapatrie que les agrégats de masse, patches, visites, stacks et équilibre CTA. P1a peut réfuter la sparsité pratique du mécanisme, jamais établir sa complétude ni fermer la sortie P1.

Cette porte n'est pas une autorité de support. Sa sortie peut seulement affirmer que toute paire diamètre canonique encore susceptible de porter un support trois ou quatre figure dans le flux d'ancres. Le différentiel rationnel à $n=32$ doit d'abord rejouer chaque prune, puis une unique session G4 exécute ce différentiel sous sanitizer et passe directement aux deux nuages 50 000. La route est no-go si source--cover plus cordes dépasse 400 ms chaud, si la majorité de la masse paire atteint les microtuiles, si la queue lourde sérialise le kernel, si $M$ ne tient pas son enveloppe de 200 ms, ou si l'ABI introduit une allocation, un memset ou une synchronisation par tuile ou niveau. Ces seuils falsifient P1; ils ne qualifient ni le `warm_e2e`, ni les dizaines de millions.

## 7. Ne pas confondre les deux RNG

Le RNG du graphe pondéré des facettes déjà découvertes est une réduction de connectivité valide : remplacer une arête par un chemin strictement plus léger préserve les composantes à tous les seuils. Il peut donc réduire un flux après génération.

Il ne découvre pas ses propres sommets ni arêtes. Construire naïvement le graphe des facettes revient à matérialiser les structures combinatoires interdites, et les incidences silencieuses déjà enregistrées empêchent de lire la seule réduction Gabriel comme une autorité de complétude. Le Théorème 1 réfute le RNG de points comme source par cliques; il ne réfute pas le RNG de facettes comme compression postérieure.

## 8. Portes falsifiables avant un nouveau test G4

1. Étendre l'oracle rationnel borné pour comparer, sur toutes les fixtures $n\leq14$, les supports acceptés exhaustivement et ceux retrouvés par disque, demi-plans et profondeurs; inclure permutations, égalités de diamètre, parallélisme et shell supplémentaire.
2. Implémenter un census CPU séparant les degrés et la dégénérescence du RNG, du graphe épaissi et de $G_\tau$, leurs arêtes manquantes contre l'oracle, puis $a$, $m_e$, $M$, profondeurs par niveau, $Z_e$, intersections brutes évitées, doublons, largeurs exactes et fallbacks, sur les trois familles de la porte P0 à $n=32$.
3. Refuser comme autorité tout graphe épaissi qui omet une arête de support; refuser aussi la route shallow si le travail du constructeur de niveaux reste proportionnel à $\sum_e m_e^{2}$ ou si sa sortie n'est pas bit-à-bit identique à l'oracle.
4. Fermer les rejets binary64 du prototype par intervalles extérieurs et repli exact, avec corpus ULP/`nextafter`; aucune comparaison flottante ne peut omettre un candidat.
5. La sous-porte `P15-HOCUDA-P0` autorise un backend device strictement `proposal_only`, destiné à mesurer le range-report et les demi-plans sur des ancres explicites; elle n'est jamais une source complète.
6. `P15-HOCUDA-P1a` vient en premier : à $n=32$, ses reçus bornés doivent rejouer exactement chaque prune et l'identité de masse; après build Release/audit et compute-sanitizer, la même session G4 va directement aux deux nuages 50 000 et ne rapatrie que les agrégats. Ce profiler sans émission peut uniquement falsifier la sparsité pratique et laisse la sortie P1 ouverte.
7. La sous-porte complète `P15-HOCUDA-P1` exige ensuite que le différentiel rationnel unique à $n=32$ retrouve chaque paire diamètre canonique et rejoue chaque prune, égalité, ambiguïté et reprise avant l'adoption D2D par les cordes. Source--cover plus cordes au-dessus de 400 ms chaud, un scheduler sériel, une synchronisation par niveau, une majorité de masse aux microtuiles ou un volume $M$ incompatible avec 200 ms classent P1 no-go; aucun palier intermédiaire ne retarde ce verdict.
8. Les dizaines de millions restent interdites tant que le pipeline exact complet à 50 000 points n'a pas terminé sous le contrat, avec reprise, mémoire et sortie aval réelles.

La preuve seule ne justifie toujours aucun kernel autoritaire de niveaux peu profonds. `P15-HOCUDA-P0` reste un falsificateur de travail et `P15-HOCUDA-P1` reste une spécification documentaire sans code; leurs sorties ne sont pas consommables scientifiquement. La campagne directe précédente confirme la frontière paire à 2,396 s, révèle que la germination de 120 s est CPU et archive l'arrêt ciblé des VM dans [`phase15_rng_jung_g4_20260808/RESULTATS.md`](../validation/phase15_rng_jung_g4_20260808/RESULTATS.md). Ni cette campagne ni les sous-portes ne remplacent le différentiel exact et aucun SLO n'est ouvert.

## 9. Références externes

- G. T. Toussaint, [*The relative neighbourhood graph of a finite planar set*](https://doi.org/10.1016/0031-3203(80)90066-7), 1980.
- T. Lim et R. J. McCann, [*Isodiametry, variance, and regular simplices*](https://www.math.toronto.edu/mccann/papers/LimMcCann19.pdf), annexe A pour une formulation moderne de Jung et son cas d'égalité.
- D. Attali, A. Lieutier et D. Salinas, [*Vietoris--Rips complexes also provide topologically correct reconstructions of sampled shapes*](https://dattali.github.io/Publications/2012-cgta-Rips.pdf), §2 pour le sandwich Rips--Čech issu de Jung.
- T. M. Chan, [*On the Bichromatic k-Set Problem*](https://doi.org/10.1145/1824777.1824782) ([copie auteur](https://tmc.web.engr.illinois.edu/bi.pdf)), pour les niveaux d'arrangements de demi-plans et la borne cumulative $\Theta(m\kappa)$. Le préprint ultérieur arXiv:1609.07709 est retiré au profit de ce résultat antérieur et n'est pas employé comme autorité.
- B. Aronov et S. Har-Peled, [*On Approximating the Depth and Related Problems*](https://doi.org/10.1137/060669474) ([copie auteur](https://sarielhp.org/p/04/depth/depth.pdf)), §4.2 pour la construction randomisée des faces de profondeur au plus $k$ en dimension deux.
- P. K. Agarwal, M. de Berg, J. Matoušek et O. Schwarzkopf, [*Constructing Levels in Arrangements and Higher Order Voronoi Diagrams*](https://doi.org/10.1137/S0097539795281840) ([copie auteur](https://pure.tue.nl/ws/portalfiles/portal/2495316/Metis201003.pdf)), pour les algorithmes de construction des premiers niveaux de droites.
- H. Edelsbrunner et G. Osang, [*A Simple Algorithm for Higher-order Delaunay Mosaics and Alpha Shapes*](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf), comme comparaison avec la matérialisation globale que le chemin produit MorseHGP3D évite.
