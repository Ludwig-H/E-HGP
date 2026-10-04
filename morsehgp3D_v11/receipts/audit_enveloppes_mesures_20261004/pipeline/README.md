# Contrelecture des diagnostics du pipeline e49

Pin produit : `e49ea46907e3c4bb9e0942e2205cf490748136e4` ; lecture depuis les copies Git de `source/`. HEAD auteur au départ : `66372e621dcee58daaa7d7309875ab157894acf4`. Lecture statique et modèle scalaire/Python uniquement ; aucune compilation, exécution C++, campagne G4 ou qualification nouvelle. Le diff Git a été consulté avant la copie ; la preuve et le replay utilisent ensuite ces copies figées.

## Constat matériel : le lecteur exige un ordre de démarrage inexistant

`source/bench/full_acceleration_diagnostics.py:71–72` exige `max(start_lanes) <= min(finish_lanes)`. Le pool réclame puis exécute immédiatement chaque tranche (`source/src/sched/pool.cpp:55–64,99–108`), sans barrière de démarrage. Une voie peut prendre tous les blocs, finir, puis d'autres voies sans travail démarrer (`source/src/tower/forest_pipeline.cpp:53–79,102–110,199`). Le même problème est possible si une voie déjà démarrée est retardée avant son écriture de start. L'ordre des réclamations ne force donc pas celui des lectures d'horloge.

Le replay appelle le **vrai `validate` capturé**, avec deux voies démarrant à 1 et 10 ns et finissant à 5 et 11 ns. Chaque tâche finit après son propre départ ; le lecteur refuse uniquement l'ordre global faux. Une copie AST qui remplace cette seule condition par deux bornes `<= forest_ns` accepte le même événement et conserve les refus sur types, limites, ordre 1 et diagnostics nuls de la voie étagée. Il s'agit de métadonnées consommées par ce validateur et d'un ordonnancement scalaire admissible, **pas d'une sortie native ni d'une mesure réelle**. Vingt ordonnancements bornés montrent le même mécanisme, avec 0/1/2/40 blocs et 2/3/5/39/47 voies.

Correction proposée au développeur : supprimer cette relation globale ; garder les bornes individuelles sur `forest_ns`. La propriété `start[t] <= finish[t]` demanderait les fins individuelles, actuellement non exportées. Une porte de lecteur doit accepter la chronologie max-start 10 / première-fin 5. Une porte native future sur petit nuage/K2, plusieurs fils et très peu de blocs vérifierait le raccord sans imposer de simultanéité.

## Ce que les nouveaux temps permettent d'interpréter

`forest_pipeline.cpp:204–237` définit R = dernière fin de résolution et P = max(R, dernières fins de publication), puis les phases disjointes R, P−R, V−P. Ce sont des délais muraux depuis l'origine du pipeline, pas des sommes de travail CPU. La préparation des blocs, du tri et des balayages précède l'origine (`:139–198`) ; les réductions de ledgers et les acquittements du pool viennent après les tâches. Ils restent dans le mur englobant FULL.

La queue d'un publieur est `max(0,E−R)` et celle d'une verticale `max(0,E−P)` (`:217–226`). Pour une queue positive, on reconstruit la durée complète : publication `R+tail−start`, verticale `R+publish_phase+tail−start`. Pour une queue nulle, plusieurs fins E donnent la même sortie : CPU/wait portent sur toute la tâche, mais sa durée complète est perdue. Le replay contient deux fins 7 et 19, départ 2, R=20 : même queue zéro, durées 5 et 17. Exporter `finish_ns` ou `elapsed_ns` par tâche serait le complément minimal pour calculer partout le temps de service et distinguer démarrage tardif d'une fin tardive.

L'attente est le mur autour de l'appel `atomic::wait` (`forest_concurrent.cpp:69–74`, `forest_internal.hpp:81–89`), qui peut aussi retourner immédiatement après un changement déjà intervenu. Elle contient l'administration CPU de l'attente et le délai avant reprise du fil ; elle n'est pas un temps exact de sommeil futex. Les sommes CPU et wait ne forment donc pas une partition exacte du mur. Ces champs localisent une attente ou un départ tardif ; seuls, ils ne attribuent pas exclusivement une lenteur au SMT, à l'ordonnanceur ou au débit d'instructions du publieur. Si l'horloge CPU échoue, zéro n'est pas distingué d'une vraie durée nulle ; la capsule ne transforme pas cette limite de disponibilité en défaut géométrique démontré.

## Points favorables et portée

- Chaque tâche écrit sa propre case start/finish/cpu/wait (`forest_pipeline.cpp:44–47,87,99,102–110`). Les vues haute et basse d'un balayage partagent une case wait, mais les appels sont séquentiels dans cette même tâche (`forest_vertical.cpp:176–190`). L'origine est immuable. Le pool attend tous les ouvriers avant réduction et lecture (`pool.cpp:105–112`). Aucune nouvelle course n'a été identifiée par cette lecture ; il ne s'agit pas d'un test TSan.
- Les pointeurs temporaires du publieur sont retirés avant son retour (`forest_pipeline.cpp:92`), et les buffers locaux vivent jusqu'après le retour du pool. L'attente après abandon garde la correction publiée antérieurement (`forest_internal.hpp:96–102`, `forest_vertical.cpp:190`). Les décisions de la forêt ne lisent aucun diagnostic.
- Les trois tableaux ajoutés sont budgétés **avant** toute tâche (`forest_pipeline.cpp:146,152–167`) : supplément exact `24*T` octets, avec T = L+2K−1. Pour W48/K5 et 39 voies, T=48, donc 1 152 octets. Ils sont alloués même si `timings == nullptr`, ce qui change légitimement l'admission et le nombre de points d'injection mémoire, pas les compteurs logiques. Les allocations sont libérées par RAII sur refus ; la formule inclut les quatre tableaux, pas un seul.
- Les réductions de ledgers restent après `parallel_for` réussi et contrôlées (`forest_pipeline.cpp:199–203`). Les forêts ne sont transférées qu'après réussite du pipeline (`forest_concurrent.cpp:269–273`). Les diagnostics ne sont pas des octets canoniques.

Les portes natives présentes testent l'équivalence de forêts et ledgers, mais la boucle `forest_pipeline_test.cpp:132–169` ne vérifie pas cette relation de démarrage et ne qualifie pas à elle seule les nouvelles métadonnées. L'annonce 77/77 locale et l'identité ng00 du commit sont conservées comme déclarations, non rejouées ici. Aucune part du coût LiDAR ni gain natif n'est inférée par la capsule.

## Replay et fermeture

Depuis ce dossier :

```sh
python3 -B -S check_pipeline_timing.py
python3 -O -B -S check_pipeline_timing.py
python3 -B -S verify.py
python3 -O -B -S verify.py
```

Résultats : `checks.normal.json` et `checks.optimized.json`, identiques octet pour octet, **97 gardes**, 20 ordonnancements, stderr vide. `BEFORE.json`/`AFTER.json` distinguent blobs Git et LIVE. Le seul fichier différent entre pin et LIVE initial était la note de contexte du coordinateur ; aucun constat de code n'en dépend. La fermeture inventorie tous les fichiers réguliers sauf le seul `SHA256SUMS` racine ; le lecteur rejoue les contrôles sans écrire ni modifier une source.
