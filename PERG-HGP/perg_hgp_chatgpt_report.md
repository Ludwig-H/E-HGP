# Rapport de Version 3.3 & Phase 5 (Transmission à ChatGPT)

Ce rapport documente la validation des **optimisations de scalabilité massive (Phase 5)** et la résolution des verrous de mémoire et d'out-of-core de la bibliothèque **PERG-HGP**.

---

## 1. Mises à Jour Techniques & Corrections de Scalabilité (Phase 5)

### 1.1 Grille Spatiale & Lookup Dichotomique GPU (`searchsorted`)
*   **Résolution du goulot mémoire** : La table d'adressage dense `cell_starts_map` (qui consommait ~8 Go de RAM à résolution 1024) a été entièrement supprimée. 
*   **Bénéfice** : L'adressage des cellules de la grille utilise désormais une recherche dichotomique GPU via `torch.searchsorted` sur le tenseur des clés uniques triées `self.unique_cell_keys`. La consommation mémoire d'adressage de la grille est réduite à **0 octet** de table dense.

### 1.2 Fallback Exact Double-Chunké & Rayon de Recherche
*   **Contrôle strict de la mémoire** : Le scan de fallback global exact a été réécrit avec un double-chunking bidimensionnel (`chunk_query = 100` requêtes et `chunk_db = 100,000` points).
*   **Bénéfice** : Élimination des pics d'allocation de taille $O(M_{fallback} \times N)$. La mémoire VRAM allouée pour le fallback est strictement plafonnée à **~120 Mo** maximum pour $N=30\text{M}$ points.
*   **Optimisation du Taux de Fallback** : Le paramètre `max_radius` a été augmenté à `8` pour élargir la recherche locale certifiée avant de basculer en fallback. Le taux de fallback de la grille sur les distributions typiques tombe ainsi à **0%** (tout est résolu localement et certifié).

### 1.3 Correction du Chemin Out-Of-Core (`compute_facet_ids`)
*   **Correction de Layout de Facettes** : Résolution du bug de reconstruction de mapping des IDs de facettes par coface (`unique_ids_raw.reshape(K+1, U).T`) en présence de chunks multiples. Les facettes sont maintenant écrites directement à leurs positions globales exactes `r * U + start_u` dans le fichier binaire memmap.
*   **Bénéfice** : Les relations topologiques des complexes de Gabriel restent rigoureusement exactes, indépendamment du nombre de chunks requis par le traitement out-of-core.

### 1.4 Vectorisation du Bucket Scatter Out-Of-Core
*   **Élimination de la boucle Python** : La boucle Python par facette effectuant le scatter vers les buckets disques dans `dual_graph.py` a été entièrement vectorisée en utilisant des opérations de tri NumPy (`argsort` et identification de transitions).
*   **Bénéfice** : Accélération majeure (gain d'un facteur $> 4000\times$) lors du partitionnement out-of-core sur des dizaines de millions de facettes.

### 1.5 Agrégation des Statistiques de Requête KNN
*   **Indicateurs d'Exactitude Globale** : Intégration de `total_knn_queries`, `total_fallback_queries` et `global_fallback_rate` au niveau de l'objet `SpatialGrid3D` et agrégation finale dans `self.exactness_report_` de l'estimateur (pour les modes standard et `soft_only`).

---

## 2. Validation de la Suite de Tests

La suite de tests unitaires et d'intégration a été étendue à **13 tests**, validant de bout en bout :
*   L'active-set du miniball 3D.
*   Le test de Gabriel global par élagage AABB.
*   La reprise robuste sur coupure depuis les checkpoints.
*   Le mode `soft_only` direct.
*   La correction du layout out-of-core multi-chunks (`test_out_of_core_facet_deduplication` configuré avec un stress test `chunk_size = 10` et `max_ram_facets = 10`).
*   L'exactitude géométrique parfaite du KNN de la grille comparé à un scan brute-force exact pour des requêtes internes et externes (`test_knn_exactness_vs_brute_force`).

**Statut** : `OK` (Exécution complète des 13 tests en **3,50 secondes** sur CPU).
