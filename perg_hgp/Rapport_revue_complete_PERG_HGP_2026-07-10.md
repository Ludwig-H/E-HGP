# Revue complète de PERG-HGP pour 30 millions de points dans R^3, K=10

**Date :** 10 juillet 2026  
**Dépôt :** `Ludwig-H/E-HGP`  
**Base auditée initialement :** `fe8fa10`, puis correctifs `b84f883` (`main`)

**Mise en œuvre issue de la revue :** branche `agent/power-cover-3d-cuda`, détaillée en section 14

**Cible annoncée :** 30 000 000 de points dans R^3, K=10, GPU NVIDIA Blackwell avec au moins 69 Go décimaux de VRAM (seuil opérationnel : 65 Gio libres, soit environ 69,8 Go)

## 0. Verdict

Le modèle mathématique utile de PERG-HGP est valable : la régularisation entropique locale transforme chaque observation en un site de puissance `(z_i, a_i)`, et le dixième ordre statistique des distances de puissance définit une hiérarchie de couvertures dont les composantes sont une extension naturelle du HGP de la thèse.

En revanche, `PERGHGPClusterer.fit` n'est pas un moteur crédible pour 30 millions de points. Le problème n'est pas une fonction isolée à optimiser. Le pipeline actuel cumule une grille fixe sensible au skew, un très grand nombre de requêtes de voisinage, une génération d'atlas sans preuve de complétude, 561 supports testés par coface lorsque K=10, des scans Gabriel en Python, une déduplication de facettes qui finit par rematérialiser les résultats, et un Borůvka qui relit plusieurs fois les arêtes.

La recommandation est donc de conserver PERG-HGP comme prototype d'atlas et comme banc d'essai, mais de développer un backend distinct :

```text
power_cover_3d_cuda
  -> KNN exact en streaming
  -> sites entropiques (Z, a, s)
  -> oracle exact top-10 de puissance sur LBVH/octree
  -> champs cubiques intérieur et extérieur
  -> merge forests GPU en 26-connexité
  -> raffinement adaptatif et affectation certifiée
```

La garantie réaliste de ce backend est additive en **paramètre rayon** pour la filtration et son H0 :

\[
\text{erreur totale} \le s+2H,
\]

où `s` mesure la distorsion de régularisation et `H` est la plus grande demi-diagonale des cubes. Cette borne n'est ni une garantie de labels plats, ni une garantie additive sur la densité, ni une preuve de consistance statistique à K fixé.

Les corrections appliquées pendant cette revue réparent des erreurs réelles du prototype. Elles ne constituent pas encore le nouveau backend et ne valident pas la cible 30 M.

**Mise à jour après implémentation, le 10 juillet 2026.** Un backend séparé
`power_cover_3d_cuda` existe maintenant. Il implémente une référence CPU complète,
la régularisation entropique en streaming, un garde global du top-K de puissance,
les champs cubiques en rayon et un MSF implicite 26-connexe CPU/CuPy. Le notebook
`PERG_HGP_Blackwell_30M.ipynb` contient le protocole Blackwell jusqu'à 30 M. Tous les
tests CPU passent. En revanche, aucun GPU NVIDIA n'est présent dans l'environnement
de développement : le kernel CuPy, le débit RAPIDS et le run 30 M restent donc
**à valider par l'exécution du notebook**, sans résultat GPU inventé.

Le chemin implémenté remplace le LBVH proposé par un Random Ball Cover 3D RAPIDS,
soumis à un audit échantillonné obligatoire contre le brute force cuVS, puis accepte
la puissance sous l'enveloppe float32 déclarée à l'aide d'une distance euclidienne de
garde et de `min(a)`. La marge flottante est exposée séparément ; elle n'est pas une
preuve par intervalles dirigés. La borne conditionnelle publiée par le code est donc

\[
\boxed{s+2(H+\delta_{\rm num})},
\]

et se réduit à `s+2H` en arithmétique exacte. Elle reste une borne en rayon et H0,
pas une garantie de labels plats.

## 1. Périmètre et méthode

La revue couvre :

- la lecture intégrale des deux premiers chapitres du manuscrit ;
- les définitions et théorèmes HGP, Čech, Gabriel et K-MST utilisés par le code dans les chapitres ultérieurs ;
- les packages actifs et historiques `perg_hgp`, `E-HGP` et `HGP-old` ;
- les spécifications, rapports, scripts de benchmark et notebooks du dépôt ;
- l'historique Git de `7464288` à `fe8fa10` ;
- des cas adversariaux, des comparaisons brute force et une comparaison avec le backend Cython historique.

Cartographie du dépôt :

| Élément | Rôle réel |
|---|---|
| `HGP-old/` | implémentation historique, CGAL et condensation Cython de référence |
| `E-HGP/` | première implémentation de régularisation entropique et atlas |
| `perg_hgp/` | package Python actif, champ de rang et atlas Rank-Gabriel |
| `PERG-HGP/` | spécifications et documents, pas le package importé |
| `Manuscrit_de_these_LouisHauseux.pdf` | contrat HGP non régularisé et résultats géométriques |

Limites expérimentales de cette revue :

- l'environnement local ne contient pas de GPU NVIDIA ;
- les deux notebooks Colab de benchmark ne contiennent ni cellules exécutées ni résultats enregistrés ;
- aucune mesure indépendante à 1 M, 10 M ou 30 M n'est donc disponible dans le dépôt ;
- les estimations GPU ci-dessous doivent être validées sur la machine cible avec synchronisation CUDA et mesure NVML.

## 2. Contrat mathématique

### 2.1 Convention HGP

Pour K=10, les sommets du graphe HGP sont des sous-ensembles de **cardinalité 10**, donc des simplexes de dimension 9. Les cofaces qui créent les arêtes ont cardinalité 11, donc dimension 10. L'expression « 10-face » est ambiguë et doit être évitée dans le code et les rapports.

Pour le nuage `X={x_i}`, le rayon K-NN classique est

\[
r_K(y)=K\text{-ième plus petite valeur de }\{\|y-x_i\|\}_{i=1}^n,
\]

et le niveau de forte densité paramétré par le rayon est

\[
L_K(r)=\{y:r_K(y)\le r\}
      =\{y:\#\{i:\|y-x_i\|\le r\}\ge K\}.
\]

La hiérarchie de Hartigan pertinente ici est la hiérarchie des composantes connexes de ces niveaux lorsque `r` varie.

### 2.2 Régularisation entropique canonique

Il faut distinguer trois paramètres :

- `K=10`, ordre de la couverture et de HGP ;
- `kappa`, taille effective imposée par l'entropie ;
- `m_reg`, taille du support local dans lequel le problème entropique est résolu.

Pour un support `A_i` contenant `i`, la définition canonique recommandée est

\[
q_i^\kappa\in\arg\min_{q\in\Delta(A_i)}
\sum_{j\in A_i}q_j\|x_i-x_j\|^2
\quad\text{sous}\quad H(q)\ge\log\kappa.
\]

Le coût quadratique brut est préférable au coût décalé de type UMAP : il donne la limite propre `kappa=1`, soit `q_i=delta_i`, `z_i=x_i` et `a_i=0`. Le choix actuel `K_rho=max(32,3K)` n'a pas de justification géométrique ; `kappa` doit être calibré par un budget de distorsion et par la stabilité des branches visées.

On définit

\[
\nu_i^\kappa=\sum_jq_{ij}^\kappa\delta_{x_j},\qquad
z_i=\sum_jq_{ij}^\kappa x_j,\qquad
a_i=\sum_jq_{ij}^\kappa\|x_j-z_i\|^2.
\]

L'identité fondamentale est

\[
d_i^\kappa(y)^2
=W_2^2(\delta_y,\nu_i^\kappa)
=\sum_jq_{ij}^\kappa\|y-x_j\|^2
=\|y-z_i\|^2+a_i.
\]

Le champ régularisé est alors

\[
r_{K,\kappa}(y)=K\text{-ième plus petite valeur de }
\{d_i^\kappa(y)\}_{i=1}^n,
\]

\[
L_{K,\kappa}(r)=
\{y:r_{K,\kappa}(y)\le r\}
=\{y:\#\{i:d_i^\kappa(y)\le r\}\ge K\}.
\]

La version discrète régularisée doit être définie explicitement : un indice appartient à un amas si son site de puissance couvre au moins un point de la composante continue considérée. Ce n'est pas littéralement la définition discrète euclidienne de la thèse.

### 2.3 Garantie de régularisation

Posons

\[
s_i=W_2(\nu_i^\kappa,\delta_{x_i})
=\sqrt{\|x_i-z_i\|^2+a_i},\qquad s=\max_i s_i.
\]

Par l'inégalité triangulaire de W2,

\[
|d_i^\kappa(y)-\|y-x_i\||\le s_i.
\]

Le K-ième ordre statistique est 1-lipschitz pour la norme uniforme, donc

\[
|r_{K,\kappa}(y)-r_K(y)|\le s
\]

et

\[
L_K(r-s)\subseteq L_{K,\kappa}(r)\subseteq L_K(r+s).
\]

Il s'agit d'un `s`-entrelacement en rayon, donc d'une garantie sur la filtration et sa persistance H0. Il ne faut pas le convertir sans hypothèse en une erreur additive sur une densité `lambda`, ni en une garantie sur une partition finale.

### 2.4 Portée des théorèmes de la thèse

Le théorème de correspondance entre composantes de la K-couverture et composantes du graphe d'intersection repose sur la convexité des intersections de boules. Il s'étend directement aux boules de puissance additives, car les ensembles

\[
\{y:\|y-z_i\|^2+a_i\le r^2\}
\]

sont des boules euclidiennes éventuellement vides, et leurs intersections sont convexes.

En revanche, les résultats Gabriel, Delaunay d'ordre supérieur et K-MST de la thèse sont non pondérés, utilisent une hypothèse de position générale et, pour certains, ne couvrent que les composantes non triviales. Ils ne prouvent pas qu'un atlas Rank-Gabriel pondéré et produit par un nombre fini de témoins est complet.

Une coface dont le centre de miniball a exactement les mêmes K+1 sites dans son top global ne contient aucun intrus strict : c'est un bon certificat géométrique en arithmétique exacte. Le code l'évalue en float32/float64 avec tolérances, sans arithmétique d'intervalles. Le statut honnête est donc « cohérence top globale à la tolérance annoncée », pas « preuve exacte ».

### 2.5 K=10 fixé

K=10 peut être un choix métier parfaitement pertinent pour une résolution locale finie. Il ne donne toutefois pas un estimateur de densité asymptotiquement consistant lorsque `n` tend vers l'infini : les résultats de consistance des arbres de clusters demandent typiquement un nombre de voisins qui croît avec `n`. Cette limite doit figurer dans les publications et comparaisons, sans empêcher l'usage opérationnel à résolution fixée. Voir par exemple [Chaudhuri et al., cluster tree estimation](https://arxiv.org/abs/1406.1546).

## 3. Correction et preuve de la hiérarchie cubique

Soit

\[
g(y)=r_{10,\kappa}(y).
\]

Chaque `d_i^kappa` est 1-lipschitz, et un ordre statistique conserve cette propriété. Pour un cube fermé `Q`, de centre `c_Q` et de demi-diagonale `h_Q`, on a donc

\[
g(c_Q)-h_Q\le g(y)\le g(c_Q)+h_Q
\quad\text{pour tout }y\in Q.
\]

Définissons

\[
A_r^- = \bigcup_{g(c_Q)+h_Q\le r}Q,
\qquad
A_r^+ = \bigcup_{g(c_Q)-h_Q\le r}Q.
\]

Avec `H=max_Q h_Q`, les inclusions complètes sont

\[
A_r^-\subseteq L_{10,\kappa}(r)\subseteq A_r^+
\subseteq L_{10,\kappa}(r+2H),
\]

et, symétriquement,

\[
L_{10,\kappa}(r)\subseteq A_{r+2H}^-.
\]

La filtration cubique intérieure et la filtration continue sont donc `2H`-entrelacées. Par composition avec la régularisation, la borne est `s+2H` par rapport au champ 10-NN original.

Corrections importantes par rapport au rapport précédent :

1. Pour une union de cubes **fermés**, une connexion peut se faire par une face, une arête ou un sommet. Il faut une 26-connexité en 3D, pas seulement les 6 voisins par faces.
2. L'inclusion `L_r subset A^-_{r+2H}` est nécessaire pour énoncer l'entrelacement dans les deux directions.
3. La boîte englobante axis-alignée des `z_i` suffit pour H0. La projection coordonnée sur cette boîte ne peut augmenter aucune distance `||y-z_i||`, et le segment de projection reste dans le même sous-niveau. Elle fournit une rétraction sans padding arbitraire.
4. Une branche de persistance supérieure à `2(s+2H)` ne peut pas être appariée à la diagonale par cette seule erreur. Cela ne fournit pas une correspondance point par point des labels.
5. Le merge forest cubique ne donne pas automatiquement les labels des 30 M observations. L'affectation et le traitement des cellules ambiguës sont un étage distinct.
6. La borne de nœud `dist(y,AABB)^2 + min(a)` est correcte mais peut être peu discriminante si les poids `a_i` sont très dispersés. Une preuve de correction n'est pas une preuve de performance.

## 4. État du code au début de la revue

### 4.1 Régressions de l'historique récent

| Série | Intention | Défaut constaté avant correction |
|---|---|---|
| PR 2, `5516aa9` | top-K de puissance et Gibbs | fallback global matérialisant `Q x N x 3`, certificat KNN avec slack positif |
| PR 3, `e63b45a` | annealing progressif | échelles absolues, ambiguïté et décalage du rang final |
| PR 4, `f1717c1` | miniball float64 | tolérance absolue, ridge non remis à l'échelle, fallback silencieux |
| PR 5, `76349e1` | condensation HDBSCAN | masses ignorées, membres sous seuil perdus, tailles nulles, égalités séquentielles |
| PR 6, `09091af` | checkpoints atomiques | pas de manifeste données/config/version, reprise de rang ambiguë |
| PR 7-8, `88a1604` à `fe8fa10` | compilation et vectorisation | compilation implicite extrêmement lente sur CPU, aucune preuve de profilage GPU versionnée |

Avant correctif, la suite ordinaire n'avait exécuté que 6 tests en 965,81 secondes avant interruption, essentiellement à cause de `torch.compile` et de la traversée de grilles clairsemées. Cela invalidait l'usage de la suite comme boucle de développement.

### 4.2 Audit par module après correctifs

| Module | État utile | Limite restante |
|---|---|---|
| `config.py` | paramètres centralisés | `beam_per_bucket`, `L_shell` et `support_cap` restent sans effet réel ; défaut arbitraire de `K_rho` |
| `grid.py` | certificats conservatifs, KNN de puissance exact pour les valeurs float32 stockées, fallback doublement bloqué | grille fixe et padding par occupation ; parcours de coquilles Python ; pas de best-first LBVH |
| `local_gibbs.py` | coût brut canonique, invariance d'échelle, bornage de `kappa`, cas limites et ex aequo | aucun résidu d'entropie agrégé dans le rapport final ; pas de kernel streaming dédié |
| `rank_field.py` | DP scalaire/batch cohérente sur les tests | DP directe, clipping et renormalisation modifient le champ en régime mal conditionné ; queue globale non certifiée |
| `witnesses.py` | annealing complet et énergies normalisées | générateur heuristique, pas de dédoublonnage robuste, critères de capacité incohérents entre lifting et raffinement |
| `cofaces.py` | miniball float64, tolérance relative, fallback actif/brute explicite | 561 supports par coface à K=10 ; pas de bornes d'intervalle ; tolérance numérique seulement |
| `gabriel.py` | test AABB correct dans son principe | boucles Python `coface x cellules actives`, tolérances absolues dans ce module |
| `dual_graph.py` | étoile implicite correcte pour H0, génération d'arêtes par chunks | IDs globaux en RAM, résultats rechargés sur device, skew seulement partiellement borné |
| `hierarchy.py` | condensation pondérée, multiway, stabilité et EOM réparées | Python ; naissances géométriques exactes des facettes absentes ; sélection plate postérieure |
| `estimator.py` | rang final, manifestes, paramètres fitted, budgets précoces et statuts améliorés | orchestrateur monolithique ; atlas incomplet ; plusieurs allocations tardives ; aucun backend 30 M |

### 4.3 Benchmarks et preuves expérimentales

Les artefacts présents ne valident pas le passage à l'échelle :

| Artefact | Ce qu'il mesure | Limite |
|---|---|---|
| `benchmark_perg_hgp.py` | PERG-HGP CPU contre HDBSCAN sur deux jeux de 10 000 points | aucun manifeste matériel, aucune RAM/VRAM, une seule graine, aucun résultat versionné |
| `perg_hgp_benchmark_tool.py` | temps, ARI, `ru_maxrss` et mémoire PyTorch | pas de `torch.cuda.synchronize`, RSS cumulatif du processus donc comparaison HDBSCAN biaisée après PERG, mémoire réservée non mesurée |
| `benchmark_hypermassive.py` | intention de test E-HGP à 10 M | ne teste pas PERG-HGP et ne fournit aucune sortie enregistrée |
| `E-HGP/benchmark_results.md` | petits jeux de 500 à 1 000 points | pas de logs bruts, matériel, répétitions ou intervalles ; l'ARI ne prouve pas la préservation de la hiérarchie |
| notebooks Colab | scripts d'installation et d'exécution | 0 cellule exécutée et 0 sortie enregistrée dans les deux notebooks principaux |

Le notebook PERG contient 6 cellules de code et le notebook E-HGP 9 ; toutes ont `execution_count=null`. Les scripts « large » et « hypermassive » sont donc des scénarios proposés, pas des résultats scientifiques reproductibles.

Un benchmark recevable doit séparer temps de compilation et temps d'exécution, synchroniser CUDA, mesurer `max_memory_reserved` et NVML, repartir d'un processus neuf pour chaque méthode, conserver les logs et publier le rapport d'exactitude avec le manifeste matériel.

### 4.4 Garanties actuelles, formulation correcte

Pour le chemin `atlas_exact`, le code peut dire :

> Les cofaces acceptées passent une cohérence top-(K+1) globale sur les sites float32 stockés, avec miniball réévalué en float64 et tolérance relative. Le MSF préserve exactement la connectivité du graphe edge-induced pour les poids d'arêtes float32 effectivement fournis. La complétude vis-à-vis du HGP de puissance, les naissances exactes des facettes et la sélection plate ne sont pas certifiées.

Il ne peut pas dire :

- que le HGP complet est exact ;
- que `cut_certified` est implémenté ;
- que l'arithmétique flottante constitue une preuve exacte ;
- que l'atlas contient toutes les cofaces séparantes ;
- que les labels EOM sont garantis par l'entrelacement.

Le mode `soft_only` construit un sous-graphe 1-sortant des sites, puis son MSF. C'est une heuristique d'ordre 1, pas un HGP exact.

## 5. Pourquoi l'orchestrateur actuel ne passe pas à 30 M

### 5.1 Mémoire

Ordres de grandeur pour float32 et indices int64 :

| Objet | Taille approximative |
|---|---:|
| `X`, 30 M x 3 float32 | 343 Mio, soit 360 Mo décimaux |
| `Z`, 30 M x 3 float32 | 343 Mio |
| `a`, 30 M float32 | 114 Mio |
| `s_i` ou `eta_i`, 30 M float32 chacun | 114 Mio |
| voisins 30 M x 128 int64 | 28,61 Gio |
| distances 30 M x 128 float32 | 14,31 Gio |
| voisinage global seul | 42,92 Gio |
| ancien fallback `Q x N x 3` | jusqu'à plusieurs téraoctets |

Les données permanentes minimales `X,Z,a,s` tiennent facilement. Le danger vient des structures globales `N x m`, des buffers de tri, des cofaces/facettes, des copies CPU/GPU et des allocations dont la taille dépend de l'occupation maximale d'une cellule.

Le chemin out-of-core des facettes garde notamment un tableau global d'IDs, puis recharge les facettes uniques et les IDs sur le device. Il réduit certaines pointes mais n'est pas borné par un budget mémoire strict.

### 5.2 Combinatoire et temps

Pour une coface de cardinalité 11, l'énumération des supports jusqu'à quatre sites coûte

\[
\binom{11}{1}+\binom{11}{2}+\binom{11}{3}+\binom{11}{4}=561
\]

systèmes. Deux millions de cofaces impliquent 1,122 milliard de résolutions de supports.

Autres coûts :

- 2 M de cofaces à K=10 produisent environ 20 M d'arêtes dans l'expansion étoilée ;
- Borůvka peut effectuer plusieurs dizaines de rescans complets des chunks ;
- le Gabriel global parcourt les cellules actives pour chaque coface ;
- les champs de rang effectuent plusieurs températures, itérations et requêtes de puissance pour chaque témoin ;
- une cellule très chargée fait gonfler les tenseurs au maximum d'occupation du batch ;
- le code central ne contient aucun kernel CUDA/Triton spécialisé pour ces opérations.

Une allocation nominale qui tient dans 80 Gio ne suffit pas. La fragmentation, les buffers de radix sort, les copies de chunks, la distribution spatiale et les échecs de certificat déterminent le pic réel.

### 5.3 Matériel cible ambigu

Le nom « G4 » n'est pas synonyme de « GPU 80 Go ». Google Cloud documente actuellement les machines G4 autour de GPU RTX PRO 6000 Blackwell de 96 Go, alors qu'une carte de 80 Go évoque plutôt un A100 dans une famille A2. Voir la [documentation GPU Google Cloud](https://docs.cloud.google.com/compute/docs/gpus).

Par ailleurs, Colab Pro ne garantit pas un modèle de GPU ni une quantité fixe de ressources ; la disponibilité varie dans le temps. Voir la [FAQ officielle Colab](https://research.google.com/colaboratory/intl/en-GB/faq.html).

Chaque benchmark doit donc enregistrer au minimum :

```text
nvidia-smi
nom, UUID et mémoire du GPU
driver et version CUDA
CPU et RAM hôte
versions PyTorch, CUB/cuVS et compilateur
stockage local, espace libre et débit
```

Les performances float64 doivent être mesurées : elles sont importantes pour les audits de miniball et diffèrent fortement entre RTX et A100.

## 6. Architecture recommandée : `power_cover_3d_cuda`

### 6.1 Séparation des backends

```text
perg_hgp/
  core/
    entropy.py
    power_sites.py
    distortion.py
    hierarchy_contract.py
  backends/
    power_cover_3d_cuda/
    atlas_conservative/
    exact_small/
  postprocess/
    point_assignment.py
    flat_selection.py
```

Contrats publics recommandés :

```yaml
mode: spatial_enveloped       # R3, borne conditionnelle au contrat numérique déclaré
mode: atlas_conservative      # connectivité edge-induced exacte, complétude inconnue
mode: graph_conditional       # exact seulement pour un graphe candidat déclaré
mode: exact_small             # oracle exhaustif
```

Rank-Gabriel reste utile pour explorer la géométrie et produire des cofaces explicites. Il ne doit plus être le chemin critique de la cible 30 M.

### 6.2 Index spatial

1. Recentrer et mettre à l'échelle les coordonnées en conservant la transformation.
2. Trier par clé Morton 64 bits avec CUB radix sort.
3. Construire un LBVH ou octree linéaire, avec feuilles de 32 à 64 points.
4. Stocker par nœud son AABB, sa plage Morton, son effectif et ensuite `min(a)`.
5. Subdiviser les feuilles surchargées et traiter explicitement doublons, nuages plans/linéaires et points identiques.

Une grille uniforme seule ne fournit pas une borne d'occupation et ne doit pas être le moteur principal.

### 6.3 Régularisation en streaming

Pour des blocs initiaux de l'ordre de `2^18` requêtes, à ajuster par profil :

1. calculer les `m_reg` voisins exacts par parcours LBVH/octree ;
2. résoudre Gibbs avec coûts normalisés ;
3. calculer immédiatement `z_i`, `a_i`, `s_i` et le résidu entropique ;
4. écrire `Z,a,s` et libérer les voisins du bloc.

Il ne faut jamais conserver le graphe global `N x m_reg`. Avec `m_reg=64`, un chunk de 262 144 points demande environ 128 Mio pour des indices int64 et 64 Mio pour les distances, avant temporaires. Des indices uint32 divisent le premier coût par deux puisque 30 M tient dans 32 bits non signés.

Le choix de `kappa` suit un budget : tester plusieurs valeurs, mesurer `s=max(s_i)` et retenir la régularisation la plus forte compatible avec la persistance minimale recherchée. `kappa` et K ne doivent pas être liés par une règle fixe.

### 6.4 Oracle exact top-10 de puissance

Pour chaque nœud spatial `C`, conserver

\[
LB_C(y)=\operatorname{dist}(y,\operatorname{AABB}(C))^2+min_{i\in C}a_i.
\]

La requête maintient un registre des dix plus petites valeurs et visite les nœuds best-first. Elle s'arrête seulement lorsque la meilleure borne restante est supérieure à la dixième valeur courante.

Approche numérique :

- parcours et distances courantes en float32 ;
- recalcul float64 des candidats et nœuds proches du seuil ;
- bornes arrondies vers l'extérieur ou intervalles sur les comparaisons finales ;
- statut `certified`, `uncertain` ou `failed`, jamais un fallback approximatif silencieux ;
- métriques : nœuds visités, fraction float64, gap de certification et débit.

La variation des poids peut dégrader la borne `min(a)`. On peut stocker plusieurs sous-bornes par enfant, mais toute optimisation doit préserver la monotonie et être testée contre un scan exhaustif.

### 6.5 Évaluation du champ cubique

Une base `256^3` représente 16 777 216 cubes. Ordres de grandeur :

| Tableau par cube | Taille |
|---|---:|
| valeur float32 | 64 Mio |
| parent Union-Find int32 | 64 Mio |
| ordre/ID int32 | 64 Mio |
| deux bornes float32 | 128 Mio |

Ces tableaux sont modestes sur 80 Go, contrairement à l'évaluation de 16,8 M requêtes top-10, qui doit être le vrai objet du benchmark.

Pour chaque cube, calculer

```text
lower_Q = g(c_Q) - h_Q
upper_Q = g(c_Q) + h_Q
```

puis construire deux forêts :

- extérieure, activation à `lower_Q` ;
- intérieure, activation à `upper_Q`.

Implémentation :

- radix sort GPU des événements ;
- Union-Find sur les 26 voisins, avec seulement 13 voisins antérieurs visités lorsque l'ordre le permet ;
- aucune matérialisation du graphe cubique complet ;
- événements égaux traités comme un batch multiway ;
- sortie déterministe à permutation des enfants près.

### 6.6 Raffinement adaptatif

`512^3` coûte huit fois `256^3`. `1024^3` dépasse un milliard de cubes et ne doit pas être une grille globale.

Raffiner seulement :

- les cubes responsables d'un désaccord entre les forêts intérieure et extérieure ;
- les voisinages des événements de fusion dont la persistance est scientifiquement pertinente ;
- les régions où `2h_Q` domine le budget de régularisation `s` ;
- les points dont l'affectation est ambiguë à la coupe demandée.

Pour conserver une borne globale simple, imposer une demi-diagonale maximale aux feuilles finales. Une structure adaptative demande aussi un traitement correct des incidences entre cubes de tailles différentes.

### 6.7 Affectation des observations

Les sorties primaires doivent être les deux merge forests et leurs intervalles de
fusion. Pour obtenir des labels canoniques à une coupe demandée, il faut encore :

1. construire pour chaque site la boule de puissance
   `B_i(r)={y: ||y-z_i||^2+a_i<=r^2}` ;
2. tester son intersection avec les unions de cubes des composantes intérieure et
   extérieure (un test boule/boîte fournit le prédicat élémentaire exact) ;
3. n'affecter le site que si les intersections désignent une composante unique et si
   les deux encadrements concordent ;
4. sinon, raffiner localement ou retourner un masque d'incertitude ;
5. ne traiter la localisation de `x_i` ou `z_i` dans sa cellule, ou un vote, que comme
   un post-traitement heuristique explicitement déclaré.

Cette affectation par intersection n'est pas implémentée dans le backend livré en
section 14.

## 7. Garanties à exposer

| Propriété | Statut possible |
|---|---|
| support `m_reg`-NN | exact/certifié pour chaque bloc |
| Gibbs local | résidu de simplex et d'entropie mesuré |
| régularisation vs 10-NN | `s`-entrelacement en rayon |
| top-10 de puissance | certifié sous l'enveloppe float32 déclarée, ou incertain |
| grille vs champ régularisé | `2H`-entrelacement |
| grille vs 10-NN original | `s+2H` en rayon et H0 |
| connectivité du graphe edge-induced généré | exacte combinatoirement pour ses poids fournis |
| complétude de l'atlas | inconnue sans preuve de couverture/coupe |
| labels plats | post-traitement, non garanti par l'entrelacement seul |
| K=10 statistique | résolution finie-échantillon |
| calcul flottant | certificat conditionnel à l'enveloppe déclarée ; exact formel seulement avec bornes dirigées/intervalles validés |

Schéma machine-lisible proposé :

```yaml
neighbor_support: exact | certified | approximate
entropy_residual_max:
regularization_interleaving_radius:   # s
power_knn_certified_fraction:
power_knn_uncertain_count:
miniball_primal_residual_max:
miniball_dual_residual_max:
spatial_interleaving_radius:          # 2H
total_interleaving_radius:            # s + 2H
inner_outer_merge_disagreement_count:
edge_induced_connectivity_exact_for_float32_edge_weights:
atlas_completeness: complete | cut_certified | unknown
full_power_hgp_complete:
facet_births_exact:
flat_selection: eom | persistence | heuristic | none
hardware_manifest:
```

## 8. Plan d'implémentation

### Phase 0 : verrouiller les oracles

- top-K euclidien et de puissance contre brute force ;
- Gibbs contre résolution indépendante et changements d'unité de `1e-12` à `1e12` ;
- miniball contre énumération exhaustive/SLSQP, y compris doublons et poids extrêmes ;
- condensation contre `HGP-old` ;
- tests de permutation et d'événements égaux ;
- oracle complet de HGP de puissance pour petits `n`.

### Phase 1 : sites entropiques streaming

- Morton/LBVH ;
- KNN certifié ;
- kernel Gibbs ;
- stockage `Z,a,s` ;
- rapport des résidus et quantiles de `s_i`.

### Phase 2 : top-10 de puissance

- `AABB + min(a)` ;
- parcours best-first ;
- audit float64 et bornes dirigées ;
- tests de translations, poids extrêmes, égalités et cellules denses.

### Phase 3 : H0 cubique

- grilles `64^3`, `128^3`, `256^3` ;
- Union-Find 26-connexe ;
- événements multiway ;
- comparaison bit à bit ou structurelle avec une référence CPU.

### Phase 4 : raffinement et affectation

- forêts intérieure/extérieure ;
- politique de raffinement ;
- intervalles de branches ;
- affectation des points et masque d'incertitude.

### Phase 5 : validation 30 M

- profilage complet par étage ;
- optimisation des kernels dominants seulement après profil ;
- trois répétitions sur le matériel déclaré ;
- artefacts et logs publiés avec le notebook.

Il s'agit d'un développement CUDA/CUB substantiel, pas d'une refactorisation de quelques jours de l'orchestrateur Python.

## 9. Matrice d'acceptation

### 9.1 Données

- uniforme 3D ;
- mélanges de densités et amas presque en contact ;
- LiDAR/surfaces minces ;
- grille régulière avec ex aequo ;
- doublons massifs ;
- une cellule extrêmement occupée ;
- nuages plans, linéaires et constants ;
- outliers ;
- poids `a_i` avec forte dynamique.

### 9.2 Échelles

```text
100 k, 1 M, 3 M, 10 M, 30 M points
64^3, 128^3, 256^3 cubes
au moins 3 graines pour les scénarios aléatoires
```

### 9.3 Correction

- rappel KNN de 100 % pour les requêtes déclarées certifiées ;
- top-10 de puissance identique à brute force sur les tailles accessibles ;
- aucune requête incertaine transformée silencieusement en résultat exact ;
- invariance aux tailles de chunks ;
- résidu d'entropie relatif enregistré ;
- résidus primal et dual du miniball enregistrés ;
- hiérarchie invariant à l'ordre des événements égaux ;
- forêts GPU identiques à la référence CPU ;
- inclusions intérieure/extérieure vérifiées sur cas synthétiques.

### 9.4 Ressources et temps

- VRAM et RAM sous 85 % des ressources déclarées ;
- aucun tableau global `N x m_reg` ;
- aucun tenseur `Q x N` ;
- aucune liste explicite des arêtes de la grille cubique ;
- stockage scratch local, pas Google Drive FUSE, avec au moins 30 Gio libres ;
- temps cible provisoire inférieur à 10 heures pour 30 M, à confirmer par mesure et non par extrapolation.

Mesures obligatoires :

```text
temps synchronisé CUDA par étage
points/s et requêtes/s
pic VRAM NVML
pic RSS hôte
volume et débit I/O
nœuds LBVH visités par requête
fraction des audits float64
distribution des gaps de certification
quantiles de s_i, h_Q et largeur des intervalles de fusion
```

Critère scientifique d'arrêt : si `s+2H` est du même ordre que la persistance des branches étudiées, le calcul ne permet pas de conclure à cette résolution. Il faut diminuer `kappa`, raffiner ou publier l'indétermination.

## 10. Place de l'atlas Rank-Gabriel

L'atlas reste utile pour :

- visualiser des cofaces explicites ;
- tester des conjectures Gabriel pondérées ;
- comparer avec l'oracle exact sur petites tailles ;
- fournir un sous-complexe conservatif en haute dimension, hors cible de ce rapport.

Pour une famille de cofaces déjà validées avec leur vrai rayon, Gabriel n'est pas nécessaire pour éviter les fausses connexions : chaque coface est un simplexe valide de Čech de puissance. Un atlas incomplet ne peut qu'omettre ou retarder des fusions.

La connectivité filtrée du graphe edge-induced de l'atlas peut être construite sans développer les `K*C` arêtes : trier les cofaces par rayon, puis unionner directement leurs K+1 IDs de facettes. C'est l'expansion étoilée implicite, avec événements égaux traités en multiway. Cela ne restaure ni les vraies naissances des facettes, ni la complétude de l'atlas, ni le coût de génération des cofaces.

La filtration de Čech d'ordre supérieur parcimonieuse de Buchet, Dornelas et Kerber fournit une piste théorique d'approximation pour dimension de doublement, K et epsilon fixés, mais ses constantes et son extension aux poids entropiques ne donnent pas aujourd'hui un moteur de production K=10. Voir [Sparse Higher Order Čech Filtrations](https://drops.dagstuhl.de/entities/document/10.4230/LIPIcs.SoCG.2023.20).

## 11. Correctifs appliqués pendant la revue

Les changements locaux apportés au package actif sont :

1. `local_gibbs.py`
   - coût quadratique brut par défaut ;
   - normalisation par ligne et invariance aux unités ;
   - validation de `1 <= kappa <= m` ;
   - cas `kappa=1`, `kappa=m` et minima ex aequo.
2. `grid.py`
   - correction du nuage constant ;
   - suppression du slack positif non conservatif du certificat KNN ;
   - fallback exact doublement bloqué pour la puissance ;
   - scan bloqué des petites bases afin d'éviter les coquilles vides pathologiques.
3. `rank_field.py` et `witnesses.py`
   - `torch.compile` uniquement sur activation explicite, CUDA et Triton ;
   - énergies de rang normalisées et tolérances de capacité relatives.
4. `cofaces.py`
   - calculs de miniball en float64 ;
   - tolérances relatives aux unités de puissance ;
   - choix du plus petit support faisable ;
   - fallback explicite si l'active-set stagne, duplique un indice ou échoue ;
   - rayons couvrants et conversion float32 arrondis vers l'extérieur.
5. `hierarchy.py`
   - conservation des membres issus de composantes sous seuil ;
   - utilisation réelle des masses `W_nodes` ;
   - fusions à poids égal en un événement multiway ;
   - stabilité et sélection EOM ;
   - vote compatible avec les appartenances initiales.
6. `estimator.py`
   - paramètres fitted séparés des paramètres constructeur sklearn ;
   - manifeste de checkpoint versionné, hash des données, de la configuration et du code algorithmique ;
   - montée et sauvegarde explicites des rangs jusqu'à K+1 ;
   - refus de `cut_certified`, non implémenté ;
   - calcul et exposition de `s` ;
   - score de la thèse `S_tau=sum_{sigma superset tau} rho(sigma)^(-expZ)` ;
   - limites mémoire vérifiées plus tôt ;
   - rapport d'exactitude limité à l'atlas généré et à la précision numérique stockée.
7. `tests/test_perg_hgp.py`
   - cas adversarial du certificat KNN ;
   - top-K de puissance contre brute force ;
   - Gibbs sur douze ordres de grandeur ;
   - miniball à très petite échelle ;
   - non-mutation des paramètres ;
   - mismatch de checkpoint ;
   - composantes sous seuil et fusions multiway.

Validation locale après correctifs :

- 27 tests `perg_hgp` passent ;
- `compileall` passe ;
- `git diff --check` passe ;
- 3 900 arbres aléatoires donnent les mêmes enfants, appartenances, rayons, masses et stabilités que le backend Cython historique, à l'ordre des enfants près ;
- le chemin spatial du top-K de puissance est testé avec 5 000 sites, au-delà du fallback des petites bases ;
- 120 miniballs aléatoires, traduits et remis à des échelles de `1e-9` à `1e9`, concordent avec un programme convexe SLSQP indépendant à une erreur relative maximale de `2,1e-11`.

Ces tests sont des régressions de correction. Ils ne remplacent pas les benchmarks CUDA de la section 9.

## 12. Priorités restantes dans le prototype

Même si le backend spatial devient la priorité, les défauts suivants doivent rester visibles dans `perg_hgp` :

1. calculer la vraie naissance géométrique de chaque facette de cardinalité K ;
2. séparer le merge forest brut de la condensation EOM et des labels par vote ;
3. rendre le miniball active-set réellement batché, sans 561 énumérations à K=10 ;
4. remplacer les tolérances Gabriel absolues restantes par des audits relatifs/ternaires ;
5. borner l'out-of-core en octets, y compris les IDs et les sorties finales ;
6. remplacer Borůvka à rescans par un tri unique et une Union-Find adaptée à l'expansion implicite ;
7. documenter la sémantique mathématique exacte du rang final et la borne de queue du champ soft ;
8. supprimer ou implémenter les paramètres publics actuellement inactifs ;
9. ajouter un oracle pondéré exhaustif qui compare la hiérarchie à tous les seuils ;
10. ne réintroduire des kernels compilés qu'avec un benchmark et un cache contrôlé.

## 13. Conclusion

La bonne cible scientifique est une hiérarchie H0 du dixième champ de distance de puissance, avec deux sources d'erreur séparées et mesurées : `s` pour la régularisation, `2H` pour la discrétisation spatiale.

La bonne cible logicielle n'est pas une optimisation incrémentale de `PERGHGPClusterer.fit`. C'est un backend CUDA 3D dédié, fondé sur Morton/LBVH, des voisins streaming, un top-10 de puissance certifié et deux merge forests cubiques 26-connexes.

PERG-HGP Rank-Gabriel doit rester un atlas expérimental : la connectivité de son graphe edge-induced est exacte pour les poids numériques effectivement produits, mais ses naissances de facettes et sa complétude restent inconnues pour le HGP complet. Cette séparation rend les garanties plus fortes, l'API plus honnête et la cible 30 M techniquement testable.

## 14. Mise en œuvre des recommandations après la revue

Cette section est le journal de réalisation demandé. Elle distingue ce qui a été
implémenté et testé sur CPU de ce qui attend encore une mesure GPU.

### 14.1 Lecture scientifique effectuée avant modification

Les Parties I et II du manuscrit ont été lues intégralement, pages PDF 35 à 134.
Les invariants suivants ont guidé le code :

1. le champ est paramétré par le rayon, avec la convention d'intersection à `2r` ;
2. le K-ième voisin compte le point lui-même lorsque la requête est une observation ;
3. la cible primaire est la hiérarchie des composantes continues des niveaux K-NN ;
4. pour K supérieur ou égal à 2, les amas discrets naturels forment un recouvrement,
   non nécessairement une partition ;
5. pour K=10, les faces HGP ont cardinalité 10 et les cofaces cardinalité 11 ;
6. les égalités sont des événements multiway, jamais une suite binaire dépendant de
   l'ordre ;
7. la preuve du théorème de correspondance s'étend aux boules de puissance par
   convexité, contrairement aux raccourcis Gabriel/Delaunay pondérés ;
8. une hiérarchie H0 correcte peut être calculée directement sur le champ continu
   approché, sans énumérer les faces de cardinalité 10.

Les synthèses de lecture ont aussi relevé les ambiguïtés du manuscrit sur les boules
ouvertes/fermées, le facteur deux, la position générale, les ex aequo, les paramètres
de HDBSCAN et la différence entre rayon, densité et score EOM. Les nouveaux tests
couvrent ces pièges lorsqu'ils affectent le backend.

### 14.2 Nouvelle architecture livrée

Le chemin massif est séparé du prototype Rank-Gabriel :

```text
perg_hgp/backends/power_cover_3d_cuda/
  contracts.py       configuration, rapports et budget mémoire
  spatial_core.py    Gibbs, streaming, top-K puissance et champ cubique
  cuda_runtime.py    RAPIDS RBC, audit cuVS et manifeste matériel
  cubical_msf.py      MSF implicite CPU et CuPy/Borůvka
  api.py              PowerCover3D et PowerCoverHierarchy
```

Le backend public est `PowerCover3D`. `PERGHGPClusterer` conserve sa sémantique
d'atlas historique ; aucune revendication de complétude ne lui a été ajoutée.

#### Normalisation

Les données sont recentrées et mises à l'échelle isotropiquement avant les prédicats
GPU. Les rayons, sites, poids et bornes exposés sont reconvertis dans les unités
d'entrée. Les axes constants deviennent des grilles de taille un, ce qui couvre les
nuages plans, linéaires et constants sans dupliquer artificiellement les cellules.
Le recentrage est effectué dans le dtype d'entrée avant le cast float32 ; le test
`1e9 + {0, 0.25, 0.5, 0.75}` empêche la perte silencieuse de ces écarts. Les centres
absolus rendus par l'API sont donc dénormalisés en float64.

#### K-NN euclidien de régularisation

La référence CPU utilise `scipy.spatial.cKDTree`. Le chemin CUDA utilise
`cuml.neighbors.NearestNeighbors(algorithm="rbc")`, disponible pour l'euclidien 3D.
Le notebook compare obligatoirement un échantillon de listes, jusqu'à `m_reg`, avec
le brute force exact de cuVS. Un échec arrête un run strict. Cet audit empirique
détecte les régressions mais ne prouve pas les requêtes non échantillonnées.

Références d'API : [cuML NearestNeighbors](https://docs.rapids.ai/api/cuml/stable/api/generated/cuml.neighbors.nearestneighbors/),
[cuVS brute force](https://docs.rapids.ai/api/cuvs/stable/python_api/neighbors_brute_force/).

Les voisins sont demandés par chunks. Le code calcule immédiatement `Z`, `a`, `s_i`,
le résidu de simplexe et le résidu d'entropie, puis libère indices, distances,
probabilités et coordonnées voisines du chunk. Aucun tableau global
`N x m_reg` n'est conservé. Le maximum `s` reste exact ; les quantiles diagnostiques
sont calculés sur au plus un million d'indices déterministes afin d'éviter un tri ou
une copie globale de 30 millions de valeurs.

#### Régularisation canonique

Le solveur traite explicitement :

- `kappa=1`, qui donne une masse de Dirac et retrouve `z_i=x_i`, `a_i=s_i=0` ;
- `kappa=m_reg`, qui donne la loi uniforme ;
- les minima ex aequo, pour lesquels la loi uniforme sur les minimiseurs est une
  solution optimale admissible ;
- les changements d'unités, grâce à une normalisation par ligne des coûts.

La revue finale a ajouté un contre-exemple de quasi-ex aequo
`[0, 1e-20, 1]` pour `kappa=1.5`. La première version utilisait un plancher de
température et violait la contrainte d'entropie. Le solveur définitif distingue
seulement les égalités représentées, travaille en log inverse-température à partir du
plus petit gap strict et refuse un résultat dont les résidus d'entropie ou de simplexe
dépassent la tolérance.

`pilot_kappa_calibration` teste plusieurs valeurs de `kappa` sur un échantillon et
choisit la plus forte compatible avec un budget pilote. Le `s=max(s_i)` du run complet
est toujours recalculé ; le backend s'arrête si ce `s` dépasse le budget final. Le pilote
n'est donc jamais présenté comme une borne globale.

#### Top-K de puissance global

Le backend ne suppose pas que les plus proches voisins de puissance sont les K plus
proches centres euclidiens. Il demande `C+1` voisins selon le contrat euclidien du
backend, évalue la puissance des C premiers et utilise le dernier comme garde. Pour
tout site omis, en arithmétique exacte,

\[
\|y-z_i\|^2+a_i
\ge d_{C+1}(y)^2+\min_j a_j.
\]

Si le K-ième candidat, augmenté de son enveloppe numérique, est sous cette borne
diminuée de la même enveloppe, la requête est acceptée sous ce contrat float32. Sinon
`C` double de 64 jusqu'à 1 024 par défaut. Arrivé au plafond : exception en mode
strict, ou statut `certified=False` explicite. Il n'existe aucun fallback ANN
silencieux.

Le wrapper refuse aussi explicitement `C+1 > floor(sqrt(N))`, seuil auquel cuML peut
substituer brute force à RBC. Les petites campagnes plafonnent donc `C` en fonction
de `N`, tandis que la cible 30 M autorise largement le plafond 1 025. L'audit du
second index porte sur de vrais centres de la grille, et pas seulement sur des sites
d'entraînement.

#### Marge numérique

Les points normalisés sont stockés en float32. L'enveloppe de comparaison utilise un
multiple explicite de l'epsilon machine. Elle est convertie en une erreur de rayon
`delta_num` et incorporée aux deux filtrations :

```text
outer_Q = g_hat(c_Q) - H - delta_num
inner_Q = g_hat(c_Q) + H + delta_num
```

Cette décision évite d'appliquer `+/-H` à l'énergie carrée, erreur qui invaliderait la
preuve. Le rapport JSON utilise le mode `spatial_enveloped` et expose
`numerical_radius_error` séparément. Une future
arithmétique d'intervalles dirigée pourrait réduire cette marge ; le code actuel ne
prétend pas à une preuve en nombres réels au-delà de l'enveloppe déclarée.

#### Champ cubique et 26-connexité

Le champ est évalué par chunks sur les centres des cubes de la boîte englobante des
sites. Les tableaux globaux sont seulement les valeurs et statuts par cube. Les cubes
sont fermés : la connexité est 26-connexe en 3D, 8-connexe sur un nuage plan et
2-connexe sur une ligne.

#### MSF implicite

La référence CPU visite les voisins implicitement et produit un MSF déterministe avec
la clé totale `(poids, edge_id)`. Le chemin CuPy compile des `RawKernel` Borůvka :

1. scan des 13 directions canoniques de chaque cellule ;
2. meilleure arête sortante par composante avec `atomicMin` sur une clé 64 bits ;
3. union déterministe par racine minimale avec `atomicCAS` ;
4. compression et répétition jusqu'à `N-1` arêtes ;
5. tri du seul résultat compact.

La liste des arêtes cubiques n'est jamais matérialisée. Sur `256^3`, le modèle compte
16 777 216 sommets et 216 338 940 arêtes implicites. Le résultat compact fait environ
256 Mio, le pic device propre au MSF environ 448 Mio, et environ 3,22 Gio de liste
d'arêtes explicite sont évités.

Sur une grille uniforme, chacun des champs intérieur et extérieur est une translation
du champ de base de `+(H+delta_num)` et `-(H+delta_num)` respectivement ; ils diffèrent
donc entre eux de `2(H+delta_num)`. Ils ont la même topologie MSF. Une seule exécution
de Borůvka est effectuée, puis les deux forêts sont obtenues par translation de leurs
naissances et fusions.

### 14.3 Oracles de Phase 0 ajoutés

`perg_hgp/reference/power_oracle.py` fournit une frontière indépendante pour petites
tailles :

- top-K de puissance bloqué, stable par `(valeur, id)` ;
- min-ball additive float64 par énumération exhaustive des supports de cardinalité au
  plus quatre, avec résidus primal, dual, actif, stationnarité et simplexe ;
- toutes les facettes de cardinalité K et toutes les cofaces de cardinalité K+1 ;
- aucun filtre Gabriel ;
- vraies naissances en rayon ;
- composantes incluant les facettes isolées ;
- événements égaux atomiques ;
- comparaison K=1 avec le Single-Linkage au rayon distance/2.

L'ancien `reference/oracle.py` est conservé pour compatibilité historique, mais le
nouvel oracle est celui à utiliser pour la correction du power-HGP.

### 14.4 API, packaging et checkpoints

Le wheel utilisait `packages=["perg_hgp"]` et excluait déjà `reference/`. La découverte
inclut maintenant `perg_hgp*`, donc les oracles et backends sont empaquetés.

Le shim d'import racine chargeait les classes une seconde fois sous
`perg_hgp.inner`. Il utilise maintenant le chemin canonique `perg_hgp.*` ;
`perg_hgp.WitnessPool is perg_hgp.witnesses.WitnessPool` est vrai.

Le hash de checkpoint de l'atlas parcourt récursivement les fichiers Python, CUDA,
C++ et en-têtes du package. Une modification du nouveau backend ne peut donc plus
laisser un manifeste de code inchangé.

Le wheel `perg_hgp-0.2.0` contient notamment :

```text
perg_hgp/reference/power_oracle.py
perg_hgp/backends/power_cover_3d_cuda/api.py
perg_hgp/backends/power_cover_3d_cuda/cubical_msf.py
```

### 14.5 Rapport machine-lisible et sorties

`PowerCoverHierarchy` contient les sites, le champ, le MSF de base, les forêts
intérieure/extérieure, les intervalles de fusion, les temps par étage, le budget mémoire
et `AccuracyReport`. `save()` publie atomiquement un NPZ versionné (MSF, masque de
certification et translations intérieure/extérieure), éventuellement les sites, et un
JSON strict sans `NaN`/`Infinity`, avec :

- support voisin et audits RBC ;
- résidus d'entropie ;
- `s`, `H`, `delta_num` et la borne totale ;
- fraction de top-K certifiés et compte incertain ;
- manifeste matériel ;
- limites sur K fixe, H0 et labels.

Les configurations rejettent désormais booléens, entiers tronqués et valeurs non
finies. Si le mode exploratoire non strict conserve au moins une requête incertaine,
le rapport passe à `spatial_uncertain`, met les bornes spatiale et totale à `null` et
`fusion_intervals()` refuse de présenter les fusions comme un encadrement.

La sortie plate est volontairement absente. `components_at_cut()` donne les
composantes de cubes intérieure/extérieure, mais localiser simplement `x_i` ou `z_i`
dans un cube ne satisfait pas la définition canonique

\[
\exists y\in C:\ \|y-z_i\|^2+a_i\le r^2.
\]

L'affectation certifiée des observations et le raffinement adaptatif non uniforme
restent donc des étapes futures, et non des heuristiques cachées.

### 14.6 Tests CPU exécutés

Commandes finales de validation :

```text
cd perg_hgp && python -m pytest -q
cd E-HGP && python -m pytest -q
python -m compileall -q perg_hgp/perg_hgp
git diff --check
python -m pip wheel perg_hgp --no-deps
```

Résultats observés avant publication :

```text
perg_hgp : 92 passed, 17 warnings historiques, 10,96 s
E-HGP    : 1 passed, 8,18 s
wheel    : sous-packages reference/ et backends/ présents
```

Les 92 tests comprennent :

- les 27 régressions historiques de l'atlas ;
- 18 tests du MSF cubique ;
- 31 tests du cœur spatial, incluant les quasi-ex aequo, grands offsets float64 et
  artefacts JSON/NPZ versionnés ;
- 8 tests du contrat CUDA/RBC sans GPU ;
- 8 tests de l'oracle power-HGP exhaustif.

Les audits déterministes intégrés aux tests comparent aussi les composantes du MSF
cubique à celles du graphe brut à de nombreux seuils, ainsi que le Borůvka du graphe
dual historique à son oracle Kruskal. Les tests couvrent égalités, valeurs négatives,
dimensions dégénérées, doublons, translations, homothéties et changements de chunks.

### 14.7 Contrôle CPU de bout en bout

Un contrôle reproductible a été exécuté avec NumPy `default_rng(20260710)`. Les
observations sont tirées par
`labels=rng.integers(0, 4, N)` puis
`X=mu[labels]+rng.normal(0, 0.35, (N, 3)).astype(float32)`, où
`mu=[(-2,0,0),(2,0,0),(0,2,1),(0,-2,-1)]` :

```text
N=10 000, dimension=3, K=10, kappa=4, m_reg=64
grille=32^3, chunks régularisation/puissance=2 048, candidats puissance=64 -> 512
graine=20260710
```

Résultat :

```text
mode : spatial_enveloped
requêtes acceptées sous l'enveloppe déclarée : 100 %
requêtes incertaines : 0
cubes : 32 768
arêtes MSF : 32 767
régularisation : 0,738 s
index des sites : 0,002 s
champ de puissance : 0,522 s
MSF cubique : 1,032 s
total : 2,298 s
RSS maximal du processus : environ 585 Mio
s = 0,33615 ; H = 0,15500 ; delta_num = 0,00254
borne totale = 0,65122 dans les unités synthétiques
```

Ce contrôle prouve le fonctionnement CPU à cette taille. Il ne constitue ni une
extrapolation de temps à 30 M, ni une mesure Blackwell.

### 14.8 Budget analytique pour 30 M

Pour `N=30 000 000`, `m_reg=64`, chunk de régularisation 262 144, chunk de
puissance 65 536, plafond 1 024 candidats et une grille `128^3` ou `256^3`, le
modèle révisé après revue des durées de vie compte :

```text
tableaux persistants + provision index : 3,02 Gio
chunk de régularisation connu : 0,95 Gio
chunk puissance au pire plafond : 1,26 Gio
pic analytique conservatif des tableaux modélisés : 4,27 Gio
```

Le kernel fusionné lit directement `sites[indices]` et calcule les puissances sans
matérialiser `candidats`, `différences`, `différences^2` ni les poids rassemblés en
`Q x C x 3`. Une taille de chunk distincte évite aussi le pic de 14,5 Gio identifié
pendant la revue du premier brouillon. Le chiffre reste volontairement séparé d'une
mesure : les buffers internes RAPIDS/CUB, l'espace de travail de partition, RMM,
NVRTC et le driver ne sont pas entièrement modélisés. Le notebook vérifie le budget
avec la VRAM réellement libre, impose une limite de 85 %, partage l'allocateur RMM
entre CuPy et cuML et mesure le pic NVML.

### 14.9 Notebook Blackwell livré

`PERG_HGP_Blackwell_30M.ipynb` :

- installe RAPIDS par le [script Colab officiel](https://docs.rapids.ai/deployment/stable/platforms/colab/) ;
- clone la branche et installe le package en mode éditable ;
- refuse un GPU non Blackwell ou moins de 65 Gio (environ 69,8 Go décimaux) de VRAM
  libre pour le full run ;
- exécute toute la suite CPU ;
- audite RBC contre cuVS brute force sur les points bruts et des centres de grille ;
- audite séparément l'index RBC utilisé par le pilote de `kappa` avant toute
  sélection ;
- smoke-compile le kernel CuPy et compare GPU/CPU sur cas aléatoires et adversariaux ;
- audite RBC sur des cas uniformes, skew, plans, lignes, constantes, doublons et
  poids dynamiques ; le MSF est testé séparément sur champs négatifs, ex aequo,
  motifs structurés et champ constant ;
- permet les échelles 100 k, 1 M, 3 M, 10 M et 30 M ;
- synchronise CUDA, mesure NVML, RSS, disque et temps par étage ; le `PASS` impose
  VRAM et RAM hôte sous 85 %, ainsi qu'au moins 30 Gio de scratch restant après
  sauvegarde ;
- conserve les artefacts sur scratch local avant archivage ;
- conclut par `PASS`, `INCONCLUSIVE` ou `FAIL` selon les certificats, ressources et,
  pour le jeu synthétique, l'existence d'au moins une persistance dépassant deux fois
  la borne. Ce dernier test ne valide pas à lui seul plusieurs branches scientifiques.

Le notebook versionné ne contient aucune sortie et aucun `execution_count`. Il est un
protocole à exécuter, pas un résultat GPU.

### 14.10 Écarts assumés et travail restant

| Recommandation | Statut | Commentaire |
|---|---|---|
| Oracles Phase 0 | Fait CPU | power top-K, min-ball, complexe complet, H0 cubique |
| Sites entropiques streaming | Fait CPU/CUDA API | GPU à mesurer |
| Morton/LBVH exact | Remplacé | RBC 3D RAPIDS + audit cuVS ; fallback brute force implicite refusé |
| Top-10 puissance certifié | Conditionnel | garde euclidienne + `min(a)`, escalade ; certificat sous l'enveloppe float32 déclarée |
| Bornes numériques | Enveloppe explicite | `delta_num` mesuré ; dérivation dirigée formelle future |
| H0 cubique 26-connexe | Fait CPU/CuPy | kernel GPU non exécuté localement |
| Pas de liste d'arêtes cubiques | Fait | Borůvka implicite 13 directions |
| Forêts intérieure/extérieure | Fait | une topologie, deux translations sur grille uniforme |
| Raffinement adaptatif non uniforme | Non fait | grille 128^3/256^3 configurable ; arrêt si borne insuffisante |
| Affectation canonique des 30 M sites | Non fait | aucune étiquette heuristique présentée comme certifiée |
| Validation 30 M Blackwell | Notebook prêt | attente d'exécution et de logs utilisateur |
| Trois répétitions full 30 M | Paramétrable | une par défaut pour respecter la durée Colab |

Le backend est donc **implémenté et prêt à être testé** pour la cible. Ses grands
tableaux explicites sont bornés par chunks, mais les allocations internes RAPIDS/CuPy
doivent encore être mesurées. Le terme « validé à 30 M » ne devra être utilisé qu'après
un notebook terminé avec `uncertain=0`, MSF GPU conforme à la référence, ressources
sous 85 % et une étude de `s+2(H+delta_num)` branche par branche sur les données
scientifiques visées.
