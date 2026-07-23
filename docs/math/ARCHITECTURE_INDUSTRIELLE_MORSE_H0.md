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

## Incrément 14E — vue canonique et borne incumbent

La sélection 14D produit déjà un tableau strictement croissant de $D$ clés complètes distinctes. Le nouveau point d'entrée 10.5c reçoit cette vue non propriétaire, vérifie clé par clé le domaine, le cardinal commun et l'ordre strict, puis utilise la position comme identité de graine. Il ne construit donc ni les $D$ records `ClosureSeed`, ni leur copie triée, ni le second tableau de clés distinctes. L'entrée historique conserve ces populations seulement lorsqu'elle doit réellement remettre en ordre des identités ou dédupliquer des clés.

Sur l'ABI LP64 auditée, un record de graine occupe 96 octets et une clé 88 octets. Le pic de la voie 14D perd donc $96D+96D+88D=280D$ octets. Les deux populations initiales encore vivantes sont les $A$ bras sélectionnés et les $D$ clés que 14D doit posséder pour sa jointure. Nœuds, arêtes, caches de miniballs et tables de mémoïsation restent transitoires et constituent des optimisations ultérieures distinctes.

Le top-$K$ LBVH borné possède parallèlement une surcharge à incumbents. Une proposition fournit au plus le rang demandé en `PointId`; le CPU vérifie domaine, unicité et exclusions, recalcule exactement toutes leurs distances et les impute au même budget que les autres feuilles. Les voisins ainsi certifiés initialisent seulement une borne supérieure. Toute boîte de borne strictement supérieure peut être élaguée; une égalité est toujours descendue. Les feuilles incumbentes déjà évaluées ne sont ni réévaluées ni recomptées dans les sous-arbres élagués, de sorte que la partition des points éligibles reste exacte.

Ce socle rend possible un transcript GPU compact par clé sans lui donner d'autorité scientifique. Le transcript et son audit devront rester séparés du delta 14D; une clé absente ou une proposition mauvaise déclenchera simplement la traversée exacte normale. Aucun producteur GPU, aucune classe de difficulté observée et aucune qualification de performance ne font encore partie de 14E.

## Incrément 14F — transcript sparse et baseline exacte

Le transcript hôte 14F conserve seulement $R\leq D$ lignes proposées, triées par clé complète, avec au plus $K$ identifiants par ligne. Il ne transporte ni centre, ni distance, ni cutoff, ni verdict de prune ou de successeur. Ses caps physiques et logiques sont contrôlés avant copie; la revalidation au point d'usage lie ensuite le payload au lot, au niveau exact, au stamp locator, aux clés canoniques initiales et au namespace du nuage avant toute construction de fermeture.

La baseline exacte d'une clé est sa facette source complète $F$. Au centre de sa miniball, les $K$ points de $F$ fournissent un cutoff au plus égal à $\beta(F)$. Pour une proposition $P$, le CPU évalue une fois chaque point de $U=F\cup P$, avec $K\leq\lvert U\rvert\leq2K$, et réduit exactement cette population aux $K$ meilleurs voisins pour initialiser la heap. Puisque $F\subseteq U$, le cutoff de $U$ n'est jamais pire que celui de $F$. Les points écartés de la heap mais égaux au cutoff restent dans la coquille, et toute borne LBVH égale est descendue.

Les appels historiques restent non amorcés. L'enveloppe 14F, elle, utilise toujours $F$ : un record présent ajoute $P$, tandis qu'une absence, une ligne vide ou un successeur dynamique utilise $P=\varnothing$. La forêt scientifique ne retient aucun record ou audit top-$K$; un objet séparé agrège seulement les comptes exacts observés. Ainsi un producteur futur peut échouer vers des lignes absentes ou vides sans créer de nouvelle sémantique.

Le stockage du transcript est $O(KR)$ et sa durée de vie est celle du lot. Il ne crée aucune facette absente, coface, incidence globale, cellule, Gamma ou mosaïque d'ordre supérieur. Le producteur CUDA, son epoch et son digest, le raccord à la session ancrée 14D, les classes de difficulté, les arènes réutilisables, `warm_e2e` et les runs durables restent ouverts.

## Incrément 14G — préparation propositionnelle, commit non amorcé

La session 14D expose désormais une préparation explicite avec transcript. Son ordre est fermé : jointure du plan, du chunk, du lot et des lanes; préflight des populations 14D; reconstruction stable des $A$ bras; tri-déduplication des $D$ clés; puis seulement revalidation 14F. Un dépassement de cap ne lit donc pas le transcript. Un transcript explicitement fourni sur un lot vide n'est pas ignoré : la fermeture vide sert à sa validation et à son audit, puis disparaît tandis que le delta conserve exactement le raccourci historique sans fermeture.

La sortie est une enveloppe de durée courte. Un succès contient d'une part le delta scientifique 14D, d'autre part l'audit scalaire de consommation; un rejet de transcript contient seulement l'audit de rejet et aucun delta. Aucun record, candidat, pool, partition, coquille, nœud ou graphe n'entre dans l'enveloppe ou dans la session. Le curseur reste immobile pendant toutes les préparations.

Le commit n'a volontairement pas de paramètre transcript. Une fois la préparation terminée, le producteur et son payload peuvent être détruits; `commit_prepared` reconstruit le même lot par la voie canonique historique, sans incumbent externe, puis compare le delta entier. Ce rejeu coûte encore une seconde exécution exacte du lot, mais il empêche qu'un epoch, digest, compteur GPU ou audit futur devienne une autorité scientifique par simple égalité structurelle. Il crée aussi une dette de vivacité : une préparation accélérée peut tenir sous un budget où le rejeu non amorcé s'épuise, auquel cas le curseur reste correctement fermé mais ne peut pas avancer sous les mêmes caps.

Le delta logique reste $O(KD+A)$ et le transcript sparse fourni par l'appelant $O(KR)$ pour $R\leq D$. `SelectedArm` conserve encore une clé complète par bras pendant que les clés distinctes sont construites, soit un scratch de sélection $O(KA+KD)$. Ces trois populations peuvent coexister avec l'unique fermeture locale; aucun pic mémoire global n'est donc qualifié. L'internement des clés lors de la reconstruction et les capacités réutilisables restent des optimisations séparées.

14G ne construit toujours aucune facette absente, coface, incidence globale, cellule, Gamma ou mosaïque de Delaunay d'ordre supérieur. Il ne qualifie ni producteur CUDA, ni SLO 50 k, ni capacité 10 M+, ni statut public.

## Incrément 14H — ticket scellé et commit sans rejeu

14H remplace le passage public d'un delta mutable par une capacité créée uniquement à l'intérieur de la préparation exacte. Ce ticket non copiable possède le delta $O(KD+A)$, un sceau de session partagé, l'epoch, le curseur source complet, le curseur successeur déjà validé et le stamp locator. Il appartient à l'appelant; la session ne retient aucun état préparé. Un déplacement neutralise la source et toute tentative de commit consomme la destination, qu'elle réussisse ou non.

Le commit compare seulement le sceau, l'epoch, cinq indices source et le stamp vivant, puis affecte cinq indices successeurs prévalidés. Il ne parcourt pas le delta, ne relance aucun top-$K$, miniball ou 10.5c et ne lit ni transcript ni audit. Sa complexité est donc constante dans les tailles $A$, $D$ et $K$. L'audit peut être détaché et détruit avant commit; s'il reste présent, il est déplacé séparément sans devenir une autorité. Le chemin historique à rejeu frais est conservé pour le diagnostic et incrémente le même epoch.

Le ticket est une preuve de provenance exacte locale au processus, pas un certificat durable ni un replay indépendant. Son bénéfice précis est la vivacité : après émission d'un ticket courant, aucun second budget géométrique ne peut bloquer son avancement. Le scheduler devra limiter le nombre de tickets simultanément détenus par l'appelant; 14H ne qualifie pas ce pic. Il ne réalise toujours ni transaction locator, ni quotient, ni réduction hiérarchique, et ne construit aucune structure globale interdite.

## Goulots restant à traiter après 14H

Trois coûts dominants restent visibles après 14D :

- les traversées exactes top-$K$ par clé distincte, encore linéaires au pire cas;
- les constructions de miniballs et les fallbacks rationnels, à regrouper par cardinal et difficulté;
- la capacité réutilisable des scratchs et le pic des tickets détenus par l'appelant, qui ne doivent pas dupliquer durablement les grandes arènes dans le protocole `warm_e2e`.

La priorité de développement devient donc : brancher un producteur GPU borné sur la frontière désormais scellée, réutiliser des capacités de scratch sans conserver d'objet scientifique et instrumenter `warm_e2e`. Les runs et checkpoints compacts viennent ensuite pour le profil 10 M+. Multiplier les oracles combinatoires ou réintroduire les gateways historiques ne réduit aucun de ces coûts.

## Planification sans promotion

Le premier socle de Phase 14 calcule, depuis 10.1 et 10.2, les comptes exacts de minima, simplexes, bras et références de clés par lot. La borne $B+J$ sur les nœuds reste volontairement conservatrice puisque les minima d'ordre au moins deux sont latents et qu'un groupe crée au plus un nœud. Un modèle mémoire explicite fourni par l'appelant transforme ces comptes et une réserve de sommets de descente en octets. Le plan résident est accepté seulement si la totalité tient dans un unique chunk; le plan streaming agrège des lots consécutifs jusqu'aux caps et rejette atomiquement tout lot qui ne tient pas seul.

Ce planificateur ne construit ni facette, ni descente, ni forêt. Son prédicat recalcule en $O(C)$ pour $C$ chunks les partitions d'intervalles, agrégats, bornes, réserves, octets et frontières. Il ne mesure aucune latence et ne qualifie aucun volume. Son rôle est d'empêcher qu'une politique d'exécution casse les égalités exactes ou masque un besoin de mémoire avant le lancement scientifique. Comme il ne conserve aucun digest par lot, une falsification coordonnée entre chunks reste hors de ce contrôle interne; un rejeu frais contre 10.1--10.2 est requis avant persistance ou exécution industrielle.
