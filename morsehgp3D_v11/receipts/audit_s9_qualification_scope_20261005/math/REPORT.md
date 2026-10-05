# S9 — portée de qualification du plancher maximal

Pin lu : `c97776ea8b8730bc41c8da4b137879cb8b544c2c`, contenant la S9
`3d47eaa9313164f878e39dd7d59ee3a08032659e`. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed`. Aucun build, programme natif ni GCP exécuté.

La différence utile est une limite de qualification, pas un défaut du moteur.
Au pin c977, `tools/g4_matrix.json` exclut `_vs_python` de `release_long`.
`tests/points/points_oracle_stdlib.py:205` compare exactement les dates, les
propriétaires et les partitions aux plateaux annoncés, mais ne consulte pas
les colonnes `floor` et `strict`. Le différentiel existant
`tests/points/points_vs_python.py:113` compare ces deux colonnes à la chaîne
Python ; cette preuve locale ne devient pas une exécution de ce différentiel
sur G4. Une session dédiée avec les dépendances Python épinglées et les
différentiels existants suffirait à combler cette distinction de portée ; aucune
nouvelle porte n'est proposée.

La contre-épreuve est volontairement limitée à une sortie de sonde synthétique
F8/K2/m3, préparée à partir de `Definition` et de niveaux rationnels obtenus par
énumération des supports positifs de cardinal 2 à 4 admissibles dans Cat_K.
Pour le site 0, la date est `sqrt(100)+sqrt(250)-sqrt(625/4)` et le plancher
correct vaut le rang 6, de niveau `625/4`. On change seulement `floor[0]` en
rang 5, de niveau `144`, en conservant date, strict=1, propriétaire et arbre.
Le niveau suivant, `625/4`, est encore sous la date : le plancher modifié est
donc faux. Le comparateur standard accepte pourtant les deux sorties avec les
mêmes comptes (8 sites, 1 retardé, 5 plateaux).

Les seuls contrôles sémantiques `_check_hanging(..., True)` et
`_check_point_tree(..., True)` du lecteur acceptent également cette mutation.
Le premier certifie la borne inférieure et le drapeau strict
(`bench/mhgp11_formats.py:1041`), sans maximalité ; pour une entrée stricte,
le second rattache le plateau à la date, sans consulter le plancher
(`bench/mhgp11_formats.py:1070`). Il ne s'agit pas d'un fichier MHGP11PT
sérialisé : ni `read_points`, ni `check_directory`, ni tous les contrôles de
conteneur/provenance n'ont été rejoués. Le résultat prouve cette frontière
des contrôles cités, sans prétendre démontrer une acceptation complète de fichier.

La lecture du moteur apporte la distinction causale essentielle :
`src/points/settle.cpp:69` certifie `P(floor)` puis `non P(floor+1)` sur tout
le catalogue, avant d'écrire la colonne. Le contre-exemple synthétique ne
contredit donc pas l'implémentation actuelle et n'établit aucun défaut
géométrique ou de mémoire. Aucun nouveau cœur mathématique S10/plat n'était
présent dans le worktree observé ; les quatre fichiers de correction du tri
correspondaient aux empreintes déjà capturées, sans nouveau rejeu.

Sources et preuve : `source_manifest.json` contient les empreintes de 18 fichiers
obtenus par `git show` au pin fixé avant l'exécution. Cet inventaire n'est pas
une revue intégrale de ces 18 fichiers. Les petites fonctions de préparation
`levels_of` et `point_tree` reprennent les helpers de la capsule S9 antérieure
(`check_points.py`, SHA256
`4c72d7e22c861a210aed595c2a86f7c64fbd1634c806663e36b4a0644eb6c97e`) ;
elles sont incorporées dans `check_gap.py`, sans dépendance à cette ancienne
capsule ni répétition de sa suite. L'oracle et les contrôles testés viennent du
pin courant. Aucun essai en échec n'a eu lieu dans cette capsule ; le premier
script exécuté est conservé sous `attempts/initial_check_gap.py`.

Commandes exécutées, codes et sorties : `executions.json`, `normal.json`,
`optimized.json` et leurs fichiers stderr. Deux exécutions, normal et `-O`,
ont rendu le code 0, avec des sorties identiques et des stderr vides.
Toutes les vérifications du script utilisent des exceptions explicites, sans
`assert`. Le rejeu portable proposé reconstruit uniquement les sources listées
et vérifie leurs empreintes ; son exécution est laissée à la consolidation :

```sh
python3 replay.py --repo /workspaces/E-HGP
python3 -O replay.py --repo /workspaces/E-HGP
```

À reprendre : les fichiers listés dans `SHA256.json`, plus ce manifeste lui-même.
Le dossier `snapshot/` est omis de la publication : Git fournit les sources.
