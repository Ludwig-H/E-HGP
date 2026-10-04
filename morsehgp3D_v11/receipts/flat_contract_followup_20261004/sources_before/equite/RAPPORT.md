# Équité de la comparaison avec HDBSCAN : tête N-aire, ex æquo, convention de niveau, lignes à publier

4 octobre 2026, rôle « équité » du workflow `v11-points-select`. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
GCP non utilisé. Aucune commande git qui écrit, aucune construction native, aucun réseau. Tout ce qui suit tourne
sur de **petits nuages** (n ≤ 300, et n ≤ 30 pour la tour) : ce sont des oracles de correction, jamais des
mesures à l'échelle ni des pentes. Chaque chiffre renvoie à un script et à sa sortie gardés dans ce dossier (§ 10).
La v10 est lue comme source de données et de contre-exemples, jamais comme autorité.

## 0. Résumé

**Question.** Comment comparer la sortie plate tirée de la tour à `sklearn.cluster.HDBSCAN` sans biais, en séparant
l'effet de la hiérarchie de l'effet de la sélection ?

**Décisions proposées.**

| # | Décision | Raison (section) |
| --- | --- | --- |
| D1 | Une seule tête de référence, **N-aire**, appliquée aux deux hiérarchies : condensation à mcs sur les plateaux atomisés, stabilité avec $\lambda=\ell^{-z}$, EOM, feuilles, ε à règle cohérente | elle coïncide avec sklearn sur tout arbre binaire sans ex æquo (th. A) et ne dépend que de l'ultramétrique (§ 3) |
| D2 | Contrôle d'implantation : le **code de sklearn** (`tree_to_labels`) appelé sur la **binarisation canonique** (gros enfants d'abord) donne la même sortie (th. B) : aucune réimplémentation de l'adversaire n'est nécessaire pour appliquer « sa » sélection à la tour | § 3.3 |
| D3 | L'adversaire reste `HDBSCAN` tel quel (ligne R0), `min_samples = k`, `algorithm='kd_tree'` épinglé, publié avec son **étendue sur 5 permutations de l'entrée**, même machine, même session G4 | ses étiquettes dépendent de l'ordre d'entrée et du tri (§ 3.5) |
| D4 | Chaque arbre dans **son unité native** (rayon pour la tour, distance d'atteignabilité mutuelle pour HDBSCAN) ; aucun facteur 2 appliqué ; ε exprimé en milieu de niveaux propres | un facteur global n'a aucun effet sur EOM ni feuilles (prop. H) ; `alpha` n'est pas un facteur global (prop. C) |
| D5 | Lignes R0 à R7 et **décomposition** ex æquo / z / hiérarchie à tête égale / plafond / regret de sélection ; z = 1 toujours publié | § 6 |
| D6 | **Porte d'étalonnage à k = 1** : R3 ≡ R1 et R4 ≡ R2 exactement, sinon le harnais est faux | prop. C (ii), 288/288 contrôlés |
| D7 | Oracles d'antichaîne **exacts** : m05 (programme dynamique), PQ (sac à dos sur l'arbre), meilleur IoU sans seuil (programme linéaire en nombres entiers) | contrôlés contre l'énumération, 120/120 |
| D8 | Grilles identiques pour les deux têtes N-aires ; dev et test disjoints ; LiDAR : séquences d'entraînement en dev, 08 (démos comprises) en test une seule fois | § 5 |
| D9 | Complétion, critère d'existence, `alpha`, `min_samples = k + 1` : tout bras donné à un côté l'est à l'autre sous sa forme analogue, ou l'asymétrie est déclarée | § 5 |

**Prouvé** (§ 3, avec contrôles § 4) : sémantique exacte de sklearn 1.9.1, identique en 1.7.2 (version des sessions
G4) ; lemme D (la diagonale des entrées est sans effet pour mcs ≥ 2) ; théorèmes A et B ; proposition G (à
`min_samples` ≥ 3 les ex æquo ont une probabilité positive, à `min_samples` ≤ 2 nulle sur données continues, et tout
k sur données quantifiées) ; proposition T (ce que la binarisation change, cinq mécanismes, fixtures exactes) ;
proposition H (facteur global sans effet) ; proposition C (correspondance des niveaux) ; proposition E (la stabilité
de HDBSCAN n'estime pas l'excès de masse de la définition 19 de la thèse, sous idéalisation déclarée) ; exactitude
des trois oracles.

**Constaté au passage** : sur le codespace (numpy 2.5.3), `HDBSCAN(cluster_selection_epsilon > 0)` lève un
`TypeError` dès qu'ε déclenche une remontée (1 672 configurations sur 4 352) ; sklearn donne à `alpha` un sens
différent selon `algorithm` (facteur global en `brute`, autre filtration en `kd_tree`).

**Conjectural** : l'ampleur des effets d'ex æquo aux tailles 8 000 à 32 000 ; le coût du programme linéaire à
l'échelle ; les prédictions P1–P7 (§ 7).

## 1. Ce que fait sklearn (lecture du code installé)

**Versions.** Codespace : scikit-learn 1.9.1, numpy 2.5.3, scipy 1.18.1, Python 3.12.1, AMD EPYC 7763, numpy
`X86_V3` (AVX2, sans AVX-512). Sessions G4 de la v11 : scikit-learn 1.7.2, numpy 2.2.6, Python 3.10 (copie partielle
dans `build/v10-persist/revue3_distant/home_partial/`). Le diff des cinq fichiers du module `_hdbscan` entre 1.7.2 et
1.9.1 ne touche qu'aux imports, aux docstrings, à l'avertissement sur `copy` et à une compréhension d'ensemble :
**l'algorithme est identique**. numpy ne l'est pas (tri, conversions).

**Chaîne** (`sklearn/cluster/_hdbscan/`, numéros de ligne de 1.9.1) :

1. *Distance de cœur.* Voie `kd_tree` (choisie par `algorithm='auto'` en euclidien dense) : `NearestNeighbors(...)
   .kneighbors(X, min_samples)[:, -1]` (`hdbscan.py` l. 345-356) ; le point lui-même est rendu à distance 0, donc
   $\mathrm{core}(x)$ est la distance au $(\mathrm{min\_samples}-1)$-ième **autre** point, c'est-à-dire $r_K(x)$ de la
   définition 16 de la thèse avec $K=\mathrm{min\_samples}$. Voie `brute` : `np.partition` de la matrice à diagonale
   nulle (`_reachability.pyx` l. 131-135), même quantité.
2. *Atteignabilité mutuelle.* $d_{mr}(a,b)=\max(\mathrm{core}(a),\mathrm{core}(b),d(a,b)/\alpha)$ dans
   `mst_from_data_matrix` (`_linkage.pyx` l. 189-196). En voie `brute`, `distance_matrix /= alpha` précède le calcul
   des cœurs (`hdbscan.py` l. 254 puis 264) : `alpha` y divise **aussi** les cœurs, ce n'est qu'un facteur global.
3. *Arbre couvrant.* Prim dense en $O(n^{2})$ : un point ajouté à la fois ; à égalité, le plus petit indice
   d'entrée gagne (comparaisons strictes, l. 200-217).
4. *Lien simple.* `_process_mst` trie les arêtes par `np.argsort` (tri par défaut, **non stable**) puis
   `make_single_linkage` fusionne deux à deux dans cet ordre (`hdbscan.py` l. 165-168). Un plateau d'égalités exactes
   devient une **chaîne binaire** de nœuds de même valeur.
5. *Condensation* (`_tree.pyx` l. 122-237), parcours en largeur depuis la racine, $\lambda=1/\mathrm{valeur}$
   ($+\infty$ à 0) : deux enfants gros (taille ≥ mcs) → deux nouveaux clusters nés à λ ; deux petits → tous leurs
   points sortent du cluster courant à λ, qui s'arrête ; un gros → il **continue** le cluster, les points du petit
   sortent à λ.
6. *Stabilité* (l. 240-278) : $S(C)=\sum(\lambda_{\mathrm{ligne}}-\lambda_{\mathrm{naissance}}(C))\times\mathrm{taille}$
   sur les lignes de parent $C$ (points sortis, taille 1 ; clusters enfants, à leur λ de naissance) ;
   $\lambda_{\mathrm{naissance}}(\mathrm{racine})=0$. De façon équivalente $S(C)=\sum_{x\in C}(\lambda_x^{C}-\lambda_b(C))$
   où $\lambda_x^{C}$ est le λ auquel $x$ quitte $C$.
7. *Sélection* (`_get_clusters`, l. 643-799). EOM : clusters par identifiant décroissant (enfants avant parents),
   racine exclue sauf `allow_single_cluster` ; les enfants l'emportent seulement si leur somme est **strictement**
   supérieure (égalité : le parent), `max_cluster_size` force la descente. Feuilles : feuilles de l'arbre des
   clusters ; aucune scission → tout est bruit. ε (`epsilon_search`, `traverse_upwards`, l. 578-640) : un cluster
   retenu né à une distance **< ε** est remplacé par son premier ancêtre né à une distance **> ε**, ou par l'enfant de
   la racine sur le chemin (la racine si `allow_single_cluster`).
8. *Étiquettes* (`_do_labelling`, l. 433-512) : union-find sur les lignes dont l'enfant n'est pas retenu ; un point
   reçoit le plus bas cluster retenu sur la chaîne de son cluster de sortie. Un point rattaché à la racine est bruit,
   sauf si la racine est le seul cluster retenu (`allow_single_cluster`) : il est alors étiqueté si son λ de sortie
   dépasse le plus grand λ des lignes de la racine (ou $1/\varepsilon$).

**Ce que sklearn calcule, comme définition.** Sur son arbre binaire $T$ : un cluster est une chaîne maximale de nœuds
$v_0,v_1,\dots,v_m$ où $v_{i+1}$ est l'unique enfant gros de $v_i$, $v_0$ étant la racine ou un enfant d'une scission
à deux gros ; son ensemble de points est celui de $v_0$ ; chaque point le quitte au λ du nœud de la chaîne où son côté
(petit) est lâché, ou à la mort de la chaîne ; la stabilité est la somme ci-dessus ; l'EOM retient le cluster dont la
stabilité est au moins la meilleure somme de ses descendants. C'est la définition de Campello et al. **appliquée à un
arbre binaire**, donc dépendante de la binarisation dès qu'il y a des ex æquo (§ 3.5).

**Deux défauts d'environnement, vérifiés ici.**

- numpy 2.5 rend toute conversion d'un tableau de taille 1 en scalaire fatale ; `traverse_upwards` (l. 588) en fait
  une : `HDBSCAN(cluster_selection_epsilon=ε)` échoue (`TypeError`) dès qu'ε provoque une remontée. Sur le
  codespace : **1 672 configurations sur 4 352** à ε > 0 ([equivalence_out.json](equivalence_out.json)). Sous numpy
  2.2.6 (environnement G4), la même conversion n'émet qu'un `DeprecationWarning`
  ([numpy_versions.sh](numpy_versions.sh), [numpy_versions_out.txt](numpy_versions_out.txt)). Une session qui
  mettrait numpy à jour perdrait ε sans changer de version de sklearn. Dans les deux versions, `argsort` par défaut
  n'est pas stable sur un tableau à ex æquo (même script).
- `alpha` : en `brute`, `alpha = 2` rend les mêmes étiquettes que `alpha = 1` (24/24) ; en `kd_tree`, jamais celles de
  `brute` à `alpha = 2` (0/24) ([levels_out.txt](levels_out.txt), L4b). La v10 l'avait noté (EVAL_v2, D20).

## 2. Définitions N-aires (toute hiérarchie ultramétrique)

**Hiérarchie à diagonale.** $X$ fini, $u:X\times X\to[0,\infty)$ symétrique, $u(i,j)\geq\max(u(i,i),u(j,j))$,
$u(i,l)\leq\max(u(i,j),u(j,l))$. La diagonale $e_i=u(i,i)$ est la date d'entrée. Au niveau $r$ (coupe fermée), les
**blocs** sont les classes de $X_r=\lbrace i: e_i\leq r\rbrace$ pour la relation $u\leq r$. Cas de la tour :
$H^{r}_{k+1}$, $u(i,j)$ = rayon de rencontre des lignées, $e_i$ = date d'entrée. Cas de HDBSCAN : $u$ = distance
cophénétique (minimax) de l'arbre couvrant de $d_{mr}$, $e_i=\mathrm{core}(i)$ chez Campello, 0 dans la
représentation de sklearn.

**Dendrogramme atomisé** $D(u)$ : feuilles = points au niveau 0 ; nœuds internes = blocs d'au moins deux points à leur
niveau de naissance, chacun ayant pour enfants les blocs ou singletons qu'il réunit, **tous strictement plus bas** ; un
plateau (plusieurs composantes réunies au même niveau exact) est **un** nœud à au moins trois enfants, jamais une
chaîne. Depuis sklearn : `from_linkage` fusionne toute chaîne de nœuds binaires de même valeur flottante. Depuis la
tour : `from_ultrametric` sur les valeurs exactes (`RValue` de la v11, sommes de radicaux, comparaison certifiée).

**Tête N-aire** ([nary_head.py](nary_head.py)), pour mcs ≥ 2 et $\varphi$ strictement décroissante
($\varphi(\ell)=\ell^{-z}$, $\varphi(0)=+\infty$, $\lambda_b(\mathrm{racine})=0$) :

- *condensation* : au nœud $v$ (niveau $\ell$) du cluster $c$, soit $B$ l'ensemble de ses enfants de taille ≥ mcs ; les
  points des autres enfants sortent de $c$ à $\ell$ ; si $|B|\geq 2$, $c$ meurt à $\ell$ et chaque élément de $B$ devient
  un cluster né à $\ell$ ; si $|B|=1$, l'enfant gros continue $c$ ; si $|B|=0$, $c$ meurt à $\ell$ ;
- *stabilité* : $S(c)=\sum_{(x,\ell_x)}(\varphi(\ell_x)-\varphi(b_c))+\sum_{d}|d|(\varphi(\mathrm{mort}_c)-\varphi(b_c))$, sur
  les sorties de $c$ et ses clusters enfants $d$ ;
- *EOM* : de bas en haut, $c$ retenu si $S(c)\geq\sum_d\tilde{S}(d)$ (égalité : le parent), sinon
  $\tilde{S}(c)=\sum_d\tilde{S}(d)$ ; racine exclue sauf `allow_single_cluster` ;
- *feuilles* : clusters sans enfant, racine exclue ; aucune scission → tout est bruit ;
- *ε, règle cohérente* : un cluster retenu de naissance $b<\varepsilon$ est remplacé par son plus bas ancêtre strict
  non racine de naissance $b\geq\varepsilon$, à défaut par l'enfant de la racine sur le chemin (la racine si
  `allow_single_cluster`). L'option `eps_mode='sklearn'` reproduit l'asymétrie de sklearn ($<$ puis $>$) ;
- *étiquettes* : le plus bas cluster retenu sur la chaîne du cluster de sortie du point ; règle de sklearn pour la
  racine (bruit, sauf cluster unique et seuil).

**Binarisation canonique** $\beta(D)$ (`binarize`) : à chaque nœud, chaîne binaire au même niveau qui réunit d'abord
les enfants gros, puis rattache les petits un à un ; valeur transmise à sklearn : $\ell^{z}$, de sorte que son
$\lambda=1/\mathrm{valeur}$ vaut $\ell^{-z}$.

## 3. Théorèmes

### 3.1 Lemme D (la diagonale ne compte pas)

*Énoncé.* Soit $\tilde{u}$ la même ultramétrique à diagonale nulle. À tout niveau $r$, les blocs d'au moins deux
points de $u$ et de $\tilde{u}$ coïncident. Pour mcs ≥ 2, condensation, stabilités, sélections et étiquettes de la
tête N-aire sont donc les mêmes sur $u$ et sur $\tilde{u}$.

*Preuve.* Si $B$ est un bloc de $\tilde{u}$ au niveau $r$ avec $|B|\geq 2$, chaque $i\in B$ a un $j\neq i$ dans $B$
avec $u(i,j)\leq r$, donc $e_i\leq u(i,j)\leq r$ : $B\subset X_r$, et hors diagonale les deux relations sont la même.
La réciproque est immédiate. Un point qui n'est pas encore entré est, dans $\tilde{u}$, un singleton, donc un petit
enfant (mcs ≥ 2) : il sort au même niveau dans les deux lectures. □

*Contrôle.* Sur 24 nuages à k = 1 et sur toutes les scènes de la démonstration, les blocs d'au moins deux sites de
l'oracle (`h.blocks`, diagonale comprise), les composantes de $u\leq r$ calculées exactement, et les nœuds vivants du
dendrogramme coïncident à **295 niveaux** FULL ([levels_out.txt](levels_out.txt), L6 ;
[tower.py](tower.py), `check_blocks`).

### 3.2 Théorème A (coïncidence sur les arbres binaires sans ex æquo)

*Énoncé.* Si l'arbre du lien simple $T$ de sklearn n'a aucun nœud interne ayant un enfant interne de même valeur, alors
$D=T$ et, pour tout mcs ≥ 2, la tête N-aire et `tree_to_labels` de sklearn ont les mêmes clusters (ensembles de
points), les mêmes naissances, les mêmes λ de sortie, donc les mêmes stabilités (aux arrondis de sommation près), la
même sélection EOM ou feuilles, avec ou sans `allow_single_cluster`, avec ε nul ou différent de toute naissance, et
les mêmes étiquettes à renumérotation près.

*Preuve.* Les trois cas de `_condense_tree` sont les cas $|B|=2$, $0$ et $1$ de la règle N-aire pour deux enfants, au
même $\lambda=1/\mathrm{valeur}$. Les lignes de parent $C$ sont exactement les sorties de $C$ et ses clusters enfants
à leur λ de naissance : même stabilité. L'EOM de sklearn parcourt les identifiants décroissants, qui sont un ordre
topologique (les nouveaux clusters sont numérotés en largeur), avec la même inégalité stricte, la même exclusion de la
racine et le même test de taille. Pour ε, les comparaisons $<\varepsilon$ et $>\varepsilon$ de sklearn coïncident avec
$<$ et $\geq$ dès qu'aucune naissance n'égale ε. Pour l'étiquetage, chaque ligne (parent, enfant non retenu) est lue
quand l'enfant est encore un singleton du union-find (parcours en largeur), donc le représentant reste du côté du
parent : un point reçoit le plus bas cluster retenu au-dessus de sa ligne de sortie. Le cas particulier de la racine
est recopié. □

*Contrôle* ([equivalence.py](equivalence.py), [equivalence_out.json](equivalence_out.json)) : 150 arbres (5 familles,
n = 30, 80, 200, k = 1, 2, 3, 5, 10), 6 752 configurations (mcs, EOM ou feuilles, `allow_single_cluster`, ε nul ou
milieu de niveaux). **Aucun** écart entre sklearn tel quel et la tête N-aire sur un arbre sans plateau
(`i_ne_ii_without_plateau = 0`). Les 1 015 écarts observés sont tous sur des arbres à plateaux (§ 3.5).

### 3.3 Théorème B (binarisation canonique) et corollaire

*Énoncé.* Pour tout dendrogramme atomisé $D$ et mcs ≥ 2, `tree_to_labels` de sklearn appliqué à $\beta(D)$ (valeurs
$\ell^{z}$) rend la sortie de la tête N-aire avec $\varphi=\ell^{-z}$, pour EOM et feuilles, avec ou sans
`allow_single_cluster`, ε nul ; et pour ε > 0 différent de toute naissance, **sauf** si une remontée atteint une racine
scindée en au moins trois enfants gros.

*Preuve.* Au nœud $v$ de niveau $\ell$, enfants gros $b_1,\dots,b_p$ et petits $s_1,\dots,s_q$, la condensation de
sklearn descend $\beta$ de haut en bas. Les petits rattachés en dernier sortent du cluster courant à $\lambda(\ell)$,
qui continue sur la partie restante tant qu'elle est grosse : c'est la sortie N-aire. Si $p=0$, la chaîne finit par
deux côtés petits : tous les points restants sortent à $\lambda(\ell)$ (cas $|B|=0$). Si $p=1$, $b_1$ continue (cas
$|B|=1$). Si $p\geq 2$, le cluster meurt au nœud $U_p=U_{p-1}\cup b_p$ ; les nœuds intermédiaires $U_j$
($2\leq j\leq p-1$) deviennent des clusters nés et morts à $\lambda(\ell)$, sans point sorti, donc de stabilité
**nulle**, et chaque $b_i$ naît à $\lambda(\ell)$ comme dans la règle N-aire. Les enfants de $U_j$ sont gros et, $D$
étant atomisé, chaque point de $b_i$ le quitte à un niveau strictement inférieur à $\ell$ : $S(b_i)>0$ car $\varphi$
est strictement décroissante. L'EOM ne retient donc jamais $U_j$ ($\sum>0=S(U_j)$) et lui transmet
$\tilde{S}(U_j)=\tilde{S}(U_{j-1})+\tilde{S}(b_j)$ : le parent compare sa stabilité à $\sum_i\tilde{S}(b_i)$, comme en
N-aire, et sa stabilité propre est la même ($|U_{p-1}|+|b_p|=\sum_i|b_i|$). Les $U_j$ ne sont pas des feuilles. Pour ε,
une remontée traverse les $U_j$ (naissance $\ell$, comme les $b_i$) sans s'y arrêter tant qu'elle n'atteint pas la
racine. Si elle l'atteint, `traverse_upwards` rend l'enfant **binaire** de la racine sur le chemin, une union
$U_{p-1}$ quand $p\geq 3$, là où la règle N-aire rend $b_i$ : c'est l'exception. □

*Exception exhibée* ([root_eps.py](root_eps.py), [root_eps_out.txt](root_eps_out.txt)) : racine au niveau 10 à trois
enfants gros, ε = 20 : la règle N-aire garde les trois ; sklearn rend, selon la binarisation de la racine,
`abc | defghi`, `abcdef | ghi` ou `abcghi | def`. Jamais rencontrée dans la campagne (`ii_ne_iii_root_eps = 0`).

*Contrôle.* Tête N-aire contre code de sklearn sur $\beta(D)$ : **5 192/5 192** là où le code compilé s'exécute, et
**6 752/6 752** avec la transcription des fonctions `cdef` (sélection et ε seulement ; condensation et étiquetage
restent ceux de sklearn ; la transcription égale le code compilé sur 5 080/5 080 configurations,
[sk_transcription.py](sk_transcription.py)).

*Corollaire (outil d'équité).* Pour appliquer « la sélection de sklearn » à la hiérarchie de la tour, il suffit de
passer $\beta(D)$ à `tree_to_labels` : même code que l'adversaire, résultat fonction de la seule ultramétrique (pas de
l'ordre de binarisation), égal à la tête N-aire. Le paramètre z passe par les valeurs ($\ell^{z}$), ε par $\varepsilon^{z}$.

### 3.4 Proposition G (fréquence des ex æquo dans l'arbre de HDBSCAN)

*Énoncé.* (i) Pour `min_samples` ≤ 2 et des coordonnées tirées d'une loi à densité, deux arêtes de l'arbre couvrant
ont la même valeur avec probabilité nulle. (ii) Pour `min_samples` ≥ 3, un plateau (au moins trois composantes réunies
au même niveau) apparaît avec probabilité positive. (iii) Sur coordonnées entières, des plateaux apparaissent à tout k.

*Preuve.* (i) Chaque poids $\max(\mathrm{core}(a),\mathrm{core}(b),d(a,b))$ est la distance d'une paire de points
(le cœur est la distance au plus proche voisin, ou 0). Deux arêtes distinctes de même poids exigent soit deux paires
distinctes à même distance (mesure nulle), soit deux arêtes réalisées par la même paire, ce qui force les deux arêtes à
être la même (le seul point à distance $\mathrm{core}(a)$ de $a$ est génériquement son plus proche voisin). (ii) Les
arêtes de $x$ vers les points intérieurs à sa boule de cœur valent toutes **exactement** $\mathrm{core}(x)$ dès que
leur cœur est plus petit. Configuration explicite : $A=\lbrace 0;0{,}1;0{,}2\rbrace$, $x=0{,}98$,
$B=\lbrace 1{,}8;1{,}9;2{,}0\rbrace$, `min_samples` = 3 : $\mathrm{core}(x)=d(x;1{,}8)=0{,}82$ et
$d_{mr}(x;0{,}2)=d_{mr}(x;1{,}8)=0{,}82$ ; $A$, $\lbrace x\rbrace$, $B$ fusionnent au même niveau. L'égalité est une
identité sur l'ouvert défini par des inégalités strictes (ordre des voisins de $x$, cœurs de $A$ et $B$ plus petits,
aucune autre jonction en dessous), qui a une mesure positive. (iii) Égalités de distances entières. □

*Contrôles.* [genericity.py](genericity.py) : la configuration (ii) garde un plateau à trois enfants sous 200/200
perturbations gaussiennes (σ = 0,005 et 0,01), et en 3D ; à `min_samples` = 2, 0/200. Campagne : arbres à plateaux
12/30 à k = 1 et 2 (les 12 nuages entiers ou en réseau), **30/30 à k = 3 et 5, 29/30 à k = 10** ; 1 223 nœuds de plateau
sur 150 arbres. Les scènes v11 sont quantifiées (grille u21, LiDAR au millimètre) : elles sont dans le cas (iii).

### 3.5 Proposition T (ce qui diffère aux ex æquo)

Soit un plateau $v$ de niveau $\ell$, enfants gros $b_i$ et petits $s_j$. Selon l'ordre dans lequel sklearn le
binarise :

- **T1, petit enfant collé.** Un petit fusionné à un gros avant la scission sort de ce gros, non du parent : il prend
  l'étiquette de ce frère si celui-ci est retenu. N-aire : il sort du parent (bruit si le parent n'est pas retenu).
- **T2, cluster fantôme.** Des petits réunis d'abord, jusqu'à mcs, forment un cluster né et mort à $\ell$, de stabilité
  nulle, qui n'est une composante à **aucun** niveau. C'est une feuille : `leaf` le retient toujours, l'EOM dès que son
  parent perd.
- **T3, fausse scission.** S'il n'y a qu'un gros $b$ mais qu'un fantôme se forme, le parent meurt à $\ell$ et $b$ devient
  un nouveau cluster. L'EOM compare alors $S_{\mathrm{bin}}(P)$ à $S(b)+0$ au lieu de comparer
  $S_{N}(P)=S_{\mathrm{bin}}(P)+S(b)$ aux descendants de $b$ : la sélection change.
- **T4**, ε au-dessus d'une racine N-aire (§ 3.3). **T5**, ε égal à une naissance : asymétrie $<$/$>$ de sklearn,
  indépendante des plateaux.

*Fixtures exactes* ([ties.py](ties.py), [ties_out.txt](ties_out.txt)) :

| Fixture | Tête N-aire | sklearn |
| --- | --- | --- |
| F0, deux triangles de la thèse (§ 6.1), k = 2 : les cinq arêtes valent $\sqrt{2}$ | tout bruit (mcs 2 et 3) | sur cette machine, tout bruit pour les **720** ordres d'entrée ; sous les **120** ordres possibles des cinq arêtes égales (émulation d'autres tris), **11 partitions** à mcs 2, dont `ABC | DEF` 24 fois, `AB | CDEF` 24 fois ; à mcs 3, `ABC | DEF` 24 fois sur 120 |
| F1, pont : $L=\lbrace 0..3\rbrace$, $b=6$, $R=\lbrace 9..12\rbrace$, `min_samples` = 1, mcs = 3 | `0123 | 9ABC`, b en bruit | b donné à $L$ (1 543 ordres) ou à $R$ (1 457) sur 3 000 ordres d'entrée tirés au hasard |
| F2, fantôme et fausse scission : $P=\lbrace 0,1,2,3,6,9\rbrace$, $C$ loin, mcs = 2 | `012369 | abcd` | `0123 | 69 | abcd` pour 3 000/3 000 ordres sur cette machine ; l'émulation au tri stable, Prim partant de 0, rend `012369 | abcd` |
| F3, ε = naissance exacte | règle cohérente : `x1, x2` en bruit | sklearn (transcrit) donne à E ses points tombés : ensembles retenus emboîtés |

Sur F0, l'arbre de HDBSCAN n'a qu'une fusion à six enfants : **aucune hiérarchie de HDBSCAN ne sépare ABC de DEF**,
mais une autre implantation du tri peut faire « réussir » la fixture à sklearn par des clusters de stabilité nulle.
La correction de ce test n'est donc pas lisible sur `labels_`, seulement sur l'arbre.

*Petits nuages.* [ties.py](ties.py) section C1 (huit ordres de tri des seules arêtes égales, fonctions de sklearn) :
aucune variation à k ≤ 2 sur flottants ; à k ≥ 3 la partition change dans 4 à 15 cas sur 18 selon la famille ;
l'écart de score m05 entre ordres atteint 0,227 (`blobs3d`, k = 5, `leaf`) et **0,97** sur coordonnées entières en
`leaf` ; en EOM il reste ≤ 0,017. [ties_classify.py](ties_classify.py) : dans 25 à 49 % des configurations à
k ≥ 3, la sortie de sklearn contient un cluster qui n'est pas un bloc de sa propre hiérarchie N-aire (7,5 à 17 % des
points) ; sur données entières, dès k = 1 en `leaf` (8 % des points).

*Comment le rapporter* : publier R1 (tête N-aire sur l'arbre de HDBSCAN, z = 1) à côté de R0 ; l'écart R1 − R0 est
l'effet de binarisation, ni de la hiérarchie ni de la tour. Publier par scène : nœuds de plateau, points dans des
petits enfants de plateaux (majorant des points T1), clusters non-blocs de sklearn, étendue de R0 sur 5 permutations.

### 3.6 Proposition H (facteur global) et forme de φ

*Énoncé.* Multiplier tous les niveaux par $c>0$ ne change ni la condensation, ni la sélection EOM ou feuilles, ni les
étiquettes, pour $\varphi=\ell^{-z}$ quel que soit $z>0$ ; ε se transporte en $c\varepsilon$. *Preuve.* La condensation
ne lit que l'ordre des niveaux et les tailles ; $S_c=c^{-z}S$ pour tout cluster, et toutes les comparaisons de l'EOM
sont homogènes de degré 1. Plus généralement, toute transformation affine positive de φ convient. □ *Contrôles* : sklearn
sur $X$ et $X/2$, 180/180 étiquettes identiques ; tête N-aire, niveaux multipliés par 0,5, 2 et 3,7, z = 1, 2, 3 :
1 620/1 620 ([levels_out.txt](levels_out.txt), L1-L2).

*La forme de φ compte* : sur le même arbre, z = 1 et z = 3 donnent des partitions différentes dans 30 configurations
sur 180 (L3). z est un choix de modèle, à appliquer identiquement aux deux arbres. Mesurer z sur le niveau **carré**
de la tour revient à doubler z en rayon.

### 3.7 Proposition C (correspondance des niveaux)

- (i) *Cœur* : $d_k(x)$ de la tour (oracle, point compris) est égal **au bit près** à la distance de cœur de sklearn à
  `min_samples` = k : 1 235/1 235 (L5). C'est la même définition (thèse, déf. 16).
- (ii) *k = 1* : $H^{r}_{1}$ (liaison simple en rayon, note v11, H2) et HDBSCAN à `min_samples` = 1 ont les mêmes blocs,
  à niveaux exactement doublés pour HDBSCAN (24/24), et les têtes N-aires rendent les mêmes partitions pour tout mcs,
  toute sélection et z = 1 ou 3 (288/288, L6). D'où la porte D6.
- (iii) *k ≥ 2* : aucun facteur global ne relie les deux arbres ; la question « facteur 2 » a deux sens. Comme
  conversion d'unité, il est sans effet sur EOM et feuilles (prop. H). Comme convention, `alpha = 2` de sklearn
  ($\max(\mathrm{core},\mathrm{core},d/2)$ : cœur en rayon, paire en rayon) est une **autre filtration** : 0/36 arbres
  identiques à `alpha = 1` pour k = 2 à 10 (L4). La tour est en rayon partout ; HDBSCAN par défaut mêle rayon (cœurs)
  et diamètre (paires).
- (iv) *Thèse* : le Fait 10 ($u_{RSL}=\frac{1}{2}\min\max d_{mreach,K}$) divise aussi le cœur par 2, alors que la
  Remarque qui le suit demande $r_K\leq r$ et $d\leq 2r$, c'est-à-dire `alpha = 2`. Pour deux points à distance 2 de
  cœurs 2 : 1 selon le Fait 10, 2 selon la Remarque (L7). Les deux lectures sont des filtrations différentes dès K ≥ 2.

Conséquence pour l'EOM : chaque arbre reste dans son unité native, la même forme $\ell^{-z}$ s'applique aux deux, et
`alpha` est un réglage public de l'adversaire, pas une conversion.

### 3.8 Proposition E (critique de la définition 19 de la thèse)

*Énoncé (idéalisé).* Supposons qu'un point d'échantillon $x$ d'un cluster feuille $C$ quitte $C$ au niveau
$\lambda_x=g(f(x))$, $g$ croissante, et notons $t_C$ la densité de naissance. Alors, par la loi des grands nombres,
$S_n(C)/n\to\int_C(g(f(x))-g(t_C))\,f(x)\,dx$. Aucune $g$ ne rend cette limite proportionnelle à l'excès de masse
$E(C)=\int_C(f-t_C)\,dx$ de la définition 19 ; en revanche la stabilité pondérée
$S'(C)=\sum_{x\in C}(1-\lambda_C/\lambda_x)$ avec $\lambda$ = densité ($z$ = dimension) estime $E(C)$.

*Preuve.* La somme empirique est une intégrale contre $f\,dx$. Une proportionnalité pour toutes densités imposerait
$(g(f)-g(t))f=c(f-t)$ pour $f>t>0$ ; en $f\to t$, $g'(t)t=c$, donc $g=c\log$, et $f=2t$ donne $2t\log 2\neq t$.
Pour $S'$ : $\frac{1}{n}\sum(1-t_C/f(x))\to\int_C(1-t_C/f)f\,dx=E(C)$. □

Lecture : l'estimateur « $\widehat{E}(C)\propto\sum(\hat{\lambda}_x-\hat{\lambda}_{\min})$ » de la thèse (§ 4.4.4)
n'estime pas la déf. 19, et « HDBSCAN estime la densité par $1/r_x$ » est faux en dimension 3 (densité K-NN
$\propto r^{-3}$). L'EOM est une **famille** de critères (φ, pondération) ; aucun n'est canonique. D'où D5 : même
critère pour les deux arbres, z = 1 publié comme convention de sklearn, pas comme « l'excès de masse ». $S'$ est
invariante par facteur global (degré 0) : une tête candidate pour le rôle « modèle », non mesurée ici.

## 4. Contrôles numériques (petits nuages, oracles de correction)

| Contrôle | Script, sortie | Résultat |
| --- | --- | --- |
| fit de sklearn contre `tree_to_labels` sur son propre arbre | [equivalence.py](equivalence.py), [equivalence_out.json](equivalence_out.json) | 5 080/5 080 identiques (les autres plantent : numpy 2.5) |
| transcription des fonctions `cdef` contre le code compilé | idem | 5 080/5 080, 0 écart |
| tête N-aire contre sklearn sur $\beta(D)$ | idem | 5 192/5 192 (compilé), 6 752/6 752 (transcription) |
| sklearn tel quel contre tête N-aire | idem | 5 737/6 752 identiques ; 1 015 écarts, **tous** sur des arbres à plateaux ; 354 en EOM, 661 en `leaf` |
| écarts par k (configurations) | idem | k = 1 : 30/1 248 ; 2 : 30/1 248 (données entières) ; 3 : 279/1 376 ; 5 : 353/1 440 ; 10 : 323/1 440 |
| ε avec numpy 2.5.3 | idem | `TypeError` dans 1 672/4 352 configurations |
| antichaînes exactes contre énumération | [oracle_check.py](oracle_check.py), [oracle_check_out.txt](oracle_check_out.txt) | 120 arbres, 1 740 antichaînes : m05, PQ et meilleur IoU 120/120 |
| facteur global, forme de φ, `alpha`, cœur, étalonnage k = 1, Fait 10 | [levels.py](levels.py), [levels_out.txt](levels_out.txt) | § 3.6 et 3.7 |
| ex æquo : fixtures et ordres de tri | [ties.py](ties.py), [ties_out.txt](ties_out.txt), [ties_out.json](ties_out.json) | § 3.5 |
| clusters non-blocs de sklearn | [ties_classify.py](ties_classify.py), [ties_classify_out.txt](ties_classify_out.txt) | § 3.5 |
| plateaux génériques à `min_samples` = 3 | [genericity.py](genericity.py), [genericity_out.txt](genericity_out.txt) | § 3.4 |
| exception ε à la racine | [root_eps.py](root_eps.py), [root_eps_out.txt](root_eps_out.txt) | § 3.3 |

## 5. Protocole d'équité

**Entrées.** Les deux méthodes reçoivent le **même** multiensemble de coordonnées quantifiées (entiers u21 en
`float64` pour sklearn, sans renormalisation propre à un côté) ; doublons traités pareil des deux côtés (sites
distincts pour les deux, ou lignes avec multiplicité pour les deux). Mêmes points void retirés de l'évaluation.

**Ordre et taille.** `min_samples = k` (point compris, identique au temps de cœur de la tour, prop. C (i)) ; ligne de
diagnostic à `min_samples = k + 1`, puisque $H^{r}_{k+1}$ fait entrer les sites à partir de $\alpha_{k+1}$. Même mcs,
compté en sites.

**Tête.** Même tête N-aire pour les deux arbres (D1), contrôlée par le code de sklearn sur $\beta(D)$ (D2). Même
$\varphi=\ell^{-z}$ dans l'unité native de chaque arbre ; z = 1 toujours publié ; $z^{*}$ réglé sur dev, le même
ensemble de valeurs pour les deux arbres (proposition : {1, 2, 3}, 3 étant la dimension). Ne jamais appliquer z au
niveau carré d'un côté et au rayon de l'autre.

**Adversaire tel quel.** `HDBSCAN(min_samples=k, algorithm='kd_tree', metric='euclidean', copy=True)`, les autres
paramètres publics réglés sur dev : mcs, `cluster_selection_method`, `cluster_selection_epsilon`,
`allow_single_cluster`, `alpha` ∈ {1, 2}. `alpha` n'existe pas côté tour : asymétrie déclarée, au détriment de la
tour (comme v10, D6). `max_cluster_size` non utilisé. Un plantage (ε avec numpy ≥ 2.5) est un **refus compté**, jamais
un repli silencieux.

**Grilles et réglage** (identiques pour R1–R4) : mcs ∈ {k, 5, 10, 20, 50, ⌊√n⌋} ; sélection ∈ {EOM, feuilles} ;
z ∈ {1, 2, 3} ; ε ∈ {0, milieu des niveaux propres aux quantiles 0,5 et 0,9} (milieu entre deux niveaux distincts, pour
ne jamais tomber sur une naissance, cf. T5) ; `allow_single_cluster` = faux (vrai en diagnostic). Une configuration
par (méthode, k), choisie sur dev par la métrique primaire ; « oracle de réglage » (meilleure configuration par scène de
test) publié pour **tous** les côtés, comme borne seulement.

**Dev et test.** Synthétique : graines neuves pour le test, jamais vues pendant le développement (les 128 scènes
déjà mesurées sont du dev). LiDAR : dev sur des trames des séquences d'entraînement ; la séquence 08, démos de
`Zoltan/` comprises, sert une fois, à la fin (`Zoltan/FoundationModel/MESURE.md`, partitions explicites). Les
mesures v11 actuelles sont toutes en 08 : aucun réglage ne doit en être tiré.

**Machine.** Toutes les lignes dans la même session G4, même Python, mêmes versions épinglées (sklearn 1.7.2 ou 1.9.1 :
algorithme identique ; numpy à noter, avec `np.show_runtime()` et l'empreinte de `_tree.pyx`). Les étiquettes de
sklearn ne se comparent jamais d'une machine à l'autre.

**Coût de sklearn à l'échelle.** Un seul `fit` par (scène, k, `alpha`) suffit : `tree_to_labels` sur
`_single_linkage_tree_` rend exactement `labels_` (5 080/5 080) pour toute la grille de sélection ; plus 5 `fit`
permutés à la configuration retenue, pour l'étendue.

**Métriques.** Le même code pour toutes les lignes ([nary_head.py](nary_head.py), `score_flat`) : bruit = non affecté,
jamais une classe ; void exclu ; par objet vrai, IoU, précision, rappel du cluster apparié. Agrégats : **m05** (somme
des IoU > 1/2 sur le nombre d'objets), **best** (moyenne du meilleur IoU, sans seuil), **PQ** (SQ, RQ), nombre de
clusters et part de bruit. Une métrique primaire choisie avant le test (proposition : m05, qui a un oracle exact et
tranche au seuil des « sauvetages » de la note v11).

**Complétion et existence** (rôle « modèle ») : la ligne sans complétion est toujours publiée. Toute complétion
appliquée à la tour (lignée, vote) est accompagnée, pour HDBSCAN, de l'analogue applicable (plus proche point
étiqueté, remplissage borné b ρ de la v10). Un critère d'existence par taille de cœur est applicable aux deux : sur
l'arbre de HDBSCAN, tout point d'un bloc est un cœur à ce niveau ($d_{mr}\geq\mathrm{core}$), le critère s'y réduit à
la taille ; sur $H^{r}_{k+1}$ il est plus strict. Les masses fractionnaires du § 9.1 n'ont pas d'analogue : bras
déclaré propre à la tour.

**Statistique.** Unité : la scène ; écarts appariés par scène ; intervalles bootstrap stratifiés par famille ; chaque
terme de la décomposition (§ 6) avec son intervalle ; trames voisines corrélées déclarées.

## 6. Lignes à publier et décomposition

| Ligne | Contenu | Rôle |
| --- | --- | --- |
| R0 | `HDBSCAN` tel quel, réglé sur dev ; R0p : min, moyenne, max sur 5 permutations de l'entrée | l'adversaire |
| R1 | tête N-aire sur l'arbre de HDBSCAN (`alpha` réglé), z = 1 | R1 − R0 : binarisation |
| R2 | idem, $z^{*}$ | R2 − R1 : effet de z sans tour |
| R3 | tête N-aire sur $H^{r}_{k+1}$, z = 1 | — |
| R4 | idem, $z^{*}$ | R4 − R2 : hiérarchie à tête égale |
| R5, R6 | antichaîne oracle de chaque hiérarchie au même mcs : sur tous les blocs, et sur les seuls clusters condensés (« cond ») | plafond, et coût de la condensation |
| R7 | niveau B (meilleur bloc par objet, sans antichaîne) | le niveau B actuel de la note v11 |
| diag. | `min_samples = k + 1` ; `allow_single_cluster` ; avec complétion ; plateaux et clusters non-blocs | — |

Décomposition, pour une métrique $M$ et les configurations retenues :
$M(R4)-M(R0)=[M(R1)-M(R0)]+[M(R2)-M(R1)]+[M(R4)-M(R2)]$ et
$M(R4)-M(R2)=[O(T)-O(H)]-[(O(T)-M(R4))-(O(H)-M(R2))]$, où $O$ est l'oracle de la métrique sur la tour $T$ ou sur
HDBSCAN $H$. Le premier crochet est le plafond de hiérarchie, le second la différence de regret de sélection. Pour
isoler la binarisation, R1 est aussi évaluée à la configuration de R0. L'oracle de HDBSCAN porte sur son dendrogramme
**N-aire** : les nœuds binaires intermédiaires de sklearn ne sont pas des blocs de sa hiérarchie.

Lecture voulue par l'utilisateur (« si HDBSCAN gagne, c'est l'algorithme tiré de la tour qui est à revoir ») : si
$O(T)<O(H)$ sur une cohorte, c'est la hiérarchie qui est en cause ; si $O(T)\geq O(H)$ mais $M(R4)<M(R2)$, c'est la
sélection ; si $M(R4)\geq M(R2)$ mais $M(R4)<M(R0)$, c'est un effet de z ou de binarisation.

**Oracles exacts** ([nary_head.py](nary_head.py)) : m05 par programme dynamique sur l'arbre
($f(v)=\max(w(v),\sum_c f(c))$, $w(v)$ = IoU > 1/2 du bloc, unique par disjonction) ; PQ par sac à dos sur le nombre
d'appariements $t$ (à l'optimum FP = 0, $PQ=2S(t)/(|G|+t)$) ; meilleur IoU sans seuil par programme linéaire en nombres
entiers (HiGHS : $y_v$ binaire, $x_{ov}\leq y_v$, $\sum_v x_{ov}\leq 1$, somme des $y$ au-dessus de chaque feuille ≤ 1).
Complexité des deux premiers : linéaire en nœuds, quadratique en objets pour PQ. Celle du troisième à 32 000 points :
non établie (complexité exacte du problème non établie non plus).

**Démonstration** ([demo_rows.py](demo_rows.py), [demo_rows_out.txt](demo_rows_out.txt)). Quatre scènes de 6 à 24
sites, k = 2 et 3, mcs 2 à 4, $H^{r}_{k+1}$ par l'oracle (étage A ou B), blocs recoupés à chaque niveau FULL. Ce sont
des contrôles du harnais, sans valeur statistique :

- deux triangles, k = 2 : R3 = R4 = 1 (ABC | DEF) contre R0 = R1 = R2 = 0 ; $O(H)=0$, $O(T)=1$ : effet de hiérarchie
  pur ;
- `deux_densites`, k = 2, mcs 3 : R0 = 0 mais R0p de 0 à 0,5 en m05 selon l'ordre d'entrée ; à mcs 2, R0 = 0,312 et
  R1 = 0 : la binarisation de sklearn l'avantage ici ; la tour 0,5 ;
- `pont_chaine` : $O(T)=O(H)=0{,}9$ ; la tête `leaf` sur la tour tombe à 0 (R3) quand HDBSCAN reste à 0,825 :
  regret de sélection, pas défaut de hiérarchie ; z = 3 dégrade l'EOM sur la tour (0,409 contre 0,818) ;
- `trois_amas`, k = 2 : toutes les lignes à 1 en EOM à mcs ≥ 3 ; à k = 3, z = 3 coûte 0,143 à la tour (R4).

## 7. Prédictions écrites d'avance et plan G4

Prédictions pour la campagne E1 au niveau C (sessions gardées, n = 8 000, 16 000, 32 000), à juger avant toute
conclusion :

- **P1** (théorème) : à k = 1, R3 ≡ R1 et R4 ≡ R2 scène par scène. Tout écart est un défaut du harnais.
- **P2** : sur les scènes quantifiées u21 et LiDAR, l'arbre de HDBSCAN a des plateaux à tout k ; à k ≥ 3, R1 ≠ R0 sur
  une part non nulle des scènes, et $|M(R1)-M(R0)|$ est plus grand en `leaf` qu'en EOM.
- **P3** : à k ≥ 3, l'étendue R0p sur 5 permutations est non nulle sur la majorité des scènes.
- **P4** : $M(R2)>M(R1)$ en moyenne sur le synthétique (la v10 mesurait déjà +0,039 avec z sur l'arbre de HDBSCAN) :
  une part du gain attribué à la tour en v10 est un effet de z.
- **P5** : $O(T)\geq O(H)$ en moyenne à k = 3, 5, 10 sur le synthétique (cohérent avec le niveau B de la note v11) ;
  $M(R4)-M(R2)$ est positif mais plus petit que $M(R4)-M(R0)$.
- **P6** : sur LiDAR à mcs = k, le regret de sélection de la tour dépasse celui de HDBSCAN (la v10 sur-segmentait :
  5 732 clusters contre 867 par trame) ; la qualification de $H^{r}_{k+1}$ le réduit (prédiction E1 du juge).
- **P7** : `alpha = 2` améliore R1/R2 à z = 3 plus qu'à z = 1 (cœurs et paires dans la même unité).

Plan par session (repères de la session F : 206 s pour 128 scènes synthétiques, 440 s pour 5 démos à 4 ordres) : par
(scène, k) un `fit` par `alpha`, la grille de sélection par `tree_to_labels`, 5 `fit` permutés ; la tour par la
chaîne v11 existante ; le dendrogramme N-aire de $H^{r}_{k+1}$ émis par le balayage exact de `evaluate_hanging`
(`bench/points_hierarchy.py`), qui groupe déjà les plateaux ; jamais de matrice par paire. Coûts à mesurer, non
estimés ici.

## 8. Lecture critique

**Thèse.**
- Déf. 19 et § 4.4.4 : l'estimateur n'estime pas l'excès de masse défini (prop. E) ; $1/r_x$ n'est pas la densité en
  dimension 3.
- Fait 10 contre Remarque : facteur 1/2 incohérent (prop. C (iv)).
- § 5.2 : la règle « si loss(père) < somme, garder le père », avec loss = −Ẽ, envoie l'égalité aux enfants ; sklearn
  l'envoie au parent. Détail, mais une tête « hackée » doit fixer sa règle d'égalité.
- Algorithme 1 (§ 9.2.3) : Kruskal sur le graphe dual **binarise** les multifusions (T1-T3 s'y appliquent) ; l'étape 9
  (réaffectation au plus proche voisin) est une complétion qui n'est pas offerte à HDBSCAN dans les comparaisons de la
  thèse ; mcs y porte sur des masses fractionnaires de faces, pas sur des points : mcs n'a pas le même sens des deux
  côtés.
- § 6.1 : la conclusion sur les deux triangles tient sur l'**arbre** (une seule fusion à six enfants), pas sur
  `labels_`, qui dépend de l'ordre de tri (F0).

**Auditeur.** « Cela ne garantit pas stabilité de l'IoU ni d'une sélection de labels » (réponse Q1) : juste, et plus
fort que dit. L'EOM est discontinue même sans ex æquo (une égalité $S(P)=\sum\tilde{S}$ bascule), et la sortie de
sklearn n'est même pas une fonction de son ultramétrique aux ex æquo : aucune stabilité de sortie plate ne peut être
attendue de l'adversaire non plus. E2 doit mesurer la stabilité au niveau C pour les **deux** méthodes.

**v10 (données, pas autorité).**
- EVAL_v2 § 2.5 définit déjà la condensation N-aire (même règle que D1) mais affirme que, sur un plateau binarisé,
  « les deux formulations ne diffèrent que par des clusters de durée nulle ». C'est trop faible : ces clusters de durée
  nulle changent la sélection EOM de clusters non dégénérés (T3, F2) et portent jusqu'à 17 % des points sur données
  flottantes, 21 % sur données entières (k = 5, `leaf`) ([ties_classify_out.txt](ties_classify_out.txt)).
- EVAL_v2 règle ε sur des quantiles des rayons de fusion : un quantile de niveaux **est** un niveau, donc une égalité
  ε = naissance (T5). Prendre des milieux.
- « Tour 0,758–0,761 contre HDBSCAN 0,703–0,716 » mélange hiérarchie et tête : z sur l'arbre de HDBSCAN seul donne
  +0,039 ; à K = 1, où les deux hiérarchies coïncident, la v10 publiait 0,776 contre 0,715, un effet de tête pur de
  sa propre lecture. La porte D6 rend ce contrôle obligatoire.
- « sklearn rend des étiquettes différentes selon la machine » : mécanisme identifié (Prim par indice, `argsort` non
  stable dépendant du jeu d'instructions, binarisation), et il agit aussi à machine fixe par l'ordre d'entrée.

**Note v11 (`HIERARCHIE_POINTS.md`).** « Même sélection pour toutes les hiérarchies » : elle doit être la tête N-aire,
la sélection binaire de sklearn n'étant pas définie sans choix d'ordre. Le niveau B groupe déjà les ex æquo
(`evaluate_linkage`) : cohérent. Les mesures G4 utilisaient sklearn 1.7.2 : même algorithme que 1.9.1, mais numpy
2.2.6 ; leurs étiquettes HDBSCAN ne sont pas reproductibles bit à bit sur le codespace aux ex æquo.

**Ce rapport.** Tout est mesuré à n ≤ 300 (tour : n ≤ 24) : aucune ampleur à l'échelle n'est établie. La transcription
des fonctions `cdef` est validée là où le code compilé tourne, pas au-delà. Les écarts d'arrondi de sommation entre
tête N-aire et sklearn sont possibles en théorie (égalités au dernier bit), jamais observés (0/6 752). Les métriques
m05, best et PQ sont proposées ; le choix de la métrique primaire revient à l'utilisateur.

## 9. Ouvert

- Ampleur des effets d'ex æquo (R1 − R0, R0p, clusters non-blocs) aux tailles d'intérêt : à mesurer (P2, P3).
- Coût du programme linéaire du meilleur IoU à 32 000 points ; complexité exacte de ce problème d'antichaîne.
- Règle ε pour une racine N-aire : la règle cohérente garde les enfants de la racine ; d'autres conventions sont
  défendables (conjecture : sans effet mesurable, la racine n'étant presque jamais N-aire en gros enfants).
- La stabilité pondérée $S'$ (prop. E) comme tête « excès de masse au sens de la déf. 19 » : non mesurée.
- Le rapprochement `alpha = 2` / $H^{r}_{k+1}$ : la v10 le mesurait pour C∩X seulement.

## 10. Reproduction

Depuis ce dossier, `PYTHONDONTWRITEBYTECODE=1 python3 -B -u <script> > <sortie>` ; aucun octet écrit hors du
dossier. Lecture seule de `contexte/oracle_hierarchy.py` et de l'arbre `build/v11-claude-20261003` (commit ab1a739d1).

| Script | Sortie | Durée (codespace) |
| --- | --- | --- |
| `nary_head.py` | bibliothèque : dendrogramme N-aire, tête, binarisation, appel de sklearn, oracles, métriques | — |
| `sk_transcription.py` | transcription des fonctions `cdef` de `_tree.pyx` (ε) | — |
| `tower.py` | dendrogramme exact de $H^{r}_{k+1}$ (étages A et B), recoupe des blocs | — |
| `equivalence.py 20261004` | `equivalence_out.json` | 45 s |
| `ties.py` | `ties_out.txt`, `ties_out.json` | environ 6 min |
| `ties_classify.py` | `ties_classify_out.txt`, `ties_classify_out.json` | environ 1 min |
| `levels.py` | `levels_out.txt`, `levels_out.json` | environ 3 min |
| `oracle_check.py` | `oracle_check_out.txt` | environ 30 s |
| `genericity.py` | `genericity_out.txt` | quelques secondes |
| `root_eps.py` | `root_eps_out.txt` | instantané |
| `numpy_versions.sh` | `numpy_versions_out.txt` (numpy 2.5.3 et 2.2.6) | instantané |
| `demo_rows.py` | `demo_rows_out.txt`, `demo_rows_out.json` | environ 1 min |
