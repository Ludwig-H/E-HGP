# MorseHGP3D — cœur C++20/CUDA

Ce répertoire contient le nouveau cœur, indépendant de `HGP-old`. La Phase 15 est active sous `reference_cpu / hgp_reduced / budgeted`; la porte d'entrée est satisfaite, la sortie reste ouverte et aucun statut public exact n'est revendiqué.

## Priorité d'implémentation

Le prochain composant produit est un catalogue GPU résident de toutes les paires dont le rang diamétral fermé vérifie $2\leq R\leq K_{\max}+1$. Chaque record doit contenir la paire canonique, le niveau exact, le rang, l'intérieur strict et le shell complet. Une seule exécution à $K_{\max}$ construit tous les buckets; un simplexe Gabriel porté de cardinal $q$ alimente l'ordre $q-1$, et seulement sous `RelevantGP` ce cardinal vaut systématiquement $R$.

Le pipeline attendu est :

```text
nuage canonique
  -> Morton + LBVH résidents
  -> tuiles d'ancres et ownership Morton exact une fois
  -> parcours proche-en-premier et 48 banques Yao certifiées
  -> prunes de régions avec masse, survivants ou résidu explicite
  -> rang fermé exact sur les seuls survivants
  -> count / scan / payload
  -> tri, déduplication, chunks et transcript terminal
```

`requested_order=K` cible les rangs fermés au plus $K+1$ et remplit jusqu'à $K$ témoins par chambre. Une demande « au plus $K_{\mathrm{total}}$ points au total » cible le rang au plus $K_{\mathrm{total}}$ et requiert $K_{\mathrm{total}}-1$ témoins pour un prune. L'échec du cutoff Yao48 conserve un candidat; il ne prouve jamais que la paire sera publiée.

Après ce catalogue viennent la frontière indépendante des triangles aigus, puis les tétraèdres bien centrés. Dans la fenêtre certifiée sous `RelevantGP`, les triangles droits, obtus ou dégénérés et les tétraèdres non bien centrés sont déjà ramenés à des supports plus petits. Hors de ce domaine, une cosphère de rang élevé produit `unsupported_degeneracy` plutôt qu'une fausse fermeture.

## Ce qui est intégré

| cible CMake | statut et rôle |
|---|---|
| `morsehgp3d::exact`, `contract`, `spatial` | arithmétique exacte, coordonnées canoniques, prédicats, Morton/LBVH et oracles spatiaux |
| `morsehgp3d::facet_miniball`, `pair_support`, `higher_support` | analyse exacte des supports et primitives de flux |
| `morsehgp3d::hierarchy` | miniballs, Gamma/Gabriel bornés, EMST/Borůvka et réduction hiérarchique de référence |
| `morsehgp3d::yao48_ranked_pair_candidates_reference` | oracle borné du cutoff directionnel exact et des candidats; source quadratique isolée |
| `morsehgp3d::exact_ranked_diametral_pair_catalog_reference` | catalogue exact end-to-end borné à 512 points, comparé à un scan indépendant |
| `morsehgp3d::gpu_ranked_diametral_pair_catalog_host_contract` | API, lease SoA, budgets et reçus fail-closed testés par un launcher hostile; aucun kernel CUDA ni résultat scientifique |
| `morsehgp3d::gpu_morton_yao48_pair_frontier_host_reference` | spécification exécutable `architecture_only` du parcours Morton et des banques Yao48 fusionnés, avec prunes, survivants, résidu et masse exacte |
| `morsehgp3d::gpu_exact_diametral_phi_host_contract` | contrat hostile et oracle multiprécision du signe ponctuel exact, disponible sans CUDA |
| `morsehgp3d::gpu_exact_diametral_phi` | premier composant CUDA à trois étages : intervalle dirigé, limbs dyadiques fixes, puis lot CPU exact de repli; qualification de composant seulement |

Les deux bibliothèques `*_reference` sont exportées séparément et ne sont pas liées par `morsehgp3d::hierarchy`. Leur coût quadratique sert uniquement à falsifier le futur producteur. Elles ne doivent pas être renommées ni réutilisées comme chemin produit. Le contrat hôte/fake n'est pas installé : il fixe seulement l'ABI interne que le futur target CUDA devra satisfaire.

Le vrai noyau CUDA résident du catalogue n'est pas encore présent. Le prédicat ponctuel transfère encore un lot hôte vers le device puis ses décisions vers l'hôte; la frontière Morton--Yao48 reste une spécification host/fake. Ils qualifient deux briques avant intégration et refusent tout claim de catalogue, de `RelevantGP`, de SLO ou de statut public. Les composants CUDA historiques restent des primitives, des diagnostics ou des preuves d'infrastructure; ils ne constituent pas par composition implicite la nouvelle voie Phase 15.

## Frontières d'exactitude

- Morton est un ordre de données et de parcours, jamais une preuve de proximité.
- Le cutoff Yao48 est appliqué seulement lorsqu'une chambre contient le seuil effectif de témoins distincts certifiés; une chambre sous-remplie descend sans cutoff.
- Ces témoins n'ont pas besoin d'être les plus proches; une borne plus large réduit seulement le nombre de prunes.
- Les survivants Yao48 constituent un sur-ensemble exhaustif, pas la réponse; seuls eux sont classifiés par le prédicat exact.
- Les égalités de coque ne sont jamais prunées par approximation.
- Une proposition flottante devient une décision seulement après filtre certifié, expansion exacte ou multiprécision.
- Un cap de travail, mémoire, temps ou sortie échoue fermé et conserve la frontière résiduelle.
- La fermeture d'une paire fournit des candidats de même miniboule; seuls ceux qui contiennent tout l'intérieur strict sont Gabriel, et elle ne fournit pas tous les triangles aigus.
- Aucun tableau de toutes les paires, tous les triplets, toutes les cofaces ou toutes les cellules n'est une structure persistante admissible.
- Aucun scan dense des paires n'est un fallback produit. Le pire cas quadratique est arrêté par les caps avec `budget_exhausted` et un résidu non nul.

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
