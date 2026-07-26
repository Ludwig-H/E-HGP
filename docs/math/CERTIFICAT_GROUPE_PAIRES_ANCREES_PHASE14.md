# Certificat groupé exact discret, parcours préparé et curseur Morton — Phases 14Q--14R P8g--P8r

## Statut et portée

Ce document fixe un lemme de prune exact pour un groupe borné d'ancres et un nœud certifié du LBVH, son parcours préparé parent--enfant, l'ordonnancement borné de tous les groupes, l'expansion orientée et reprenable de leurs seuls terminaux ouverts, puis la classification reprenable de chaque candidate. P8g fournit la première primitive `reference_cpu / hgp_reduced / grouped_exact_certificate`; P8h ajoute le parcours préparé, P8i l'ordonnancement Morton, P8j le curseur orienté et P8k la classification fermée. P8o resserre le certificat commun sur les ancres réelles, P8p ajoute le fallback singleton et P8q les sous-groupes récursifs. P8r cherche en plus un cœur borné de sous-arbres témoins, mais rejoue toujours les points retenus par P8g. Ces composants restent sous `deployment_status=architecture_only` et `public_status=not_claimed`. Ils ne réduisent aucune hiérarchie et ne qualifient ni `warm_e2e`, ni 50 k, ni 10 M+.

Pour un couple groupe--nœud, la primitive mutualise les évaluations de $G$ ancres contre $W$ témoins sans relâcher les ancres vers les coins hybrides de leur boîte. Un slot témoin inspecté coûte entre une et $G$ évaluations ancre--nœud : la première ancre non strictement négative rend ce slot inconclusif, tandis qu'un succès commun exige les $G$ évaluations. P8h prépare les bornes ponctuelles des ancres réelles et du pool une seule fois par contexte, puis évite de recalculer chez un enfant tout succès commun strict effectivement rencontré dans l'ordre canonique. P8i prépare un seul groupe Morton à la fois et ne conserve aucun catalogue global. Le pire cas conserve le produit groupes × nœuds × ancres × témoins. Le pool commun n'a aucune obligation de rappel : sa seule fonction est de présenter au prédicat exact des candidats susceptibles de fournir une preuve commune.

## Diagnostic borné post-P8o

L'unique passe post-P8o `uniform_latin`, 50 000 points et $K=10$, est un diagnostic exact incomplet, jamais un benchmark de qualification. Elle observe $259{,}065\ \mathrm{ms}$ au total et $54{,}900\ \mathrm{ms}$ dans la voie paire, puis s'arrête au cap total de 20 000 signes exacts groupés. Le travail atteint 107 nœuds, 6 576 slots témoins, 756 réemplois hérités et 138 découvertes strictes. Les 38 prunes recertifiées couvrent 330 368 couples dirigés; les 352 candidates ouvertes sont toutes classées `above_rank` après 12 808 visites P8k.

L'identité de couverture atteinte dans le premier groupe de 32 ancres est seulement :

$$330\,368+352=32\times10\,335<32\times50\,000.$$

Ainsi, seuls 10 335 équivalents-feuilles sur 50 000 sont fermés pour ce premier groupe lorsque le cap de signes est atteint. La session ne termine ni ce groupe ni la partition dirigée, ne peut pas être scellée et ne publie aucune réponse scientifique. Ce diagnostic localise le plafond après P8o : les prunes communes existent, mais le pool commun ne suffit pas à achever le reste du produit sous le cap courant.

## Identité diamétrale

Pour une ancre $p$, un second support $q$ et un témoin $x$, on pose :

$$\Phi_x(p,q)=(x-p)\cdot(x-q).$$

Le point $x$ est strictement intérieur à la boule fermée de diamètre $[p,q]$ si et seulement si $\Phi_x(p,q)<0$. Son lien exact avec la marge ancrée $H_x$ de P8e/P8f est :

$$\Phi_x(p,q)=\left\Vert x-p\right\Vert^2-(x-p)\cdot(q-p)=-H_x(p,q).$$

## Théorème de groupe

Soit $P$ un ensemble fini non vide d'ancres réelles, $Q$ la boîte exacte d'un nœud LBVH et $W$ un pool borné de `PointId` distincts, disjoint de $P$. Pour chaque $x\in W$, on définit le maximum discret commun :

$$M_x(P,Q)=\max_{p\in P,\;q\in Q}(x-p)\cdot(x-q).$$

Soit $s_{\max}\in\lbrace 2,\ldots,11\rbrace$ et $m=s_{\max}-1$. S'il existe $m$ témoins distincts $x_1,\ldots,x_m\in W$ tels que $M_{x_j}(P,Q)<0$ pour tout $j$, alors, pour toute ancre réelle $p\in P$ et tout point réel $y$ couvert par le nœud $Q$, les $m$ témoins sont strictement intérieurs à la boule de diamètre $[p,y]$.

En effet, $p\in P$ et $y\in Q$, donc, pour chaque témoin certifié :

$$\Phi_{x_j}(p,y)\leq M_{x_j}(P,Q)<0.$$

La stricte négativité exclut automatiquement $x_j=p$, $x_j=y$ et $p=y$. Avec les deux supports, la boule fermée contient donc au moins :

$$m+2=(s_{\max}-1)+2=s_{\max}+1$$

`PointId` distincts. La seule cardinalité strictement intérieure vaut au moins $m=s_{\max}-1$; c'est son union avec les deux supports, de cardinalité au moins $s_{\max}+1$, qui dépasse la fenêtre fermée. La conclusion utilisée par le produit est ainsi plus forte qu'un simple rang observé élevé et exclut aussi le cas diagnostique extra-shell. Le prune ne doit pas être justifié en aval par le seul libellé « rang fermé supérieur ».

## Calcul exact de la borne

Le témoin $x$ et chaque ancre $p$ sont ponctuels. Pour une ancre fixée, la dépendance en $q$ est affine sur chaque axe de $Q$; le maximum est donc atteint à une extrémité. En dimension trois :

$$M_x(\lbrace p\rbrace,Q)=\sum_{i=1}^{3}\max_{q_i\in[Q_i^-,Q_i^+]}(x_i-p_i)(x_i-q_i).$$

Le maximum commun est ensuite le maximum fini sur les seules ancres réelles :

$$M_x(P,Q)=\max_{p\in P}M_x(\lbrace p\rbrace,Q).$$

L'implémentation peut donc réutiliser `exact_diametral_phi_aabb_maximum_sign` avec la boîte ponctuelle de chaque ancre et la boîte $Q$. Le chemin dyadique borné décide la plage binary64 ordinaire; si cette enveloppe est insuffisante, le fallback `BigInt` exact intervient avant toute décision. Le slot est un succès commun si et seulement si les $G$ signes sont strictement négatifs. Une valeur positive ou nulle permet d'arrêter immédiatement ce slot comme inconclusif, sans inspecter les ancres suivantes. La primitive n'alloue aucune arène persistante ou proportionnelle à $n$, mais elle ne revendique pas une absence littérale d'allocation transitoire.

## Le pool est propositionnel

Un témoin n'a pas besoin d'appartenir à la banque de chaque ancre. Dès que $M_x(P,Q)<0$, la quantification universelle sur $P\times Q$ fournit toute l'autorité nécessaire. Le pool peut donc provenir d'une banque graine, d'un voisin représentatif ou d'un producteur device borné. Une omission ne crée qu'un résultat inconclusif et ne peut supprimer une paire pertinente.

Le contrat courant impose seulement un pool strictement croissant, sans doublon, de taille au plus 64 et disjoint du groupe d'au plus 32 ancres. Aucun tableau $64n$ n'est construit.

## Élimination des coins hybrides de la boîte d'ancres

Le certificat v1 remplaçait $P$ par sa boîte englobante $A$. Celle-ci contient des coins hybrides qui ne sont pas nécessairement des ancres réelles et pouvait donc perdre un prune pourtant commun. Considérons :

$$P=\lbrace (1,-2,0),(-2,1,0)\rbrace,\qquad q=(1,1,0),\qquad x=(0,0,0).$$

Pour chacune des deux ancres réelles, $\Phi_x(p,q)=-1$, donc le nouveau maximum discret vaut :

$$M_x(P,\lbrace q\rbrace)=-1<0.$$

Pourtant, la boîte $A=[-2,1]\times[-2,1]\times\lbrace 0\rbrace$ contient le coin hybride $a=(1,1,0)$ et le certificat v1 observait :

$$\Phi_x(a,q)=2.$$

Le maximum discret élimine exactement cette contre-relaxation : le point $a$ n'appartient pas à $P$ et ne participe plus au maximum. Il reste une condition suffisante, jamais une condition nécessaire : le pool peut omettre les témoins utiles et la boîte $Q$ peut encore contenir des points qui ne sont pas des feuilles réelles. Un échec reste donc inconclusif et retombe sur la descente ou le parcours individuel; il ne peut jamais être interprété comme une absence de témoin.

## Provenance et atomicité

Un reçu positif est lié simultanément à l'identité process-local du nuage et du LBVH, à l'indice du nœud, à sa plage de feuilles, à sa boîte exacte, au rang fermé maximal demandé et à la liste canonique des ancres. Tout ce payload scientifique est privé après construction. Son rejeu doit appeler `certifies(...)` avec le nœud, le rang et les ancres attendus; un simple test de `certified()` ne suffit pas à l'appliquer à un autre contexte.

Les caps d'ancres, de témoins et de prédicats exacts sont vérifiés avant l'opération correspondante. Pour un slot actif, le contexte conserve l'offset de la prochaine ancre réelle à évaluer. Une interruption après l'ancre d'offset $a$ reprend à l'offset $a+1$, sans répéter les signes déjà strictement négatifs et sans avancer au témoin suivant. Si un cap manque après quelques succès stricts, aucun succès commun ni témoin partiel n'est publié. Un résultat inconclusif ou épuisé n'a aucune autorité de prune.

## Bornes et structures évitées

Pour $G\leq32$ ancres et $W\leq64$ témoins, la préparation des bornes ponctuelles coûte $O(G+W)$ et un nœud coûte au plus $O(GW)$ prédicats exacts, avec arrêt dès $m\leq10$ succès communs. Chaque slot effectivement ouvert coûte entre une et $G$ évaluations; un slot hérité d'un parent coûte zéro nouvelle évaluation. Le contexte ajoute seulement l'offset scalaire de l'ancre active : il ne conserve ni matrice ni masque par ancre de taille $G\times W$. Le payload fixe d'identifiants occupe au plus $8(32+64+10)=848$ octets, hors métadonnées, offset et jetons process-local.

Cette primitive ne construit ni corde par nœud, ni allocator global de records, ni tableau par ancre du nuage complet, ni catalogue de paires, facettes, cofaces ou incidences, ni cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur. Le LBVH certifié existant reste la seule structure globale géométrique.

## Parcours préparé et monotonie parent--enfant — P8h

Soit $Q'$ la boîte certifiée d'un enfant d'un nœud de boîte $Q$. Puisque $Q'\subseteq Q$, la monotonie du maximum sur son domaine donne, pour tout témoin $x$ :

$$M_x(P,Q')\leq M_x(P,Q).$$

Ainsi, tout succès strict du parent reste strict chez l'enfant :

$$M_x(P,Q)<0\quad\Longrightarrow\quad M_x(P,Q')<0.$$

P8h encapsule cette implication dans un contexte non agrégat, non constructible par défaut, non copiable et déplaçable avec révocation de la source. P8o y prépare les bornes ponctuelles des ancres réelles et des au plus 64 témoins une seule fois, conserve une pile DFS de capacité fixe, au plus un nœud actif reprenable, le slot témoin actif et son offset d'ancre, puis émet au plus un événement par appel. Les plages de feuilles et les succès communs hérités restent privés; l'appelant ne peut ni fournir un masque brut, ni extraire l'état intermédiaire d'un nœud inconclusif. Il n'existe aucun masque par ancre : un bit témoin n'est héritable qu'après négativité stricte sur les $G$ ancres. Un prune positif transporte encore le certificat commun et doit être rejoué par `certifies(...)` avec le nuage, le LBVH, le nœud, le rang et les ancres attendus.

L'héritage ne change jamais l'ordre canonique du pool. À chaque nœud, le curseur parcourt les slots dans l'ordre croissant; lorsqu'il atteint un succès commun hérité, il évite les $G$ évaluations ancre--nœud et incrémente alors seulement le compteur de réutilisation. Pour un slot neuf, il parcourt les ancres dans l'ordre canonique et facture chaque évaluation avant d'avancer l'offset. Si les slots antérieurs ferment déjà le rang, un succès hérité plus tardif n'est ni parcouru, ni compté, ni publié dans le certificat terminal. La reprise au milieu d'un slot conserve exactement le même ordre, les mêmes décisions et le même travail physique qu'un passage monolithique, à l'exception du nombre d'appels et d'épuisements.

Un épuisement du budget de visites ou de prédicats conserve le nœud actif privé sans publier de certificat partiel. Une feuille inconclusive émet un seul `PointId` à classifier ultérieurement, jamais $G$ paires acceptées. Un pool trop petit émet un fallback de sous-arbre et termine le contexte. Une autorité étrangère est rejetée avant toute mutation d'audit; déplacer le contexte révoque sa source.

La validation courte GCC Release passe les deux CTests P8g/P8h en 0,20 seconde. L'installation et l'export, puis le consumer externe qui lie la cible installée et appelle réellement `start_at_root` et `advance`, passent 1/1 en 0,01 seconde. Aucun benchmark, sanitizer, kernel, test massif ou GCP n'est lancé pour P8h.

## Ordonnancement Morton borné — P8i

P8i partitionne l'ordre certifié des feuilles LBVH en intervalles contigus $[jG,\min((j+1)G,n))$, avec $1\leq G\leq32$. Il copie seulement les identifiants du groupe actif, les trie strictement par `PointId`, puis propose au plus $W\leq64$ témoins hors groupe. Pour chaque distance de rang Morton, le halo prend d'abord la feuille disponible à gauche, puis celle de droite, jusqu'au cap, avant de trier le pool par `PointId`. Ce choix améliore seulement la cohérence spatiale : il n'affirme ni voisinage exact, ni rappel, ni borne sur le nombre de feuilles non résolues.

Les intervalles forment une partition disjointe et exhaustive de $[0,n)$, donc chaque `PointId` devient ancre exactement une fois. Pour une paire non ordonnée de petite extrémité $p$ et grande extrémité $q$, le groupe de $p$ est son unique propriétaire. Si le terminal contenant $q$ reste ouvert, son expansion exhaustive et le filtre $p<q$ émettent cette paire une fois; le groupe de $q$ la rejette par orientation. Si le terminal est omis, le certificat P8g commun prouve que la paire dépasse la fenêtre de rang. Cette implication donne la complétude générale; le différentiel P8b--P8c ne compare que sa projection scientifique sur une fixture, car ses listes candidates et son pool top-$L$ peuvent légitimement différer du halo groupé.

Le contexte P8i initial garde un seul ordinal, deux plages Morton, 32 ancres, 64 témoins et un contexte P8h actif. Son état supplémentaire est donc $O(G+W+H)$, où $H$ est la profondeur de la pile P8h, indépendamment du nombre total de groupes. Les frontières `group_complete` avancent seulement après la complétion authentifiée du P8h actif. Un épuisement conserve ce contexte sans avancer l'ordinal; une autorité étrangère ou une source déplacée est rejetée avant mutation. P8p puis P8q étendent cet état de manière fixe, comme détaillé ci-dessous, sans changer la partition Morton initiale des ancres.

Les fixtures permanentes vérifient la partition et le halo indépendamment, recertifient chaque prune P8h par un P8g frais, conservent le flux et le travail exact entre un passage monolithique et des budgets `(1,1)`, puis comparent les événements et diagnostics P8c au chemin P8b--P8c exact. Un pool volontairement trop court rouvre les 276 paires d'un nuage de 24 points et conserve la même projection scientifique. Le CTest passe sous GCC Release en 0,04 seconde et Clang 18 Release en 0,05 seconde; l'installation/export et le consumer passent 1/1 en 0,01 seconde. Aucun benchmark, CUDA ou GCP n'est lancé pour P8i.

## Partition `common-first` puis singletons — P8p

Soit $P$ le groupe actif, $R$ la racine de requête et $T_R$ son sous-arbre LBVH. Le parcours commun P8o/P8h traite d'abord $T_R$. Un nœud dont le certificat commun réussit ferme le bloc entier $P\times L(C)$. Un nœud hors diagonale dont le certificat commun est inconclusif n'est plus redescendu avec le même groupe et le même pool : il devient une frontière authentifiée $Q$, est retiré de la pile commune et délègue son bloc. Pour chaque $p\in P$, dans l'ordre canonique, l'ordonnanceur lance alors un P8h singleton complet à la racine $Q$ avec un halo propre $W_p$, attend sa terminaison, puis passe à l'ancre suivante. Le parcours commun suspendu ne reprend qu'après la fermeture des $G$ singletons. Si le pool commun contient moins que les $s_{\max}-1$ témoins structurellement nécessaires, le fallback fail-open historique du groupe entier reste terminal direct et P8j l'ouvre exhaustivement; aucune fausse frontière singleton n'est fabriquée.

Le halo $W_p$ contient au plus $W\leq64$ feuilles distinctes de $p$, proposées alternativement à gauche puis à droite de sa feuille dans l'ordre Morton global, puis triées par `PointId`. Il n'a aucune obligation de rappel. Une insuffisance du halo ouvre une feuille ou un sous-arbre terminal pour cette seule ancre; elle n'autorise jamais une omission. Inversement, chaque prune singleton est recertifié pour exactement la liste d'ancres $\lbrace p\rbrace$ et exactement le nœud délégué ou l'un de ses descendants.

La diagonale reste strictement fail-open. Si la boîte exacte d'un nœud contient la coordonnée d'au moins une ancre réelle du groupe, le parcours commun ne dépense aucun signe témoin sur ce nœud et descend ses enfants. À une feuille, il émet une frontière inconclusive, y compris pour l'auto-couple. Des boîtes d'enfants qui se recouvrent peuvent rendre ce test conservateur et provoquer une descente supplémentaire; elles ne peuvent ni produire un faux prune, ni perdre une paire. Les auto-couples et orientations inverses sont ensuite comptés explicitement par P8j au lieu d'être cachés dans une autorité commune impossible.

### Théorème de partition

Les nœuds certifiés communs $\mathcal{C}$, les terminaux directs fail-open $\mathcal{U}$ et les frontières $\mathcal{F}$ émis par la DFS forment une coupe disjointe des feuilles de $R$. Par conséquent :

$$P\times L(R)=\bigsqcup_{C\in\mathcal{C}}P\times L(C)\;\bigsqcup\;\bigsqcup_{U\in\mathcal{U}}P\times L(U)\;\bigsqcup\;\bigsqcup_{Q\in\mathcal{F}}\bigsqcup_{p\in P}\lbrace p\rbrace\times L(Q).$$

Chaque bloc direct $P\times L(U)$ est intégralement ouvert par P8j. Pour chaque bloc délégué $\lbrace p\rbrace\times L(Q)$, le P8h singleton est lui-même une DFS exacte : ses prunes certifiés et ses plages terminales forment une partition disjointe de $L(Q)$. Comme le nœud $Q$ a déjà été retiré de la pile commune et qu'un seul parcours singleton de $Q$ est exécuté pour chaque $p$, aucun bloc ne peut être omis ou visité par deux propriétaires. Le filtre $p<q$ projette ensuite cette partition dirigée sur chaque paire non ordonnée exactement une fois; P8l conserve l'identité entre masse prunée et orientations effectivement examinées.

La provenance reflète cette réduction. Une étape de frontière porte encore le groupe complet et la plage exacte de $Q$, sans témoin ni autorité de prune. Pendant un singleton, chaque étape terminale ou positive porte une plage d'ancre $[\ell_p,\ell_p+1)$ et le seul `PointId` $p$. P8j copie ce snapshot borné à l'ouverture d'une plage avant de la drainer; il ne relit donc pas comme ancres terminales le groupe complet que P8i conserve pour reprendre ensuite le parcours commun.

À tout instant, P8p conserve le contexte P8h commun suspendu et au plus un contexte P8h singleton actif : deux contextes, deux piles fixes, au plus un nœud actif par contexte, le halo commun et un halo singleton supplémentaire. Il ne conserve ni $G$ contextes singleton, ni $G$ halos, ni matrice ou masque $G\times W$, ni état nouveau proportionnel à $n$. Avec $T_Q$ le sous-arbre d'une frontière, les $G$ parcours singleton coûtent honnêtement $O(GW\lvert T_Q\rvert)$ au pire pour cette frontière. En sommant sur tous les groupes et toutes les frontières, P8p conserve une borne quadratique $O(n^2W)$ dans le pire cas; il ne revendique aucune amélioration sous-quadratique.

Les fixtures permanentes exercent la descente diagonale sans signe, l'émission stable des frontières, la délégation de chaque frontière à toutes les ancres singleton, les halos propres, la recertification des prunes, le snapshot singleton $[\ell_p,\ell_p+1)$, l'identité des candidates et la stabilité entre budgets amples et unitaires. La recertification finale des quatre CTests ciblés passe 4/4 sous GCC Release en 0,54 seconde et 4/4 sous Clang 18 Release en 0,49 seconde. Il s'agit d'une validation logicielle courte, sans benchmark massif, CUDA ou GCP.

L'unique diagnostic post-P8p autorisé sur `uniform_latin`, $n=50\,000$, $K=10$, s'arrête en 174,871 ms à `total_grouped_traversal_exact_predicate_capacity`. Les 360 visites dépensent 20 000 signes exacts et un seul fallback singleton est préparé avant l'arrêt. Ce run incomplet établit seulement que le premier rejeu singleton domine le budget observé; il n'autorise ni extrapolation de durée, ni qualification 50 k.

## Partition récursive des sous-intervalles d'ancres — P8q

Soit une frontière P8p authentifiée $A\times L(Q)$, où $A=[a_0,a_G)$ est l'intervalle Morton contigu du groupe. P8q remplace la liste immédiate des $G$ singletons par une pile DFS fixe de sous-intervalles contigus. Pour tout $B=[b_0,b_1)$ de cardinal au moins deux, il prépare un P8o limité au seul nœud $Q$. Si le certificat commun réussit, le reçu P8g ferme exactement $B\times L(Q)$. Si le résultat est inconclusif, l'événement interne `anchor_subgroup_split` retire $B$ et pousse $[b_0,m)$ puis $[m,b_1)$, avec $m=b_0+\lfloor(b_1-b_0)/2\rfloor$. Si $b_1-b_0=1$, P8q lance le P8h singleton complet de P8p sur $Q$.

### Théorème de partition récursive

La racine $A$ et chaque sous-intervalle partagé satisfont $B=B_0\sqcup B_1$. Par induction sur l'arbre fini, ses feuilles terminales et ses nœuds certifiés forment une partition de $A$. Pour un nœud certifié, P8g ferme son produit avec $L(Q)$; pour une feuille singleton, P8h partitionne $L(Q)$ entre prunes recertifiés et terminaux fail-open. Le produit distribue sur l'union disjointe, donc l'ensemble des blocs fermés et ouverts est exactement $A\times L(Q)$, sans omission ni duplication. Une sonde inconclusive ne transporte aucun masque positif, et un événement de partage n'est ni une candidate ni une décision scientifique.

Le pool $W_B$ est reconstruit juste avant la sonde ou le parcours singleton et contient au plus $W\leq64$ feuilles extérieures à $B$. Si $Q$ est strictement situé d'un côté de $B$ dans l'ordre Morton, P8q propose d'abord jusqu'à $\lceil3W/4\rceil$ feuilles de ce côté, complète depuis l'autre côté puis revient au côté préféré. Si les intervalles se chevauchent, il reprend l'alternance symétrique gauche--droite de P8p. Le tri final par `PointId` fixe l'identité canonique du pool. Cette orientation est seulement une heuristique de proposition : elle n'a aucune obligation de rappel et chaque succès est recalculé par le prédicat dyadique exact.

L'audit rend la réduction vérifiable. Les visites et signes sont séparés en lanes commune, sous-groupe et singleton, dont la somme égale le total physique. Chaque sonde préparée se termine en partage ou en prune de sous-groupe; chaque singleton préparé se termine; la masse des ancres déléguées égale la masse certifiée par sous-groupes plus le nombre de singletons préparés. Les compteurs de pools, d'entrées orientées et le maximum de sous-intervalles en attente restent bornés respectivement par les nombres de préparations, $W$ et $G$.

À tout instant, le contexte commun est suspendu avec au plus un contexte actif de sonde ou singleton, un seul pool supplémentaire et une pile d'au plus $G$ sous-intervalles. La borne d'état reste $O(G+2W+2H)$ avant les $R$ records P8l; aucune banque de pools, matrice $G\times W$, arène de paires, arbre dual global, facette, coface, incidence, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur n'est construite. À une profondeur de partition donnée, la somme des cardinalités sondées vaut au plus $G$; une partition équilibrée complètement inconclusive ajoute donc au plus $O(GW\lceil\log_2 G\rceil)$ signes avant les $G$ parcours singleton de pire cas $O(GW\lvert T_Q\rvert)$. P8q ne revendique ainsi aucune nouvelle borne globale sous-quadratique.

Les quatre validations courtes ciblées recertifient l'arbre de sous-intervalles, chaque prune, les identités d'audit et l'égalité roomy--segmentée. Elles passent 4/4 sous GCC Release en 0,33 seconde et 4/4 sous Clang Release en 0,29 seconde. L'exclusion explicite des témoins qui appartiennent à $Q$, l'héritage de relations strictes d'une sonde parente et le choix d'un témoin depuis une cible continue de milieu ou projection restent des dettes non bloquantes. Dès qu'une exécution complète à chaud sur 50 000 points et $K=10$ est strictement sous 0,5 seconde, ces dettes ne retardent pas le passage au sink externe et au chemin 10 M+.

## Cœur dynamique borné de sous-arbres témoins — P8r

Pour chaque nœud requête $Q$ du fallback P8q, P8r exécute un préflight exact puis parcourt au plus 64 nœuds témoins authentifiés. Un nœud témoin qui recouvre $Q$ ou contient une ancre est développé. Sinon, sa boîte n'engendre un reçu que si le maximum dyadique exact de la forme diamétrale est strictement négatif pour chaque ancre réelle. Les plages de feuilles des reçus sont disjointes; elles fournissent donc des points distincts, dont au plus dix sont sélectionnés puis rejoués par P8g. Le reçu de sous-arbre guide la sélection mais ne devient jamais l'autorité publiée : seul le certificat P8g frais autorise le prune.

L'ordre par milieu des intervalles les plus proches est `heuristic`. Le préflight non favorable, la limite de 64, un nombre insuffisant de témoins ou le passage à un frère efface l'état propositionnel, restaure le halo immuable et continue fail-open. L'état ajouté est une DFS bornée, au plus dix reçus et dix `PointId`; il ne matérialise ni arbre dual global, ni banque de témoins, ni arène de paires, cellule, coface, incidence, Gamma ou mosaïque de Delaunay d'ordre supérieur.

Les cinq CTests GCC Release ciblés passent 5/5 en 0,43 seconde. L'unique gate final 50 k/$K=10$ s'arrête incomplet avec le code 2 après 177,470 ms; ses 120 prunes ferment 1 509 560 paires dirigées sans candidate, mais aucune autorité terminale n'est scellée. Le gate inférieur à 0,5 seconde reste donc non satisfait. Le smoke local à 257 points ne conserve qu'un préfixe P8l non scellé et tous ses champs de capacité massive ou publique restent faux.

## Curseur de candidates orientées — P8j

P8j consomme directement les feuilles non résolues et les plages de fallback de P8i/P8q; il absorbe les événements de frontière et de partage sans les convertir en candidates. À l'ouverture d'une plage, il copie la liste bornée des ancres portée par l'étape terminale : le groupe complet pour un terminal commun ou le seul `PointId` pour un terminal singleton. Il conserve ce snapshot, la provenance du groupe et de la plage d'ancres, la plage Morton terminale courante, son offset de feuille et l'offset dans les ancres copiées. Pour une plage, l'ordre déterministe parcourt d'abord les feuilles, puis les ancres triées par `PointId`; chaque comparaison d'orientation est facturée et l'offset est avancé avant toute émission. Une étape `candidate_pair` ne paraît que si $p<q$ et contient cette unique paire. Une interruption après l'émission ne peut donc ni la répéter, ni en sauter une.

La preuve de P8i/P8q devient ainsi exécutable sans adaptateur de produit global. Le groupe contenant le petit `PointId` est l'unique propriétaire d'une paire non ordonnée. Si le terminal correspondant est ouvert, P8j l'émet exactement lors du test $p<q$; les autres groupes la rejettent. Si le terminal est absent, un certificat P8g imbriqué, commun au groupe, à un sous-groupe ou propre au singleton, autorise exactement son omission. Les événements `candidate_pair`, `budget_exhausted` et `group_complete` ne sont pas des décisions scientifiques; une candidate doit encore être classifiée exactement par P8c/P7b. Un prune transporte toujours l'étape P8i/P8h et l'autorité P8g rejouable.

Les quatre plafonds d'un appel portent séparément sur les avances P8i, les orientations, les visites de nœuds P8h et les prédicats exacts P8h. Les deux derniers sont agrégés sur toutes les avances P8i internes du même appel. Un terminal déjà ouvert se draine sans nouveau budget d'avance P8i; inversement, un budget d'orientation nul ne bloque ni prune, ni frontière tant qu'aucun terminal n'est ouvert. Tout arrêt est typé, conserve les offsets et ne copie que le snapshot borné nécessaire au terminal actif. Le contexte est move-only; l'étape l'est aussi afin d'interdire une collection accidentelle des grosses autorités rares.

L'état vivant supplémentaire à P8i est une plage, deux offsets, son audit et un snapshot fixe d'au plus $G$ ancres. Avec les deux contextes P8h et leurs tableaux de capacité fixe, la borne intégrée reste $O(G+W+H)$ au-delà du nuage et du LBVH, avec un facteur constant correspondant notamment aux deux piles. Il n'existe ni `std::set` produit, ni vecteur de candidates, ni arène de sorties, ni tableau par paire; l'ensemble utilisé par le CTest est explicitement un oracle de doublons de 24 points, jamais une structure de production.

Le différentiel permanent compare un budget ample à `(1,1,1,1)`. Il exige le même ordre de candidates, les mêmes prunes et frontières, le même travail exact P8h et, après consommation immédiate par P8c, les mêmes événements et diagnostics que P8b--P8c. Le fallback complet sur 24 points facture exactement 576 orientations, soit 276 candidates canoniques et 300 orientations inverses ou auto-couples, puis conserve la même projection scientifique. Les budgets nuls, la reprise d'une plage ouverte sans avance P8i, la révocation par déplacement et l'autorité étrangère sont également permanents. Le CTest ciblé passe sous GCC Release en 0,18 seconde et Clang 18 Release en 0,08 seconde; l'installation/export et le consumer externe passent 1/1 en 0,01 seconde avec une émission réelle $p<q$. Aucun benchmark, sanitizer, CUDA, test massif ou GCP n'est lancé pour P8j.

## Classifieur exact reprenable de boule fermée — P8k

P8k réécrit le classifieur P8c sous forme d'un contexte move-only lié aux identités process-local du nuage et du LBVH. Il ne mutualise pas le code du flux durable P7b : ce dernier reste inchangé et sert d'oracle indépendant. Une candidate P8j ouvre exactement un contexte actif. Celui-ci conserve les deux supports, le rang fermé maximal, les bornes exactes des supports, l'ancre d'intervalle centrée, une frontière DFS de capacité compile-time, au plus neuf identifiants strictement intérieurs pour $K\leq10$, la cardinalité de coque, son plus petit témoin supplémentaire, la cardinalité extérieure et les compteurs de partition.

Chaque visite physique d'un nœud est facturée avant le retrait de la frontière et avant toute mutation scientifique. Un budget nul ne visite rien; un budget épuisé conserve donc exactement le prochain nœud. La capacité de frontière est bornée par la profondeur Morton certifiée plus la profondeur possible des partages de codes égaux et ne dépend ni du nombre de candidates, ni du nombre de points. Un contexte ne contient aucun vecteur de taille $n$ et aucune arène de sorties.

Le filtre d'intervalles binary64 centré ne décide que lorsque son enveloppe outward exclut strictement zéro. Toute incertitude, y compris l'égalité, retombe sur les bornes AABB exactes; la partition des visites entre intérieur intervalle, extérieur intervalle et fallback exact est vérifiée à chaque arrêt. Le mode du filtre est figé au démarrage. Une modification incompatible de l'environnement flottant pendant un parcours actif échoue avant l'audit; après certification de `record_ready`, `above_rank` ou `complete`, la remise à disposition du terminal n'exécute plus de filtre et reste indépendante du thread ou du mode d'arrondi ultérieur.

L'automate public distingue `budget_exhausted`, `record_ready`, `above_rank` et `complete`. Une paire est fermée dès que plus de $s_{\max}-2$ points strictement intérieurs sont certifiés; aucun centre n'est alors construit. Pour une paire pertinente, le centre, le niveau et le vecteur borné d'intérieurs ne sont matérialisés que par `take_result`, après que P8l a préflighté sa capacité de sortie. Le record utilise exactement les types historiques `ExactPairSupportEvent` ou `ExactPairSupportExtraShellDiagnostic` et ne peut être consommé qu'une fois. Le contexte constitue l'autorité de progression; l'agrégat de résultat historique demeure forgeable et ne devient jamais, seul, une autorité terminale de session.

Les fixtures permanentes comparent une avance ample à une segmentation d'une visite sur toutes les paires d'un nuage tridimensionnel et pour chaque rang fermé de 2 à 6. Elles exigent les mêmes décisions, événements, diagnostics, compteurs de visites, décisions d'intervalles, fallbacks exacts et agrégations de sous-arbres; seuls les nombres d'appels et d'épuisements peuvent différer. Pour chacun de ces cinq rangs, les records sont ensuite identiques au flux P7b exhaustif et complet. Les frontières $K=10$ conservent exactement neuf intérieurs et rejettent dès le dixième; la cosphère conserve le témoin supplémentaire canonique. Les changements d'autorité, déplacements, doubles consommations, budgets nuls et changements d'environnement flottant avant et après terminal sont également fermés.

Le test d'intégration P8j--P8k impose déjà, dans son adaptateur borné, qu'aucune candidate suivante ne soit demandée tant que le classifieur actif n'est pas `above_rank` ou consommé. Il conserve l'identité du fallback $576=276+300$ et les mêmes sorties scientifiques. Il ne constitue toutefois ni une session de production, ni une autorité terminale sparse. Les deux CTests ciblés passent sous GCC Release en 0,15 seconde au total et sous Clang 18 Release en 0,10 seconde; installation/export et consumer externe passent 1/1 en 0,01 seconde avec ouverture, reprise et consommation réelles d'un contexte P8k. Aucun benchmark, sanitizer, CUDA, test massif ou GCP n'est lancé pour P8k.

## Limites restantes

Tous les curseurs P8i--P8l sont reprenables seulement entre appels du même processus. Ils ne possèdent ni codec, ni digest, ni epoch persistante et ne constituent pas un checkpoint durable ou une reprise après crash. P7b conserve volontairement son ancien format exact-only et sa seconde implémentation; l'identité de records ne signifie pas l'identité de travail, de digest ou de checkpoint.

P8l donne la priorité au P8k actif, préflight la sortie, impose des caps totaux typés et scelle une autorité terminale process-local après vérification des masses prunées et des orientations. P8m consomme cette autorité dans la façade et P8n installe exclusivement cette voie paire dans le runner. Ces garde-fous ferment honnêtement une session bornée, mais ne réduisent pas le pire cas : une classification P8k coûte jusqu'à $O(n)$ visites et, combinée au fallback P8j $\Theta(n^2)$, conserve un pire cas $O(n^3)$. Le runner reste résident et refuse plus de 50 k points; les sorties restent résidentes et les supports d'arité trois et quatre restent des univers implicites séparés. Le chemin 10 M+ n'est donc ni ouvert ni revendiqué.

La prochaine porte est un parcours exact borné bloc LBVH contre bloc LBVH : les blocs positifs réutilisent P8r et les blocs inconclusifs se partagent sans arbre dual global ni arène de paires. Une seule exécution complète à chaud `uniform_latin`, 50 000 points et $K=10$ doit ensuite sceller l'autorité terminale. Le sink et le checkpoint de Phase 15 puis le run GCP gardé à 10 000 001 points restent nécessaires; l'ancien flux P7b demeure un oracle borné et aucun benchmark ne change le statut public.
