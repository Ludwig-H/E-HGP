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
| `morsehgp3d::gpu_morton_yao48_device_tiled_pair_frontier` | couverture CUDA tuilée réellement résidente : candidates, prunes et banques restent sur device; run4 qualifie le composant borné et run5 localise `candidate_capacity` |
| `morsehgp3d::gpu_morton_yao48_ranked_pair_tile_classifier` | classifieur CUDA multi-ordre résident, `count/scan`, payload fermé et tri canonique implémentés; qualification G4 native encore requise |
| `morsehgp3d::gpu_exact_closed_rank23_pair_terminal_catalog` | drain terminal fixé aux rangs fermés 2--3, publication seulement après fermeture de la masse et zéro résidu/fallback; ce n'est ni Gamma2, ni k2, ni une hiérarchie |

Les deux bibliothèques `*_reference` sont exportées séparément et ne sont pas liées par `morsehgp3d::hierarchy`. Leur coût quadratique sert uniquement à falsifier le futur producteur. Elles ne doivent pas être renommées ni réutilisées comme chemin produit. Le contrat hôte/fake n'est pas installé : il fixe seulement l'ABI interne que le futur target CUDA devra satisfaire.

Une première couverture Morton--Yao48 tuilée existe désormais en CUDA réel : elle conserve les candidates, les reçus de prune et les banques sur device. Son contrat host/fake et la qualification G4 bornée run4 sont validés. Le cap de 2 048 visites est un quantum reprenable dans le même processus; chaque subdivision ajoute un contrôle scalaire de huit octets D2H et une synchronisation, sans transfert de candidate ou de reçu. Le nouveau classifieur consomme ces chunks sur device, décide exactement les rangs fermés 2--3 sur sa voie fixe, construit les payloads intérieur/shell par `count/scan` et conserve le nuage dans la lease terminale. Toute demande de calcul large reste un fallback non consommé et censure la sortie. Les tests host/fake stricts et ASan/UBSan passent; le binaire CUDA n'est pas qualifié avant le différentiel G4 toutes-paires/tous-témoins et Compute Sanitizer. Même qualifiée, cette tranche de support deux ne reconstruit ni Gamma2, ni la hiérarchie k2, ni les morphismes verticaux, et ne vaut aucune mesure 50 k ou SLO.

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

Le prochain gate GPU qualifie le classifieur résident rangs 2--3 contre l'oracle exhaustif borné indépendant et Compute Sanitizer. La suite obligatoire persiste les niveaux exacts, ajoute la frontière de support trois, forme les facettes/cofaces et toutes leurs incidences, ferme la couverture et la verticalité, puis compare le flux et sa réduction à l'oracle Hartigan borné. Seulement après viennent le falsificateur de croissance et le résultat produit complet à 50 000/$K_{\max}=10$ avec la vue aval `min_cluster_size=20`. Les tailles 1 M, 10 M et 30 M de la campagne produit sont séquentielles et chacune doit fermer avant la suivante; les profils directs run5 ne satisfont pas ces gates.

Toute session G4 passe par les scripts gardés de [`gcp-migration`](../gcp-migration/) et se termine par la certification `TERMINATED` de la cible exacte.
