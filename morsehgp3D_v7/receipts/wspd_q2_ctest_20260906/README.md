# Rejeu CTest ciblé du réemploi q2

6 septembre 2026, CPU local, `public_status=not_claimed`. **19/19 CTests
passent** sur les sources courantes portant `generate.hpp` `345129a7…` :
deux portes du delta, descente fusionnée, treize portes de pile de témoins,
ownership WSPD et ses deux mutants. Ce n'est pas la suite CTest globale.
Les [16 premiers résultats](targeted_ctest.log) et les [trois derniers](ownership_ctest.log)
sont les journaux bruts produits par CTest. Le [différentiel O2/ASan-UBSan](../wspd_terminal_q2_reuse_20260906/README.md)
reste une qualification séparée ; ce supplément est Release seulement.

La première configuration dans `build/v7_wspd_q2_ctest_20260906` a échoué
par absence de Boost dans les chemins système. Elle est conservée en build,
sans prétendre disposer d'un journal stdout/stderr public de cet appel.
ROOT a observé le code 1 et le diagnostic CMake demandant les en-têtes.
La reprise part dans un autre répertoire neuf, avec les en-têtes déjà
présents localement, sans installation :

```bash
cmake -S morsehgp3D_v7 -B build/v7_wspd_q2_ctest_20260906_r2 -DCMAKE_BUILD_TYPE=Release -DMHGP7_DIGEST_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build build/v7_wspd_q2_ctest_20260906_r2 --parallel 2 --target mhgp7_wspd_terminal_reuse_gate mhgp7_selftest mhgp7_witness_stack_gate
ctest --test-dir build/v7_wspd_q2_ctest_20260906_r2 --output-on-failure --no-tests=error -R '^mhgp7_(wspd_terminal_reuse(_bad_argument)?|fused_descent|witness_stack(_helper|_witness|_lane_mask|_lane_mask_mutant|_unknown_case|_empty_case|_duplicate_case|_unknown_argument|_unknown_mutant|_empty_mutant|_duplicate_mutant|_mutant_case_mismatch)?)$'
ctest --test-dir build/v7_wspd_q2_ctest_20260906_r2 --output-on-failure --no-tests=error -R '^mhgp7_wspd_(ownership|mutant_cap|mutant_split)$'
```

Configuration et compilation terminent à 0, constaté par ROOT dans la
session ; les deux appels CTest ont aussi utilisé `--output-log` pour
produire les fichiers copiés ici. Les [hashes](source_context.json) décrivent
le contexte de source courant, pas une nouvelle compilation hermétique.
Le binaire utilisé pour les [mesures s8/10/12](../full_wspd_q2_separation_20260906/README.md)
a sa propre capture de compilation. GCP non utilisé.
