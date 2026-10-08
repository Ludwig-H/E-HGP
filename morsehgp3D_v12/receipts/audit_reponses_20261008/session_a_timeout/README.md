# Session A : portée du timeout LiDAR

8 octobre 2026, source **5f5c0c83fcb7df842996f584e3870bfbf3017d89**. Lecture seule des sources et métadonnées ;
aucun moteur/GCP. [Provenance et arrêt](../session_a_provenance/README.md) restent séparés.

Le plan lance `ctest --no-tests=error -L lidar --output-on-failure -j 6` avec **180 s pour toute la commande**.
Les métadonnées primaires attestent `timeout`, code **124**, 180,003 s, groupe fermé après destruction résiduelle.
Le log contient **six Passed**, puis `Start mhgp12_tower_chain_m0`, sans verdict final de cette porte.

Les portes LiDAR sont `RUN_SERIAL`, malgré `-j 6`. Les six durées affichées totalisent **49,24 s** : environ
131 s restent nominalement pour la dernière porte, avant transitions et arrondis. Ce n'est pas son temps mesuré.
Sa limite CTest déclarée est **7 200 s** (`tests/tower/tests.cmake:156–158`). Le plafond externe l'interrompt avant
cette limite ; aucune borne de temps FULL ne se juge sur la durée de ce test de correction.

`mes_m0.py --chaine` prévoit neuf cas : uniformes 8k/16k/32k à K5, ng00–02 à K5/K10. Chacun joue **W1 puis W8**
(tranches de 97 événements à W8), soit 18 chaînes CPU, neuf exports sur disque et neuf lectures sémantiques Python.
`tower_chain.cpp` appelle catalogue CPU → `resolve_tower` → `build_forests` → validation/export. Il **n'appelle pas
`build_tower`** ni le graphe recouvert A. Ses helpers T/M/V/R ont toutefois été refactorisés dans la livraison A :
ce constat ne les exonère pas d'une régression. Il ne signifie pas « sans parallélisme ».

Le wrapper CMake retient stdout/stderr jusqu'à la fin de la porte ; Python attend aussi chaque enfant avec sortie
capturée. L'absence de progrès visible ne localise donc ni le cas ni l'étage interrompu. **Ni lenteur anormale ni
deadlock ne sont prouvés.** Prochaine qualification proposée : commande distincte avec délai adapté annoncé,
journal persistant début/fin par cas, W1/W8 et lecteur sémantique. Un simple `flush` Python ne traverse pas le wrapper.

[Rejeu](check.py) normal/−O identique : neuf hashes source, plan, archive de résultats et deux membres épinglés,
sans recopier bruts ou chemins distants. [Résultats](results.json), [pins](capture.json).
Commande : `python check.py --repo /workspaces/E-HGP` (ajouter `-O` pour la seconde lecture).
