# SHA-256 matériel : sources et traces locales

Contrelecture du commit `3d1d172db8789539b605b12c68bb5de0b34eb31f`, le 8 octobre 2026. Capture datée exactement dans `capture.json`. Aucun binaire exécuté, aucune compilation, aucune donnée de scène lue. Les sources vivantes des huit fichiers épinglés étaient identiques aux octets Git avant clôture ; les trois journaux étaient stables.

La trace locale du 8 octobre, 05:46 UTC, affiche **« voie materielle presente » et 136 contrôles / 0 échec**. La lecture du dispatch et des corps rend cette trace compatible avec un exercice effectif des instructions SHA, puis de la voie portable forcée. Le simple statut CTest Passed, sur un autre processeur, ne suffirait pas : la porte réussit aussi sans SHA matériel et le dit explicitement. Aucune qualification G4 ni d'une architecture différente ne découle de cette trace.

## Dispatch et continuité

`hardware_present()` exige SHA (CPUID 7, EBX 29), SSSE3 et SSE4.1 (CPUID 1, ECX 9 et 19), exactement les familles utilisées par le corps ciblé. Hors x86-64 GCC/Clang, le corps matériel n'est pas compilé. Le drapeau atomique est initialisé une fois ; seule l'API de test peut le changer. Il n'introduit pas d'état de hachage partagé.

`update()` complète d'abord le bloc partiel, absorbe ensuite un nombre strictement positif de blocs complets, puis garde la queue. `64 * blocks <= n - offset` ; aucun nouveau accès ne dépasse le segment fourni. Le chemin portable traite les blocs successifs puis le dernier ; la profondeur de l'appel récursif reste bornée à deux. Le chemin matériel conserve ABEF/CDGH entre blocs et ajoute l'état sauvegardé à chaque bloc ; il rétablit les huit mots ordinaires à la sortie. Cette représentation commune permet le changement de voie entre mises à jour. `finish()` conserve son fonctionnement sur une copie avec le même remplissage. Pas de défaut de dispatch ou de continuité identifié par cette lecture ; elle ne remplace pas une preuve exhaustive de toutes les instructions SIMD.

## Portée exacte des portes

Les **136 contrôles** sont : deux vecteurs FIPS fixes sur deux voies (4), treize longueurs `0, 1, 55, 56, 63, 64, 65, 127, 128, 129, 1000, 4096, 4100` et cinq découpages `1, 63, 64, 65, 1000` sur deux voies (130), un message de 4100 octets en morceaux de 300 alternant les voies (1), puis restauration du drapeau initial (1). Il ne s'agit pas de toutes les longueurs de 0 à 4100, ni de tous les découpages possibles. Sur les messages synthétiques, la référence est la voie portable ; les quatre vecteurs fixes donnent un attendu indépendant.

Le même journal comporte l'ancienne porte SHA (27 contrôles) et les deux portes différentielles contre `hashlib` (1 242 empreintes / 1 244 contrôles chacune, normales et Python `-O`). Ces dernières utilisent le dispatch par défaut, sans relever elles-mêmes le marqueur matériel. La campagne fast close contient **680 tests Passed et une sentinelle LiDAR Skipped sur 681**, zéro échec. `CMakeCache.txt` déclare Release/u21 et le répertoire source principal ; cette déclaration et les sources présentes ne reconstituent pas une attestation complète des octets utilisés à la compilation du binaire historique.

Les quatre nouveaux mutants ont chacun un emplacement unique vérifié dans la source : permutation des octets, addition de l'état ABEF remplacée par XOR, décalage de l'ordonnancement, impossibilité de réactiver la voie matérielle. Les trois premiers visent la porte SHA indépendante ; le dernier est distingué par la restauration finale du drapeau. Leur efficacité nécessite une machine où SHA est présent. **Aucune trace primaire de leur exécution n'a été retrouvée dans la recherche limitée autorisée** : nous ne reprenons donc pas comme résultat indépendant l'annonce « quatre mutants tués » du commit. La localisation correcte d'une mutation ne prouve pas sa compilation ou sa mise à mort.

## Temps et rejeu

Le gain annoncé `6,96 s → 2,9–3,2 s` porte sur sérialisation/empreinte FUL1 locale ; aucune trace primaire de ce microchronométrage n'est incluse ou contre-jugée ici. Dans la sonde épinglée, `run_wall()` fixe `t.wall` avant `after_wall()`, qui effectue validation puis `full_digest()`. Une empreinte plus rapide réduit donc le coût des campagnes avec `--digest`, **sans établir un gain du mur FULL contractuel**. Ni ce rapport local ni ces portes ne qualifient un gain G4.

```sh
python check.py --repo /chemin/du/depot
python -O check.py --repo /chemin/du/depot
```

Ce rejeu relit les huit objets Git et les extraits archivés, sans lancer les portes natives. L'option `--evidence /copie/exterieure` vérifie aussi les trois journaux complets (`last_test.txt`, `ctest_fast.txt`, `cmake_cache.txt`) contre leurs hashes et les extraits. Ces journaux, volumineux et comportant des chemins locaux, restent hors dépôt. Le rejeu normal et `-O`, avec les journaux, produit la même sortie.
