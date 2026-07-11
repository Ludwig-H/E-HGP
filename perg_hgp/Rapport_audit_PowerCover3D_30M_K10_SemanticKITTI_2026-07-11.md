# Audit ciblé de `PowerCover3D`

## 30 millions de points, \(K=10\) et scènes SemanticKITTI

- **Dépôt audité :** [`Ludwig-H/E-HGP`](https://github.com/Ludwig-H/E-HGP)
- **Révision auditée :** [`150c1ce16387e45e085c32b17c15847ecac9d390`](https://github.com/Ludwig-H/E-HGP/commit/150c1ce16387e45e085c32b17c15847ecac9d390)
- **Date :** 11 juillet 2026
- **Périmètre :** `PowerCover3D`, son backend CPU de référence, son backend CUDA/RAPIDS, le notebook Blackwell 30 M, les tests associés, les chapitres 1 et 2 de la thèse et le pipeline SemanticKITTI historique.

---

## Résumé exécutif

Le cœur idéal de `PowerCover3D` est **mathématiquement cohérent** :

\[
\text{régularisation entropique locale}
\longrightarrow
\text{sites de puissance}
\longrightarrow
\text{champ K-ième}
\longrightarrow
\text{enveloppe cubique}
\longrightarrow
\text{MSF de }H_0.
\]

En arithmétique réelle, avec un oracle top-K exact, les identités de puissance, la stabilité vis-à-vis du champ K-NN, la 1-lipschitzianité, l'encadrement cubique et le MSF sont valides.

En revanche, le logiciel du commit audité n'est pas encore un clusterer HGP massif complet : il calcule une hiérarchie de cubes, mais ne produit ni l'appartenance canonique des observations, ni un merge tree enrichi, ni une partition plate exploitable sur SemanticKITTI.

| Question | Verdict honnête |
|---|---|
| Peut-il tenir en mémoire à 30 M ? | **Plausible** sur un gros GPU. Le modèle analytique annonce environ 4,27 Gio, mais plusieurs workspaces et temporaires ne sont pas mesurés. |
| Peut-il terminer assez vite à 30 M ? | **Non démontré.** Le solveur Gibbs actuel est probablement le premier goulet ; aucun résultat GPU 30 M n'est versionné. |
| \(K=10\) crée-t-il une explosion combinatoire ? | **Non.** Le coût dépend surtout de `m_reg`, du nombre de candidats de puissance et de la grille. |
| La conception mathématique est-elle bonne ? | **Oui pour une hiérarchie \(H_0\) approchée**, mais la certification de bout en bout est incomplète. |
| L'implémentation est-elle bonne ? | **Très bonne pour un prototype de recherche**, pas encore satisfaisante comme produit ou clusterer SemanticKITTI. |
| Peut-on revendiquer de meilleurs résultats avec \(K=10\) ? | **Non à ce stade.** Le gain de stabilité est plausible, mais les risques de fusion et de perte des petits objets sont importants. |

Mon verdict opérationnel est donc :

> **Go pour poursuivre la R&D et lancer une campagne GPU instrumentée ; no-go pour annoncer que 30 M est validé, que le calcul est certifié de bout en bout ou que \(K=10\) améliore SemanticKITTI.**

---

## 1. Méthode d'audit et vérifications exécutées

Les éléments suivants ont été inspectés en détail :

- le contrat public dans [`POWER_COVER_3D.md`](POWER_COVER_3D.md) ;
- les structures et le modèle mémoire dans [`contracts.py`](perg_hgp/backends/power_cover_3d_cuda/contracts.py) ;
- la régularisation, le top-K de puissance et la grille dans [`spatial_core.py`](perg_hgp/backends/power_cover_3d_cuda/spatial_core.py) ;
- l'orchestration complète dans [`api.py`](perg_hgp/backends/power_cover_3d_cuda/api.py) ;
- le wrapper RAPIDS dans [`cuda_runtime.py`](perg_hgp/backends/power_cover_3d_cuda/cuda_runtime.py) ;
- le MSF implicite dans [`cubical_msf.py`](perg_hgp/backends/power_cover_3d_cuda/cubical_msf.py) ;
- le notebook [`PERG_HGP_Blackwell_30M.ipynb`](../PERG_HGP_Blackwell_30M.ipynb) ;
- les tests CPU et les contrats CUDA sans matériel NVIDIA ;
- le [`manuscrit de thèse`](../Manuscrit_de_these_LouisHauseux.pdf), notamment chapitres 1 §1.3–1.5 et 2 §2.3–2.4 ;
- le pipeline [`HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py`](../HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py) et son log d'évaluation.

### Tests exécutés

Les quatre suites ciblées `PowerCover3D` ont été exécutées sur CPU :

```text
test_power_cover_spatial_core.py
test_power_oracle.py
test_cubical_msf.py
test_cuda_runtime_contract.py

75 passed in 1.03 s
```

`compileall` passe également sur le package. Aucun GPU NVIDIA n'était disponible localement ; les kernels CuPy/RAPIDS n'ont donc pas été exécutés dans cet audit.

Deux contre-tests supplémentaires ont été réalisés :

1. une entrée float64 de très grande étendue montrant une collision après normalisation/cast float32 ;
2. un support avec huit minima ex æquo montrant que `entropy_target_deviation_max` peut être grand alors que la solution est mathématiquement optimale.

Un micro-benchmark CPU indicatif, sur la machine d'audit, utilise le même nuage gaussien allongé de 20 000 points (`seed=20260711`, écarts-types 8, 8 et 0,8), \(K=10\), `m_reg=64` et une grille `32×32×8` :

| \(\kappa\) | Temps total | Régularisation | \(s_{0.99}\) | \(s_{\max}\) |
|---:|---:|---:|---:|---:|
| 1 | 1,10 s | 0,67 s | 0 | 0 |
| 4 | 1,77 s | 1,30 s | 1,00 | 5,26 |

Ce benchmark n'est **pas** extrapolable au GPU. Il illustre seulement le surcoût du solveur entropique et la fragilité d'un budget fondé sur un maximum global : un petit nombre d'outliers peut dominer \(s_{\max}\).

### Évidence GPU disponible dans le dépôt

Le notebook Blackwell et le notebook GPU générique ont zéro cellule exécutée et zéro sortie enregistrée. Aucun `benchmark_metrics.json`, aucun fichier d'acceptation 30 M et aucun profil de candidats n'est versionné. Il n'existe pas non plus de workflow GitHub Actions.

Le notebook Blackwell courant épingle le package au commit `8649331`, antérieur au commit audité. Le code principal de `PowerCover3D` est identique entre ces révisions. La logique de fallback, contenue dans le notebook courant, sera bien exécutée ; seule son unité de test ajoutée au commit courant ne fait pas partie de la suite installée par le notebook.

---

## 2. Ce que calcule réellement `PowerCover3D`

Il faut distinguer `PowerCover3D` du prototype historique `PERGHGPClusterer`.

`PERGHGPClusterer` tente de construire un atlas Rank-Gabriel et des cofaces. `PowerCover3D` abandonne cette énumération combinatoire et évalue directement un champ scalaire sur une grille. Cette seconde stratégie est la bonne direction pour le passage à l'échelle.

Le pipeline est le suivant :

1. recentrage, normalisation isotrope et conversion float32 des points ;
2. construction d'un index euclidien sur les observations ;
3. calcul en chunks des lois de Gibbs locales et des sites \((z_i,a_i)\) ;
4. construction d'un second index euclidien sur les \(z_i\) ;
5. calcul du K-ième rayon de puissance aux centres des cubes ;
6. construction d'une filtration cubique intérieure et extérieure ;
7. calcul d'un MSF implicite 26-connexe ;
8. retour d'une hiérarchie de cubes et d'un rapport de précision.

Les trois paramètres fondamentaux sont correctement séparés :

- \(K\) est l'ordre de couverture ;
- \(\kappa\) est la perplexité entropique minimale ;
- `m_reg` est la taille du support local dur.

Ainsi, prendre \(K=10\) ne signifie ni `kappa=10`, ni `m_reg=10`.

### D'où vient réellement la scalabilité

La scalabilité ne vient pas, à elle seule, de l'entropie. Elle vient surtout de :

- l'abandon de l'énumération des cofaces ;
- l'évaluation d'un champ scalaire sur une grille ;
- des KNN spatiaux en streaming ;
- un kernel fusionné pour les distances de puissance ;
- un MSF cubique implicite.

Le même schéma est possible à \(\kappa=1\), auquel cas les sites redeviennent idéalement les observations. L'entropie est un choix de régularisation et de stabilité, pas la cause principale de la parallélisation GPU.

---

## 3. Audit mathématique

### 3.1 Problème entropique local

Sur un support fixé \(A_i\) de taille au plus `m_reg`, le backend résout :

\[
q_i^\kappa\in\arg\min_{q\in\Delta(A_i)}
\sum_jq_j\|x_i-x_j\|^2,
\qquad H(q)\ge\log\kappa.
\]

La région admissible est convexe, car un super-niveau d'une fonction concave est convexe, et l'objectif est linéaire. Hors cas dégénéré, les conditions KKT donnent une loi de Gibbs :

\[
q_{ij}(\beta_i)
=
\frac{\exp(-\beta_i c_{ij})}
{\sum_\ell\exp(-\beta_i c_{i\ell})},
\qquad
c_{ij}=\|x_i-x_j\|^2.
\]

Le traitement de \(\kappa=1\), de \(\kappa=m_{\rm reg}\) et des minima ex æquo est cohérent dans `spatial_core.py:127-315`.

Une précision importante : \(\kappa=\exp H(q)\) est une **perplexité effective**, pas le nombre de coefficients non nuls. Pour une température strictement positive, tous les points du support ont généralement un poids positif.

### 3.2 Identité de puissance et interprétation Wasserstein

Le code forme :

\[
z_i=\sum_jq_{ij}x_j,
\qquad
a_i=\sum_jq_{ij}\|x_j-z_i\|^2.
\]

La décomposition de variance donne exactement :

\[
\begin{aligned}
d_i^\kappa(y)^2
&=\|y-z_i\|^2+a_i\\
&=\sum_jq_{ij}\|y-x_j\|^2\\
&=W_2^2(\delta_y,\nu_i^\kappa),
\end{aligned}
\]

où \(\nu_i^\kappa=\sum_jq_{ij}\delta_{x_j}\).

En particulier :

\[
s_i^2
=d_i^\kappa(x_i)^2
=\|x_i-z_i\|^2+a_i
=\sum_jq_{ij}\|x_i-x_j\|^2.
\]

Le problème entropique choisit donc, sur le support fixé, la loi ayant une perplexité au moins \(\kappa\) et la plus petite distorsion quadratique autour de \(x_i\). C'est une justification mathématique claire du modèle.

### 3.3 Relèvement euclidien exact en dimension 4

Une propriété importante n'est pas exploitée par l'implémentation :

\[
d_i^\kappa(y)
=
\left\|(y,0)-\left(z_i,\sqrt{a_i}\right)\right\|_{\mathbb R^4}.
\]

Le top-K de puissance est donc exactement un top-K euclidien en dimension 4, où les requêtes vivent dans l'hyperplan de quatrième coordonnée nulle.

Cette observation fournit une voie naturelle vers un oracle exact : un LBVH/BVH 4D ou un arbre branch-and-bound sur les points \((z_i,\sqrt{a_i})\). Elle éviterait le garde RBC 3D global actuellement utilisé.

### 3.4 Stabilité vis-à-vis du champ K-NN

Par l'inégalité triangulaire dans le relèvement 4D :

\[
\left|d_i^\kappa(y)-\|y-x_i\|\right|\le s_i.
\]

L'ordre statistique K-ième est 1-lipschitz pour la norme uniforme. Par conséquent :

\[
|r_{K,\kappa}(y)-r_K(y)|
\le s,
\qquad
s=\max_i s_i.
\]

On obtient les inclusions de sous-niveaux :

\[
L_{r_K}(t-s)
\subseteq
L_{r_{K,\kappa}}(t)
\subseteq
L_{r_K}(t+s).
\]

Cette partie est solide. Elle reste même vraie si le solveur Gibbs n'est qu'approché : pour tout site synthétique \((z_i,a_i\ge0)\), il suffit de définir \(s_i=d_i(x_i)\).

La portée doit cependant rester précise. C'est une garantie :

- sur le paramètre rayon ;
- sur les filtrations et leur \(H_0\) ;
- déterministe, une fois \(s\) connu.

Ce n'est pas :

- une erreur additive sur une densité \(r^{-3}\) ;
- une garantie de partition plate ;
- une preuve de réduction de variance statistique ;
- une preuve de consistance du cluster tree ;
- une preuve d'amélioration de SemanticKITTI.

### 3.5 K-couverture, boîte englobante et connexité

Chaque fonction

\[
d_i(y)=\sqrt{\|y-z_i\|^2+a_i}
\]

est 1-lipschitz. Le champ K-ième \(r_{K,\kappa}\) l'est donc aussi.

Le sous-niveau est exactement une région de K-couverture :

\[
L_{K,\kappa}(r)
=
\left\{y:
\#\{i:d_i(y)\le r\}\ge K
\right\}.
\]

La restriction à la boîte englobante des \(z_i\) est suffisante pour \(H_0\). La projection coordonnée vers cette boîte n'augmente aucune distance aux sites ; un chemin extérieur se projette donc dans la boîte sans augmenter le champ.

Pour des cubes fermés, deux cubes qui se touchent par une face, une arête ou un sommet forment une union connexe. La 26-connexité est donc la convention correcte.

Si une arête de cubes \((u,v)\) reçoit le poids

\[
w(u,v)=\max(f(u),f(v)),
\]

alors tout MST/MSF conserve les composantes à tous les seuils. Le principe de `cubical_msf.py` est correct.

### 3.6 Enveloppe cubique et borne plus fine disponible

Soit \(Q\) un cube de centre \(c_Q\), de demi-diagonale \(H\). Si l'évaluation numérique du centre a une erreur bornée par \(\delta\), alors, pour tout \(y\in Q\) :

\[
|g(y)-\widehat g(c_Q)|\le H+\delta.
\]

En posant \(e=H+\delta\), les activations :

\[
\text{outer}_Q=\widehat g(c_Q)-e,
\qquad
\text{inner}_Q=\widehat g(c_Q)+e
\]

donnent bien :

\[
A^-_t\subseteq L_t\subseteq A^+_t,
\]

et une distance d'interleaving au plus \(2e\) pour chacune des deux enveloppes.

La borne publiée \(2(H+\delta)\) est donc valide sous l'hypothèse que \(\delta\) borne réellement toutes les erreurs numériques concernées.

Elle est toutefois conservatrice. La filtration de base déjà stockée,

\[
A^0_t
=
\bigcup_{\widehat g(c_Q)\le t}Q,
\]

est directement \((H+\delta)\)-entrelacée au champ continu. Le rapport devrait publier deux contrats :

- `base_forest` : erreur spatiale \(H+\delta\) ;
- enveloppes intérieure/extérieure avec inclusion au même seuil : \(2(H+\delta)\).

### 3.7 Erreur d'entrée oubliée dans la borne publiée

Le pipeline normalise puis convertit toujours la géométrie de calcul en float32 (`api.py:686-700`). Le \(s\) publié est donc calculé par rapport au nuage quantifié, pas nécessairement au nuage d'entrée exact.

Contre-exemple exécuté :

```text
x1 = 0
x2 =   999 999 999
x3 = 1 000 000 000

coordonnées normalisées float32 : 0, 1, 1
distance originale(x2,x3)       : 1
distance quantifiée(x2,x3)      : 0
```

Pour \(\kappa=1\), le pipeline peut annoncer \(s=0\), alors qu'une perturbation physique de 1 a déjà été introduite.

Il faut calculer en streaming :

\[
\varepsilon_X
=
\max_i\|x_i-\widetilde x_i\|,
\]

où \(\widetilde x_i\) est le point après normalisation, cast float32 et dénormalisation.

La borne vis-à-vis des données originales doit être au minimum :

\[
\varepsilon_X+s^\uparrow+(H+\delta)
\]

pour la forêt de base, et :

\[
\varepsilon_X+s^\uparrow+2(H+\delta)
\]

pour les enveloppes intérieure/extérieure. Le symbole \(s^\uparrow\) rappelle que \(s\) doit lui-même être une borne supérieure arrondie vers l'extérieur, pas seulement un maximum float32.

Sur une frame SemanticKITTI en coordonnées locales, \(\varepsilon_X\) sera généralement très petit. Cela ne dispense pas de l'inclure dans un contrat qui se veut certifié.

### 3.8 Le certificat top-K est conditionnel à RBC

Le garde utilisé est mathématiquement valide si l'index renvoie les vrais \(C+1\) voisins euclidiens dans l'ordre. Pour le premier site omis :

\[
\min_{i\ \mathrm{omis}}
\bigl(\|y-z_i\|^2+a_i\bigr)
\ge
d_{\rm guard}(y)^2+\min_i a_i.
\]

Le K-ième candidat est globalement correct si sa borne supérieure est sous cette borne inférieure.

Mais le wrapper déclare lui-même :

```text
exact = False
numerically_proven = False
euclidean_order_contract = True
```

Seules 32 requêtes sont auditées par défaut contre cuVS brute force. Cet audit est utile pour détecter une régression, mais ne prouve ni les autres requêtes, ni l'ordre exact, ni l'enveloppe flottante.

La documentation NVIDIA actuelle décrit RBC comme une partition utilisant l'inégalité triangulaire et indique explicitement que le mode `brute` produit des résultats exacts ; elle ne fournit pas, dans cette page d'API, le certificat numérique supposé par le code : [NVIDIA cuML `NearestNeighbors`](https://docs.rapids.ai/api/cuml/nightly/api/generated/cuml.neighbors.nearestneighbors/).

Les noms `CertifiedPowerKNN`, `strict_certification` et `certified_fraction` sont donc trop forts. Le statut exact est :

> garde accepté sous un contrat empirique d'ordre RBC et sous une enveloppe float32 déclarée, non dirigée.

### 3.9 Aucun plafond de 1 024 candidats n'est garanti

Le rang euclidien d'un vrai voisin de puissance peut être arbitrairement élevé.

On peut placer plus de 1 024 sites très proches de la requête avec \(a_i=1\), puis dix sites un peu plus éloignés avec \(a_i=0\). Les dix meilleurs sites de puissance peuvent alors apparaître après le rang euclidien 1 024.

Par conséquent :

- `candidate_k_max=1024` n'est pas une garantie de terminaison ;
- un run strict peut s'arrêter ;
- un run non strict peut retourner un champ incertain ;
- le garde global utilisant \(\min a_i\) peut être extrêmement lâche sur un LiDAR hétérogène.

Le relèvement 4D est ici l'amélioration mathématique et algorithmique la plus naturelle.

### 3.10 Autres limites mathématiques importantes

#### Identité à \(\kappa=1\)

Le code vérifie que la première distance KNN est proche de zéro, mais pas que l'indice propre \(i\) figure réellement dans le support. Des doublons exacts au même emplacement ne changent pas \(z_i=x_i\), mais rendent la convention d'identité et le départage du support non canoniques. Le vrai risque géométrique vient d'un index approximatif qui renverrait un voisin seulement quasi nul : `kappa=1` n'imposerait alors plus \(z_i=x_i\).

Il faut injecter explicitement l'indice propre et départager par `(distance, id)`.

#### Support dur et stabilité statistique

Le support des `m_reg` plus proches voisins peut changer brutalement lorsque deux points s'échangent à la frontière du support. La borne par \(s\) contrôle la distance au champ original, mais ne prouve pas que la régularisation réduit la variance ou résiste mieux aux perturbations.

Une revendication statistique plus forte nécessiterait un support continu, une borne de queue ou une analyse de stabilité du support.

#### Optimalité numérique de Gibbs

Le solveur vérifie la faisabilité entropique et le simplexe, mais ne publie pas d'écart primal-dual ni de résidu KKT complet. Cela affecte la fidélité au problème entropique idéal, même si la borne synthétique par \(s\) reste valable pour les sites stockés. Un résidu KKT ne suffit d'ailleurs pas à borner numériquement \(z_i\) et \(a_i\) : si cette fidélité est revendiquée, il faut aussi des bornes d'accumulation sur \(q\), \(z\) et \(a\).

#### Minima ex æquo et diagnostic erroné

Pour huit minima ex æquo et \(\kappa=4\), la loi uniforme sur les huit minima est optimale et a :

```text
H(q)                 = log(8)
log(kappa)           = log(4)
constraint_residual  = 0
target_deviation     = log(2) ≈ 0,693
```

Le notebook exige pourtant `entropy_target_deviation_max <= 5e-4`. Il peut donc rejeter à tort une exécution parfaitement optimale contenant des doublons ou de nombreux coûts minimaux égaux.

Il faut mesurer la déviation à la cible uniquement lorsque la contrainte est active et compter séparément les optima dégénérés.

#### Intervalles de fusion

`fusion_intervals(relative_to_original_knn=True)` aligne correctement les arêtes des deux forêts cubiques, car elles ne diffèrent que d'une translation uniforme des niveaux.

En revanche, un entrelacement avec le champ K-NN original ne fournit pas automatiquement un appariement canonique branche par branche. Deux selles proches peuvent permuter et une branche courte peut être envoyée sur la diagonale d'un diagramme de persistance.

Il faut renommer ces valeurs comme enveloppes de niveaux des arêtes cubiques, ou construire explicitement un appariement de diagrammes/merge trees.

---

## 4. Passage à l'échelle : 30 M et \(K=10\)

### 4.1 Mémoire analytique

Avec les valeurs par défaut :

```text
N                       = 30 000 000
K                       = 10
kappa                   = 4
m_reg                   = 64
chunk régularisation    = 262 144
chunk puissance         = 65 536
candidats max           = 1 024
```

le modèle de `estimate_memory` donne :

| Poste | Estimation |
|---|---:|
| Persistant | 3,017 Gio |
| Chunk de régularisation | 0,953 Gio |
| Chunk de puissance au plafond 1 025 | 1,255 Gio |
| Pic analytique annoncé | 4,273 Gio |

Pour plusieurs grilles :

| Grille | Cubes | Arêtes implicites | Pic VRAM analytique `estimate_memory` | Pic MSF device | Pic MSF hôte | Résultat MSF compact |
|---|---:|---:|---:|---:|---:|---:|
| \(128^3\) | 2 097 152 | 26 822 908 | 4,273 Gio | 0,055 Gio | 0,094 Gio | 0,031 Gio |
| \(256^3\) | 16 777 216 | 216 338 940 | 4,273 Gio | 0,437 Gio | 0,750 Gio | 0,250 Gio |
| \(512\times512\times32\) | 8 388 608 | 106 404 028 | 4,273 Gio | 0,219 Gio | 0,375 Gio | 0,125 Gio |

Cette architecture mémoire est crédible : aucune matrice globale \(N\times64\), aucun tenseur global de candidats et aucune liste explicite des arêtes cubiques ne sont construits.

Mais 4,273 Gio n'est pas une mesure. Le chiffre ne modélise pas exactement :

- les workspaces internes RAPIDS/CUB/RMM ;
- la compilation NVRTC et le driver ;
- toutes les allocations transitoires du solveur Gibbs ;
- les temporaires float64 lors de la dénormalisation des 30 M centres ;
- les copies hôte pendant le tri final ;
- la fragmentation réelle de l'allocateur.

La mémoire a donc de bonnes chances de tenir sur un GPU de 80 Gio, mais le dépôt ne permet pas encore d'affirmer le pic réel.

### 4.2 Le vrai coût : 1,92 milliard de voisins

La régularisation traite :

\[
30\,000\,000\times64
=1{,}92\times10^9
\]

slots voisins.

Pour une ligne active avec \(1<\kappa<m_{\rm reg}\), le solveur appelle au minimum la routine d'évaluation entropique :

- deux fois pour les bornes initiales ;
- 48 fois pour la bissection ;
- une fois pour la solution finale.

Cela représente au moins :

\[
1{,}92\times10^9\times51
\approx97{,}9\times10^9
\]

évaluations de slots, hors expansions de bracket. Chaque évaluation calcule deux tableaux d'exponentielles, soit environ 196 milliards d'exponentielles élémentaires dans le cas actif général.

Le problème n'est pas seulement arithmétique : la version CuPy crée et balaie plusieurs grands temporaires à chaque itération. C'est très probablement le premier goulet à profiler.

Un test float32 sur 10 000 lignes de 64 coûts a donné la même déviation maximale, \(7{,}19\times10^{-7}\), avec 28 et 48 itérations. Les itérations supplémentaires stagnent donc déjà à la précision float32 sur ce cas.

### 4.3 Coût du champ de puissance

Si 64 candidats suffisent :

| Grille | Évaluations de candidats |
|---|---:|
| \(128^3\) | 134 217 728 |
| \(256^3\) | 1 073 741 824 |

Si chaque requête escalade successivement à 64, 128, 256, 512 et 1 024, la somme est 1 984 candidats par requête :

| Grille | Évaluations cumulées au pire plafond |
|---|---:|
| \(128^3\) | environ 4,16 milliards |
| \(256^3\) | environ 33,29 milliards |

Chaque escalade réinterroge actuellement RBC depuis le début. Le profil réel dépend donc fortement de l'histogramme des candidats, qui n'est pas disponible pour une scène LiDAR réelle.

### 4.4 Coût du MSF

Le MSF implicite évite correctement la liste d'arêtes, mais Borůvka rescane les 13 directions canoniques à chaque phase.

Au plafond de phases codé :

- \(128^3\) : environ 617 millions de visites d'arêtes ;
- \(256^3\) : environ 5,62 milliards.

Le calcul se termine en outre par :

- une copie GPU vers CPU des \(N_{\rm cubes}-1\) arêtes ;
- un `np.lexsort` hôte ;
- des coupes `components_at_cut` utilisant des boucles Python séquentielles.

Le pipeline n'est donc pas entièrement GPU.

### 4.5 \(K=10\) est-il le problème de calcul ?

Non. Tant que \(K\le64\), l'algorithme interroge déjà au moins 64 candidats puis extrait le K-ième par partition. Passer de \(K=3\) à \(K=10\) change peu le coût.

Les paramètres dominants sont :

1. `m_reg=64` et le solveur Gibbs ;
2. le nombre de cubes ;
3. le nombre de candidats nécessaire à la garde ;
4. le nombre de phases Borůvka.

La réponse à la question « 30 M et \(K=10\) est-il possible ? » est donc :

- **structurellement compatible avec un calcul streaming**, car l'explosion combinatoire en \(K\) a disparu ;
- **en mémoire probablement oui** sur un gros GPU ;
- **avec cette implémentation précise, le temps et la terminaison certifiée restent inconnus**.

Il n'existe pas de garantie asymptotique de temps subquadratique : une recherche exacte peut visiter \(O(N)\) sites par requête dans le pire cas, et le nombre de cubes doit lui-même croître avec l'étendue si la résolution physique reste fixe.

### 4.6 Le fallback global de \(\kappa\) multiplie le coût

Le budget de distorsion n'est testé qu'après la régularisation complète. Si \(s_{\max}\) dépasse le budget, le notebook relance un run complet avec un \(\kappa\) plus faible.

Une suite `8 → 4 → 2 → 1` peut donc :

- reconstruire les index ;
- retraiter les 1,92 milliard de voisins plusieurs fois ;
- finir à \(\kappa=1\), c'est-à-dire sans régularisation entropique.

C'est à la fois un risque de temps et un mauvais comportement statistique : un seul point lointain peut désactiver le lissage de toutes les zones denses.

---

## 5. Audit de l'implémentation

### 5.1 Points forts

Les qualités suivantes sont réelles :

- contrats de configuration stricts et typés ;
- séparation correcte de \(K\), \(\kappa\) et `m_reg` ;
- oracle CPU exact pour la géométrie quantifiée et les sites stockés, sur petites tailles ;
- calcul de la régularisation en streaming ;
- kernel CUDA de puissance évitant \(Q\times C\times3\) ;
- absence de fallback ANN silencieux ;
- MSF implicite et déterministe ;
- gestion correcte des égalités du champ ;
- invariance par translation/homothétie testée ;
- rapports machine-lisibles qui reconnaissent plusieurs limites ;
- documentation honnête sur l'absence de labels canoniques.

Le code est nettement au-dessus d'un prototype jetable. Il a été écrit avec une vraie attention aux contrats et à la reproductibilité.

### 5.2 Bloqueurs fonctionnels

| Bloqueur | Conséquence |
|---|---|
| `components_at_cut()` renvoie des labels de cubes | Impossible de produire directement les labels des 30 M observations. |
| `flat_selection="none"` | Aucune partition d'instances n'est disponible. |
| Pas de merge tree enrichi | Impossible d'appliquer proprement masse, persistance, bbox, moments ou prioris par classe. |
| Pas de `fit_predict`, `predict` ou `load` | API incomplète pour une utilisation de clustering. |
| Pas de checkpoints d'étapes | Une panne tardive impose de recommencer les index, la régularisation et le champ. |
| Coupes Python séquentielles | Une coupe de grande grille est lente avant même l'affectation des points. |

Le code indique explicitement `full_power_hgp_complete=False`. Ce drapeau est correct et doit rester ainsi, au minimum jusqu'à l'implémentation de l'appartenance observations-composantes, et tant que la complétude continue/exacte du top-K et de la discrétisation n'est pas établie.

### 5.3 Backend CUDA inutilisable par défaut sur les petites scènes

`RBCAuditedIndex` impose :

\[
\text{max\_k}\le\lfloor\sqrt N\rfloor.
\]

Avec les valeurs publiques :

- `m_reg=64` exige \(N\ge4096\) pour l'index brut ;
- `candidate_k_max+1=1025` exige \(N\ge1\,050\,625\) pour l'index de puissance.

Une frame SemanticKITTI d'environ \(10^5\) points ne passe donc pas avec la configuration par défaut. Un sous-nuage par classe ou par tuile est encore plus petit.

Le notebook ajuste le plafond à \(\lfloor\sqrt N\rfloor-1\), mais cette réduction peut empêcher la garde d'aboutir. Il faut un routeur explicite :

- backend exact cuVS/brute bloqué pour petits \(N\) ;
- backend exact spatial pour tailles moyennes ;
- index massif certifiable pour grands \(N\).

### 5.4 Solveur Gibbs correctement formulé, mal adapté à 30 M

Le solveur vectorisé est lisible et robuste pour les tests CPU. Pour 30 M, les 48 bissections et les nombreux temporaires CuPy sont trop coûteux.

L'optimisation pertinente est un kernel CUDA par ligne/bloc qui :

1. charge les 64 coûts en registres ou mémoire partagée ;
2. résout la température par Newton sécurisé par bissection ;
3. utilise un nombre d'itérations adapté au dtype ;
4. accumule directement \(z_i\), \(a_i\) et \(s_i\) ;
5. ne matérialise ni `neighbors`, ni `offsets` en \(Q\times64\times3\).

Les identifiants KNN sont également convertis en int64, alors que le reste de l'architecture est limité à des identifiants int32. Employer int32 réduirait mémoire et bande passante.

### 5.5 Dépendances et packaging

Le module public importe immédiatement l'ancien atlas. `torch`, scikit-learn, YAML et tqdm sont donc obligatoires même pour le backend NumPy/CuPy de `PowerCover3D`.

Il faudrait séparer :

```text
perg_hgp[power-cpu]
perg_hgp[power-cuda]
perg_hgp[atlas]
perg_hgp[test]
```

et rendre les imports publics paresseux.

---

## 6. Cas SemanticKITTI

### 6.1 L'expérience historique ne valide pas `PowerCover3D`

Le pipeline historique versionné :

- traite 200 frames de la séquence 08 par défaut ;
- utilise `SEMANTIC_MODE="Oracle"` ;
- regroupe séparément chaque classe sémantique vérité terrain ;
- utilise HGP avec \(K=3\), pas \(K=10\) ;
- ajoute une coordonnée temps, mais appelle le clusterer frame par frame sur XYZ ;
- effectue l'association temporelle via UOT/Kalman ;
- recopie les labels sémantiques vérité terrain dans les prédictions et ne remplace que les identifiants d'instances.

Le log donne :

| Mesure | Valeur |
|---|---:|
| LSTQ | 0,8253 |
| \(S_{assoc}\) | 0,6811 |
| \(S_{cls}\) | 1,0 par construction |
| Faux négatifs | 271 |
| Splits | 51 |
| Merges | 148 |
| ID switches | 337 |

Les événements de merge sont presque trois fois plus fréquents que les splits dans ce log, sans que ce simple compte mesure leur impact métrique relatif. Un \(K\) plus élevé peut réduire certains splits, mais risque d'augmenter encore les merges.

Les trois notebooks SemanticKITTI n'ont aucune sortie exécutée enregistrée. L'expérience historique ne constitue donc ni une validation de `PowerCover3D`, ni une preuve en faveur de \(K=10\).

Les définitions officielles des tâches sont disponibles sur [SemanticKITTI](https://semantic-kitti.org/tasks.html) et le jeu de données est décrit dans [Behley et al., ICCV 2019](https://openaccess.thecvf.com/content_ICCV_2019/papers/Behley_SemanticKITTI_A_Dataset_for_Semantic_Scene_Understanding_of_LiDAR_Sequences_ICCV_2019_paper.pdf).

### 6.2 Une grille cubique globale est mal adaptée au LiDAR

Pour une boîte illustrative de \(100\times100\times6\) mètres :

| Grille | Largeurs de cellule | \(H\) | Contribution publiée \(2H\) | Cubes |
|---|---:|---:|---:|---:|
| \(128^3\) | 0,781 × 0,781 × 0,047 m | 0,553 m | 1,106 m | 2,10 M |
| \(256^3\) | 0,391 × 0,391 × 0,023 m | 0,276 m | 0,553 m | 16,78 M |
| \(512\times512\times32\) | 0,195 × 0,195 × 0,188 m | 0,167 m | 0,334 m | 8,39 M |

Une grille \(256^3\) gaspille des cellules verticalement tout en restant grossière horizontalement. Avant même la distorsion \(s\), la contribution d'interleaving en rayon atteint 0,55 à 1,10 m. Ce n'est pas une erreur de position des objets, mais ce décalage du paramètre de filtration est déjà du même ordre que des séparations géométriques pertinentes pour les petites instances.

Sur une séquence agrégée, la boîte devient longue et largement vide. Une grille globale est alors soit trop grossière, soit gigantesque.

La configuration doit partir d'une taille de voxel physique et d'une tolérance d'interleaving, pas d'un nombre arbitraire de cellules par axe.

### 6.3 Effet attendu de \(K=10\)

Dans un modèle de Poisson homogène de dimension intrinsèque \(d\), pour un point de requête indépendant du processus :

\[
\lambda v_dR_K^d\sim\Gamma(K,1).
\]

À un point observé lorsque le voisin propre est compté, la convention de Palm donne plutôt \(\Gamma(K-1,1)\) pour \(K\ge2\). Les chiffres ci-dessous sont donc illustratifs pour le champ continu, pas une loi universelle des rayons évalués sur les observations.

Le coefficient de variation du volume KNN au point de requête indépendant vaut approximativement \(K^{-1/2}\) :

\[
K=3: 0{,}577,
\qquad
K=10: 0{,}316.
\]

Le gain de stabilité est donc réel dans ce modèle, environ 45 % sur cette mesure.

Mais l'échelle spatiale augmente comme \(K^{1/d}\) :

\[
\frac{R_{10}}{R_3}
\approx
\begin{cases}
1{,}49,&d=3,\\
1{,}83,&d=2.
\end{cases}
\]

Or un LiDAR échantillonne surtout des surfaces, donc une dimension intrinsèque souvent proche de 2, avec une densité fortement dépendante de la distance et de l'angle.

#### Gains plausibles

- moins de petites composantes dues au bruit ;
- moins de splits et de jitter sur les voitures denses ;
- hiérarchie plus stable dans les zones bien échantillonnées ;
- coût informatique presque inchangé par rapport à \(K=3\).

#### Risques

- une instance de moins de dix retours ne peut pas former seule une région de couverture d'ordre 10 ;
- perte de rappel sur les objets lointains et occultés ;
- fusion de personnes, vélos ou véhicules proches ;
- lissage entropique à travers une frontière lorsque les 64 voisins mélangent plusieurs objets ;
- accumulation temporelle naïve transformant les objets mobiles en traînées.

Je ne recommande donc pas \(K=10\) universel. Il doit être testé par classe, distance et nombre de retours, avec \(K\in\{3,5,10\}\).

### 6.4 Le backend n'est pas 4D

L'API exige `(N,3)` et le MSF accepte au plus trois dimensions. RBC est lui-même limité à l'euclidien en 2D/3D dans la documentation cuML.

Étendre directement la grille à 4D ferait exploser le nombre de cellules. Pour SemanticKITTI, le design raisonnable est :

1. clustering géométrique 3D par frame par défaut ;
2. association temporelle au niveau objet ;
3. tracker parcimonieux, sans matrice dense point-à-point ;
4. fenêtre multi-frame seulement après compensation de l'ego-mouvement **et** du mouvement objet estimé, éventuellement dans une boucle tracker → reclustering.

### 6.5 Sortie SemanticKITTI manquante

La définition discrète cohérente avec la thèse est :

\[
i\in C^{\rm discret}(r)
\iff
B_i^\kappa(r)\cap C\ne\varnothing,
\]

où :

\[
B_i^\kappa(r)
=
\{y:\|y-z_i\|^2+a_i\le r^2\}.
\]

Un site peut légitimement couvrir plusieurs composantes. Une simple localisation de \(x_i\) ou \(z_i\) dans un cube ne remplace pas ce test.

Tant que cette relation n'est pas calculée, `PowerCover3D` ne peut pas produire les fichiers `.label`, les volumes, les boîtes d'instances ou les entrées du tracker.

### 6.6 Design recommandé

```text
logits sémantiques et probabilités
  → points « thing »
  → frame, ou fenêtre courte compensée en pose et en mouvement objet
  → tuiles 3D chevauchantes en coordonnées locales
  → PowerCover par classe ou support sémantiquement compatible
  → merge tree et appartenance confirmée/incertaine
  → sélection plate avec prioris de taille/persistance
  → raccord des halos
  → association temporelle des objets
```

Le support local peut être filtré par :

- classe sémantique probable ;
- saut de profondeur ;
- cohérence de normale ;
- anneau LiDAR ou angle de vue ;
- fenêtre temporelle compensée.

Ces restrictions ne cassent pas l'identité de puissance ni la borne par \(s_i\), tant que le support utilisé est déclaré.

Une extension scientifique ultérieure pourrait utiliser un seuil de couverture pondéré :

\[
r_T(y)
=
\inf\left\{r:
\sum_{i:d_i(y)\le r}w_i\ge T
\right\},
\]

afin de compenser la densité d'acquisition ou les répétitions temporelles. Cette piste doit venir après un baseline non pondéré correctement validé.

---

## 7. Améliorations prioritaires

Les recommandations ci-dessous sont volontairement limitées aux points qui changent réellement la validité, la scalabilité ou l'utilité du système.

### Priorité 0 — produire un véritable clusterer

Implémenter :

1. l'intersection boule de puissance/composante ;
2. la carte de composantes induite par \(A^-\subseteq A^+\), puis un statut `confirmed`, `excluded` ou `uncertain` ;
3. un merge tree avec masse, persistance, bbox/OBB, moments et ambiguïté ;
4. une sélection plate ;
5. `fit_predict`, `predict` et `load`.

La sortie fondamentale doit rester une relation potentiellement multivaluée `site → composantes`. Une intersection avec une composante intérieure confirme une présence, mais pas une identité de branche unique si plusieurs composantes intérieures tombent dans la même composante extérieure. L'étiquette `confirmed` unique exige donc un appariement inner → outer non ambigu. L'adaptateur SemanticKITTI doit produire séparément une partition et déclarer sa règle de résolution des recouvrements comme post-traitement lorsqu'aucune composante n'est unique.

À 30 M, le test doit être un prédicat GPU boule/boîte ou boule/cellule, avec arrêt dès que deux composantes distinctes sont rencontrées. Une boucle naïve `sites × cubes` serait prohibitive.

Sans cette étape, aucun résultat SemanticKITTI n'est possible, même si le calcul 30 M termine.

### Priorité 1 — remplacer RBC + garde globale par un oracle de puissance exact

Exploiter le relèvement :

\[
(z_i,a_i)\mapsto(z_i,\sqrt{a_i})\in\mathbb R^4.
\]

Construire un LBVH/BVH 4D ou un branch-and-bound dont chaque nœud fournit une borne de distance. Recalculer les candidats frontières en float64 ou par intervalles dirigés.

Cette modification :

- supprime la dépendance au rang euclidien 3D ;
- évite le garde très lâche utilisant \(\min a_i\) ;
- donne une vraie terminaison certifiée ;
- vise un backend certifiable pour les tailles moyennes et massives.

La terminaison est finie, mais le coût pratique n'est pas garanti en pire cas : un branch-and-bound exact peut encore visiter \(O(N)\) sites pour une requête.

Le relèvement est exact, mais un LBVH 4D performant sur 30 M est un projet substantiel. La trajectoire réaliste est donc :

1. immédiatement, cuVS brute force bloqué pour les petits \(N\), plafond RBC dynamique et statut explicitement conditionnel pour les grands \(N\) ;
2. ensuite, LBVH/BVH 4D certifiable pour les tailles moyennes et massives.

Si RBC est conservé provisoirement, renommer tous les statuts en `conditional_guard_*` et stratifier les audits par distance, densité, poids \(a_i\) et position dans la grille.

### Priorité 2 — inverser le problème entropique sous budget de distorsion

Au lieu de fixer un \(\kappa\) global puis de recommencer le run, résoudre localement :

\[
\max_{q\in\Delta(A_i)}H(q)
\quad\text{sous}\quad
\sum_jq_j\|x_i-x_j\|^2\le B_i^2.
\]

Comme :

\[
s_i^2
=
\sum_jq_{ij}\|x_i-x_j\|^2,
\]

le budget contrôle directement la borne. Hors cas limites, la solution est encore une loi de Gibbs et fournit une perplexité locale \(\kappa_i=\exp H(q_i)\). Si \(B_i^2\) dépasse le coût de la loi uniforme, \(\beta=0\) et la solution est uniforme ; si \(B_i=0\) avec un minimum unique, la limite est une Dirac ; en cas de minima ex æquo, la masse s'uniformise sur ces minima.

Deux modes sont utiles :

- budget absolu global \(B_i=B\), donnant directement \(s\le B\) ;
- budget relatif \(B_i=\gamma R_K(x_i)\), plus adapté aux variations de densité LiDAR.

Le budget relatif borne la distorsion par rapport à l'échelle KNN locale, mais ne détecte pas à lui seul une frontière : \(R_K\) peut même y augmenter. Une règle plus sûre est \(B_i=\min(B_{\rm abs},\gamma R_K(x_i))\), complétée par un support compatible avec les discontinuités sémantiques, de normale ou de profondeur. Cette formulation évite plusieurs runs complets et conserve davantage d'entropie dans les régions admissibles.

Le solveur doit être fusionné dans un kernel CUDA par ligne, avec Newton sécurisé, itérations adaptées au float32 et indices int32.

### Priorité 3 — définir la grille par une tolérance physique

Ajouter :

- `voxel_size` ou `max_spatial_error` ;
- shapes anisotropes calculées depuis la bbox ;
- refus du run si la borne dépasse la tolérance métier ;
- tuiles XY avec halos et raccord explicite ;
- raffinement adaptatif autour des fusions et cellules ambiguës.

La largeur du halo doit couvrir au moins le rayon nécessaire au support `m_reg`, le plus grand rayon de filtration étudié et les marges \(s\), \(H\) et numériques pertinentes. Sans ce contrat, les voisinages, les sites et les composantes changent artificiellement au bord des tuiles.

Pour des cellules variables, utiliser :

\[
\widehat g(c_Q)\pm(H_Q+\delta_Q).
\]

Les forêts intérieure et extérieure ne seront alors plus de simples translations ; elles devront être calculées séparément.

### Priorité 4 — rendre la certification numérique réellement bout en bout

Ajouter au rapport :

- \(\varepsilon_X\), erreur de quantification de l'entrée ;
- une borne supérieure dirigée de \(s\) ;
- des intervalles de puissance dirigés ;
- une vérification d'unicité et d'ordre des voisins ;
- un résidu KKT/primal-dual de Gibbs et des bornes d'accumulation sur \(q,z,a\) ;
- le contrat fin \(H+\delta\) pour `base_forest` ;
- le contrat d'enveloppe \(2(H+\delta)\) ;
- un vocabulaire qui distingue preuve, condition et audit empirique.

Corriger également le faux échec `entropy_target_deviation_max` sur les minima ex æquo.

### Priorité 5 — exécuter une vraie campagne GPU et ajouter des checkpoints

Avant toute revendication :

1. exécuter 100 k, 1 M, 3 M, 10 M et 30 M ;
2. versionner les rapports de temps, VRAM/RSS et histogrammes de candidats ;
3. profiler séparément RBC brut, Gibbs, index de puissance, champ et Borůvka ;
4. sauvegarder après les sites régularisés, après le champ et après le MSF ;
5. ajouter CI CPU et un smoke test CUDA réel ;
6. comparer nuage synthétique et données LiDAR réelles.

Séparer enfin les dépendances `power-cpu`, `power-cuda` et `atlas`.

---

## 8. Protocole expérimental décisif

### 8.1 Ablation minimale

Avec exactement les mêmes sémantiques, prioris et tracker :

| Famille | Configuration |
|---|---|
| Baselines | DBSCAN, HDBSCAN |
| Historique | HGP \(K=3\) |
| Effet de \(K\) | PowerCover \(K\in\{3,5,10\},\kappa=1\) |
| Effet de l'entropie | mêmes \(K\), \(\kappa\in\{2,4\}\) ou budget local |
| Effet de la grille | plusieurs tailles physiques et inner/base/outer |

Il est indispensable de comparer :

\[
(K=10,\kappa=1)
\quad\text{à}\quad
(K=10,\kappa>1),
\]

sinon l'effet de l'entropie est confondu avec celui de \(K\).

### 8.2 Deux niveaux SemanticKITTI

1. **Sémantique oracle**, afin d'isoler la qualité du clustering d'instances.
2. **Sémantique prédite**, afin de mesurer le vrai système panoptique.

Le réglage de \(K\), de \(\kappa\), des coupes et des prioris doit être réalisé sur des séquences d'entraînement ou un sous-ensemble de développement distinct. La séquence 08 doit ensuite être utilisée une seule fois pour la validation finale, afin d'éviter une fuite entre screening et évaluation.

### 8.3 Métriques obligatoires

- PQ, RQ et SQ des classes `thing` ;
- \(S_{assoc}\) et LSTQ ;
- splits, merges, faux négatifs et ID switches ;
- rappel par classe ;
- rappel par distance au capteur ;
- rappel par nombre de points : `<10`, `10–49`, `≥50` ;
- désaccord inner/outer et taux `uncertain` ;
- \(s\), \(H\), \(\delta\), \(\varepsilon_X\) et borne totale ;
- temps par étape, débit, VRAM, RAM et volume disque ;
- histogramme des candidats de puissance.

### 8.4 Critère go/no-go proposé

Retenir \(K=10\) seulement si :

- le gain de PQ `thing` ou de \(S_{assoc}\) est d'au moins deux points absolus sur la séquence complète ;
- le gain subsiste avec une sémantique prédite ;
- le rappel des objets lointains et de moins de 50 points ne baisse pas de plus de 2–3 points ;
- les merges n'augmentent pas de façon significative ;
- moins de 1 % des points `thing` restent ambigus entre les enveloppes à la coupe choisie ;
- le temps et la mémoire respectent le budget matériel annoncé.

Le gain et l'évolution des merges doivent être accompagnés d'intervalles de confiance par bootstrap de frames ou de séquences, plutôt que d'un seuil ponctuel seul.

Sinon, conserver \(K=3\) ou \(K=5\) pour les petites classes et réserver \(K=10\) aux véhicules denses.

---

## 9. Verdict final

### Mathématiques

Le modèle en arithmétique réelle est bon. L'identité de puissance, le relèvement 4D, la borne par \(s\), la K-couverture, la 1-lipschitzianité, l'enveloppe cubique et le MSF forment un ensemble cohérent.

La certification publiée n'est toutefois pas encore formelle : elle omet l'erreur d'entrée float32, utilise un \(s\) non arrondi vers l'extérieur, dépend d'un ordre RBC audité seulement par échantillon et n'offre aucune garantie de terminaison avant 1 024 candidats.

### Passage à l'échelle

La mémoire est plausible et l'algorithme évite bien la combinatoire en \(K\). \(K=10\) est donc compatible avec l'architecture.

Le débit reste la question ouverte. Le solveur Gibbs effectue un volume massif d'exponentielles et de passages mémoire, les requêtes de puissance peuvent escalader, et Borůvka rescane les arêtes à chaque phase. Sans résultat Blackwell exécuté, annoncer 30 M serait prématuré.

### Implémentation

L'implémentation est soigneuse, défensive et bien testée sur CPU. C'est un excellent prototype de recherche.

Elle n'est pas encore satisfaisante comme clusterer : pas de labels des observations, pas de merge tree enrichi, pas de sélection plate, pas de backend CUDA pour petits sous-nuages, pas de checkpoints, pas de benchmark GPU versionné et pas de CI.

### SemanticKITTI

\(K=10\) peut réduire le bruit et certains splits sur les voitures denses. Il peut aussi supprimer les instances de moins de dix retours et accroître les merges, déjà dominants dans le log historique.

La bonne stratégie n'est pas un \(K=10\) universel sur une grille globale. Il faut une résolution physique anisotrope ou adaptative, des tuiles, un backend multi-régime, une régularisation locale sous budget et une ablation par classe/distance.

### Conclusion en une phrase

> `PowerCover3D` est une excellente base de hiérarchie \(H_0\) massive, mais il faut encore fermer le certificat numérique, optimiser Gibbs, remplacer l'oracle RBC conditionnel et surtout produire l'appartenance observations-composantes avant de pouvoir le considérer comme un HGP-Clusterer 30 M ou conclure sur SemanticKITTI.

---

## Références externes

- [NVIDIA cuML — `NearestNeighbors`](https://docs.rapids.ai/api/cuml/nightly/api/generated/cuml.neighbors.nearestneighbors/)
- [SemanticKITTI — tâches officielles](https://semantic-kitti.org/tasks.html)
- [Behley et al., *SemanticKITTI: A Dataset for Semantic Scene Understanding of LiDAR Sequences*, ICCV 2019](https://openaccess.thecvf.com/content_ICCV_2019/papers/Behley_SemanticKITTI_A_Dataset_for_Semantic_Scene_Understanding_of_LiDAR_Sequences_ICCV_2019_paper.pdf)
