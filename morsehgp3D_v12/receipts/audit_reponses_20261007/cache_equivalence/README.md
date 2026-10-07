# Équivalence du corps cache, 7 octobre 2026

Le corps non commis `buffer.cpp` passe de SHA-256 `1844a7d6e6d063347ace171be4ca9899669b08b30b2bf2106fa73d9b096c47cd` à `5c885dbf0e4813f1a8ee653115092536348877e6b93aa5b831d7490fe0d6e756` par trois substitutions locales dans `cached_acquire` : supprimer `BlockCache& cache = *account.cache;`, puis remplacer `cache.mutex` et `cache.exact` par les accès directs correspondants à `account.cache`.

`capture.json` conserve le diff exact, les trois substitutions et leurs contrôles : chacune est unique dans cette fonction et leur application redonne **le fichier nouveau entier**, sans autre différence. `buffer.hpp` (`3c7fffc6…`) et la porte officielle `alloc_fault.cpp` (`1111d433…`) correspondent encore à la capture qualifiée dans [cache/](../cache/README.md).

Sous la durée de vie contractuelle de `BudgetAccount`, son cache reste le même objet : même mutex verrouillé et même compteur incrémenté, dans le même ordre. L'alias local n'a aucun effet propre. Les corps de `hold`, restitution, allocation et poison sont intacts. Il s'agit donc d'une **équivalence de source vérifiée**, pas d'un nouveau rejeu des portes. Les résultats natifs antérieurs restent épinglés à `1844a7d6` ; ce reçu explicite leur raccord au seul changement d'alias de `5c885dbf`.

Aucun test natif supplémentaire, aucune modification du produit ou des reçus historiques, aucun GCP. La future fusion d'autres modifications devra être contrôlée séparément. `public_status=not_claimed`.
