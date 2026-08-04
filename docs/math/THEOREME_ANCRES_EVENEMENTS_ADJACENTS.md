# Théorème des ancres entre événements critiques de rangs adjacents

> **Statut scientifique.** Les parties locales 4.1 à 4.3 sont démontrées sous les hypothèses strictes de la section 3; l'identification forestière de la section 4.4 reste conditionnelle aux bindings O3, à un état horizontal inférieur déjà exact et à un lot O4 complet. Le lemme de facette commune de la section 5 démontre la verticale relativement à des composantes Gamma complètes. La production sparse totale de ces composantes, l'adéquation de tous les lots événementiels et la reconstruction M.1 restent des obligations de preuve. Ce document interdit donc toute promotion de `full_pi0`, de M.1 ou du statut public `exact` par le seul certificat local décrit ici.

## 1. Résultat recherché

Le morphisme vertical ne doit pas être construit en supprimant opportunément des points d'un représentant déjà réduit. Son unité sémantique est un **même événement critique exact** observé dans deux ordres adjacents.

Soit $X\subset\mathbb{R}^{3}$ fini, $n=\lvert X\rvert$, et soit $e=(c,a,I,U)$ un événement critique générique, où $I=X\cap\mathring{B}(c,\sqrt{a})$ et $U=X\cap\partial B(c,\sqrt{a})$. On pose

$$S_e=I\cup U,\qquad s_e=\lvert S_e\rvert.$$

Lorsque $2\leq s_e\leq K_{\mathrm{eff}}$, le même événement possède un rôle de naissance à l'ordre $s_e$ et un rôle de selle d'indice un à l'ordre $s_e-1$, au même niveau carré exact $a$. Si $\rho_e$ désigne le rayon employé dans le manuscrit, notre convention est $a=\rho_e^2$. Cette dualité fournit l'ancre verticale primitive. Les flèches vers des ordres plus bas sont exclusivement des compositions de ces ancres adjacentes.

L'**ordre** $k$, le **rang fermé** $s_e=\lvert I\rvert+\lvert U\rvert$ et l'**arité du support minimal** $\lvert U\rvert$ sont trois quantités distinctes. En dimension trois, un événement peut avoir un rang fermé élevé tout en étant déterminé par deux, trois ou quatre points de support. Le terminal de la descente a rang fermé exactement $k$, même si son support minimal possède une arité comprise entre un et quatre. Le journal vertical est indexé par le rang fermé; les noyaux géométriques exacts ne traitent que l'arité du support.

Le théorème utile se décompose en trois assertions :

1. chaque bras de la selle de rang $s_e$ descend strictement vers un événement de naissance de rang exactement $s_e-1$;
2. les bindings O3 transportent les terminaux vers des handles de $L_k^{<}(a)$; relativement à un état inférieur déjà exact et à un lot O4 complet, contenant toutes les naissances, selles et incidences requises, la contraction simultanée donne la composante fermée cible de la naissance supérieure portée par $e$;
3. la verticale d'une composante entière est indépendante du représentant, puis se propage horizontalement de façon naturelle.

## 2. Briques du manuscrit et limite de leur portée

Les résultats du manuscrit employés sont les suivants.

| résultat | pages PDF / manuscrit | rôle exact ici |
|---|---:|---|
| Théorème 2 | 86–87 / 60–61 | identification naturelle entre $\pi_0(L_k(a))$ et les composantes de $\Gamma_k(a)$ |
| Proposition 5 | 112 / 86 | suffisance des adjacences élémentaires par cofaces de cardinal $k+1$ |
| Définition 27 | 112–113 / 86–87 | facettes actives strictes et simplexes séparants |
| Théorème 4 | 114–115 / 88–89 | tout simplexe séparant est Gabriel sous position générale |
| Théorème 6 | 118–119 / 92–93 | un simplexe Gabriel est porté par la mosaïque de Delaunay de son ordre |
| Théorème 7 | 124–125 / 98–99 | un $k$-simplexe Gabriel se retrouve depuis deux facettes actives portées à l'ordre $k-1$ |

Le Théorème 7 est une brique de **génération inverse de candidats** : deux facettes actives peuvent révéler le simplexe supérieur. Il n'établit ni que ces deux facettes sont des événements critiques de rang inférieur, ni la totalité des attaches, ni la contraction simultanée, ni la naturalité verticale. De plus, la Proposition 6 et le Théorème 5 du manuscrit sont `false_in_general` dans leur formulation élaguée actuelle, à cause des incidences silencieuses de la fixture exacte à cinq points. Ils ne peuvent donc pas servir d'autorité à la tour v2.

Le théorème ci-dessous utilise le manuscrit pour la structure Gamma et la visibilité Gabriel, puis la théorie de Morse de la distance K-NN et la descente exacte de miniballs pour établir la relation événement–événement.

## 3. Hypothèses strictes

Fixons $1\leq k<n$ et un événement $e=(c,a,I,U)$ de rang fermé $k+1$. Les hypothèses cumulatives sont :

- $S=I\cup U$ contient exactement $k+1$ points;
- $2\leq\lvert U\rvert\leq4$;
- $U$ est l'unique support minimal de la boule critique, est affinement indépendant, et $c\in\mathrm{relint}\,\mathrm{conv}(U)$;
- les coordonnées barycentriques des points de $U$ sont strictement positives;
- le shell extérieur est vide : aucun point de $X\setminus S$ n'est sur la sphère critique;
- toute facette visitée $F$, terminale comprise, possède un certificat `Proper(F)` : son support minimal essentiel $U(F)=F\cap\partial B(c_F,\sqrt{\beta(F)})$ est unique, affinement indépendant et contient $c_F$ dans l'intérieur relatif de son enveloppe convexe;
- pour chaque facette intermédiaire non-Gabriel $F$, un certificat distinct conserve en outre la partition exacte intérieur--shell--extérieur de $X$ relativement à sa miniball, l'absence d'extra-shell, le point extérieur strictement intérieur qui entre dans le choix top-$k$ et la décroissance stricte du successeur;
- les requêtes top-$k$, les centres, les niveaux, les comparaisons et les partitions de shell sont exacts;
- aucun plateau n'est rencontré; une égalité non couverte arrête le certificat au lieu d'être perturbée;
- tous les événements et tous les bras requis par le périmètre sont présents.

Ces hypothèses sont plus fortes que la seule absence de dégénérescence parmi les événements déjà acceptés. Le caractère `Proper` est certifié individuellement pour toute facette visitée. Deux autorités d'absence d'extra-shell sont ensuite nécessaires et ne doivent pas être confondues : `RelevantGP` certifie les facettes Gabriel `Proper` acceptées, tandis que le certificat individuel de chaque facette intermédiaire non-Gabriel recalcule sa partition globale, son absence d'extra-shell et sa décroissance. L'antécédent de `RelevantGP` est faux précisément lorsqu'un intrus strict rend cette facette non stationnaire; `RelevantGP` ne peut donc pas certifier cette étape de descente.

## 4. Théorème descendant événement–événement

### 4.1 Dualité locale des rôles

Pour un événement de rang fermé $s$, posons $N_c=\lvert I\rvert+\lvert U\rvert=s$, $N_c^{I}=\lvert I\rvert$ et $N_c^{\partial}=\lvert U\rvert$. Le Théorème 1 de Reani--Bobrowski donne, pour chaque ordre admissible $N_c^{I}<q\leq N_c$, le même centre critique, le même niveau et l'indice local $\mu_q=N_c-q=s-q$. Comme $\lvert U\rvert\geq2$, les deux ordres $q=s$ et $q=s-1$ sont admissibles. Avec $s=k+1$, on obtient

$$\mu_{k+1}=0,\qquad\mu_k=1.$$

Le même objet géométrique et le même identifiant d'événement portent donc une naissance à l'ordre $k+1$ et une selle à l'ordre $k$. Il ne s'agit pas de deux événements rapprochés par une comparaison flottante.

### 4.2 Bras stricts

Pour chaque $u\in U$, le bras local est la facette

$$F_u=S\setminus\lbrace u\rbrace.$$

Chaque point de $U$ est essentiel. Retirer $u$ donne par conséquent

$$\beta(F_u)<\beta(S)=a.$$

Les $\lvert U\rvert$ bras sont tous conservés, même si plusieurs d'entre eux atteignent ultérieurement la même composante. La multiplicité locale $\lvert U\rvert-1$ n'autorise jamais à supprimer un bras.

### 4.3 Descente vers un minimum de rang exactement inférieur

Pour une facette $F$ de cardinal $k$, soit $c_F$ le centre de sa miniball et soit $\mathcal{N}_k(c_F)$ la famille exacte de ses choix top-$k$. Dans cette section seulement, « active en son propre centre » signifie $F\in\mathcal{N}_k(c_F)$; cette stationnarité top-$k$ ne doit pas être confondue avec les facettes actives strictes de la Définition 27. Si $F\notin\mathcal{N}_k(c_F)$, le shell global étant exactement son support essentiel, tout choix top-$k$ retire au moins un point de ce support pour faire entrer un point strictement intérieur. Un successeur n'est admissible qu'après certification exacte de

$$\beta(G)<\beta(F).$$

La valeur $\beta$ décroît strictement sur l'ensemble fini des facettes de cardinal $k$. Toute descente admissible termine donc sur une facette $M_u\in\mathcal{N}_k(c_{M_u})$. Cette appartenance exclut tout point de $X\setminus M_u$ strictement intérieur à la miniball : un tel point remplacerait un point maximal de $M_u$ dans le choix top-$k$. L'absence d'extra-shell exclut ensuite toute égalité extérieure, donc $B_{M_u}\cap X=M_u$. La facette terminale définit ainsi un événement critique de rang fermé exactement $k$, d'indice zéro à l'ordre $k$.

Le chemin continu est certifié, pas seulement la suite discrète des valeurs $\beta$. Pour chaque successeur $F\to G$, avec $\gamma(t)=(1-t)c_F+tc_G$, l'identité quadratique de corde et le choix top-$k$ donnent sur le demi-segment engagé

$$D_k(\gamma(t))\leq g_G(\gamma(t))\leq(1-t)g_G(c_F)+t\beta(G)-t(1-t)\left\Vert c_G-c_F\right\Vert^2<\beta(F),\qquad0<t\leq1.$$

Pour le segment initial $\gamma_u(t)=c+t(c_{F_u}-c)$, la positivité barycentrique de $u$ donne un coefficient sortant strictement positif. Avec $A_p=\left\Vert c-p\right\Vert^2-a$ et $B_p=2(c-p)\mathbin{\cdot}(c_{F_u}-c)$, la borne rationnelle

$$\tau_u=\min\left(\left\lbrace1\right\rbrace\cup\left\lbrace\frac{A_p}{-2B_p}:p\in X\setminus S,\ B_p<0\right\rbrace\right)>0$$

maintient $u$ et tous les points extérieurs hors du niveau sur $0<t\leq\tau_u$; elle identifie donc le germe $u$. Les certificats analytiques complets sont ceux de la section 7.1 et du lemme de corde de [Attaches par descente de miniball](ATTACHES_DESCENTE_MINIBALL.md). La polyligne privée de $c$ reste dans $L_k^{<}(a)$, donc le bras et le minimum terminal appartiennent à la même composante topologique.

Un `pre_batch_component_handle` désigne cette composante stricte sous une forme typée : racine exacte `full_pi0`, racine réduite lorsqu'elle existe, ou carrier de naissance latent pour une composante singleton omise par `hgp_reduced`. La liaison O3 à ce handle ne découle pas du seul chemin.

> **Théorème A — descente adjacente.** Sous les hypothèses de la section 3, chaque bras d'un événement de rang $k+1$ possède un chemin exact strict vers au moins un événement de naissance de rang exactement $k$, de niveau strictement inférieur. Un certificat de chemin désigne cette naissance terminale; un certificat O3 séparé la lie à un `pre_batch_component_handle` complet. Aucun chemin ne désigne directement un événement de rang plus bas.

La descente peut être plusieurs-vers-un. Deux choix admissibles peuvent aussi terminer sur des naissances différentes de rang $k$; leurs chemins les placent dans la même composante topologique du bras, puis le certificat O3 doit résoudre leurs handles dans le même snapshot strict et vérifier leur égalité. Une racine publique et un carrier latent singleton ne peuvent pas demeurer deux représentations distinctes de cette même composante : le mismatch échoue fermé, sauf si un binding explicite résout le carrier vers cette racine dans le snapshot. Une règle canonique accélère la construction, mais ne remplace pas cette indépendance sémantique.

### 4.4 Ancre naissance–selle

Considérons d'abord l'objet topologique exact. Notons $C_e$ l'unique composante de $L_k(a)$ qui contient le centre critique $c$, donc aussi les germes fermés de $e$. Le carrier formel $B_e$ de la naissance d'ordre $k+1$ est introduit avant le quotient du lot source; il n'est pas nécessairement une composante source fermée autonome.

Le centre $c$ appartient à $L_{k+1}(a)$ et à $L_k(a)$. L'inclusion fournit donc une ancre du carrier $B_e$ vers $C_e$. Si $B_e$ survit seul après le lot source, $C_e$ est l'image de sa composante. Si une selle distincte du même niveau le fusionne immédiatement avec d'autres carriers, l'image de la composante source post-lot n'est définie qu'après vérification que toutes leurs ancres inférieures coïncident.

Supposons maintenant que l'état horizontal strict inférieur soit déjà exact et que le lot fermé $(k,a)$ soit certifié complet. Les bindings O3 transportent les terminaux vers leurs `pre_batch_component_handle` figés dans $L_k^{<}(a)$. Le lot O4 doit inclure toutes les naissances simultanées, toutes les selles, les facettes égales obtenues en retirant un point intérieur de $I$ et toutes les incidences silencieuses nécessaires; ses hyperarêtes sont ensuite contractées simultanément. Le handle post-lot $Q_e$ du groupe contenant le rôle selle de $e$ représente alors $C_e$.

> **Théorème B — ancre critique adjacente conditionnelle.** Relativement à un état horizontal inférieur exact et à un lot complet certifié conforme, le carrier de naissance de rang $k+1$ porté par $e$ s'ancre vers l'unique handle post-lot $Q_e$ du rôle selle du même événement à l'ordre $k$. La cible d'une composante source post-lot exige en plus la convergence des ancres de tous ses carriers. Aucune ancre publique fermée n'est posée sur l'état pré-lot.

La partie topologique de ce résultat est l'ancre $B_e\mapsto C_e$. L'identification $C_e\leftrightarrow Q_e$ est conditionnelle à toutes les prémisses du Théorème B : état horizontal exact, lot complet, O3 et O4. Un locator positif ou une fenêtre Morton ne fournit pas à lui seul cette totalité.

### 4.5 Corollaire de visibilité ascendante du manuscrit

Supposons ici $k\geq2$ et la position générale globale de la Définition 26 du manuscrit. La boule critique de $e$ ne contient aucun point de $X\setminus S$ dans son intérieur. Le label $S$, de cardinal $k+1$, est donc un $k$-simplexe de Gabriel. Choisissons deux points distincts $u,v\in U$. Les facettes $F_u$ et $F_v$ sont actives strictes et vérifient

$$F_u\cup F_v=S.$$

Le Théorème 7 du manuscrit affirme que ces deux facettes sont portées par des arêtes élémentaires de $\mathrm{Del}_{k-1}(X)$. Les deux certificats de descente de la section 4.3 les relient en outre à des événements de naissance de rang $k$. Tout événement de rang $k+1$ possède ainsi un témoin dont le support géométrique minimal contient au plus quatre points; chacune de ses facettes porte néanmoins $k$ identifiants, et chaque chemin possède seulement la borne combinatoire $\binom{n}{k}-1$ ou un budget explicite, pas une longueur constante.

Ce corollaire fournit une base de candidats supérieurs : une source sparse complète de facettes portées peut joindre deux facettes dont l'union a cardinal $k+1$, puis classifier exactement le support minimal de leur union. Cette voie devient sous-exhaustive tout en restant complète seulement avec un schéma d'ownership et de pruning dont la preuve est séparément établie. Il ne faut pas inverser la conclusion : les deux événements terminaux seuls ne déterminent pas nécessairement $S$, plusieurs bras peuvent descendre vers la même naissance, et le catalogue des minima de rang $k$ n'est pas une source exhaustive de facettes portées. Le chemin ou au moins la facette active initiale doit rester dans le témoin, et tout prune Morton doit certifier qu'aucune paire canonique portée n'a été omise.

Pour $k=1$, $S$ est une paire de Gabriel et la brique de visibilité est la triangulation de Delaunay ordinaire, avec l'EMST comme certificat de la forêt H0. Le symbole $\mathrm{Del}_0$ n'est ni requis, ni introduit.

### 4.6 Crosswalk exécutable vers le quotient O4 résident

Fixons un lot inférieur $(k,a)$ déjà construit par l'autorité résidente normalisée. Notons $\mathcal{E}_{k,a}$ son hypergraphe typé figé : il contient les hyperarêtes des selles directes, les incidences résiduelles ou silencieuses, les carriers enracinés ou latents et les facettes égales. Soit $q_{k,a}$ le quotient en composantes connexes de cet unique hypergraphe. Pour le rôle selle inférieur d'un événement $e$ de rang $k+1$, l'autorité de plan donne une unique référence directe, la construction figée donne une unique hyperarête $h_e$, et le quotient donne un unique groupe $g_e=q_{k,a}(h_e)$.

Le crosswalk persistant ne compare jamais les valeurs numériques des identifiants de racines de la forêt événementielle et du résident. Il engage l'identité de projection de $e$, la référence directe, l'indice de $h_e$, le groupe $g_e$, le niveau exact, l'ordre, l'identité du snapshot et la racine résultante du groupe après commit. Son digest engage aussi les nombres d'hyperarêtes directes et résiduelles, de tokens typés et de groupes du lot. Ainsi, une selle directe ne peut pas être déplacée vers un autre groupe en modifiant de concert le record et sa racine.

Pour $k\geq2$, la cible exécutable de l'ancre est la racine résidente post-lot du groupe $g_e$. Les hyperarêtes silencieuses n'ont pas besoin de porter un identifiant d'événement : leur présence dans $\mathcal{E}_{k,a}$ influe déjà sur $q_{k,a}$, et donc sur le groupe auquel $h_e$ est lié. Le binding selle--groupe est total seulement si chaque selle directe du lot apparaît exactement une fois et si l'autorité vivante certifie que le lot normalisé inclut toutes les incidences exigées par son périmètre.

Le cas de base $k=1$ est différent, car le plan résident exclut volontairement l'ordre un au profit de l'autorité Borůvka. Pour un événement $e$ de rang deux, sa naissance d'ordre deux retient la facette à deux points $S_e$. Après avancement de la coupe K1 fermée jusqu'à $a$, les deux requêtes singleton doivent rendre la même racine K1 authentifiée. Cette racine est le binding de base de $e$; l'absence, l'inégalité ou un digest de nuage différent fait échouer le crosswalk.

> **Proposition C — crosswalk O4 conditionnel.** Si le namespace des projections événementielles et les digests de nuage coïncident, si les plans et lots normalisés sont complets dans leur périmètre certifié, et si l'autorité K1 est exacte, alors chaque naissance de rang $r\geq2$ possède exactement une cible adjacente exécutable : la racine K1 fermée du même événement pour $r=2$, et la racine post-lot du groupe $q_{r-1,a}(h_e)$ de sa selle pour $r\geq3$. Cette proposition certifie le membership événement--lot et non l'adéquation topologique générale O4 à `full_pi0`.

La racine cible de la Proposition C remplace l'ancre forestière conditionnelle dans le chemin produit. Une étape séparée doit encore vérifier que toutes les ancres des carriers d'un même groupe source convergent, puis que ces bindings couvrent les composantes réduites et latentes requises. Le crosswalk ne positionne donc à lui seul ni `vertical_maps_complete`, ni `global_morse_obligation_replayed`, ni `m1_replayed`.

## 5. Lemme vertical de facette commune

La verticale possède une formulation exacte dans Gamma, indépendante de la descente choisie. Soit $C$ une composante de $\Gamma_{k+1}(a)$. Choisissons un label $Q\in C$, puis une facette $F\subset Q$ de cardinal $k$. Définissons $V_{k,a}(C)$ comme la composante de $\Gamma_k(a)$ contenant $F$. Lorsque $k=n-1$, on emploie l'extension contractuelle $\Gamma_n(a)$, réduite au label terminal $X$ lorsqu'il est actif; ce bord n'est pas fourni directement par la définition du manuscrit.

On emploie ici les adjacences élémentaires de la Proposition 5. Cette définition est indépendante de $F$, car toutes les facettes de $Q$ sont reliées par la coface active $Q$. Elle est indépendante de $Q$, car deux labels élémentairement adjacents $Q,Q'$ de la source partagent la facette $Q\cap Q'$ de cardinal $k$. Un chemin source propage donc une cible unique.

Pour rendre explicite la compatibilité géométrique, notons $T_a(A)=\bigcap_{x\in A}B(x,\sqrt{a})$. Si $F\subset Q$, alors $T_a(Q)\subseteq T_a(F)$. Les régions témoins de chaque label source sont donc incluses dans celles de ses facettes cibles, et une facette commune transporte l'intersection le long de tout chemin source.

Pour $a\leq b$, le même label et la même facette restent actifs. Il vient

$$h_{k,a,b}\circ V_{k,a}=V_{k,b}\circ h_{k+1,a,b}.$$

Par le Théorème 2 du manuscrit, $V_{k,a}$ est exactement l'application induite par l'inclusion $L_{k+1}(a)\subseteq L_k(a)$. La naturalité topologique est donc démontrée relativement à des memberships et bindings Gamma complets. La production sparse de témoins couvrant toute composante source reste séparément à certifier.

## 6. Propagation dans une forêt et composition des ordres

Une ancre élémentaire est attachée au carrier de naissance, même si ce carrier est latent dans `hgp_reduced`. Pour un nœud source ultérieur de niveau $b$, on avance horizontalement dans la forêt inférieure chacune des ancres de ses carriers jusqu'à $b$. Le nœud possède une cible verticale si et seulement si toutes ces images coïncident. Cette égalité est une vérification, pas un choix de représentant.

Lors d'une multifusion source, les images de tous les enfants doivent de même converger après le lot inférieur fermé de même niveau. La cible commune devient l'ancre du parent. Ce procédé établit le lemme conditionnel de propagation seulement si la totalité des carriers, l'exactitude des forêts horizontales et le rejeu de tous les lots sont déjà certifiés; il ne ferme pas O7 par construction. Sous ces prémisses, l'induction vérifie alors chaque carré de naturalité.

L'événement de rang $k+1$ possède des rôles H0 primitifs seulement aux ordres $k+1$ et $k$. Pour $j<k$, la flèche correcte est

$$V_{j,k+1,a}=V_{j,a}\circ V_{j+1,a}\circ\cdots\circ V_{k,a}.$$

Une implémentation peut mettre cette composition en cache. Elle ne doit pas inventer un raccourci « rang $k+1$ vers rang $j$ » dont l'égalité avec cette composition n'est pas certifiée.

## 7. Lots égaux et interactions entre événements

Les niveaux égaux ne sont pas traités par une perturbation symbolique cachée. Pour chaque couple $(k,a)$, la transaction sémantique est :

1. figer les composantes de $L_k^{<}(a)$;
2. introduire toutes les naissances de rang $k$ du niveau;
3. résoudre tous les bras de toutes les selles de rang $k+1$ sur l'état strict figé;
4. former un hypergraphe unique incluant les incidences certifiées du lot;
5. calculer toutes ses composantes sans mutation scientifique;
6. committer les unions et les naissances;
7. poser les ancres vers l'état inférieur fermé post-lot.

La fermeture d'équivalence d'un hypergraphe est indépendante de l'ordre des hyperarêtes. L'obligation O4 demeure néanmoins ouverte : il faut démontrer que les événements et incidences produits représentent toujours exactement le passage de $L_k^{<}(a)$ à $L_k(a)$, notamment lorsque des naissances et selles distinctes partagent le niveau.

## 8. Cas terminal $s=n$ lorsque $K_{\mathrm{eff}}=n$

Le contrat produit courant adopte l'hypothèse plus forte $n\geq K_{\mathrm{eff}}+2$. Le cas terminal décrit dans cette section reste donc supporté par les composants mathématiques génériques et par les fixtures, mais il est hors du périmètre de qualification industrielle actuel. Cette marge exclut aussi le cas $n=K_{\mathrm{eff}}+1$ où l'événement terminal de rang $n$ serait encore une selle au dernier ordre demandé; au sommet produit, seules les naissances de rang au plus $K_{\mathrm{eff}}$ et les selles requises de rang au plus $K_{\mathrm{eff}}+1<n$ sont admises.

Lorsque la tour demandée atteint effectivement l'ordre terminal et $n\geq2$, c'est-à-dire $K_{\mathrm{eff}}=n\geq2$, il n'existe aucun événement de rang $n+1$. En revanche, l'événement de rang $n$ porté par $S=X$ est obligatoire. On a

$$L_n(a)=\bigcap_{x\in X}B(x,\sqrt{a}).$$

Cet ensemble est vide pour $a<\beta(X)$, puis non vide et convexe pour $a\geq\beta(X)$. Il possède une unique composante, née au niveau $\beta(X)$. Le même événement joue le rôle de selle à l'ordre $n-1$, avec les bras $X\setminus\lbrace u\rbrace$ pour $u\in U$.

Dans ce seul cas terminal, le lien de rang $n$ vers $n-1$ doit donc exister même lorsque la naissance supérieure n'appartient à aucun groupe d'incidence source. Un bridge qui exige un groupe source non vide omet cette composante terminale et viole M.1. Il ne faut ni inventer une coface de rang $n+1$, ni lancer une descente supplémentaire à l'ordre $n$.

Lorsque $K_{\mathrm{eff}}<n$, la tour s'arrête à l'ordre demandé. Elle exige les liens de toutes les naissances de rang au plus $K_{\mathrm{eff}}$, mais n'exige pas l'événement de rang $n$ hors périmètre. Les événements de rang $K_{\mathrm{eff}}+1$ restent seulement des selles nécessaires à l'ordre $K_{\mathrm{eff}}$.

Pour $n=K_{\mathrm{eff}}=1$, l'unique naissance de $T_1$ est conservée et aucune flèche verticale n'existe. Il ne faut inventer ni ordre zéro, ni selle d'ordre zéro.

Dans `hgp_reduced`, ce carrier terminal isolé n'est pas un nœud public. Son ancre reste néanmoins une donnée interne nécessaire à `full_pi0`, à l'audit de totalité et à toute vue qui pourrait le rendre visible.

## 9. Certificat exécutable minimal

Un record `ExactCriticalEventRankTowerLink` doit être indexé par l'identité complète de l'événement, ou par son `source_event_projection_index` tant que le journal reste interne. Il contient au minimum :

- l'identité de l'événement;
- le birth record source d'ordre $r$;
- le saddle record cible d'ordre $r-1$;
- l'unique niveau carré exact commun;
- une tranche CSR de exactement $\lvert U\rvert$ bras;
- pour chaque bras, son point de shell retiré, sa naissance terminale de rang $r-1$, son niveau strictement inférieur et son `pre_batch_component_handle`;
- pour chaque naissance terminale, l'égalité exacte de la facette, du centre rationnel et du niveau avec l'événement inférieur joint;
- le groupe atomique inférieur contenant la selle;
- la racine cible post-lot de ce groupe;
- les identités ou stamps immuables des snapshots inférieur pré-lot et post-lot nécessaires au rejeu.

Le certificat complet exige :

1. exactement un lien pour chaque rôle de naissance de rang $2\leq r\leq K_{\mathrm{eff}}$, y compris $r=n$ lorsque $K_{\mathrm{eff}}=n$;
2. exactement un rôle de selle du même événement à l'ordre $r-1$ et au même niveau;
3. exactement un bras par point de $U$;
4. un terminal de rang exactement $r-1$ et de niveau strictement inférieur pour chaque bras;
5. un `pre_batch_component_handle` valide pour tous les terminaux, y compris un carrier latent singleton;
6. une unique racine post-lot contenant la selle entière, conditionnellement au certificat O4 du lot;
7. des identités pré-lot et post-lot qui interdisent de rejouer les terminaux contre une coupe différente;
8. aucun payload scientifique partiel en cas d'absence, d'ambiguïté, de budget insuffisant ou de dégénérescence non couverte.

`ExactMorseGammaPartitionSweep` est le falsificateur borné existant le plus proche : il joint déjà les bras aux naissances terminales, fige les racines, contracte les selles simultanées puis compare la partition mono-ordre à Gamma a posteriori. Il ne valide pas encore la jointure verticale entre le rôle de naissance supérieur et le rôle de selle inférieur du même événement. Le chemin produit doit reprendre sa sémantique locale sans embarquer Gamma exhaustif, puis ajouter et propager cette jointure adjacente.

## 10. Représentation GPU-friendly et rôle de Morton

En dimension trois, $2\leq\lvert U\rvert\leq4$. Le journal naturel est donc formé de tableaux plats :

- une structure de tableaux d'événements triés par `(order, exact_level, event_id)`;
- une arène CSR de deux à quatre bras par lien adjacent, et aucune tranche pour les naissances de rang un;
- une jointure triée ou hachée de chaque terminal vers un birth record inférieur;
- un union-find segmenté par lot de niveau exact;
- un tableau d'ancres adjacentes;
- un pointer-jumping sur les parents horizontaux et sur les compositions verticales.

Le stockage est proportionnel aux événements acceptés, aux bras et aux nœuds de descente effectivement découverts. Il ne contient aucune arène indexée a priori par toutes les paires, tous les triplets, tous les quadruplets, toutes les facettes de cardinal $k$, les cellules Gamma ou la mosaïque de Delaunay d'ordre supérieur. Cette représentation est output-sensitive, mais aucun théorème courant ne borne encore le nombre d'événements acceptés ou d'étapes de descente par une quantité sous-combinatoire; le pire cas reste combinatoire.

Morton/LBVH intervient en amont pour partager le parcours GPU, proposer les supports de tailles deux à quatre et exécuter les requêtes top-$k$ des descentes. Il n'est pas l'autorité mathématique de la verticale. Toute branche Morton prunée doit porter un certificat exact de non-contribution; une simple fenêtre locale ne prouve pas la complétude du catalogue.

Le Théorème 7 du manuscrit peut guider une génération inverse depuis des facettes inférieures, mais cette voie est distincte du théorème descendant : la descente est plusieurs-vers-un et ne peut pas être inversée depuis le seul catalogue des minima.

## 11. Statut des obligations

| énoncé | statut |
|---|---|
| dualité naissance de rang $k+1$ / selle d'ordre $k$ pour le même événement | `proved_under_local_morse_hypotheses` |
| $\beta(F_u)<a$, germe exact et descente stricte | `proved_under_strict_descent_hypotheses` |
| terminal de chaque bras = naissance de rang exactement $k$ | `proved_under_strict_descent_hypotheses` |
| cible verticale par facette commune dans Gamma complet | `proved_from_manuscript_theorem_2_and_elementary_adjacencies` |
| naturalité de la verticale Gamma complète | `proved_from_persistent_common_facet_witness` |
| jointure bornée du sweep 6.23 | `validated_reference_falsifier` |
| génération sparse exhaustive de tous les événements et de toutes les attaches | `proof_obligation` O1/O3 |
| adéquation de tout quotient événementiel simultané au changement topologique | `proof_obligation` O4 |
| propagation produit totale et rejeu de tous les carrés sans Gamma exhaustif | `proof_obligation` O7 |
| reconstruction M.1 complète | `proof_obligation` |

Ce découpage autorise l'implémentation et la falsification du bon lien événementiel sans déclarer prématurément la tour exacte.

## Références

- L. Hauseux, [*Manuscrit de thèse*](../references/MANUSCRIT_THESE_HAUSEUX.pdf), chapitres 6, 8 et 9, en particulier Théorèmes 2, 4, 6 et 7.
- Y. Reani et O. Bobrowski, [*Morse Theory for the k-NN Distance Function*](https://arxiv.org/abs/2403.12792), pour l'indice local et les rôles critiques de la distance K-NN.
- [Définition de la hiérarchie HGP en dimension trois](DEFINITION_HGP_3D.md).
- [Attaches par descente de miniball](ATTACHES_DESCENTE_MINIBALL.md).
- [Contrat candidat M.1](../contracts/M1_RECONSTRUCTION.md).
