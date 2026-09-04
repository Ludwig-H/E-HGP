# Qualification courante de C et mesures mono

4 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le CLI C reconstruit indépendamment par l'auditeur est identique au binaire mesuré et requalifié par le constructeur : SHA-256 `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`. La qualification locale d'AxisBounds et les replays d'interface ferment la réserve d'exécution de ce delta. Les reçus de la suite complète C et de la campagne B/C sont cohérents après vérification indépendante ; leurs exécutions restent attribuées au constructeur.

## Exécutions de l'auditeur

Les [sources figées](receipts_iteration3/axis_source.json) et la [construction Release](receipts_iteration3/axis_execution.json) rattachent le CLI et la porte AxisBounds au worktree lu. Les six [portes census](CENSUS_AXIS_COURANT.md) passent, dont cinq mutants avec divergence identifiée. Le CLI repasse les 26 scènes et six corruptions rescellées des [interfaces](AUDIT_INTERFACES_20260904.md). Aucun drift des sources consommées ni des binaires n'est observé dans ces reçus.

La suite complète exécutée indépendamment reste celle de 279 portes sur le snapshot initial. Les quatre portes archive et les quatre portes mono ont leurs qualifications ciblées propres. Il n'y a pas de seconde exécution indépendante des 292 portes C dans cette itération.

## Vérification des reçus du constructeur

Le [reçu de contrelecture](receipts_iteration3/constructor_receipts_review.json) conserve les recalculs :

- Release C : 46 fichiers conformes en hash et taille au manifeste, JUnit de 292 noms uniques sans échec ni skip, inventaire égal à la synthèse, sources et binaires identiques avant/après. Le CLI testé a le même hash que celui reconstruit ici.
- Campagne mono : 22 fichiers bruts conformes au manifeste ; six processus terminés en code 0 ; chaînes de dix digests recalculées depuis les sorties ; cardinalités et digests identiques pour les trois paires et entre séparations.

La campagne porte sur un seul nuage uniforme de 8 000 points, coordonnées bornées à 200, seed 3, tour K=1..10, CSR, `threads=1`, `fold_inflight=1`, jonction entre ordres, digest inclus et affinité CPU 6. L'objet est `verified_events_only`, sans archive ni complétion silencieuse. B est le moteur précédant les deux changements mono/AxisBounds ; C les combine. Ces comparaisons ne séparent donc pas causalement leurs contributions individuelles.

| Séparation | Temps externe B (s) | Temps externe C (s) | Réduction observée |
| --- | --- | --- | --- |
| 8 | 127,997 | 105,932 | 17,24 % |
| 10 | 125,524 | 105,594 | 15,88 % |
| 12 | 126,527 | 105,112 | 16,93 % |

Les durées sont celles du processus complet mesuré par le banc, digest compris. Une seule paire par séparation, sans répétitions statistiques, échauffement ni intervalle de confiance : ce tableau est une observation utile, pas une qualification de p95 ou du contrat 50k. Le digest commun est `f57157901b2a20282f5c418e48a376f3e33dab0047328020eb3f9846408974c7`.

Le [bilan du constructeur](../docs/RESULTATS_MONO_20260904.md) borne correctement ces résultats. La génération et le fold dominent désormais le census dans les profils : environ 60 s et 23 s contre 17 s. Sur ce cas, augmenter la séparation change peu la durée totale ; cette seule entrée ne motive pas un changement de défaut. La prochaine mesure utile doit porter sur les structures réellement dominantes et sur l'objet avec complétion si c'est celui livré.

## CI et suite constructive

Le premier run GitHub de `d9e4ee01` compte un échec sur 292. L'échec vient du test de sonde sous `LD_LIBRARY_PATH` hérité. Le [correctif local du harnais](SONDE_CI_COURANTE.md) passe indépendamment ses 23 scènes en Python normal et optimisé ; lanceur et agrégateur sont inchangés et le refus d'environnement injecté reste effectif. Aucun nouveau résultat GitHub vert n'est attesté ici.

Les preuves [lanes](ARITHMETIQUE_LANES_COURANTE.md) et [entiers larges](ARITHMETIQUE_LARGE_COURANTE.md) ferment les bornes locales sous leurs préconditions. Les prochaines petites portes doivent rattacher ces clauses au code compilé et isoler chaque site de mutant. L'index/front, l'environnement numérique, la composition du binaire livré, la verticale éventuelle et les coûts de la route complétée conservent leurs qualifications propres.

Le README scellé du reçu Release C décrit sa préparation initiale. Pour l'état terminé, utiliser sa [synthèse scellée](../receipts/release_c_20260904/summary.json) ; ne pas modifier rétroactivement les octets du reçu fermé pour actualiser ce texte initial.

GCP non utilisé par l'auditeur. Les activités distantes du constructeur ne sont ni exécutées ni certifiées dans cette note.
