# Certificat FULL C++ : qualification structurelle indépendante

5 septembre 2026, sources publiées dans `f4c0734c53a18d1e2de477ca09584c8f15c938f9`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le premier composant FULL est qualifié pour construire et relire les forêts déjà décidées de ce corpus.** Les nœuds, minima, parents, coupes et couvertures du vrai C++ concordent avec le juge indépendant, en O2 et sous ASan/UBSan. Aucun défaut produit n’a été trouvé. Ce résultat ferme le premier jalon structurel. Le [raccord géométrique](PRODUCTEUR_FULL_GABRIEL_COURANT.md) possède désormais une qualification distincte, sans réattribuer à ce lecteur la découverte des minima ou des parents.

La [preuve mathématique FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) et la présente qualification ont des objets distincts. Le composant porte justement `structural_only` et `full_minima_merge_forest_v1`. Il ne calcule aucune boule, aucun portail, aucun catalogue Gabriel, aucune verticale ou masse et n’est pas raccordé à la CLI F. Une forêt syntaxiquement valide peut omettre une fusion géométriquement nécessaire ; ce module ne prétend pas détecter cette omission.

## 1. Invariants fermés par la lecture du code

La [contrelecture sémantique](receipts_full_cpp_20260905/semantic_review.md) vérifie les anciens parents actifs, gelés avant le lot, et leur consommation unique avant toute nouvelle sortie. Les groupes simultanés ont des ensembles de parents disjoints ; les cycles, parents déjà absorbés et références créées dans le lot sont refusés. Le tri temporaire interdit les minima répétés entre niveaux.

Les trois arènes privées conservent seulement les nœuds, les labels des minima et les références de parents en CSR. Les couvertures restent dérivées. Les identités sont celles des composantes, même quand deux unions de PointId sont égales. Pour L minima, I fusions et R racines, les liens valent L+I−R et I≤L−R ; aucune borne de L par n n’est transférée aux ordres supérieurs.

Les niveaux sont comparés par produits croisés exacts, pas par égalité de représentation. Tous les points K1 naissent à zéro ; zéro ouvert les exclut. Pour K=n dans la capacité K≤10, une construction réussie possède nécessairement l’unique feuille du domaine et aucune fusion. Les lots validés sont atomiques malgré leur parcours physique séquentiel.

Les plafonds sont prospectifs. Dans le parcours des descendants, l’invariant `visited + pending <= max_nodes` rend les soustractions sûres ; le budget des points compte les références avant déduplication. Les copies sont interdites, les déplacements sans allocation invalident leur source. Refus tardifs et pannes mémoire ne publient aucune valeur partielle. La capacité allouée, le RSS, la coexistence avec les lots de l’appelant et le temps CPU ne sont pas bornés par ces seuls comptes.

## 2. Raccord exécuté au vrai composant

Le [pont C++](full_cpp_bridge.cpp) compile une copie littérale de huit en-têtes produit épinglés. Il reçoit des lots, des coupes et des plafonds ; il ne contient ni attendu topologique, ni géométrie, ni partition Gamma. Le [juge Python](full_cpp_audit.py) traduit les seuls journaux FULL déjà scellés vers l’ordre canonique de l’API : feuilles triées avant fusions, parents réadressés vers les identifiants denses. Il vérifie ensuite les trois arènes et toutes les lectures du C++.

Les 50 ordres et 285 événements scellés sont encodés deux fois : fractions réduites, puis numérateurs et dénominateurs multipliés par des facteurs distincts pour les lots et les coupes. Cela donne 100 représentations des mêmes objets, pas 100 nuages indépendants. Gamma exhaustif, borné à sept points, juge les composantes par inclusion des labels minimaux, leurs points et les carrés horizontaux ; il n’est jamais appelé par le composant produit.

| Contrôle par binaire nominal | Résultat |
| --- | --- |
| Corpus provenant de Gamma | 100 représentations, 570 nœuds, 4 530 coupes et 4 450 carrés horizontaux |
| Cas structurels supplémentaires | 6 représentations, 46 nœuds et 78 coupes ; aucune réalisabilité géométrique revendiquée |
| Total consommé par le C++ | 106 constructions, 616 nœuds, 382 minima, 510 références parentales, 4 608 coupes |
| Couvertures sous plafonds exacts | 616 succès, un pour chaque nœud historique |
| Plafonds de sous-arbre diminués d’un | 1 232 refus exacts, sans valeurs partielles |
| Plafond de construction diminué d’un | 106 refus `full_node_budget`, sans arène publiée |

Les sorties O2 et ASan/UBSan sont identiques octet pour octet ; aucun diagnostic sanitizer n’est présent. Les jugements passent normalement et sous `-O`, avec les mêmes comptes. Ces compilations sont des sondes indépendantes du composant, pas une reconstruction ni une réexécution des CTests du pipeline F. Les [sources, commandes, sorties et limites](receipts_full_cpp_20260905/README.md) sont conservées.

## 3. Identités et lots : les cas discriminants

Un cas structurel crée les minima `01`, `02`, `13`, `23`, puis fusionne simultanément `[0,3]` et `[1,2]`. Les deux racines obtenues couvrent chacune `{0,1,2,3}` et restent distinctes. Leur fusion ultérieure est nécessaire malgré l’absence de point nouveau. Le C++ conserve ces identités et cette fusion : une comparaison de couvertures ne remplace pas les parents.

Le même cas est renommé par une bijection non monotone comportant `UINT32_MAX`, avec tri des labels et réadressage de toutes les références. Une troisième fixture ajoute une feuille `45` au niveau des deux premières fusions, puis réunit les branches. Elle vérifie l’ordre « naissances avant fusions » dans le stockage, sans rendre la nouvelle naissance disponible comme ancien parent. Ces fixtures sont séparées des 50 ordres géométriques, dont aucun journal ne mélange naissance et fusion au même niveau.

## 4. Mutants et reçus du constructeur

Trois copies privées du véritable en-tête sont corrompues séparément ; les sources produit restent intactes. Chaque adaptateur mutant termine normalement, mais le juge indépendant refuse sa sortie avec le code 1 attendu.

| Mutation privée | Refus du juge, normal et optimisé |
| --- | --- |
| Inverser le côté ouvert/fermé | `full_cpp.cut_boundary` |
| Conserver actifs les anciens parents après fusion | `full_cpp.cut_parent_still_active` |
| Conserver les références ponctuelles répétées dans l’union | `full_cpp.coverage_duplicates` |

La [contre-vérification des reçus constructeur](receipts_full_cpp_20260905/constructor_receipt_review.md) confirme séparément 2/2 CTests Release et 2/2 ASan/UBSan. Chaque configuration rapporte 68 contrôles dans les positifs et 218 dans le mode rejets, qui répète les positifs : ce ne sont pas 286 tests indépendants. Les 45 refus de construction, 19 refus de lecture et 15 pannes persistantes d’allocation concordent avec les sources et les logs. Onze pins sources et deux binaires correspondent aux témoins publiés.

Leurs limites de provenance restent explicites : `LastTest.log` est l’autorité brute, sans XML ; configure/build n’ont pas leurs sorties archivées ; les pins sont postérieurs aux tests. La nouvelle compilation indépendante et ses dépendances capturées ne réécrivent pas cette provenance historique. Les anciennes 339 portes F ne deviennent pas des portes FULL.

## 5. Raccord suivant et attribution conservée

Le [producteur de lots FULL](PRODUCTEUR_FULL_GABRIEL_COURANT.md) a désormais sa qualification propre : minima Gabriel dès leur naissance, alias des facettes consommées, portails vers les racines strictement antérieures et multifusions atomiques. Ce rapport conserve les preuves du seul composant structurel. La réutilisation d’un minimum comme directe de l’ordre inférieur peut fournir son ancre verticale après fermeture du plateau inférieur.

Le manifeste extérieur devra lier entrée, ordres, métrique, horizon, convention de coupe et succès terminal. Une coupe après le dernier événement ne prouve pas que toutes les fusions nécessaires ont été fournies. Les facettes pondérées et leur affectation temporelle restent distinctes des minima topologiques. Ce raccord ne demande ni Gamma exhaustif dans le produit, ni reprise des qualifications MEB déjà closes.

La [contrelecture du prochain protocole](receipts_full_cpp_20260905/portal_next_step_review.md) justifie aussi les recherches limitées aux retraits essentiels et la réutilisation de la MEB à la première extension par un intrus. Elle précise les prémisses de catalogue, d’alias et de régularité ; elle ne transfère aucun résultat compilé au producteur, qui possède ses propres reçus.

La variante courante `G_full_structural` reconnaît les 145 fichiers du snapshot complet : 142 pins F inchangés, CMake relu et les deux nouveaux fichiers C++. Sa portée nouvelle est celle de ce module et de ses propres preuves ; les variantes D/E/F et leurs reçus restent intacts. GCP non utilisé.
