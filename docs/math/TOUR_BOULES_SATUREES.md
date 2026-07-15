# Audit de la tour globale de boules saturées

> **Verdict audité.** La saturation par miniball, le recouvrement simplicial de Čech, le graphe d'intersection des générateurs et son seuillage par une forêt couvrante de poids maximum donnent une représentation exacte de toutes les composantes de Gamma. Ce résultat combinatoire est indépendant de la position générale. En revanche, la suite des forêts de générateurs n'est pas encore le `MergeForest` contractuel, la lecture de toute activation descendante comme événement de Morse est fausse en général, et l'énumération brute des supports n'est pas scalable au régime $n=50\,000$, $K_{\max}=10$.

Ce document audite la proposition `SaturatedBallTower`. Il en sépare les théorèmes prouvés, les obligations logicielles et les hypothèses de performance. Il ne modifie ni la base de preuve publique v2, ni le statut du backend courant.

## 1. Cadre et vocabulaire

Soit $X\subset\mathbb{R}^{3}$ un ensemble fini de points distincts. Pour tout $Q\subseteq X$ non vide, on note

$$\beta(Q)=\min_{y\in\mathbb{R}^{3}}\max_{q\in Q}\left\Vert y-q\right\Vert^{2}$$

et $B_Q$ l'unique miniball **fermée** de $Q$. Sa saturation dans le nuage est

$$\mathrm{Sat}(Q)=X\cap B_Q.$$

Un générateur saturé est l'ensemble $S=\mathrm{Sat}(Q)$ muni du niveau $t(S)=\beta(S)$, de sa boule exacte et d'au moins un support témoin. La famille $\Sigma_X$ contient un seul générateur par saturé distinct. Les supports multiples d'une même boule sont de la provenance, pas des sommets distincts du graphe de générateurs.

La saturation est idempotente, mais elle n'est pas monotone en général : $Q\subseteq R$ n'implique pas $\mathrm{Sat}(Q)\subseteq\mathrm{Sat}(R)$. Par exemple, dans le plan $z=0$, prenons $Q=\left\lbrace(-1,0,0),(1,0,0)\right\rbrace$, $R=Q\cup\left\lbrace(0,10,0)\right\rbrace$ et ajoutons $(0,-9/10,0)$ à $X$. La miniball de $Q$ contient ce dernier point, tandis que celle de $R$, de centre $(0,99/20,0)$ et de rayon $101/20$, ne le contient pas. La saturation ne doit donc pas être traitée comme un opérateur de clôture ordonné dans une preuve de pruning ou dans un cache.

Tous les énoncés ci-dessous valent pour les coupes fermées avec $t(S)\leq a$. Pour une coupe ouverte, chaque inégalité d'activation devient strictement $t(S)<a$.

## 2. Théorèmes structurants

### S.1 — invariance de la miniball par saturation

Pour tout $Q\subseteq X$ non vide,

$$\beta(\mathrm{Sat}(Q))=\beta(Q),\qquad B_{\mathrm{Sat}(Q)}=B_Q,\qquad \mathrm{Sat}(\mathrm{Sat}(Q))=\mathrm{Sat}(Q).$$

**Preuve.** On a $Q\subseteq\mathrm{Sat}(Q)$, donc la monotonie de la miniball donne $\beta(Q)\leq\beta(\mathrm{Sat}(Q))$. La boule $B_Q$ contient par définition tous les points de $\mathrm{Sat}(Q)$, donc elle est admissible pour ce dernier ensemble et donne l'inégalité inverse. Les rayons sont égaux; l'unicité de la miniball euclidienne impose alors l'égalité des boules, puis l'idempotence. $\square$

### S.2 — complétude par supports de taille au plus quatre

Tout générateur saturé de $\Sigma_X$ possède un support minimal $U\subseteq X$ tel que

$$1\leq\lvert U\rvert\leq4,\qquad c_U\in\mathrm{relint}\,\mathrm{conv}(U),\qquad B_U=B_S,\qquad \mathrm{Sat}(U)=S.$$

**Preuve.** Choisissons un $Q$ dont le saturé vaut $S$. En dimension trois, la miniball de $Q$ possède un support minimal affinement indépendant d'au plus quatre points. Un support minimal a des coordonnées barycentriques strictement positives pour son centre, donc le centre appartient à l'intérieur relatif de son enveloppe convexe. Ce support détermine la même miniball que $Q$; S.1 donne alors le même saturé $S$. $\square$

Réciproquement, tout sous-ensemble affinement indépendant $U$ de taille au plus quatre dont le centre circonscrit dans $\mathrm{aff}(U)$ est dans $\mathrm{relint}\,\mathrm{conv}(U)$ détermine sa miniball. Une classification fermée exacte de tous les points construit donc un vrai générateur $\mathrm{Sat}(U)$.

Ainsi, l'énumération de **tous** les supports $U$ de tailles un à quatre, suivie d'un range reporting fermé exact et d'une déduplication exacte, est complète. Cette conclusion ne requiert pas de position générale. En présence d'un grand shell cosphérique, plusieurs supports minimaux peuvent témoigner la même boule : l'implémentation doit conserver le shell fermé entier, agréger les témoins et choisir une provenance canonique.

### S.3 — génération exacte du complexe de Čech

Définissons le complexe abstrait fermé

$$\mathcal{C}(a)=\left\lbrace Q\subseteq X:Q\neq\varnothing,\ \beta(Q)\leq a\right\rbrace$$

et le simplexe abstrait non vide $\Delta(S)=\left\lbrace Q:\varnothing\neq Q\subseteq S\right\rbrace$. Alors

$$\mathcal{C}(a)=\bigcup_{\substack{S\in\Sigma_X\\t(S)\leq a}}\Delta(S).$$

**Preuve.** Si $Q\in\mathcal{C}(a)$, S.1 donne $Q\subseteq\mathrm{Sat}(Q)$ et $t(\mathrm{Sat}(Q))=\beta(Q)\leq a$. Réciproquement, si $Q\subseteq S$ et $t(S)\leq a$, la miniball de $S$ est une boule admissible pour $Q$, donc $\beta(Q)\leq t(S)\leq a$. $\square$

Ce théorème est la formulation exacte de la rétropropagation globale. Le générateur $S$ représente implicitement toutes ses faces; aucune énumération de $2^{\lvert S\rvert}-1$ sous-ensembles n'est nécessaire.

### S.4 — composantes par intersections de générateurs

À l'ordre $k\geq1$, le graphe $\Gamma_k(a)$ a pour sommets les $k$-sous-ensembles $Q$ tels que $\beta(Q)\leq a$ et relie deux sommets qui diffèrent d'un point lorsque leur union est encore dans $\mathcal{C}(a)$. Pour un générateur actif $S$ de taille au moins $k$, les $k$-faces de $S$ portent le graphe de Johnson $J(\lvert S\rvert,k)$, qui est connexe.

Toute arête de Gamma est contenue dans le générateur saturé de sa coface, et toute arête de Johnson portée par un générateur actif est une arête de Gamma. Par conséquent,

$$\Gamma_k(a)=\bigcup_{\substack{S\in\Sigma_X\\t(S)\leq a,\ \lvert S\rvert\geq k}}J_k(S).$$

Définissons $H_k(a)$ avec un sommet par générateur actif de taille au moins $k$ et une arête $ST$ lorsque $\lvert S\cap T\rvert\geq k$. Deux graphes de Johnson de la famille ont un sommet commun exactement sous cette condition. Le théorème élémentaire des composantes d'une union de sous-graphes connexes donne donc une bijection entre les composantes de $H_k(a)$ et celles de $\Gamma_k(a)$.

La couverture en observations de la composante représentée par $\mathcal{A}\subseteq\Sigma_X$ vaut simplement

$$\mathrm{coverage}(\mathcal{A})=\bigcup_{S\in\mathcal{A}}S.$$

Cette famille est une couverture; elle n'est pas nécessairement une partition du nuage.

### S.5 — compression simultanée des ordres par Kruskal décroissant

Sur le graphe $H_1(a)$ des générateurs actifs, qui ne contient que les paires d'intersection non vide, attribuons à chaque arête le poids

$$w(S,T)=\lvert S\cap T\rvert.$$

Soit $F(a)$ une **forêt couvrante de poids maximum**, calculée par Kruskal en poids décroissants avec un ordre total global sur les identités canoniques d'arêtes pour les égalités. Cet ordre ne dépend ni du lot d'activation, ni de l'ordre de découverte. Pour tout $k$, les composantes de la sous-forêt contenant les arêtes de poids au moins $k$, après suppression des sommets de capacité $\lvert S\rvert<k$, sont exactement celles de $H_k(a)$ et donc de $\Gamma_k(a)$.

**Preuve.** Kruskal traite d'abord toutes les arêtes de poids au moins $k$. À la fin de ce préfixe, sa forêt couvre chaque composante du sous-graphe défini par ce seuil. Les arêtes de poids inférieur ajoutées ensuite ne modifient pas ce préfixe après seuillage. $\square$

Le mot « maximale » seul est insuffisant et doit être proscrit ici. Dans un triangle de poids $2,2,1$, l'arbre formé d'une arête de poids $2$ et de celle de poids $1$ est couvrant et maximal au sens d'inclusion, mais il déconnecte le seuil $k=2$ alors que les deux arêtes de poids $2$ connectent le triangle.

Les inclusions $H_{k+1}(a)\subseteq H_k(a)$ rendent les applications verticales uniques au niveau des **composantes de coupe**. Cela ne construit pas encore les identifiants, journaux et morphismes canoniques du contrat logiciel.

### S.6 — sous-famille certifiée

Pour toute sous-famille $\Sigma'\subseteq\Sigma_X$, l'union des simplexes actifs est un sous-complexe de $\mathcal{C}(a)$. Les connexions obtenues sont donc toujours des connexions de Gamma. Une sous-famille peut omettre des faces, retarder des naissances ou des fusions et produire des composantes trop fines; elle ne peut pas créer une fausse connexion.

Cette garantie possède la sémantique scientifique interne `mode=budgeted, forest_semantics=partial_refinement`. Le schéma v2 ne contient toutefois aucune base de preuve pour une sous-famille saturée : avant migration contractuelle, l'expérience reste un artefact de recherche et ne sérialise aucun `MorseHGP3DResult` public, même conditionnel. Après une telle migration, son statut public resterait non exact tant que l'exhaustivité de la famille n'est pas certifiée. Les fusions d'une forêt partielle ne sont pas, par ce seul fait, des événements HGP exacts.

## 3. Résolution de la fixture à cinq points

Dans `gabriel-point-set-counterexample-5-points-v1`, les cofaces non-Gabriel `ACD` et `ACE` ont la même miniball que `AC`. Leur saturation fermée est

$$S_1=\mathrm{Sat}(AC)=\mathrm{Sat}(ACD)=\mathrm{Sat}(ACE)=ACDE$$

au niveau $33/2$. Le générateur $S_1$ contient implicitement les six arêtes `AC`, `AD`, `AE`, `CD`, `CE`, `DE`; l'incidence silencieuse de `AC` est donc présente avant la fusion ultérieure.

Le générateur $S_2=ABC$ s'active au niveau $83886/3563$ et vérifie $\lvert S_1\cap S_2\rvert=2$. Il connecte donc exactement les deux blocs à l'ordre deux, sans les connecter à l'ordre trois.

Une sonde locale non versionnée exécutée pendant cet audit avec l'oracle rationnel existant a retrouvé 22 saturés distincts et l'accord des composantes de Gamma, du graphe d'intersection et de sa forêt de Kruskal aux coupes critiques fermées et aux ordres un à cinq. Son algorithme de réduction était séparé, mais il partageait la géométrie de référence : ce n'est donc pas un différentiel géométriquement indépendant. Cette sonde soutient le diagnostic; elle ne remplace ni les preuves S.1–S.5, ni une fixture et une campagne versionnées.

## 4. Ce que la construction change, et ce qu'elle ne change pas

### 4.1 Deux bornes de rang différentes

La borne active $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ reste correcte pour le catalogue des événements de Morse susceptibles de modifier immédiatement $H_0$ aux ordres demandés. Elle ne borne pas la taille des générateurs saturés nécessaires à S.3.

Une $k$-face peut avoir une miniball contenant un nombre arbitraire de points strictement intérieurs. Son unique saturé au même niveau peut donc avoir une taille proche de $n$, même pour $k\leq10$. Une troncature $\lvert S\rvert\leq K_{\mathrm{eff}}+1$ est fausse en général pour la tour saturée pure.

### 4.2 Lecture Morse à corriger

Sous les hypothèses génériques adaptées à **tous** les rangs, un générateur $S=I\cup U$ peut correspondre à un minimum à l'ordre $\lvert S\rvert$ et à un événement d'indice un à l'ordre $\lvert S\rvert-1$. Cette interprétation n'est pas couverte par le `RelevantGP` actuel, limité aux rangs utiles au catalogue tronqué.

Aux ordres $k\leq\lvert S\rvert-2$, l'activation implicite de $\Delta(S)$ est un fait combinatoire de Čech/Gamma. Il est incorrect de l'appeler systématiquement un événement de Morse d'indice $\lvert S\rvert-k$ : la multiplicité locale $\binom{\lvert U\rvert-1}{\lvert S\rvert-k}$ peut être nulle. La tour saturée conserve une incidence latente; elle ne crée pas artificiellement un point critique.

La fixture permanente [`morse_rank_window.json`](../../tests/fixtures/regressions/morse_rank_window.json) fixe un saturé de rang quatre avec deux points intérieurs, deux points de support et une observation extérieure. Le niveau de sa miniball est critique seulement pour $k=3,4$; $D_2(c)$ est strictement inférieur à ce niveau et $D_5(c)$ lui est strictement supérieur.

### 4.3 Une forêt de coupe n'est pas encore le `MergeForest`

À niveau fixé, $F(a)$ est un certificat compact de toutes les composantes par ordre. Quand $a$ croît, les remplacements d'arêtes de la forêt de poids maximum sont des changements de représentation, pas des naissances ou des fusions topologiques.

Une implémentation conforme doit encore, par lot de niveau exact :

- comparer les composantes strictement pré-lot et post-lot à chaque ordre demandé;
- produire les naissances, prolongements et multifusions canoniques;
- enregistrer chaque croissance silencieuse dans `coverage_log`;
- construire les applications verticales au même état post-lot;
- assurer la commutation des carrés et des identifiants déterministes;
- rendre le rejeu indépendant de l'ordre des supports, des arêtes et des threads.

La suite persistante des snapshots de $F(a)$ permet ces calculs, mais ne les remplace pas.

## 5. Mise à jour dynamique et pruning

Lorsqu'un lot de nouveaux générateurs est activé, chaque nouvelle arête relie au moins un sommet du lot. En insertion seule, si $F^{-}$ est la forêt de Kruskal avant le lot et $E_{\mathrm{new}}$ l'ensemble complet de ces nouvelles arêtes, relancer Kruskal sur $F^{-}\cup E_{\mathrm{new}}$ avec le même ordre total global produit la même forêt canonique que sur toutes les arêtes post-lot. C'est la propriété de sparsification des forêts de poids maximum, appliquée aux poids raffinés par l'ordre canonique. Elle suppose :

- que toutes les arêtes du lot ont été générées ou exclues par certificat;
- que le lot de niveaux égaux est atomique;
- que le même ordre total global départage partout les poids égaux;
- qu'aucune suppression de sommet ou d'arête ancienne n'est intercalée.

**Preuve de sparsification.** Toute ancienne arête $e\notin F^{-}$ a été rejetée par Kruskal parce que ses extrémités étaient déjà reliées par un chemin d'arêtes anciennes qui précèdent toutes $e$ dans l'ordre total. Ce chemin demeure dans $F^{-}$. Dans le graphe $F^{-}\cup E_{\mathrm{new}}$, Kruskal relie donc encore les extrémités de $e$ avant sa position, éventuellement par des arêtes nouvelles encore antérieures. L'arête $e$ resterait rejetée dans le calcul complet; toutes les anciennes arêtes non-arbre peuvent être omises simultanément. $\square$

Après finalisation du lot, les anciennes arêtes non retenues peuvent être libérées pour le calcul insertionnel courant, à condition que leur complétude de génération reste attestée dans le certificat.

Si $S\subseteq T$ et $T$ est actif, l'inclusion $\Delta(S)\subseteq\Delta(T)$ autorise sémantiquement le retrait de $S$ de la famille de coupe. Pour tout autre générateur $R$, on a $\lvert R\cap S\rvert\leq\lvert R\cap T\rvert$. Toutefois, retirer $S$ transforme l'algorithme insertionnel en algorithme avec suppressions. La contraction persistante, le rewiring, la provenance et les applications historiques constituent une obligation de preuve séparée. Le premier oracle exact doit donc exécuter le pruning **désactivé** et le comparer ensuite à une variante activée.

Le pruning par inclusion intervient en outre après la découverte et la saturation des générateurs concernés. Il peut réduire l'état courant, mais il n'annule ni le coût déjà payé ni l'historique antérieur.

## 6. Audit de complexité

Le nombre de supports bruts vaut

$$C_U=n+\binom{n}{2}+\binom{n}{3}+\binom{n}{4}=O(n^{4}).$$

À $n=50\,000$, le seul terme $\binom{n}{4}$ vaut $260\,385\,417\,812\,487\,500$. Cette voie brute est exclue du régime produit actuel.

Soit $M=\lvert\Sigma_X\rvert$, $L_{\mathrm{sat}}=\sum_{S\in\Sigma_X}\lvert S\rvert$ et $d_x=\lvert\left\lbrace S:x\in S\right\rbrace\rvert$. Les coûts à mesurer sont au minimum :

$$M\leq C_U=O(n^{4}),\qquad L_{\mathrm{sat}}\leq nM=O(n^{5}),\qquad P_{\mathrm{post}}=\sum_{x\in X}\binom{d_x}{2}=\sum_{S<T}\lvert S\cap T\rvert.$$

La saturation naïve par balayage global effectue $O(nC_U)=O(n^{5})$ classifications exactes. Le graphe d'intersection peut contenir $O(M^{2})=O(n^{8})$ paires, et $P_{\mathrm{post}}$ peut atteindre $O(nM^{2})=O(n^{9})$ visites avec répétition. Un accumulateur temporaire pour un seul nouveau générateur peut déjà contenir $M-1$ compteurs.

La borne $M-1$ ne concerne que les arêtes de la forêt **résidente**. Elle ne borne ni les memberships, ni les postings, ni le scratch du join, ni le nombre d'arêtes examinées. Le journal persistant possède seulement la borne générale $O(M^{2})$ héritée des paires examinables; aucune borne sous-quadratique n'est démontrée pour cette famille. Le tri des niveaux rationnels, la bit-complexité multiprécision, la déduplication des boules et la sérialisation des couvertures doivent également être comptés.

La construction de la bifiltration multicoverture de Corbet, Kerber, Lesnick et Osang motive une complexité combinatoire $O(n^{d+1})$ sous position générale pour son propre modèle tous ordres. En dimension trois, l'article donne aussi une taille tronquée $O(n^{2}K^{2})$ pour son modèle et décrit une construction naïve tous ordres en $O(n^{5})$. Ces résultats fournissent des baselines importantes, mais ne démontrent ni la tour d'ensembles saturés ci-dessus, ni une borne $O(n^{4})$ pour ses memberships, son join ou son historique. L'expression « ombre $H_0$ du rhomboid tiling » est une interprétation, pas un théorème de cette référence. Voir la [copie référencée dans le dépôt](../references/pdfs/corbet-et-al-multicover-bifiltration-arxiv-2103.07823v3.pdf).

## 7. Intérêt pratique par régime

| régime | intérêt | décision auditée |
|---|---|---|
| petits $n$, tous les ordres | très élevé | nouvel oracle exact indépendant de Gamma explicite et du catalogue Morse tronqué |
| tailles modérées, tous les ordres | élevé à mesurer | forte compression des faces, mais coût des saturés et intersections non borné par la forêt |
| $K_{\max}=10$, $n=50\,000$ | faible en brut | no-go pour l'énumération $O(n^4)$; conserver la voie top-$m$ actuelle |
| supports proposés puis saturés | moyen à élevé | améliore la sous-filtration certifiée interne; toute exposition `partial_refinement` attend une migration du schéma |
| futur moteur output-sensitive | potentiel élevé | exige des certificats de complétude pour les générateurs et le join d'intersections |

Le produit actuel demande au plus dix ordres. L'affirmation « plus légère pour $K=n$ » décrit donc une extension tous ordres et non le domaine de performance actuellement promis.

## 8. Obligations avant activation contractuelle

Une future base de preuve distincte, par exemple `saturated_ball_overlap_proved`, ne pourra être ajoutée qu'après une migration contractuelle explicite. Elle ne doit réutiliser ni `CriticalEvent`, ni `gamma_cofaces`, car un générateur peut avoir un rang proche de $n$ et plusieurs supports témoins.

Le certificat devra au minimum établir :

1. `support_universe_complete` : tous les supports de tailles un à quatre ont été examinés ou exclus;
2. `closed_ball_ranges_complete` : chaque saturé contient exactement tous les points de sa boule fermée;
3. `ball_dedup_complete` : les boules et supports multiples sont canonicalisés sans perte;
4. `generator_batches_complete` : tous les générateurs sont activés dans le bon lot exact;
5. `overlap_join_complete` : chaque intersection nécessaire a été comptée ou exclue;
6. `generator_msf_complete` : chaque forêt de Kruskal préserve toutes les coupes demandées;
7. `merge_replay_complete` : naissances, multifusions et croissances silencieuses sont rejouables;
8. `vertical_maps_complete` : les applications verticales sont uniques et tous les carrés commutent.

Jusqu'à cette migration, `gamma_exhaustive_reference` reste la seule base active exacte de `hgp_reduced`; `full_pi0` conserve son statut contractuel actuel. La preuve S.1–S.5 ouvre une voie indépendante de M.1, mais ne ferme ni M.1 pour la voie Morse, ni les obligations logicielles de cette nouvelle voie.

## 9. Expérience décisive

### 9.1 Oracle exact borné

Pour $n\leq14$, énumérer les supports de tailles un à quatre, construire leurs saturés sans pruning et comparer, à chaque coupe ouverte et fermée et pour chaque ordre interne $1\leq k\leq n$. Les comparaisons de sérialisation v2 restent limitées à $1\leq k\leq\min(10,n)$ et n'activent aucune nouvelle sortie publique. Vérifier :

- le complexe descendant matérialisé lorsque le budget le permet;
- les sommets, arêtes et composantes de Gamma exhaustif;
- le graphe $H_k(a)$;
- une forêt de Kruskal recalculée depuis zéro;
- sa version incrémentale lot par lot;
- les profils `full_pi0` et `hgp_reduced` et leurs journaux;
- les applications verticales et les carrés de naturalité.

La matrice doit inclure la fixture à cinq points, des niveaux égaux, des permutations d'entrée et de lots, des shells cosphériques à supports multiples, et une famille où le saturé d'une petite face contient arbitrairement beaucoup de points. Toute contradiction devient une fixture minimale permanente avant optimisation.

### 9.2 Shadow benchmark borné

Avant toute extrapolation, mesurer sur CPU et avec arrêt budgétaire $n\in\left\lbrace16,24,32,48,64,96,128\right\rbrace$ pour des nuages volumiques, surfaciques, en amas et adversariaux :

- supports proposés et acceptés par taille;
- $M_{\mathrm{sat}}$, $L_{\mathrm{sat}}$ et distribution de $\lvert S\rvert$;
- longueur et maximum des postings $d_x$;
- $P_{\mathrm{post}}$, paires uniques et pic de l'accumulateur;
- arêtes examinées, retenues et remplacements de forêt;
- octets de l'historique et pic mémoire;
- temps de saturation, déduplication, join, Kruskal, requêtes et rejeu;
- bit-complexité et replis exacts;
- comparaison aux compteurs $\sum_m(M_m+P_m+V_m+J_m)$ de la voie actuelle.

Les baselines obligatoires sont : graphe statique complet contre postings dynamiques, forêt recalculée contre mise à jour insertionnelle, pruning désactivé contre pruning expérimental, et supports proposés par la cascade actuelle puis saturés. Cette dernière baseline reste partielle tant qu'elle n'a pas de certificat d'exhaustivité.

### 9.3 Porte de décision

Le prototype devient une référence interne exacte bornée seulement après zéro différence et certificats complets. Une voie de production ne s'ouvre que si une génération output-sensitive évite l'énumération de tous les quadruplets, si un certificat sparse évite le join dense sans manquer d'intersection, et si la persistance déterministe tient sous les budgets publiés. Sinon, la tour reste un oracle interne exact petit $n$ et une amélioration partielle hybride.

## 10. Statut scientifique après audit

| affirmation | statut |
|---|---|
| S.1, S.3, S.4, S.5 et S.6 | `proved_here` |
| complétude de l'énumération abstraite par supports de taille au plus quatre | `proved_here` |
| conformité d'une implémentation complète de cette énumération | `conditional_theorem` sous certificats exacts |
| troncature des générateurs à $K_{\mathrm{eff}}+1$ | `false_in_general` |
| snapshots de forêt égaux au `MergeForest` contractuel | `false_in_general` sans construction de rejeu supplémentaire |
| persistance, pruning et canonisation dégénérée conformes au contrat | `proof_obligation` |
| scalabilité au régime $50\,000$, $K_{\max}=10$ | `false_in_general` pour la voie brute; `experimental_target` pour une voie output-sensitive |
