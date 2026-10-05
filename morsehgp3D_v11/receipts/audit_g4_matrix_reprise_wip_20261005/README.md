# Reprise WIP : label LiDAR impossible dans gcc_release court

Lecture seule du WIP `build/v11-impl-l3`, matrice SHA-256 `da0735542a1f806160eeb8cdb853703ca0e9f288626d521c88ff3c2f0fc9747d`, capturée identique avant/après. Ce WIP n'est ni commis ni qualifié. Aucun build, test natif ou accès cloud.

`g4_matrix.json:24` exclut désormais `lidar` et tous les labels d'échelle de `gcc_release`. L'exigence conditionnelle `require_labels_if_data=["lidar"]` demeure aux lignes 33–35. Avec `data_complet`, le juge réel `g4_matrix.py:460–461,478–479` refuse donc cette configuration : aucune porte sélectionnée ne peut faire passer le label exigé.

La projection sur l'inventaire fina2 épinglé sélectionne 873 noms uniques, tous de labels `fast/oracle/unit`, aucun `lidar`. Le rejeu borné donne au juge le cas le plus favorable, **toutes ces portes supposées PASS** : il rend `floor_violated`, raison `aucune porte passee pour le(s) label(s) : lidar`. Retirer uniquement cette exigence conditionnelle du lot court rend le même témoin `ok`. Ce témoin construit vérifie la logique du juge ; ce n'est pas une campagne ni un journal natif du WIP.

Correction proposée : retirer `require_labels_if_data` de `gcc_release` court ; conserver les vraies portes LiDAR dans les lots d'échelle. Aucune modification du validateur n'est nécessaire.

`proof.json` ancre l'état Git, la matrice, le juge et l'archive/inventaire. `matrix_snapshot.json` conserve seulement la configuration, sans données LiDAR. Rejeu stdlib local : `python3 -B replay.py`, puis `python3 -O -B replay.py`. Il requiert l'objet Git et l'archive locale fina2 ; il ne lance aucun calcul produit. Les autres reprises, délais et corrections du banc ne sont pas qualifiés par cette preuve.
