# Gardes du journal : contre-fixtures causales et réservation des arènes

10 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce complément transforme les trous de tests transmis par le [second auditeur, § 9](../receipts_raccord_ancres_20260910/README.md#9-transmis-à-lauditeur-du-journal-hors-de-ma-qualification) en fixtures exécutables. **Dix cas valides, neuf rejets précis et six contrôles de lecture passent en O2 et ASan/UBSan ; neuf mutants sont détectés par leurs cas nommés.** Aucun défaut nominal du journal n’est établi. Ces fixtures sont prêtes à reprendre dans la gate constructeur ; elles n’y sont pas intégrées par cet audit.

Deux sources sont distinguées : le journal publié par d188e3de (`e8e65b21…`) et le **WIP figé** `7608e70e…`, qui ajoute seulement le précomptage et la réservation de cinq arènes. Il n’expose aucune API incrémentale. Le WIP possède ici une qualification structurelle bornée, sans promotion de la tour, de sa géométrie ou de la génération WSPD. Les autres qualifications du journal restent dans leurs [reçus propres](../receipts_coverage_cpp_20260910/README.md).

## Les fixtures discriminent une propriété à la fois

| Propriété supprimée par le mutant | Contre-fixture et observation qui le détecte |
| --- | --- |
| Cardinal minimal de naissance | K3, domaine {0,1,2}, population {0,1} ; refus `birth_size.reject`. Le positif ajoute seulement le site manquant et ajuste le masque entier. |
| Inclusion de l’intérieur à la naissance | K2, I={2}, U={0,1}, masque entier mais I exclu ; refus `birth_interior.reject`. Le positif change uniquement le drapeau. |
| Une référence par naissance | K2, populations {0,1} et {2,3}, une action de naissance portant deux références ; refus `birth_ref_count.reject`. Le positif en porte une. |
| Tri des parents | Deux naissances puis **dernier lot** de fusion [1,0] ; refus `parent_order.reject`. Le positif [0,1] vérifie aussi CSR, parents et successeurs. Aucun lot ultérieur ne masque le défaut. |
| Drapeau de lecture de l’intérieur | K3, naissance {0,1,2}, puis contribution de I={5}, U={3,4}, masque sélectionnant 3 et I exclu : fermé {0,1,2,3}, ouvert {0,1,2}. Le site 5 manque réellement de tous les héritages. Avec I inclus, fermé {0,1,2,3,5}. |
| Plafond K=10 | K11 avec **domaine et population de onze sites**, masque 2047 ; refus `order_max.reject`. K10 réussit sur la même entrée. La taille du domaine ne peut pas masquer le plafond. |
| Domaine de cardinal au moins K | Domaine et population {0,1}, K3 ; refus précoce `coverage_invalid_domain`, contre K2 valide. Ce mutant est détecté par la **raison exacte** : sans cette garde, la naissance serait encore refusée, mais par `coverage_birth_population`. |
| Exhaustivité des singletons K1 | Domaine {0,1}, une seule naissance à zéro ; refus `k1_count.reject`. Ajouter celle de 1 suffit au positif. |
| Refus de la racine ABSENT | Sur un certificat valide, vérifier le **statut invalide**, pas seulement les valeurs vides ; refus nommé `reader.absent.shell_only`. |

Tous les rejets de construction contrôlent un objet vide et illisible : ordre nul, aucune banque, aucune arène. Les tests de lecteur et les mutants de garde sont séparés pour éviter qu’une erreur de lecture sur ABSENT masque un autre rejet. Le cas n<K contrôle un diagnostic, sans prétendre qu’il ajouterait à lui seul une interdiction géométrique.

La population de la continuation utilisée pour le drapeau de lecture a cardinal K. Ce test ne dépend donc pas de la question des petites populations contributives. Dans le contrat structurel actuel, le plancher K est imposé aux naissances ; l’association d’une référence de continuation à un census géométrique admissible reste au producteur. Le carré K2 à quatre parents est déjà présent dans d188e3de : ce manque historique ne doit plus être une demande ouverte.

## Réservation et refus transactionnels

La fixture mixte contient deux naissances, une continuation et une fusion : trois nœuds, deux parents et trois contributions. Le même exécutable de test compte les allocations **du constructeur seul**, après préparation de sa banque et des lots.

| Source testée | Allocations observées | Pannes injectées, toutes refusées proprement |
| --- | ---: | ---: |
| Journal publié e8e65b21, O2 | 14 | 14 |
| Réservation WIP 7608e70e, O2 et SAN | 5 | 5 par build |

Chaque ordinal d’allocation est refusé séparément et rend `ResourceExhausted`, `coverage_allocation_failed`, sortie vide. Un lot final invalide après un préfixe valide vérifie aussi l’absence de sortie partielle. `length_error`, épuisement des identifiants et débordement physique de `size_t` ne sont pas exercés par ces petites entrées. Ce décompte ne prouve aucun gain de temps ou de pic RSS de la tour.

Différence observée et admise : premier lot avec dénominateur nul et première allocation simultanément refusée. L’ancien journal rend `InvalidInput` sans allocation ; le WIP rend `ResourceExhausted`, puisque ses réservations précèdent la validation des lots. Les deux sorties sont vides. Aucun contrat de priorité entre ces deux erreurs n’est imposé ici ; une comparaison de versions doit conserver cette distinction.

## Provenance et vérification

[guard_probe.cpp](guard_probe.cpp) porte les attentes explicites, sans oracle du producteur. Les [sources](source_refs.json) et neuf dépendances du journal sont figées ; chaque compilation a une fermeture de dix fichiers projet, son probe compris. Les neuf variantes privées se reconstruisent par les remplacements uniques de [mutations.json](mutations.json), dont les hashes sont confrontés aux compilations. Les ELF locaux et headers système ne sont pas distribués.

[record.py](record.py) conserve douze compilations strictes C++20 et deux passages des douze exécutables. Le premier passage échoue uniquement pour LeakSanitizer sous ptrace ; [son reçu](test_r1/receipt.json) reste inchangé. Le [rejeu autorisé hors bac à sable](test_r2/receipt.json) emploie les mêmes sources et binaires, `detect_leaks=1`, et réussit. Les neuf mutants quittent avec le code 1 et leur diagnostic causal exact ; les trois builds nominaux quittent avec le code 0. L’égalité O2/SAN du WIP est complète, y compris les compteurs de ce probe.

Le lecteur [verify.py](verify.py) vérifie les sceaux, les dépendances, les mutations reconstruites, les commandes closes et les observations de [review.json](review.json), sans compilation ni moteur. Il reste actif sous Python `-O`. Le recorder refuse l’écrasement de ses répertoires de capture ; les tentatives échouées ne sont jamais réécrites.

```bash
python3 -B morsehgp3D_v7/audits/receipts_journal_guards_20260910/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_journal_guards_20260910/verify.py
```

## Contrecalcul ponctuel sur le cache du second auditeur

La [suite cache](../receipts_raccord_ancres_20260910/suite_cache_20260910/README.md) mesure des entrées de 48 octets et décrit correctement l’arrondi de capacité à la puissance de deux supérieure à 16n. Son approximation « 7,7 Gio à dix millions » omet cet arrondi. Avec n=10 000 000, il faut **268 435 456 entrées, soit 12 884 901 888 octets = 12 Gio**, pour ce seul cache. [Les deux sources de ce calcul](cache_source_refs.json) sont épinglées ; aucune allocation massive n’a été tentée. Le calcul est inclus dans le lecteur portable.

Un budget de cache peut limiter sa capacité puis conserver les collisions évictives ou désactiver le mémo ; cela limite une accélération facultative, sans refuser le nuage ni supprimer une autorité géométrique. Le choix de budget et sa qualification appartiennent au constructeur. Les verrous des lots groupés, des blocs réels 50k et de la gate ASan du cache restent séparés. GCP non utilisé par cet audit.
