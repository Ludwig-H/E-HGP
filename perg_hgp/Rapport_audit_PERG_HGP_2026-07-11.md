# Audit mathématique, logiciel et expérimental de PERG-HGP

**Date :** 11 juillet 2026  
**Dépôt :** `Ludwig-H/E-HGP`  
**Commit audité :** [`150c1ce16387e45e085c32b17c15847ecac9d390`](https://github.com/Ludwig-H/E-HGP/tree/150c1ce16387e45e085c32b17c15847ecac9d390) — *fix Blackwell kappa fallback*  
**Thèse :** chapitre 1, pages 1–8 du manuscrit (pages PDF 27–34) ; chapitre 2, pages 11–22 (pages PDF 37–48).

## Résumé exécutif

Le verdict dépend de l’objet que l’on appelle « PERG-HGP ».

| Question | Réponse honnête |
|---|---|
| Le modèle entropique est-il mathématiquement cohérent ? | **Oui pour le nouveau backend `PowerCover3D`.** Il définit proprement une couverture par boules de puissance et possède une borne d’entrelacement en rayon. |
| Calcule-t-il exactement HGP-Clusterer tel que défini dans la thèse ? | **Non.** Il approxime de façon contrôlée le \(H_0\) du champ K-NN original ; il ne reconstruit ni le complexe HGP complet, ni les amas discrets de points. |
| L’atlas `PERGHGPClusterer` est-il exact ? | **Non.** Les cofaces acceptées sont cohérentes pour l’atlas trouvé, mais la complétude de l’atlas n’est pas garantie. C’est un prototype heuristique. |
| Le code est-il bien conçu ? | **Le cœur CPU de `PowerCover3D` est bon pour un logiciel de recherche.** Contrats, diagnostics, oracles et tests sont sérieux. |
| Le passage GPU à 30 millions de points est-il établi ? | **Non.** La conception mémoire est crédible, mais aucun résultat CUDA/30 M exécuté n’est versionné. |
| Peut-on déjà l’utiliser comme remplaçant massif de HGP-Clusterer ? | **Non.** Il manque surtout l’appartenance des observations aux composantes, un merge tree exploitable et la sélection plate. |
| \(K=10\) a-t-il de bonnes chances d’améliorer SemanticKITTI ? | **C’est une hypothèse plausible pour les véhicules proches et denses, pas un résultat.** Un passage naïf de \(K=3\) à \(K=10\) peut au contraire augmenter les fusions et les faux négatifs sur les objets petits ou lointains. |

Ma conclusion générale est donc la suivante : **la direction mathématique de `PowerCover3D` est bonne et mérite d’être poursuivie ; l’implémentation actuelle est un moteur de hiérarchie cubique prometteur, pas encore un clusterer d’instances prêt pour SemanticKITTI ni une validation du cas 30 M.**

## 1. Périmètre et vérifications

L’audit a couvert :

- les deux premiers chapitres de la thèse, avec contrôle visuel des définitions et équations ;
- l’ensemble du package actif `perg_hgp` ;
- le backend `power_cover_3d_cuda`, ses contrats, son solveur Gibbs, son garde top-K, son champ cubique et son MSF implicite ;
- les oracles CPU, tests, notebooks et scripts de benchmark ;
- l’ancienne implémentation `HGP-old` et l’expérience SemanticKITTI ;
- l’historique Git récent jusqu’au commit audité.

Vérifications locales effectuées :

- compilation syntaxique de tous les modules Python : réussie ;
- 75 tests ciblant `PowerCover3D`, les oracles de puissance, le MSF cubique et le contrat CUDA : **75 réussis** ;
- construction du wheel `perg_hgp-0.2.0` : réussie, avec les sous-packages `backends` et `reference` inclus ;
- essai CPU end-to-end avec \(K=10\), \(\kappa=4\), grille \(16^3\) : terminé avec toutes les requêtes gardées ;
- deux essais CPU supplémentaires à 2 000 et 10 000 points : terminés avec une fraction de garde égale à 1 ;
- comparaison du top-K de puissance contre l’oracle bloqué : erreur absolue maximale \(2{,}1\times10^{-7}\) sur le cas aléatoire testé ;
- covariance par homothétie : facteur observé \(3{,}2500006\) pour une homothétie de facteur \(3{,}25\).

Ces contrôles valident le chemin CPU et la logique testable sans GPU. Ils ne remplacent pas l’exécution du noyau CuPy, de RAPIDS RBC et du benchmark 30 M sur la machine cible. La suite historique de l’atlas n’a pas été réexécutée dans cet environnement, qui ne disposait pas de PyTorch.

## 2. Contrat provenant des chapitres 1 et 2

### 2.1 Champ K-NN et hiérarchie de Hartigan

La thèse définit, pour tout \(y\in\mathbb R^p\),

\[
r_K(y)=\inf\{r\ge 0:\#(\overline B(y,r)\cap X)\ge K\},
\]

et le niveau paramétré par le rayon

\[
L_K(r)=\{y:r_K(y)\le r\}.
\]

La densité K-NN est

\[
\widehat f_K(y)=\frac{K}{n\omega_p r_K(y)^p}.
\]

La hiérarchie pertinente est celle des composantes connexes de \(L_K(r)\) lorsque \(r\) varie. Deux conséquences du manuscrit sont essentielles pour juger le code :

1. le champ doit être pensé dans tout l’espace, pas uniquement aux observations ;
2. l’amas discret associé à une composante continue \(C\) n’est pas \(C\cap X\), mais l’ensemble des observations couvertes par \(C\), c’est-à-dire \(X\cap\delta_r(C)\).

Le second point est précisément l’étage qui manque encore à `PowerCover3D`.

### 2.2 Cas \(K=1\) et rôle du MSF

Pour \(K=1\), \(L_1(r)\) est une union de boules. Ses composantes sont celles du graphe géométrique et du MST élagué au même seuil. Le chapitre 2 rappelle plus généralement qu’un MSF préserve les composantes d’un graphe pondéré à tous les sous-niveaux.

Le MSF cubique de `PowerCover3D` est donc la bonne structure mince pour conserver la hiérarchie \(H_0\) de la filtration discrétisée.

### 2.3 Cas \(K\ge2\)

Les objets élémentaires sont les intersections de \(K\) boules. Les amas naturels peuvent se recouvrir : ils ne forment pas nécessairement une partition de \(X\). Toute procédure produisant un label unique par point — vote, EOM, MILP géométrique — est donc un **post-traitement** et non la définition de la hiérarchie de Hartigan.

## 3. Deux implémentations qu’il faut distinguer

Le dépôt sépare désormais explicitement les deux chemins dans le [README de `perg_hgp`](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/README.md#L1-L15).

| Chemin | Objet calculé | Statut réel |
|---|---|---|
| `PERGHGPClusterer` | Atlas explicite de témoins, cofaces Rank-Gabriel, graphe dual, MSF, condensation et vote | Prototype expérimental ; atlas incomplet |
| `PowerCover3D` | Champ K-ième distance de puissance sur une grille, enveloppes intérieure/extérieure et MSF cubique | Chemin massif recommandé ; approximation contrôlée de \(H_0\) |

Le second ne doit pas être décrit comme une accélération fidèle ligne à ligne du premier. C’est une **reformulation du problème au niveau du champ continu et de son \(H_0\)**.

### L’entropie n’est pas, à elle seule, la raison du passage au GPU

La narration « l’entropie transforme un problème combinatoire CPU en problème GPU » est trop forte. Le passage à l’échelle vient surtout de quatre décisions :

1. ne plus énumérer les simplexes/cofaces ;
2. interroger directement le champ scalaire en des centres de cubes ;
3. utiliser un top-K spatial en streaming ;
4. calculer un MSF sur une grille implicite.

Ces quatre étapes fonctionneraient aussi pour \(\kappa=1\), donc pour le champ K-NN non régularisé. L’entropie apporte une nouvelle géométrie lissée et une borne de stabilité ; elle n’est pas le mécanisme unique de parallélisation.

Il faut également distinguer :

- le Gibbs local qui régularise chaque site ;
- le soft-rank par polynômes symétriques de l’atlas Rank-Gabriel.

`PowerCover3D` n’utilise que le premier. Son ordre \(K\) final reste dur.

## 4. Audit mathématique de `PowerCover3D`

### 4.1 Problème entropique local

Pour chaque observation \(x_i\), un support dur \(A_i\) de taille `m_reg` est construit. Le backend résout

\[
q_i^\kappa\in\arg\min_{q\in\Delta(A_i)}
\sum_{j\in A_i}q_j\lVert x_i-x_j\rVert^2,
\qquad H(q)\ge\log\kappa.
\]

Le problème est convexe : l’objectif est linéaire et le sur-niveau d’une fonction concave \(H\) est convexe. Hors cas dégénérés, la contrainte est active et la solution a la forme de Gibbs

\[
q_{ij}\propto\exp\!\left(-\frac{\lVert x_i-x_j\rVert^2}{\eta_i}\right).
\]

Le solveur moderne traite proprement \(\kappa=1\), \(\kappa=m_{\rm reg}\), les minima ex æquo, l’invariance d’unités et les résidus numériques. L’implémentation est dans [`spatial_core.py`, lignes 127–315](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/spatial_core.py#L127-L315).

Le support dur fait partie du modèle implémenté. Si l’intention était un Gibbs global sur les \(n\) observations, aucune borne de troncature au-delà de `m_reg` n’est fournie. Il est plus propre de définir officiellement le modèle local.

### 4.2 Boules de puissance

Le code pose

\[
z_i=\sum_jq_{ij}x_j,
\qquad
a_i=\sum_jq_{ij}\lVert x_j-z_i\rVert^2,
\]

et

\[
d_i^\kappa(y)^2
=\sum_jq_{ij}\lVert y-x_j\rVert^2
=\lVert y-z_i\rVert^2+a_i.
\]

Le champ calculé est

\[
r_{K,\kappa}(y)
=K\text{-ième plus petite valeur de }
\{d_i^\kappa(y)\}_{i=1}^n.
\]

Il s’agit d’un **nouvel estimateur régularisé par couverture de puissance**, pas littéralement de l’estimateur K-NN inchangé. On peut lui associer

\[
\widehat f_{K,\kappa}(y)
=\frac{K}{n\omega_3r_{K,\kappa}(y)^3},
\]

mais toutes les garanties additives doivent rester formulées en rayon.

### 4.3 Borne de régularisation

Posons

\[
s_i=\sqrt{\lVert x_i-z_i\rVert^2+a_i},
\qquad s=\max_i s_i.
\]

Par l’inégalité triangulaire dans \(L^2(q_i)\),

\[
\left|d_i^\kappa(y)-\lVert y-x_i\rVert\right|\le s_i.
\]

Le K-ième ordre statistique est 1-lipschitz pour la norme uniforme. Par conséquent,

\[
|r_{K,\kappa}(y)-r_K(y)|\le s
\]

et

\[
L_K(r-s)\subseteq L_{K,\kappa}(r)\subseteq L_K(r+s).
\]

Cette partie est mathématiquement correcte et le maximum \(s\) est bien calculé sur toutes les observations dans [`spatial_core.py`, lignes 318–453](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/spatial_core.py#L318-L453).

Ce résultat ne garantit pas :

- une erreur additive uniforme sur la densité ;
- une correspondance point par point des clusters ;
- la préservation d’une branche dont la persistance est comparable à \(s\) ;
- la consistance de Hartigan pour \(K=10\) fixé.

### 4.4 Enveloppe cubique

Chaque \(d_i^\kappa\) est 1-lipschitz et le K-ième ordre statistique conserve cette propriété. Pour un cube fermé \(Q\), de centre \(c_Q\) et de demi-diagonale \(h_Q\),

\[
g(c_Q)-h_Q\le g(y)\le g(c_Q)+h_Q,
\qquad g=r_{K,\kappa}.
\]

Avec \(H=\max_Qh_Q\), les complexes cubiques intérieur et extérieur vérifient un entrelacement de largeur \(2H\). En ajoutant la marge numérique globale \(\delta_{\rm num}\), la borne publiée est

\[
\boxed{\varepsilon_{\rm total}=s+2(H+\delta_{\rm num}).}
\]

Le choix des cubes fermés impose la 26-connexité en dimension 3. Le poids d’une arête de cubes est \(\max(g_u,g_v)\), puis un MSF conserve toutes les composantes. Cette construction est correcte ; voir [`spatial_core.py`, lignes 805–914](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/spatial_core.py#L805-L914) et [`cubical_msf.py`](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/cubical_msf.py).

La boîte englobante des \(z_i\) suffit pour \(H_0\) : la projection coordonnée sur cette boîte ne peut augmenter la distance à aucun centre et rétracte les chemins extérieurs.

### 4.5 Portée exacte de la garantie

La garantie porte sur les filtrations et leur module de persistance \(H_0\). Une branche dont la persistance dépasse largement \(2\varepsilon_{\rm total}\) est robuste à cette approximation. En revanche, [`fusion_intervals()`](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/api.py#L167-L187) ne doit pas être interprété comme un appariement canonique, fusion par fusion, avec le merge tree K-NN original.

Le critère scientifique utile est donc simple : **si \(s+2(H+\delta_{\rm num})\) est du même ordre que la persistance d’une branche ou que l’écart physique entre deux objets, le calcul ne permet pas de conclure sur cette branche.**

### 4.6 Certification numérique conditionnelle

Le garde de puissance est correct si les \(C+1\) candidats fournis sont réellement les premiers voisins euclidiens ordonnés. Pour le \((C+1)\)-ième voisin à distance \(d_{\rm guard}\), tout site omis satisfait

\[
\lVert y-z_i\rVert^2+a_i
\ge d_{\rm guard}^2+\min_j a_j.
\]

Le code double \(C\) jusqu’à séparation du K-ième candidat ou jusqu’au plafond, sans repli silencieux. C’est une bonne conception.

La réserve est RAPIDS RBC. Le wrapper déclare lui-même `exact=False` et `numerically_proven=False` dans [`cuda_runtime.py`, lignes 56–70](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/cuda_runtime.py#L56-L70), puis audite quelques requêtes contre cuVS brute force. Cet audit est une excellente détection de régression, mais pas une preuve pour les requêtes non testées. La [documentation NVIDIA](https://docs.rapids.ai/api/cuml/nightly/api/generated/cuml.neighbors.nearestneighbors/) ne présente explicitement comme exact que le chemin brute force.

De même, la marge float32 est une enveloppe déclarée, pas une arithmétique dirigée. La formulation correcte est donc :

> hiérarchie enveloppée conditionnellement à l’ordre euclidien RBC et au modèle d’erreur float32 déclaré.

Il serait préférable de renommer `power_knn_certified_fraction` en `conditional_guard_pass_fraction`, ou de conserver les deux champs séparément.

## 5. Audit de l’atlas `PERGHGPClusterer`

### 5.1 Soft-rank

Le champ de rang mou construit à partir des polynômes symétriques élémentaires est mathématiquement naturel et converge vers les statistiques d’ordre lorsque la température tend vers zéro. Mais l’implémentation travaille sur seulement `m_active` sites, utilise des clamps, clippe et renormalise les poids, et termine à température non nulle. Il n’existe ni borne de queue globale ni borne de convergence des points fixes.

### 5.2 Témoins progressifs

La recherche de témoins dans [`witnesses.py`](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/witnesses.py) est heuristique : initialisations limitées, lifts locaux, budgets, absence de couverture exhaustive des bassins et fallbacks arbitraires lorsqu’un filtre vide un lot. Une coface importante peut donc ne jamais être proposée.

### 5.3 Cofaces et test de Gabriel

Pour une coface candidate, recalculer son miniball additif et vérifier les mêmes \(K+1\) premiers sites à son centre est un certificat cohérent en arithmétique exacte et hors égalités. Cela certifie la coface **présente**, pas l’absence de cofaces manquantes.

À \(K=10\), le solveur actif peut essayer

\[
\sum_{j=1}^4\binom{11}{j}=561
\]

supports par coface. Ce chemin ne doit pas être optimisé en priorité pour 30 M : l’approche cubique est structurellement meilleure.

### 5.4 Graphe dual et sortie plate

L’étoile de \(K\) arêtes remplaçant le clique d’une coface conserve correctement son \(H_0\), et le MSF conserve le graphe explicite à tous les seuils. Mais l’atlas omet :

- les facettes non incidentes à une coface trouvée ;
- certaines cofaces non découvertes ;
- les vraies naissances géométriques des facettes ;
- les facettes isolées.

La condensation et le vote produisent ensuite une partition heuristique, alors que les amas discrets de la thèse peuvent se recouvrir. Le propre rapport du code l’admet dans [`estimator.py`, lignes 724–755](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/estimator.py#L724-L755).

Enfin, le mode nommé `atlas_exact` est trompeur : il signifie au mieux « connectivité exacte du graphe de l’atlas produit », pas « atlas complet » ni « HGP exact ». Un nom comme `candidate_atlas_consistent` serait plus sûr.

### 5.5 Paramètre entropique historique

Le choix historique `K_rho=max(32,3*K)` n’a pas de justification géométrique. Pour \(K=10\), il impose \(K_\rho=32\), donc un lissage très fort. Le nouveau `PowerCoverConfig` corrige ce problème en distinguant explicitement `K`, `kappa` et `m_reg` dans [`contracts.py`, lignes 78–158](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/contracts.py#L78-L158).

## 6. Qualité de l’implémentation

### 6.1 Points forts

Le nouveau backend est nettement plus sérieux que le prototype initial :

- séparation nette du chemin massif et de l’atlas ;
- validation stricte des paramètres ;
- normalisation isotrope avant calcul float32 ;
- Gibbs et voisinages traités en chunks ;
- aucune matrice globale \(N\times m_{\rm reg}\) ;
- kernel fusionné pour les valeurs de puissance ;
- garde top-K sans acceptation silencieuse ;
- MSF 26-connexe sans liste explicite des arêtes ;
- rapports de précision, mémoire et matériel lisibles par machine ;
- oracles CPU float64 et cas limites ;
- documentation inhabituellement honnête sur les garanties et non-garanties.

Les 75 tests CPU ciblés qui passent couvrent notamment le Gibbs, les ex æquo, les transformations d’unités, le top-K contre brute force, l’oracle HGP de puissance, \(K=1\), la 26-connexité et le déterminisme.

### 6.2 Bloqueurs actuels

#### Bloqueur 1 — aucune appartenance canonique des observations

La sortie primaire est une hiérarchie de **cubes**. [`components_at_cut()`](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/perg_hgp/perg_hgp/backends/power_cover_3d_cuda/api.py#L189-L195) renvoie des labels de cubes, pas de points. Le rapport déclare `flat_selection="none"` et la documentation reconnaît que l’affectation canonique n’est pas implémentée.

Pour retrouver l’analogue de la définition discrète de la thèse, l’indice \(i\) doit appartenir à une composante \(C\) lorsque sa boule de puissance

\[
B_i^\kappa(r)=\{y:\lVert y-z_i\rVert^2+a_i\le r^2\}
\]

intersecte \(C\). Une simple localisation de \(x_i\) ou \(z_i\) dans un cube n’est pas équivalente.

#### Bloqueur 2 — pas de merge tree exploitable par les a priori

Le MSF compact contient la topologie, mais l’API ne construit pas encore un arbre de composantes avec masse, observations couvertes, boîte englobante, moments géométriques et ambiguïtés intérieur/extérieur. Sans cet arbre, les a priori de dimensions physiques utilisés dans l’expérience SemanticKITTI ne peuvent pas être appliqués proprement.

#### Bloqueur 3 — aucun résultat CUDA versionné

Les notebooks `PERG_HGP_Blackwell_30M.ipynb` et `PERG_HGP_Colab_GPU_Benchmark.ipynb` ont zéro cellule exécutée et zéro sortie enregistrée. Ils constituent de bons protocoles, pas des benchmarks. Il n’y a pas non plus de workflow CI dans le dépôt.

#### Bloqueur 4 — résolution globale de la grille

Une grille uniforme globale donne une erreur \(H\) liée à la taille totale de la boîte, pas à la densité locale des observations. Sur une scène de 200 m de largeur, une grille \(128^3\) a des cellules horizontales d’environ 1,56 m et une demi-diagonale proche de 1,1 m. L’incertitude \(2H\) est alors de l’ordre de la largeur d’un piéton.

La configuration accepte déjà une forme anisotrope. À nombre de cubes comparable, \(512\times512\times32\), des tuiles ou un octree sont beaucoup plus pertinents qu’une grille cubique sur une scène LiDAR aplatie.

#### Limitation opérationnelle — défaut CUDA incompatible avec les petits sous-nuages

Avec `candidate_k_max=1024`, l’index construit en pratique une capacité de 1 025 voisins, alors que le wrapper RBC refuse `max_k > floor(sqrt(N))`. Les paramètres CUDA par défaut demandent donc \(N\ge1\,050\,625\). Le notebook 30 M adapte ce plafond, mais l’API publique ne le fait pas.

Cette limitation est particulièrement gênante pour SemanticKITTI, où le script historique clusterise séparément chaque frame et chaque classe. Il faut un backend brute-force exact explicite pour les petits \(N\), ou un sélecteur automatique documenté.

### 6.3 Passage à 30 millions de points

Le plan mémoire analytique pour \(N=30\,000\,000\), `m_reg=64`, chunks 262 144/65 536 et plafond de 1 024 candidats donne :

| Grille | Cubes | Arêtes implicites | Pic pipeline analytique | Résultat MSF compact |
|---:|---:|---:|---:|---:|
| \(128^3\) | 2 097 152 | 26 822 908 | 4,27 Gio | 0,031 Gio |
| \(256^3\) | 16 777 216 | 216 338 940 | 4,27 Gio | 0,25 Gio |

Ces chiffres sont cohérents avec le code, mais ils n’incluent pas exactement tous les workspaces internes RAPIDS/RMM/CUB. Ils doivent rester présentés comme estimations.

Le coût réel est important : la régularisation traite environ \(30\text{ M}\times64=1{,}92\) milliard de voisins, en streaming ; le Borůvka implicite rescannera le stencil lors de plusieurs phases ; le résultat final est trié sur l’hôte. La mémoire rend le calcul plausible sur une grosse Blackwell, mais le temps reste inconnu.

`PowerCover3D.fit` est actuellement in-core et sans checkpoint. Si le maximum global \(s\) invalide le \(\kappa\) choisi par le pilote, le notebook peut relancer toute la régularisation 30 M plusieurs fois, jusqu’à \(\kappa=1\). Un succès technique peut donc finalement correspondre à l’absence de régularisation entropique.

Enfin, `components_at_cut()` reconstruit les composantes sur CPU avec une boucle Python sur les sommets et arêtes. À \(256^3\), ce chemin n’est pas une primitive interactive crédible ; le futur merge tree doit être construit une seule fois dans un backend compilé/GPU.

Verdict de passage à l’échelle :

- **architecture mémoire : plausible** ;
- **correction CPU : bien étayée** ;
- **correction CUDA réelle : non démontrée** ;
- **débit 30 M : inconnu** ;
- **sortie utile au clustering de points : incomplète**.

## 7. Ce que montre réellement l’expérience SemanticKITTI du dépôt

L’expérience historique n’est ni une segmentation sémantique, ni un clustering 4D pur.

1. Elle utilise les 200 premières frames de la séquence 08 et `SEMANTIC_MODE="Oracle"` dans le [script historique](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py#L128-L145).
2. Les classes `thing` sont extraites depuis la vérité terrain avant le clustering ([lignes 450–513](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py#L450-L513)).
3. Le traitement est frame par frame et le clustering reçoit \((x,y,z)\), même si une colonne temporelle avait été construite ([lignes 1116–1155](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py#L1116-L1155)). La cohérence temporelle vient du tracker, de l’UOT et du Kalman.
4. HGP utilise \(K=3\), pas \(K=10\) ([lignes 582–589](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py#L582-L589)).
5. À l’évaluation, les labels sémantiques sont recopiés depuis la vérité terrain et seuls les identifiants d’instance sont modifiés ([lignes 1499–1534](https://github.com/Ludwig-H/E-HGP/blob/150c1ce16387e45e085c32b17c15847ecac9d390/HGP-old/tests/SemanticKITTI/HGP-Clusterer_SemanticKITTI_4DPanoptic_final.py#L1499-L1534)).

Le log conservé donne :

\[
\mathrm{LSTQ}=0{,}82527,
\qquad S_{\rm assoc}=0{,}68107,
\qquad S_{\rm cls}=1.
\]

Le LSTQ est donc mécaniquement

\[
\sqrt{S_{\rm cls}S_{\rm assoc}}
=\sqrt{0{,}68107}
=0{,}82527.
\]

Ce score n’est pas comparable à une méthode 4D panoptique end-to-end. Le protocole officiel distingue bien la qualité sémantique et la qualité d’association dans le [benchmark SemanticKITTI](https://semantic-kitti.org/tasks.html).

Le même log compte :

- 271 faux négatifs ;
- 51 splits ;
- 148 merges ;
- 337 changements d’identité.

Les erreurs dominantes sont donc déjà la perte d’objets, la sous-segmentation et l’instabilité temporelle. Le passage à \(K=10\) ne répond pas automatiquement à ce profil et peut renforcer les merges/FN.

Enfin, les trois notebooks SemanticKITTI versionnés ont zéro sortie. Le log séparé est le seul résultat exploitable ; aucune baseline HDBSCAN/DBSCAN/HGP avec le même tracker et aucune ablation en \(K\) ne sont présentes.

## 8. \(K=10\) : bénéfices et risques attendus

Dans un processus de Poisson homogène de dimension intrinsèque \(q\),

\[
\lambda v_qR_K^q\sim\Gamma(K,1).
\]

Le coefficient de variation de \(R_K^q\) vaut \(K^{-1/2}\). Passer de \(K=3\) à \(K=10\) le réduit de \(0{,}577\) à \(0{,}316\), soit environ 45 %.

Mais l’échelle spatiale typique augmente comme

\[
\frac{R_{10}}{R_3}\simeq(10/3)^{1/q}.
\]

Elle est multipliée par environ 1,49 en dimension 3, et 1,83 sur une surface de dimension intrinsèque 2.

### Gains plausibles

- moins de petits modes dus au bruit ;
- hiérarchies plus stables pour les voitures proches et bien échantillonnées ;
- diminution possible des splits et du jitter frame à frame ;
- meilleur retard de certains ponts de bruit.

### Risques

- tout objet de moins de dix retours ne peut former seul un niveau d’ordre 10 ;
- plus de faux négatifs à rayon fixé ;
- après recalibrage de la coupe, davantage de fusions entre objets proches ;
- le support entropique peut traverser une frontière d’instance ;
- les petites classes et les objets lointains sont pénalisés.

La densité LiDAR dépend fortement de la portée et de l’incidence. SemanticKITTI est constitué de scans 360° clairsemés et séquentiels à 10 Hz ; voir le [papier du jeu de données](https://openaccess.thecvf.com/content_ICCV_2019/papers/Behley_SemanticKITTI_A_Dataset_for_Semantic_Scene_Understanding_of_LiDAR_Sequences_ICCV_2019_paper.pdf). Un \(K\) fixe mesure donc en partie la densité d’acquisition, pas uniquement la densité des objets.

| Tâche | Pronostic actuel |
|---|---|
| mIoU sémantique | Aucun gain direct : PERG-HGP ne prédit pas de classe |
| Clustering géométrique pur | Hypothèse crédible, non démontrée |
| Instances sous sémantique oracle | Plausible sur voitures denses ; risqué pour petites classes et longue portée |
| Panoptique avec sémantique prédite | Faible à modéré après complétion de l’API |
| 4D panoptique/LSTQ | Faible actuellement : backend 3D et tracking externe |

**Il n’est donc pas scientifiquement défendable d’annoncer aujourd’hui que \(K=10\) fera mieux.** Il est défendable d’annoncer que le nouveau backend rend enfin cette hypothèse testable à grande échelle.

## 9. Expérience minimale décisive

### 9.1 Préconditions

Avant tout score SemanticKITTI :

1. produire des appartenances de points avec statut `inner`, `outer` ou `ambiguous` ;
2. construire un merge tree avec masse et géométrie ;
3. définir une sélection plate unique pour toutes les méthodes ;
4. figer le réseau sémantique, les a priori et le tracker ;
5. épingler le commit de l’API d’évaluation au lieu de télécharger `master` à chaque exécution.

### 9.2 Ablation factorielle

| Variante | Effet isolé |
|---|---|
| DBSCAN | baseline de proximité |
| HDBSCAN | baseline hiérarchique de densité |
| HGP-old, \(K=3\) | référence historique |
| PowerCover, \(K=3,\kappa=1\) | effet du nouveau backend/grille |
| PowerCover, \(K=3,\kappa>1\) | effet propre de l’entropie |
| PowerCover, \(K=10,\kappa=1\) | effet propre de \(K\) |
| PowerCover, \(K=10,\kappa>1\) | proposition complète |

Grille minimale :

- \(K\in\{3,5,10\}\) ;
- \(\kappa\in\{1,2,4\}\), ou calibration locale ;
- au moins deux résolutions ayant des \(H\) physiquement interprétables ;
- mêmes coupes et même budget de calibration.

Mesures principales :

- \(S_{\rm assoc}\), PQ/PQ† des `things`, RQ ;
- splits, merges, faux négatifs, ID switches ;
- rappel par classe ;
- rappel par distance : 0–20, 20–40, 40–60, >60 m ;
- rappel par taille : <10, 10–49, 50–199, ≥200 points ;
- temps, VRAM, fraction de gardes conditionnelles acceptées ;
- \(s\), \(H\), \(\delta_{\rm num}\) et désaccord intérieur/extérieur.

Une passe sur 200 frames peut servir de screening. La conclusion doit ensuite être vérifiée sur toute la séquence 08, d’abord avec sémantique oracle, puis avec les prédictions figées d’un même backbone.

### 9.3 Critère go/no-go

La piste \(K=10\) serait convaincante si :

- \(K=10,\kappa>1\) bat à la fois \(K=10,\kappa=1\) et \(K=3,\kappa>1\) ;
- le gain atteint au moins 2 points absolus de \(S_{\rm assoc}\) ou de PQ-thing sur la séquence 08 complète ;
- le rappel des objets lointains et de moins de 50 points ne baisse pas de plus de 2–3 points ;
- le gain persiste avec une sémantique prédite ;
- moins de 1 % des points `thing` changent de composante entre les sorties intérieure et extérieure à la coupe choisie.

Un no-go est justifié si le meilleur \(K\) reste 3 ou 5, si les merges/FN augmentent plus que les splits ne diminuent, ou si la résolution cubique requise devient moins compétitive qu’un HDBSCAN/DBSCAN par frame.

## 10. Améliorations prioritaires seulement

### Priorité 0 — rendre la sortie utilisable

**Implémenter l’affectation canonique site–composante et le merge tree enrichi.**

À chaque seuil \(r\), l’intersection d’une boule de puissance avec un cube se teste par la distance du centre \(z_i\) à l’AABB du cube. Cette opération se parallélise et permet de distinguer appartenance certaine, impossible et ambiguë entre les enveloppes intérieure/extérieure. Le merge tree doit agréger masse, points, boîte, moments et ambiguïtés.

Sans cet étage, aucun benchmark de clustering ou SemanticKITTI n’est réellement possible.

### Priorité 1 — valider la promesse GPU

**Exécuter et versionner le protocole Blackwell complet**, au minimum à 100 k, 1 M, 3 M, 10 M et 30 M, avec :

- commit exact ;
- versions CUDA/RAPIDS/CuPy ;
- temps synchronisés ;
- pic NVML et RSS ;
- histogramme des candidats ;
- rapport d’exactitude ;
- artefacts et erreurs.

Ajouter une CI CPU permanente et un smoke test CUDA régulier. Tant que le notebook reste vide, il ne faut pas utiliser « 30 M validé ».

### Priorité 2 — fermer ou renommer la certification numérique

Deux options sont correctes :

1. utiliser un index spatial dont les bornes inférieures sont formellement garanties, avec arithmétique dirigée pour les comparaisons proches ;
2. conserver RBC et le float32, mais renommer systématiquement la sortie comme **conditionnellement enveloppée**.

La seconde option est acceptable pour un logiciel expérimental ; la première est nécessaire pour revendiquer une certification mathématique complète.

### Priorité 3 — adapter la discrétisation au LiDAR

**Utiliser une grille anisotrope, tuilée ou adaptative pilotée par un budget sur \(H\).** Une résolution fixe \(128^3\) ne doit pas être un défaut universel. Le run doit refuser une configuration où \(2(H+\delta_{\rm num})\) dépasse une tolérance physique ou la persistance minimale visée.

### Priorité 4 — régularisation locale

Le \(\kappa\) global peut être remplacé par le plus grand \(\kappa_i\) satisfaisant par exemple

\[
s_i\le\gamma R_K(x_i),
\]

avec retour à \(\kappa_i=1\) dans les zones rares. La preuve globale reste valable avec \(s=\max_i s_i\). Cette adaptation est plus pertinente pour SemanticKITTI qu’un \(\kappa\) universel et évite de lisser les petits objets comme les gros.

### Priorité 5 — éviter les recalculs massifs

Ajouter des checkpoints pour les sites régularisés et réutiliser les voisinages, ou des statistiques suffisantes, lors de la calibration de \(\kappa\). Ajouter un backend exact pour les petites tailles et séparer les dépendances optionnelles `atlas`, `cpu-power` et `cuda`, afin que le chemin massif n’impose pas inutilement tout PyTorch/scikit-learn.

### Ce qu’il ne faut pas prioriser

- optimiser davantage les 561 supports/coface de l’atlas pour le cas 30 M ;
- présenter `soft_only` comme une baseline de qualité ;
- ajouter des heuristiques de labels par cellule sans statut d’ambiguïté ;
- multiplier les benchmarks jouets avant d’avoir les appartenances de points et le protocole SemanticKITTI factoriel.

## 11. Verdict final

### Mathématiques

`PowerCover3D` est satisfaisant comme nouvel objet mathématique :

- régularisation entropique locale bien définie ;
- représentation exacte par sites de puissance ;
- entrelacement \(s\) avec le champ K-NN ;
- entrelacement cubique \(2(H+\delta_{\rm num})\) ;
- MSF correct pour \(H_0\).

Il ne faut cependant pas confondre « approximation stable du \(H_0\) K-NN » avec « HGP-Clusterer exact », ni « borne sur le rayon » avec « garantie de labels ».

### Implémentation

Le cœur CPU est bien implémenté et bien testé pour un prototype de recherche. Le design streaming et le MSF implicite rendent le cas 30 M crédible en mémoire. La preuve expérimentale CUDA, le débit réel, l’appartenance des points et le merge tree manquent encore. En l’état : **prototype avancé, pas logiciel de production**.

### SemanticKITTI

Pouvoir choisir \(K=10\) est scientifiquement intéressant. Cela peut améliorer la stabilité des instances suffisamment denses, mais le profil d’erreurs actuel et la variation de densité LiDAR rendent une dégradation tout aussi plausible. Le résultat historique \(\mathrm{LSTQ}=0{,}825\) est oracle-conditionné et ne constitue aucune preuve en faveur de \(K=10\).

La bonne prochaine étape n’est pas une nouvelle optimisation interne de l’atlas. C’est :

1. terminer la sortie observationnelle de `PowerCover3D` ;
2. exécuter le run GPU versionné ;
3. lancer l’ablation SemanticKITTI qui sépare proprement l’effet de \(K\), de \(\kappa\), de la grille et du downstream.

---

## 12. Suivi post-audit dans la révision de travail

Cette section décrit les corrections réalisées après l’audit du commit
`150c1ce`. Elle ne modifie pas rétroactivement le verdict historique et ne
constitue toujours pas un résultat Blackwell exécuté.

| Constat de l’audit | Correction implémentée | Validation CPU |
|---|---|---|
| `kappa` global et relances 30 M | mode `local_distortion` : maximisation d’entropie sous \(s_i\le\min(B_{\rm abs},\gamma R_K(x_i))\) en une passe | budgets nul/intermédiaire/uniforme, invariance et propriétés aléatoires |
| Échelle locale sans théorème | publication du ratio observé et du contrat \((1-2\gamma)r_K\le r_{K,\rm reg}\le(1+2\gamma)r_K\) lorsque `local_scale_k=K`, \(\gamma<1/2\) | comparaison aléatoire au champ K-NN exact |
| Solveur CuPy trop coûteux | RawKernel CUDA fusionné par ligne, 28 bissections, accumulation directe de \(z,a,s,\kappa_i\) | source compilable et contrat CPU ; exécution CUDA seulement dans le notebook |
| Top-K CPU limité par le garde 3D | relèvement exact \((z_i,\sqrt{a_i})\in\mathbb R^4\) avec cKDTree | égalité au brute force et cas de 1 100 sites ex æquo |
| Ex æquo rejetés au plafond CUDA | intervalle étroit sur la valeur du rang K lorsque les bornes ne se chevauchent qu’à la marge numérique | cas constant au-delà de l’ancien cap |
| Petits nuages CUDA incompatibles avec RBC | routeur explicite : RBC seulement s’il honore le support ou le cap complet plus la garde ; cuVS brute force sinon, sans réduction silencieuse | tests du plan brut/puissance aux frontières \(\lfloor\sqrt N\rfloor\) ; exécution cuVS réservée au notebook |
| Auto-voisin RBC potentiellement positif ou absent | canonisation de l’identifiant propre, ou d’un doublon de coordonnées exact, avec coût nul avant le rang local et Gibbs | faux index, doublons et rang \(R_K\) après insertion canonique |
| \(\varepsilon_X\) absent | mesure/enveloppe de quantification, rejet des entiers hors \([-2^{53},2^{53}]\), recalcul float64 de \(s^\uparrow\), violation du budget absolu incluant \(\varepsilon_X\) | contre-exemples grande translation, écart \(10^{-20}\) et dépassement stocké du cap absolu |
| Contrats de dtype incomplets | calcul normalisé float32, sorties physiques float64, rejet des échelles dont les poids carrés sous-débordent/débordent ou deviennent non finis | extrêmes float64, poids positif non représentable et promotion des forêts |
| Résidu de simplexe CUDA présenté comme mesuré | `simplex_residual_max=null` pour le kernel fusionné ; parité séparée avec l’oracle CPU | sérialisation JSON de `null` et cas budget nul/actif/uniforme |
| Une seule borne cubique | champs séparés `base_total_interleaving_radius` et `total_interleaving_radius` | tests de recomposition des deux formules |
| `grid_shape=128³` universel | grille uniforme anisotrope dérivée de `min_resolved_radius`, refus au-delà de `max_grid_cells`, contrôle de représentabilité des centres | formes anisotropes, axes dégénérés, budgets et résolution float32 |
| Aucune appartenance observationnelle | référence CPU CSR par intersection exacte boule de puissance–AABB, statuts `confirmed/possible/excluded` et appariement inner→outer | relation multivaluée et ambiguïtés de branche |
| Sauvegarde non rechargeable et artefacts mélangeables | schéma 2, `load()`, manifeste strict et `run_id` partagé par JSON/NPZ ; suppression d’un ancien fichier de sites non manifesté | round-trip avec/sans sites, mélange de générations, formes et compteurs incohérents |
| Dépendances atlas imposées | imports atlas paresseux ; cœur CPU limité à NumPy/SciPy ; extras `atlas` et `test` | import et suite complète |
| Notebook avec fallback global | protocole Blackwell local, smoke du kernel fusionné, \(N=30\,000\,000\), \(K=10\), deux bornes, fichiers d’échec et verdict `CONDITIONAL_PASS/INCONCLUSIVE/FAIL` | validation JSON et compilation de toutes les cellules |

La suite locale complète exécutée après ces changements donne **124 tests
`perg_hgp` réussis**, avec **1 test E-HGP réussi** séparément. Les 17
avertissements proviennent du test historique de partitionnement secondaire de
`dual_graph.py`. La compilation Python du package et de toutes les cellules du
notebook réussit.

Les validations de la colonne CPU prouvent les formules, le routage et les
contrats de sérialisation sans matériel NVIDIA. Elles ne constituent pas une
exécution de RBC, de cuVS ou du RawKernel : ces trois chemins restent soumis
aux smoke tests du notebook Blackwell.

### Portée actualisée

Le chemin CPU est désormais exact pour le top-K de puissance des sites stockés
et utilisable de quelques milliers de points jusqu’aux tailles compatibles avec
la RAM et la grille. Le chemin massif CUDA dispose d’un solveur local conçu pour
30 M, mais RBC et les enveloppes flottantes gardent un statut conditionnel.

La politique `min_resolved_radius` sépare bien l’échelle physique minimale de
la taille globale de la scène et refuse un budget impossible. Elle demeure une
grille uniforme anisotrope conservatrice, pas une grille locale par cellule.
Une vraie discrétisation adaptative devra utiliser \(H_Q,\delta_Q\), raffiner
les corridors de fusion et construire séparément les MSF inner/outer.

### Bloqueurs restant ouverts

- aucune sortie Blackwell ni mesure 30 M n’a été produite dans l’environnement
  CPU de cette révision ;
- le routeur cuVS exact pour les petites tailles est implémenté mais n’a pas
  été exécuté localement ; un index massif 4D certifiable reste à écrire ;
- l’appartenance boule–composante 30 M demande encore un kernel GPU deux passes ;
- le merge tree enrichi, les checkpoints/reprises et la sélection plate ne
  sont pas encore disponibles ;
- aucun nouveau score SemanticKITTI ne peut donc être revendiqué.
