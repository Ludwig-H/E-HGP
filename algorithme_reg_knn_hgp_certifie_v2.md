# Algorithme certifié pour HGP régularisé : Čech de puissance, Gabriel pondéré et `K`-MST GPU

## 0. Analyse honnête de la version précédente

La version précédente décrivait correctement le modèle régularisé par sites de puissance et distinguait déjà trois couches : modèle exact, génération de candidats, certification. C'était la bonne séparation. Mais elle donnait encore trop de place à la construction progressive de la mosaïque complète par témoins. C'est élégant sur le papier, mais ce n'est pas le meilleur cœur algorithmique pour HGP. Construire une mosaïque complète alors qu'on veut seulement une hiérarchie, c'est l'équivalent géométrique d'imprimer Internet pour retrouver une citation. Impressionnant, idiot, et mauvais pour la mémoire globale.

Le recentrage proposé ici est le suivant :

1. **Le modèle exact reste le même** : estimateur `K`-NN régularisé par atomes entropiques, niveaux de Hartigan comme régions de `K`-couverture par boules de puissance, complexe de Čech de puissance.
2. **La structure minimale n'est pas la mosaïque complète** : pour reconstruire la hiérarchie, il suffit des cofaces `K`-séparantes ; sous hypothèse de position générale, elles sont Gabriel pondérées. Le bon objet algorithmique est donc un **oracle de cofaces Gabriel pondérées**, pas une énumération de toutes les cellules de Voronoï d'ordre `K`.
3. **La mosaïque d'ordre supérieur devient un fournisseur de candidats**, utile surtout en petite dimension. Elle n'est pas l'objectif par défaut.
4. **Le top-`k` entropique et les témoins sont rétrogradés au statut qu'ils méritent** : générateurs GPU rapides de candidats. Ils ne certifient rien. L'exactitude vient des tests durs : miniball pondérée, Gabriel global, audit de coupe ou audit complet.
5. **La meilleure voie dépend du régime** :
   - petite dimension et sortie raisonnable : fournisseur exact de Gabriel via mosaïque/arrangement ou supports actifs de rang borné ;
   - petite ou moyenne dimension avec gros `n` : génération GPU par témoins + certification de coupe ;
   - grande dimension ou `K` grand : `Lazy Cut-Borůvka` sur le graphe dual, avec listes `β`, bornes inférieures et certificats locaux de queue.

Ce fichier est donc une version resserrée et plus fidèle à l'esprit des deux premières parties de la thèse : comme le Single-Linkage ne construit pas tous les graphes géométriques mais un MST, HGP régularisé ne doit pas construire toute la mosaïque d'ordre `K`, mais seulement un `K`-MST porté par des cofaces Gabriel pondérées.

---

## 1. Objectif

On veut construire la hiérarchie exacte, ou certifiée aussi loin que possible, des amas de forte densité de Hartigan d'un estimateur `K`-NN régularisé.

L'entrée est un nuage euclidien

\[
X=\{x_1,\ldots,x_n\}\subset\mathbb R^p.
\]

La sortie principale est une hiérarchie de recouvrements, représentée par un `K`-MST sur un graphe dual de faces de cardinal `K`.

L'objectif par défaut est :

```yaml
target: hgp_hierarchy
```

La construction complète de la mosaïque de Delaunay d'ordre `K` n'est qu'un mode secondaire :

```yaml
target: full_mosaic
```

Elle ne doit être activée que si la dimension est petite et si la taille de sortie est raisonnable.

---

## 2. Paramètres principaux

On utilise les paramètres suivants.

```yaml
K: integer                      # ordre final de l'estimateur K-NN et de HGP
kappa: float >= 1               # taille effective entropique de chaque atome local
p: integer                      # dimension ambiante
n: integer                      # nombre de points
regularization: entropy_local | sinkhorn_sparse
support_mode: full | knn
m_reg: integer                  # taille du support local si support_mode = knn
target: hgp_hierarchy | full_mosaic
geometry_mode: auto | lowdim_exact | lowdim_gpu_certified | highdim_lazy
audit: none | cut | exhaustive_cell
candidate_engine: exact_support | witness_soft_topk | beta_lazy | hybrid
soft_topk: canonical | fermi
rho_solver: support_enum | active_set
gabriel_test: global | local_then_global
mst_solver: boruvka | kruskal
precision_generation: float32 | tf32 | float64
precision_certification: float64
```

Choix recommandé par défaut :

```yaml
target: hgp_hierarchy
geometry_mode: auto
regularization: entropy_local
support_mode: knn
soft_topk: canonical_if_K_le_20_else_fermi
candidate_engine: hybrid
audit: cut
rho_solver: active_set
gabriel_test: local_then_global
mst_solver: boruvka
precision_generation: float32
precision_certification: float64
```

Le résultat doit toujours inclure un rapport d'exactitude explicite :

```yaml
exactness_report:
  model_exact_for_chosen_supports: true | false
  accepted_cofaces_are_true_gabriel: true | false
  full_mosaic_complete: true | false | unknown
  hgp_hierarchy_complete: true | false | unknown
  cut_certificates_missing: integer
  cell_audits_missing: integer
  gabriel_global_tests_done: integer
  gabriel_local_only_count: integer
  mode_used: lowdim_exact | lowdim_gpu_certified | highdim_lazy
```

---

## 3. Modèle exact : atomes entropiques et sites de puissance

### 3.1 Supports locaux

Pour chaque point `x_i`, on choisit un support local `A_i` contenant `i`.

Deux modes sont autorisés :

```yaml
support_mode: full
```

avec

\[
A_i=\{1,\ldots,n\},
\]

ou

```yaml
support_mode: knn
```

avec `A_i` égal au voisinage `m_reg`-NN de `x_i`, incluant `i`.

Important : si `support_mode = knn`, l'algorithme est exact pour le modèle régularisé localisé ainsi défini. Il ne faut pas prétendre qu'il calcule exactement le modèle `support_mode = full`. Ce genre de glissement rhétorique marche en conférence, pas dans une preuve.

### 3.2 Régularisation locale par entropie

Pour `kappa >= 1`, définir

\[
q_i^\kappa
=
\arg\min_{q\in\Delta(A_i)}
\sum_{j\in A_i}q_j\|x_i-x_j\|^2
\quad\text{sous}\quad
H(q)\ge \log\kappa,
\]

avec

\[
H(q)=-\sum_j q_j\log q_j.
\]

La solution a la forme Gibbs :

\[
q_{ij}^{\kappa}
=
\frac{\exp(-\|x_i-x_j\|^2/\eta_i)}
{\sum_{\ell\in A_i}\exp(-\|x_i-x_\ell\|^2/\eta_i)},
\]

où `eta_i` est déterminé par

\[
\exp(H(q_i^\kappa))=\kappa.
\]

Cas limite :

\[
\kappa=1\Rightarrow q_i^\kappa=\delta_i.
\]

### 3.3 Variante Sinkhorn sparse

Si la haute dimension crée des hubs, remplacer les softmax indépendants par un transport entropique sparse :

\[
\min_{P\ge0,\ \operatorname{supp}(P)\subset E}
\sum_{(i,j)\in E}P_{ij}\|x_i-x_j\|^2
+
\varepsilon_{reg}KL(P\|A)
\]

sous contraintes

\[
P\mathbf 1=\mathbf 1,
\]

et éventuellement

\[
P^T\mathbf 1\le c
\quad\text{ou}\quad
P^T\mathbf 1=c.
\]

Les lignes de `P` deviennent les poids `q_i`.

Statut : exact pour le problème Sinkhorn sparse choisi, pas pour le softmax indépendant. Les deux sont deux régularisations différentes.

### 3.4 Sites de puissance

Définir

\[
z_i=\sum_{j\in A_i}q_{ij}x_j,
\]

\[
a_i=\sum_{j\in A_i}q_{ij}\|x_j-z_i\|^2.
\]

Alors

\[
W_2^2(\delta_y,\nu_i^\kappa)
=
\sum_jq_{ij}\|y-x_j\|^2
=
\|y-z_i\|^2+a_i.
\]

On pose

\[
\phi_i(y)=\|y-z_i\|^2+a_i.
\]

Chaque observation régularisée devient donc un site de puissance `(z_i,a_i)`.

---

## 4. Estimateur `K`-NN régularisé et niveaux de Hartigan

### 4.1 Rayon `K`-NN régularisé

Définir

\[
r_{K,\kappa}(y)^2
=
K\text{-ième plus petite valeur de }\{\phi_i(y)\}_{i=1}^n.
\]

Un estimateur de densité possible est

\[
\widehat f_{K,\kappa}(y)
=
\frac{K}{n\omega_d(r_{K,\kappa}(y)^2+\delta^2)^{d/2}}.
\]

Pour la hiérarchie, la fonction de densité exacte importe peu : une fonction strictement décroissante de `r_{K,kappa}` donne les mêmes ensembles de niveau, seulement reparamétrés.

### 4.2 Niveaux

\[
L_{K,\kappa}(r)
=
\{y\in\mathbb R^p:r_{K,\kappa}(y)\le r\}.
\]

Équivalent :

\[
L_{K,\kappa}(r)
=
\{y:\#\{i:\phi_i(y)\le r^2\}\ge K\}.
\]

Pour toute face

\[
\tau\subset[n],\quad |\tau|=K,
\]

définir la région témoin

\[
T_{\tau,\kappa}(r)
=
\bigcap_{i\in\tau}\{y:\phi_i(y)\le r^2\}.
\]

Alors

\[
L_{K,\kappa}(r)
=
\bigcup_{|\tau|=K}T_{\tau,\kappa}(r).
\]

Chaque région témoin est convexe, donc connexe.

### 4.3 Complexe de Čech de puissance

Pour tout simplexe non vide `sigma`, définir

\[
\rho_\kappa(\sigma)^2
=
\inf_{y\in\mathbb R^p}\max_{i\in\sigma}\phi_i(y).
\]

Le complexe de Čech de puissance est

\[
\check C_\kappa(r)
=
\{\sigma\subset[n]:\rho_\kappa(\sigma)\le r\}.
\]

### 4.4 Graphe des faces et `K`-polyèdres régularisés

Le graphe auxiliaire `Gamma_K^kappa(r)` a pour sommets les faces `tau` de cardinal `K` telles que `rho_kappa(tau) <= r`.

Deux faces `tau` et `tau'` sont adjacentes si leurs régions témoins s'intersectent :

\[
T_{\tau,\kappa}(r)\cap T_{\tau',\kappa}(r)\ne\varnothing.
\]

Il suffit de propager les adjacences élémentaires par cofaces de cardinal `K+1` : une coface

\[
\sigma,\quad |\sigma|=K+1,
\]

connecte ses `K+1` facettes de cardinal `K`.

Les composantes de ce graphe sont les `K`-polyèdres régularisés.

### 4.5 Théorème d'exactitude

Pour tout `r >= 0`, les composantes connexes de `L_{K,kappa}(r)` sont en bijection avec les composantes de `Gamma_K^kappa(r)`.

Preuve : `L_{K,kappa}(r)` est l'union des régions témoins convexes `T_tau`. Le graphe `Gamma_K^kappa(r)` est le graphe d'intersection de ce recouvrement. Les composantes d'une union finie de connexes sont données par les composantes du graphe d'intersection.

Cas limites :

\[
\kappa=1\Rightarrow\text{HGP classique sur l'estimateur K-NN},
\]

\[
K=1\Rightarrow\text{Single-Linkage régularisé par puissance}.
\]

---

## 5. Structure mince : Gabriel pondéré et `K`-MST

### 5.1 Graphe dual

Le graphe dual d'ordre `K` a pour sommets les faces `tau` de cardinal `K`. Une coface

\[
\sigma,\quad |\sigma|=K+1,
\]

connecte toutes ses facettes `tau prec sigma` avec poids

\[
w(\sigma)=\rho_\kappa(\sigma).
\]

En mémoire, une étoile suffit : choisir une facette pivot `tau_0`, puis ajouter les arêtes duales

\[
\tau_0\leftrightarrow\tau,
\quad \tau\prec\sigma,
\quad \tau\ne\tau_0.
\]

### 5.2 `K`-MST

Si le graphe dual contient toutes les cofaces nécessaires, un MST du graphe dual préserve les composantes à tous les niveaux :

\[
\pi_0(\Gamma_K^\kappa(r))
=
\pi_0(T_K^{\le r})
\quad\forall r.
\]

C'est l'analogue exact du MST du Single-Linkage.

### 5.3 Cofaces `K`-séparantes

Une coface `sigma` est `K`-séparante si, au niveau `rho_kappa(sigma)`, elle fusionne au moins deux composantes distinctes de faces.

On n'a besoin que de ces cofaces pour la hiérarchie.

### 5.4 Condition Gabriel pondérée

Soit `sigma` une coface de cardinal `K+1`. Soit `c_sigma` un centre réalisant `rho_kappa(sigma)`.

Le gap Gabriel pondéré est

\[
g(\sigma)
=
\min_{j\notin\sigma}
\left[\phi_j(c_\sigma)-\rho_\kappa(\sigma)^2\right].
\]

La coface est Gabriel pondérée si

\[
g(\sigma)\ge0.
\]

Sous position générale de puissance, toute coface `K`-séparante est Gabriel pondérée. Donc, pour reconstruire la hiérarchie, il suffit de trouver les cofaces Gabriel pondérées qui peuvent entrer dans le `K`-MST.

### 5.5 Conséquence algorithmique majeure

Le problème n'est pas :

```text
construire toute la mosaïque d'ordre K.
```

Le problème est :

```text
trouver assez de cofaces Gabriel pondérées pour que le K-MST soit cut-complet.
```

C'est le cœur de cette version améliorée.

---

## 6. Calcul exact de `rho_kappa(sigma)`

### 6.1 Problème

Pour

\[
\sigma=\{i_0,\ldots,i_m\},
\]

calculer

\[
\rho_\kappa(\sigma)^2
=
\min_y\max_{i\in\sigma}(\|y-z_i\|^2+a_i).
\]

C'est la miniball additivement pondérée.

### 6.2 Méthode par support actif

Pour un support candidat

\[
S=\{0,1,\ldots,s\}\subseteq\sigma,
\quad s\le p,
\]

avec ancre `0`, poser

\[
v_u=z_u-z_0,
\]

\[
G_{uv}=\langle v_u,v_v\rangle,
\]

\[
h_u=\frac12(\|v_u\|^2+a_u-a_0).
\]

Résoudre

\[
G\alpha=h.
\]

Centre candidat :

\[
c_S=z_0+\sum_{u=1}^s\alpha_uv_u.
\]

Coordonnées barycentriques :

\[
\lambda_0=1-\sum_{u=1}^s\alpha_u,
\quad
\lambda_u=\alpha_u.
\]

Rayon carré candidat :

\[
t_S=a_0+\alpha^TG\alpha.
\]

Validité :

\[
\lambda_u\ge-\texttt{tol}
\]

et

\[
\phi_i(c_S)\le t_S+\texttt{tol}
\quad\forall i\in\sigma.
\]

### 6.3 Choix du solveur

```yaml
rho_solver: support_enum | active_set
```

Recommandation :

```text
if K <= 5 and p <= 5:
    support_enum
else:
    active_set
```

`support_enum` est simple et très rapide pour triangles/tétraèdres. `active_set` est préférable dès que `K` approche `20`, car le support optimal a taille au plus `p+1`, même si `sigma` a `K+1` sommets.

### 6.4 GPU

Le calcul de `rho` tient en registres pour `p` petit. Le goulot n'est pas ce calcul. Le goulot est la génération excessive de cofaces candidates. Donc il faut filtrer avant d'appeler le solveur `rho`.

---

## 7. Test Gabriel global

Pour un batch de centres `C = {c_b}`, calculer

\[
\Phi_{bj}=\phi_j(c_b)
=\|c_b\|^2+\|z_j\|^2+a_j-2\langle c_b,z_j\rangle.
\]

En matrice :

\[
\Phi(C,Z)
=\|C\|^2\mathbf 1^T
+\mathbf 1(\|Z\|^2+a)^T
-2CZ^T.
\]

C'est un GEMM, donc GPU-friendly.

Pour chaque coface `sigma`, accepter Gabriel global si

\[
\min_{j\notin\sigma}\Phi_{\sigma j}
\ge
\rho_\kappa(\sigma)^2-\texttt{tol_gabriel}.
\]

Paramètre :

```yaml
gabriel_test: global | local_then_global
```

Recommandation :

```text
local_then_global
```

mais toute coface entrant dans le `K`-MST certifié doit passer le test global. Un test local est un filtre, pas un certificat. Oui, encore cette distinction. Elle est pénible, donc elle est probablement importante.

---

## 8. Trois régimes algorithmiques

## 8.1 Mode A : `lowdim_exact`

À utiliser lorsque :

```text
p <= 3 ou p <= 4,
K <= 20,
la taille de sortie est raisonnable,
et l'on veut un résultat exact.
```

### 8.1.1 Principe

Utiliser un fournisseur exact de la mosaïque de puissance d'ordre `K` ou, mieux pour la hiérarchie, un fournisseur exact de cofaces Gabriel pondérées.

Deux fournisseurs exacts sont admissibles :

```yaml
exact_provider: weighted_order_mosaic | ranked_supports
```

#### Fournisseur `weighted_order_mosaic`

Construire la mosaïque de Voronoï/Delaunay de puissance d'ordre `K` ou `K+1` par un algorithme combinatoire exact de basse dimension. Puis extraire les cofaces candidates, tester `rho`, tester Gabriel global, construire le `K`-MST.

Statut : exact, mais peu GPU-centric. Le GPU accélère les tests géométriques batchés, pas nécessairement la construction combinatoire.

#### Fournisseur `ranked_supports`

Utiliser le théorème de support suivant.

Soit `S` un support actif valide de taille `q <= p+1`, avec centre `c_S` et rayon carré `t_S`. Poser

\[
I_S=\{j\notin S:\phi_j(c_S)<t_S\}.
\]

Si

\[
|I_S|+|S|=K+1
\]

et s'il n'y a pas de point extérieur à égalité hors de `S` en position générale, alors

\[
\sigma=S\cup I_S
\]

est une coface Gabriel pondérée de cardinal `K+1`.

Réciproquement, toute coface Gabriel pondérée non dégénérée apparaît ainsi depuis le support actif de sa miniball pondérée.

Donc le problème exact devient : énumérer tous les supports actifs `S` de rang intérieur `|I_S| <= K+1-|S|`.

Statut : exact si tous ces supports sont énumérés. Le calcul du rang et du gap est GPU-friendly. L'énumération complète des supports reste un problème combinatoire de basse dimension.

### 8.1.2 Pipeline exact basse dimension

```text
1. Construire les sites de puissance (z_i,a_i).
2. Fournir tous les supports actifs S pertinents jusqu'au rang K.
3. Pour chaque support S en batch GPU :
      a. calculer c_S, t_S ;
      b. calculer tous les phi_j(c_S) par GEMM ;
      c. compter I_S = {j : phi_j(c_S) < t_S} ;
      d. si |I_S| + |S| = K+1, émettre sigma = S union I_S ;
      e. dédupliquer sigma ;
      f. recalculer rho_kappa(sigma) par miniball pondérée ;
      g. tester Gabriel global.
4. Construire le graphe dual.
5. Calculer le K-MST.
```

### 8.1.3 Complexité

Si `S_exact` est le nombre de supports pertinents fournis :

- calcul centres/rayons : `O(S_exact p^3)` petit ;
- rangs Gabriel : `O(S_exact n p)` par GEMM ;
- miniball finale : `O(G K p^3)` ou active-set ;
- MST dual : `O(sort(E_dual))` ou Borůvka.

L'algorithme est output-sensitive par rapport au nombre de supports/cellules pertinents. Il n'échappe pas à la taille de sortie. Aucun algorithme sérieux n'y échappe ; les autres mettent juste la poussière sous le tapis et appellent ça une approximation.

### 8.1.4 Exactitude

Exact pour le modèle complet si :

1. les sites `(z_i,a_i)` sont construits exactement pour la régularisation choisie ;
2. le fournisseur de supports/cellules est complet ;
3. les tests `rho` et Gabriel global sont exacts ;
4. le `K`-MST est calculé exactement.

---

## 8.2 Mode B : `lowdim_gpu_certified`

À utiliser lorsque :

```text
p petit ou moyen,
K <= 20,
la mosaïque complète est trop coûteuse,
mais la hiérarchie doit être certifiée autant que possible.
```

### 8.2.1 Principe

On génère rapidement des cofaces candidates avec le GPU : témoins, top-`k` entropique, supports locaux, listes `β`. Puis on certifie les cofaces acceptées et on certifie les coupes nécessaires au `K`-MST.

La mosaïque complète n'est pas construite.

### 8.2.2 Top-`k` entropique canonique

Pour un témoin `y`, poser

\[
w_i(y)=\exp(-\phi_i(y)/\varepsilon).
\]

La loi de Gibbs sur les sous-ensembles de taille `k` est

\[
P_\varepsilon(Q\mid y)
=
\frac{\prod_{i\in Q}w_i(y)}{e_k(w_1(y),\ldots,w_n(y))}.
\]

Le polynôme symétrique élémentaire `e_k` se calcule par récurrence :

\[
E_{i,\ell}=E_{i-1,\ell}+w_iE_{i-1,\ell-1}.
\]

Complexité par témoin :

\[
O(nk).
\]

Recommandé si `K <= 20`.

Garantie :

\[
0\le F_k(y)-F_{k,\varepsilon}^{can}(y)
\le \varepsilon\log {n\choose k}.
\]

Hors frontière, le top-`k` mou converge exponentiellement vers le top-`k` dur lorsque `epsilon -> 0`.

### 8.2.3 Top-`k` Fermi

Résoudre

\[
u_i^\varepsilon(y)
=
\frac{1}{1+\exp((\phi_i(y)-\lambda(y))/\varepsilon)},
\]

avec

\[
\sum_i u_i^\varepsilon(y)=k.
\]

La racine `lambda` est unique. Bisection ou Newton sécurisé.

Complexité par témoin :

\[
O(nI_\lambda).
\]

Recommandé si `n` est très grand ou si le mode canonique devient trop cher.

Statut : relaxation convexe de l'hypersimplexe ; excellent générateur, pas distribution Gibbs exacte sur les `k`-uplets.

### 8.2.4 Témoins recommandés

On ne disperse pas naïvement des témoins dans tout l'espace. On utilise des témoins qui servent la hiérarchie :

```yaml
witness_sources:
  - accepted_gabriel_centers
  - centers_from_weighted_miniballs_of_current_candidates
  - chebyshev_centers_of_certified_cells_if_available
  - entropy_maximizers_near_uncertain_frontiers
  - low_discrepancy_points_only_for_initialization
```

Le meilleur témoin est souvent le centre d'une coface Gabriel déjà acceptée. Il tombe exactement là où les top-`K` changent. Quelle surprise : regarder là où la géométrie se passe marche mieux que jeter des points au hasard. La civilisation avance.

### 8.2.5 Génération de candidats

Pour chaque témoin `y` :

1. calculer toutes les puissances par GEMM ;
2. extraire `Top_{K+h}` ;
3. émettre seulement les cofaces

\[
\sigma=Q_K(y)\cup\{j\},
\quad j\in Top_{K+h}(y)\setminus Q_K(y),
\]

où `Q_K(y)` est le top-`K` dur.

Ne jamais énumérer tous les sous-ensembles de `Top_{K+h}`.

Choix :

```yaml
h_effective: min(p_intrinsic + 2, h_max)
h_max: 16
```

Pour `p` grand, utiliser `p_intrinsic`, pas `p`. Sinon la bande active devient une machine à produire des déchets combinatoires.

### 8.2.6 Certification

Chaque coface candidate suit le pipeline dur :

```text
sigma candidate
  -> weighted_miniball(sigma) gives rho, center
  -> Gabriel global test
  -> if accepted, insert into dual graph
```

Les candidats acceptés sont vrais si le test Gabriel global passe.

La hiérarchie est complète seulement si l'audit de coupe passe.

---

## 8.3 Mode C : `highdim_lazy`

À utiliser lorsque :

```text
p grand,
n grand,
ou K suffisamment grand pour rendre la mosaïque irréaliste.
```

### 8.3.1 Principe

On abandonne la mosaïque complète. L'objectif est uniquement le `K`-MST de la hiérarchie.

On utilise un Borůvka paresseux sur le graphe dual, avec un oracle de meilleure coface sortante.

### 8.3.2 Distances binaires de puissance

Pour deux sites, définir

\[
\beta_{ij}=\rho_\kappa(\{i,j\}).
\]

Formule fermée. Poser

\[
d_{ij}=\|z_i-z_j\|.
\]

Si `d_ij > 0`,

\[
u_{ij}=\operatorname{clip}\left(
\frac{d_{ij}^2+a_j-a_i}{2d_{ij}},0,d_{ij}
\right),
\]

\[
\beta_{ij}^2
=\max\{u_{ij}^2+a_i,(d_{ij}-u_{ij})^2+a_j\}.
\]

Si `d_ij=0`,

\[
\beta_{ij}^2=\max(a_i,a_j).
\]

### 8.3.3 Borne inférieure de coface

Pour toute coface `sigma`,

\[
\rho_\kappa(\sigma)
\ge
\max_{i,j\in\sigma}\beta_{ij}.
\]

Définir

\[
LB(\sigma)=\max_{i,j\in\sigma}\beta_{ij}.
\]

Si une composante de Borůvka possède déjà un candidat sortant de poids `U_C`, toute coface avec

\[
LB(\sigma)\ge U_C
\]

est inutile.

### 8.3.4 Listes `β`

Pour chaque site `i`, stocker les `L` plus petits voisins selon `β` :

```text
N_i^L = sorted neighbors by beta_ij
lambda_i^L = beta of first omitted neighbor, or certified lower bound
```

Pour une face

\[
\tau=\{i_1,\ldots,i_K\},
\]

une extension

\[
\sigma=\tau\cup\{j\}
\]

ne peut battre `U_C` que si

\[
j\in\bigcap_{i\in\tau}N_i^\beta(U_C).
\]

On génère donc les extensions par intersection de préfixes de listes triées.

### 8.3.5 Certificat de queue

Si

\[
U_C<\min_{i\in\tau}\lambda_i^L,
\]

alors toute extension capable de battre `U_C` est dans les listes stockées. L'oracle est certifié pour cette face.

Si le certificat échoue, augmenter `L` localement.

### 8.3.6 Borůvka paresseux

```text
Initialize components on discovered K-faces.

while more than one component:
    for each component C in parallel:
        U_C = +inf
        best_C = none

        for tau in boundary_faces(C):
            Ext = intersect_prefixes(N_i^L for i in tau, threshold=U_C)

            for j in Ext:
                sigma = sorted_tuple(tau union {j})
                if canonical_owner(sigma) != tau:
                    continue
                if all K-facets of sigma are already in C:
                    continue

                if LB(sigma) >= U_C:
                    continue

                rho, center = weighted_miniball(sigma)
                if rho >= U_C:
                    continue

                if gabriel_local_filter_fails(sigma):
                    continue

                if gabriel_global_required_now:
                    if not gabriel_global(sigma, center, rho):
                        continue

                U_C = rho
                best_C = sigma

        certify_tail_conditions(C, U_C)

    add all certified best_C
    union components
```

### 8.3.7 Exactitude

Le mode `highdim_lazy` est exact pour la hiérarchie si, à chaque passe, l'oracle retourne une meilleure coface sortante certifiée pour chaque composante.

Sinon :

- les cofaces acceptées avec Gabriel global sont vraies ;
- le `K`-MST peut être incomplet ;
- `hgp_hierarchy_complete` doit valoir `unknown` ou `false`.

---

## 9. Audit de complétude

## 9.1 Audit complet de mosaïque

Paramètre :

```yaml
audit: exhaustive_cell
```

Pour chaque cellule `Q` d'ordre `k`, tester tous les `j notin Q` non générés :

\[
C_{k\to k+1}(Q,j)=\varnothing ?
\]

Chaque test est un LP de faisabilité en contraintes affines.

Si tous les tests passent pour `k=1,...,K+1`, la mosaïque est complète.

Exact mais cher. À réserver à `target=full_mosaic` en petite dimension.

## 9.2 Audit de coupe

Paramètre :

```yaml
audit: cut
```

Pour chaque composante `C` pendant Borůvka, prouver qu'aucune coface non générée ne traverse la coupe avec un poids inférieur au meilleur candidat `U_C`.

Méthode principale : certificats de queue des listes `β`.

Si pour toutes les faces de frontière `tau`,

\[
U_C<\min_{i\in\tau}\lambda_i^L,
\]

alors les extensions non stockées ne peuvent pas battre `U_C`.

Si le certificat échoue, élargir localement les listes.

## 9.3 Audit Gabriel

Toute coface entrant dans le `K`-MST final doit passer Gabriel global.

Un test Gabriel local est seulement un filtre de pré-sélection.

## 9.4 Rapport

```yaml
audit_report:
  audit_mode: none | cut | exhaustive_cell
  cuts_tested: int
  cuts_certified: int
  cuts_uncertified: int
  local_expansions_requested: int
  local_expansions_failed: int
  cells_tested: int
  cells_certified_empty: int
  cell_audits_missing: int
  gabriel_global_tests: int
  gabriel_local_only: int
```

---

## 10. LP de cellule et oracle de séparation

Ce module est surtout utilisé en mode `full_mosaic` ou pour certifier des cellules progressives en basse dimension.

Pour `Q` de cardinal `k`, la cellule de puissance d'ordre `k` est

\[
C_k(Q)=\{y:\phi_i(y)\le\phi_j(y),\ i\in Q,\ j\notin Q\}.
\]

Chaque contrainte est affine :

\[
2\langle z_j-z_i,y\rangle
\le
\|z_j\|^2-\|z_i\|^2+a_j-a_i.
\]

Oracle de séparation :

1. résoudre un LP avec un petit actif de contraintes ;
2. obtenir un point `y` ;
3. calculer

\[
M_Q(y)=\max_{i\in Q}\phi_i(y),
\quad
m_{\bar Q}(y)=\min_{j\notin Q}\phi_j(y);
\]

4. si `M_Q(y) <= m_bar_Q(y) + tol`, cellule certifiée non vide ;
5. sinon ajouter la contrainte violée `(i*,j*)` avec

\[
i^*=\arg\max_{i\in Q}\phi_i(y),
\quad
j^*=\arg\min_{j\notin Q}\phi_j(y).
\]

Statut : exact si l'oracle de séparation teste globalement tous les sites.

---

## 11. Génération par témoins : statut exact et heuristique

Un batch témoin n'est exact que sous condition de couverture.

Pour une cellule `Q` d'ordre `k`, la transition vers `Q union {j}` correspond à

\[
C_{k\to k+1}(Q,j)
=
C_k(Q)\cap\{y:\phi_j(y)\le\phi_\ell(y),\ \forall \ell\notin Q\cup\{j\}\}.
\]

Si le batch `Y_Q` contient au moins un témoin dans chaque sous-cellule non vide `C_{k->k+1}(Q,j)`, alors il retrouve toutes les transitions.

Sans audit, c'est une hypothèse, pas un résultat.

Garantie probabiliste : si les témoins sont tirés selon une mesure `mu_Q` sur `C_k(Q)`, alors

\[
P(Y_Q\cap A=\varnothing)=(1-\mu_Q(A))^{M_Q}.
\]

Les petites cellules demandent donc beaucoup de témoins. C'est précisément pourquoi le batch témoin doit être vu comme un générateur, pas comme le fondement de l'exactitude.

---

## 12. Architecture GPU

### 12.1 Layout mémoire

```text
Z:            float [n,p]
a:            float [n]
normZ2a:      float [n] = ||z_i||^2 + a_i
Y:            float [B,p]
normY2:       float [B]
```

Puissances :

\[
D=\|Y\|^2\mathbf 1^T+
\mathbf 1(\|Z\|^2+a)^T-2YZ^T.
\]

Ne jamais stocker une matrice `M_total x n` complète si un calcul batché suffit.

### 12.2 Kernels nécessaires

```text
regularize_entropy_local
regularize_sinkhorn_sparse
compute_power_distances_batch
partial_topk_batch
canonical_topk_dp_batch
fermi_lambda_solve_batch
emit_witness_candidates
beta_pair_weights_batch
intersect_beta_prefixes
weighted_miniball_batch
gabriel_local_filter
gabriel_global_gemm
cell_lp_separation_oracle
build_dual_edges_star
boruvka_dual_mst
radix_deduplicate_tuples
```

### 12.3 Précision

```yaml
precision_generation: float32 or tf32
precision_certification: float64
lp_tol: 1e-10 to 1e-8 after rescaling
gabriel_tol: 1e-10 to 1e-8 after rescaling
fallback_cpu_robust_predicates: true in lowdim_exact
```

Les générations peuvent être en float32. Les certificats doivent être en float64, voire avec fallback exact/robuste en petite dimension.

---

## 13. Sélection automatique du mode

```text
if target == full_mosaic:
    if p <= 4 and estimated_output_size <= output_budget:
        geometry_mode = lowdim_exact
        candidate_engine = exact_support
        audit = exhaustive_cell
    else:
        warn("Full mosaic not realistic; switching to hgp_hierarchy is recommended")
        geometry_mode = lowdim_gpu_certified
        audit = exhaustive_cell if user insists else cut

if target == hgp_hierarchy:
    if p <= 4 and estimated_output_size <= output_budget:
        geometry_mode = lowdim_exact
        candidate_engine = exact_support
        audit = cut
    elif p <= 8 and K <= 20:
        geometry_mode = lowdim_gpu_certified
        candidate_engine = hybrid
        audit = cut
    else:
        geometry_mode = highdim_lazy
        candidate_engine = beta_lazy
        audit = cut
```

Top-`k` :

```text
if K <= 20 and memory_budget_ok:
    soft_topk = canonical
else:
    soft_topk = fermi
```

`rho` :

```text
if K <= 5 and p <= 5:
    rho_solver = support_enum
else:
    rho_solver = active_set
```

---

## 14. Pseudo-code principal

```text
Algorithm CERT-EW-HGP
Input:
    X in R^{n x p}
    K, kappa
    target, geometry_mode, audit
    regularization, support_mode
    tolerances and budgets

Output:
    certified cofaces Gabriel
    dual K-MST if target = hgp_hierarchy
    optional full mosaic cells
    exactness_report

1. Sites = RegularizeSites(X, kappa, regularization, support_mode)
       returns Z, a

2. if geometry_mode == lowdim_exact:
       SigmaCand = ExactLowDimGabrielProvider(Z,a,K)
       SigmaGab  = CertifyCofacesByRhoAndGabriel(SigmaCand,Z,a)
       if target == full_mosaic:
           Cells = ExactOrAuditedOrderMosaic(Z,a,K)

3. if geometry_mode == lowdim_gpu_certified:
       SigmaCand = HybridCandidateGeneration(Z,a,K)
           sources:
             - witness soft top-K
             - accepted Gabriel centers
             - beta-neighbor expansions
             - optional LP-certified cells
       SigmaGab = CertifyCofacesByRhoAndGabriel(SigmaCand,Z,a)

4. if geometry_mode == highdim_lazy:
       SigmaGab, DualEdges = LazyCutBoruvka(Z,a,K,audit=cut)
       goto step 7

5. Build dual graph:
       for sigma in SigmaGab:
           facets = all K-subsets of sigma
           add star edges with weight rho(sigma)

6. if audit == cut:
       CertifyCutCompletenessOrExpandCandidates()

7. Compute K-MST on dual graph.

8. Build hierarchy:
       face births = rho(tau)
       merges = sorted MST edges
       cluster at radius r = connected components of MST edges with weight <= r

9. Return hierarchy and exactness_report.
```

---

## 15. Pseudo-code : fournisseur exact basse dimension par supports classés

```text
Algorithm RankedSupportGabrielProvider
Input:
    weighted sites (Z,a)
    K
    support_provider exact for all active supports with rank <= K

Output:
    candidate cofaces SigmaCand

1. Supports = support_provider(Z,a,K)

2. for S in Supports parallel:
       c_S, t_S, valid = weighted_ball_of_support(S)
       if not valid:
           continue

       phi = power_distances(c_S, Z, a)     # GEMM batched
       Interior = {j not in S : phi_j < t_S - tol}
       BoundaryExtra = {j not in S : abs(phi_j - t_S) <= tol}

       if BoundaryExtra not empty:
           handle_degeneracy_or_send_to_robust_backend

       if |Interior| + |S| == K + 1:
           sigma = sorted_tuple(S union Interior)
           emit sigma

3. deduplicate SigmaCand
4. return SigmaCand
```

Exactitude : complète si `support_provider` est complet. Sinon, les cofaces émises et ensuite certifiées sont vraies, mais la complétude est inconnue.

---

## 16. Pseudo-code : `Lazy Cut-Borůvka`

```text
Algorithm LazyCutBoruvka
Input:
    weighted sites (Z,a)
    K
    initial L for beta-neighbor lists

Output:
    certified Gabriel cofaces entering K-MST
    dual MST edges
    exactness report

1. BuildBetaLists(Z,a,L)
       N_i^L sorted by beta_ij
       lambda_i^L tail certificates

2. Initialize discovered K-faces from beta lists and/or seed witnesses.

3. Initialize UnionFind over discovered K-faces.

4. while components remain:
       for each component C parallel:
           U_C = +inf
           best = none
           uncertified_faces = 0

           for tau in BoundaryFaces(C):
               Ext = IntersectPrefixes(N_i^L for i in tau, threshold=U_C)

               for j in Ext:
                   sigma = tau union {j}
                   if not canonical_owner(tau,sigma):
                       continue
                   if all facets(sigma) in same component C:
                       continue
                   if LB(sigma) >= U_C:
                       continue

                   rho, center = WeightedMiniball(sigma)
                   if rho >= U_C:
                       continue

                   if LocalGabrielFilter(sigma,center,rho) fails:
                       continue

                   if GlobalGabriel(sigma,center,rho) passes:
                       U_C = rho
                       best = sigma

               if U_C >= min_tail_lambda(tau):
                   uncertified_faces += 1
                   request local expansion of beta lists for vertices of tau

           if uncertified_faces == 0 and best != none:
               emit certified best
           else:
               mark C uncertified and expand locally

       add all certified best cofaces
       union their incident components

5. return accepted cofaces and exactness report
```

Exactitude : si toutes les composantes ont une meilleure coface sortante certifiée à chaque passe, le MST est exact par propriété de coupe.

---

## 17. Complexités

### 17.1 Régularisation

Softmax local :

\[
O(n\,m_{reg}\,p)
\]

après construction des voisinages.

Sinkhorn sparse :

\[
O(I_{sink}\,|E|)
\]

pour les scalings, plus le coût des produits sparse-dense pour les barycentres.

### 17.2 Évaluation de puissances

Pour `M` témoins :

\[
O(Mnp)
\]

via GEMM.

### 17.3 Top-`k` mou

Canonique :

\[
O(MnK).
\]

Fermi :

\[
O(MnI_\lambda).
\]

### 17.4 Miniball pondérée

Support enumeration :

\[
O\left({K+1\choose \le p+1}p^3\right)
\]

par coface.

Active-set : typiquement

\[
O(Kp^3)
\]

par coface en dimension fixée.

### 17.5 Gabriel global

Pour `G` cofaces :

\[
O(Gnp)
\]

via GEMM.

### 17.6 `K`-MST

Si `E_dual` arêtes duales :

- Kruskal : `O(sort(E_dual))` ;
- Borůvka : `O(E_dual log V_dual)` en travail, massivement parallèle.

### 17.7 Remarque de sortie

Si la hiérarchie exacte contient elle-même un nombre énorme de faces ou de fusions, aucun algorithme ne peut la produire en temps sous-linéaire en sa taille. Même avec un GPU. Même en écrivant “entropic” dans le titre.

---

## 18. Ce qui est exact et ce qui ne l'est pas

| Élément | Statut | Condition |
|---|---:|---|
| Estimateur `K`-NN régularisé par puissance | Exact | pour les supports et la régularisation choisis |
| Niveaux comme `K`-couverture | Exact | définition directe |
| Région témoin convexe | Exact | boules de puissance |
| Correspondance Hartigan ↔ `K`-polyèdres | Exact | graphe d'intersection complet |
| `rho_kappa(sigma)` | Exact | solveur miniball exact/tolérance robuste |
| Gabriel global | Exact | test sur tous les sites |
| `K`-MST | Exact | graphe dual complet ou cut-complet |
| Mosaïque complète par fournisseur exact | Exact | fournisseur complet |
| Top-`k` entropique | Heuristique de génération | converge quand `epsilon -> 0`, ne certifie pas |
| Batch témoin | Heuristique de génération | exact seulement sous couverture |
| Test Gabriel local | Filtre | pas certificat global |
| `β`-listes top-`L` | Certifiable localement | exact si queues certifiées |
| Arrêt sans nouveaux candidats | Heuristique | jamais un certificat |

---

## 19. Recommandations par régime

### 19.1 Petite dimension, sortie raisonnable

```yaml
target: hgp_hierarchy
geometry_mode: lowdim_exact
candidate_engine: exact_support
audit: cut
soft_topk: canonical
rho_solver: active_set
gabriel_test: global
mst_solver: kruskal
```

Pour la mosaïque complète :

```yaml
target: full_mosaic
geometry_mode: lowdim_exact
audit: exhaustive_cell
```

### 19.2 Petite dimension, `K <= 20`, sortie potentiellement grosse

```yaml
target: hgp_hierarchy
geometry_mode: lowdim_gpu_certified
candidate_engine: hybrid
soft_topk: canonical
audit: cut
rho_solver: active_set
gabriel_test: local_then_global
mst_solver: boruvka
```

### 19.3 Grande dimension

```yaml
target: hgp_hierarchy
geometry_mode: highdim_lazy
candidate_engine: beta_lazy
soft_topk: fermi
regularization: sinkhorn_sparse_if_hubness_else_entropy_local
audit: cut
rho_solver: active_set
gabriel_test: global_on_mst_candidates
mst_solver: boruvka
```

### 19.4 Données massives

```yaml
support_mode: knn
m_reg: 32 to 256
regularization: entropy_local or sinkhorn_sparse
target: hgp_hierarchy
geometry_mode: highdim_lazy
audit: cut with local expansion
```

La sortie est exacte pour le modèle localisé si les certificats passent.

---

## 20. Sortie hiérarchique

À un niveau `r` :

1. garder les faces `tau` avec `rho_kappa(tau) <= r` ;
2. garder les arêtes du `K`-MST avec poids `<= r` ;
3. les composantes donnent les composantes de faces ;
4. le cluster discret associé est

\[
P_C=\bigcup_{\tau\in C}\tau.
\]

Pour `K >= 2`, les clusters peuvent se recouvrir. Ce n'est pas une erreur. C'est la sortie naturelle du modèle de Hartigan discret associé aux régions de `K`-couverture.

---

## 21. Fichier de sortie recommandé

```yaml
model:
  K: int
  kappa: float
  regularization: entropy_local | sinkhorn_sparse
  support_mode: full | knn
  m_reg: int

sites:
  Z_path: string
  a_path: string

cofaces:
  - sigma: [int]
    rho: float
    center: [float]
    gabriel_gap: float
    certificate: global_gabriel

dual_mst:
  - tau_a: [int]
    tau_b: [int]
    weight: float
    owner_sigma: [int]

hierarchy:
  face_births:
    tau: rho
  merges_sorted_by_radius: path

exactness_report:
  model_exact_for_chosen_supports: bool
  accepted_cofaces_are_true_gabriel: bool
  full_mosaic_complete: bool | unknown
  hgp_hierarchy_complete: bool | unknown
  audit_mode: none | cut | exhaustive_cell
  cut_certificates_missing: int
  cell_audits_missing: int
  gabriel_global_tests_done: int
  gabriel_local_only_count: int
```

---

## 22. Ce qu'il ne faut pas faire

1. Ne pas présenter le top-`K` entropique comme une mosaïque exacte.
2. Ne pas présenter un batch témoin comme complet sans audit.
3. Ne pas construire toute la mosaïque si l'objectif est seulement HGP.
4. Ne pas confondre Gabriel local et Gabriel global.
5. Ne pas utiliser Rips comme objet exact sur `R^p` pour cet estimateur.
6. Ne pas énumérer toutes les cliques d'un graphe de voisinage pour `K` proche de `20`.
7. Ne pas annoncer l'exactitude du modèle complet si `support_mode = knn` définit un modèle localisé.
8. Ne pas masquer les coupes non certifiées. Les mathématiques finissent toujours par retrouver les traces de pas.

---

## 23. Synthèse finale

La meilleure version de l'algorithme est :

\[
\text{données}
\to
\text{atomes entropiques}
\to
\text{sites de puissance}
\to
\text{Čech de puissance exact}
\to
\text{cofaces Gabriel pondérées}
\to
K\text{-MST certifié}.
\]

La mosaïque de Delaunay d'ordre supérieur pondérée reste la structure géométrique qui organise les cofaces, mais elle ne doit pas être construite intégralement sauf en petite dimension et petite sortie.

La version GPU pertinente n'est donc pas un “Sinkhorn de Delaunay exact” miraculeux. C'est un système hybride : l'entropie sert à régulariser les observations et à générer rapidement des candidats ; l'exactitude vient des certificats géométriques et des certificats de coupe.

Le principe de décision est simple :

```text
Si tout est certifié, la hiérarchie est exacte.
Si les cofaces acceptées sont Gabriel globales mais les coupes ne sont pas certifiées, les cofaces sont vraies mais la hiérarchie peut être incomplète.
Si seuls les témoins et le top-K entropique sont utilisés, l'algorithme est rapide mais heuristique.
```

C'est moins spectaculaire qu'une promesse de Delaunay quantique sur GPU. C'est aussi beaucoup moins faux.
