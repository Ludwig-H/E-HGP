# Qualification Python du dendrogramme ponctuel

27 septembre 2026. Capture close **PASS**, quatre commandes code0, avec
`/home/codespace/.python/current/bin/python`, en normal puis `-O`.
Durée de capture observée : 4,947 s ; ce n'est pas un benchmark du moteur.

| test, dans chaque mode | résultat |
| --- | --- |
| `test_point_tree.py` | 181 fixtures, 3 124 coupes strictes/fermées, 27 refus, trois intégrations FULL |
| `test_point_eom_audit.py` | neuf gates unittest, oracle Fraction indépendant sur petits arbres à masses unitaires |

Les comptes normal/−O ne sont pas additionnés. Les deux stdout du test
d'arbre sont identiques et ses stderr vides. L'audit EOM écrit son compte
et `OK` sur stderr ; ses stdout sont vides. Les durées unittest diffèrent,
donc ses stderr ne sont pas annoncés bit-identiques.

## Reçu et fermeture

Capture privée :
`/tmp/mhgp9-point-dendrogram-qualification-20260927-ltxbaw2x/receipt.json`.
SHA256 : `1507b1117092574ad2a2466bdabe6cab78517171d66ba48b7b05cea3d6e6414c`.

Le dossier conserve l'intention initiale, le préflight, les quatre argv/cwd/
environnements/codes, stdout/stderr, 14 artefacts hachés et le reçu final.
**729 empreintes avant/après identiques** ferment les quatre sources gelées,
le runner, les imports locaux et oracles dynamiques, l'interpréteur,
les preuves FULL historiques, leur build, les fichiers générés de
l'adaptateur et les artefacts compilés hérités. Les trois entrées u32le
sont également comparées aux coordonnées des fixtures du reçu historique.
Les sources système héritées sont hachées ; ce n'est pas une image
hermétique de l'OS et de toute la bibliothèque standard Python.

Le reçu FULL lu est
`/workspaces/E-HGP/build/v9-weighted-full-qualification-20260927-r1/receipt.json`,
SHA256 `35126c2a591a4d410b44d86a80967b50f4574bcdb9cadc5d94d9a982bafaba9c`.
E5 K2, carré K2 et carré K1 sont les seules intégrations géométriques
rejouées en Python. Les autres artefacts historiques sont contrôlés par
leurs hashes, pas présentés comme une nouvelle exécution des 33 fixtures.
**Aucun build, binaire natif, GPU ou GCP exécuté** dans cette capture.

[Runner](run_qualification.py) gelé, SHA256
`67d17c57c85d2faaa53562d5dac03e619433864e0e88c90817d5d7457b885d9c` :

```sh
/home/codespace/.python/current/bin/python -B morsehgp3D_v9/experiments/point_dendrogram_20260927/run_qualification.py
```

Le défaut crée un nouveau `mktemp` sous `/tmp` ; `--output CHEMIN` exige un
chemin inexistant. Aucune capture n'est écrasée. Le document seul n'est
pas une archive autonome : conserver les fichiers privés référencés.

## Portée

Le **squelette ponctuel** a n feuilles et au plus n−R multifusions, donc
O(n) nœuds. L'**objet complet retourné**, avec `source_to_point_node` et
`merge_source_nodes`, garde une provenance O(V+n), où V est le nombre de
nœuds source ; ne pas qualifier le payload complet de O(n).

La géométrie et les coupes restent rationnelles exactes. Le raccord EOM
emploie des cardinalités ponctuelles unitaires et des calculs binary64 ;
il refuse les collisions de dates exactes converties et exige une vraie
racine unique, n≥1, m≥2. Forêts et n=0 restent représentables/coupables,
sans racine artificielle ajoutée pour EOM. Ces gates ne prouvent ni des
marges EOM exactes générales, ni une qualité statistique ou une supériorité
sur HDBSCAN. Voir [le contrat du producteur](POINT_TREE.md).
