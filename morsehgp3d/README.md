# MorseHGP3D — cœur C++20/CUDA

Ce répertoire contient le nouveau cœur, indépendant de `HGP-old`. La Phase 15 est active sous `reference_cpu / hgp_reduced / budgeted`; la porte d'entrée est satisfaite, la sortie reste ouverte et aucun statut public exact n'est revendiqué.

## Priorité d'implémentation

Le prochain composant produit est un catalogue GPU résident de toutes les paires dont le rang diamétral fermé vérifie $2\leq R\leq K_{\max}+1$. Chaque record doit contenir la paire canonique, le niveau exact, le rang, l'intérieur strict et le shell complet. Une seule exécution à $K_{\max}$ alimente les ordres $k=R-1$.

Le pipeline attendu est :

```text
nuage canonique
  -> Morton + LBVH résidents
  -> top-Kmax exact dans 48 chambres demi-ouvertes
  -> rapports de régions et cutoffs Yao48 exacts
  -> frontière bloc--bloc conservatrice
  -> émission exacte une fois
  -> rang fermé filtré puis exact
  -> count / scan / payload
  -> tri, déduplication, chunks et transcript terminal
```

Après ce catalogue viennent la frontière indépendante des triangles aigus, puis les tétraèdres bien centrés. Les triangles droits, obtus ou dégénérés et les tétraèdres non bien centrés sont déjà ramenés à des supports plus petits.

## Ce qui est intégré

| cible CMake | statut et rôle |
|---|---|
| `morsehgp3d::exact`, `contract`, `spatial` | arithmétique exacte, coordonnées canoniques, prédicats, Morton/LBVH et oracles spatiaux |
| `morsehgp3d::facet_miniball`, `pair_support`, `higher_support` | analyse exacte des supports et primitives de flux |
| `morsehgp3d::hierarchy` | miniballs, Gamma/Gabriel bornés, EMST/Borůvka et réduction hiérarchique de référence |
| `morsehgp3d::yao48_ranked_pair_candidates_reference` | oracle borné du cutoff directionnel exact et des candidats; source quadratique isolée |
| `morsehgp3d::exact_ranked_diametral_pair_catalog_reference` | catalogue exact end-to-end borné à 512 points, comparé à un scan indépendant |

Les deux dernières bibliothèques sont exportées séparément et ne sont pas liées par `morsehgp3d::hierarchy`. Leur coût quadratique sert uniquement à falsifier le futur producteur. Elles ne doivent pas être renommées ni réutilisées comme chemin produit.

Le vrai noyau CUDA résident du catalogue n'est pas encore présent. Les composants CUDA historiques restent des primitives, des diagnostics ou des preuves d'infrastructure; ils ne constituent pas par composition implicite la nouvelle voie Phase 15.

## Frontières d'exactitude

- Morton est un ordre de données et de parcours, jamais une preuve de proximité.
- Le cutoff Yao48 est appliqué seulement lorsqu'une chambre contient $K_{\max}$ témoins distincts certifiés; une chambre sous-remplie descend sans cutoff.
- Les égalités de coque ne sont jamais prunées par approximation.
- Une proposition flottante devient une décision seulement après filtre certifié, expansion exacte ou multiprécision.
- Un cap de travail, mémoire, temps ou sortie échoue fermé et conserve la frontière résiduelle.
- La fermeture d'une paire fournit des candidats intérieurs, pas tous les triangles aigus.
- Aucun tableau de toutes les paires, tous les triplets, toutes les cofaces ou toutes les cellules n'est une structure persistante admissible.

Le contrat scientifique détaillé est dans le [catalogue des paires](../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) et la [frontière des supports trois et quatre](../docs/math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md).

## Construction

```bash
cmake -S morsehgp3d -B build/morsehgp3d-cpu-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d-cpu-release --parallel
ctest --test-dir build/morsehgp3d-cpu-release --output-on-failure
```

Configuration avec sanitizers :

```bash
cmake -S morsehgp3d -B build/morsehgp3d-sanitizer \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMORSEHGP3D_BUILD_TESTS=ON \
  -DMORSEHGP3D_ENABLE_SANITIZERS=ON
cmake --build build/morsehgp3d-sanitizer --parallel
ctest --test-dir build/morsehgp3d-sanitizer --output-on-failure
```

Les compilations GCC et Clang utilisent les avertissements stricts et un mode flottant certifié. Toute nouvelle cible de test C++ doit passer par la liste centralisée des sanitizers, contrôlée par `tests/configuration/check_phase3_build.py`.

## Gate GPU

Un benchmark G4 n'est utile qu'après intégration d'un vrai pipeline résident et d'un différentiel borné. L'ordre obligatoire est : tests exacts courts, Compute Sanitizer, falsificateur à 12 500 points, puis trente nuages frais à 50 000/$K_{\max}=10$. Les tailles 1 M, 10 M et 30 M sont séquentielles et chacune doit fermer avant la suivante.

Toute session G4 passe par les scripts gardés de [`gcp-migration`](../gcp-migration/) et se termine par la certification `TERMINATED` de la cible exacte.
