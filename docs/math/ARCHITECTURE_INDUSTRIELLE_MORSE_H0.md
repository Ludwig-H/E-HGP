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

## Goulots restant à traiter en Phase 14

Trois coûts dominants restent visibles après cette réduction :

- les traversées exactes top-$K$ par clé distincte, encore linéaires au pire cas;
- les constructions de miniballs et les fallbacks rationnels, à regrouper par cardinal et difficulté;
- le rejeu frais, qui ne doit pas dupliquer durablement les grandes arènes dans le protocole `warm_e2e`.

La priorité de développement devient donc : batch GPU des descentes, durées de vie d'arènes explicites, puis instrumentation. Multiplier les oracles combinatoires ou réintroduire les gateways historiques ne réduit aucun de ces trois coûts.

## Planification sans promotion

Le premier socle de Phase 14 calcule, depuis 10.1 et 10.2, les comptes exacts de minima, simplexes, bras et références de clés par lot. La borne $B+J$ sur les nœuds reste volontairement conservatrice puisque les minima d'ordre au moins deux sont latents et qu'un groupe crée au plus un nœud. Un modèle mémoire explicite fourni par l'appelant transforme ces comptes et une réserve de sommets de descente en octets. Le plan résident est accepté seulement si la totalité tient dans un unique chunk; le plan streaming agrège des lots consécutifs jusqu'aux caps et rejette atomiquement tout lot qui ne tient pas seul.

Ce planificateur ne construit ni facette, ni descente, ni forêt. Son prédicat recalcule en $O(C)$ pour $C$ chunks les partitions d'intervalles, agrégats, bornes, réserves, octets et frontières. Il ne mesure aucune latence et ne qualifie aucun volume. Son rôle est d'empêcher qu'une politique d'exécution casse les égalités exactes ou masque un besoin de mémoire avant le lancement scientifique. Comme il ne conserve aucun digest par lot, une falsification coordonnée entre chunks reste hors de ce contrôle interne; un rejeu frais contre 10.1--10.2 est requis avant persistance ou exécution industrielle.
