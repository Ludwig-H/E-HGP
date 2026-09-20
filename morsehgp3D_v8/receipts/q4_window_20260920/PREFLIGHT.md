# Préflights de construction30, avant gel

Ces contrôles exploratoires ne sont pas les captures finales189sources.
Les sondes Release et Clang ASan/UBSan ont compilé ; un premier appel
Release8k/dense_permuted/K10 a donné les mêmes sorties que29.
Ses chronos exploratoires ne sont pas repris comme benchmark final.

Les premières compilations de la **gate** ont échoué, code2, dans les
deux nouveaux builds. Le moteur avait compilé ; le fichier de test
mélangeait des pointeurs `const Points*` et `Points*` dans une liste
d'initialisation dont le type devait être déduit.

```text
cmake --build build/v8_q4_window_20260920 --parallel 4 --target mhgp8_q4_window_gate
tests/q4_window_gate.cpp:337:77: error: unable to deduce std::initializer_list<auto>
deduced conflicting types: const std::vector<mhgp8::Point3>* and std::vector<mhgp8::Point3>*
exit code: 2

cmake --build build/v8_q4_window_sanitize_20260920 --parallel 4 --target mhgp8_q4_window_gate
tests/q4_window_gate.cpp:337:28: error: deduced conflicting types
('const Points *' vs 'Points *') for initializer list element type
exit code: 2
```

La correction porte sur le typage de la fixture, pas sur le produit.
Ce journal conserve l'échec ; aucun reçu de qualification n'avait
encore été lancé ou gelé. Les futurs builds/tests corrigés sont distincts
de ce premier essai, qui n'est pas réinterprété comme PASS.

## Préparation de la mutation compilée : chemin d'objet corrigé

Le premier lancement du helper de mutations est sorti avec code1 avant
même d'écrire son manifeste : l'inventaire demandait par erreur
`CMakeFiles/mhgp8_q4_window_gate.dir/tests/q4_shallow_gate.cpp.o`.
Le nom correct est `q4_window_gate.cpp.o`. Aucun mutant n'avait été
compilé ou exécuté ; ce n'est pas un mutant tué ni un échec du moteur.
Le [helper exact avant correction](preflight/run_mutants_before_object_path_fix.py)
est conservé. Seul ce chemin du helper hors189sources change ; les
sources moteur/gate/runner gelées sont inchangées. La nouvelle capture
utilisera un autre répertoire et son propre manifeste.
