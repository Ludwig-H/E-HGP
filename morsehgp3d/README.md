# MorseHGP3D — cœur certifié

Ce répertoire porte la nouvelle implémentation décrite par la roadmap. `HGP-old/` reste une référence historique séparée et n'est pas importée par ce cœur.

État actuel : Phase 2A, backend `reference_cpu`, couche numérique commune aux profils, mode `certified`.

Le premier lot fournit :

- l'interprétation exacte des coordonnées binary64 comme dyadiques;
- `ExactRational3` homogène et canonique;
- `ExactLevel` non négatif, canonique et compatible avec le contrat v2;
- le fallback `boost::multiprecision::cpp_int` qui servira d'oracle aux filtres CPU.

Ce lot ne ferme ni la Phase 2A ni G1. Les filtres bornés, expansions, prédicats géométriques, compteurs et outils de replay sont ajoutés par lots testables ultérieurs.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
```

Le target exporté est `morsehgp3d::exact`. Après `cmake --install`, un consommateur peut utiliser `find_package(MorseHGP3D CONFIG REQUIRED)` sans dépendre de chemins d'en-têtes propres à la machine de construction.
