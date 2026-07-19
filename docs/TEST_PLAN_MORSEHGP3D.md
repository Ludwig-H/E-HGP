# Plan de validation de MorseHGP3D

> [!IMPORTANT]
> Ce document est un **contrat de validation pour une implémentation future**. Les seuils de temps sont des objectifs réfutables sur une configuration G4 donnée, pas des garanties de complexité ni des performances déjà mesurées.

## 1. Objet du plan

MorseHGP3D doit construire, pour un nuage fini $X\subset\mathbb{R}^{3}$ et tous les ordres $1\leq k\leq K_{\max}\leq10$, une hiérarchie HGP en échelle. Le plan vérifie séparément :

1. la validité de chaque prédicat géométrique ;
2. la complétude du catalogue critique lorsqu'elle est annoncée ;
3. l'exactitude des forêts horizontales et des applications verticales ;
4. l'absence de faux certificat dans les modes incomplets ;
5. la robustesse numérique et le traitement explicite des dégénérescences ;
6. la reproductibilité, le débit, la mémoire et la reprise sur une VM G4 Spot.

Le résultat mathématique testé est prioritaire sur toute mesure de qualité par rapport à des étiquettes « terrain ». Un bon score de clustering ne compense jamais une erreur de hiérarchie.

Chaque campagne valide aussi [`implementation_status.toml`](implementation_status.toml) : une phase ne peut être marquée fermée sans porte satisfaite et preuves référencées.

## 2. Contrat mathématique observable

### 2.1 Filtration

Pour $t\geq0$, on note

$$L_k(t)=\left\lbrace y\in\mathbb{R}^{3}:\#\bigl(X\cap\overline{B}(y,\sqrt{t})\bigr)\geq k\right\rbrace.$$

La sortie observable doit représenter les composantes de cette filtration, ou le périmètre réduit explicitement défini par la spécification active. Les niveaux sont toujours stockés et comparés comme des distances au carré.

Pour $n\geq1$, les tests calculent $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Si $s_{\max}\geq2$, la dernière profondeur générique attendue est $m_{\star}=s_{\max}-2$; pour $s_{\max}=1$, seuls le minimum de rang un et la forêt triviale sont attendus. Les campagnes couvrent explicitement $K_{\max}=1$, $n<K_{\max}$, $n=K_{\max}$ et $n\geq K_{\max}+1$; en particulier, $n=K_{\max}=10$ donne $s_{\max}=10$ et $m_{\star}=8$, jamais un événement de rang onze.

Pour une sphère candidate $(c,r)$, on pose

$$I=X\cap B^{\circ}(c,r),\qquad U=X\cap\partial B(c,r),\qquad s=\lvert I\rvert+\lvert U\rvert.$$

Sous les hypothèses de position générale du mode certifié, $U$ est affine indépendant, $1\leq\lvert U\rvert\leq4$ et $c\in\mathrm{relint}\,\mathrm{conv}(U)$. Le catalogue doit enregistrer le support, le rang fermé $s$, le niveau $r^2$ et les ordres auxquels l'événement intervient. En particulier, un événement de rang $s$ porte la naissance d'indice $0$ à l'ordre $s$ et la selle d'indice $1$ à l'ordre $s-1$, lorsque ces ordres appartiennent au périmètre demandé.

### 2.2 Deux périmètres de sortie à ne pas confondre

Chaque exécution indique un `profile` :

| valeur | objet comparé | exigence |
|---|---|---|
| `hgp_reduced` | toutes les composantes à $k=1$, puis les K-polyèdres non triviaux, leurs unions de points et leurs fusions pour $k\geq2$ | conformité à l'EMST à $k=1$, puis réduction des composantes de Gamma exhaustif |
| `full_pi0` | toutes les composantes de $L_k(t)$, y compris les naissances et la généalogie triviales | comparaison complète à l'oracle topologique ; statut certifié seulement si la spécification le permet |

Un test ne doit jamais accepter une sortie `hgp_reduced` comme preuve implicite de `full_pi0`. À l'ordre $k\geq2$, les unions de points associées aux composantes forment en général une **couverture** et non une partition ; leur recouvrement est donc autorisé et doit être testé.

Le contrat actif est v2. `hgp_reduced` exact exige `reconstruction_contract_id=hgp-reduced-v2`, `proof_basis=gamma_exhaustive_reference` et `effective_backend=reference_cpu`. Une sortie issue du flot Gabriel brut exige `proof_basis=gabriel_positive_connectivity`, `forest_semantics=partial_refinement`, `require_exact=false` et un statut non exact. Les tests T0 rejettent toute autre combinaison, notamment la base v1 `reduced_manuscript_theorem_5` et la base future non activée `incidence_complete_reduction_proved`.

### 2.3 Modes et statuts d'exécution

Le mode demandé vaut `certified` ou `budgeted`; `forest_semantics` vaut `exact`, `partial_refinement` ou reste absent si aucune forêt n'est publiée. Le statut public obtenu reste distinct :

| statut public | signification testable | assertions autorisées |
|---|---|---|
| `exact` | catalogue complet sur le périmètre annoncé, tous les signes décisifs certifiés, lots de niveaux égaux fermés, aucune limite atteinte | égalité avec l'oracle et assertions d'absence |
| `conditional` | chaque événement émis est certifié, mais la complétude du catalogue ou des attaches n'est pas prouvée | validité des événements présents seulement |
| `budget_exhausted` | un budget explicite a interrompu une fermeture; le résultat éventuel est conditionnel | validité des événements présents et raison d'arrêt |
| `unsupported_degeneracy` | l'entrée sort du domaine exact déclaré, par exemple plateau non pris en charge | diagnostic déterministe, aucune hiérarchie présentée comme exacte |
| `numeric_failure` | un prédicat n'a pas pu être décidé ou rejoué | aucune sortie scientifique exploitable |

Un mode `budgeted` obtient `public_status=conditional` ou `public_status=budget_exhausted`, même si son rappel mesuré vaut $100\%$ sur tous les exemples connus. Avec `forest_semantics=partial_refinement`, il rend un `PartialScope` contenant profondeurs et ordres fermés, cellules canoniques closes et loci non résolus; il ne permet aucune assertion d'absence et son statut ne vaut jamais `exact`. Une réponse d'événements seuls omet `forest_semantics`. Si `require_exact=true`, toute impossibilité de produire `exact` doit provoquer un échec explicite, jamais une rétrogradation silencieuse; `forest_semantics=partial_refinement` est refusé à la validation.

## 3. Architecture générale des tests

Les tests sont répartis en sept niveaux. Un niveau ne devient bloquant qu'après validation de tous les niveaux qui le précèdent.

| niveau | objet | oracle principal | fréquence minimale |
|---:|---|---|---|
| T0 | schémas, déterminisme des générateurs, documentation et formats | validation statique | chaque changement |
| T1 | prédicats, miniballs, rangs et égalités de niveaux | arithmétique exacte indépendante | chaque changement |
| T2 | catalogue et hyperarêtes | oracle exhaustif $n\leq12$, étendu à $n\leq14$ | PR puis nocturne |
| T3 | forêts, couvertures et flèches verticales | graphe exhaustif à chaque seuil | PR puis nocturne |
| T4 | implémentations CPU/GPU et variantes de noyaux | tests différentiels et métamorphiques | nocturne G4 |
| T5 | robustesse, dégénérescences et charges adversariales | `exact`, `unsupported_degeneracy` ou statut budgétaire attendu | nocturne et release |
| T6 | performance, mémoire, streaming et reprise Spot | budgets et artefacts de mesure | jalon G4 et release |

Une optimisation GPU ne peut être fusionnée si elle modifie une sortie canonique T1–T4 sans que le changement soit justifié dans la spécification mathématique.

## 4. Oracle exhaustif indépendant

### 4.1 Périmètre

L'oracle exhaustif est obligatoire pour $n\leq12$ dans la suite de PR et pour $n\leq14$ dans la suite nocturne ou de release. Il couvre tous les ordres $1\leq k\leq\min(K_{\max},n)$, et pas seulement $k=10$.

L'oracle doit être développé séparément du backend :

- aucune réutilisation des noyaux CUDA de propositions, de rang ou de miniball ;
- aucun partage du code de réduction hyper-Kruskal, hormis le schéma sérialisé ;
- décisions par rationnels, expansions exactes ou multiprécision avec borne de signe ;
- entrées entières ou rationnelles pour les fixtures exactes ;
- double implémentation recommandée pour les cas délicats : bibliothèque géométrique exacte et référence multiprécision directe.

Une comparaison CPU/GPU n'est pas un oracle indépendant si les deux chemins utilisent le même prédicat fautif.

### 4.2 Énumération du catalogue critique

Pour chaque sous-ensemble $U\subseteq X$ de taille $1$ à $4$, l'oracle :

1. vérifie son rang affine ;
2. construit exactement le centre équidistant dans $\mathrm{aff}(U)$ ;
3. teste $c\in\mathrm{relint}\,\mathrm{conv}(U)$ par signes barycentriques exacts ;
4. classe tous les points de $X$ comme intérieurs, frontaliers ou extérieurs ;
5. rejette le candidat si le shell réel n'est pas exactement celui représenté, sauf traitement explicite d'un plateau ;
6. calcule $s=\lvert I\rvert+\lvert U\rvert$ et conserve les événements pertinents jusqu'au rang $s_{\max}$ ;
7. déduplique par la clé canonique `(interior_ids, boundary_ids, level_exact)`.

L'oracle compare au backend :

- la présence de chaque événement ;
- l'absence de chaque faux événement en mode `certified` lorsque le statut public vaut `exact` ;
- $I$, $U$, $s$, $r^2$ et la classe de dégénérescence ;
- les ordres de naissance et de selle ;
- l'égalité exacte ou l'ordre strict entre niveaux.

### 4.3 Énumération combinatoire des K-polyèdres

Pour chaque $k$, l'oracle énumère tous les labels $Q\in\binom{X}{k}$ et tous les co-labels $S\in\binom{X}{k+1}$. Il calcule exactement le rayon carré $\beta(Q)$ de la plus petite boule englobante de $Q$ et $\beta(S)$ pour les cofaces.

Soit $\Theta_k$ l'ensemble trié des valeurs distinctes de ces rayons, complété par des sentinelles strictement avant et après chaque niveau. À chaque $t\in\Theta_k$, l'oracle construit le graphe fini suivant :

- un sommet pour chaque $Q$ tel que $\beta(Q)\leq t$ ;
- une adjacency entre les facettes de $S$ lorsque $\beta(S)\leq t$ ;
- une réduction en composantes connexes sans supposer que les identifiants du backend coïncident.

Cette construction sert de référence directe aux K-polyèdres. Une variante séparée construit le sous-graphe K-Gabriel afin de vérifier sa garantie de connectivité positive, de mesurer les connexions manquantes et de falsifier toute promotion abusive. Elle doit reproduire le désaccord permanent `gabriel-point-set-counterexample-5-points-v1`; elle ne vérifie plus le théorème de réduction comme base exacte.

### 4.4 Comparaison à tous les seuils

Comparer seulement la liste des fusions est insuffisant en présence de niveaux égaux. Pour chaque valeur critique $a$, on compare les états :

1. à un niveau strictement inférieur à $a$ ;
2. au niveau fermé $a$ ;
3. à un niveau strictement supérieur à $a$ et inférieur au niveau distinct suivant.

La comparaison porte sur les partitions de labels actifs, les unions de points couvertes, les parents de la forêt et les flèches verticales. Deux sorties sont équivalentes si ces objets coïncident après canonicalisation, même si elles choisissent des représentants DSU ou des étoiles d'hyperarête différents.

### 4.5 Budget combinatoire et surveillance de l'oracle

L'oracle journalise le nombre de sous-ensembles, le temps et le pic mémoire. Les limites $n=12$ et $n=14$ sont des valeurs par défaut, non des hypothèses mathématiques :

- PR : toutes les fixtures déterministes et au moins 100 graines aléatoires par classe pour $n\leq12$ ;
- nocturne : au moins 1 000 nuages aléatoires répartis entre $4\leq n\leq14$ ;
- release : reprise de toutes les graines ayant déjà révélé un défaut, sans limite d'ancienneté ;
- minimisation automatique d'un contre-exemple par suppression de points et réduction des coordonnées.

Toute graine fautive devient une fixture permanente avant correction du code.

## 5. Tests unitaires des prédicats

### 5.1 Matrice des prédicats

| prédicat | cas positifs et négatifs obligatoires | sortie exigée |
|---|---|---|
| orientation affine en 3D | points génériques, coplanaires, quasi coplanaires | signe exact ou zéro exact |
| côté d'un plan de bissection | deux labels, grands offsets, différence d'un ULP | signe certifié |
| centre de support | $\lvert U\rvert=1,2,3,4$ | centre, dimension, témoin |
| relative-intériorité | support aigu, droit, obtus, barycentrique presque nul | tous les signes barycentriques |
| intérieur/frontière/extérieur | écarts larges puis quasi nuls | classe exacte |
| rang fermé | shells de tailles $1$ à $8$, multiplicités | $(\lvert I\rvert,\lvert U\rvert,s)$ |
| miniball | support effectif de taille $1$ à $4$, points redondants | rayon, centre, support minimal |
| égalité de niveaux | valeurs rationnelles égales, voisines et mal conditionnées | lot exact ou ordre exact |
| niveau rationnel homogène | supports 3/4, dénominateurs de signes initiaux variés, facteurs communs | `ExactLevel` canonique avec dénominateur positif |
| top-$k$ avec exclusions | $1\leq k\leq10$, jusqu'à 9 identifiants exclus | voisins, shell final, certificat |
| clé canonique | permutations, duplications d'émission, plateaux | sérialisation identique |

Pour la revue G1 qui clôt la Phase 2A, « tout T1 » désigne toutes les décisions numériques signées et toutes les constructions exactes effectivement livrées par le laboratoire de prédicats CPU : prédicats, centres, rangs affines locaux, classifications de supports et de sphères, niveaux, égalités et canonisation. Les requêtes spatiales globales de la matrice — notamment top-$k$ avec exclusions, shell global et miniball d'un nuage complet — restent soumises à leurs phases d'implémentation et aux portes G2 à G4; elles ne peuvent ni bloquer rétroactivement la référence arithmétique de 2A, ni être réputées validées par sa fermeture. Cette séparation n'autorise aucun faux signe : toute primitive numérique appelée ultérieurement reste couverte par G1 et doit conserver zéro décision erronée.

Dans le découpage incrémental de la Phase 2A, le lot 2A.5 couvre les centres circonscrits non triviaux de deux à quatre points définis par la feuille de route. Le lot 2A.6 complète la matrice avec le singleton, dont le centre est le point lui-même, la dimension affine zéro et le niveau carré nul; il certifie aussi les signes barycentriques, la réduction exacte des supports de frontière et le côté exact d'une sphère. Le lot 2A.7 ajoute le produit croisé instrumenté des `ExactLevel`, le tie-break par cardinalité puis identifiants canoniques, l'agrégation des provenances réduites et les lots de niveaux exactement égaux. Le centre reste un témoin vérifié, jamais un critère supplémentaire masquant l'association contradictoire d'un même support à deux niveaux. Cette chaîne locale ne remplace pas l'énumération de tous les sous-supports exigée par l'oracle miniball.

### 5.2 Cascade de précision

Chaque décision suit le contrat : proposition rapide, filtre borné, expansion exacte, puis multiprécision CPU rare. Les tests instrumentent séparément les étages `fp32`, `fp64_filtered`, `expansion` et `cpu_mp`.

Les obligations sont :

- aucun signe accepté par un filtre ne diffère de l'oracle exact ;
- un filtre indécis transmet le cas à l'étage suivant ;
- la désactivation de `fp32` ou de `fp64_filtered` ne change jamais la sortie canonique ;
- le mode `certified` ne tolère aucun résultat `unknown` et ne publie `exact` qu'après décision de tous les signes ;
- le taux de repli est mesuré, mais aucun seuil de taux ne justifie une approximation ;
- les calculs compilés avec et sans contraction FMA, et avec les optimisations rapides désactivées, donnent la même sortie exacte.

Un corpus spécialisé est généré en déplaçant un point de part et d'autre du zéro d'un déterminant par pas de $1$, $2$, $4$ et $8$ ULP, dans les deux directions.

Les centres et rayons de supports trois ou quatre sont testés comme rationnels, pas comme dyadiques supposés. Pour $a/b$ et $c/d$ avec dénominateurs positifs, le verdict est celui du signe exact de $ad-bc$. La suite construit des niveaux égaux avec représentations non réduites différentes, des niveaux stricts arbitrairement proches, des chaînes de tri mêlant plusieurs tailles de support et des cas qui forcent le fallback bigint. Tri, lot égal, hash, sérialisation, checkpoint et reprise doivent rester identiques.

Pour les formes et plans de 2A.8c, le corpus distingue quatre origines fermées : coefficients binary64, plan par trois points binary64, bisecteur de puissance binary64 et plan rationnel exact sans provenance. Il force séparément orientation 2D, côté d'un plan, rang normal et augmenté, intersection unique ou affine, incohérence et incidence d'un quatrième plan. Chaque famille doit atteindre les signes disponibles aux intervalles, à l'expansion et au fallback multiprécision; ce dernier contient aussi des recettes binary64 valides dont l'expansion échoue fermée, et pas seulement des plans exacts dépourvus de provenance. Les rangs, dimensions, étages et compteurs JSON sont vérifiés avec des types entiers stricts afin qu'un booléen ne soit jamais accepté comme `0` ou `1`. Permutations des plans liants, inversion d'orientation, recettes obliques et bisecteurs multi-points complètent les cas d'axes. Le mode adaptatif et `multiprecision_only` doivent publier le même témoin scientifique, tandis que l'étage diagnostique peut différer.

### 5.3 Témoins de sommets

Si l'algorithme manipule des cellules polyédriques, chaque sommet décisif est accompagné d'un témoin combinatoire : contraintes liantes, approximation, classe de dégénérescence et intervalle d'erreur. Les tests reconstruisent le signe de toute contrainte supplémentaire à partir de ce témoin, et vérifient qu'un sommet géométriquement imprécis ne puisse jamais produire un certificat de fermeture par simple tolérance.

### 5.4 Campagne de fermeture GPU de phase 2B

La porte 2B est testée par deux campagnes résumables distinctes. La première rejoue exactement la graine et les racines scientifiques gelées par la phase 2A. La seconde utilise la graine indépendante `4257325f47505532` et doit produire une racine de corpus différente. Chaque campagne de production contient 10 800 000 cas de base — 3 600 000 pour chacun de `compare_squared_distances`, `orientation_3d` et `power_bisector_side` — plus 1 080 000 signes métamorphiques. L'agrégat attendu contient donc 23 760 000 signes lorsque les deux certificats sont complets. Une graine indépendante et deux racines distinctes interdisent la substitution d'un seul corpus aux deux campagnes; elles ne constituent pas une preuve de disjonction cas par cas et le certificat ne doit pas la revendiquer.

Pour chaque entrée, le signe attendu de l'oracle entier-dyadique indépendant doit coïncider avec le replay CPU forcé en multiprécision. Toute transformation métamorphique doit en plus satisfaire sa relation de signe déclarée. Le chemin GPU est ensuite comparé à ces trois contrôles : un signe connu doit être un certificat FP64 strict de même signe; un `unknown` doit être compté, transmis au CPU et résolu sans `remaining_unknown`. Les compteurs par prédicat, signe, relation, étage GPU et fallback doivent fermer exactement sur le nombre de lignes.

Un chunk contient des lots homogènes par prédicat et au plus 196 608 cas de base. Le checkpoint conserve les hashes des exécutables, du générateur, de la configuration et du commit propre, la chaîne contiguë des chunks et quatre racines ordonnées pour corpus, oracle, sortie CPU et sortie GPU. La suite négative couvre au minimum : reprise après un seul chunk, deuxième écrivain, chunk altéré ou orphelin, racine forgée, état complet forgé, compteur incohérent, sortie JSON non canonique, changement du dépôt ou d'un exécutable, relation métamorphique fausse, signe GPU contradictoire et témoin de puissance non représentable. Toute anomalie échoue fermé et laisse le dernier checkpoint committé inchangé.

Le certificat final persiste les répartitions par transformation et par relation métamorphique. Il impose en outre l'égalité entre zéros exacts et signes nuls, la fermeture du tri-state GPU, puis le replay multiprécision de chaque ligne. Un mode de vérification en lecture seule relit une copie quiescente de tous les checkpoints, chunks, certificats et exécutables sans créer ni modifier de fichier; toute altération de l'agrégat racine doit être détectée, y compris en mode mono-campagne. Ce dernier porte `single_campaign_only` et ne peut jamais usurper la justification `independent_seed_and_distinct_corpus_root` réservée aux deux certificats compatibles.

Un certificat incomplet conserve `phase_gate_closed=false` et `qualification_claimed=false`. Même deux certificats complets ne ferment la phase qu'après vérification de leurs références, de leurs racines et de l'agrégat, puis application séparée des autres obligations matérielles et opérationnelles de la porte.

### 5.5 Contexte GPU résident

Le contexte résident est testé sur des séquences grand-petit-grand pour les trois prédicats, puis sur leur alternance dans un même handle. Les décisions, identifiants, compteurs, audits et fallbacks doivent être identiques au chemin éphémère. Deux futures partageant un contexte sérialisent seulement leur section GPU; deux contextes distincts restent indépendants. La destruction ou le déplacement du handle après planification ne doit pas invalider la future, tandis qu'un handle déplacé refuse tout nouveau travail.

Un lot vide ne crée ni stream ni allocation. Une capacité déjà suffisante ne provoque aucune nouvelle allocation et chaque sortie utile est entièrement réécrite afin de détecter un résultat résiduel. Une faute injectée empoisonne seulement le contexte concerné et toutes ses utilisations suivantes échouent fermé. La qualification réelle exécute ces séquences sous `compute-sanitizer` avec contrôle des fuites; le benchmark publie séparément le processus froid et `warm_context_e2e`, qui inclut encore validation, packing et transferts et ne doit pas être présenté comme un débit de noyau pur.

La mesure finale `warm_context_e2e` de Phase 2B utilise un même `PredicateFilterContext`, 65 536 cas déterministes par prédicat, un échauffement non mesuré puis 31 répétitions mesurées. La génération des cas précède l'échauffement; le chronomètre hôte entoure l'appel asynchrone propriétaire jusqu'à `.get()` et inclut donc copie du lot, validation, packing, transferts, noyau, fallback CPU exact et synchronisation. Dans chaque lot, les indices congrus à deux modulo trois sont des zéros exacts et imposent le chemin `unknown` GPU vers CPU; les deux autres classes sont bien conditionnées et doivent être certifiées sur le GPU réel. Le JSON conserve dans l'ordre les 31 durées en nanosecondes, puis `min_ns`, `p50_ns`, `p95_ns`, `max_ns` et la déviation absolue médiane `mad_ns`, tous rejouables par rang le plus proche. L'artefact de production lie cette sortie à un commit propre, au hash du harness, à l'image CUDA, à NVCC et à l'unique G4 visible; il exige par prédicat 2 031 616 entrées, 1 354 421 signes GPU connus, 677 195 replis et zéros exacts, 31 lots de fallback et aucun inconnu restant. Le test hôte réduit à neuf cas et trois répétitions avec lanceur simulé valide le schéma mais ne peut jamais fermer la porte matérielle. Aucun seuil de latence ne conditionne la correction de Phase 2B : une mesure lente reste publiable et ferme cette obligation si ses comptes et sa provenance sont valides.

## 6. Tests de Gamma, du catalogue Gabriel et de la réduction

### 6.1 Gamma exact et événements Gabriel partiels

Le backend `reference_cpu` exact énumère tous les sommets, cofaces et incidences de Gamma. Pour toute sphère critique de rang $k+1$, l'ensemble $S=I\cup U$ est comparé au simplex K-Gabriel attendu. La voie Gabriel doit émettre les $k+1$ facettes $S\setminus\lbrace x\rbrace$ et une hyperarête de niveau $r^2$, sans revendiquer l'exhaustivité de Gamma.

Les tests vérifient :

- la même connectivité pour une clique complète et pour une étoile de $k$ unions ;
- l'indépendance vis-à-vis du choix du pivot de l'étoile ;
- l'absence de perte après tri, déduplication et streaming ;
- la conservation de l'union de points couverte ;
- le cas $q=0$ qui crée une naissance réduite, $q=1$ qui ajoute seulement un `coverage_delta`, et $q\geq2$ qui crée une multifusion ;
- l'activation de toutes les facettes d'un événement, y compris celles qui ne sont pas des bras stricts ;
- l'égalité du profil exact avec Gamma exhaustif pour $n\leq14$ ;
- la présence de chaque `GammaCoface` dans exactement un lot du même ordre et niveau, y compris lorsqu'aucun `CriticalEvent` n'existe à ce niveau ;
- l'inclusion de chaque connexion Gabriel dans une composante Gamma au même niveau ;
- la divergence attendue du contre-exemple exact, avec interdiction de `public_status=exact` pour la voie Gabriel ;
- le rejet des simplexes non Gabriel, même s'ils proviennent d'une liste locale de voisins.

### 6.2 Lots de niveaux égaux

Toutes les naissances, facettes et hyperarêtes d'un même niveau exact sont appliquées comme un lot. Le test exécute artificiellement toutes les permutations d'un petit lot, puis plusieurs ordonnancements GPU aléatoires. Après canonicalisation, la forêt multifusion, les composantes et les flèches verticales doivent être identiques.

Un lot n'est jamais binarisé en fusions artificielles dépendant de l'ordre des threads. Si un format de sortie impose des nœuds binaires, les nœuds auxiliaires doivent être marqués, partager exactement le même niveau et se contracter vers la même multifusion canonique.

### 6.3 Streaming et runs externes

Pour chaque fixture moyenne, la réduction est rejouée avec des budgets de lot qui forcent :

- un seul lot ;
- 2, 3, 7 et 31 lots ;
- des runs externes très petits ;
- un changement de frontière de lot au milieu d'un niveau égal ;
- une reprise après chaque run écrit.

Le résultat canonique doit être invariant. Un niveau égal ne peut être finalisé tant que tous les runs susceptibles de contenir ce niveau n'ont pas été fusionnés.

Pour $m<m_{\star}$, les fragments provenant de parents ou chunks différents ne sont jamais unis géométriquement dans la v2. Les tests font découvrir le même label $Q$ depuis une, deux, trois et plusieurs sources, puis reconstruisent $C_{m+1}(Q)$ depuis la boîte paddée $\Omega$ avec des amorces de contraintes différentes, y compris l'amorce vide. À chaque sommet provisoire, les co-maximiseurs de $Q$ sont comparés exactement aux co-1-NN de $X\setminus Q$; toutes les égalités actives sont réconciliées. Cellule, strates et certificat final doivent être identiques. À $m=m_{\star}$, les morceaux et leurs strates sont fermés, mais le test exige qu'aucun $C_{m_{\star}+1}$ ne soit construit ou propagé. Un prototype futur de complexe de fragments reste non certifiant tant qu'il ne passe pas ce différentiel et ne possède pas sa propre preuve.

La boîte $\Omega$ est testée sur des centres critiques situés sur les six faces, les douze arêtes et les huit sommets de l'AABB non paddée. Le padding dyadique doit les placer strictement à l'intérieur; seules les faces du padding portent le bit artificiel et elles ne produisent aucun événement.

### 6.4 Multiplicité de Morse et nombre de bras

Pour un événement de support frontal $U$ et d'indice $\mu$, l'oracle enregistre la multiplicité de Reani–Bobrowski $\Delta=\binom{\lvert U\rvert-1}{\mu}$. À l'indice un, il construit les $\lvert U\rvert$ bras et vérifie que le lot tue au plus $\lvert U\rvert-1$ classes de $H_0$. Les fixtures obligatoires couvrent :

- $\lvert U\rvert=2$ avec fusion binaire;
- $\lvert U\rvert=3$ avec trois bras et fusion triple;
- $\lvert U\rvert=4$ avec quatre bras et fusion quadruple;
- deux ou plusieurs bras déjà dans la même composante avant le lot;
- tous les bras déjà connectés, donc aucune fusion $H_0$ même si l'événement peut créer une classe de $H_1$ non représentée.

La sortie est une multifusion canonique. Elle ne doit jamais être binarisée par l'ordre des threads; tout nœud binaire auxiliaire imposé par un format est marqué et se contracte vers le lot unique.

### 6.5 Tour globale de boules saturées

Cette suite teste la voie candidate de la phase 17 sans l'activer comme base de preuve publique. Pour chaque nuage exact de $n\leq14$, l'oracle structurel interne couvre $1\leq k\leq n$, y compris le cas terminal; les assertions du contrat v2 restent séparément bornées à $1\leq k\leq\min(10,n)$.

Le système sous test utilise le noyau exact C++ issu de la phase 2A. La référence Gamma reste l'oracle Python `Fraction`, soumis au contrôle d'indépendance existant. Les deux chemins ne doivent pas appeler la même implémentation de miniball, de classification de sphère ou de Kruskal; un différentiel seulement structurel qui partagerait sa géométrie ne fermerait pas 17A.

L'oracle :

1. énumère tous les supports affinement indépendants de tailles un à quatre;
2. calcule leur miniball exacte et exige le centre dans $\mathrm{relint}\,\mathrm{conv}(U)$;
3. classifie chaque observation comme intérieur, shell ou extérieur de la boule fermée;
4. construit $S=\mathrm{Sat}(U)$ et vérifie $\beta(S)=\beta(U)$ ainsi que l'idempotence;
5. déduplique les boules et saturés tout en conservant tous les supports témoins;
6. matérialise $\Delta(S)$ seulement lorsque le budget combinatoire le permet;
7. compare l'union descendante au complexe de Čech exhaustif;
8. matérialise l'union des graphes de Johnson $J_k(S)$ et compare exactement ses $k$-faces et arêtes à Gamma;
9. développe chaque composante de $H_k(a)$ en sa famille de $k$-faces et compare cette partition ainsi que sa couverture aux composantes de Gamma;
10. compare de la même façon une forêt de Kruskal de poids maximum seuillée à $H_k(a)$ et à Gamma;
11. répète les étapes 7–10 pour les coupes ouvertes $t<a$ et fermées $t\leq a$.

Le graphe $H_k(a)$ n'est pas isomorphe à Gamma : ses sommets sont des générateurs, tandis que ceux de Gamma sont des $k$-faces. La comparaison exacte des sommets et arêtes concerne donc Gamma et l'union matérialisée des graphes de Johnson. Pour $H_k(a)$ et sa forêt seuillée, elle concerne les composantes après expansion en $k$-faces, puis les couvertures en observations, pas seulement leur nombre. Les générateurs dont $\lvert S\rvert<k$ sont absents de la coupe d'ordre $k$. Pour $k\geq2$, une composante omise par `hgp_reduced` est identifiée sémantiquement comme une seule $k$-face isolée; le nombre de supports témoins ne doit jamais la rendre artificiellement non triviale.

La forêt statique de chaque coupe est comparée à la mise à jour insertionnelle lot par lot. Les tests mélangent l'ordre des supports, des arêtes, des poids égaux et des threads simulés, puis exigent :

- le même ordre total canonique de Kruskal;
- les mêmes composantes à chaque seuil d'ordre;
- le même état strict pré-lot et fermé post-lot;
- les mêmes naissances, prolongements, multifusions et `coverage_delta`;
- les mêmes identifiants canoniques après rejeu;
- les mêmes applications verticales et carrés de naturalité;
- une reprise après checkpoint identique à une exécution continue.

Le checkpoint contient les checksums de l'entrée et de la configuration, le catalogue et la déduplication actifs, les postings, la forêt courante, le dernier lot entièrement committé, le curseur du flux restant, l'identifiant de l'ordre total canonique et le journal de rejeu. Le test régénère le suffixe depuis les checksums et le curseur, puis exige son identité. Un état pris au milieu d'un lot est temporaire : il est annulé ou rejoué intégralement et ne devient jamais un snapshot publiable.

Les remplacements d'arêtes de la forêt de générateurs ne sont jamais comparés directement aux événements HGP. Seules les différences de composantes pré-lot/post-lot alimentent le `MergeForest` attendu.

Fixtures obligatoires :

- `gabriel-point-set-counterexample-5-points-v1`, avec le générateur `ACDE` au niveau $33/2$, puis l'intersection de capacité deux avec `ABC` au niveau $83886/3563$;
- `morse-rank-window-regression-v1`, qui impose $\lvert I\rvert=2$, $\lvert U\rvert=2$, les seuls ordres critiques trois et quatre, $D_2(c)<a$ malgré l'action combinatoire descendante et $D_5(c)>a$ au-dessus du rang fermé;
- une famille obtenue en ajoutant arbitrairement des points strictement dans la miniball d'une petite face, afin de falsifier toute troncature $\lvert S\rvert\leq K_{\mathrm{eff}}+1$;
- shells cosphériques de plus de quatre points et plusieurs supports minimaux de la même boule;
- niveaux égaux provenant de boules distinctes et niveaux rationnels presque égaux;
- nuages colinéaires, coplanaires et tridimensionnels, doublons refusés selon le contrat courant;
- saturés emboîtés, disjoints, fortement recouvrants et antichaînes d'inclusion;
- le contre-exemple non monotone de l'audit, où $Q\subseteq R$ mais $\mathrm{Sat}(Q)\nsubseteq\mathrm{Sat}(R)$;
- triangle abstrait de capacités $2,2,1$, qui rejette une forêt seulement maximale ne contenant pas les deux arêtes de capacité deux.

Le pruning par inclusion est désactivé dans la baseline. Sa variante expérimentale doit produire les mêmes coupes, couvertures, journaux et morphismes, avec une table explicite de domination et de provenance. Elle est testée contre des suppressions d'un sommet feuille et d'un sommet interne de la forêt; l'algorithme ne peut libérer les anciennes arêtes avant rewiring certifié.

Pour chaque sous-famille de générateurs, la suite vérifie l'inclusion dans Gamma et l'absence de fausse connexion. Elle construit aussi des sous-familles qui retardent volontairement une naissance et une fusion, afin d'interdire la promotion de leurs nœuds partiels en événements HGP exacts. ANN, Delaunay ou le raffinement courant ne sont que des sources de propositions; chaque support retenu est resaturé exactement et l'artefact conserve la sémantique scientifique interne `partial_refinement` tant que l'univers complet n'est pas certifié. Aucun `MorseHGP3DResult` v2 n'est émis pour cette voie avant ajout contractuel d'une base de preuve dédiée.

La campagne de performance CPU bornée couvre $n\in\left\lbrace16,24,32,48,64,96,128\right\rbrace$ avec arrêt budgétaire. Elle est hors CI et explicitement opt-in. Chaque manifeste fixe avant exécution `time_budget_s`, `host_budget_bytes`, `scratch_budget_bytes` et `output_budget_bytes`; une observation censurée par budget est une sortie valide à publier comme telle, pas un échec à masquer. La campagne compare graphe statique complet, postings dynamiques, forêt recalculée, mise à jour insertionnelle et pruning expérimental sur les familles volumique, surfacique, en amas et adversariale. Une moyenne favorable ne change aucun statut scientifique.

## 7. Invariants de hiérarchie

### 7.1 Invariants horizontaux

Pour chaque ordre $k$ :

1. les niveaux le long de toute arête enfant–parent sont non décroissants ;
2. la forêt est acyclique et tout identifiant actif a un représentant ;
3. à chaque seuil d'oracle, les composantes reconstruites depuis la forêt coïncident avec celles du graphe exhaustif ;
4. une fusion en lot relie exactement les composantes incidentes juste sous le niveau ;
5. une croissance $q=1$ conserve la racine et ajoute exactement son `coverage_delta` ;
6. aucune composante ne se scinde lorsque $t$ augmente ;
7. le nombre de composantes vérifie la comptabilité naissances–fusions du lot ;
8. l'union de points stockée pour un nœud est celle de ses facettes ou enfants ;
9. les recouvrements entre composantes de la couverture d'ordre $k\geq2$ sont préservés, non arbitrairement résolus en partition.

### 7.2 Invariants verticaux

La monotonie $L_{k+1}(t)\subseteq L_k(t)$ impose qu'une composante d'ordre $k+1$ soit incluse dans une unique composante d'ordre $k$ au même niveau. Pour chaque flèche verticale, les tests vérifient :

- existence et unicité de la cible ;
- inclusion de l'union de points couverte lorsque cet invariant est pertinent au périmètre réduit ;
- cohérence avec l'ancre de rang de chaque sphère critique ;
- absence de cible créée à un niveau strictement supérieur ;
- commutation avec la montée en échelle.

Si $h^k_{t,t'}$ désigne l'application horizontale et $v^k_t$ l'application verticale de l'ordre $k+1$ vers l'ordre $k$, l'identité testée est

$$v^k_{t'}\circ h^{k+1}_{t,t'}=h^k_{t,t'}\circ v^k_t.$$

Elle est vérifiée à tous les couples de seuils consécutifs de l'oracle, ainsi qu'autour de chaque niveau partagé par plusieurs ordres.

### 7.3 Cohérence des préfixes en ordre

Une exécution avec $K_{\max}=10$ doit donner, après restriction, exactement le même résultat que des exécutions indépendantes avec $K_{\max}=1,2,\ldots,9$. Ce test détecte les caches inter-ordres mal invalidés et les heuristiques dont le budget dépend involontairement de l'ordre maximal.

### 7.4 Miniball local et préconditions de descente

Pour toute facette de cardinal $1\leq k\leq10$, l'oracle `reference_cpu` énumère exactement les sous-supports de tailles un à quatre. Les compteurs doivent égaler $\sum_{j=1}^{4}\binom{k}{j}$, donc 385 à $k=10$, et chaque support relativement intérieur doit classifier les $k$ points sans arrêt anticipé. Le rejeu compare la facette canonique, le support choisi, le centre, le rayon carré, la partition intérieur--shell, les comptes par décision, le nombre de supports optimaux, le statut et le scope.

Les cas ciblés obligatoires sont le singleton, le triangle obtus, le triangle rectangle avec shell supplémentaire, le triangle aigu, un tétraèdre à support quatre, un tétraèdre réduit à une face, le carré à deux supports optimaux, six points cosphériques où un triple extérieur englobant doit être rejeté au profit d'un support positif de cardinal quatre, et dix points colinéaires. Les falsifications portent séparément sur chaque champ. Le support canonique ne rend pas la descente admissible : le test futur doit exiger séparément `boundary_point_ids==support_point_ids`, l'absence de point de $X\setminus F$ sur le shell, le top-$k$ exact et la décroissance stricte. L'oracle indépendant `reference/morsehgp3d_oracle/geometry.py::minimum_enclosing_ball` partage désormais la sémantique des supports positifs; son test à six points est obligatoire, mais le rejeu C++ frais ne remplace pas le futur raccord différentiel.

## 8. Ancrage fondamental à $k=1$

À l'ordre $1$, la hiérarchie doit coïncider avec le single linkage porté par un arbre couvrant euclidien minimal. Une arête EMST $(u,v)$ de longueur carrée $\lVert u-v\rVert^2$ produit dans $L_1$ le niveau de fusion $\lVert u-v\rVert^2/4$ ; les tests vérifient explicitement ce facteur au lieu de comparer deux conventions implicites.

Pour $n\leq14$, l'oracle utilise Kruskal sur le graphe euclidien complet. Pour les tailles intermédiaires, il utilise une implémentation CPU exacte indépendante ; une implémentation GPU d'EMST publiée peut servir de second différentiel, jamais d'unique oracle.

En l'absence d'ex æquo, on compare les arêtes de l'EMST et leurs poids. En présence d'ex æquo, plusieurs arbres sont valides : on compare plutôt la partition induite à tout seuil distinct, le poids total exact et la multifusion canonique. Les tests couvrent :

- $n=1$, forêt réduite triviale et aucun raffinement ;
- $K_{\max}=1$ avec $n\geq2$, profondeur $m_{\star}=0$, minima de rang un et seules selles de rang deux ;
- points aléatoires volumiques ;
- points collinéaires et coplanaires ;
- grilles et shells avec nombreux poids égaux ;
- deux amas reliés par une arête unique ;
- changements d'échelle et grands offsets ;
- exécution mono-lot et streaming.

Un désaccord à $k=1$ bloque immédiatement tout benchmark de production.

Le profil de travail séparé de la voie external-1NN exacte mesure par ronde les évaluations exactes de graines, les bornes point--AABB, les distances point--point, les visites de nœuds et les pics par source. Son checker exige l'identité entre bornes AABB et visites, la partition exacte des visites entre élagages stricts, expansions, distances et réutilisation de graine, les sommes inter-rondes, la réduction au moins par moitié des composantes et zéro record candidat persistant. Une matrice G4 ne change aucun statut de phase : son artefact reste `benchmark_only`, sans revendication de qualification, de scalabilité, de résultat scientifique ou de réduction hiérarchique.

La réduction explicite external-1NN vers `K1CompactForest` possède un test hôte distinct. Il exige un rejeu frais du témoin avant construction, l'immuabilité et le statut `not_performed/local_emst_witness_only` de cette source, puis l'égalité de chacune des cinq arènes, de la racine, des poids et des compteurs avec les réductions fraîches du témoin et de l'ancre Borůvka. Le singleton, une chaîne multi-ronde et un carré à multifusion d'arité quatre sont obligatoires. Une mutation isolée de chaque arène, des statuts, du scope, des poids, des compteurs ou de la politique fiable doit faire tomber `local_k1_hierarchy_certified`. Seul le résultat séparé peut publier `compact_k1_forest_certified/local_k1_compact_forest_only`; ce certificat local ne vaut ni `public_status=exact`, ni tour multi-ordre.

## 9. Générateurs de données

Chaque générateur possède une version, une graine, des paramètres sérialisés et un hash du nuage produit. Les coordonnées brutes des grands jeux ne sont pas commitées si elles sont régénérables.

### 9.1 Familles volumiques et stochastiques

| famille | paramètres à balayer | difficulté visée |
|---|---|---|
| uniforme | cube, boule, pavé d'aspect $1$, $10$, $100$ | régime volumique favorable, effet des bords |
| Poisson inhomogène | gradients linéaire, radial et par morceaux ; rapport d'intensité jusqu'à $100$ | rangs locaux très variables |
| mélanges gaussiens | nombre d'amas, séparation, covariance, poids | naissances et fusions contrôlables |
| mélanges non gaussiens | Student, Laplace, uniforme ellipsoïdal | queues lourdes et frontières nettes |
| bruit et outliers | $0\%$, $1\%$, $5\%$, $20\%$ | petites branches et événements parasites |
| processus clusterisés | Thomas ou Neyman–Scott | nombreuses échelles locales |

Pour les mélanges, le nombre d'amas parcourt **chaque entier de 1 à 64** au moins dans la suite de taille moyenne. La suite de passage à l'échelle retient $1,2,4,8,16,32,64$. Les autres axes comprennent :

- séparation normalisée $\Delta/\sigma\in\{1,1.5,2,3,5,8\}$ ;
- conditionnement des covariances dans $\{1,10,100,1000\}$ ;
- rapports de masse maximal/minimal $1$, $2$, $10$ et $100$ ;
- amas emboîtés, satellites et tailles suivant une loi de puissance ;
- orientation aléatoire indépendante de chaque covariance.

Les étiquettes de mélange servent seulement aux métriques secondaires. Elles ne définissent pas l'oracle HGP.

### 9.2 Familles géométriques de dimension intrinsèque faible

Les jeux suivants sont échantillonnés avec et sans bruit normal :

- segment, cercle, hélice et courbe en Y ;
- plan, deux plans sécants, ruban et grille surfacique ;
- sphère, ellipsoïde, tore et coquilles concentriques ;
- tube, cylindre, cône, embranchement vasculaire et filament ;
- deux boules reliées par un pont de densité variable ;
- vallées imbriquées et amas séparés par une membrane clairsemée ;
- scènes LiDAR synthétiques avec sol, façades, objets minces et occlusions.

Ces familles sont indispensables : une méthode quasi linéaire sur un Poisson volumique peut devenir très dense sur une surface ou un filament.

### 9.3 Familles adversariales

| famille | paramètre critique | comportement attendu |
|---|---|---|
| courbe des moments | $n$ croissant | croissance proche du pire cas Delaunay |
| grande sphère cosphérique | taille du shell | plateau déclaré, jamais résolu par jitter silencieux |
| grille exacte | dimension et pas | niveaux massivement égaux |
| duplications | multiplicité $2$ à $1000$ | agrégation explicite et rang pondéré |
| quasi-coplanarité | hauteur de $2^{-p}$ | sollicitation des filtres exacts |
| tétraèdres quasi plats | barycentrique proche de zéro | décision de relative-intériorité |
| offset gigantesque | translation jusqu'à $10^{12}$ avec écarts jusqu'à $10^{-6}$ | annulation catastrophique |
| échelles extrêmes | facteur de $10^{-12}$ à $10^{12}$ | équivariance et normalisation |
| ponts longs | densité et longueur | nombreuses fusions proches |
| contre-exemple aux listes L-NN | nombre de distracteurs | vérification qu'aucune liste locale fixe ne certifie la complétude |
| sorties denses | $n$ et géométrie | garde de budget honnête, absence d'OOM non contrôlé |

Les cas dont la combinatoire dépasse le budget peuvent finir `conditional` ou `budget_exhausted`; ils ne peuvent jamais finir `exact` avec un catalogue tronqué.

### 9.4 Données réelles

Une suite non bloquante puis une suite de release utilisent des scènes LiDAR et des nuages 3D publics à licence compatible. Chaque jeu possède une fiche de provenance, une somme SHA-256, un script de préparation et une version figée. Les données réelles servent à mesurer le profil de charge, pas à établir l'exactitude mathématique.

## 10. Matrice des tailles

| classe | valeurs de $n$ | rôle |
|---|---|---|
| micro | $4$ à $14$ | oracle exhaustif et réduction de contre-exemples |
| petite | $10^3$, $3\times10^3$ | tests GPU rapides et profils de lancement |
| moyenne | $10^4$, $3\times10^4$, $5\times10^4$, $10^5$ | objectif interactif et matrice complète de difficultés |
| grande | $10^6$, $3\times10^6$ | streaming, mémoire et reprise |
| extrême | $10^7$ | démonstration hors cœur géométrique, uniquement sur familles sélectionnées |

Tous les ordres $1$ à $10$ sont demandés en une seule exécution dès que $n\geq10$. Les cas $n<10$ s'arrêtent naturellement à $k=n$.

La matrice exhaustive des paramètres n'est pas appliquée à $10^7$ points. Elle est structurée en trois anneaux :

1. **couverture** : tous les paramètres, jusqu'à $10^4$ points ;
2. **interaction** : plans fractionnaires de paramètres, jusqu'à $10^5$ points ;
3. **échelle** : familles représentatives et adversariales choisies, jusqu'à $10^7$ points.

## 11. Tests différentiels et métamorphiques

### 11.1 Différentiels

La même entrée est traitée par :

- l'oracle exhaustif et le backend CPU de référence pour les micro-tailles ;
- le backend CPU de référence et le backend GPU pour les tailles compatibles ;
- le noyau de cellules de référence et toute primitive externe accélérée ;
- le pipeline avec filtres rapides actifs puis désactivés ;
- l'hyperarête en étoile puis sa clique explicite ;
- le calcul en mémoire puis le calcul en runs externes.

Les sorties sont canonicalisées avant comparaison. Pour un mode `budgeted`, chaque événement produit doit appartenir à la sortie exacte de référence ; le rappel est mesuré mais n'est pas une condition d'exactitude.

### 11.2 Transformations métamorphiques exactes

| transformation | invariant attendu |
|---|---|
| permutation des points | même sortie après réindexation inverse |
| permutation signée des axes | mêmes bits de niveaux et même combinatoire |
| translation dyadique exactement représentable | même combinatoire et mêmes niveaux |
| homothétie de facteur $2^q$ exactement représentable | même combinatoire, niveaux multipliés par $2^{2q}$ |
| permutation des lots et des blocs CUDA | même sortie canonique |
| variation du budget de streaming | même sortie si les deux exécutions ont le statut `exact` |
| restriction de $K_{\max}$ | préfixe identique de la tour |
| changement de pivot d'une hyperarête | même composante et même multifusion |
| changement des étages de filtre | même décision exacte |

Une transformation n'entre dans cette table que si les coordonnées IEEE transformées sont exactement celles de la transformation dyadique abstraite, vérification faite avant l'appel au backend. Les générateurs réservent des marges d'exposant et rejettent tout cas avec arrondi, overflow, underflow ou passage sous-normal involontaire. Une rotation orthogonale générale, même rationnelle, appartient aux tests différentiels : on compare le backend à l'oracle sur la **nouvelle entrée arrondie**, mais on ne lui attribue pas abusivement l'invariance exacte de l'entrée initiale.

### 11.3 Propriétés d'inclusion

Les tests génératifs vérifient aussi :

$$L_{k+1}(t)\subseteq L_k(t),\qquad L_k(t)\subseteq L_k(t')\quad\text{si }t\leq t'.$$

Après ajout de points $Y$, on a à ordre fixé

$$L_k^X(t)\subseteq L_k^{X\cup Y}(t).$$

Sur l'oracle discret, l'ajout de points peut fusionner des composantes mais ne doit pas invalider cette inclusion géométrique. Ce test est appliqué par insertion progressive de points et comparaison des applications induites, sans supposer que les arbres eux-mêmes restent identiques.

### 11.4 Déterminisme

Pour une entrée, une configuration et un statut identiques, les enregistrements scientifiques canonicalisés doivent être bit à bit identiques. Les temps, compteurs d'ordonnancement et représentants internes DSU peuvent différer. Les tests répètent chaque petite entrée avec :

- 20 ordonnancements de blocs ;
- plusieurs tailles de bloc et de lot ;
- allocation mémoire fraîche ou réutilisée ;
- concurrence maximale puis exécution sérialisée de diagnostic.

## 12. Dégénérescences et politique d'échec

### 12.1 Cas obligatoires

La suite couvre : points dupliqués, supports collinéaires ou coplanaires, shells de plus de quatre points, sphères cosphériques, barycentriques nuls, rayons nuls, niveaux de fusion égaux et coordonnées non finies.

Pour chaque famille, le comportement attendu est déclaré dans la fixture parmi :

- `exact` après agrégation exacte des multiplicités ;
- `exact` avec hyperévénement de plateau explicitement pris en charge ;
- `unsupported_degeneracy` avec diagnostic et aucun résultat exact revendiqué ;
- `conditional` ou `budget_exhausted` si tous les événements émis restent certifiés mais la fermeture est abandonnée.

Le jitter aléatoire ou une tolérance utilisée comme égalité sont interdits en mode certifié. Une perturbation symbolique n'est acceptée que si la sortie précise qu'elle concerne le problème perturbé et si une procédure de contraction vers le plateau original est démontrée et testée.

Avec `mode=budgeted, forest_semantics=partial_refinement`, chaque fixture vérifie en plus le `PartialScope` : aucun ordre ou profondeur ouverte ne figure parmi les périmètres fermés, chaque cellule déclarée close possède son certificat canonique, tous les loci dégénérés sont sérialisés et le résultat porte `public_status=conditional` ou `public_status=budget_exhausted`. Avec `public_status=unsupported_degeneracy`, `forest_semantics` est absent et seuls les événements vérifiés sont exposés. La même entrée avec `require_exact=true` doit refuser une forêt partielle avant calcul.

### 12.2 Tests de frontière du domaine certifié

Pour chaque hypothèse de généricité, une paire de fixtures est fournie : une entrée qui la satisfait avec une marge décroissante et sa limite dégénérée. Le backend doit passer de `exact` au traitement de plateau ou à `unsupported_degeneracy` exactement au zéro certifié, et non à une tolérance dépendant de l'échelle.

## 13. Métriques secondaires de clustering

Les générateurs à amas connus permettent de mesurer, à titre exploratoire :

- stabilité des branches en échelle et en ordre ;
- rappel des vallées et des ponts implantés ;
- pureté, ARI ou information mutuelle après une règle de coupe déclarée ;
- sensibilité aux déséquilibres de masse et de densité ;
- distorsion des niveaux de fusion par rapport à l'oracle sur les tailles accessibles.

Ces métriques évaluent des choix heuristiques de condensation ou de coupe. Elles ne participent jamais au verdict d'exactitude de MorseHGP3D.

## 14. Protocoles de performance sur G4

### 14.1 Configuration figée

La configuration de référence est la VM G4 définie par les scripts actifs du dossier `gcp-migration`, en priorité `g4-standard-48` avec un seul GPU. Chaque rapport enregistre :

- projet, zone, type de machine et identifiant du GPU ;
- image, noyau Linux, pilote NVIDIA, version CUDA et hash du conteneur ;
- fréquence, limite de puissance, température initiale et éventuel throttling ;
- nombre de vCPU, mémoire hôte, mémoire GPU et espace disque local disponible ;
- hash Git, options de compilation, bibliothèques et variables d'environnement ;
- graine, hash du jeu, $K_{\max}$, $K_{\mathrm{eff}}$, mode, profil et budgets typés ;
- statut de préemption et identifiant de tentative.

Les benchmarks n'utilisent ni horloge verrouillée ni privilège spécial non documenté. Toute différence de configuration sépare les séries.

### 14.2 Trois chronométrages distincts

| protocole | début | fin | usage |
|---|---|---|---|
| `cold_e2e` | nouveau processus, données sur disque local | sortie et certificat écrits, GPU synchronisé | coût réel d'une première requête |
| `warm_e2e` | processus et allocateur initialisés, nouveau transfert du nuage | sortie matérialisée, GPU synchronisé | service réutilisant le runtime |
| `resident_core` | points et index spatial déjà résidents sur le GPU | catalogue et hiérarchie disponibles sur le GPU | coût algorithmique répété |

Le provisionnement de la VM, le téléchargement du jeu et la compilation ne sont inclus dans aucun de ces temps ; ils sont rapportés séparément. Le temps `warm_e2e` inclut validation, transfert hôte–GPU, construction de l'index, calcul et transfert du résultat. Le temps `resident_core` doit préciser exactement quels index sont réutilisés.

Les temporisations GPU utilisent des événements CUDA et une synchronisation explicite ; le total hôte utilise une horloge monotone. Les exécutions d'échauffement sont exclues mais comptées.

### 14.3 Répétitions et statistiques

- $n\leq10^4$ : 10 démarrages froids et 50 répétitions chaudes ;
- $10^4<n\leq10^5$ : 5 démarrages froids et 20 répétitions chaudes; la qualification produit spéciale à $n=50\,000$ en exige 30 conformément à la phase 14 ;
- $10^5<n\leq10^6$ : 3 démarrages froids et 7 répétitions chaudes ;
- $n>10^6$ : au moins 3 répétitions complètes, ou justification du coût avec trois tentatives indépendantes.

Le rapport publie $p50$, $p95$, maximum, écart absolu médian et chaque valeur brute. Une moyenne seule est interdite. Une exécution préemptée ne compte pas comme mesure de temps, mais reste dans le rapport de fiabilité.

### 14.4 Objectifs réfutables

Les seuils suivants concernent `warm_e2e`, $K_{\max}=10$, un seul GPU G4, sur une famille volumique favorable dont le certificat reste sparse. Ils incluent la matérialisation de la hiérarchie demandée.

`BenchmarkOutputContract-v1` impose le profil `hgp_reduced`, les dix forêts, les applications verticales, les lots et le certificat minimal copiés en mémoire hôte épinglée avant la fin du chronomètre. Le catalogue de replay complet, l'expansion des unions de points et l'écriture disque sont chronométrés séparément. Un benchmark qui change ce payload appartient à une autre série.

| $n$ | objectif $p95$ | condition |
|---:|---:|---|
| $1\,000$ | $\leq25$ ms | exécution certifiée |
| $10\,000$ | $\leq200$ ms | exécution certifiée |
| $50\,000$ | $\leq1$ s | exécution certifiée ; objectif interactif principal |
| $100\,000$ | $\leq3$ s | exécution certifiée |
| $1\,000\,000$ | $\leq60$ s | seulement si le catalogue et les runs restent sparse |
| $10\,000\,000$ | $\leq600$ s | seulement si le catalogue reste sparse ; streaming autorisé |

Ces seuils ne valent pas pour toutes les géométries. Le pire cas exact peut produire une sortie quadratique dès les structures de Delaunay 3D. En cas de dépassement, le test échoue par rapport à l'objectif de produit, mais ne conclut pas à une erreur mathématique. Le rapport conserve la mesure au lieu de masquer l'échec ou de modifier le seuil.

Pour $3\times10^6$ points, la campagne mesure sans imposer initialement un SLA distinct ; un objectif ne sera fixé qu'après deux releases reproductibles.

### 14.5 Régimes de benchmark

Chaque taille est testée au minimum sur :

1. un Poisson uniforme volumique ;
2. un mélange équilibré de 8 amas ;
3. un mélange de 32 amas avec rapport de masse $100$ ;
4. une surface bruitée ;
5. un pont de faible densité ;
6. une famille adversariale choisie pour densifier le catalogue.

Les objectifs du tableau précédent ne sont évalués que sur 1 et 2. Les autres familles mesurent la dégradation et le passage honnête vers `conditional`, `unsupported_degeneracy` ou `budget_exhausted`.

### 14.6 Budgets et stockage G4

Chaque test de streaming fixe `device_budget_bytes`, `host_budget_bytes`, `scratch_budget_bytes`, `output_budget_bytes` et `time_budget_s`. Les valeurs absolues, consommées et restantes sont comparées à un oracle de comptabilité à chaque frontière transactionnelle. Les cas limites placent chaque budget un octet sous, exactement sur et un octet au-dessus du besoin prévu.

Sur G4, les manifestes n'acceptent que Hyperdisk pour le stockage persistant et Titanium SSD pour le scratch local rapide; `g4-standard-48` peut en utiliser jusqu'à quatre. Une reprise qui doit survivre à la VM n'est validée qu'après copie et checksum du checkpoint sur Hyperdisk.

Pour chaque run et merge, la suite force une panne après création du temporaire, après écriture, après synchronisation, après renommage et avant publication du manifeste. L'ancien état reste lisible jusqu'à validation atomique du nouveau. Si l'espace ne couvre pas simultanément ancien état, temporaire, merge, checkpoint et marge, aucune nouvelle écriture ne commence et le résultat vaut `budget_exhausted` depuis le dernier checkpoint durable.

## 15. Compteurs obligatoires

Chaque exécution émet un enregistrement structuré, versionné, comprenant au minimum :

### 15.1 Entrée et sortie

- $n$, boîte englobante, échelle de normalisation, duplications et multiplicités ;
- $K_{\max}$, $K_{\mathrm{eff}}$, $s_{\max}$, $m_{\star}$, `profile`, mode demandé et statut obtenu ;
- nombre de composantes, branches, flèches verticales et points couverts par ordre ;
- nombre de niveaux distincts et taille maximale d'un lot égal ;
- raison exacte d'un statut `conditional`, `budget_exhausted`, `unsupported_degeneracy` ou `numeric_failure`.

### 15.2 Catalogue et géométrie

- candidats proposés, dédupliqués, acceptés et rejetés ;
- répartition par taille de support, rang fermé, ordre de détection et ordre d'usage ;
- labels parents, cellules ouvertes/fermées, contraintes de découpage et sommets ;
- violations de fermeture trouvées, rondes de fermeture et taille maximale des files ;
- visites LBVH, élagages, requêtes top-$k$, requêtes de rang et exclusions ;
- hyperarêtes, facettes, émissions redondantes, unions DSU utiles et inutiles ;
- runs externes, octets écrits/lus et volume de fusion;
- cellules canoniques reconstruites, amorces vides ou non, violateurs et co-ties ajoutés par ronde;
- pour la phase 17 : supports bruts par taille, supports bien centrés, boules et saturés distincts $M_{\mathrm{sat}}$;
- memberships $L_{\mathrm{sat}}=\sum_S\lvert S\rvert$, distribution des capacités et `peak_active_inclusion_maxima`, avec coût du join d'inclusion compté séparément;
- longueurs des postings $d_x$, $P_{\mathrm{post}}=\sum_x\binom{d_x}{2}$, paires uniques et pic de l'accumulateur;
- arêtes d'intersection examinées et retenues, remplacements de forêt, octets de l'historique et temps des requêtes de coupe;
- certificats de complétude des supports, ranges fermés, déduplication, lots, join, forêt, rejeu et applications verticales.

### 15.3 Numérique

- appels et décisions à chaque étage de précision ;
- replis multiprécision CPU, zéros exacts et cas indécis ;
- plateaux, shells de plus de quatre points et supports non essentiels ;
- marges minimale et quantiles des prédicats filtrés, sans les utiliser comme tolérance.

### 15.4 Temps et ressources

- temps `cold_e2e`, `warm_e2e`, `resident_core` et temps de chaque phase ;
- temps GPU par famille de noyaux, nombre de lancements et octets traités ;
- pics de mémoire GPU et hôte, pic des arènes, fragmentation et nombre d'allocations ;
- budgets device, hôte, scratch, sortie et temps : limite, pic, réserve et raison du premier refus ;
- octets Hyperdisk et Titanium SSD, espace transactionnel réservé et checkpoints rendus durables ;
- débit des tris, réductions, requêtes LBVH et écritures de runs ;
- énergie ou puissance moyenne si la télémétrie est disponible sans perturber la mesure ;
- événements de préemption, reprise et recomputation.

Un compteur absent rend le benchmark diagnostiquement incomplet, même si le temps total est disponible.

## 16. Fixtures et artefacts reproductibles

### 16.1 Arborescence cible

```text
tests/
  fixtures/
    exact/
    hierarchy/
    degeneracies/
    metamorphic/
    regressions/
  generators/
  oracle/
  property/
  differential/
  performance/
benchmarks/
  manifests/
  baselines/
```

Les gros nuages sont générés ou téléchargés sur la VM et ne sont pas versionnés comme blobs sans nécessité.

### 16.2 Manifestes

Chaque fixture ou benchmark décrit :

- identifiant stable et objectif du cas ;
- générateur, version, graine et paramètres ;
- hash SHA-256 des coordonnées canoniques ;
- convention d'unités et politique de duplications ;
- $K_{\max}$ et `profile` ;
- statut attendu et hypothèses de généricité ;
- sortie canonique attendue ou référence vers l'oracle ;
- provenance et licence pour les données externes.

Les sorties dorées stockent des enregistrements triés, pas seulement un hash : catalogues critiques, niveaux exacts sérialisés, hyperarêtes, multifusions, unions de points et flèches verticales. Le hash accélère la comparaison mais ne remplace pas un diff lisible.

### 16.3 Régressions

Tout défaut corrigé ajoute :

1. l'entrée minimale qui le reproduit ;
2. la sortie erronée et la sortie correcte ;
3. le prédicat ou l'invariant violé ;
4. la graine et le contexte de compilation ;
5. un test qui échoue sur le commit fautif.

## 17. Campagnes d'exécution

### 17.1 À chaque changement

- validation T0 ;
- tous les prédicats T1 ;
- fixtures exactes et dégénérées ;
- oracle complet jusqu'à $n=12$ ;
- propriétés métamorphiques courtes ;
- comparaison $k=1$ avec l'EMST exhaustif.

### 17.2 Campagne longue G4 manuelle

Cette campagne n'est jamais déclenchée par un workflow GitHub planifié. Chaque shard exige une autorisation utilisateur explicite, un manifeste de durée prévisionnelle et les deux coupe-circuits vérifiés. Sa durée GCE est strictement inférieure à huit heures; le runner cesse de prendre du travail et checkpoint au moins trente minutes avant l'échéance, puis arrête et vérifie la VM. Une matrice trop grande est répartie sur plusieurs sessions indépendantes, chacune terminée par `TERMINATED`.

La campagne de signes de phase 2B suit en plus la section 5.4 : chaque session ne commite que des chunks atomiques, reprend la chaîne de racines déjà vérifiée et peut s'arrêter entre deux transactions sans publier de certificat final. Les deux corpus de 11 880 000 signes sont achevés sur autant de sessions bornées que nécessaire; aucun shard isolé ni état `running` ne satisfait la porte.

- oracle aléatoire jusqu'à $n=14$ ;
- différentiel CPU/GPU sur au moins 1 000 nuages ;
- tous les nombres d'amas de 1 à 64 à taille moyenne ;
- matrice de précision et d'ordonnancement ;
- tailles jusqu'à $10^5$ sur les six régimes ;
- un cas $10^6$ tournant par shard ;
- test d'interruption et de reprise d'un checkpoint ;
- vérification finale de l'arrêt de la VM.

### 17.3 Release

- reprise de toutes les régressions historiques ;
- matrice complète jusqu'à $10^5$ ;
- au moins trois graines pour $10^6$ et $3\times10^6$ ;
- un passage $10^7$ sur Poisson favorable, mélange et charge dense contrôlée ;
- rapport de performance brut, environnement et compteurs ;
- test de préemption Spot pendant au moins trois phases distinctes ;
- validation des critères de passage de la section 19.

## 18. GCP, Spot, reprise et extinction

### 18.1 Garde de durée de vie

Aucune campagne ne démarre si la VM ne possède pas une durée de vie maximale finie conforme aux scripts `gcp-migration`, une action terminale explicite et un arrêt invité armé. Le pré-test doit vérifier l'état réel renvoyé par GCE, pas seulement les variables du script.

La suite GCP vérifie au minimum :

- refus d'une création ou d'un démarrage sans limite de durée ;
- refus avant mutation GCE d'une clé SSH de session absente, chiffrée, symbolique, trop permissive, mal appariée, physiquement située dans le dépôt ou absente d'un profil OS Login unique à expiration bornée; import temporaire, propagation explicite de la même clé et de la même échéance UTC absolue vers start, SSH et SCP, absence de renouvellement relatif, révocation après certification de génération `TERMINATED`, nettoyage après preuve qu'aucune génération n'a changé et conservation sous l'échéance initiale lorsque cette preuve manque ;
- présence d'un `maxRunDuration` borné et d'une échéance future : `terminationTimestamp` exposé et cohérent, ou à défaut somme certifiée d'un `lastStartTimestamp` frais et de la durée GCE relue; la borne sûre retranche toute tolérance d'acceptation, soit 300 secondes dans les scripts actuels ;
- action de fin `STOP` et non redémarrage automatique incontrôlé ;
- arrêt invité de secours armé ;
- labels de projet permettant un balayage de sécurité ;
- arrêt même après erreur de test, signal ou préemption.
- deadline de travail par défaut au moins trente minutes avant cette borne GCE sûre, sans nouvelle unité lancée après cette deadline; une campagne transactionnelle et reprenable peut employer une marge réduite d'au moins quinze minutes seulement si chaque unité est bornée à 240 secondes au plus, si un checkpoint atomique est publié après chaque unité, si vérification, copie et nettoyage sont eux-mêmes bornés par le temps restant et si les époques permettent d'auditer la marge choisie; preflight, build et unités conteneurisées restent bornés dynamiquement, avec une réserve explicite pour tuer les descendants et nettoyer avant la deadline. Toute autre campagne conserve la marge de trente minutes.
- `timeout`, `date` et `sleep` résolus par leurs chemins système fixes, avec binaire et chaîne de parents root et non inscriptibles par le groupe ou les autres avant le premier calcul de deadline ; GNU `timeout` exécuté en mode de groupe de processus, sans `--foreground`, avec preuve hermétique qu'un descendant qui survit à `SIGTERM` reçoit le `SIGKILL` terminal.
- avant suppression des temporaires invités, remontée bornée des 240 dernières lignes et de 65 536 octets au plus du journal de toute unité de qualification en échec, sans publication d'un artefact de succès.
- résolution de `nvcc` malgré un `PATH` non interactif réduit, depuis un `CUDA_HOME` absolu ou les emplacements CUDA 12.9 usuels, sans accepter une autre version du toolkit.
- accès Docker direct ou via le chemin fixe `/usr/bin/docker` certifié root et non inscriptible avant `sudo -n`, sans exécuter un client injecté par `PATH`, avec timeout individuel, retry non mutatif borné et aucune nouvelle sonde après la deadline, puis diagnostic borné du daemon, de systemd, des paquets et du journal avant tout build en cas d'échec ;
- chaque conteneur de qualification possède un `cidfile`, un nom et un label privés; tout repli par nom atteste aussi l'image, le label et le CID disponible avant `docker rm -f`, puis exige une absence stable après succès, échec ou signal, et tout CID invalide ou échec de nettoyage interdit l'artefact final ;
- si et seulement si l'option explicite de préparation Docker est présente : relecture préalable des deux bornes, Ubuntu 22.04 `amd64` et toolkit NVIDIA figé, refus des familles de paquets concurrentes, des états `dpkg` partiels, des chemins élevés ou configurations ambigus, simulation APT sans suppression ni changement d'un paquet installé, installation des candidats exacts `docker.io` et `docker-buildx` depuis les seuls dépôts existants, validation de `daemon.json`, activation bornée des services, conservation du boot ID et de l'arrêt invité, recertification GCE après préparation, puis premier conteneur GPU laissé au worker non mutatif.

### 18.2 Checkpoints

Les checkpoints sont atomiques et contiennent : hash de l'entrée, hash Git, paramètres, ordre courant, runs achevés, dernier niveau entièrement fermé, compteurs et sommes de contrôle des artefacts. Un checkpoint n'est publiable qu'après écriture dans un chemin temporaire, synchronisation puis renommage atomique.

Les tests interrompent le processus :

1. après construction de l'index spatial ;
2. au milieu de la génération de cellules ou de candidats ;
3. entre deux runs externes ;
4. au milieu d'un grand lot de niveau égal ;
5. pendant la réduction hyper-Kruskal ;
6. juste avant l'écriture de la sortie finale.

Après reprise, la sortie canonique et le statut doivent être identiques à une exécution sans interruption. Aucun événement ni union ne peut être dupliqué. Un lot égal partiellement appliqué est annulé ou rejoué intégralement.

### 18.3 Préemption Spot

Une campagne de qualification force ou simule une préemption aux points d'interruption 2, 3 et 5 de la section 18.2. Elle mesure le temps perdu, le volume relu et la quantité recomputée. Une préemption ne peut transformer une exécution incomplète en `exact`.

### 18.4 Condition terminale obligatoire

À la fin de **toute** session G4, succès ou échec :

1. exécuter le script d'arrêt et de vérification prévu dans `gcp-migration` ;
2. attendre l'état GCE `TERMINATED` de l'instance ciblée ;
3. balayer toutes les instances portant le label du projet E-HGP ;
4. inventorier et signaler les autres instances actives sans faire échouer la campagne à cause d'elles ;
5. ne jamais arrêter, modifier ou supprimer une instance autre que la cible exacte sans autorisation explicite ;
6. enregistrer l'heure, l'instance ciblée et sa preuve d'état dans l'artefact de campagne.

Une campagne n'est pas « terminée » tant que cette condition n'est pas satisfaite. Les workflows GitHub ordinaires ne doivent jamais créer ni démarrer automatiquement une ressource facturable.

## 19. Portes de passage et critères go/no-go

| porte | condition `go` | condition `no-go` immédiate |
|---|---|---|
| G0 — formats | schéma v2 actif, v1 archivé, manifests et sorties canoniques stables | sortie non versionnée, base de preuve interdite ou résultat non reproductible |
| G1 — prédicats | zéro faux signe sur tout T1 | un filtre certifie un mauvais signe |
| G2 — catalogue | égalité exhaustive jusqu'à $n=14$ | événement manquant ou faux avec statut `exact` |
| G3 — hiérarchie | référence exacte égale à Gamma; voie Gabriel incluse positivement et marquée partielle | ordre de threads modifiant la hiérarchie, connexion inventée ou Gabriel brut annoncé exact |
| G4 — verticalité | unicité et commutation à tous les seuils | flèche absente, multiple ou non commutative |
| G5 — GPU | zéro différence canonique seulement pour une future base exacte prouvée; sinon toutes les connexions GPU sont incluses dans Gamma et le statut reste conditionnel | approximation silencieuse, connexion absente de Gamma, repli non signalé ou faux statut exact |
| G6 — 50k interactif | objectif $p95\leq1$ s atteint sur les deux familles favorables | dépassement : pas de revendication interactive, profilage requis |
| G7 — million | fin sous budget sur cas sparse avec statut conforme à la base disponible, sans dépassement mémoire | OOM non contrôlé, perte de run/checkpoint ou faux statut exact |
| G7b — trois millions | trois graines sparse au statut honnête, streaming transactionnel et reprise vérifiée | OOM, cellule non fermée, état durable incohérent ou faux statut exact |
| G8 — adversarial | sortie exacte, ou statut limité honnête et exploitable | catalogue tronqué annoncé `exact` |
| G9 — exploitation GCP | reprise identique et instance ciblée vérifiée `TERMINATED` | instance ciblée encore active ou garde de durée absente |

Les portes G1 à G5 sont absolues. Les objectifs G6, G7 et G7b sont des décisions de produit : leur échec déclenche une analyse et peut reporter le jalon scalable, mais ne doit jamais conduire à affaiblir les tests mathématiques.

## 20. Jalons de validation

### 20.1 `v1-correctness`

Ce jalon peut être qualifié uniquement si :

1. son domaine de statut `exact` est écrit sans ambiguïté ;
2. T1–T4 ne révèlent aucun désaccord ;
3. le périmètre `hgp_reduced` est validé exhaustivement jusqu'à $n=14$ pour tous les ordres ;
4. `full_pi0`, s'il n'est pas démontré, est désactivé ou explicitement conditionnel ;
5. les données dégénérées sont traitées ou refusées sans jitter silencieux ;
6. le backend GPU produit soit la même sortie sous une future base exacte prouvée, soit une connectivité positive incluse dans Gamma avec statut conditionnel ;
7. les compteurs permettent d'expliquer le coût et tout abandon de budget ;
8. le test $k=1$ coïncide avec l'EMST ;
9. le jalon $50\,000$ points possède une mesure G4 reproductible, sans que l'étiquette `interactive` soit accordée si elle dépasse une seconde ;
10. chaque campagne GCP se clôt par une preuve d'état `TERMINATED` de son instance exactement ciblée.

### 20.2 `v1-interactive-scalable`

Ce jalon exige `v1-correctness`, G6, G7 et G7b. Il exige aussi au moins une campagne à dix millions qui se termine sans corruption ni OOM non contrôlé : soit `forest_semantics=exact, public_status=exact` si le certificat reste sparse, soit `mode=budgeted, forest_semantics=partial_refinement` avec `public_status=conditional` ou `public_status=budget_exhausted`, checkpoint durable, `PartialScope` et diagnostic complet. Seul `public_status=exact` peut valider l'objectif conditionnel de dix minutes.

Le principe final est simple : **chaque événement peut être rapide ou lent, mais il doit être vrai ; chaque catalogue peut être complet ou budgété, mais son statut doit le dire ; chaque VM peut être préemptée, mais elle ne doit jamais rester allumée par oubli.**
