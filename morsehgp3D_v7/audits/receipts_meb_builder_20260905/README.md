# Qualification bornée du Builder MEB privé à deux budgets

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le vrai dispatcher du Builder privé conserve les résultats F sur le corpus compilé, avec Work persistant et publication distincte des charges physiques.** Les trois builds nominaux passent chacun 3 444 étapes MEB locales et 60 appels au wrapper de complétion de l'overlay. Les injections rendent les miroirs exceptionnels observables, et les quatre fautes de raccord sont réfutées. Il s'agit du header privé et de ses adaptations d'audit ; la ligne produit, son CLI et ses archives ne sont pas modifiés par ce reçu.

Le [run conservé](compiled/run.json) est terminé : compilation et exécutions du 5 septembre, 12:45:06–12:45:47 UTC, sous une échéance de 180 secondes. Ces durées documentent la session de qualification ; elles ne mesurent pas un gain de performance. Les résultats courants sont produits par le [juge de rejeu](../meb_builder_audit.py) dans [normal.json](compiled/normal.json) et [optimized.json](compiled/optimized.json). Le [driver de capture historique](compiled/driver_at_capture.py) est conservé séparément : les renforcements ultérieurs de provenance et de causes de rejet portent sur les mêmes bruts, sans nouvelle exécution binaire attribuée.

## Sources et raccord effectivement consommé

La [capture initiale](inputs/capture_manifest.json), sa [contrelecture](inputs/source_review.json) et la [table d'inclusion](inputs/include_map.json) distinguent les autorités suivantes :

| Autorité | Fichier conservé | Pin principal |
| --- | --- | --- |
| Builder candidat privé | [silent_incidence.hpp](inputs/overlay/silent_incidence.hpp) | `6e517c5705ca5d21dfe8fb920510ee50d61af7c53a465cfe3175959ff45a0b15` |
| Helper de proposition consommé | [meb_proposal.hpp](inputs/overlay/meb_proposal.hpp) | `33255ebcb92864acd6322424618ebdc2d4e1253e917004c1c4d76a3798ecf352` |
| Référence F entière | [silent_incidence.hpp F](../receipts_resolver_20260905/qualification/snapshots/source/silent_incidence.hpp) | `f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76` |
| Delta complet du Builder | [builder_from_F.patch](inputs/documents/builder_from_F.patch.txt) | `42c495bcce7b9a9a22bd13a59f271e96981ac8a3dc5420c7c8da36cb20a07e51` |

Le [contrat de port du constructeur](inputs/documents/BUILDER_PORT.md.txt) décrit la préparation privée. Sa mention initiale « non compilé » ne remplace pas l'autorité du run d'audit ultérieur. Le [bridge](../meb_builder_bridge.cpp) inclut F et l'overlay dans la même unité avec un renommage explicite des types et du namespace privé du candidat. Les primitives épinglées sont partagées ; le repli candidat conserve le corps F après seul renommage de sa méthode.

Chaque build possède un depfile et 20 dépendances locales : le bridge et 19 headers. Le bloc de provenance du rejeu contrôle l'inventaire exact, les sources reconstruites depuis la table, les transformations d'instrumentation/mutation et leurs patches conservés. Les pins du run et les hashes des flux relient les commandes exécutées à leurs entrées et sorties. Le hash propre du depfile n'était pas enregistré lors de la capture ; son contenu est désormais contrôlé et hashé au rejeu, sans inventer un pin historique. Ce contrôle ne prétend pas à une compilation hermétique : les depfiles `-MMD` n'inventorient pas les headers système, et les sources des bibliothèques système ne sont pas recapturées par ce reçu.

## Deux juges complémentaires

La comparaison à F examine le booléen local, le statut et le motif, les 13 champs legacy, tous les champs de boule et les événements. Elle conserve des sentinelles non vides et un statut initial artificiel dans les appels locaux. Les cinq nouveaux champs P, A, pivots, certificats et replis sont jugés séparément. La référence F est un différentiel complet des terminaux ; elle partage ses primitives avec l'overlay.

Le [juge rationnel indépendant](../meb_rational_oracle_20260905.py) résout les centres par Gram et élimination rationnelle, énumère les supports positifs et juge la contenance. Il fournit le premier rang de référence, la coquille et la boule attendue sans consulter la sortie F. Le niveau q4 brut est contrôlé en plus de sa valeur rationnelle. Les [cas compilés](compiled/cases.json) reprennent 3 430 appels de ce corpus et ajoutent 14 étapes de séquences persistantes ou de frontières scalaires. Les [attendus mathématiques](budget_cases.json) et leur [contrelecture](contract_review.md) justifient les séquences et la fixture de chaîne ; leur catalogue plus large n'est pas présenté intégralement comme exécuté ici. La [lecture des cas et des sorties](observed_contract_review.json) fixe cette couverture et les deux terminaux CHAIN à L2/L8, P2.

| Build nominal | Route | Étapes locales | Wrappers de complétion | Succès locaux certifiés q2/q3/q4 |
| --- | --- | ---: | ---: | --- |
| O2 | `NoObserver` du port | 3 444 | 60 | 416 / 313 / 64 |
| O1 avec UBSan | `NoObserver` du port | 3 444 | 60 | 416 / 313 / 64 |
| O2 instrumenté | Même dispatcher, hooks de causalité et de charge | 3 444 | 60 | 416 / 313 / 64 |

Les builds emploient C++20 et `-Wall -Wextra -Wpedantic -Werror` ; UBSan arrête sur la première erreur. Chaque nominal observe également 1 655 refus legacy et 40 refus de coquille dans les appels locaux. Le rejeu compare l'intégralité des sorties nominales native O2 et instrumentée, hors hooks. La route rapide reste active sous instrumentation.

Les séquences nommées comprennent P7/L12 sur quatre appels du **même Builder**, la diminution du cap P sans remise à zéro de Work, c supérieur à L, et des charges initiales proches de MAX. Pour le triangle de rang4, P7/L12 donne c=(4,8,12,12), p=(5,7,7,7), A=(0,4,8,8). Les injections à MAX qualifient les opérations à partir de compteurs fournis ; elles ne représentent pas un travail physique historique exécuté.

## Complétion et preuve du lien local vers run

Les 60 appels publics sont ceux de `build_silent_cofaces` **dans l'overlay privé** : deux catalogues K2, cinq caps L et six caps P. Le catalogue est construit par le juge rationnel à partir des points. Ces appels traversent réellement `Builder::run`, son cœur, ses chaînes, ses budgets et la purge de son wrapper ; ils ne constituent pas un run du pipeline produit ou d'une tour complète.

Sur le triangle, les trois arêtes du cœur donnent trois MEB de rang1. Sur CHAIN5, les cinq cofaces directes donnent le cœur AB, AC, AD, BC, BE, DE et la trace de huit MEB AB, ABD, BDE, AC, AD, BC, BE, DE. La coface silencieuse ABD, support AB, intérieur D et niveau16, est ajoutée avant le troisième MEB ; BDE est ensuite le terminal direct. Les douze cas CHAIN achevés par build rendent cet événement exact. Les caps trop courts entraînent un refus fermé et des événements purgés.

La [preuve statique du raccord](semantic_review.md), avec son [manifeste de pins](semantic_review.json), complète ce corpus. Work est un membre unique du Builder ; le repli appelle directement `miniball_reference` dans ce même objet, sans récursion. Le certificat local conserve le niveau et `support[0]`, donc le sommet retiré et l'étape suivante de descente. Comme le reste de `run` est littéral, une induction sur ses appels conserve la trace logique nominale, les événements et les refus F sur le domaine déclaré. Les preuves géométriques déjà closes sont consommées ; cette qualification ne rouvre pas les certificats horizontal, vertical ou du vote.

## Exceptions et fautes de raccord

Huit cas exceptionnels nominaux sont exécutés sur la copie instrumentée : après charge P ou après charge F, avec `bad_alloc` ou `runtime_error`, en appel local puis dans la chaîne publique. Les [cas injectés](compiled/fault_cases.json) et le [flux observé](compiled/instrumented_faults.stdout.gz) conservent cette attribution. Les calculs nominaux de proposition n'allouent pas ; les injections sont des coutures d'audit, sans prétendre simuler un calendrier naturel de panne identique à F.

En local, le résultat appartient à l'appelant : les charges payées sont observées après déroulement de pile. Dans le wrapper CHAIN, une coface existe avant chacune des exceptions injectées. `bad_alloc` est convertie en `silent_allocation_failure`, les événements sont purgés et les miroirs restent observables dans le résultat retourné. Pour `runtime_error`, le wrapper propage l'exception et **ne retourne aucun résultat interne**. Les deux cas publics correspondants qualifient la propagation et le hook attestant l'événement antérieur ; ils ne qualifient pas des compteurs intérieurs depuis un faux résultat retourné.

Les quatre copies fautives sont `reset_work`, `drop_P_mirror`, `drop_A_mirror` et `charge_after`. Elles traversent le vrai dispatcher candidat. Les deux suppressions de miroir sont également réfutées sur les cas d'exception : quatre rejets de raccord et deux rejets exceptionnels supplémentaires. Le juge exige les causes attendues, conservées dans les résultats de rejeu, et ne compte pas une erreur quelconque de lecture comme un mutant détecté. La [porte du lecteur](compiled/inspector_checks.py) passe aussi [normalement](compiled/inspector_checks_normal.json) et [sous Python optimisé](compiled/inspector_checks_optimized.json) : un positif et six rejets précis, dont une dépendance étrangère conservant la cardinalité vingt et un stdout mutant JSON invalide. Aucun brut n’est modifié par ces fautes injectées en mémoire. Ce sont des mutations de copies d'audit ; elles ne constituent pas l'enregistrement d'un nouveau mutant dans le produit.

## Rejeu et limites restantes

Depuis la racine du dépôt, le rejeu des bruts conservés ne compile et n'exécute aucun binaire :

```bash
python3 -B morsehgp3D_v7/audits/meb_builder_audit.py
python3 -B -O morsehgp3D_v7/audits/meb_builder_audit.py
```

Le [run.json](compiled/run.json) est l'autorité des exécutions historiques ; les deux résultats JSON sont l'autorité du jugement courant. Les contrôles de provenance et de cause peuvent être renforcés sur ces mêmes flux, avec conservation du driver de capture. Le répertoire garde les stdout/stderr compressés, les commandes, les depfiles et les patches propres à chaque variante. Aucun hash évolutif du driver de rejeu n'est figé dans le présent README.

La source produit reste F ; aucune option P, publication A/P, version de comptabilité CLI/archive ou politique de budget de tour n'est intégrée par ce reçu. P0 conserve les champs legacy sans prouver une identité d'instructions ou de coût. Cette campagne locale ne mesure ni performance, ni gain de tour, ni SLO ; les mesures antérieures du helper ne se transfèrent pas au coût du nouveau dispatcher et de ses miroirs. Aucun build global, moteur complet, GPU ou GCP n'est attribué. GCP non utilisé.
