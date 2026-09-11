# Dialogue actif avec le constructeur

11 septembre 2026, publication **ce842a3f** contre-lue. Les sources et reçus historiques restent inchangés. La [coordination](COORDINATION_AUDITEURS.md) sépare nos travaux de ceux du second auditeur.

## Demandes actives

**Qualifier le raccord FULL du journal lorsqu’il est intégré.** Les [gardes](receipts_journal_guards_20260910/README.md), [préfixes indépendants](receipts_incremental_prefix_20260911/README.md) et [104 cas appariés constructeur](../receipts/incremental_journal_prototype_20260911/README.md) sont acquis sur leurs octets épinglés. Le prototype reprend aussi les gardes indépendantes ; inutile de les redemander comme tests absents. Restent la consommation par le vrai producteur, les verticales et les coûts de toute la tour, conformément au plan constructeur.

## Deux raccourcis géométriques à distinguer

Notre [complément statique](receipts_static_followup_20260911/README.md) prouve qu’un hit de **population complète** après échange donne la même BallId que la MEB suivante, avec rayon strictement décroissant. La table de semis existe déjà ; la fixture cinq points permet de qualifier un compteur d’observation avant activation. Aucun nombre de hits ni gain temporel n’est encore mesuré.

À la question du constructeur sur un support survivant : un témoin positif entièrement conservé certifie au contraire un **rayon égal**. Le support choisi courant contient le site retiré ; il faut donc un autre témoin et des slots actualisés. Le complément précise le critère exact, les coûts et l’ordre canonique à garder pour préserver la trajectoire. Ces deux optimisations ne doivent pas partager un prédicat approximatif.

## Demandes closes et preuves à conserver

| Point | Preuve et conclusion actuelle |
| --- | --- |
| Quatre blocs nommés 50k | [Rejeu indépendant du second auditeur, 223a3897](receipts_cache_commit_20260911/README.md#7-quatre-blocs-nommés-50k-second-verrou-p1--exécution-cpu-locale), reprise CPU réussie et mutant tué ; trois parents pré-lot et ancre K10 conformes. Premier SIGTERM conservé ; ni complétude ni arité finale ni contrat de vitesse. |
| Représentants initiaux R/U | [Diagnostic livré](../receipts/initial_representatives_20260911/README.md), instrument placé avant K1/cache, quatre mutants, uniforme 8k et scanline non régulière. Demande satisfaite ; R/U n’est pas un facteur de vitesse. |
| Résolution statique et échelle | [Route optionnelle publiée](../docs/RESOLUTION_STATIQUE_CPU_20260911.md), O2/SAN, six mutants, 24 CTests, triplet 8k/16k/32k et s8/10/12. Les sources et compteurs sont contre-vérifiés, sans gain de latence attribué sur hôte partagé. |
| Exception après admission | [O2 constructeur](../receipts/static_worker_failure_20260911/README.md), puis [nouveau SAN indépendant](receipts_static_followup_20260911/README.md) sur le même binaire. Deux pannes, sortie vide, jointures et réutilisation nominale ; le premier échec ptrace reste inchangé. |
| Préfixes du journal | Notre gate indépendante et l’adaptation constructeur du cas mixte sont distinctes et favorables. Le [calcul net de résidence](receipts_incremental_review_20260911/README.md) reste une borne logique historique, pas un gain RSS. |
| Anciennes corrections cache/journal | Lots groupés, injecteur `nothrow`, CSR/carré K2, réservations et restauration des preuves sont acquis depuis ad7ffd28 ; détails repris dans les [documents constructeur](../docs/OPTIMISATIONS_CACHE_ET_GPU_20260910.md). |

La [contrelecture ad7ffd28 du second auditeur](receipts_cache_commit_20260911/README.md#8-contre-lecture-adversariale-des-onze-surfaces-aucun-p1) rassemble les remarques secondaires. Son point principal restant est le jugement du vrai raccord census→tour à K9/K10 ; ses constats antérieurs à ce842a3f demandent confrontation avec cette livraison avant reprise. Le choix de l’intrus peut changer les trajectoires et les coûts, pas les composantes prouvées.

Les [tours 50k publiées](../docs/RESULTATS_TOUR_CACHE_G4_20260910.md) terminent avec payloads CPU/hybride identiques ; FULL reste dominant. Un passage par configuration, aucune qualification 1 s/100 ms ou massive. Les deux arrêts ciblés sont lus dans les reçus du constructeur ; GCP non utilisé par cet audit.
