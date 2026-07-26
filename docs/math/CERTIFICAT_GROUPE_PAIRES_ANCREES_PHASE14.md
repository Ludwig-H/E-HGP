# Certificat groupé exact discret, parcours préparé et curseur Morton — Phase 14Q P8g--P8k et P8o

## Statut et portée

Ce document fixe un lemme de prune exact pour un groupe borné d'ancres et un nœud certifié du LBVH, son parcours préparé parent--enfant, l'ordonnancement borné de tous les groupes, l'expansion orientée et reprenable de leurs seuls terminaux ouverts, puis la classification reprenable de chaque candidate. P8g fournit la première primitive `reference_cpu / hgp_reduced / grouped_exact_certificate`; P8h ajoute le mode `reference_cpu / hgp_reduced / prepared_grouped_exact_traversal`; P8i ajoute `reference_cpu / hgp_reduced / bounded_morton_group_schedule`; P8j ajoute `reference_cpu / hgp_reduced / bounded_morton_oriented_candidate_cursor`; P8k ajoute `reference_cpu / hgp_reduced / resumable_exact_anchored_pair_closed_ball_classifier`. P8o resserre exclusivement le certificat commun de P8g--P8h en remplaçant la boîte englobante des ancres par leur ensemble discret réel. Ces composants restent sous `deployment_status=architecture_only` et `public_status=not_claimed`. Ils ne réduisent aucune hiérarchie et ne qualifient ni `warm_e2e`, ni 50 k, ni 10 M+.

Pour un couple groupe--nœud, la primitive mutualise les évaluations de $G$ ancres contre $W$ témoins sans relâcher les ancres vers les coins hybrides de leur boîte. Un slot témoin inspecté coûte entre une et $G$ évaluations ancre--nœud : la première ancre non strictement négative rend ce slot inconclusif, tandis qu'un succès commun exige les $G$ évaluations. P8h prépare les bornes ponctuelles des ancres réelles et du pool une seule fois par contexte, puis évite de recalculer chez un enfant tout succès commun strict effectivement rencontré dans l'ordre canonique. P8i prépare un seul groupe Morton à la fois et ne conserve aucun catalogue global. Le pire cas conserve le produit groupes × nœuds × ancres × témoins. Le pool commun n'a aucune obligation de rappel : sa seule fonction est de présenter au prédicat exact des candidats susceptibles de fournir une preuve commune.

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

Le contexte garde un seul ordinal, deux plages Morton, 32 ancres, 64 témoins et un contexte P8h actif. Son état supplémentaire est donc $O(G+W+H)$, où $H$ est la profondeur de la pile P8h, indépendamment du nombre total de groupes. Les frontières `group_complete` avancent seulement après la complétion authentifiée du P8h actif. Un épuisement conserve ce contexte sans avancer l'ordinal; une autorité étrangère ou une source déplacée est rejetée avant mutation.

Les fixtures permanentes vérifient la partition et le halo indépendamment, recertifient chaque prune P8h par un P8g frais, conservent le flux et le travail exact entre un passage monolithique et des budgets `(1,1)`, puis comparent les événements et diagnostics P8c au chemin P8b--P8c exact. Un pool volontairement trop court rouvre les 276 paires d'un nuage de 24 points et conserve la même projection scientifique. Le CTest passe sous GCC Release en 0,04 seconde et Clang 18 Release en 0,05 seconde; l'installation/export et le consumer passent 1/1 en 0,01 seconde. Aucun benchmark, CUDA ou GCP n'est lancé pour P8i.

## Curseur de candidates orientées — P8j

P8j consomme directement les feuilles non résolues et les plages de fallback de P8i. Il conserve seulement la plage Morton terminale courante, son offset de feuille et l'offset dans les ancres du groupe actif. Pour une plage, l'ordre déterministe parcourt d'abord les feuilles, puis les ancres triées par `PointId`; chaque comparaison d'orientation est facturée et l'offset est avancé avant toute émission. Une étape `candidate_pair` ne paraît que si $p<q$ et contient cette unique paire. Une interruption après l'émission ne peut donc ni la répéter, ni en sauter une.

La preuve de P8i devient ainsi exécutable sans adaptateur de produit global. Le groupe contenant le petit `PointId` est l'unique propriétaire d'une paire non ordonnée. Si le terminal correspondant est ouvert, P8j l'émet exactement lors du test $p<q$; les autres groupes la rejettent. Si le terminal est absent, seul le certificat P8g imbriqué autorise son omission. Les événements `candidate_pair`, `budget_exhausted` et `group_complete` ne sont pas des décisions scientifiques; une candidate doit encore être classifiée exactement par P8c/P7b. Un prune transporte toujours l'étape P8i/P8h et l'autorité P8g rejouable.

Les quatre plafonds d'un appel portent séparément sur les avances P8i, les orientations, les visites de nœuds P8h et les prédicats exacts P8h. Les deux derniers sont agrégés sur toutes les avances P8i internes du même appel. Un terminal déjà ouvert se draine sans nouveau budget d'avance P8i; inversement, un budget d'orientation nul ne bloque ni prune, ni frontière tant qu'aucun terminal n'est ouvert. Tout arrêt est typé, conserve les offsets et ne copie aucun payload P8i. Le contexte est move-only; l'étape l'est aussi afin d'interdire une collection accidentelle des grosses autorités rares.

L'état vivant supplémentaire à P8i est scalaire : une plage, deux offsets et son audit. La borne reste donc $O(G+W+H)$ au-delà du nuage et du LBVH. Il n'existe ni `std::set` produit, ni vecteur de candidates, ni arène de sorties, ni tableau par paire; l'ensemble utilisé par le CTest est explicitement un oracle de doublons de 24 points, jamais une structure de production.

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

La prochaine porte est exactement une passe bornée `uniform_latin`, 50 000 points, $K=10$, sur P8o. Elle doit lire séparément les slots témoins, les réemplois hérités, les signes physiques, les prunes, les candidates et le travail P8k, puis faire corriger le nouvel axe dominant avant toute campagne de latence. Un sink externe et une stratégie non combinatoire scalable pour les supports trois--quatre restent obligatoires avant de lever la garde résidente pour 10 M+. L'ancien flux P7b reste un oracle borné; aucun nouveau gate GCP ne précède ce diagnostic CPU.
