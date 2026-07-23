# Architecture industrielle de la chaîne Morse horizontale

## Portée

Cette note couvre le passage direct de la Phase 10 à la Phase 14. Le backend courant est `reference_cpu`, le profil `hgp_reduced`, le mode scientifique `certified` et le mode de déploiement initial `architecture_only`. Aucun temps de réponse, volume maximal ou statut public n'est revendiqué.

Les deux usages partagent exactement le même moteur scientifique :

1. flux direct de supports terminaux de Phase 9;
2. minima et catalogue complet des simplexes de Gabriel 10.1, y compris les selles tardives;
3. au plus quatre bras stricts par selle 10.2;
4. descentes certifiées 10.5c sous snapshot gelé;
5. quotient hypergraphe atomique sur les carriers typés $R(r)$ ou $L(h)$, sans suppression préalable des latents, puis classification par $q_R$;
6. commit des unions et des minima, ceux d'ordre au moins deux restant latents;
7. journal compact des nœuds, enfants et racines finales.

Seul l'ordonnanceur change entre le profil résident 50 k et le profil streaming massif.

## Complexité structurante

Notons $B$ le nombre de minima directs, $J$ le nombre de simplexes de Gabriel, $A$ le nombre de bras stricts, $V$ le nombre de clés distinctes visitées par les descentes, $G$ le nombre de groupes atomiques et $C$ le nombre de références d'enfants. En dimension trois :

$$A=\sum_{S}\lvert U_S\rvert\leq4J.$$

Le journal horizontal conserve une quantité de données proportionnelle à :

$$O(B+A+J+G+C).$$

Le graphe fonctionnel de descente est transitoire et coûte :

$$O(A+KV),\qquad K\leq10.$$

Il est projeté vers les seules liaisons bras--racine avant le lot suivant. Ces bornes ne contiennent ni $\binom{n}{K}$, ni $\binom{n}{K+1}$. Elles ne constituent toutefois pas une garantie sous-linéaire en $n$ : une requête top-$K$ LBVH peut visiter un nombre linéaire de nœuds, et le nombre d'événements directs dépend de la géométrie du nuage.

## Profil résident interactif

Pour un nuage d'environ 50 000 points et $K\leq10$, l'objectif est de conserver nuage, LBVH, flux compact, carriers, racines réduites optionnelles, scratch de descentes et forêt dans les arènes résidentes. Tous les lots exacts doivent tenir dans un unique plan d'exécution; aucun niveau égal ne peut être coupé. La cible future utilise des lots de bras par classes de cardinal, des traversées top-$K$ groupées, des CUDA Graphs et un fallback exact explicitement compté.

Le protocole `warm_e2e` inclura le transfert du nuage, le LBVH, les décisions certifiées, la réduction et la matérialisation. Un plan résident accepté signifie seulement que les capacités déclarées suffisent; il ne signifie jamais que le p95 est inférieur à une seconde.

## Profil streaming massif

Pour 10 000 000 points ou davantage, un chunk est une suite de lots exacts complets. La frontière d'un chunk tombe uniquement entre deux lots; elle ne coupe jamais des selles de même ordre et de même niveau. Après chaque chunk, l'ordonnanceur peut publier un run checksummé contenant les actions compactes, le stamp du locator et la frontière source consommée.

Le flux de supports de Phase 9, les descentes et la réduction peuvent employer des budgets différents, mais une saturation doit conserver le lot courant intact pour reprise. Le merge externe porte sur les identités canoniques des lots et des actions, pas sur des cellules géométriques. Aucun atlas, aucune couture de mosaïque et aucun historique Gamma n'est nécessaire.

## Incrément 14B — transaction DSU sparse

Le locator 10.5a ne clone plus ses $H$ parents à chaque lot. Après les requêtes strictement pré-lot, il réserve un journal de $U_b$ handles pour $U_b$ demandes d'union. Une union effective journalise sa racine réorientée avant l'unique écriture de parent; le DSU reste sans compression de chemin. Si $W_b$ unions changent réellement une racine, alors

$$W_b\leq\min(U_b,H-1)\quad\text{pour }H>0.$$

Un succès conserve ces $W_b$ écritures et abandonne le journal. Tout rejet post-unions restaure exactement les $W_b$ racines en ordre inverse. Les requêtes restent strictement pré-lot, les doublons voient toutes les unions du lot, et schéma, digest canonique comme vérificateur structurel restent inchangés. Les trois compteurs de travail du résultat de lot sont éphémères et exclus de l'histoire durable.

Le scratch transactionnel passe ainsi de $O(H)$ à $O(U_b)$, avec exactement $W_b$ écritures sur succès et $2W_b$ sur rollback. Aucun parcours des handles non touchés, aucune facette, coface, cellule, incidence Gamma ou mosaïque d'ordre supérieur n'est introduit. Ce gain concerne directement les petits lots interactifs et les chunks massifs; il ne borne pas la profondeur des chaînes DSU et ne qualifie à lui seul aucun SLO.

## Incrément 14C — lanes structurelles des descentes

`ExactDirectSparseFacetDescentBatchPlanResult` rejoue fraîchement 14A avec le plafond de chunks fourni, puis classe, à l'intérieur de chaque lot exact et de son chunk propriétaire, les familles de selles selon le cardinal deux, trois ou quatre de leur support positif. Le plafond est testé avant chaque rétention de chunk et tout dépassement efface le préfixe transitoire : aucun plan source partiel n'entre dans 14C. Il existe donc au plus trois lanes par lot. Une lane conserve uniquement les intervalles candidats du journal 10.2, le cardinal de facette, le cardinal de support, le nombre d'intérieurs et des compteurs de travail; elle ne copie aucune clé, facette, permutation de bras ou valeur exacte.

Pour une facette de cardinal $k\leq10$, une construction locale autonome examine au plus $N_k$ supports candidats par passe, avec :

$$N_k=\sum_{j=1}^{\min(4,k)}\binom{k}{j}\leq385.$$

Le rejeu frais de 10.5b effectue au plus quatre passes par graine initiale. Une famille de support $r\in\lbrace2,3,4\rbrace$ possède exactement $r$ bras et reçoit donc la borne autonome $4rN_k$. Le plan additionne cette borne sans supposer les réemplois de la fermeture commune, puis découpe les graines en lancements d'au plus `maximum_initial_seed_work_item_count_per_launch`. Au bord $K=10$, une famille de support deux et neuf intérieurs porte ainsi deux graines et au plus $2\times4\times385=3080$ examens initiaux.

Les lanes ne sont pas des transactions scientifiques indépendantes. Toutes celles d'un lot doivent lire le même snapshot locator gelé, sélectionner leurs familles une seule fois dans un scratch stable, alimenter une unique fermeture 10.5c et partager sa mémoïsation; les résultats reviennent par `arm_seed_index` avant le quotient atomique. Cette contrainte évite de recalculer les suffixes partagés et interdit de couper un niveau égal entre kernels ou checkpoints.

Le prédicat `complete_architecture_plan()` vérifie seulement la forme et les agrégats du payload compact. Une redistribution coordonnée des comptes pourrait conserver cette forme; `fresh_reconstruction_required_before_execution=true` impose donc `verify_exact_direct_sparse_facet_descent_batch_plan` avant toute exécution ou persistance. Le plan ne borne pas encore les successeurs découverts par la fermeture commune, la profondeur LBVH, la difficulté des rationnels, les octets de scratch combinés avec 14A ou le temps GPU. Il ne qualifie ni le SLO 50 k, ni le profil 10 M+.

Cet incrément prépare l'exécuteur 14D commun aux deux profils : résident, toutes les lanes restent dans l'unique chunk; streaming, elles restent dans le chunk de leur lot indivisible. Il ne construit aucune facette absente, incidence Gamma, coface globale, cellule ou mosaïque de Delaunay d'ordre supérieur.

## Incrément 14D — exécuteur ancré et fermeture transitoire

`ExactDirectSparseFacetDescentAnchoredBatchExecutor` ouvre une session en reconstruisant et comparant exactement le plan 14C une seule fois. Il possède ensuite cette reconstruction fraîche et un curseur canonique; aucun appel par lot ne rejoue 14A ou le flux complet 10.1--10.2. Les références vers le nuage, le LBVH, la façade et les journaux restent vivantes et immuables, tandis que le locator est gelé séparément pendant chaque préparation et vérification. Ces autorités doivent être des lvalues dont la durée de vie couvre la session; l'API n'interdit pas encore statiquement un temporaire.

Pour un lot comportant $A$ bras, la sélection parcourt une fois ses familles, reconstruit les facettes à la demande et trie les clés complètes afin d'obtenir $D\leq A$ graines distinctes. Une unique fermeture 10.5c partage le snapshot et la mémoïsation entre toutes les lanes. Sa portée lexicale se termine avant la publication : seuls survivent $D$ records clé--carrier--témoin et $A$ jointures compactes par `arm_seed_index`. Aucun `node_index`, `terminal_node_index`, graphe, arête, projection de graine ou miniball transitoire n'entre dans le delta.

Le stockage logique du delta vaut $O(KD+A)$. Le scratch de sélection vaut $O(KA+KD)$ en plus de l'unique fermeture locale; il ne s'additionne jamais aux fermetures des lots antérieurs. Le vérificateur rejoue seulement le lot courant sous le même plan ancré. Une égalité complète avance le curseur d'exécution, sans effectuer le quotient, les unions du locator ou le commit de hiérarchie; un diagnostic ou un stamp périmé conserve le lot rejouable.

Le profil résident peut conserver une seule session et un seul plan pour tout le nuage de 50 k points. Le profil streaming utilise le même moteur séquentiel et doit évacuer chaque delta avant le lot suivant; les runs et checkpoints durables restent à implémenter. Cette discipline supprime l'accumulation du graphe 10.5c, mais ne borne encore ni les visites top-$K$, ni les rationnels, ni le temps GPU, ni le p95.

## Goulots restant à traiter en Phase 14

Trois coûts dominants restent visibles après 14D :

- les traversées exactes top-$K$ par clé distincte, encore linéaires au pire cas;
- les constructions de miniballs et les fallbacks rationnels, à regrouper par cardinal et difficulté;
- la capacité réutilisable des scratchs et le rejeu exact du lot courant, qui ne doivent pas dupliquer durablement les grandes arènes dans le protocole `warm_e2e`.

La priorité de développement devient donc : séparer les propositions GPU de leur rejeu CPU exact, réutiliser des capacités de scratch sans conserver d'objet scientifique, puis instrumenter `warm_e2e`. Les runs et checkpoints compacts viennent ensuite pour le profil 10 M+. Multiplier les oracles combinatoires ou réintroduire les gateways historiques ne réduit aucun de ces coûts.

## Planification sans promotion

Le premier socle de Phase 14 calcule, depuis 10.1 et 10.2, les comptes exacts de minima, simplexes, bras et références de clés par lot. La borne $B+J$ sur les nœuds reste volontairement conservatrice puisque les minima d'ordre au moins deux sont latents et qu'un groupe crée au plus un nœud. Un modèle mémoire explicite fourni par l'appelant transforme ces comptes et une réserve de sommets de descente en octets. Le plan résident est accepté seulement si la totalité tient dans un unique chunk; le plan streaming agrège des lots consécutifs jusqu'aux caps et rejette atomiquement tout lot qui ne tient pas seul.

Ce planificateur ne construit ni facette, ni descente, ni forêt. Son prédicat recalcule en $O(C)$ pour $C$ chunks les partitions d'intervalles, agrégats, bornes, réserves, octets et frontières. Il ne mesure aucune latence et ne qualifie aucun volume. Son rôle est d'empêcher qu'une politique d'exécution casse les égalités exactes ou masque un besoin de mémoire avant le lancement scientifique. Comme il ne conserve aucun digest par lot, une falsification coordonnée entre chunks reste hors de ce contrôle interne; un rejeu frais contre 10.1--10.2 est requis avant persistance ou exécution industrielle.
