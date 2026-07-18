# Progression Phase 5 — ancre exacte $k=1$ et EMST

## Statut

- phase : `5`, en cours;
- backend : `reference_cpu`;
- profil : `hgp_reduced`;
- mode : `certified`;
- porte d'entrée : satisfaite par les Phases 1 et 4 fermées;
- porte de sortie : non satisfaite; la voie EMST GPU scalable reste à livrer et à qualifier.

Les cinq premiers jalons CPU de Phase 5 livrent l'ancre EMST exacte, le catalogue exhaustif des paires, la réduction rang-deux du backend CPU, leur différentiel indépendant jusqu'à $n=14$, la forêt hiérarchique compacte construite depuis un EMST déjà certifié et un producteur Borůvka exact sur le LBVH global. Ils ne ferment ni la Phase 5, ni la porte globale G2 du catalogue, et ne publient aucun `public_status=exact` pour une tour MorseHGP3D complète.

## Ancre mathématique

Le nuage est d'abord transformé en `CanonicalPointCloud`; les sommets du graphe complet sont donc les `PointId` canoniques $0,\ldots,n-1$. Pour chaque paire distincte $(u,v)$, le noyau recalcule exactement la longueur carrée dyadique $d^2(u,v)$ et lui associe le niveau HGP $a(u,v)=d^2(u,v)/4$.

Toutes les fractions passent par `ExactRational` et tous les niveaux par `ExactLevel`. Le facteur $1/4$, les comparaisons, l'égalité des poids et le poids total sont donc rationnels exacts; aucune tolérance flottante n'intervient.

Les $n$ minima de rang un sont matérialisés comme feuilles singleton de niveau zéro. Pour $n=1$, la racine est cette feuille, le graphe et l'EMST sont vides, la coupe stricte au niveau zéro est vide et la coupe fermée contient le singleton.

## Lots égaux et multifusions

Les arêtes du graphe complet sont triées par `(longueur carrée exacte, u, v)`, puis groupées par égalité rationnelle exacte. Pour chaque lot :

1. les composantes strictement antérieures sont figées;
2. toutes les arêtes du lot sont projetées sur ces racines;
3. chaque composante connexe du graphe quotient devient une unique multifusion canonique;
4. les enfants sont ordonnés par leur couverture canonique en `PointId`;
5. Kruskal sélectionne ensuite un représentant déterministe de l'EMST;
6. les unions globales sont appliquées et l'état fermé du lot est enregistré.

La suite binaire des unions de Kruskal ne définit donc jamais la généalogie publique. Un chemin de six points séparés par la même longueur produit un seul nœud d'arité six, pas cinq fusions binaires dépendantes de l'ordre.

En présence d'ex æquo, le choix déterministe des arêtes permet le rejeu et l'audit, mais l'identité scientifique repose sur les coupes exactes, le poids total et les multifusions canoniques. Le résultat conserve aussi toutes les arêtes du graphe complet et tous les lots, y compris ceux devenus redondants après la connexion de l'arbre.

## API et audit

La cible installable `morsehgp3d::hierarchy` expose `build_exact_complete_graph_emst`, `build_compact_k1_forest`, `build_exact_lbvh_boruvka` et `verify_exact_lbvh_boruvka`. Le résultat de référence complet contient :

- les arêtes complètes et les arêtes EMST avec longueur carrée et niveau HGP;
- les feuilles et nœuds de multifusion de $T_1$;
- les lots égaux avec états pré-lot et post-lot;
- les multifusions avec composantes enfants figées et union fusionnée;
- les poids totaux exacts géométrique et HGP;
- des compteurs de distances, lots, redondances, événements et arités;
- le rejeu des coupes strictes ou fermées depuis le graphe complet ou l'EMST sélectionné.

L'ancre complète est volontairement quadratique : elle matérialise le graphe euclidien complet et sert de vérité terrain exacte CPU. Cette portée est distincte de l'adaptateur compact décrit ci-dessous et de la future voie GPU destinée à plusieurs millions de points.

## Forêt hiérarchique compacte

`K1CompactForest` reçoit les $n-1$ arêtes d'un EMST exact déjà certifié par un producteur géométrique. L'adaptateur brut vérifie le domaine des sommets, la positivité des longueurs, la relation exacte entre longueur carrée et niveau, l'absence de boucle, cycle ou doublon, puis la connexité. Il ne peut pas déduire des seules arêtes que leurs poids sont bien les distances du nuage ni que l'arbre est minimal; ces deux propriétés sont des préconditions explicites. L'overload depuis `K1EmstResult` est la voie ancrée actuelle.

Les feuilles singleton gardent les identifiants de $0$ à $n-1$ mais deviennent implicites. Les données persistantes sont limitées à cinq arènes : niveaux exacts distincts portés par l'EMST, arêtes de l'arbre, nœuds internes, références d'enfants CSR et descripteurs de lots. Ces niveaux sont exactement ceux où la partition change; contrairement à l'ancre par graphe complet, l'arène compacte ne recopie pas les lots de longueurs devenus redondants. Elle reste néanmoins interrogeable à tout seuil exact. Un nœud interne conserve son indice de niveau, sa tranche CSR, son plus petit `PointId` et la taille de sa couverture; aucun vecteur de couverture n'est stocké. Les couvertures, nœuds riches et multifusions restent matérialisables à la demande pour l'oracle et le diagnostic, mais matérialiser tous les nœuds d'une chaîne produit volontairement une sortie en $\Theta(n^2)$ et ne fait pas partie de la voie scalable.

La construction trie d'abord les arêtes par niveau exact puis extrémités, fige les composantes avant chaque lot, construit le quotient du lot entier et alloue un nœud par composante connexe de ce quotient. Elle prend $O(n\log n)$ avec une entrée d'ordre arbitraire et conserve $O(n)$ enregistrements persistants. Si les $m$ nœuds internes ont les arités $a_i\geq2$, l'identité d'un arbre enraciné donne :

$$\sum_{i=1}^{m}(a_i-1)=n-1,\qquad m\leq n-1,\qquad \sum_{i=1}^{m}a_i=n+m-1\leq2n-2.$$

Le nombre de niveaux et de lots est au plus $n-1$. En comptant un enregistrement par arête, niveau, nœud interne, référence d'enfant et lot, les cinq arènes satisfont donc :

$$\lvert E_T\rvert+\lvert L\rvert+m+\sum_{i=1}^{m}a_i+\lvert B\rvert\leq6(n-1).$$

Cette borne porte sur les enregistrements principaux : elle ne compte ni les métadonnées d'allocateur, ni le nombre de limbs des entiers multiprécision. Le compteur exécutable expose la borne $6(n-1)$, sa valeur réellement occupée et `stored_coverage_point_id_count=0`.

Pour la canonisation, les enfants d'une multifusion sont des composantes non vides et deux à deux disjointes du même état pré-lot. Leurs minima sont donc distincts, et l'ordre lexicographique de leurs couvertures triées est exactement l'ordre de leur plus petit `PointId`. Un seul identifiant et une seule taille remplacent ainsi la copie de chaque couverture sans modifier l'ordre public des nœuds.

Enfin, si $T$ est un EMST exact du graphe euclidien complet $G$, leurs partitions coïncident de chaque côté de tout seuil :

$$\pi_0(T_{<\lambda})=\pi_0(G_{<\lambda}),\qquad \pi_0(T_{\leq\lambda})=\pi_0(G_{\leq\lambda}).$$

En effet, si un chemin actif de $G$ reliait deux composantes encore séparées dans $T$, l'une de ses arêtes traverserait la coupe créée par une arête inactive du chemin de $T$. L'échange remplacerait cette dernière par une arête strictement plus légère, en contradiction avec la minimalité de $T$. Les deux partitions encadrant un lot déterminent son graphe quotient et ses multifusions; deux témoins EMST différents sous ex æquo produisent donc les mêmes nœuds, coupes et multifusions canoniques, même si leurs arêtes sélectionnées diffèrent.

## Producteur Borůvka exact sur le LBVH

`build_exact_lbvh_boruvka` remplace la matérialisation du graphe complet par des recherches point-vers-LBVH exactes sur l'index Morton global déjà construit. La ronde $r$ fige les composantes et leur label canonique, égal au plus petit `PointId`; elle ne modifie ces labels qu'après avoir déterminé tous les minima sortants. Pour une arête aux extrémités canoniques $u<v$, l'ordre total est

$$\kappa(e)=\bigl(d^2(e),u,v\bigr).$$

Chaque composante choisit son unique arête sortante minimale pour $\kappa$. Cette arête est le minimum unique de la coupe définie par la composante; la propriété de coupe l'inclut donc dans l'arbre de Kruskal canonique $T^{\star}$ construit avec le même ordre total. Par induction sur les rondes, toutes les arêtes acceptées appartiennent à $T^{\star}$, et leur ensemble dédupliqué est une forêt. Tant qu'il reste $c_r>1$ composantes, chacune est incidente à une arête choisie; aucune composante contractée n'est isolée dans cette forêt. Toute composante post-ronde contient donc au moins deux composantes pré-ronde, d'où

$$c_{r+1}\leq\left\lfloor\frac{c_r}{2}\right\rfloor,$$

et au plus $\lceil\log_2(n)\rceil$ rondes pour $n\geq2$. Les arêtes finales sont retriées par $\kappa$ avant leur remise à `K1CompactForest`; l'ordre des rondes Borůvka ne définit jamais les lots égaux ni les multifusions publiques.

À chaque ronde, un passage postordre marque chaque nœud LBVH comme uniforme pour une composante figée ou mixte. Un sous-arbre uniforme de la composante requérante est rejeté sans évaluation de feuilles. Pour les autres nœuds, la borne exacte de distance à l'AABB ne permet un prune que lorsqu'elle est strictement supérieure au meilleur poids courant. Une égalité descend obligatoirement, car une arête de même poids peut encore gagner sur les extrémités de $\kappa$; ni epsilon ni ordre flottant ne décide ce cas.

Le résultat `K1ExactBoruvkaResult` reste volontairement distinct de `K1EmstResult` : il ne matérialise ni graphe complet, ni hiérarchie, ni couverture persistante, et son `emst_witness_certified` est un certificat local, pas un `public_status`. `verify_exact_lbvh_boruvka` rejoue séparément les labels figés, minima de composantes, arêtes acceptées, contractions, poids exacts et borne de rondes depuis le nuage et l'index immuables; il ne fait confiance ni au booléen de résultat, ni aux compteurs fournis. Ce producteur et ce vérificateur forment le socle CPU de correction pour la prochaine ronde de propositions GPU stackless; ils ne qualifient encore aucun backend GPU.

## Catalogue rang-deux et décision Gabriel

Pour chaque paire canonique $(u,v)$, une seconde voie reconstruit indépendamment son centre diamétral $c=(u+v)/2$, son rayon carré $a=\left\Vert u-v\right\Vert^2/4$, puis interroge tout le nuage par `brute_force_closed_ball`. La partition globale exacte sépare intérieur strict, shell complet et extérieur; aucune liste locale, exclusion de support, tolérance ou limite de visites n'intervient.

La décision distingue trois cas :

1. `rank_two_critical` : l'intérieur est vide et le shell vaut exactement $\lbrace u,v\rbrace$;
2. `extra_shell_degeneracy` : l'intérieur est vide mais le shell contient un autre site;
3. `interior_blocked` : un site au moins appartient à l'intérieur strict.

Les deux premiers cas appartiennent au graphe de Gabriel diagnostique du contrat existant. Seul le premier entre dans la réduction rang-deux certifiée. Le second reste une décision exacte et une arête diagnostique, mais force `catalog_status=unsupported_degeneracy` et interdit `locally_supported`; il n'est ni masqué comme rejet numérique, ni promu sous l'hypothèse de position générale.

Le catalogue conserve les enregistrements des $\binom{n}{2}$ paires, leurs centres, longueurs, niveaux, intérieurs, shells et rangs fermés, ainsi que les trois projections d'arêtes : graphe complet, rang-deux critique et Gabriel diagnostique. Son coût de référence est $O(n^3)$ évaluations exactes de distance et sa mémoire est quadratique.

## Certificat EMST–rang-deux

Une arête d'un EMST ne peut avoir ni point strictement intérieur à sa boule diamétrale, ni point supplémentaire sur son shell : dans les deux cas, ce troisième sommet forme avec ses extrémités un cycle dont les deux autres arêtes sont strictement plus courtes. Toute arête EMST est donc `rank_two_critical`.

À chaque seuil exact, l'arbre témoin de Kruskal est ainsi inclus dans le graphe rang-deux, lui-même inclus dans le graphe complet. Le rejeu du certificat compare explicitement, aux niveaux réunis des trois graphes, les coupes strictes et fermées des cinq chemins suivants : graphe complet, EMST sélectionné, graphe rang-deux, arbre témoin rang-deux et graphe de Gabriel diagnostique. L'égalité des états strictement pré-lot et fermés post-lot impose alors les mêmes multifusions canoniques.

La réduction rang-deux possède son propre parcours de lots. Elle fige les composantes strictement antérieures, construit le graphe quotient du lot entier, matérialise une multifusion par composante connexe, puis seulement sélectionne et applique les arêtes témoins. Le certificat sépare l'égalité sémantique des coupes, multifusions, hiérarchie et poids de l'égalité supplémentaire du témoin déterministe. Cette dernière reste exposée comme contrôle d'implémentation mais n'entre pas dans l'agrégat `anchor_equivalence_certified`, car plusieurs EMST sont valides sous ex æquo.

## Vérifications des lots CPU

Le test C++ strict couvre le singleton, le facteur exact $3\mapsto3/4$, deux fusions disjointes dans un même lot, une multifusion d'arité six, les $24$ permutations d'un carré avec ex æquo, les poids exacts et l'accord des coupes du graphe complet avec celles de l'EMST à chaque seuil strict et fermé. Il vérifie également que les lots partitionnent le graphe complet, que chaque perte de composante égale le nombre d'arêtes sélectionnées et que la racine couvre exactement tous les `PointId`.

La compilation GCC avec avertissements transformés en erreurs, le test ciblé, le profil ASan/UBSan et l'export CMake passent. Le différentiel indépendant du troisième lot complète ci-dessous cette non-régression du noyau.

Le second test strict couvre les trois décisions de paire, une perturbation d'un ULP de chaque côté d'un shell diamétral, deux multifusions disjointes dans le même lot, un tétraèdre régulier 3D dont les six arêtes rang-deux se contractent en une multifusion d'arité quatre, les diagonales extra-shell et les $24$ permutations d'un carré, les coupes strictes et fermées exactement au seuil, ainsi que la fixture non locale où chaque extrémité possède cinq observations plus proches que la paire Gabriel recherchée. Il ferme aussi tous les compteurs du catalogue et compare nœuds, poids et témoins à l'ancre EMST.

Le test strict de la forêt compacte compare ses matérialisations à l'ancre complète sur le singleton, une chaîne de 32 points à niveaux tous distincts, deux fusions simultanées, une multifusion d'arité six et les $24$ permutations du carré. Il permute aussi l'ordre et l'orientation des arêtes d'entrée, ferme chaque compteur et la borne $6(n-1)$, puis rejette les arbres incomplets, cycliques ou munis de poids incohérents. Deux EMST distincts sous ex æquo sont enfin comparés sur un carré prolongé par une fusion ultérieure : les arêtes témoins diffèrent, mais l'induction par lots restitue exactement les mêmes nœuds, coupes, multifusions et poids. Les compilations GCC et Clang en avertissements stricts, le CTest ciblé Release et le même test instrumenté ASan/UBSan passent.

Le test Borůvka couvre le singleton, une chaîne dont deux rondes sont nécessaires, un carré prolongé par une fusion ultérieure et ses $24$ permutations. Il compare le témoin final à Kruskal sur le graphe complet, ferme la réduction par moitié, les tags LBVH et les compteurs, puis vérifie que le rejeu ignore un booléen de certification falsifié mais rejette un poids exact altéré, un index étranger ou un nuage déplacé.

## Différentiel indépendant jusqu'à $n=14$

`morsehgp3d_hierarchy_k1_dump` accepte des coordonnées binary64 brutes par un protocole versionné, canonise le nuage, construit l'ancre C++ et émet un objet JSON canonique par cas. Le document expose chaque paire avec centre, niveau, partition globale et classification, les graphes rang-deux et Gabriel, les deux arbres témoins, leurs poids, les nœuds canoniques jusqu'à la racine, les multifusions, le certificat et, pour chaque niveau exact, les coupes strictes puis fermées des cinq chemins de rejeu. Le validateur ne se contente donc pas de relire les booléens du certificat.

`check_hierarchy_k1.py` recalcule directement chaque boule diamétrale avec `fractions.Fraction`, construit son propre graphe rang-deux et son propre quotient gelé par lots, puis utilise l'EMST stdlib indépendant de `tests/reference_emst`. L'oracle Gamma exhaustif fournit une troisième implémentation des niveaux et des coupes Gamma/Gabriel. Les sorties C++ ne fournissent aucun cutoff, candidat ou résultat intermédiaire à ces oracles.

La campagne contient 50 nuages exacts. Elle couvre toutes les tailles de $1$ à $14$, les dimensions affines un à trois dès leur cardinal minimal, les trois classifications de paire, les niveaux égaux, deux multifusions simultanées, les multifusions d'arité six et huit, un carré et un tétraèdre régulier, les deux côtés d'un shell à un ULP, des fractions dyadiques, les exposants binary64 extrêmes et la fixture Gabriel absente des petites listes locales. Pour chaque cas, les centres, niveaux, arêtes, poids, coupes, nœuds et multifusions coïncident exactement entre les voies applicables. Cela exerce un oracle par graphe complet exhaustif sur chaque nuage jusqu'à $n=14$; cela ne prétend ni énumérer tous les nuages possibles, ni remplacer les campagnes statistiques ultérieures.

L'identité des arêtes témoins reste un diagnostic d'implémentation séparé. Le validateur accepte explicitement un autre arbre témoin sous ex æquo dès lors qu'il appartient au graphe certifié et conserve toutes les coupes et le poids exact; la porte scientifique repose sur ces invariants, les nœuds canoniques et les multifusions.

GCP n'a pas été utilisé pour ces lots CPU.

## Suite immédiate

Le producteur Borůvka CPU ne matérialise plus le graphe complet, mais son parcours séquentiel exact sert encore de référence de correction et non de voie scalable. Le lot suivant introduit une première ronde GPU stackless sur le LBVH global avec un contrat de sur-ensemble de candidats, sans troncature; les décisions indécises retombent vers le CPU exact, puis les contractions et le témoin complet sont vérifiés par le socle décrit ci-dessus. Aucune promotion de phase ne précédera la boucle GPU complète et sa qualification scalable.
