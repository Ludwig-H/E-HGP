# Rapport de diagnostic PERG-HGP Phase 5

Date d'audit : 2026-07-09

Etat audite :
- HEAD initial observe : `7464288` (`perf: set default max_ram_facets to 100M for total in-memory processing on Colab Pro G4`)
- Worktree audite : modifications non commitees dans `config.py`, `estimator.py`, `dual_graph.py`, `grid.py`, `witnesses.py` et `PERG_HGP_Colab_GPU_Benchmark.ipynb`
- Fichier non suivi observe : `test_diagnose.py` (artefact de diagnostic, non integre au rapport)

## Synthese

Gemini a corrige plusieurs points critiques de scalabilite : le KNN ne construit plus les tenseurs `(M_active, O)` pour toute la coquille de rayon, le MST dual est appele via un generateur d'aretes, `lift_witness_pool` maintient maintenant un top-budget courant, et le defaut local de `max_ram_facets` est revenu a `2_000_000` dans le worktree.

Le passage a l'echelle n'est toutefois pas encore clos. Le bug le plus important restant est fonctionnel : si `m_local > N`, `query_knn_grid` retourne des voisins invalides `-1` avec distances infinies, et `compute_regularized_sites` peut produire des NaN. Cela arrive avec les parametres par defaut sur de petits jeux, hors benchmark tool.

## Validations effectuees

### Tests de non-regression

Commandes executees avec succes :

```text
python -m unittest discover -s perg_hgp/tests -v
15 tests OK en 7.259 s

python -m unittest discover -s E-HGP/tests -v
1 test OK en 18.880 s

git diff --check
OK

python -m py_compile perg_hgp/perg_hgp/config.py perg_hgp/perg_hgp/dual_graph.py perg_hgp/perg_hgp/estimator.py perg_hgp/perg_hgp/grid.py perg_hgp/perg_hgp/witnesses.py
OK
```

### KNN grille

Le chunking des offsets est maintenant place avant les allocations `nx`, `ny`, `nz` et `n_cell_idx`. Le cout de lookup est donc borne par `M_active x 32` au lieu de `M_active x O`.

Probe exactitude :

```text
37 requetes / 200 points / m = 1, 2, 5, 10, 128
Exact contre brute force
Temps observe : environ 5.2 s
```

Estimation basse du pic lookup par sous-chunk d'offsets :

```text
m=1   grid_chunk=10000  lookup ~= 12.8 MiB
m=2   grid_chunk=10000  lookup ~= 12.8 MiB
m=10  grid_chunk=10000  lookup ~= 12.8 MiB
m=128 grid_chunk=10000  lookup ~= 12.8 MiB
```

### MST dual streaming

Le chemin principal de l'estimateur utilise maintenant `build_dual_edges(...).chunks(...)` via une lambda transmise a `dual_graph_mst`.

Probes effectuees :

```text
stress Boruvka generateur vs Kruskal : 80 cas OK sur l'etat `6d77636`
stress Boruvka generateur vs Kruskal : 10 cas OK sur le worktree modifie
```

### `max_ram_facets`

Le worktree remet le defaut a `2_000_000` :

```text
PERGHGPClusterer().get_params()["max_ram_facets"] == 2000000
```

Effet attendu :

```text
U=2,000,000, K=20 -> total_facets=42,000,000 -> out-of-core avec seuil 2,000,000
U=2,000,000, K=20 -> in-memory avec seuil 100,000,000
```

Ce retour a `2_000_000` est necessaire pour les environnements Colab standard.

## Points encore problematiques

### 1. Bug NaN quand `m_local > N`

`query_knn_grid` peut retourner des indices `-1` et des distances `inf` quand le nombre de voisins demandes depasse le nombre de points disponibles.

Probe direct :

```text
X: 20 points
Q: 5 requetes
m_local=30
idx min/max: -1 / 19
distances finies par ligne: 20 seulement
```

Avec `compute_regularized_sites` :

```text
N=50, m_local=128
entropy_target=6  -> Z contient NaN, a contient NaN
entropy_target=32 -> Z contient NaN, a contient NaN
entropy_target=128 -> Z/a finis, eta inf
```

Avec l'estimateur par defaut sur `N=50` :

```text
PERGHGPClusterer(exactness_mode="soft_only", K=2, grid_resolution=8)
fit_predict termine sans exception
Z_ contient des NaN
global_fallback_rate = 0.5

PERGHGPClusterer(exactness_mode="atlas_exact", K=2, grid_resolution=8)
fit_predict termine sans exception
Z_ contient des NaN
global_fallback_rate = 0.8571428571
```

Correctifs recommandes :
- Dans `fit`, capper `m_local`, `m_active` et `K_rho` par `N` avant tout appel KNN/Gibbs.
- Dans `query_knn_grid`, ne pas laisser `-1` se propager comme indice valide.
- Dans `compute_regularized_sites`, masquer explicitement les voisins invalides au lieu de laisser `inf` entrer dans les produits Gibbs.

### 2. `build_dual_edges` reste dangereux en mode legacy

Le nouveau `build_dual_edges` retourne un `DualEdgesIterable`, ce qui evite l'allocation immediate dans le chemin estimateur. Mais son `__iter__` materialise encore les quatre tableaux complets `(U, K)` pour conserver le depliage legacy :

```python
edge_u, edge_v, edge_w, edge_coface = build_dual_edges(...)
```

Cette compatibilite est utile pour les tests existants, mais elle reste non scalable si un utilisateur externe deconstruit directement l'objet.

Correctif recommande :
- Documenter explicitement que le depliage legacy est reserve aux petits graphes.
- Ajouter une methode explicite `materialize()` pour le mode legacy, et faire de `__iter__` un iterateur de chunks, ou au minimum emettre un warning si `U*K` depasse un seuil.

### 3. Out-of-core facettes : le disque et les tableaux finaux restent massifs

Le mode out-of-core evite le gros `np.unique` global, mais il ne rend pas tout constant en memoire/disque.

Estimations :

```text
U=2,000,000, K=20
temp disk raw+bucketed+unique+orig ~= 9.7 GiB
unique_ids_raw RAM ~= 0.3 GiB
facet_ids final int64 ~= 0.3 GiB
unique_facets worst ~= 3.1 GiB

U=20,000,000, K=20
temp disk raw+bucketed+unique+orig ~= 97.0 GiB
unique_ids_raw RAM ~= 3.1 GiB
facet_ids final int64 ~= 3.1 GiB
unique_facets worst ~= 31.3 GiB
```

Conclusion : le mode out-of-core est une amelioration majeure, mais l'objectif `30M / K=20` peut encore dependre fortement du nombre reel de cofaces certifiees et de la duplication des facettes.

### 4. Risque de skew dans les buckets out-of-core

`compute_facet_ids` utilise 64 buckets selon le premier sommet de facette. Si les premieres coordonnees de facettes sont tres skewed, un bucket peut rester tres grand et etre charge en RAM lors de `np.unique`.

Correctif recommande :
- Ajouter un second niveau de partitionnement si `bucket_counts[b]` depasse un seuil memoire.
- Ajouter un test avec distribution de facettes volontairement skewed.

### 5. Notebook Colab encore imparfait

Le notebook a ete corrige pour utiliser `max_ram_facets=2000000`, mais le titre contient encore :

```text
30 000 000 de Points (=10$)
```

Il faut restaurer `($K=10$)`.

### 6. Benchmark minimal peu favorable

Commande :

```text
timeout 120s python -u perg_hgp_benchmark_tool.py --n_samples 50 --centers 2 --k 2 --min_cluster_size 5 --device cpu --exactness_mode soft_only --output_format json
```

Resultat :

```text
PERG-HGP soft_only : 27.77 s, ARI -0.0025, clustered 12%, fallback 50%
HDBSCAN : 0.021 s, ARI 1.0, clustered 100%
```

Ce benchmark ne bloque pas la scalabilite massive, mais il montre que `soft_only` n'est pas une baseline qualitative robuste sur petits jeux.

## Recommandations prioritaires

1. Corriger le cas `m_local > N` avant toute autre revendication de robustesse API.
2. Ajouter un test de regression : `PERGHGPClusterer()` par defaut sur `N < 128` ne doit produire aucun NaN.
3. Ajouter un test direct `compute_regularized_sites` avec indices invalides ou, mieux, garantir que `query_knn_grid` n'en renvoie jamais aux appelants non prepares.
4. Rendre le mode legacy de `build_dual_edges` explicitement non scalable ou le remplacer par une API `materialize()` volontaire.
5. Ajouter une protection bucket-skew dans `compute_facet_ids`.
6. Corriger le titre du notebook Colab.

## Verdict

Les corrections de Gemini sont significatives : le KNN et le MST dual ont franchi un vrai palier de scalabilite. Mais l'implementation n'est pas encore prete a etre declaree robuste pour passage a l'echelle general. Le principal bloqueur immediat n'est plus le MST, mais la robustesse du KNN/Gibbs face aux petits `N` et aux voisins manquants, ainsi que les garanties memoire finales de `compute_facet_ids`.
