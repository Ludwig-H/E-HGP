# B — provenance et fermeture de la session

8 octobre 2026. Session `v12.20261008.t2db`, lancée à 08:08:52 UTC, revenue avec
`completed`, worker 0, DONE 0, zéro erreur. Archive 461057 octets SHA `e54ce9e3…8c8e7c` :
357 entrées de manifeste contrôlées, 272 journaux JSONL. Arrêt ciblé certifié, code 0 en une tentative,
RUNNING→TERMINATED, garde invitée intacte et réserve libérée. Les groupes des quatre commandes
sont clos, sans processus résiduel tué ni flux tronqué. Rejeu local normal et −O identique.

| Commande du plan | Code | Mur du processus, s | Délai, s | Portes primaires |
| --- | ---: | ---: | ---: | --- |
| socle_ctest | 0 | 141,336 | 900 | 724/724 Passed, aucun skip |
| t2d_b_pilote | 0 | 581,939 | 1500 | rapport et 272 journaux ; admission séparée |
| lidar_ctest | 0 | 763,894 | 900 | 7/7 Passed, dont `mhgp12_tower_chain_m0` |
| mutants_index_num_tour | 0 | 269,801 | 1200 | 3/3 Passed : index, num, tower |

Ces murs sont ceux des **commandes**, pas des trames HGP. Les résultats individuels de mutants ne
sont pas détaillés dans ces stdout de CTest. Les portes tournent sur le paquet 41d4 ; le succès LiDAR
est une nouvelle preuve au code B livré, sans réécrire le timeout 180 s de l'ancienne session A.

La [préparation déjà publiée](../session_b_protocole/README.md) est confirmée : paquet source
`41d4d828b70ad033f8bf392aca20a4af13cea242`, SHA `dc717c43…c843ad5`,
plan `74686743…716711c`, archive avant **réelle** 902 SHA `0f91cda2…67045`.
Relecture de 342 sources exactes à Git 902 et 357 à Git 41d4 (`src/bench/tests/cmake/CMakeLists.txt`).
Le pilote f374a89b, le manifeste des bras 3999a137 et le lecteur FULL 3594c5d3 sont aussi identiques
entre objets Git et paquet. Les sources vivantes ultérieures ne sont pas utilisées.

Le pilote reconstruit les huit bras depuis 902 : `avant`, `avant_bis` (même binaire), `garde` (L1),
`report` (L2), `temoins` (L3), `census` (L1+L2+L3), `proposition` (L4) et `apres` (L1+L2+L3+L4).
Les dix fichiers natifs du lot après sont annoncés conformes au produit 41d4, sans écart ; la relecture
des postimages est dans la préparation et le [raccord B](../t2db_raccord/README.md).
Les huit SHA de binaires déclarés sont identiques à l'ouverture et à la fermeture ; les quatre SHA
des binaires informatifs sont conservés. Aucun ELF n'est rapatrié : ces SHA restent des déclarations
liées aux journaux de construction et de campagne, pas un rehachage local des exécutables.

**Le paquet courant n'est pas le bras après chronométré.** Les bras 902 et 902+B partagent le même
ancien pool et le même catalogue, sans A ni C nouveau. G est CPU : `tower_probe.cpp` au pin902
construit le catalogue une fois avant les passes (l270–276), puis mesure `resolve_tower` seul.
Le FULL informatif est séquentiel sur catalogue CUDA, avec verticales et registre ; validation,
empreinte et libération sont hors mur. Ce FULL n'est pas la Session recouverte courante.
Un retrait ultérieur de L4 ne transforme pas automatiquement les temps du lot après en temps du
nouveau produit ; le bras `census` est une preuve distincte à comparer explicitement.

Le rapport déclare le lot, report, témoins et census adoptés, garde seule et proposition rejetées,
sans refus de campagne. Le présent lecteur contrôle cette déclaration et sa provenance ; le
[recalcul indépendant des admissions, statistiques et verdicts](../session_b_admission/README.md)
porte les chronos utilisables. Il distingue aussi les compteurs K5 des informations K10.

`capture.json` contient uniquement les champs publics nécessaires, hashes et tailles. Aucun contenu
de scène, identité de compte, commande de récupération ni reçu contrôleur brut n'est copié ici.
Les noms/tailles/hashes des données sont des déclarations épinglées ; leurs payloads n'ont pas été lus.

```sh
python3 check.py --repo DEPOT --session SESSION_B --before-sources ARCHIVE_SOURCE_902
python3 -O check.py --repo DEPOT --session SESSION_B --before-sources ARCHIVE_SOURCE_902
```

Le lecteur réutilise les deux contrôleurs d'archive/source publiés et épinglés, ainsi que la capture
immuable de préparation. Tous les hashes des fichiers locaux sont revérifiés en fin de lecture.
L'audit n'a lancé aucun moteur, build ni appel cloud.
