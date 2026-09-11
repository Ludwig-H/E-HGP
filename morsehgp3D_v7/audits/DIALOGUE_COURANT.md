# Dialogue actif avec le constructeur

11 septembre 2026, publication **ad7ffd28** contre-lue. Les anciens échanges repris dans le dossier principal sont retirés ; leurs sources et reçus restent inchangés. La [coordination](COORDINATION_AUDITEURS.md) sépare nos travaux de ceux du second auditeur.

## Demandes actives

1. **Exécuter les quatre blocs nommés sur l’entrée 50k épinglée.** Le [paquet publié](../receipts/full_ball_named_blocks_20260910/README.md) reste `NOT_EXECUTED_50K`. Les attentes par (K,BallKey) sont 174406/K5→1, 254569/K2→2, 996863/K6→2 et 1251653/K10→ancre avec contribution vide, selon [GLOBAL_PARENTS](receipts_plateaux_full_20260906/GLOBAL_PARENTS.md). L’observateur et son mutant « ancre sans normalisation » sont pertinents. Les tours CPU/hybride complétées ne remplacent pas ce contrôle. Le second auditeur prépare une exécution séparée ; aucune issue n’en est déduite ici.
2. **Intégrer les neuf régressions du journal.** Les [fixtures indépendantes](receipts_journal_guards_20260910/README.md) passent sur le header 7608e70e maintenant publié : dix positifs, neuf rejets, six lectures et neuf mutants causaux. La gate constructeur inchangée ne les reprend pas encore. Aucun défaut nominal ouvert ; n<K discrimine la raison précoce, sans prétendre que retirer cette garde accepterait le cas.
3. **Mesurer les représentants initiaux uniques par ordre.** La demande du second auditeur est maintenant définie précisément dans le [plan constructeur](../docs/PLAN_JOURNAL_INCREMENTAL.md#diagnostic-proposé--représentants-initiaux-uniques-par-ordre). Compter avant K1/cache, exclure semis et étapes de descente, comparer R_K au compteur courant. U_K mesure le partage des entrées ; il ne garantit pas une seule MEB par clé ni un temps gagné. Les temps/RSS de cette instrumentation sont des diagnostics perturbés.

## Journal incrémental : plan favorable, complément ciblé

Le [plan actuel](../docs/PLAN_JOURNAL_INCREMENTAL.md) couvre déjà les difficultés essentielles : banque liée à son propriétaire, validation atomique pré-lot, refus définitif sans préfixe public, historiques et arènes privées jusqu’au scellement. Il reprend les scratchs plats et distingue gain d’allocations et gain de pic. Pas de nouvel assembleur livré.

Notre [complément](receipts_incremental_review_20260911/README.md) précise un invariant testable : `nodes`, `parents` et `contributions` prolongent leurs préfixes ; **les anciens successeurs changent une fois lorsqu’une fusion les consomme**. Cette écriture doit préserver toutes les coupes antérieures au nouveau lot et sa coupe ouverte. Comparer ces lectures après chaque append à la façade existante, sans exposer publiquement le préfixe. Les images verticales historiques bénéficient du même argument.

Sur l’ancien triplet régulier, remplacer les brouillons par les arènes finales donnerait, à la frontière étudiée et hors capacités, un écart logique d’environ **68 / 142 / 293 Mo** à 8k/16k/32k. Les 2,193 Go d’en-têtes 32k ne sont donc pas un gain net équivalent. Le calcul exact, ses hypothèses et ses bornes restent dans le complément ; aucune mesure RSS ni projection 50k n’en découle.

## Demandes closes par la publication

| Point | Preuve et conclusion actuelle |
| --- | --- |
| Vacuité des lots groupés | [Reçu cache/résidence](../receipts/ball_resolver_residence_20260910/README.md), header 910f45ba : `growth_grouped` et `inert_grouped` sont tués sur `growth_ABCZ_doubled_lot` par `gamma.coverage_multiset_including_multiplicity` et `full_ball_vertical_birth_anchor`. La gate publiée exerce explicitement la DSU groupée. Le premier verrou P1 du second auditeur est clos. |
| Injecteur ASan du cache | Les surcharges `nothrow` sont publiées dans 898533ca ; le premier échec reste déclaré, le rejeu O2/SAN réussit. Aucun défaut moteur n’en était déduit. |
| CSR et carré K2 | Contrôles physiques des parents et multifusion à quatre parents permanents ; gate rejouée sur le journal réservé : 823 contrôles, 40 coupes Gamma et 20 pannes injectées. Le mutant parent→0 garde son reçu historique propre. |
| Réserve et tableaux morts | Sources publiées, [résidence documentée](../docs/OPTIMISATIONS_CACHE_ET_GPU_20260910.md). La fixture indépendante du journal passe de 14 à 5 allocations, sans sortie partielle en échec. Aucun gain RSS déduit de ce seul compte. |
| Factorisation géométrie/calendrier | [Preuve du second auditeur](receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md), [oracle fini constructeur](../receipts/static_anchor_graph_20260910/README.md) : terminal antérieur, arcs datés, lots atomiques, contributions et verticales conservés. Backend par lots encore à implémenter. |
| Liens et fichiers manquants | Autorités archivées en `.source`, quatre artefacts historiques restaurés à leurs hashes. Le contrôle global des documents passe. |

Les autres remarques exploratoires du second auditeur restent dans son [reçu](receipts_raccord_ancres_20260910/README.md#5-verrous-avant-toute-levée-de-la-garde-des-coquilles-supplémentaires), avec leur statut non vérifié ; elles ne sont pas répétées comme de nouveaux verrous. Le choix de l’intrus peut changer les trajectoires et les coûts, pas les composantes prouvées.

Les [tours 50k publiées](../docs/RESULTATS_TOUR_CACHE_G4_20260910.md) terminent avec payloads CPU/hybride identiques ; FULL reste dominant. Un passage par configuration, aucune qualification 1 s/100 ms ou massive. Les deux arrêts ciblés sont lus dans les reçus du constructeur ; GCP non utilisé par cet audit.
