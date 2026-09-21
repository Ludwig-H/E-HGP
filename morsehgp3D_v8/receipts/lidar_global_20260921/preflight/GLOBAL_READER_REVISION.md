# Renforcement du lecteur global, sans rejeu des 48 mesures initiales

La première version de `run_wspd_q34_lidar.py` est [archivée exactement](global_runner_initial.py), SHA-256 `100b69f4a6f38cf60a3f2cd3a197b370d828f4c6eaf76cd37739d545073494db`, identique à l'empreinte du manifeste `global_vwtz76da`. Sa capture v1 contient 48 mesures natives ; elles n'ont pas été rejouées pour obtenir un résultat favorable.

Le lecteur actuel renforce cinq points : inventaires exacts de tous les champs et tableaux ; types entiers stricts, sans booléens assimilés à des nombres ; normalisation complète des records, clés primitives et digests ; reconstruction de toute la matrice depuis la commande de lancement avec vérification des entrées METADATA/SHA/FNV ; fermeture protégeant l'écriture du résultat même si une empreinte devient illisible. Les coûts de préparation, mémoire, parallélisme et sorties sont aussi inclus dans les rapports de croissance.

Il relit explicitement le schéma v1 comme historique : `historical_capture=true`, sans prétendre que les tests supplémentaires avaient déjà été exécutés lors de sa capture. Les lectures historiques normales et sous `-O` de `global_vwtz76da` passent, ainsi que les autotests **31 corruptions** dans chaque mode. L'ancien schéma ne reçoit pas silencieusement les champs de provenance ajoutés au nouveau.

Une seule nouvelle exécution native a ensuite été lancée pour vérifier le chemin v2 : [global_3ml5xuwx](../global_3ml5xuwx/COMPLETION.json), n64/K5/s8, un worker, backend28, records complets. Elle se ferme PASS avec 457 sorties, inventaire197. Les lectures `read --check-live --compact` et `selftest` passent normalement et sous `-O`, code 0 pour les quatre commandes, 31 corruptions refusées. Les captures v2 enregistrent désormais aussi branche/worktree, compilateur, matrice explicite, cwd/environnement des commandes et erreurs de fermeture.

Ces contrôles renforcent la preuve des reçus ; ils ne changent aucun des 196 fichiers de l'inventaire des arêtes et ne qualifient pas un gain de vitesse. Aucun grand benchmark global ni test GCP n'a été lancé par cette reprise du lecteur.
