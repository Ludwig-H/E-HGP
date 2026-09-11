# MEB : première paire maximale, qualification CPU privée

Ce reçu porte exclusivement sur la nouvelle fonction `anchor_meb_diameter`, dans le profil de coordonnées entières u16 et pour 1 à 10 sites distincts. Aucune source produit n'est remplacée. Aucun résultat Welzl, CUDA ou d'un autre reçu n'est hérité. Statut public : `not_claimed` ; ce paquet est une qualification locale bornée, pas un contrat de latence.

La [preuve](sources/current/PROOF.md), le [prototype](sources/current/anchor_meb_diameter.hpp), le [diff de fonction](sources/current/variant.diff) et le [juge](sources/current/gate.cpp) restent lisibles directement. Les autres sources et tous les journaux sont stockés sans modification dans des objets zlib dédupliqués. `storage_map.json` conserve leurs noms logiques et leurs SHA-256 ; `capture_manifest.json` et le lecteur lient les six captures à la même fermeture de 68 fichiers. Les sources réellement compilées sont les copies `source_snapshot` propres à chaque capture, pas l'état actuel du produit.

## Résultat et portée

O2 et SAN ROOT ont la même sortie brute : 52 488 contrôles, 6 416 comparaisons MEB, dont 161 cas jugés par l'oracle rationnel Gram et 6 255 appels réellement rencontrés dans trois constructions depuis de vrais census n=32, s=8/10/12, K=1..10. Les appels K9 et K10 sont rendus non vacus par le juge. Treize cas de rejet, la dernière valeur représentable du compteur et son accumulation sont contrôlés séparément.

Les supports canoniques, coquilles, clés, niveaux et statuts restent identiques au MEB nominal épinglé `386072c8`. Les 161 cas Gram vérifient en plus indépendamment la géométrie, le premier support et le nouvel ordre de travail. Le corpus inclut carré à diamètres égaux, triangle aigu, toutes leurs permutations, bords u16 et contre-exemple K7 permanent. Les trois flux de census passent par le Builder privé épinglé `83f1c78e`, avec observation test-only de chaque invocation MEB ; le compte observé égale celui de validation plus résolution. Cela ne recertifie pas indépendamment la complétude WSPD ni toutes les sorties de la tour.

Dans le seul périmètre des 6 416 comparaisons nominales, le lecteur recalcule depuis les sorties : 437 473 → 293 135 supports formés (144 338 de moins), 579 018 → 258 574 tests de puissance (320 444 de moins), avec 158 088 distances de paires supplémentaires explicitement facturées. Ces nombres n'incluent pas tout le travail de juge, ses appels de référence, ni les exercices de rejet. Les durées des commandes sont instrumentées et ne sont pas des benchmarks ; aucune accélération temporelle ou baisse de registres GPU n'est déduite.

## Six captures, seize commandes

| Capture | Commandes et codes exacts | Diagnostic causal |
| --- | --- | --- |
| `o2_r1` | compilation 0, selftest 0, argument inconnu 2, argument absent 2 | nominal |
| `san_root_r1` | compilation 0, selftest 0, argument inconnu 2, argument absent 2 | ASan/UBSan, LSan actif |
| `mutant_ties_r1` | compilation 0, mutant 1 | `diameter.canonical` |
| `mutant_counter_r1` | compilation 0, mutant 1 | `diameter.paid_pairs` |
| `mutant_order_r1` | compilation 0, mutant 1 | `diameter.paid_powers` |
| `mutant_shell_r1` | compilation 0, mutant 1 | `diameter.shell` |

Les mutants changent respectivement premier en dernier maximum, cachent une distance évaluée, rétablissent l'ancien ordre de confinement sous le nouveau contrat comptable et doublent une partie de la coquille. Le mutant d'ordre est comptable, pas une réfutation géométrique de l'ancien algorithme. Chaque capture est close, ses sources et son instantané sont stables. Il n'existe pas de tentative en échec omise dans cette campagne privée.

## Lecture et reproduction

Depuis ce dossier :

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --show captures/o2_r1/selftest.stdout
python3 -B verify.py --extract /tmp/meb-diameter-extraction-fresh
```

L'extraction exige une destination inexistante et reconstruit tous les fichiers logiques, notamment les six instantanés compilés. Le lecteur ne compile et n'exécute aucun C++. Il vérifie les inventaires exacts, les SHA, les commandes, les flux et codes, les causes des mutants, les dépendances de dépôt déclarées par `-MMD`, les sources d'origine et l'identité des sorties O2/SAN. Il recompose aussi le diff de la fonction depuis les sources. Ses contrôles restent actifs sous Python `-O`. L'outil de conditionnement `publish.py` est conservé, mais ne constitue pas une nouvelle qualification géométrique.

Les recettes originales `record.py` et `prepare.py` sont conservées byte-identiques. `prepare.py` est historique : pour rejouer, utiliser les sources extraites, pas un nouvel export de l'actif. Après extraction, on peut lancer depuis `sources/current/` le recorder original avec un nouveau `--out` ; le chemin Boost constant doit être fourni tel quel ou adapté dans une NOUVELLE copie explicitement qualifiée. Les captures existantes ne sont jamais écrasées.

## Limites de provenance et d'intégration

Les commandes strictes C++20 (`-Wall -Wextra -Wpedantic -Werror`) et les sources sont épinglées ; les binaires ELF ne sont pas distribués. Leurs SHA avant/après et leur taille vérifiée à l'emballage sont conservés : un lecteur portable peut vérifier la cohérence de ces attestations, pas recalculer le SHA d'un ELF absent. Le compilateur g++ (exécutable/version), les en-têtes système et Boost ne sont **pas épinglés** dans ces captures ; Boost est fourni en `-isystem` et donc exclu de `-MMD`. Ce paquet n'est pas une fermeture hermétique d'outillage. Les fichiers `.cu/.cuh` présents dans la copie source ne prouvent aucune compilation CUDA.

Le compteur supplémentaire `diameter_pairs` est distinct des champs de `AnchorMebWork` dans l'API exercée ; un alias intentionnel n'est pas qualifié. La fonction privée n'est pas raccordée au produit, au terminal GPU ni à un batch résident. Le prochain raccord devra préserver ce travail payé et requalifier sa propre fermeture. Aucun contrat 50k/1 s, 100 ms ou plusieurs dizaines de millions de points n'est acquis ici. GCP non utilisé par cette campagne ni par son emballage.
