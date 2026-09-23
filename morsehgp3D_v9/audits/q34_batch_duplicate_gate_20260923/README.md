# S2 : un filtre batch de cardinalité correcte peut omettre une clé q3

Audit local du WIP S2 du 23 septembre 2026, sans modification du constructeur. La source testée est `build/v9-open-worktree/morsehgp3D_v9/src/gen/pipeline/wspd_q34.cpp`, SHA-256 `1e34498e40f5a7a40c0e22ad14bb14bbaf23347494e0313553f51910d50c7892` (HEAD du worktree `5577f0f2a`, modifications non commitées). Le reçu dépend de ce snapshot et de sa bibliothèque vivante ; il fixe le SHA CMake au moment de la configuration (le CMake WIP a ensuite ajouté une autre porte, sans changement de `mhgp9_gen`). Ce n'est pas une qualification du GPU ni une archive binaire autonome.

La source de `wspd_q34.cpp` publiée ensuite dans le port S2 `a6d81f9ce`
a le même SHA-256 ; le contre-exemple structurel reste applicable à cette
version publiée. La mention WIP décrit l'origine du reçu, pas un écart de
source avec le port publié.

Suivi du correctif publié dans **`2059189d`** (23 septembre, 16 h 11 UTC) :
un parcours ajouté après le garde de masse
impose appartenance au rectangle, ordre strict et masque inclus. Il vise
le contre-exemple ci-dessous. Mais le nouveau mutant « duplicate » dans
`chain_batch_filter_gate.cpp:188–191` ajoute un survivant **et incrémente
`expanded_pairs`** ; le garde antérieur de `wspd_q34.cpp:1195` le refuse
avant le nouveau parcours, indépendamment de celui-ci. Pour valider
causalement la correction, remplacer un survivant par une copie d'un autre
à cardinalité et comptes inchangés, puis contrôler le motif du refus.
Le mutant « foreign » garde le compte et exerce l'appartenance dans le
[rejeu indépendant](recheck_2059189d.cpp) ci-dessous. L'ancien reçu reste
celui de `a6d81f9ce` ; le nouveau garde ferme **ce contre-exemple local**.

Le rejeu [stdout](recheck_2059189d.stdout) compile la source publiée
`2059189d84d9e6878230c3169fd6226d70ac4109` extraite par `git archive`,
SHA-256 `wspd_q34.cpp=911edce89839e84197de73694d2289b1a2672e231dccaca3a2a28759f7724885`,
`libmhgp9_gen.a=c9df948a1b58553c6f5d1fd7618301148512d9d2c72bca59b79205498b5184df`.
Sur la fixture à trois sites/K3, le flux moteur égale le batch honnête ;
la copie du survivant du rectangle 0 à la place de celui du rectangle 1,
**sans changer cardinalité ni compteurs**, est refusée avec le motif
`duplicate, unordered or widened pair`. Le déplacement d'un rang `b`
vers la diagonale est refusé avec `pair outside its surviving rectangles`,
`expanded_pairs=3` inchangé. Sidecar SHA-256
`6a31cc9f2d529a055cdabee7207d546fd61066f7b52c669e80f9905305e8ad6c`,
binaire `5e4f2476ce941a866aeb07b008a7a5eade3c788564c6eab9ee91719a909c4e9d`,
stdout `2dc38b39b1b80de26dba9009b132c93f265b23fea1c8282d1c27c09a256025f3`.
Le test intégré « duplicate » reste non causal malgré ce bon résultat du
nouveau contrôle : corriger sa mutation et exiger le motif avant de compter
ce test comme porte structurelle.

La fixture a trois points u18 distincts : `(0,0,0)`, `(100,0,0)`, `(30,60,0)`. Le triangle est strictement aigu : son plus grand côté est l'arête `(0,1)` de carré 10 000, inférieur à la somme des deux autres carrés 4 500 + 8 500. À `K=3`, sa boule q3 a profondeur zéro et doit paraître dans le flux. Le sidecar `check.cpp` compare d'abord **tout le flux** du moteur parallèle et du batch CPU honnête (arité, coefficients de clé, support, profondeur, coquille), puis remplace un couple survivant par un autre de même masque. Le callback malveillant ne change ni la taille de la sortie, ni les masques de rectangles, ni les masses, ni les comptes de rejets, ni les visites déclarées.

Le cas trouvé remplace le survivant d'indice 1, issu du rectangle 1 et des rangs `(0,2)`, par une copie du survivant d'indice 0, issu du rectangle 0 et des rangs `(0,1)`. Les deux chemins honnêtes émettent la même unique présentation q3 ; le chemin altéré n'en émet aucune. Le retour normal de `run_wspd_q34_batched` prouve que `validate_completion` accepte ce lot. Les objets de compteurs `witness` sont égaux champ par champ entre lots honnête et altéré, de même que `expanded_pairs=3` et `q3_edges=3`. Le [reçu](receipt.json) contient les empreintes et la sortie du gate.

La cause est à `wspd_q34.cpp:1171–1208` : la validation vérifie la taille, les rangs globaux et la plage des bits du masque, puis **déduit** `rejected_pairs` de la seule cardinalité des survivants. Elle ne relie pas chaque survivant à son rectangle ou à l'ordinal unique de sa paire. `validate_completion` à `:1255` ne vérifie que les égalités de masse. Un doublon et une omission se compensent donc même si une clé q3 disparaît. Ce test injecte la corruption au contrat `Q34BatchFilter` ; il ne démontre aucune erreur de calcul CUDA spontanée. Il montre que l'intégration actuelle ne la détecterait pas.

Action proposée : transporter l'ordinal de paire dans la sortie batch et vérifier, avant l'étape des arêtes, l'ordre strict, les bornes de son rectangle et la cohérence du masque ; ajouter des mutants duplicate/drop et swap à cardinalité constante. Cela détecte les erreurs structurelles. L'égalité **géométrique** des masques GPU doit rester qualifiée par une porte différentielle complète face au CPU ou par une preuve et ses tests causaux ; les seules identités de masse ne la certifient pas.

Rejeu **historique de la faille** depuis `/workspaces/E-HGP`, sur le commit
`a6d81f9ce9f7e916384b044b771665f621a086eb` dont la source ciblée a
le même SHA que le WIP mesuré, avec Boost :

```sh
mkdir -p /tmp/mhgp9-s2-a6-src
git archive a6d81f9ce9f7e916384b044b771665f621a086eb morsehgp3D_v9 | tar -x -C /tmp/mhgp9-s2-a6-src
cmake -S /tmp/mhgp9-s2-a6-src/morsehgp3D_v9 -B /tmp/mhgp9-q34-batch-duplicate-audit-build -DCMAKE_BUILD_TYPE=Release -DMHGP9_ENABLE_CUDA=OFF -DMHGP9_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build /tmp/mhgp9-q34-batch-duplicate-audit-build --target mhgp9_gen -j4
c++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -I/tmp/mhgp9-s2-a6-src/morsehgp3D_v9/src/gen -isystem build/v7_boost_gate/extracted/usr/include morsehgp3D_v9/audits/q34_batch_duplicate_gate_20260923/check.cpp /tmp/mhgp9-q34-batch-duplicate-audit-build/libmhgp9_gen.a -lpthread -o /tmp/mhgp9-q34-batch-duplicate-audit-build/check
/tmp/mhgp9-q34-batch-duplicate-audit-build/check
```

Pour rejouer **le garde publié** sans modifier le constructeur :

```sh
mkdir -p /tmp/mhgp9-s2-2059189d-src
git archive 2059189d84d9e6878230c3169fd6226d70ac4109 morsehgp3D_v9 | tar -x -C /tmp/mhgp9-s2-2059189d-src
cmake -S /tmp/mhgp9-s2-2059189d-src/morsehgp3D_v9 -B /tmp/mhgp9-s2-2059189d-build -DCMAKE_BUILD_TYPE=Release -DMHGP9_ENABLE_CUDA=OFF -DMHGP9_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build /tmp/mhgp9-s2-2059189d-build --target mhgp9_gen -j4
c++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -I/tmp/mhgp9-s2-2059189d-src/morsehgp3D_v9/src/gen -isystem build/v7_boost_gate/extracted/usr/include morsehgp3D_v9/audits/q34_batch_duplicate_gate_20260923/recheck_2059189d.cpp /tmp/mhgp9-s2-2059189d-build/libmhgp9_gen.a -lpthread -o /tmp/mhgp9-s2-2059189d-recheck
/tmp/mhgp9-s2-2059189d-recheck
```
