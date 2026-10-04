# Matrice de couverture de lecture

Toutes les références sont relatives à `source/morsehgp3D_v11/`. « Favorable »
désigne une relecture du code figé, sans nouvelle exécution native. Les portes
mentionnées sont celles enregistrées dans les `tests.cmake` et les tests capturés ;
leur présence et l'identité au pin c40 ne remplacent pas un reçu G4.

| Invariant | Source relue / point difficile | Portes ciblées présentes | Conclusion |
|---|---|---|---|
| Factories q1–q4 et budgets 18/21/24 | `num/geometry.hpp`, `sphere.cpp:24–102`, `budgets.hpp`, `integer.hpp`, `wide.hpp` ; q3 formule réduite, q4 centre fermé et Level différé, entiers signés contrôlés | géométrie/Fraction, candidate, power, checked-power, integer, overflow ; `tests/num/tests.cmake` | Favorable ; Sphere est une circonsphère, sa minimalité/criticité est une condition séparée. |
| Positivité et présentation | `num/q4_weights.hpp`, `orientation_certificate.hpp`, `predicates.cpp:36–57,183–230` ; tous les poids, signe du déterminant brut, certificat global ; dispatch par arité de présentation | q4 weights, orientation certificate, obtus, dégénérés, permutation et mutants | Favorable ; ne pas remplacer le tag de présentation par qmin. |
| Puissance et boîtes fermées | `num/power_certificate.hpp`, `predicates.cpp`, `center_region.cpp`, `centers.cpp` ; LB/UB exactes, contacts non rejetés, rank/dimension séparés | power-bounds, center-region, centers, profils et mutations | Favorable ; pas de filtre flottant géométrique F6 dans cette source. |
| Ordre rationnel et FENV | `num/level.hpp`, `catalogue/sort_level_key.hpp`, `sort_indices.cpp` ; clés normales, marge, repli exact, clé totale Level/S*/ordinal | sort FENV/mixed modes/FTZ-DAZ, égalités non réduites, permutations, pannes mémoire | Favorable ; décisions exactes après zone incertaine. ASan18 catalogue/FENV absent du périmètre annoncé. |
| Cloud/index propriétaire | `index/index.hpp`, `build.cpp`, `access.hpp` ; Cloud transféré uniquement après succès, boîtes/permutation immuables, médianes et escape | index unit/fault, moves, index étranger, structure et oracle | Favorable ; vues empruntées exigent un index immobile vivant. |
| Census strict/coquille | `index/census.cpp:45–88`, `census_workspace.cpp:74–96` ; lower>0/upper<0 stricts, deux passes, saturation explicite, workspace privé/CAS | census Fraction, contacts, borrowed/reentrance/concurrency, callbacks qui lèvent, fault | Favorable ; résultat saturé ne certifie pas une coquille complète. |
| Propriété G1 et enveloppes | `catalogue/boxes.cpp:60–89,119–158`, `catalogue.cpp:92–101` ; K témoins distincts et stricts sur fermeture, centres demi-ouverts, enveloppe max+1 | boxes, closed contacts, foreign/owner, mono-parallèle | Favorable ; invalidité d'un support témoin n'invalide pas son site comme dominateur. |
| Criticité/canonicalisation/I-U | `catalogue/leaf.cpp:116–192`, `support.cpp:15–124` ; inside avant census, I/U complets avant S*, admission avant Level, tous les supports | qmin3-from-q4, extended-shell, obtus-prefix, contacts et oracle catalogue | Favorable ; q3 obtus n'empêche pas la récursion q4. Un support non canonique ne remplace pas la visite du S* canonique. |
| Coupes de familles et caches | `catalogue/leaf.cpp:74–99,196–256`, `center_line_cache.hpp`, `small_pair_graph.hpp` ; G3 distinct witness union, seuil K+1−q, J2 fermé, reset par feuille/passe, fallback >32 | G3 descendants, region obtuse, pair-graph, live_rows/cache et mutants | Favorable ; pas de coupe par angle q3 ni de truncation à 32. |
| Frontier/jobs/deux passes | `frontier*`, `adaptive_*`, `parallel.cpp:36–62,109–170`, `internal.hpp` ; ordinals fixes, chemins possédés, replay, quotas, rebasing unique des populations | frontier identity, adaptive replay, parallèle/heaviest-first, faults et mutants | Favorable ; budget de planification atteint ⇒ DFS complet depuis l'antichaîne, pas abandon géométrique. |
| Single-pass/assembly/refus | `single_pass*`, `assemble.cpp`, `assembly_parallel.cpp` ; pages possédées, admission des coexistences, append transactionnel, sous-objets distincts dans halos | single-pass identité et pannes, parallel assembly, capacity | Favorable ; refus tardif sans catalogue reste prévu. Capacité Buffer n'est pas RSS. |
| Export points u24 | `bench/points_export.cpp:266–273,327–330`, `num/sphere.cpp:63–102` | **nouveau témoin autonome** `check_export_width.py` normal/−O ; gate native encore à ajouter | **Refus nécessaire sur tétraèdre u24 valide**, Level 196/148 bits. Profil points u21 inchangé. |

Pas de nouvel argument sous-quadratique global, de gain temporel ou de capacité
multi-millions déduit des modèles bornés. Les coûts fixes de frontier déjà signalés
dans la tranche 17 ne constituent pas une nouvelle découverte ici.
