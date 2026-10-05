# Fins : cause exacte des six échecs supports_route ASan u24

**Les six échecs sont exclusivement des attentes de ligne avec SHA u21 après une exécution native conforme de la sonde u24.** Source/worker épinglés `38b76701b9b0198fc1c37afe16e1480e638e513c`. Archive close `claudefins`, SHA-256 `b7421fe2176310847f3ae6ba250a5b6815c4ca81acec09d4e740508a9c62100a`. Les faits minimaux du reçu sont dans `receipt_facts.json`, les hashes des sources dans `source_manifest.json`. Aucune compilation, exécution native ou action cloud par cet auditeur.

Chaque bloc réellement consigné dans `LastTest.log`, conservé en `.txt` sous `excerpts/`, contient :

- les quatre diagnostics order_tree/full_tower à W1/W4 et celui de l'appel public compute ;
- `supports_route_verdict conforme` avec les comptes et registres attendus ;
- uniquement `run_expect_verdict ligne_absente`, suivi de l'attente gravée u21 ; aucune ligne ECART, refus ou plancher.

Le juge épinglé contrôle le code exact 0 avant la ligne attendue : atteindre `ligne_absente` établit que la sonde a rendu 0. Le garde de la sonde n'imprime `conforme` qu'après avoir comparé fichiers/manifeste/journal entre les voies et entre W, et fichiers/manifeste avec compute. Dans les six diagnostics W1, le pic public est égal au pic FULL et différent de celui d'order_tree. La contrelecture ne transforme pas ces diagnostics en comparaison de performance.

La comparaison de chaque verdict réel à son attente source ne trouve que deux différences : `fichier` et `manifeste`. Tous les comptes, le journal et `fils=1,4` concordent.

| Cas | Fichier u24 observé | Manifeste u24 observé |
|---|---|---|
| lidar_ng00_k5 | `4b781f21b6e9fd84` | `0e13ef92eda96e3e` |
| lidar_ng01_k5 | `ac1fd4da1b693572` | `12882c6a35b51bd3` |
| lidar_ng02_k5 | `b66e1a9e116e56c7` | `4963819ee6bcd867` |
| scale16000 | `1716d15eb3a5afa4` | `39af11215297416d` |
| scale32000 | `1a68904408bcae5f` | `54f75f49aceae6b7` |
| scale8000 | `8346120e7c4d1d69` | `1079d104c5e64543` |

Les SHA sont les préfixes de 16 caractères réellement imprimés par la sonde ; ils ne sont pas recalculés à partir d'un fichier de données. Les fichiers publiés n'ont pas été recopiés.

## Sanitizers et limites

Les 45 journaux runtime archivés inspectés (`LastTest.log`, `ctest.log`, `sanitizer.log`, `junit.xml` des onze tranches et le stderr de commande) ne contiennent aucun marqueur d'erreur ASan, UBSan, LSan ou TSan. Les `sanitizer.log` sont vides. `sanitizer_scan.json` garde les membres, tailles, SHA et marqueurs cherchés ; `--archive` rejoue cette recherche sur l'archive exacte. `compile_flags.json` conserve uniquement les définitions/flags du moteur et de la sonde : u24 et `-fsanitize=address,undefined` sont effectivement présents.

Le périmètre CTest reste : ASan u24 **74/80 Passed, 6/80 Failed**, TSan u21 **80/80 Passed**, aucun non joué dans ces tranches. Cette preuve explique les six échecs sans les reclasser en Passed, sans déduire une qualification globale et sans transférer les résultats à une autre source ou profil. Aucun défaut moteur ou diagnostic sanitizer nouveau n'est établi par ces reçus. Le constat de porte u18/u24 déjà publié peut maintenant être complété par une cause runtime précise pour les six cas u24 de fins ; cette preuve ne remplace pas les détails absents de fina2.

## Rejeu stdlib

Depuis ce dossier :

```sh
python3 replay.py
python3 -O replay.py
```

Sorties identiques à `proof.json`, SHA-256 `12b24c8b203abc6d8524ed6c26d9994073046b8498536e52051c59963207e182`. Le script vérifie les extraits et sources, les six seules différences de hashes, le code contrôlé, les diagnostics de pics et les nombres CTest déclarés. Il n'importe ni ne lance de programme du dépôt.

Pour rejouer aussi la recherche exhaustive sur les logs de l'archive conservée :

```sh
python3 replay.py --archive /chemin/results.tar.gz
python3 -O replay.py --archive /chemin/results.tar.gz
```

Ces sorties ont été vérifiées identiques à `archive_replay_proof.json`, SHA-256 `c0dd89ef3bb4d5f10a19594358bf89915555867b9014939e087f2e91c54f0413`, sur l'archive dont le SHA est déclaré ci-dessus. Les scripts, sources et preuves sont clos par `closure.json`. Aucun octet KITTI n'est inclus ; les extraits ne contiennent que métadonnées de porte, comptes, empreintes, diagnostics et verdicts.
