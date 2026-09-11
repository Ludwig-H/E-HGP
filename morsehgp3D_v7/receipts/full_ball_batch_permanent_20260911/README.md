# Preuve portable : cinq CTests du callback CPU

Exploration v7 hors registre, profil u16, `public_status=not_claimed`. Ce paquet contient la nouvelle qualification CMake O2 et ASan/UBSan/LSan des cinq CTests proposés pour le callback de lot. L'unique header produit consommé est la couture `83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`, sans instrumentation ni macro `MHGP7_TESTING`. Aucun résultat antérieur n'est hérité.

Les deux tests nominaux CPU1 et CPU4 parcourent chacun les 34 nuages et 150 ordres du juge FULL borné n≤8 : 298 742 contrôles Gram/Gamma, 3 916 coupes, 87 230 contrôles verticaux, quatre descentes à rayon égal. Chaque voie compare les mêmes forêts physiques et le travail géométrique avec la voie du même moteur sans callback : 49 lots, 103 requêtes, 103 terminaux BallId directs, 5 704 contrôles physiques et 442 contrôles de travail. Les 65 refus incluent 32 cas après préfixe et 44 erreurs dont le travail payé reste connu ; les arguments inconnu et absent exigent exactement le code 2. Cela qualifie ces cinq CTests, pas la suite complète.

Les trois nouveaux fichiers sont exclusivement sous `tests/`. Le callback calcule ses attentes directement à l'entrée depuis une recette scalaire de référence, avant injection des fautes. Ce calcul de juge paie 119 appels MEB supplémentaires par voie, déclarés séparément et exclus du travail du callback : ce n'est ni une optimisation ni une mesure de performance. Cette recette provient du moteur historique `6763` et n'est pas indépendante ; l'oracle géométrique indépendant de la forêt reste Gram/Gamma. Les BallId directs vérifient les requêtes effectivement reçues, pas une trace interne instrumentée de Builder.

Le paquet conserve les helpers originaux épinglés, les candidats, le header réellement compilé, la baseline, le patch CMake, les scripts de préparation/capture, les sources O2/SAN identiques, les commandes, les dépendances de la cible et les sorties physiques CTest. `sources/current/` rend visibles les sources qualifiées ; `storage_map.json` déduplique sans perte les captures. Les ELF ne sont pas distribués : seuls leurs deux hashes sont conservés. Aucun vendor ni bytecode Python n'est inclus.

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /chemin/neuf
```

Le lecteur vérifie les pins et la provenance complète, les cinq enregistrements CTest, leurs codes exacts, les résultats physiques, les flags stricts C++20 et les sanitizers avec détection des fuites active. Il ne compile rien, n'exécute aucune géométrie et ne contacte aucun service. L'extraction est create-only ; les commandes historiques avec chemins absolus sont conservées comme attribution, non comme promesse d'exécution inchangée ailleurs. Pour reconstruire, utiliser le CMake de `sources/current/morsehgp3D_v7/` et fournir le chemin local des en-têtes Boost déjà disponibles ; aucune installation ni copie vendor n'est automatique.

Cette preuve ne qualifie pas le callback GPU, le raccord census K1..10 avec callback, ni les contrats 50k/1 s ou multi-millions. Aucun temps de CTest n'est un benchmark. GCP non utilisé. Les sources actives et `audits/` n'ont pas été modifiées par ce travail privé.

## Révision physique du stockage documentaire

La copie qualifiée `sources/current/morsehgp3D_v7/bench/COMPARE_MONO.md` est stockée physiquement sous `COMPARE_MONO.md.source`, avec exactement les mêmes octets. Ce snapshot documentaire n'est pas une documentation autonome dont les liens relatifs auraient été relocalisés. Les trois chemins logiques restent inchangés dans `storage_map.json` ; l'extraction rétablit le nom `.md`. Aucune source, commande, sortie, attente ou observation qualifiée n'est modifiée. Le manifeste de capture et le lecteur sont byte-identiques à la version précédente.

L'ancien manifeste physique, son stockage et son README sont conservés sous `provenance/layout_v1/`. Le nouveau manifeste désigne seulement ce nouveau conditionnement ; il ne remplace ni ne réinterprète le hash historique `6396a594daeab25f409e1f2a5ce812b29ec7bc599e4d7328632f02106533365c`. Les chemins historiques restent attribuables via cette provenance. Les détails du renommage sont dans `provenance/layout_v2.json`. Cette révision n'ajoute aucun résultat géométrique, CTest, CUDA ou de performance.
