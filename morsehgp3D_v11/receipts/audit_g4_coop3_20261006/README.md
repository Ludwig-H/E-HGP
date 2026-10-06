# Coop3 : retour de la boucle explicite, session close

Lecture locale seulement ; aucun build, natif, calcul HGP, payload LiDAR ou accès cloud par l'auditeur.

Session `v11.20261006.claudecoop3`, pin `9eee2ed4bcef1e960cdf2456012b84416854dc20`. DONE0, completed/stopped, worker0, génération finale identique à la génération démarrée et arrêt ciblé certifié ; les clés/verrou ont été libérés selon le reçu. Archive SHA256 `ad2fa6444b63463f255038b647be0e1cde18c5c7b98ef9459de7cadf35dda68a`. Le rejeu contrôle reçu/plan/paquet/archive, manifeste des résultats et **639 fichiers utiles du paquet identiques aux blobs Git**. Le plan vise le retour de `extend` explicite, avec référence GPU un fil proche de L4 ; aucun nouveau noyau de feuille cohérente n'est joué.

Deux commandes closes code0 : K5/feuille16, 5 prises froides et12 passes chaudes ; K10/feuille24, 3 prises froides et8 passes chaudes. Trois trames sans sol entières, profils u21 Release CUDA, W48 ; CPU16379/GPU81915/GPU-coop212987. Un binaire CUDA12.9.41 construit puis réutilisé, SHA `6cac3eadcc495e42eeb9f3a83bbddab70461c7896c0090538677c6c0979214e5`. Les métadonnées d'entrée et leurs hashes sont vérifiés, aucun octet d'entrée n'est lu ni copié.

**90 dumps conservés conformes**, 72 prises froides+18 derniers dumps chauds. Le juge `gpu_ab.py` épinglé compare également le registre complet du catalogue au CPU et ses listes de refus sont vides. Les registres individuels ne sont pas conservés dans les reports : preuve par source du juge+verdict clos, pas relecture de lignes natives absentes. Les 252 constructions FULL portent un statut réussi ; **162 passes chaudes intermédiaires ne sérialisent ni dump ni registre**. Les90 identités individuelles, hashes des références de registre et bornes brutes passes2..P figurent dans summary.json.

Temps du **dernier appel chaud GPU un fil**, en ms. Coop2 et3 portent les mêmes données, W48, K, feuille, masques et nombres de jobs/fill/copies. Le seul delta produit entre ee3eabe5e et9eee est `leaf_device.hpp` : restauration du corps explicite de `extend`. Comparaison de deux observations closes, sans médiane ni promesse de gain FULL.

| Cas | Fill coop2 | Fill coop3 | Exécuteur coop2 | Exécuteur coop3 |
| --- | ---: | ---: | ---: | ---: |
| K5 lidar_ng00 | 34.657 | 17.438 | 75.572 | 58.685 |
| K5 lidar_ng01 | 40.968 | 19.665 | 74.669 | 53.813 |
| K5 lidar_ng02 | 17.646 | 12.605 | 55.929 | 51.877 |
| K10 lidar_ng00 | 232.102 | 111.697 | 455.812 | 341.822 |
| K10 lidar_ng01 | 238.811 | 116.204 | 419.183 | 301.348 |
| K10 lidar_ng02 | 201.161 | 105.564 | 407.938 | 321.199 |

Les bornes brutes de fill GPU, passes2..P : K5 ng00 17.426–17.452ms, ng01 19.665–19.707ms, ng02 12.582–12.633ms ; K10 ng00 111.346–111.786ms, ng01 116.174–116.532ms, ng02 104.636–105.564ms. La régression du remplissage un fil a disparu sur ces observations. Les mesures ne livrent aucun contrat100ms ; une durée fill seule n'est pas une durée FULL. Les vrais temps wall/domain/forest sont conservés séparément dans summary.json, sans résumé par médiane.

Portée : pas de sanitizer, unité, mutant, Nsight ou porte de la nouvelle feuille cohérente dans ce plan. La garde CUDA ordinaire unresolved/atomicExch déjà signalée reste à corriger ; ces mesures ne la clôturent pas. Le WIP arbre couvrant et MHGP11SPv2 en cours est exclu du pin joué.

Rejeu en lecture seule depuis ce dossier : `python3 -B replay.py --check summary.json`, puis `python3 -O -B replay.py --check summary.json`. Le script lit la session locale et Git ; il compare le JSON figé sans l'écraser. Pour déplacer le dépôt/session, options `--repo` et `--session`. Manifeste SHA256SUMS de la capsule distinct du manifeste des résultats G4.
