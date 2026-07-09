# PERG-HGP 3D massif : algorithme GPU-friendly pour 30M de points, `K = 10` ou `K = 20`

**Nom de travail :** `PERG-HGP` pour **Progressive Entropic Rank-Gabriel HGP**.  
**Cas d'usage cible :** nuage de points 3D massif, typiquement `n = 30_000_000`, ordre HGP `K = 10` ou `K = 20`, exécution sur Google Colab Pro avec un GPU de type G4/L4/A100/T4 selon disponibilité.  
**But :** construire une hiérarchie de densité de type HGP entropiquement régularisée, suffisamment proche de l'esprit HGP exact, mais calculable par morceaux sur GPU.

Ce document est un cahier des charges pour implémenter une bibliothèque Python/Cython/C++/CUDA. Il est volontairement directif. Les variantes existent, bien sûr, mais ce n'est pas une invitation à faire du tourisme algorithmique avec 30 millions de points. La mémoire GPU n'est pas une ressource spirituelle.

---

## 0. Résumé exécutif

Pour `p = 3`, `n = 30M`, `K = 10` ou `K = 20`, il est irréaliste de construire :

- le complexe de Čech complet ;
- la mosaïque de Delaunay d'ordre `K` ;
- tous les `(K+1)`-simplexes ;
- ou même tous les `K`-faces nécessaires à un HGP exact non régularisé.

L'algorithme proposé remplace donc l'énumération combinatoire par une procédure de **témoins critiques entropiques**.

Le pipeline recommandé est :

```text
X brut
  -> grille spatiale GPU / Morton sort
  -> voisinages locaux
  -> Gibbs local UMAP-like avec échelle eta_i
  -> sites régularisés (z_i, a_i)
  -> témoins critiques soft du champ de rang, rang k = 1..K+1
  -> candidats cofaces sigma de taille K+1
  -> certification locale ou globale Gabriel, selon budget
  -> graphe dual d'atlas, construit par facettes hashées
  -> MST / Boruvka GPU
  -> hiérarchie exacte de l'atlas généré
```

Le résultat final est :

1. **exact pour l'atlas de cofaces effectivement généré et certifié ;**
2. **exact pour le modèle HGP régularisé complet seulement si l'atlas est complet ou cut-complet ;**
3. **heuristique mais certifiante partielle dans le mode massif réaliste.**

Le mode par défaut pour ce cas d'usage doit donc être :

```text
mode = "atlas_soft_rank_certified_local"
```

et non :

```text
mode = "full_exact_hgp"
```

Le second mode est une belle fiction pour 30M de points et `K = 20`. Une fiction mathématiquement élégante, certes, mais les fictions élégantes n'allouent pas de VRAM.

---

## 1. Statut mathématique : ce qui est exact, ce qui ne l'est pas

### 1.1 Modèle exact HGP classique

Pour un nuage `X = {x_i}_{i=1}^n \subset R^3`, l'estimateur `K`-NN dur classique est défini par :

```math
r_K(y) = \inf\{r \ge 0 : |B(y,r) \cap X| \ge K\}.
```

Les niveaux de forte densité, paramétrés par le rayon, sont :

```math
L_K(r) = \{y \in R^3 : r_K(y) \le r\}
       = \{y : |B(y,r) \cap X| \ge K\}.
```

HGP exact reconstruit les composantes connexes de `L_K(r)` via les `K`-polyèdres du complexe de Čech. Les objets élémentaires sont les `K`-faces, c'est-à-dire les simplexes de cardinal `K`. Deux `K`-faces sont connectées lorsqu'elles sont contenues dans une même coface de cardinal `K+1`.

### 1.2 Modèle exact régularisé utilisé ici

L'algorithme commence par remplacer chaque point `x_i` par un site régularisé `z_i` avec un coût additif `a_i >= 0`. On définit :

```math
\Phi_i(y) = \|y - z_i\|^2 + a_i.
```

Le rayon de rang régularisé dur est :

```math
R_K(y) = K\text{-ième plus petite valeur de } \{\Phi_i(y)\}_{i=1}^n.
```

Les niveaux du modèle régularisé dur sont :

```math
L_{K,\eta}(t) = \{y : R_K(y) \le t\}
              = \{y : \#\{i : \Phi_i(y) \le t\} \ge K\}.
```

Lorsque `a_i = 0` et `z_i = x_i`, on retrouve le modèle K-NN classique, à la convention `t = r^2` près.

### 1.3 Ce qui est exact dans PERG-HGP

PERG-HGP est exact sur les points suivants :

1. Le Gibbs local avec `eta_i` résout exactement le problème entropique local choisi, à tolérance numérique près.
2. Les sites `(z_i, a_i)` définissent exactement une distance de puissance :

   ```math
   W_2^2(\delta_y, \nu_i) = \|y - z_i\|^2 + a_i.
   ```

3. Le top-`k` canonique entropique est une vraie régularisation de Gibbs du top-`k` dur.
4. La coquille de rang `b^{(k)}` est exactement le gradient du rang soft par rapport aux énergies.
5. Les témoins critiques soft sont les points fixes du gradient nul du champ de rang soft.
6. Une coface qui passe un test Gabriel global est un vrai simplexe de Gabriel du modèle régularisé dur.
7. Le MST du graphe dual d'atlas préserve exactement les composantes de tous les sous-niveaux de cet atlas.

### 1.4 Ce qui n'est pas exact dans le mode massif

Les étapes suivantes sont heuristiques :

1. la construction ANN ou grille tronquée des voisinages ;
2. le choix fini des témoins ;
3. le beam search rang par rang ;
4. les filtres locaux Gabriel ;
5. l'hypothèse que les cofaces Gabriel utiles ont toutes été générées.

Le mode massif doit donc annoncer clairement :

```text
Cette méthode calcule une hiérarchie exacte de l'atlas généré.
Elle n'est pas une preuve d'exhaustivité de HGP complet, sauf si l'audit cut-complete est activé et réussi.
```

---

## 2. Paramètres recommandés pour `n = 30M`, `p = 3`, `K = 10` ou `20`

### 2.1 Paramètres principaux

```yaml
K: 10                  # ou 20
K_rho: auto            # max(32, 3*K), plafonné par m_local
m_local: 128           # voisins locaux pour Gibbs et active sets
m_active: 128          # active set témoin, peut être 192 pour K=20
alpha: 0.0             # 0.0 pour densité Hartigan ; 0.25 optionnel ; éviter 1.0 pour clustering de densité
rank_relaxation: canonical_first_then_fermi_if_needed
rank_eps_schedule: [1.0, 0.5, 0.25, 0.125]  # multiplié par l'échelle locale médiane
support_cap: 4         # en 3D : supports actifs jusqu'à tétraèdres
beam_per_bucket: 4     # 2 à 8 selon VRAM
witness_budget_per_rank: 2_000_000  # ajuster à la VRAM
local_gabriel: true
global_gabriel: selective
cut_audit: optional
```

### 2.2 Paramètres mémoire

```yaml
use_float16_distances: true
store_neighbor_graph: true_if_vram_ge_48gb_else_memmap
store_q_weights: false
store_z_a_eta: true
candidate_coface_dtype: uint32
facet_hash: uint128_two_uint64
edge_weight_dtype: float32
```

### 2.3 Paramètres Colab

Le code doit détecter la machine au démarrage :

```python
import torch, subprocess
print(torch.cuda.get_device_name(0))
print(torch.cuda.get_device_properties(0).total_memory / 2**30, "GB")
```

Ensuite :

```python
if vram_gb >= 80:
    profile = "g4_96gb_or_a100_80gb"
elif vram_gb >= 40:
    profile = "a100_40gb"
elif vram_gb >= 24:
    profile = "l4_24gb"
else:
    profile = "t4_16gb_out_of_core"
```

Le mode cible supposé ici est `g4_96gb_or_a100_80gb`. Il faut cependant coder tous les chemins out-of-core. Colab adore remplacer le matériel sous vos pieds avec la délicatesse d'un serveur capricieux.

---

## 3. Architecture logicielle recommandée

```text
hgp_entropy/
  pyproject.toml
  setup.py
  README.md

  hgp_entropy/
    __init__.py
    config.py
    io.py
    normalize.py
    grid.py
    local_gibbs.py
    witnesses.py
    rank_field.py
    cofaces.py
    dual_graph.py
    hierarchy.py
    diagnostics.py

  hgp_entropy_cuda/
    bindings.cpp
    morton.cu
    grid_build.cu
    knn_grid.cu
    eta_solver.cu
    barycenter.cu
    rank_dp.cu
    witness_update.cu
    miniball3d.cu
    coface_topk.cu
    gabriel_grid.cu
    facet_hash.cu
    union_find.cu
    boruvka.cu

  hgp_entropy_cython/
    external_sort.pyx
    memmap_utils.pyx
    cpu_union_find.pyx

  notebooks/
    00_check_gpu.ipynb
    01_preprocess_grid.ipynb
    02_local_gibbs.ipynb
    03_witnesses.ipynb
    04_cofaces_and_hierarchy.ipynb

  tests/
    test_miniball3d.py
    test_rank_dp.py
    test_eta_solver.py
    test_facet_hash.py
    test_small_exact_hgp.py
```

### 3.1 Dépendances Python

Minimum :

```text
numpy
scipy
numba
cupy-cuda12x ou torch
pybind11
cython
zarr
numcodecs
tqdm
```

Optionnel mais utile :

```text
faiss-gpu        # seulement si disponible ; en 3D la grille maison suffit souvent mieux
rapids-cudf      # pour sort/groupby GPU si l'environnement le permet
rapids-cugraph   # optionnel pour MST/Boruvka, mais ne pas en dépendre
```

Le cœur doit rester indépendant de RAPIDS. RAPIDS est confortable jusqu'au moment où Colab décide que non, finalement, aujourd'hui ce sera un environnement nu comme un caillou.

---

## 4. Structures de données

### 4.1 Points

Pour `n = 30M`, ne pas stocker les points en `float64`.

```cpp
struct PointsSOA {
    float* x;      // length n
    float* y;      // length n
    float* z;      // length n
};
```

En Python :

```python
X = {
    "x": np.memmap("x.f32", dtype="float32", mode="r+", shape=(n,)),
    "y": np.memmap("y.f32", dtype="float32", mode="r+", shape=(n,)),
    "z": np.memmap("z.f32", dtype="float32", mode="r+", shape=(n,)),
}
```

Pourquoi SoA et pas AoS ? Parce que les kernels lisent souvent une coordonnée à la fois. Les accès coalescés valent mieux que les structures jolies. Encore une victoire de la plomberie sur l'esthétique.

### 4.2 Grille spatiale

On normalise les points dans une boîte `[0,1]^3`, puis on calcule une clé Morton 64 bits.

```cpp
uint64_t* morton_key;     // length n
uint32_t* sorted_index;   // length n
uint64_t* cell_key;       // length n_cells
uint64_t* cell_start;     // length n_cells
uint32_t* cell_count;     // length n_cells
```

La grille doit être adaptative en pratique : si certaines cellules contiennent trop de points, on subdivise localement ou on utilise une grille plus fine pour les zones denses.

### 4.3 Voisinages locaux

Si VRAM suffisante :

```cpp
uint32_t* nbr_idx;   // shape [n, m_local]
uint16_t* nbr_d2h;   // shape [n, m_local], distance^2 quantifiée float16
```

Si VRAM limitée :

```text
neighbors.idx.u32.memmap
neighbors.d2.f16.memmap
```

Taille indicative pour `n = 30M`, `m = 128` :

```text
nbr_idx uint32  : 30M * 128 * 4  = 15.36 GB
nbr_d2 float16  : 30M * 128 * 2  =  7.68 GB
Total           : 23.04 GB
```

C'est possible sur 96 GB, pas sur T4 16 GB. Donc il faut coder le streaming.

### 4.4 Sites régularisés

```cpp
float* z_x;       // length n
float* z_y;
float* z_z;
float* a;         // variance additive, length n
float* eta;       // échelle locale, length n
float* rho;       // distance locale minimale, length n
```

Taille : environ `30M * 6 * 4 = 720 MB`. Très acceptable.

### 4.5 Témoins

```cpp
struct WitnessPool {
    float* wx;             // M
    float* wy;
    float* wz;
    float* score;          // M
    uint8_t* rank;         // M
    uint64_t* sig1;        // M, hash top signature
    uint64_t* sig2;        // M, second hash
    uint32_t* top_idx;     // optional shape [M, Kmax+2]
};
```

`top_idx` peut être stocké seulement pour les témoins finalistes.

### 4.6 Cofaces candidates

Pour `K = 20`, une coface contient `21` indices.

```cpp
uint32_t* coface_idx;    // shape [C, K+1]
float* coface_weight;    // rho or rho^2
float* coface_cx;        // optional center x
float* coface_cy;
float* coface_cz;
uint32_t* flags;         // local/global Gabriel, valid, rejected, etc.
```

Taille pour `C = 2M`, `K=20` :

```text
coface_idx: 2M * 21 * 4 = 168 MB
weights + center + flags: < 50 MB
```

### 4.7 Facettes hashées pour graphe dual

Chaque coface de taille `K+1` produit `K+1` facettes de taille `K`.

```cpp
struct FacetRecord {
    uint64_t hash1;
    uint64_t hash2;
    uint32_t coface_id;
    uint8_t  dropped_pos;
};
```

Après tri et unique :

```cpp
uint64_t* facet_id_for_record;  // same length as records
```

Les collisions de hash doivent être rares avec deux `uint64`, mais pour le mode strict on garde une vérification collisionnelle sur échantillon ou sur buckets suspects.

### 4.8 Graphe dual d'atlas

On stocke une étoile par coface.

```cpp
uint64_t* edge_u;       // length C*K
uint64_t* edge_v;       // length C*K
float* edge_w;          // length C*K
uint32_t* edge_coface;  // length C*K
```

Pour `C=2M`, `K=20`, cela donne `40M` arêtes. Faisable sur 96 GB. Pas drôle sur 16 GB. Rien n'est drôle sur 16 GB avec 30M de points, à part fermer l'onglet.

---

## 5. Étape 1 : normalisation et grille spatiale

### 5.1 Normalisation

```python
bbox_min = X.min(axis=0)
bbox_max = X.max(axis=0)
scale = max(bbox_max - bbox_min)
Xn = (X - bbox_min) / scale
```

Stocker `bbox_min` et `scale` pour revenir aux unités originales.

### 5.2 Choix de la taille de cellule

Prendre un échantillon `S` de `1M` points si possible, sinon `200k`.

1. Construire une grille provisoire.
2. Estimer les distances au `K_rho`-ième voisin sur l'échantillon.
3. Poser :

```text
cell_size = median(r_Krho_sample) / 2
```

Puis ajuster pour que le nombre moyen de candidats dans un voisinage `3x3x3` ou `5x5x5` soit entre `2*m_local` et `8*m_local`.

### 5.3 Morton sort

Kernel : `morton_encode.cu`.

```cpp
morton_key[i] = morton3D_quantized(Xn[i], bits_per_axis=21);
```

Puis tri par clé :

```text
thrust::sort_by_key(morton_key, sorted_index)
```

Construire ensuite `cell_key`, `cell_start`, `cell_count` par réduction sur clés triées.

---

## 6. Étape 2 : voisinages locaux GPU

### 6.1 Recherche top-`m` par grille

Pour chaque point `i`, rechercher les candidats dans les cellules autour de sa cellule. Augmenter le rayon de cellules jusqu'à obtenir au moins `m_local` candidats.

Pseudo-code kernel :

```cpp
for point i in parallel:
    heap = FixedTopK<m_local>()
    radius_cells = 1
    while heap.candidate_count < min_candidates and radius_cells <= max_radius:
        for neighbor_cell in cube(cell(i), radius_cells):
            for j in points_of_cell(neighbor_cell):
                if j != i:
                    d2 = squared_distance(i, j)
                    heap.push(j, d2)
        radius_cells += 1
    write top m_local neighbors
```

En 3D, ce kernel doit être fortement optimisé :

- points triés par cellules ;
- un bloc traite une cellule ou un paquet de points de cellule ;
- shared memory pour les points des cellules voisines ;
- top-k partiel en registres ;
- distances en `float32`, stockage `float16` optionnel.

### 6.2 Certificat de queue local

Pour un point ou témoin, on peut certifier qu'un active set est assez grand si la plus petite distance possible vers les cellules non visitées dépasse le dernier voisin retenu avec une marge :

```math
E_{tail} - E_{K+2} > \tau_{tail}\,\varepsilon.
```

Recommandation :

```yaml
tail_margin_tau: 12.0
```

Cela ne donne pas une exactitude absolue pour toute la suite, mais donne un contrôle numérique utile : la masse Gibbs omise est négligeable.

---

## 7. Étape 3 : Gibbs local avec échelle `eta_i`

### 7.1 Coût local UMAP-like

Pour chaque point `i`, avec voisins `j in A_i` :

```math
\rho_i = \min_{j \ne i} \|x_i - x_j\|.
```

Coût recommandé :

```math
c_{ij} = \max(0, \|x_i - x_j\| - \rho_i)^2.
```

On résout :

```math
q_{ij}(\eta_i) = \frac{\exp(-c_{ij}/\eta_i)}{\sum_{\ell \in A_i}\exp(-c_{i\ell}/\eta_i)}
```

avec :

```math
\exp(H(q_i)) = K_\rho.
```

où :

```math
H(q_i) = -\sum_j q_{ij}\log q_{ij}.
```

### 7.2 Choix de `K_rho`

Recommandation :

```python
K_rho = min(m_local, max(32, 3*K))
```

Donc :

```text
K=10 -> K_rho=32
K=20 -> K_rho=60
```

### 7.3 Solver GPU de `eta_i`

Bisection parallèle :

```cpp
for each i in parallel:
    lo = eta_min
    hi = eta_max
    for it in range(32):
        mid = 0.5*(lo+hi)
        q = softmax(-c_i / mid)
        eff = exp(entropy(q))
        if eff < K_rho: lo = mid
        else: hi = mid
    eta_i = hi
```

Pour accélérer :

- utiliser `logsumexp` ;
- commencer par `eta_max = max(c_i) * 100 + eps` ;
- si les distances sont toutes nulles ou quasi nulles, poser `eta_i = eps` et `q` uniforme sur les voisins à distance nulle.

### 7.4 Calcul des sites régularisés

```math
z_i = \sum_{j\in A_i} q_{ij} x_j.
```

```math
a_i = \sum_{j\in A_i} q_{ij}\|x_j-z_i\|^2.
```

Formule stable :

```math
a_i = \sum_j q_{ij}\|x_j\|^2 - \|z_i\|^2.
```

Ne pas stocker `q_ij` sauf mode debug. Recalculer au besoin par blocs.

---

## 8. Étape 4 : énergie finale

Pour la hiérarchie de densité, utiliser par défaut :

```math
\Phi_i(y)=\|y-z_i\|^2+a_i.
```

C'est `alpha = 0`.

Optionnel :

```math
\Phi_i(y)=\frac{\|y-z_i\|^2+a_i}{\eta_i^\alpha}.
```

Recommandation :

```yaml
alpha: 0.0
```

Pourquoi ? Parce que `alpha > 0` uniformise les densités comme UMAP, ce qui est utile pour la réduction de dimension mais dangereux pour une hiérarchie de forte densité. Si l'utilisateur veut vraiment corriger une densité d'échantillonnage très inhomogène, autoriser :

```yaml
alpha: 0.25
```

mais éviter `alpha = 1` pour clustering de densité.

---

## 9. Étape 5 : champ de rang soft

### 9.1 Top-`k` canonique entropique

Pour un témoin `w`, active set `A_w`, et énergie `E_i = Phi_i(w)`, poser :

```math
u_i = \exp(-(E_i - E_0)/\varepsilon), \quad E_0 = \min_{i\in A_w}E_i.
```

La distribution canonique sur les sous-ensembles `Q` de taille `k` est :

```math
\pi_{k,\varepsilon}(Q|w)
= \frac{\prod_{i\in Q}u_i}{e_k(u)}.
```

Le libre-énergie est :

```math
F_{k,\varepsilon}(w) = kE_0 - \varepsilon\log e_k(u).
```

Le rang soft est :

```math
Q_{k,\varepsilon}(w) = F_{k,\varepsilon}(w)-F_{k-1,\varepsilon}(w).
```

À basse température :

```math
Q_{k,\varepsilon}(w) \to \Phi_{(k)}(w).
```

### 9.2 DP pour `e_k`

```cpp
E[0] = 1
E[1..Kmax] = 0
for i in active_set:
    for l from min(current_count,Kmax) down to 1:
        E[l] += u_i * E[l-1]
```

En production, utiliser une version rescalée ou log-domain.

Pour `m=128`, `Kmax=22`, une DP normalisée en `float32` avec rescaling par ligne est plus rapide que du pur log-domain.

### 9.3 Marges d'inclusion

```math
p_i^{(k)}(w)=\frac{u_i e_{k-1}(u_{-i})}{e_k(u)}.
```

On calcule `e_{k-1}(u_{-i})` par forward/backward DP :

```text
prefix[t,l] = e_l(u_1,...,u_t)
suffix[t,l] = e_l(u_t,...,u_m)
```

Puis :

```math
e_{k-1}(u_{-i}) = \sum_{l=0}^{k-1} prefix[i-1,l] suffix[i+1,k-1-l].
```

### 9.4 Coquille de rang

```math
b_i^{(k)}(w)=p_i^{(k)}(w)-p_i^{(k-1)}(w).
```

Clamp numérique :

```cpp
b_i = max(0, b_i)
b_i /= sum(b_i)
```

La coquille est le vrai moteur de l'algorithme.

---

## 10. Étape 6 : témoins critiques soft

Un témoin critique soft de rang `k` vérifie :

```math
w = T_k(w) = \sum_i b_i^{(k)}(w)z_i
```

pour `alpha = 0`.

Avec `alpha > 0` :

```math
T_k(w)=\frac{\sum_i b_i^{(k)}(w)\eta_i^{-\alpha}z_i}{\sum_i b_i^{(k)}(w)\eta_i^{-\alpha}}.
```

### 10.1 Kernel update

```cpp
for each witness w in parallel:
    A = top_m(Phi_i(w))
    compute b_i^k on A
    T = weighted_sum(b_i, z_i)
    w_new = (1-gamma)*w + gamma*T
```

Recommandations :

```yaml
fixed_point_iters_per_temp: 4
refresh_active_set_every: 2
gamma: 0.8
```

### 10.2 Score de témoin

Score recommandé :

```math
S_k(w)=
 -\lambda_c\|w-T_k(w)\|^2
 +\lambda_h\,\mathrm{shape}(s_{eff})
 +\lambda_g\,\Delta_k(w)
 -\lambda_r\,\mathrm{redundancy}(w).
```

Avec :

```math
s_{eff}=\exp\left(-\sum_i b_i\log b_i\right).
```

En 3D, un support actif utile a taille effective entre `2` et `4`.

```python
def shape_score(s_eff):
    # maximum near 3, accepts [2,4]
    return -((s_eff - 3.0)**2) / (2*1.0**2)
```

Gap :

```math
\Delta_k(w)=Q_{k+1,\varepsilon}(w)-Q_{k,\varepsilon}(w).
```

Redondance :

```text
same top signature hash -> keep best only
same spatial bucket -> keep top B
high overlap of top-(K+1) -> suppress lower score
```

---

## 11. Étape 7 : boucle progressive en `k`

### 11.1 Initialisation `W_1`

Modes :

```yaml
init_mode: all_points_streamed | balanced_landmarks | grid_barycenters
```

Pour `n=30M`, recommandé :

```yaml
init_mode: all_points_streamed
```

mais cela ne signifie pas garder `30M` témoins en mémoire. On procède ainsi :

1. traiter les points par chunks ;
2. appliquer 2 ou 3 itérations de point fixe de rang `1` ;
3. calculer signature + score ;
4. dédupliquer par bucket spatial et signature ;
5. garder `W_1` avec budget borné.

Recommandation :

```yaml
W1_budget: 2_000_000 to 5_000_000
```

### 11.2 Générateurs pour `W_{k+1}`

Pour chaque témoin `w in W_k`, produire des seeds pour le rang suivant.

#### Générateur A : continuation

```text
seed = w
```

#### Générateur B : shell lift

Calculer `b^{(k+1)}` sur le même active set. Prendre les `L_shell` sites les plus forts.

```yaml
L_shell: 4
```

Créer :

```math
w' = \frac{\sum_{i\in S} \omega_i z_i}{\sum_{i\in S}\omega_i}
```

avec `S` composé des sites forts de la coquille `k` et d'un site de la coquille `k+1`.

#### Générateur C : supports actifs durs

Former de petits supports `S` parmi les sites forts de :

```text
top shell k
+
top shell k+1
+
top low-energy active set
```

En 3D :

```text
|S| = 2, 3, 4
```

Calculer le centre de miniball/power-ball `c_S`. Utiliser `c_S` comme seed.

#### Générateur D : rank-capacity

Pour un support `S`, calculer :

```math
t_S = \max_{i\in S}\Phi_i(c_S).
```

Comptage mou :

```math
R_\varepsilon(S)=\sum_{j\in A(c_S)}\sigma\left(\frac{t_S-\Phi_j(c_S)}{\varepsilon}\right).
```

Garder si :

```math
|R_\varepsilon(S)-(k+1)| \le \tau_R.
```

Cela évite de générer des supports qui regardent déjà trop de points ou pas assez.

### 11.3 Dédoublonnage et beam

Après génération :

1. calculer `Top_{min(k+2,K+2)}` comme signature ;
2. hash double 64-bit ;
3. radix sort par `(sig1, sig2)` ;
4. garder le meilleur score par signature ;
5. bucket spatial ;
6. garder top `B` par bucket.

---

## 12. Étape 8 : annealing

Schéma recommandé :

```yaml
eps_schedule_mode: local_scale_median
eps_factors: [1.0, 0.5, 0.25, 0.125]
```

Définir l'échelle :

```math
\varepsilon_{base} = \mathrm{median}_w\left(\Phi_{(K+2)}(w)-\Phi_{(1)}(w)\right)/K.
```

Puis :

```math
\varepsilon_t = factor_t \cdot \varepsilon_{base}.
```

À chaque température :

```text
for k in 1..K+1:
    refine W_k
    deduplicate
    lift to W_{k+1}
```

On peut aussi faire :

```text
for k in 1..K+1:
    for eps in schedule:
        refine W_k at eps
    lift
```

Recommandation pour implémentation :

```text
outer loop on k, inner loop on eps
```

C'est plus simple pour contrôler le budget de témoins.

---

## 13. Étape 9 : cofaces finales

Pour chaque témoin final `w in W_{K+1}` :

```text
sigma = Top_{K+1}(Phi_i(w))
```

Dédoublonner `sigma` par tri des indices + hash.

### 13.1 Miniball pondérée en 3D

Pour `sigma` de taille `K+1`, `K=10` ou `20`, ne pas énumérer tous les sous-ensembles. En 3D, le support actif d'une plus petite boule a taille au plus 4 dans le cas ordinaire/power-ball.

Algorithme recommandé : active-set miniball 3D.

Pseudo-code :

```cpp
Input sigma indices
Initialize support S with farthest pair or top shell support
repeat:
    c, t = solve_support_ball(S)
    j = argmax_{i in sigma} Phi_i(c) - t
    if violation <= tol: return c,t,S
    S = update_support(S, j)  // keep minimal active support, size <= 4
```

Pour robustesse, fournir aussi une version brute pour supports de taille `<=4` parmi un petit sous-ensemble candidat :

```text
support_candidates = top violating points + current active support
```

### 13.2 Cohérence top

Après centre `c_sigma` :

```text
top = Top_{K+1}(Phi_i(c_sigma))
keep if top == sigma
```

---

## 14. Étape 10 : test Gabriel

### 14.1 Test local rapide

Tester sur :

```text
A(c_sigma) = Top_m(Phi_i(c_sigma))
```

Si un intrus `j notin sigma` a :

```math
\Phi_j(c_sigma) < \rho(\sigma)^2 - tol
```

rejeter.

### 14.2 Test global exact par grille 3D pour `alpha = 0`

Si :

```math
\Phi_j(c)=\|c-z_j\|^2+a_j
```

et l'on veut vérifier :

```math
\|c-z_j\|^2+a_j < \rho^2,
```

alors nécessairement :

```math
\|c-z_j\|^2 < \rho^2.
```

Donc il suffit de visiter les cellules de la grille des `z_j` dans la boule de rayon `rho` autour de `c`. C'est un test exact, si la grille contient tous les sites.

Kernel : `gabriel_grid.cu`.

```cpp
for candidate coface sigma in parallel:
    c = center[sigma]
    rho2 = weight[sigma]
    for cell in cells_intersecting_ball(c, sqrt(rho2)):
        for j in cell:
            val = squared_distance(c, z_j) + a_j
            if val < rho2 - tol and j notin sigma:
                reject
    accept
```

Pour `K=20`, tester `j notin sigma` via une petite recherche linéaire dans les 21 indices.

### 14.3 Si `alpha > 0`

Le test global devient :

```math
\frac{\|c-z_j\|^2+a_j}{\eta_j^\alpha}<\rho^2.
```

Le rayon spatial maximal dépend de `eta_j`. Utiliser un rayon conservatif :

```math
\|c-z_j\|^2 < \rho^2\eta_{max}^\alpha.
```

Cela peut devenir très large. Pour cette raison, le mode `alpha > 0` est déconseillé pour certification globale Gabriel.

---

## 15. Étape 11 : graphe dual d'atlas

### 15.1 Facettes

Chaque coface `sigma` de taille `K+1` produit `K+1` facettes de taille `K` :

```text
facet_r = sigma without sigma[r]
```

On calcule un hash déterministe du tableau trié `facet_r`.

Hash recommandé : deux fonctions 64-bit indépendantes.

```cpp
hash1 = splitmix64 over indices with seed A
hash2 = splitmix64 over indices with seed B
```

Créer `FacetRecord(hash1, hash2, coface_id, dropped_pos)`.

### 15.2 Assignation des IDs de facettes

Trier tous les `FacetRecord` par `(hash1, hash2)`.

Puis unique :

```text
new facet id whenever hash changes
```

Option stricte : pour buckets avec même hash et taille > 1, vérifier les indices exacts des facettes si collision suspecte. En pratique, deux `uint64` suffisent largement, mais la paranoïa contrôlée est une vertu logicielle.

### 15.3 Arêtes duales

Pour chaque coface `sigma`, récupérer ses `K+1` `facet_id`. Choisir un pivot, par exemple la facette obtenue en supprimant le plus petit index.

Ajouter :

```text
pivot -> facet_j, weight = rho(sigma), coface_id = sigma
```

pour `j = 1..K`.

Cela préserve les composantes de sous-niveaux induites par les cofaces de l'atlas.

---

## 16. Étape 12 : MST / Boruvka GPU

### 16.1 Kruskal si le graphe tient en mémoire

1. Trier `edge_w`.
2. Union-Find GPU/CPU hybride.
3. Écrire les fusions.

### 16.2 Boruvka sinon

Boruvka est plus streaming-friendly :

```text
while number_of_components decreases:
    for each component, find cheapest outgoing edge
    union all selected edges
```

Pour un atlas de `E = C*K` arêtes, Boruvka évite parfois de trier tout `E` si on lit les arêtes par chunks.

### 16.3 Sortie merge tree

Stocker :

```cpp
uint64_t* merge_left;
uint64_t* merge_right;
float* merge_weight;
uint64_t* merge_size;
uint32_t* merge_coface;
```

La hiérarchie sur l'atlas est définie par ce merge tree.

---

## 17. Attribution des points aux clusters

La hiérarchie primaire vit sur les `K`-facettes, pas directement sur les points.

Pour produire des clusters de points à un seuil `t` :

1. couper le MST dual à `t` ;
2. obtenir les composantes de facettes ;
3. pour chaque coface active, ajouter ses points à la composante de ses facettes ;
4. un point peut appartenir à plusieurs clusters : c'est un recouvrement, pas une partition.

Mode partition stricte optionnel :

```text
point label = cluster maximizing membership_score(point, cluster)
```

Score simple :

```math
score(i,C)=\sum_{\sigma\in C, i\in\sigma}\exp(-\rho(\sigma)/\tau)
```

---

## 18. Exactitude de la hiérarchie finale

### 18.1 Exacte pour l'atlas

Une fois les cofaces acceptées et le graphe dual construit, le MST est exact pour cet atlas.

Pour tout seuil `t` :

```text
components(MST_atlas <= t) == components(dual_graph_atlas <= t)
```

### 18.2 Exacte pour HGP complet seulement sous condition

La hiérarchie coïncide avec le HGP régularisé complet si l'atlas contient toutes les cofaces Gabriel nécessaires aux fusions, ou plus faiblement si l'atlas est cut-complet : pour chaque coupe de composantes, il contient au moins une coface sortante de poids minimal.

### 18.3 Audit de coupe optionnel

Pour chaque fusion importante dans le MST :

1. identifier les facettes/composantes de part et d'autre de la coupe ;
2. relancer PERG localement autour des témoins et cofaces proches de cette coupe ;
3. rechercher une coface plus légère non trouvée ;
4. si aucune n'est trouvée avec certificat de queue, marquer la coupe comme certifiée.

Ce mode est cher. Il doit être réservé aux niveaux importants, pas à toutes les micro-fusions. Même l'exactitude a besoin d'un budget.

---

## 19. Modes d'exécution recommandés

### 19.1 Mode massif par défaut

```yaml
mode: atlas_soft_rank_certified_local
alpha: 0.0
rank_relaxation: canonical
local_gabriel: true
global_gabriel: selective
cut_audit: false
```

Sortie : hiérarchie exacte de l'atlas, cofaces localement certifiées, Gabriel global pour les cofaces retenues dans le MST ou dans les niveaux demandés.

### 19.2 Mode plus exact

```yaml
mode: atlas_soft_rank_global_gabriel
alpha: 0.0
rank_relaxation: canonical
global_gabriel: true
cut_audit: partial
```

Sortie : cofaces Gabriel globales vraies ; complétude non garantie sans audit complet.

### 19.3 Mode Colab faible VRAM

```yaml
mode: out_of_core_landmark
init_mode: balanced_landmarks
W1_budget: 500_000
m_local: 64
m_active: 64
rank_relaxation: fermi_first_then_canonical_final
candidate_budget: 500_000
```

Sortie : approximation plus agressive, toujours fondée sur le même modèle.

---

## 20. Pseudo-code global

```python
def perg_hgp_3d_massive(points_path, K=20, config=None):
    cfg = load_config(config)

    # 1. Load / normalize / grid
    X = load_points_memmap(points_path)
    norm = normalize_points_inplace(X)
    grid = build_morton_grid_gpu(X, cfg.grid)

    # 2. Local neighborhoods
    nbr_idx, nbr_d2 = build_grid_knn_gpu(X, grid, m=cfg.m_local,
                                         out_of_core=cfg.out_of_core)

    # 3. Local UMAP-like Gibbs
    eta, rho = solve_local_eta_gpu(nbr_d2, K_rho=cfg.K_rho)
    z, a = compute_regularized_sites_gpu(X, nbr_idx, nbr_d2, eta, rho)

    # Optional: Sinkhorn balancing on neighbor graph
    if cfg.sinkhorn.enabled:
        P = sparse_sinkhorn_neighbor_graph(nbr_idx, nbr_d2, eta, cfg.sinkhorn)
        z, a = compute_sites_from_sparse_P(X, nbr_idx, P)

    # 4. Build grid on regularized sites z
    z_grid = build_morton_grid_gpu(z, cfg.grid_z)

    # 5. Initialize witnesses W_1
    W = init_witnesses_streamed(z, a, eta, z_grid, rank=1, cfg=cfg)

    # 6. Progressive ranks
    witness_by_rank = {1: W}
    for k in range(1, K+2):
        Wk = witness_by_rank[k]
        for eps in cfg.rank_eps_schedule:
            Wk = refine_witnesses_rank_gpu(Wk, z, a, eta, z_grid,
                                            k=k, eps=eps, cfg=cfg)
            Wk = dedupe_and_beam_gpu(Wk, k=k, cfg=cfg)
        witness_by_rank[k] = Wk

        if k < K+1:
            seeds = lift_witnesses_gpu(Wk, z, a, eta, z_grid,
                                        k=k, cfg=cfg)
            witness_by_rank[k+1] = merge_seed_pool(
                witness_by_rank.get(k+1), seeds, cfg
            )

    # 7. Final cofaces
    Wfinal = witness_by_rank[K+1]
    cofaces = extract_top_cofaces_gpu(Wfinal, z, a, eta, z_grid, K=K, cfg=cfg)
    cofaces = dedupe_cofaces_gpu(cofaces, K=K)

    # 8. Exact miniball / power-ball for cofaces
    cofaces = compute_coface_rho_centers_gpu(cofaces, z, a, eta, cfg=cfg)

    # 9. Gabriel tests
    cofaces = local_gabriel_filter_gpu(cofaces, z, a, eta, z_grid, cfg=cfg)
    if cfg.global_gabriel in ["all", "selective"]:
        cofaces = global_gabriel_grid_gpu(cofaces, z, a, eta, z_grid, cfg=cfg)

    # 10. Dual graph
    facets = build_facet_records_gpu(cofaces, K=K)
    facet_ids = assign_facet_ids_gpu(facets)
    edges = build_dual_edges_gpu(cofaces, facet_ids, K=K)

    # 11. MST/Boruvka
    merge_tree = dual_graph_mst_gpu(edges, cfg=cfg)

    # 12. Optional cut audit
    if cfg.cut_audit.enabled:
        merge_tree, cofaces = cut_audit_refinement(
            merge_tree, cofaces, z, a, eta, z_grid, cfg
        )

    return HGPEntropyResult(
        norm=norm,
        sites=(z, a, eta),
        cofaces=cofaces,
        merge_tree=merge_tree,
        config=cfg,
    )
```

---

## 21. Tests unitaires indispensables

### 21.1 Small exact HGP

Créer un petit nuage de 50 à 200 points en 2D/3D.

Comparer :

1. Čech complet brute force ;
2. Gabriel complet brute force ;
3. PERG-HGP avec budgets très grands.

Vérifier que PERG retrouve les cofaces Gabriel sur des cas simples.

### 21.2 DP top-k

Comparer les marges `p_i^(k)` obtenues par DP à une énumération brute pour `m <= 12`.

### 21.3 Coquille de rang

Vérifier :

```python
assert np.all(b >= -tol)
assert abs(b.sum() - 1) < tol
```

Comparer `b` à une différence finie du rang soft.

### 21.4 Miniball 3D

Comparer le kernel `miniball3d.cu` à une implémentation CPU brute force sur supports de taille `<=4`.

### 21.5 Gabriel global par grille

Comparer `global_gabriel_grid_gpu` à un scan complet sur petits nuages.

### 21.6 Facet hashing

Tester collisions artificielles. Vérifier que deux facettes égales ont même ID et que des facettes différentes n'ont pas le même ID en mode strict.

---

## 22. Diagnostics à produire

À chaque exécution, écrire :

```text
n_points
bbox
scale
gpu_name
vram_gb
m_local
K
K_rho
alpha
number_of_cells
avg_cell_count
max_cell_count
neighbor_tail_fail_rate
eta_min_median_max
a_min_median_max
witness_count_by_rank
candidate_cofaces_before_dedupe
candidate_cofaces_after_dedupe
local_gabriel_accept_rate
global_gabriel_accept_rate
num_facets
num_dual_edges
mst_edges
cut_audit_pass_rate
```

Enregistrer aussi des histogrammes :

- `eta_i` ;
- `a_i` ;
- scores des témoins ;
- gaps de rang ;
- rayons des cofaces ;
- tailles de composantes à différents seuils.

Sans diagnostics, l'algorithme deviendra une boîte noire. Et une boîte noire de 30 millions de points est juste une poubelle très chère.

---

## 23. Recommandations d'implémentation Colab

### 23.1 Stockage

Utiliser `/content` pour les fichiers temporaires rapides si l'espace le permet. Utiliser Google Drive seulement pour sauvegarder les résultats finaux ; Drive est trop lent pour servir de scratch intensif.

Format recommandé :

```text
.zarr pour gros tableaux chunkés
.npy memmap pour tableaux simples
```

### 23.2 Compilation

Utiliser `torch.utils.cpp_extension.load` pour compiler les kernels CUDA au runtime :

```python
from torch.utils.cpp_extension import load
hgp_cuda = load(
    name="hgp_entropy_cuda",
    sources=["bindings.cpp", "morton.cu", "knn_grid.cu", ...],
    extra_cuda_cflags=["-O3", "--use_fast_math"],
    extra_cflags=["-O3"],
)
```

Prévoir un cache dans :

```text
/content/hgp_build_cache
```

### 23.3 Reprise après crash

Chaque étape doit écrire un checkpoint :

```text
01_grid/
02_neighbors/
03_sites/
04_witness_rank_k/
05_cofaces/
06_dual_graph/
07_merge_tree/
```

Le code doit pouvoir reprendre depuis n'importe quelle étape. Colab peut déconnecter. Quelle surprise, un service interactif qui n'aime pas les calculs longs.

---

## 24. Configuration YAML cible

```yaml
project: perg_hgp_3d_30m

input:
  path: /content/data/points.npy
  dtype: float32
  layout: Nx3
  normalize: true

hardware:
  autodetect: true
  target_profile: auto
  min_free_vram_gb: 8
  use_out_of_core_if_vram_below_gb: 48

model:
  K: 20
  K_rho: auto        # max(32, 3*K)
  alpha: 0.0
  regularized_sites: true

neighbors:
  method: morton_grid
  m_local: 128
  candidate_cell_radius_max: 6
  store: memmap_if_needed
  distance_dtype: float16

local_gibbs:
  cost: shifted_squared_distance
  eta_solver: entropy_effective
  eta_bisection_iters: 32
  entropy_target: K_rho
  sinkhorn:
    enabled: false
    mode: column_cap
    epsilon_ot_factor: 1.0
    max_iter: 50
    tol: 1e-3

sites:
  store_z: true
  store_a: true
  store_eta: true
  grid_on_z: true

rank_field:
  relaxation: canonical
  fallback_relaxation: fermi
  m_active: 128
  Kmax_extra: 2
  eps_schedule_factors: [1.0, 0.5, 0.25, 0.125]
  fixed_point_iters: 4
  gamma: 0.8
  tail_margin_tau: 12.0

witnesses:
  init_mode: all_points_streamed
  W1_budget: 3000000
  budget_per_rank: 2000000
  beam_per_bucket: 4
  spatial_bucket_scale: auto
  signature_size_extra: 2
  dedupe_hash_bits: 128
  lift:
    continuation: true
    shell_lift: true
    shell_top_L: 4
    support_centers: true
    support_cap: 4
    rank_capacity: true

cofaces:
  final_rank: K+1
  dedupe: true
  max_candidates: 5000000
  miniball: active_set_3d
  top_consistency_check: true

certification:
  local_gabriel: true
  global_gabriel: selective
  global_gabriel_method: grid_exact_alpha0
  global_gabriel_for_mst_edges_only: true
  cut_audit:
    enabled: false
    max_rounds: 2
    local_refine_budget: 200000

dual_graph:
  facet_hash: uint128
  star_expansion: true
  mst_method: boruvka_if_edges_large_else_kruskal
  sort_edges: true

output:
  save_sites: true
  save_cofaces: true
  save_merge_tree: true
  save_diagnostics: true
  produce_flat_cuts: [auto]
```

---

## 25. Extensions futures

1. Ajouter `alpha > 0` avec certification conservatrice.
2. Ajouter un mode cubical soft sur octree pour comparer avec PERG-HGP.
3. Ajouter un audit cut-complete plus sérieux.
4. Ajouter un mode multi-GPU si la bibliothèque sort de Colab.
5. Ajouter une API de navigation dans l'arbre pour injecter des a priori géométriques, dans l'esprit de l'exploration guidée déjà utilisée avec HGP.

---

## 26. Phrase de documentation courte

`PERG-HGP` construit une approximation certifiante de HGP d'ordre élevé sur des nuages 3D massifs. Il remplace l'énumération des simplexes par des témoins critiques obtenus par une régularisation entropique du champ de rang. Les cofaces candidates sont certifiées par tests de Gabriel locaux ou globaux, puis condensées en une hiérarchie par MST du graphe dual d'atlas.

---

## 27. Point clé à ne pas oublier

Ne jamais écrire dans la documentation :

```text
PERG-HGP calcule exactement HGP complet pour 30M de points et K=20.
```

Écrire :

```text
PERG-HGP calcule exactement la hiérarchie de l'atlas généré et certifié.
Lorsque l'atlas est complet ou cut-complet, cette hiérarchie coïncide avec HGP complet.
En mode massif, l'algorithme est une approximation régularisée, GPU-friendly, avec certification locale ou sélective des cofaces.
```

C'est moins spectaculaire. C'est aussi beaucoup moins faux. Et, détail rarement apprécié à sa juste valeur, moins faux signifie parfois utilisable.
