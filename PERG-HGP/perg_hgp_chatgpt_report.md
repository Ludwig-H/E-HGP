# Rapport de Correctifs & Version 2 de PERG-HGP (Transmission à ChatGPT)

Ce rapport fait suite aux retours de relecture technique sur la version prototype. Les corrections prioritaires ont été intégrées directement au sein de la bibliothèque sous `/workspaces/E-HGP/perg_hgp`. Il décrit les ajustements apportés pour garantir l'exactitude géométrique, la sécurité des allocations et la scalabilité out-of-core.

---

## 1. Correctifs Majeurs Apportés

### 1.1 Séparation des modes d'exactitude (Priorité 0)
Le module [`config.py`](file:///workspaces/E-HGP/perg_hgp/perg_hgp/config.py) intègre désormais un paramètre explicite `exactness_mode` et des gardes-fous de budgets mémoire :
*   `exactness_mode` : `Literal["soft_only", "atlas_exact", "global_gabriel_certified", "cut_certified"]`.
*   **Budgets explicites** : Ajout de limites pour contrôler la combinatoire avant l'explosion :
    *   `max_witnesses_per_rank` (défaut : 100K)
    *   `max_cofaces` (défaut : 1M)
    *   `max_unique_facets` (défaut : 10M)
    *   `max_dual_edges` (défaut : 50M)
*   **Audit d'exactitude** : L'estimateur [`estimator.py`](file:///workspaces/E-HGP/perg_hgp/perg_hgp/estimator.py) produit un dictionnaire `self.exactness_report_` détaillant si les budgets ont été dépassés ou si les certificats de Gabriel sont partiels.

### 1.2 Remplacement Complet de SciPy MST par un Solveur Kruskal Direct (Priorité 1)
Le goulot d'étranglement de la matrice creuse CSR de SciPy a été éliminé dans [`hierarchy.py`](file:///workspaces/E-HGP/perg_hgp/perg_hgp/hierarchy.py).
*   **Implémentation** : La fonction `dual_graph_mst` utilise à présent l'algorithme de **Kruskal en streaming** directement sur la liste d'arêtes (`edge_u`, `edge_v`, `edge_w`) triée par NumPy. 
*   **Union-Find** : Un module d'Union-Find purement CPU avec compression de chemin a été écrit.
*   **Bénéfice** : Zéro allocation de matrice CSR géante en mémoire. L'arbre couvrant (MST) est extrait en streaming linéaire sur les arêtes triées, éliminant tout risque d'OOM sur cette étape.

### 1.3 Certification Gabriel Globale par Élagage de Bornes Spatiales (Priorité 2)
Le test de Gabriel global dans [`gabriel.py`](file:///workspaces/E-HGP/perg_hgp/perg_hgp/gabriel.py) a été complètement réécrit pour être **géométriquement exact et certifié**.
*   **Algorithme d'élagage** : Pour chaque cellule active de la grille 3D, le module précalcule le poids minimal des sites qu'elle contient : `cell_min_a = min_{j in cell} a_j`.
*   **Borne Inférieure** : Pour une coface candidate de centre $c$ et de rayon au carré $\rho^2$, la distance minimale au carré $d_{cell}^2$ entre $c$ et la boîte englobante (Bounding Box) de chaque cellule active est calculée.
*   **Critère d'élimination** : La cellule est ignorée (safe skip) si et seulement si :
    $$d_{cell}^2 + \text{cell\_min\_a} \ge \rho^2 - \text{tol}$$
*   **Bénéfice** : Ce critère garantit que si une cellule est élaguée, elle ne peut mathématiquement contenir aucun site perturbateur (intruder). Si elle ne l'est pas, ses points sont inspectés. Le test est maintenant **rigoureusement global** et certifié sur tout l'espace 3D, sans se limiter à un voisinage fixe de cellules.

### 1.4 Verrouillage Temporel de la Géométrie Additive (Priorité 3)
*   **Vérification** : Une assertion a été ajoutée au début de `fit` dans `estimator.py` pour forcer `alpha == 0.0`.
*   **Pourquoi** : Le solveur de miniball active-set en 3D actuel (taille support $\le 4$) est conçu pour la métrique de puissance additive ($\Phi_i(y) = |y-z_i|^2 + a_i$). Si $\alpha \neq 0$, le problème devient multiplicatif et non homogène. Le code lève donc une exception propre `NotImplementedError` au lieu de produire une géométrie fausse.

---

## 2. Pistes pour la Production (30M points sur Colab)

Pour porter ce prototype robuste vers un traitement industriel de 30M de points sans déconnexion Colab :

1.  **Fichiers Memory-Mapped (Zarr/Memmap) :**
    Toutes les briques intermédiaires (les coordonnées $Z$, les indices de voisinage $N \times m$, et les arêtes du graphe dual) devront être configurées en mode `out-of-core` via NumPy memmap ou des fichiers Zarr compressés pour ne charger en VRAM que les chunks en cours de traitement.
2.  **Kernel Triton pour la programmation dynamique :**
    La DP de préfixe/suffixe dans `rank_field.py` est actuellement vectorisée en PyTorch mais reste limitée par l'interpréteur Python lors de la boucle sur $m_{active}=128$. L'écriture d'un kernel Triton fusionné (fused Triton kernel) pour le calcul des polynômes symétriques élémentaires sur GPU est la clé pour traiter 30M de points rapidement.
3.  **Mode Fallback "Atlas Soft" pour $K=20$ :**
    Lorsque le dual graph des facettes menace d'exploser combinatoirement à $K=20$, le clusterer lèvera une alerte via `self.exactness_report_`. Il est recommandé d'implémenter un mode "Atlas Soft" qui construit la hiérarchie directement sur les témoins et cofaces certifiés plutôt que sur le dual des facettes de taille $K$.
