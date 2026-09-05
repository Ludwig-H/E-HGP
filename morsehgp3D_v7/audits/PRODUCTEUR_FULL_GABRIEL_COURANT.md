# Producteur FULL Gabriel : qualification indépendante

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans ce dossier, sur `main`.

**Le producteur horizontal FULL concorde avec Gamma sur les 100 ordres du corpus indépendant, encodés en 200 représentations.** Les deux builds nominaux O2 et ASan/UBSan passent chacun 16 506 coupes et 1 606 couvertures ; trois mutations privées sont réfutées. Aucun défaut produit n'a été trouvé. Ce résultat ferme le raccord des portails aux parents FULL sur ce corpus et sous l'autorité extérieure annoncée des catalogues.

Le composant examiné est `src/forest/full_gabriel.hpp`, SHA256 `e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170`. Les [sources capturées](receipts_full_producer_20260905/source_pins.json) distinguent ces octets du HEAD et des changements simultanés du constructeur. La [revue sémantique](receipts_full_producer_20260905/semantic_review.md) et la qualification exécutée ont leurs [reçus propres](receipts_full_producer_20260905/README.md).

## Confrontation indépendante exécutée

Le juge construit les catalogues par MEB rationnelles, supports positifs et puissances globales, puis les compare à un oracle Gamma séparé. Il n'appelle ni le générateur C++ de catalogues ni le modèle Python FullPortal. Les 18 cas comprennent les dix cas historiques, un singleton et sept nouveaux nuages de sept points. Tous leurs ordres 1..n sont présents. Les permutations physiques des points et des catalogues, ainsi que les multiples distincts des fractions, conservent exactement les sorties et compteurs.

Par build nominal : 1 606 nœuds, 1 020 minima, 1 406 références parentales, 16 506 coupes ouvertes/fermées et 17 774 carrés de naturalité. L'identification à Gamma passe par les ensembles de minima étiquetés, avec surjectivité et couverture ponctuelle vérifiées. Les 200 représentations restent 100 ordres, pas 200 expériences indépendantes. Le corpus ne qualifie pas K8, K9 ou K10.

Les deux fixtures budgétaires E5/K2 et E5 étendue/K2 réussissent à leurs treize plafonds exacts. Diminuer chaque plafond séparément d'un donne 26 refus nommés, sans forêt partielle. Les portails sont non vides : dix demandes, douze pas et 22 MEB au total. Les compteurs de construction du cœur F restent tous nuls. Sur E5/K2, la somme indépendante des supports examinés est cinq : un pour AC, quatre pour le triangle terminal aigu. Elle détecte une remise à zéro du budget entre appels.

La fixture permanente `E5_chain_two` ajoute Z=(4,2,2) et W=(12,3,11) à E5. Le chemin rationnel AC → CDE → DEZ descend de 33/2 à 162/25 puis 21/4 ; Z est l'unique intrus de la boule intermédiaire, de puissance −3/5. Le C++ observe une demande, deux pas, trois MEB et sept candidats sur chacun des deux encodages. Le journal publié ne contient pas ces cofaces silencieuses ; le pont observe les compteurs et juge la forêt résultante.

Les copies privées sans normalisation terminale, avec budget de supports réinitialisé, et sans résolution de portail sont toutes réfutées par le juge, normalement et sous `-O`. Les sorties nominales sont identiques octet pour octet ; compilateur et ASan/UBSan ne produisent aucun diagnostic, avec `detect_leaks=1`. Les reçus conservent aussi l'erreur initiale du transport d'audit : certains milieux rationnels dépassaient son dénominateur u64. Des coupes dyadiques strictement intermédiaires, vérifiées exactement, la corrigent sans changer le produit.

## Autorité mathématique et calendrier

Le constructeur consomme les Gabriel de cardinal K pour ses minima, et ceux de cardinal K+1 pour ses connexions. À K1 les points naissent à zéro ; à K=n≤10 la feuille terminale est conservée. Les retraits essentiels d'une directe, au plus quatre, demandent une racine strictement antérieure. Les retraits d'intérieurs sont installés au même niveau, après toutes les multifusions du lot. Les alias désignent des composantes historiques, jamais de simples ensembles de points.

Une facette stricte inconnue possède au moins deux intrus sous complétude des catalogues et conservation des alias. Avec deux intrus z,w et un essentiel u, la coface F+z conserve la boule de F ; remplacer u par w descend strictement. Le premier recalcul et la première requête spatiale sur F+z sont donc économisés. Chaque coface suivante reçoit sa propre MEB et, sauf terminal déjà certifié par le catalogue, son contrôle d'intrus et de coquille. L'ancre terminale ancienne est normalisée avant consommation, y compris si sa directe n'avait émis aucune fusion.

La revue ferme également le raccord entre la **seule paire de catalogues réguliers K/K+1** et l'inertie des autres blocs. Si une boule non Gabriel avait assez peu d'intérieurs et d'essentiels, les compléter par des points de coquille produirait un Gabriel irrégulier au cardinal K ou K+1, contredisant la prémisse des catalogues. Les blocs restants sont au-dessus de la fenêtre de rang et relèvent du théorème d'inertie. Cela n'impose pas la régularité de toutes les boules du nuage : une boule irrégulière effectivement visitée reste refusée localement. La preuve détaillée et ses hypothèses sont dans la [revue](receipts_full_producer_20260905/semantic_review.md#1-autorité-dentrée-et-domaine-mathématique).

## Ce que le composant évite de construire

Le produit garde minima, vraies multifusions, alias des facettes consommées et successeurs des nœuds historiques. Il ne construit ni mosaïque de Delaunay, ni cœur Gamma global, ni journal de ses cofaces silencieuses. Le helper F reste un objet privé persistant par ordre ; seules ses méthodes MEB et intrus sont appelées. La référence exhaustive demeure exclusivement dans le juge borné.

Cette économie de structures est établie par le code. Elle n'est pas une borne de coût industriel : les deux catalogues, leurs permutations, les alias, la DSU locale, les lots et la forêt peuvent coexister. Les plafonds sont prospectifs mais ne couvrent pas toute la RAM ni toutes les opérations de DSU. Sans cache de cofaces, plusieurs portails peuvent refaire une descente. Le nombre de minima lui-même n'est pas nécessairement linéaire en points.

## Reçus constructeur et limite d'autorité

Les [reçus constructeur contre-vérifiés](receipts_full_producer_20260905/constructor_receipt_review.md) ferment séparément sept CTests Release, dont 67 comparaisons d'ordres et 1 492 coupes pour le nominal, 80 refus ciblés et 102 injections d'allocation. Les 67 comparaisons sont 40 couples nuage/ordre et 27 permutations. Le premier essai sanitizer échoue sur LeakSanitizer/`ptrace` : son journal reste conservé. La [contre-vérification complémentaire](receipts_full_producer_20260905/constructor_closure_review.md) distingue la reprise ROOT réussie 7/7 sur les mêmes binaires, détection des fuites conservée. Les preuves du pont indépendant ont leur propre provenance.

L'autorité littérale reste `full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues`. Une liste syntaxiquement valide mais incomplète peut réussir ; la sentinelle constructeur qui omet la connexion ABC le démontre. Le statut relatif ne certifie donc pas un fournisseur arbitraire de catalogues. Le `CloudIndex` doit aussi venir du constructeur contrôlé et rester immuable pendant l'appel.

## Alias à la demande : verrou mathématique levé

La [nouvelle contrelecture](receipts_full_producer_20260905/lazy_alias_next_step_review.md) confirme que les minima, les ancres de toutes les directes et leurs successeurs suffisent. Les autres alias peuvent être un cache facultatif, jusqu’à une capacité zéro. Sur un miss strict, zéro intrus renvoie au minimum ; un intrus z donne la directe F+z, de même boule et de niveau strictement antérieur ; au moins deux intrus déclenchent la descente actuelle. Le cas à un intrus évite toute MEB supplémentaire de F+z. Le census de coquille doit rester achevé, et le label, le niveau, l’antériorité et la normalisation de l’ancre restent contrôlés.

La [fixture rationnelle permanente](receipts_full_producer_20260905/lazy_alias_fixture.json) rend ce cas concret : ABC a pour support AB et naît au niveau 4 ; omettre l’alias égal AB reste correct si la connexion ABW, au niveau 841/100, le retrouve par son unique intrus C. Onze MEB régulières et douze états Gamma sont vérifiés indépendamment. Cette vérification géométrique n’exécute aucun dispatcher paresseux.

La politique de cache et la facturation des misses doivent être versionnées : évincer ne rembourse aucun travail. Les ancres des directes sans fusion restent permanentes. Cette preuve autorise le prochain delta mémoire ; elle ne lui attribue encore ni compilation, ni gain de résidence ou de temps.

## Prochain raccord utile

Le prochain jalon est une tentative horizontale multi-ordre dont les catalogues adjacents sont partagés et dont les résultats sont liés à l'entrée, à l'ordre, à l'horizon et au succès terminal. Le coût doit inclure génération, census, portails et stockage effectivement conservé. Un simple nombre de nœuds ou l'existence d'une racine finale ne constitue pas un digest sémantique.

Pour la verticale FULL, conserver l'ancre de chaque minimum dans sa directe inférieure après fermeture du plateau, puis propager et normaliser. Pour les masses, conserver séparément toutes les incidences contributrices de l'univers déclaré ; les minima topologiques ne suffisent pas à l'Algorithme 1. CLI, archive, identité publique, horizon, verticale, masses et coût massif restent des raccords distincts. Aucun résultat D/E/F ou G structurel n'est renommé résultat de ce producteur. GCP non utilisé.
