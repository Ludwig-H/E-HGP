# FULL mono : campagne scellée et levier d’alias

5 septembre 2026. Source publiée `98bb6578`, producteur EAGER `e02d163c`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans ce dossier.

**Les cinq tentatives sont closes et leurs reçus concordent : trois réussites relatives à 8k, deux refus explicites du budget d’alias à 16k/K9 et 32k/K7.** La résidence des alias possède maintenant une décomposition exacte et une borne constructive pour le cache proposé. Quatre lacunes du juge de cohérence sont rendues reproductibles ; aucune des nouvelles identités ne réfute les observations nominales. Aucun nouveau moteur ni benchmark n’est lancé par cet audit.

## Campagne réellement qualifiée

La [contre-vérification des paquets](receipts_full_mono_20260905/constructor_receipt_review.md) lie les sources, dépendances, commandes et captures scellées au binaire `d6126f77`. Les 51 pins source avant/après des admissions micro et de la campagne mono concordent ; les 39 dépendances utilisateur du compilateur sont contrôlées. L’admission micro est distincte : une construction, six positifs n=8 et six refus de parsing, soit treize commandes. La campagne ci-dessous est CPU Release, un thread, CPU6, nuage uniforme déterministe de coordonnées u16, seed 3, Kmax=10.

| Tentative | Durée avant terminal | Pic du processus GNU time | Issue |
| --- | --- | --- | --- |
| 8k, s=8 | 150,776 s | 1 794,563 MiB | Dix ordres réussis, code 0 |
| 16k, s=8 | 275,497 s | 2 825,445 MiB | Refus `full_gabriel_alias_budget` à K9, code 2 |
| 32k, s=8 | 464,273 s | 5 009,883 MiB | Même refus à K7, code 2 |
| 8k, s=10 | 150,879 s | 1 789,051 MiB | Dix ordres réussis, code 0 |
| 8k, s=12 | 151,795 s | 1 790,164 MiB | Dix ordres réussis, code 0 |

Aucun timeout. Les quatorze ordres réussis avant les deux refus restent diagnostiques : aucune tentative complète n’en est déduite. La sonde partage les catalogues adjacents, lit la couverture terminale puis détruit chaque forêt avant l’ordre suivant. Elle ne conserve ni archive de tour ni verticale, et n’imprime aucun digest de forêt ou d’entrée. Les volumes et compteurs égaux des trois réussites 8k ne prouvent donc pas l’égalité de leurs objets. Les mesures F du [mono réduit](MONO_COURANT.md) portent un autre payload ; elles ne constituent pas un bras apparié FULL.

## Décomposition des alias et borne exploitable

La [preuve de résidence](receipts_full_mono_20260905/memory_model_review.md) porte sur le producteur EAGER capturé. Pour un ordre entièrement réussi, noter L les minima, D les directes de connexion, T les visites de facettes, V les requêtes de portail et A les alias. Le nombre Q de demandes strictes et celui E de facettes égales nouvellement installées vérifient :

$$Q=T-(K+1)D,\qquad E=(K+1)D-Q,\qquad A=L+E+V,\qquad Q\leq\min(4,K+1)D.$$

L’unicité de la MEB et du point intérieur retiré rend les E facettes égales distinctes. Elles ne sont ni des minima ni des portails antérieurs. Cette identité donne une attribution exacte des clés, sans estimation de taille d’un conteneur. Elle ne s’applique pas aux lignes refusées : leurs catalogues sont complets mais leurs visites sont interrompues.

À **8k/s8/K10**, A=6 209 024 se décompose en **600 806 minima, 5 349 726 facettes égales et 258 492 facettes de portail**. Les égalités préinstallées représentent 86,16 % des clés. Le cache facultatif des seules facettes strictes non minimales demandées est contenu dans l’ancienne table et compte au plus `min(A−L,Q)` entrées. Ici Q=2 534 359 : minima et cache réunis demandent donc au plus **3 135 165 clés**, soit au moins **3 073 859 clés de moins**. Les ancres des directes et leurs successeurs restent nécessaires. Ces nombres ne mesurent aucun gain de RAM ou de temps.

Les catalogues complets permettent aussi une admission avant la construction interrompue :

| Premier ordre refusé EAGER | Directes D | Borne du cache seul 4D | Minima séparés |
| --- | --- | --- | --- |
| 16k/K9 | 1 265 886 | 5 063 544 | 1 036 033 |
| 32k/K7 | 1 714 020 | 6 856 080 | 1 326 831 |

**Un cache de huit millions suffit à retenir toutes les demandes strictes de chacun de ces deux ordres**, si les minima sont stockés séparément et les catalogues restent identiques. La borne ne promet ni l’admission des autres budgets, ni une réussite géométrique du port, ni les ordres suivants. Le champ actuel `KCount.incidences` compte (K+1)D ; un compte séparé de la somme des tailles de support donnerait Q, plus précis, sans construire Gamma.

La [preuve J1 déjà acquise](receipts_full_producer_20260905/lazy_alias_next_step_review.md) explique le coût déplacé : une facette égale demandée plus tard se résout via sa directe F+z. Si **chaque** stricte résolue est retenue, sans éviction ni insertion sautée, et que calendrier et choix géométriques sont conservés, les demandes J≥2 et les étapes de descente restent celles d’EAGER. Les nouveaux appels MEB sont alors exactement les nouveaux misses J1. Cette égalité est maintenant vérifiée sur les 218 comparaisons sans skip du [port qualifié](CACHE_FULL_COURANT.md) ; elle n’est pas exigible avec cache zéro ou saturé.

## Résidence et temps : deux leviers distincts

À 8k/s8, la génération coûte 61,434 s et la construction FULL 68,518 s sur les 150,776 s observées. Même en remplaçant fictivement tout ce dernier coût par zéro, avec le reste strictement fixé, il demeure 82,259 s : le plafond algébrique d’accélération de cette seule substitution est 1,833. Ce diagnostic ne prédit pas le temps du cache et ne s’extrapole pas à un autre n. Génération, census et construction FULL gardent chacun un levier propre.

Les trois arènes logiques de sortie des dix ordres 8k totalisent 392 110 328 octets pour 2 404 646 minima, 3 976 472 nœuds et 3 976 462 références parentales. Cette somme décrit ce qu’occuperaient leurs éléments conservés ensemble, hors capacités et suppléments ; la sonde les détruit successivement. Les 3 113 381 boules du census représentent séparément 697 397 344 octets logiques. Ne pas additionner ces nombres au pic mesuré comme s’ils étaient des allocations absentes de celui-ci.

Le Builder est déjà détruit à l’impression de chaque ligne ; en succès, la forêt et le catalogue de minima le sont aussi. Le RSS échantillonné ne mesure donc pas le pic du dictionnaire, et le HWM cumulé ne l’attribue pas à une phase. Une mesure du prochain port doit relever les tailles et capacités pendant la vie des propriétaires et conserver le pic global distinctement. La [note de résidence](receipts_full_mono_20260905/memory_model_review.md) décrit aussi les copies de finalisation, le scratch de lot, la couverture et le partage des catalogues.

## Quatre corrections précises du juge

Les [rejeux indépendants](receipts_full_mono_20260905/judge_review.md) passent normalement et sous `-O` : cinq reçus, neuf mutations de données des selftests rejetées, 44 lignes réussies contrôlées. Quatre corruptions coordonnées du brut et de son miroir sont acceptées par le juge capturé : minima K1=1, appels MEB et miroir tous deux effacés, un alias supplémentaire sous plafond, durée terminale nulle malgré des étapes positives. Les relations suivantes les refusent :

- `minima = n` à K1, sinon cardinal du catalogue de minima ; avec une racine et seulement de vraies multifusions, `nodes ≤ 2 × minima − 1` ;
- `meb_calls = geometry_meb_calls = portal_requests + chain_steps` sur un ordre réussi ;
- identité EAGER des alias démontrée plus haut, réservée à cette politique et au succès ;
- inclusion des chronomètres d’étapes dans le temps total et cohérence de la soustraction du temps de sortie, avec tolérance d’impression déclarée.

Ces corruptions violeraient les hashes du paquet historique ; elles établissent une lacune du cœur du juge, aucun contournement du scellement complet ni défaut des cinq runs réels. Ces corrections sont désormais contre-vérifiées dans le [juge v2](CACHE_FULL_COURANT.md), avec ses propres reçus et selftests ; le juge v1 capturé reste intact. Les [contre-fixtures exécutables](receipts_full_mono_20260905/judge_runs/replay.py) et les nouvelles identités restent permanentes côté audit.

## État du chantier suivant

La présente qualification reste attachée à **`98bb6578` et au header e02d**. Le [port lazy `13c6cc72`](CACHE_FULL_COURANT.md) a maintenant sa qualification propre sur 109 ordres : J1, deux pas, cache nul/petit/suffisant, ancres muettes et lots simultanés. La nouvelle sonde avec digest est admise séparément sur 24 reçus à n=8 ; les quatre corrections du juge v1 et le supplément first-C sont contre-vérifiés.

La comparaison de coût doit employer les deux bras avec le même instrument, digest compris. Les anciens temps sans digest restent historiques ; les observations 8k ne prolongent pas le petit oracle géométrique jusqu’à K10. Les grandes campagnes concurrentes, leurs interruptions et reprises, restent hors du présent verdict. Le [dialogue actif](DIALOGUE_COURANT.md) porte la suite. GCP non utilisé.
