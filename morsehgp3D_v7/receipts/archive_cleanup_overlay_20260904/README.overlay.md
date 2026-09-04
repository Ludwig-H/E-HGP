# Correctif isolé : nettoyage de l'archive sous allocation persistante

Statut au 4 septembre 2026 : **overlay de qualification, non intégré**. Le
produit, les campagnes gelées et le registre ne sont pas modifiés. Aucun claim
mathématique ou de capacité ne découle de cette correction de cycle de vie.
GCP non utilisé par cette sous-tâche.

## Autorité et périmètre

La source produit examinée est `morsehgp3D_v7/src/io/archive.hpp`, SHA-256
`e7f056ce909527f668f0239e416bb22175cf3676900f2cd459d36a25e895c9c5`.
Le CMake réel est resté au SHA-256
`79201af402181df2810938509587fa2d190b6364a2436ab911e83116392c2b75`.
L'overlay corrigé est `src/io/archive.hpp`, SHA-256
`cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430`.
Ses répertoires `src/core`, `src/forest` et `src/pipeline` sont des liens vers
les dépendances gelées ; le seul header produit remplacé est celui d'archive.

Le test a été initialement créé, conformément à la première délégation, dans
`morsehgp3D_v7/tests/archive_cleanup_gate.cpp`. Dès que le gel a été précisé
comme couvrant aussi l'inventaire des tests, ce seul fichier nouvellement créé
a été déplacé ici et retiré du répertoire réel. Il n'a jamais été enregistré
dans le CMake gelé. Aucun autre fichier réel n'a été modifié par cette tâche.
Les sources de l'auditeur externe sont inchangées.

## Pourquoi le correctif ferme la panne constatée

`StagingDirectory`, membre RAII construit avant les mutations, possède les
descripteurs du parent et du provisoire ainsi qu'un nom dans un tableau fixe.
Le destructeur ne reconstruit aucun `path`, ne parcourt pas le répertoire et
n'appelle pas `remove_all`. Il supprime exactement `input.u16`, les dix noms
`forest_K1.bin` à `forest_K10.bin`, puis `manifest.json`, par `unlinkat` relatif
au descripteur retenu ; il tente ensuite la suppression du seul répertoire
provisoire nommé dans son parent retenu. Aucun nom issu de l'entrée n'est
interprété pendant ce nettoyage.

Les allocations préparant le nom de destination ont lieu avant la création du
provisoire. Après `mkdtemp`, le nom est copié sans allocation et l'objet membre
possède déjà la responsabilité du nettoyage, même si `openat` échoue et que
l'allocation du message d'exception échoue elle aussi. Les écritures utilisent
également `openat`, de sorte que les fichiers créés appartiennent à la même
autorité de répertoire que celle qui les nettoie.

La publication reste le `renameat2(..., RENAME_NOREPLACE)` atomique. Le membre
se marque publié immédiatement après son succès ; seul le `fsync` système du
parent suit. Son échec donne `parent_sync_confirmed=false`, sans exception ni
effacement de l'archive déjà publiée. Cette information reste un diagnostic
de durabilité, pas une promesse de résistance à une panne de courant.

Si l'OS refuse un `unlinkat` ou la suppression du répertoire, le destructeur
émet une ligne `archive_cleanup=failed provisional=... operations_failed=...
first_errno=...` via un tampon fixe et au plus trois appels `write`. Le message
ne nécessite aucune allocation C++ et n'affirme pas que le résidu a disparu.
Le format est borné en taille ; un descripteur stderr bloquant reste soumis au
comportement de l'OS. Les erreurs de fermeture de descripteur ne sont pas une
garantie de durabilité. Les fichiers étrangers inattendus ne sont pas effacés
récursivement : ils font échouer le retrait du répertoire et sont signalés.

Le code utilise Linux, `/proc/self/fd`, `openat`, `unlinkat` et `renameat2`,
cohérent avec le contrat Linux existant. Le contrôle initial `fstatat` refuse
aussi une destination qui est un lien symbolique pendant ; la protection
create-only finale ne change pas. Un contrôle d'inode détecte une substitution
déjà présente au nom provisoire ; il ne constitue pas une promesse contre un
tiers qui renomme continuellement les entrées du parent pendant le nettoyage.
Le test d'allocation concerne `operator new`, pas l'OOM killer, une terminaison
forcée ni une absence de descripteur stderr exploitable.

## Résultats reproductibles

La probe externe `audits/archive_cleanup_probe.cpp` a été copiée ici sans
modifier un octet, SHA-256 identique
`f94bc8032d4a55f4afccf43a56497866f803eb3e42f2d96b05dc9420b964a63d`.

| Exécution | Code | Observation |
| --- | ---: | --- |
| Probe externe, source gelée, `fail` | 97 | `terminate` et `input.u16` provisoire laissé |
| Même probe, overlay, `normal` | 0 | `destructor_returned`, aucun provisoire |
| Même probe, overlay, `fail` | 0 | `destructor_returned`, aucun provisoire |
| Test API historique, overlay, Release | 0 | 2 publications, défauts fsync parent/fichier, rejets existants conservés |
| Nouvelle porte, overlay, Release | 0 | 23 allocations refusées effectivement atteintes ; détail ci-dessous |
| Nouvelle porte finale, overlay, ASAN + UBSAN + fuites | 0 | Mêmes 23 refus ; aucun diagnostic sanitizer |

Le résidu de la contre-fixture gelée a été inspecté puis retiré par `unlink`
de son unique fichier synthétique et `rmdir` des deux répertoires temporaires
nommés exactement. Aucune donnée utilisateur n'a été supprimée.

La nouvelle porte n'utilise pas une exception fictive comme unique témoin :
elle remplace les allocations C++ globales, constate une allocation refusée,
puis laisse l'échec activé pendant toute la destruction. Elle vérifie :

- nettoyage sous panne persistante après l'entrée et après les dix fichiers
  d'ordres effectivement écrits ;
- huit positions d'allocation distinctes du constructeur, puis un échec OS
  après `mkdir` mais avant l'ouverture du provisoire, avec panne d'allocation
  persistante pendant la construction de l'erreur ;
- dix positions d'allocation du commit, dont le manifeste déjà créé, sans
  archive finale ni provisoire après refus ;
- les callbacks réels K1 et K2 du pipeline sur E5, tous deux écrits dans
  l'archive, puis `resource_exhausted` à l'étage fold, invalidation du payload
  provisoire et disparition de l'archive non publiée ;
- un échec du `fsync` parent post-publication qui active aussi la panne
  persistante, tout en conservant une archive complète et le diagnostic faux ;
- un refus OS effectif de suppression (`input.u16` remplacé par un répertoire),
  exactement deux opérations en échec, message borné présent même sous panne
  d'allocation et résidu reconnu, puis nettoyé par le test après rétablissement.

Sortie Release :

```text
archive_cleanup_gate=passed persistent=1 all_order_files=10 constructor_faults=8 post_mkdir_faults=1 commit_faults=10 callback_mask=6 parent_sync_fault=1 os_cleanup_fault=1 denied_allocations=23
```

Le même binaire a été rejoué par `cmake/run_expect.cmake` avec code attendu 0
et préfixe attendu `archive_cleanup_gate=passed persistent=1 all_order_files=10 ` :
code 0. Ce rejeu ne configure et ne modifie aucun CMake gelé.
Le test final est au SHA-256
`22874ff740c6092fcd9148fe20200e4a3303728ede59053d4fc3718911c42972` ; le
binaire Release avec `MHGP7_TESTING=1` est au SHA-256
`f24724c4b7e36a2ae016475a33e477b836c6954ea15742b876d959575f056ced`.

Un premier passage ASAN a échoué dans l'injecteur de test : la surcharge
`new(nothrow)` manquante était interceptée par ASAN, alors que la libération
utilisait `free` via notre `delete`. Le test final surcharge aussi les formes
nothrow et alignées, sans désactiver `alloc_dealloc_mismatch`. Ce résultat
intermédiaire n'a pas été compté comme un succès du produit. Son résidu
temporaire synthétique a été inspecté puis retiré de manière ciblée.
La reprise finale avec `detect_leaks=1`, `halt_on_error=1` pour ASAN et UBSAN
termine avec le code 0, sans diagnostic ; la sortie est conservée dans
`cleanup_asan_ubsan.txt`. Le binaire sanitizer est au SHA-256
`8864bb55e44f49cbccebc3551d13d7d533e2c4339a6fdfbb8d22d3563cfa2045`.

Commandes depuis la racine du dépôt :

```bash
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror build/v7_archive_fix/audits/archive_cleanup_probe.cpp -o build/v7_archive_fix/archive_cleanup_probe
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror build/v7_archive_fix/tests/archive_api_gate.cpp -Wl,--wrap=fsync -o build/v7_archive_fix/archive_api_gate
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 build/v7_archive_fix/tests/archive_cleanup_gate.cpp -Wl,--wrap=openat -Wl,--wrap=fsync -o build/v7_archive_fix/archive_cleanup_gate
build/v7_archive_fix/archive_api_gate
build/v7_archive_fix/archive_cleanup_gate
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Werror -pthread build/v7_archive_fix/tests/archive_cleanup_gate.cpp -Wl,--wrap=openat -Wl,--wrap=fsync -o build/v7_archive_fix/archive_cleanup_gate_asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/v7_archive_fix/archive_cleanup_gate_asan
```

## Intégration proposée après la fin du gel seulement

Trois patches séparés sont préparés, **non appliqués**, et `git apply --check`
les accepte contre l'état gelé examiné :

1. `archive_cleanup.patch` : seul le header produit d'archive ;
2. `archive_cleanup_test.patch` : ajout du test permanent, autonome dans son
   emplacement final `morsehgp3D_v7/tests/archive_cleanup_gate.cpp` ;
3. `ctest_registration.patch` : porte C++ `mhgp7_archive_cleanup`, et portes
   Python `mhgp7_incidence_campaign` / `mhgp7_incidence_campaign_optimized`
   pour le banc existant, toutes labellisées `gate` avec timeout 120 s.
   La porte C++ exige aussi le préfixe de non-vacuité dans sa sortie.

La dernière proposition ne modifie pas le banc gelé ni ses classifications.
Une fois ces changements autorisés et intégrés, reconstruire le produit et
les portes, vérifier les archives au niveau CLI, puis refaire les tests
pertinents et les pins. Les résultats précédents restent ceux de leur source
gelée ; ils ne deviennent pas rétroactivement des résultats du correctif.
