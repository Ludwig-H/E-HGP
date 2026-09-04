# Smoke combiné mono + AxisBounds, 4 septembre 2026

`public_status=not_claimed`. Qualification ciblée de la COMBINAISON des
deux optimisations intégrées, pas qualification globale C, ni benchmark.

Dans le build neuf `build/v7_axis_integrated`, seule la porte AxisBounds
avait été construite et testée (6/6). Sur instruction du responsable de
campagne, seule la cible `mhgp7_mono_inline_gate` a ensuite été construite :

```bash
cmake --build build/v7_axis_integrated --target mhgp7_mono_inline_gate --parallel 1
ctest --test-dir build/v7_axis_integrated --output-on-failure --no-tests=error -R '^mhgp7_(axis_bounds($|_)|mono_inline($|_))'
```

L'inventaire préalable contient exactement les dix noms attendus : quatre
portes mono, six portes AxisBounds. CTest rend 0, dix succès, zéro échec,
0,23 s. Les 112 fichiers normatifs inventoriés ont les mêmes hashes avant
et après : sources src/cli/oracle/tests/bench hors caches Python, CMake et
son wrapper de statut. Les binaires et flags sont épinglés à la fin ; ce
reçu ne prétend pas avoir observé une paire de hashes binaires avant/après.

Les portes mono comprennent le comptage réel des créations pthread et
les exceptions aux étapes A/B. Les portes AxisBounds imposent l'oracle
OBig et cinq divergences explicites de mutants. Chaque porte a le code de
sortie et le préfixe demandés par CMake ; le nombre dix n'est pas inféré
d'un simple code de succès de CTest.

La source reste gelée à la fin. Aucun CLI, build global ou test GCP n'a été
lancé par cet agent. Le responsable poursuit les mesures B/C et la suite
globale sous leurs reçus distincts. Les 0,23 s sont le temps des portes,
pas celui d'une construction HGP de 50k points.
