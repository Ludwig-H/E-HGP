# Contre-audit B — porte T2 sur la chaîne publique v9

22 septembre 2026. Porte autonome **réalisée localement** :
`audits/morsehgp3D_v9/public_chain_t2_gate.cpp` (SHA-256
`f6917bd377f7819729870bc335cdb80be3db2eb177770488c324f17c640003ad`).
Elle appelle réellement `run_tower_chain` avec `run_tower=true` et
`keep_catalogue=true`. Le worktree moteur détaché est à
`d27003148baa631ddface3c76e0483459c9d4334`, mais **sale** ; la porte et
les reçus locaux ne sont pas des objets de ce commit. Ne pas attribuer cette
qualification au distant, ni la confondre avec le gate produit existant
`chain_census_tower_gate`, lequel appelle `run_tower=false`.

## Résultat et limite

Le nuage T2 a **12 sites** : quatre sommets d'un tétraèdre aigu autour de
`(20,20,20)`, plus deux témoins extérieurs par face. La boule sentinelle q4
canonique `A=1, B=(-40,-40,-40), C=900`, rayon carré 300, possède quatre
points de coquille, zéro intérieur et `q_min=4` ; elle ne provient d'aucun
support q3. La porte reconstruit l'inventaire rationnel exhaustif q2/q3/q4,
compare clés, niveaux, arités et populations globales, puis juge la forêt
publique pour **K=1..3** contre les coupes ouvertes/fermées du modèle Γ T2,
les parents et les racines verticales. Quatre permutations réelles des IDs
d'entrée (identité, inversion des quatre sommets, témoins d'abord, inversion
totale) sont chacune exécutées avec `s=8/10/12` et `W=1/4` : **24 appels
publics**. La comparaison des digests et payloads entre configurations est
secondaire au juge rationnel sur chaque permutation.

| Binaire | Résultat nominal | Témoin de suppression ciblée |
| --- | --- | --- |
| GCC `-O2`, bibliothèques locales | 24/24 PASS ; 307 524 assertions ; 69 lignes oracle par permutation ; 24 émissions q4 | code 1, `cause=public_T2.inventory_cardinality` |
| Clang 18 ASan/UBSan, bibliothèques intégralement instrumentées | mêmes 24/24 PASS et comptes ; aucune alerte sanitizer/LSan | même code 1 et même cause |

`--mutant-omit-q4` confirme **avant suppression** que la clé canonique
ci-dessus figure exactement une fois dans le catalogue rendu ; il ne retire
que cette clé, puis le juge détecte la cardinalité erronée. C'est une
**mutation du harnais après l'appel public**, pas un mutant compilé dans le
producteur ; elle prouve la sensibilité de cette comparaison, pas que le
générateur produira toujours toutes les q4. Le juge rationnel et le lecteur
FULL sont repris des tests T2 v9, indépendants du générateur mais pas une
seconde implémentation indépendante du modèle Γ.

Ces 12 sites et K1..3 n'établissent ni exhaustivité générale, ni FULL K1..5
ou K1..10 sur une trame SemanticKITTI entière, ni vitesse G4, ni borne
sous-quadratique. La capture LiDAR locale à plusieurs dizaines de milliers
de sites n'a pas d'inventaire indépendant comparable.

## Rejeu et empreintes

Depuis `/workspaces/E-HGP` ; les chemins `/tmp` sont temporaires et doivent
être recréés pour un nouveau rejeu. La reconstruction des bibliothèques
sanitisées utilise **un seul job, `nice -n 19`**, pendant la campagne
développeur. Les deux commandes de test terminent en moins de 120 s ; le
second code 1 est attendu.

```sh
nice -n 19 c++ -std=c++20 -O2 -g0 -Wall -Wextra -Wpedantic -Werror -pthread -I/workspaces/E-HGP/build/v9-open-worktree -I/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include audits/morsehgp3D_v9/public_chain_t2_gate.cpp build/v9-open-worktree/build/v9/libmhgp9_chain.a build/v9-open-worktree/build/v9/libmhgp9_gen.a -o /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_o2
timeout 120s nice -n 19 /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_o2
timeout 120s nice -n 19 /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_o2 --mutant-omit-q4
cmake -S build/v9-open-worktree/morsehgp3D_v9 -B /tmp/mhgp9-public-t2-VJ2IsN/asan -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DMHGP9_SANITIZE=ON -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr
nice -n 19 cmake --build /tmp/mhgp9-public-t2-VJ2IsN/asan --target mhgp9_chain -j1
nice -n 19 clang++ -std=c++20 -O0 -g1 -Wall -Wextra -Wpedantic -Werror -pthread -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -I/workspaces/E-HGP/build/v9-open-worktree -I/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include audits/morsehgp3D_v9/public_chain_t2_gate.cpp /tmp/mhgp9-public-t2-VJ2IsN/asan/libmhgp9_chain.a /tmp/mhgp9-public-t2-VJ2IsN/asan/libmhgp9_gen.a -o /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_asan
timeout 120s nice -n 19 env ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_asan
timeout 120s nice -n 19 env ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 /tmp/mhgp9-public-t2-VJ2IsN/public_chain_t2_gate_asan --mutant-omit-q4
```

Empreintes SHA-256 de cette exécution : `census_tower_oracle.hpp`
`334a758a8a2e0d6e0580f921bf07d7ab7e81b0a0eda67a1c1ec5446449328715`,
`full_ball_tower_gate.cpp`
`767debe0eb43d83b5f496dc1a32edab56d0bca5223b0ddb0a7febf9306e0299d` ;
bibliothèques GCC chaîne/générateur
`7107859fd23f7cf575ae7da4b814c983b0905007bb529a8de90dac799236ab02` /
`04817e9f8ad5086b19bef69abde1460890d16575f7250af6c8a4daa57fe74ef7` ;
bibliothèques Clang ASan/UBSan
`bfa04bb7f4f407c524dfbeb7ea69319d991499bdd6416507e4290ee9640dd32c` /
`c2b8426c89f3143cd92925e078588ab5101fea4e80fb62b3858488835476a3ce` ;
binaires de juge GCC/Clang
`c17ceca602b71c434cbd30a040976be486853b906a3fd6ec98c11e50075e9608` /
`3df752548f297c1bd65b82c97d9a2d8857a3b68e291c91b664c3084fb854a592`.
Les archives GCC préexistaient ; la reconstruction Clang emploie la CMake
locale SHA-256
`236c73711b9ce2f4fa3c7b7f13a0eba0558776c489eecde3b172e8ea8683b4cf`.
Ce manifeste ponctuel ne gèle pas toutes les dépendances transitives du
worktree sale. Aucun lecteur Python n'intervient dans cette porte C++ :
`python -O` n'y est pas applicable ; les configurations C++ `-O2` et `-O0`
sont explicitement séparées ci-dessus.

## Réserve sur la nouvelle porte `arith_u18`

Le commentaire local de `src/tower/core/wide.hpp` affirme que la porte tue
le mutant `level-trunc-hi`. Or, dans la CMake locale, `arith_u18` est créé
par `mhgp9_product_executable` et non `mhgp9_test_executable` :
`MHGP9_TESTING` manque, donc `MHGP9_MUTANT(...)` vaut `false` à la compilation.
Son `main` n'accepte que `--selftest` et la CMake n'enregistre que ce gate et
un refus `--unknown`, **pas une exécution mutée numérique**. L'oracle
arithmétique peut être utile, mais la preuve causale « mutant tué » est
actuellement non établie ; il faut une cible de test instrumentée, une
activation du nom et un reçu code 4 sur divergence géométrique. Ce constat
vise uniquement le worktree non publié, sans modifier le moteur. Les SHA-256
locaux sont `wide.hpp`
`877ba8c4bd045c023f3cd64e8f7fc5edf0d074bfd0c5fac9510306e06f5f387e`,
`arith_u18_gate.cpp`
`84acfbbb4f15183646feb30f4c1d2d16cf0acf170f626eae03b5cc0035add4f9`
et `mutants.hpp`
`50c7c50e36f401ef323917df0ba1d0546f9ce09bb4138fb8051ca45ceadcab9a`.
