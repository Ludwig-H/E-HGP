# Première incidence sparse d'une facette — Phase 10.6-RCPU

## Portée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. La porte d'entrée de la phase est satisfaite. Le `public_status` reste `not_claimed`.

Le jalon 10.6-RCPU traite une unique facette canonique fournie par l'appelant et toutes ses cofaces obtenues par ajout d'un point. Il calcule exactement leur premier niveau d'incidence et conserve tous les points qui atteignent ce niveau, ou rend un épuisement de budget sans préfixe scientifique. Cette primitive ne choisit pas encore l'ensemble global des facettes qu'il faudra interroger et ne publie aucune naissance, racine, union, attache ou mutation de forêt.

Le chemin produit ne construit ni table de toutes les facettes, ni catalogue global de cofaces, ni incidence Gamma globale, ni cellule top-$m$, ni mosaïque de Delaunay d'ordre supérieur. Gamma exhaustif reste un oracle de falsification borné et n'entre ni dans le target, ni dans sa mémoire.

La base de preuve du contrat est `exact_one_point_coface_positive_support_reduction_weighted_source_support_radial_and_pair_aabb_lower_bound_strict_pruning_all_cominimizers_v1`. Sa portée est `single_supplied_facet_all_one_point_cofaces_only`.

## Première incidence et co-minimiseurs

Soit $P\subset\mathbb{Q}^{3}$ un nuage canonique fini et soit $F\subseteq P$ une facette non vide de cardinal $k$, avec $1\leq k\leq K\leq10$. Pour tout ensemble fini non vide $A$, notons $c_A$ le centre de son unique plus petite boule englobante fermée, et $\beta(A)$ son rayon carré exact. Posons $b=\beta(F)$, $c=c_F$ et $B_F=B(c,b)$.

Lorsque $P\setminus F$ n'est pas vide, le premier niveau d'incidence de $F$ est

$$\lambda_P(F)=\min_{x\in P\setminus F}\beta(F\cup\lbrace x\rbrace).$$

La famille complète des co-minimiseurs est

$$\mathcal{M}_P(F)=\left\lbrace x\in P\setminus F:\beta(F\cup\lbrace x\rbrace)=\lambda_P(F)\right\rbrace.$$

Si $P\setminus F$ est vide, l'API rend `complete_no_coface` et ne publie pas de valeur de $\lambda_P(F)$. On peut employer la convention mathématique $\lambda_P(F)=+\infty$, mais cette sentinelle n'entre pas dans le schéma exact.

Par monotonie de la miniball, toute coface vérifie

$$\beta(F\cup\lbrace x\rbrace)\geq\beta(F)=b.$$

Aux niveaux où $F$ est déjà présente, elle reste isolée dans le graphe Gamma des facettes tant que $\beta(F)\leq a<\lambda_P(F)$, puis devient incidente dans la coupe fermée au niveau $\lambda_P(F)$. Cette observation décrit l'incidence d'une facette donnée; elle ne prouve pas que le générateur global a interrogé toutes les facettes utiles.

## Borne radiale rationnelle

### Lemme du support pondéré

La miniball de $F$ possède un support positif $U=\lbrace p_1,\ldots,p_s\rbrace\subseteq F$, avec $s\leq4$, et des poids $\alpha_i>0$ tels que $\sum_i\alpha_i=1$, $c=\sum_i\alpha_ip_i$ et $\left\Vert p_i-c\right\Vert^2=b$ pour tout $i$. Pour tout centre $y$, l'identité de variance pondérée donne

$$\sum_i\alpha_i\left\Vert p_i-y\right\Vert^2=b+\left\Vert c-y\right\Vert^2.$$

Soit $x\in P\setminus F$, soit $s_x=\left\Vert x-c\right\Vert^2$, et considérons une boule de centre $y$ et de rayon carré $q$ contenant $F\cup\lbrace x\rbrace$. Avec $u=\left\Vert y-c\right\Vert$, $d=\sqrt{s_x}$ et $r=\sqrt{b}$, la boule satisfait

$$q\geq b+u^2,\qquad q\geq\left\Vert x-y\right\Vert^2\geq(d-u)^2.$$

Si $d>r$, posons $u_0=\frac{d^2-r^2}{2d}$. Lorsque $u\geq u_0$, la première borne est au moins $b+u_0^2$; lorsque $u\leq u_0$, la seconde est au moins $(d-u_0)^2$. Les deux valeurs coïncident :

$$b+u_0^2=(d-u_0)^2=\frac{(d^2+b)^2}{4d^2}=\frac{(s_x+b)^2}{4s_x}.$$

Définissons donc $R_b(s)=b$ pour $s\leq b$, et

$$R_b(s)=\frac{(s+b)^2}{4s}$$

pour $s>b$. Toute coface vérifie la borne radiale

$$\beta(F\cup\lbrace x\rbrace)\geq R_b(s_x).$$

Cette borne est rationnelle : elle ne dépend que des deux rayons carrés rationnels $s_x$ et $b$, et la branche divisante impose $s_x>b\geq0$, donc $s_x>0$. Les racines carrées utilisées dans la preuve ne sont jamais évaluées par l'implémentation.

### Extension à une AABB

Pour un nœud LBVH $N$ d'AABB exacte $A_N$, posons

$$\delta_z(N)=\min_{q\in A_N}\left\Vert q-z\right\Vert^2.$$

Tout site $x$ porté par $N$ vérifie $\left\Vert x-z\right\Vert^2\geq\delta_z(N)$. La fonction $R_b$ est croissante sur $[b,+\infty)$, car pour $t\geq s\geq b$ on a

$$R_b(t)-R_b(s)=\frac{(t-s)(st-b^2)}{4st}\geq0.$$

La continuité en $b$ et la branche constante sous $b$ donnent alors, pour tout site $x$ du nœud,

$$\beta(F\cup\lbrace x\rbrace)\geq L_{\mathrm{rad}}(N),\qquad L_{\mathrm{rad}}(N)=R_b(\delta_c(N)).$$

Les coordonnées binary64 canoniques et les bornes de boîte sont converties dans l'autorité rationnelle exacte avant cette comparaison. Aucun arrondi flottant ne décide un prune.

## Borne paire sur tous les points de la facette

Toute boule de rayon carré $q$ contenant deux points $p$ et $x$ vérifie, par l'inégalité triangulaire,

$$\left\Vert x-p\right\Vert\leq2\sqrt{q},\qquad q\geq\frac{\left\Vert x-p\right\Vert^2}{4}.$$

Comme une coface contient simultanément $x$ et chaque $p\in F$, on obtient la borne qui utilise toute la facette :

$$\beta(F\cup\lbrace x\rbrace)\geq\max_{p\in F}\frac{\left\Vert x-p\right\Vert^2}{4}.$$

Pour une AABB $A_N$, sa version minorante exacte est

$$L_{\mathrm{pair}}(N)=\max_{p\in F}\frac{\delta_p(N)}{4}.$$

La borne combinée du nœud est

$$L_F(N)=\max\left(L_{\mathrm{rad}}(N),L_{\mathrm{pair}}(N)\right).$$

Pour tout site admissible $x$ porté par $N$, elle satisfait $L_F(N)\leq\beta(F\cup\lbrace x\rbrace)$. La partie radiale exploite la miniball entière de $F$ par son support pondéré; la partie paire ajoute les contraintes de chacun des points de $F$. Prendre leur maximum reste donc sûr et peut seulement renforcer le branch-and-bound.

Calculer $L_F(N)$ demande exactement une distance AABB depuis $c$ et $k$ distances AABB depuis les points de $F$, soit $k+1\leq11$ évaluations exactes par nœud. Le préflight d'une expansion binaire réserve simultanément $2(k+1)$ évaluations avant d'engager ses deux enfants.

## Théorème des 176 supports à $K=10$

### Le point ajouté appartient au support hors de la boule source

**Théorème 10.6.1.** Si $x\notin B_F$, tout support positif minimal de la miniball de $F\cup\lbrace x\rbrace$ contient $x$.

**Preuve.** Soit $U_x$ un support positif minimal de cette coface. Supposons $x\notin U_x$. Alors $U_x\subseteq F$ et la sphère de $U_x$ est la miniball de $F\cup\lbrace x\rbrace$. La monotonie donne d'une part $\beta(U_x)\leq\beta(F)$, puisque $U_x\subseteq F$, et d'autre part $\beta(F)\leq\beta(F\cup\lbrace x\rbrace)=\beta(U_x)$. Les trois niveaux sont donc égaux. L'unicité de la miniball de $F$ impose que la boule de la coface soit $B_F$, mais cette boule contient $x$, contrairement à $x\notin B_F$. Ainsi $x\in U_x$. $\square$

En dimension trois, l'optimalité convexe puis le théorème de Carathéodory donnent un support positif minimal de cardinal au plus quatre. Pour un point strictement extérieur, il suffit donc d'énumérer $x$ avec zéro, un, deux ou trois points de $F$. Le nombre de supports candidats est

$$C_{\mathrm{out}}(k)=\sum_{j=0}^{3}\binom{k}{j}.$$

Cette quantité croît avec $k$. À la frontière produit $k=K=10$,

$$C_{\mathrm{out}}(10)=\binom{10}{0}+\binom{10}{1}+\binom{10}{2}+\binom{10}{3}=1+10+45+120=176.$$

Chaque sphère candidate est classifiée contre les $k+1$ points de la coface. Une coface extérieure à $K=10$ demande donc au plus

$$11\times176=1936$$

classifications exactes de points. Ce sont des bornes par point ajouté, indépendantes de $n$. Les supports non minimaux, affinement dépendants ou dont la boule n'enferme pas toute la coface sont rejetés exactement; si plusieurs supports décrivent la même miniball unique, le choix cardinal puis lexicographique ne change ni son centre, ni son niveau.

La miniball source, elle, peut nécessiter jusqu'à

$$\sum_{j=1}^{4}\binom{10}{j}=385$$

supports par passe. Sa construction et son rejeu frais sont deux passes séparées; le budget de source doit donc couvrir jusqu'à 770 examens lorsque $k=10$.

## Cas d'égalité et cas de Gabriel

**Proposition 10.6.2.** Pour tout $x\in P\setminus F$, on a

$$\beta(F\cup\lbrace x\rbrace)=\beta(F)\Longleftrightarrow x\in B_F.$$

**Preuve.** Si $x\in B_F$, la boule source contient déjà la coface; la monotonie dans les deux sens donne l'égalité. Réciproquement, si la coface possède le niveau $b$, sa miniball est une boule de rayon carré minimal $b$ contenant $F$. L'unicité de la miniball de $F$ impose qu'il s'agisse de $B_F$, donc cette boule contient $x$. $\square$

Il en résulte, lorsque $P\setminus F$ n'est pas vide,

$$\lambda_P(F)=b\Longleftrightarrow B_F\cap(P\setminus F)\neq\varnothing.$$

Le manuscrit définit un simplexe de Gabriel par la vacuité de l'intérieur de sa miniball :

$$\mathrm{int}(B_F)\cap(P\setminus F)=\varnothing.$$

Cette définition ouverte distingue trois cas exacts.

1. Si un point extérieur à $F$ est strictement intérieur à $B_F$, alors $F$ n'est pas de Gabriel et $\lambda_P(F)=b$.
2. Si aucun point n'est strictement intérieur mais qu'un point extérieur appartient à la frontière, alors $F$ est de Gabriel au sens ouvert et $\lambda_P(F)=b$. Tous les co-minimiseurs de frontière doivent être conservés.
3. Si $B_F\cap(P\setminus F)=\varnothing$, alors $F$ est Gabriel avec vacuité fermée et $\lambda_P(F)>b$. Chaque co-minimiseur est extérieur à la boule source et relève du théorème des 176 supports.

Sous la position générale du manuscrit, aucun point extérieur n'appartient à la frontière de la miniball d'un simplexe de cardinal au moins deux. Dans ce seul régime, les deux notions de vacuité coïncident et un simplexe de Gabriel ayant une coface vérifie $\lambda_P(F)>b$. L'implémentation 10.6-RCPU ne suppose pas cette généricité et ne supprime donc jamais le cas 2.

La fixture permanente à cinq points illustre le cas 1. Pour $F=AC$, on a $b=33/2$. Les points $D$ et $E$ sont strictement dans $B_F$, à distances carrées respectives $21/2$ et $31/2$ du centre source. Ainsi

$$\lambda_P(AC)=\frac{33}{2},\qquad\mathcal{M}_P(AC)=\lbrace D,E\rbrace.$$

Les deux cofaces `ACD` et `ACE` sont précisément les incidences silencieuses non-Gabriel qui doivent attacher `AC`; un tie-break qui n'en conserverait qu'une serait incorrect.

## Branch-and-bound exact et prune strict

Le builder calcule puis rejoue la miniball source. Il parcourt ensuite le LBVH avec une frontière ordonnée par $L_F(N)$. L'ordre `near_first` ou `far_first` change seulement le travail et l'ordre d'observation; il ne change pas la décision canonique finale.

Pour chaque nœud extrait et un incumbent courant $\mu$, la règle scientifique est exclusivement

$$L_F(N)>\mu\Longrightarrow N\ \text{est éliminé}.$$

Une égalité $L_F(N)=\mu$ force la descente. En effet, le nœud peut contenir un point $x$ tel que $\beta(F\cup\lbrace x\rbrace)=\mu$, et le contrat doit rendre tous les co-minimiseurs, pas seulement le premier incumbent. Un prune sur $\geq$ serait donc faux même si la valeur de $\lambda_P(F)$ restait correcte.

À une feuille, les points de $F$ sont exclus. Pour chaque autre point $x$ :

- si $x\in B_F$, la proposition d'égalité fournit directement le candidat de centre $c$ et de niveau $b$;
- si $x\notin B_F$, les au plus 176 supports contenant $x$ reconstruisent exactement la miniball de la coface;
- un niveau strictement meilleur remplace l'incumbent et efface l'ancien shell provisoire;
- un niveau égal ajoute le point au shell des co-minimiseurs.

Chaque co-minimiseur conserve un support positif témoin déterministe. Pour un point intérieur ou frontière, le support canonique source est réutilisé; `added_point_in_selected_positive_support` vaut donc faux, même si un autre support positif de la même boule pourrait contenir le point ajouté. Pour un point strictement extérieur, le support sélectionné contient nécessairement le point ajouté et ce champ vaut vrai. Le champ décrit uniquement le support témoin sélectionné, jamais l'existence dans un support quelconque ni l'appartenance à tous les supports.

**Théorème 10.6.3.** Si le parcours termine sans épuisement, le niveau rendu est $\lambda_P(F)$ et le vecteur rendu contient exactement $\mathcal{M}_P(F)$.

**Preuve.** La borne combinée est inférieure ou égale au niveau de chaque coface portée par son nœud. Tout point d'un nœud effectivement évalué reçoit sa miniball exacte par la proposition d'égalité ou le théorème du support extérieur. Lorsqu'un nœud est éliminé, sa borne est strictement supérieure à l'incumbent courant; tout incumbent ultérieur ne peut que décroître, donc aucun point de ce nœud ne peut atteindre le minimum final. Les nœuds de borne égale sont toujours descendus. À terminaison, chaque point admissible a ainsi été évalué ou strictement séparé du minimum, et tous les points d'égalité ont été retenus. $\square$

## Budgets et atomicité

Le contrat expose des plafonds fiables et séparés pour :

- les examens de supports de la miniball source;
- les visites de nœuds LBVH;
- les expansions de nœuds internes;
- les évaluations exactes de bornes AABB;
- les évaluations exactes de points candidats;
- les examens de supports de cofaces extérieures;
- les classifications point--sphère de ces supports;
- la taille de la frontière;
- le nombre de co-minimiseurs publiables.

Chaque plafond est contrôlé avant l'opération, l'incrément ou l'agrandissement correspondant. Les additions et multiplications de préflight ainsi que les compteurs échouent fermés sur dépassement de `size_t`.

Une expansion interne calcule les bornes exactes de ses enfants et vérifie la capacité requise avant de les engager dans la frontière. Une coface extérieure n'est candidate qu'après fermeture de toute son énumération locale et de toutes ses classifications. Un épuisement local ne laisse donc ni support, ni centre, ni niveau partiel dans la sortie scientifique.

Le plafond des co-minimiseurs possède une subtilité nécessaire. Un shell égal à l'incumbent courant peut dépasser provisoirement sa capacité, puis devenir inutile si un meilleur incumbent apparaît plus tard. Le builder mémorise ce dépassement sans publier de préfixe; un meilleur niveau efface le shell et le drapeau. Si le shell surchargé reste minimal à la fin du parcours, la décision devient `budget_exhausted`, le niveau et tous les co-minimiseurs sont effacés. La sortie est donc tout-ou-rien pour la famille complète $\mathcal{M}_P(F)$. Sur cette issue, `no_partial_first_incidence_payload_published` vaut vrai tandis que `all_cominimizers_retained_atomically` vaut faux; seule une sortie complète peut certifier que tous les co-minimiseurs ont réellement été retenus.

Les trois seules issues certifiées sont : `complete_no_coface`, `complete_exact_first_incidence` avec tous les co-minimiseurs, et `no_first_incidence_budget_exhausted` sans niveau ni co-minimiseur. Un budget ne prouve jamais une absence de coface, une isolation, un statut Gabriel ou une attache.

## Vérification fraîche, sans reçu durable

Le vérificateur reçoit séparément le nuage, le LBVH, la clé source, les budgets et l'ordre de parcours fiables. Il borne d'abord le stockage observé, puis reconstruit la miniball source, le branch-and-bound, les compteurs, le niveau, tous les co-minimiseurs et la décision. Aucun champ scientifique observé ne choisit les nœuds ou points du rejeu attendu.

L'égalité structurelle avec cette reconstruction fournit un rejeu frais au moment de l'appel. Elle ne fournit pas un reçu durable : aucun manifeste, digest d'autorité, checkpoint, journal de chunks ou preuve persistante ne permet de reprendre après perte de processus ou de valider ultérieurement le résultat sans refaire le calcul depuis les autorités. La primitive ne possède pas non plus d'annulation coopérative ni de reprise incrémentale.

## Architecture évitée et coût intermédiaire

Pour une requête, le stockage scientifique contient une clé de largeur $K\leq10$, une miniball source locale, un niveau optionnel et $m=\lvert\mathcal{M}_P(F)\rvert$ co-minimiseurs de taille constante. Le scratch géométrique conserve un support de quatre points au plus et une frontière LBVH bornée par l'appelant. En nombre d'enregistrements,

$$M_{\mathrm{query}}=O(K+m+W),$$

où $W$ est le pic de frontière autorisé. Aucun facteur $\binom{n}{k}$ ou $\binom{n}{k+1}$ n'apparaît dans cette requête isolée.

Le jalon évite explicitement de construire :

- les $\binom{n}{k}$ facettes possibles;
- les $\binom{n}{k+1}$ cofaces possibles;
- une table globale d'incidences facette--coface;
- les coupes, composantes ou hyperarêtes de Gamma;
- les cellules top-$m$ et leurs cofaces;
- la mosaïque de Delaunay d'ordre supérieur;
- un graphe de Gabriel global, une forêt HGP ou une arène de racines.

Le LBVH global existe déjà comme index spatial de points; 10.6-RCPU ne lui ajoute aucune structure combinatoire d'ordre $k$. Pour $K\leq10$, le travail local par point extérieur est borné par 176 supports et 1936 classifications. Un appel complet reste donc linéaire en nombre de points ou nœuds visités dans son pire cas, et non combinatoire en $n$. Appeler naïvement cette primitive sur toutes les facettes possibles recréerait toutefois l'architecture interdite sous un autre nom; le choix parcimonieux des facettes sources reste une obligation séparée.

## Limites honnêtes

Les bornes précédentes comptent des opérations et des enregistrements, pas les bits. Les coordonnées et niveaux exacts utilisent des entiers et rationnels multiprécision sans plafond de limbs. Aucun budget actuel ne borne la taille des numérateurs et dénominateurs, les octets temporaires, le coût d'un PGCD ou le temps d'une opération rationnelle. Une borne de 176 supports ne constitue donc pas une borne uniforme en temps machine.

La sortie elle-même peut être de taille $\Theta(n)$. Par exemple, si $\Theta(n)$ points de $P\setminus F$ appartiennent à $B_F$, toutes leurs cofaces ont le niveau $b$ et tous ces points appartiennent à $\mathcal{M}_P(F)$. Le branch-and-bound doit descendre les nœuds d'égalité et le résultat exact doit les conserver tous; aucun algorithme matérialisant cette sortie ne peut garantir une mémoire sous-linéaire. Le plafond de co-minimiseurs permet seulement une tentative bornée avec échec fermé.

Le pire cas visite aussi $\Theta(n)$ points ou nœuds et la frontière peut atteindre $\Theta(n)$. La forme sparse est compatible avec une exécution rapide lorsque les bornes élaguent fortement et que $m$ reste petit, mais elle ne qualifie ni le SLO de 50 000 points sous la seconde, ni la voie 10 000 000+ points. Ces deux cibles exigent encore des mesures, des budgets de limbs et d'octets, puis éventuellement un flux externe des co-minimiseurs avec un certificat d'exhaustivité.

Enfin, 10.6-RCPU certifie la première incidence de la seule facette fournie. Il ne prouve ni que toutes les facettes-portes utiles seront proposées, ni la complétude globale des gateways silencieux, ni la correction d'une réduction fondée sur Gabriel seul, ni M.1, ni une attache HGP. Un épuisement reste `unresolved` pour les étages supérieurs; même un niveau complet ne peut promouvoir aucun `public_status=exact` sans les raccords et preuves encore ouverts.

La validation GCC Release ciblée exécute neuf CTests en 2,15 secondes lors du dernier rejeu : LBVH, locator, pas 10.5b, fermeture 10.5c, première incidence et leurs quatre gardes statiques ou symboliques. Elle couvre notamment les autorités invalides, l'intérieur et la frontière, la fixture permanente `AC` avec niveau $33/2$ et co-minimiseurs $D,E$, l'effacement d'un overflow provisoire par un meilleur incumbent, un puis deux minimiseurs extérieurs à $K=10$, les neuf budgets exacts et moins-un, les mutations hostiles, un différentiel contre les miniballs exhaustives pour toutes les facettes propres d'un nuage de six points et toutes les cofaces d'une facette fournie à $n=14$.

## Statut des énoncés

| énoncé | statut | limite |
|---|---|---|
| définition de $\lambda_P(F)$ et famille complète $\mathcal{M}_P(F)$ | `definition` | une facette source fournie |
| borne radiale rationnelle pondérée | `proved_here` | suppose la miniball source exacte |
| borne paire sur tous les points de $F$ | `proved_here` | renforçante, jamais utilisée seule comme décision |
| réduction des cofaces extérieures à au plus 176 supports pour $K=10$ | `proved_here` | dimension trois et $K\leq10$ |
| équivalence niveau égal--appartenance à la boule source fermée | `proved_here` | aucune position générale requise |
| complétude du branch-and-bound avec prune strict | `proved_here` | seulement sans épuisement et avec LBVH exact validé |
| builder et vérificateur frais 10.6-RCPU | `validated_host_software` | aucun reçu durable ni budget de limbs |
| complétude globale des premières incidences utiles au journal | `proof_obligation` | le générateur des facettes sources reste à fermer |
| exactitude de Gabriel brut pour `hgp_reduced` | `false_in_general` | contre-exemple permanent à cinq points |
| SLO 50 k sous la seconde et qualification 10 M+ | `experimental_target` | pire cas et sortie linéaires |
