# Reçu J2 : coût par test du filtre des nœuds de l'arbre de boîtes, catalogue identique

29 septembre 2026. Étape J2 (lot A) du plan de performance du catalogue v10 (`gpu_design/juge/PLAN.md`, hors dépôt).
Travail sur une copie de `568d45297` dans `build/v10-j2`. Aucune branche, aucun commit, aucune commande GCP.

```text
phase=exploration_v10_hors_registre (etape J2, lot A)
backend=reference_cpu
profile=quantized_u18_input_only
mode=implementation_locale_mesuree (codespace EPYC 7763 Zen 3 AVX2, charge par d'autres sessions, 1 a 4 fils)
public_status=not_claimed
GCP non utilise
```

## 1. Résultat en bref

- **Exactitude.**
  - Dumps canoniques identiques octet pour octet à `568d45297` sur les 10 entrées du différentiel, à 1 et 4 fils,
    en build par défaut et en build `x86-64-v3`.
  - Niveaux exacts publiés (`cat.level`, que le dump n'écrit pas) identiques sur les 10 entrées.
  - Grand livre identique, hors deux compteurs retirés et deux compteurs nouveaux (§ 4).
  - 8 portes `gate` sur 8 vertes.
  - 3 mutants, tous tués (§ 5.4).
- **Filtre des nœuds seul** (tics TSC, 1 fil, build par défaut sans `-march`) :
  - ×3,4 à K = 5 et ×3,8 à K = 10, sur le quart 01 comme sur la trame 02 entière ;
  - avec l'option `MHGP10_MARCH=x86-64-v3` : ×4,5 à ×6,6.
  - Coût par test, tout le filtre compris : ~24 → ~8,7 tics TSC à K = 5, ~22 → ~6,7 à K = 10.
  - Le nombre de tests baisse aussi, de 11 à 20 %.
- **Étage des boîtes** (`catalogue_stages.t_boxes`, 1 fil, médiane de 3, exécutions alternées, § 6.1) :
  - K = 5 : ×1,62 à ×1,73 (trame 02 : 10,34 → 6,31 s) ;
  - K = 10 : ×1,45 à ×1,54 (trame 02 : 37,89 → 25,47 s) ;
  - en `x86-64-v3` : ×1,74 à ×1,88 à K = 5 et ×1,55 à ×1,66 à K = 10.

  Le reste de l'étage est surtout la feuille, qui n'est pas touchée.
- **Patch** : `build/v10-j2/J2.patch`, 4 fichiers, applicable par `git apply` à la racine du dépôt sur `568d45297`.
  - Vérifié sur une extraction propre : l'arbre patché est identique à la copie de travail, et il se reconstruit en
    un binaire bit à bit identique au binaire testé (`a3881a57…`).
  - Il s'applique aussi sur `c2049eb1c`, HEAD actuel du worktree : les trois commits suivants ne touchent pas ces
    fichiers.

## 2. Ce qui change

| id | contenu | retenu | effet mesuré sur le filtre (quart 01, rdtsc, 1 fil) |
| --- | --- | --- | --- |
| A1 | garde G supprimée comme test séparé (lemme 2) | oui | fait partie du passage à D-loc |
| A2, A3 | forme D-loc exacte en i64, comptage sans branchement (masque), Y en SoA sur la pile, boucle externe sur les sites | oui | cœur du gain : 2,42 → 0,80 Gtic (K = 5) en comptage uniforme, avec A4, A6 et `noinline` ; les autres lignes isolent chaque élément |
| A3′ | comptage par tranches : S0 d'abord, le reste de Y seulement si S0 n'exclut pas (lemme 3) | oui | −10 % (K = 5) et −11 % (K = 10) en build par défaut |
| A4 | réservoir par insertion dans un tableau fixe de 3K clés, à la place d'un `std::vector<pair>` + `nth_element` + tri par nœud | oui | −8 % (K = 5), −3 % (K = 10) |
| A5 | arène des listes par profondeur | **non** | 1 à 4 % du filtre, rien de mesurable sur t_boxes (±1,5 %) : la sortie sans branchement ne fait plus qu'une allocation par nœud (`resize`) |
| A6 | pré-ignorance par l'enveloppe de la liste parente (lemme 5) ; l'enveloppe voyage dans `Task::env` | oui | −15 % (K = 5), −13 % (K = 10) |
| A7 | option CMake `MHGP10_MARCH` (vide par défaut) : `-march=<valeur> -ffp-contract=off` | oui, désactivée par défaut | filtre −26 % (K = 5) à −42 % (K = 10) en `x86-64-v3` ; t_boxes −6 à −9 % |
| — | `[[gnu::noinline]]` sur `filter_node` : inline dans `process` (récursif, chargé), le noyau débordait ses registres sur la pile | oui | −13 % (K = 5), −16 % (K = 10) |

Fichiers du patch :

- `src/catalogue/generator.cpp` : filtre, `process`, `Task`, `Env`, `misses`, bornes ;
- `src/catalogue/catalogue.hpp` : grand livre ;
- `cli/mhgp10_catalogue.cpp` : JSON du grand livre ;
- `CMakeLists.txt` : option A7.

Écarts à la conception mesurée (`reduction_algorithmique_cpu`) :

- **Boucle externe sur les sites, Y interne.** Y (≤ 36 éléments) est sur la pile, à adresses fixes, et chaque site
  est rechargé depuis `C.X` : aucun tampon SoA par site, aucun tableau de compteurs. En scalaire et à inlining égal,
  cette forme bat de 30 % la forme « Y externe, sites internes » de la conception (0,93 contre 1,34 Gtic, quart 01,
  K = 5), que GCC compilait avec un branchement sur le résultat de chaque test.
- **Facteur 2h multiplié une fois par site**, plutôt qu'un décalage par test. Le cube n'a donc pas besoin d'être
  dyadique pour la forme D-loc, et il n'y a pas de `countr_zero`.
- **Sortie par tranches (A3′).** Elle récupère, sans branchement dans la tranche, l'effet de sortie rapide de
  l'ancienne garde. Elle vaut en build par défaut, qui est celui du worker G4. En `x86-64-v3`, le comptage uniforme
  serait meilleur : 10 à 15 % sur le filtre, 1 à 5 % sur t_boxes. On ne garde qu'une forme, sans code conditionné
  par l'ISA.
- **A5 non retenue** (mesure ci-dessus).

## 3. Lemmes utilisés, énoncés et preuves

**Notations.**

- Q = [lo, hi) est la boîte du nœud, cubique de côté h (toutes les boîtes le sont : racine cubique, découpe en huit
  moitiés), et Q̄ = [lo, hi] sa fermeture.
- P est la liste parente (indices de sites), X les coordonnées mises à l'échelle (X = x << 6), w ≥ 1 les poids.
- y domine x sur Q̄, noté y ≻ x, si et seulement si V(x, y) = max sur C ∈ Q̄ de (|Y − C|² − |X − C|²) < 0.
- Réservoir R : les c = min(|P|, 3K) premiers sites de P dans l'ordre lexicographique (dd, indice), avec
  dd(x) = |2X − (lo + hi)|².
- Y : plus petit préfixe de R de poids ≥ 3K, ou R entier. S0 : plus petit préfixe de R de poids ≥ K.
- Produit (568d45297) : x est gardé si et seulement si G(x) et non D(x), où
  - G(x) : il existe y ∈ S0 avec non (y ≻ x) ; si w(R) < K, le produit prend S0 = P entier ;
  - D(x) : W_Y(x) = Σ w(y) sur les y ∈ Y tels que y ≻ x vaut au moins K.
- J2 : x est gardé si et seulement si non D(x) (lemmes 2 et 3), avec V calculé sous la forme D-loc (lemme 1).

**Lemme 1 (forme D-loc, A2).** Avec x′ = X − lo, y′ = Y − lo et A(x) = |x′|² :

$$V(x, y) = A(y) - A(x) + \sum_{i} \max(0, 2h x'_i - 2h y'_i)$$

*Preuve.* Posons C = lo + c′ avec c′ ∈ [0, h]³. Alors
|Y − C|² − |X − C|² = A(y) − A(x) + 2 Σᵢ c′ᵢ (x′ᵢ − y′ᵢ). Chaque terme est affine en c′ᵢ, donc maximal à c′ᵢ = h si
x′ᵢ − y′ᵢ > 0 (valeur 2h (x′ᵢ − y′ᵢ)) et à c′ᵢ = 0 sinon (valeur 0). Comme 2h > 0,
2h max(0, x′ᵢ − y′ᵢ) = max(0, 2h x′ᵢ − 2h y′ᵢ). La forme par coins du produit calcule le même maximum,
|Y|² − |X|² − 2 Σᵢ cᵢ (Yᵢ − Xᵢ) avec le coin cᵢ qui minimise cᵢ (Yᵢ − Xᵢ). Les deux sont des calculs entiers exacts du
même réel : même entier, donc même décision y ≻ x ⟺ A(x) − A(y) > Σᵢ max(0, 2h x′ᵢ − 2h y′ᵢ). ∎

*Bornes* (B = 18 bits, T = 6, gravées par `static_assert(2 (B + T) + 5 <= 63)`).

- Xᵢ ∈ [0, 2^24), la racine a un côté au plus 2^24, donc |x′ᵢ| < 2^24 et 2h ≤ 2^25.
- |2h x′ᵢ| < 2^49 ; 2h (x′ᵢ − y′ᵢ) = 2h (Xᵢ − Yᵢ), donc |·| < 2^49 et le membre droit est < 3·2^49.
- A < 3·2^48.
- Clé du réservoir : |2x′ᵢ − h| < 3·2^24, donc dd < 27·2^48 < 2^53. C'est la plus grande quantité, d'où la garde
  27·2^(2(B+T)) < 2^63.
- Tout tient en i64, sans flottant.

**Lemme 2 (corollaire G ⊂ D, A1).** Pour tout x ∈ P : non G(x) ⟹ D(x). Donc G(x) et non D(x) ⟺ non D(x) : retirer
la garde ne change aucune liste.

*Preuve.* Deux cas.

- **w(R) ≥ K.** S0 et Y sont deux préfixes de la même suite triée R, avec K ≤ 3K : on a donc S0 ⊆ Y. En effet, si
  w(R) < 3K, alors Y = R ⊇ S0. Non G(x) signifie que tout y ∈ S0 domine x. Alors W_Y(x) ≥ w(S0) ≥ K, c'est-à-dire
  D(x).
- **w(R) < K.** Le produit prend S0 = P, qui contient x, et non (x ≻ x) puisque V(x, x) = 0 n'est pas < 0. G(x) est
  donc toujours vrai, et l'implication est vide. De plus, W_Y(x) ≤ w(R) < K : aucun des deux tests n'exclut rien.

∎

**Lemme 3 (sortie par tranches, A3′).**

- J2 calcule W₀ = Σ w(y) [y ≻ x] sur les s0 premiers éléments de Y, sans branchement.
- Si W₀ ≥ K, x est exclu. Sinon J2 ajoute la somme sur le reste de Y et exclut x si le total atteint K.
- Cette décision est exactement D(x).

*Preuve.* Tous les termes sont ≥ 0 : la somme partielle ne fait que croître, et W₀ ≥ K entraîne W_Y(x) ≥ K. Sinon
J2 calcule W_Y(x) en entier. ∎

La frontière s0 de J2 est le plus petit préfixe de Y de poids ≥ K (ou Y entier). Quand w(R) ≥ K, c'est le S0 du
produit. La tranche S0 fait donc, sans branchement, le travail d'exclusion de l'ancienne garde. L'exactitude ne
dépend pas de l'endroit de la coupe.

**Lemme 4 (réservoir par insertion, A4).** J2 obtient R, dans le même ordre, en gardant un tableau trié de c clés
(dd, rang dans P).

*Preuve.*

- Invariant : toute liste est une sous-suite strictement croissante de (0, …, n − 1). C'est vrai pour la liste de la
  racine. Le filtre J2 écrit les sites gardés dans l'ordre de la liste parente (compaction par rang croissant). Les
  copies partagées des tâches conservent l'ordre.
- Donc (dd, rang) et (dd, indice) donnent le même ordre.
- Chaque nouvelle clé a un rang supérieur à toutes les clés du tableau. Elle s'insère donc après les clés de même dd
  (décalage sur l'inégalité stricte). Si le tableau est plein, elle est rejetée si et seulement si
  dd ≥ dd(dernière), c'est-à-dire si elle est plus grande que la dernière dans l'ordre lexicographique.
- Le tableau contient à chaque instant les c plus petites clés vues, triées. C'est R, comme le `nth_element` suivi
  du tri du produit ; Y et S0 s'en déduisent de la même façon.
- Quand plusieurs clés ont même dd à la frontière de R ou de Y, c'est le départage par indice qui fixe Y. Il est
  identique ici : les listes sont identiques, et pas seulement « également valides ».

∎

**Lemme 5 (pré-ignorance par l'enveloppe parente, A6).** Soit E l'enveloppe fermée (min, max par axe) de la liste
L(parent) dont le nœud Q est filtré. S'il existe un axe avec E.hi < lo ou E.lo ≥ hi, alors le produit ignore Q
après filtrage (lemme K). Le classement est donc identique quand on ignore Q sans le filtrer.

*Preuve.*

- Le filtre ne fait que retirer des sites, donc L(Q) ⊆ L(parent).
- Si L(Q) est vide, le produit ignore Q.
- Sinon, max L(Q) ≤ E.hi < lo, ou min L(Q) ≥ E.lo ≥ hi, sur l'axe considéré : le test du lemme K (enveloppe de L(Q)
  hors de [lo, hi)) ignore Q.
- Dans les deux cas, le produit compte `nodes` et `skipped_bbox` et ne fait rien d'autre : ni enfant, ni feuille,
  ni stagnation propagée. J2 compte les mêmes, plus `preskipped_bbox`.

∎

*Déterminisme.*

- La racine reçoit une enveloppe neutre ; elle n'est jamais pré-ignorée.
- Chaque tâche de la frontière porte l'enveloppe de la liste de son parent (champ `Task::env`).
- La pré-ignorance est donc une fonction du seul arbre : même décision que le nœud soit traité dans la phase en
  largeur, comme racine de tâche ou en récursion. `preskipped_bbox` et `filter_tests` ne dépendent pas du nombre de
  fils (vérifié à 1 et 4 fils sur les 10 entrées).

**Conséquence.** On raisonne par récurrence sur la profondeur.

- Nœud filtré : à liste parente égale, les lemmes 1 à 4 donnent la même liste.
- Nœud pré-ignoré : le lemme 5 donne le même classement. Sa liste ne sert à rien : ni enfant, ni feuille.

Chaque nœud a donc le même classement et, s'il est filtré, la même liste que dans `568d45297`. L'arbre, les feuilles
et leurs listes sont identiques, et la feuille n'est pas modifiée. Les enregistrements et les niveaux sont donc
identiques, et la sortie canonique aussi. Rien ne dépend d'un flottant.

## 4. Grand livre : ce qui change de sens

Règle suivie : aucun compteur n'est redéfini. L'opération que comptaient deux compteurs n'existe plus ; ils sont
retirés, et deux compteurs nouveaux, de noms nouveaux, les remplacent.

| compteur (JSON de `mhgp10_catalogue`) | statut J2 |
| --- | --- |
| `nodes`, `leaves`, `sum_m`, `max_m`, `stalled_leaves` | inchangés, en sens et en valeur (10 entrées, 1 et 4 fils) |
| `leaf_dominance_tests`, `pair_tests`, `triple_tests`, `line_hits`, `quad_tests`, `judged`, `extended`, `weighted`, `max_shell` | inchangés (la feuille n'est pas touchée) ; `triple_tests`, seul compteur lu par `bench/scaling/scale_run.py`, garde son sens |
| `skipped_bbox` | inchangé en sens et en valeur ; il existait dans `CatalogueLedger`, il est désormais aussi imprimé. Valeur vérifiée par l'identité de l'arbre complet `skipped = nodes − leaves − (nodes − 1) / 8` contre la référence |
| `guard_tests` | **retiré** : la garde n'est plus évaluée comme test séparé (lemme 2) |
| `dominance_tests` | **retiré** : il comptait des tests à sortie anticipée test par test, forme qui n'existe plus |
| `filter_tests` | **nouveau** : tests de dominance D-loc effectués par le filtre des nœuds (tranche S0 pour chaque site de la liste parente, reste de Y pour ceux que S0 n'exclut pas ; nœuds pré-ignorés exclus) |
| `preskipped_bbox` | **nouveau** : nœuds ignorés par l'enveloppe parente sans filtrage (compris dans `skipped_bbox`) |

Un lecteur qui lit `guard_tests` ou `dominance_tests` échoue donc bruyamment (clé absente) au lieu de lire un nombre
dont le sens a changé. C'est le cas du script `differentiel.sh` du reçu J1, qui compare les clés `*tests` de l'ancien
JSON au nouveau. Aucune porte du dépôt ne lit ces compteurs : seul le CLI les imprime, et `scale_run.py` ne lit que
`triple_tests`. Ce reçu ne versionne pas le grand livre : `ledger_version` relève de J0, qui n'est pas fait.

Ordres de grandeur (différentiel) : `filter_tests` vaut 80 à 89 % de l'ancienne somme `guard_tests + dominance_tests`.
Par exemple, trame 02 à K = 5 : 490,7 M contre 591,7 M, avec 132 656 nœuds pré-ignorés sur 362 063 ignorés.

## 5. Exactitude

### 5.1 Différentiel des 10 entrées

- Script : `differentiel_j2.sh`. Mêmes entrées que le différentiel J1 : `lidar02_full`, `lidar00_full`,
  `lidar01_quarter_x_neg_y_neg`, `syn_shells_density_x2`, `syn_filaments_space_x4`, à K = 5 et 10.
- Référence : `build/v10-wt/mhgp10_catalogue`, à 3 fils. C'est le binaire `568d45297` ; son sha256 est identique à
  celui du build de la copie vierge, `433b4b97…`.
- J2 à 1 et à 4 fils. sha256 des dumps complets, écrits dans `/tmp` puis supprimés.
- Sorties : `differentiel_j2_final.txt` (build par défaut, binaire `a3881a57…`) et `differentiel_j2_v3.txt` (build
  `MHGP10_MARCH=x86-64-v3`, binaire `aad29bcb…`).

| entrée | K | dump (sha256, 16 premiers) | ref / J2 1 fil / J2 4 fils | v3 1 / 4 fils | grand livre |
| --- | ---: | --- | --- | --- | --- |
| lidar02_full | 5 | `8a850649ff103c1c` | identiques | identiques | identique |
| lidar02_full | 10 | `d6abe0dba4d9be33` | identiques | identiques | identique |
| lidar00_full | 5 | `3982c3ab79c4a542` | identiques | identiques | identique |
| lidar00_full | 10 | `7b2d4055f4cc7004` | identiques | identiques | identique |
| lidar01_quarter_x_neg_y_neg | 5 | `414aa4d47ffe1c55` | identiques | identiques | identique |
| lidar01_quarter_x_neg_y_neg | 10 | `7c46e50a72c08087` | identiques | identiques | identique |
| syn_shells_density_x2 | 5 | `fcd0425f8101b278` | identiques | identiques | identique |
| syn_shells_density_x2 | 10 | `8c145ca4ed47628a` | identiques | identiques | identique |
| syn_filaments_space_x4 | 5 | `e9408470b9317629` | identiques | identiques | identique |
| syn_filaments_space_x4 | 10 | `b9719e2166bb7c92` | identiques | identiques | identique |

Précisions sur les colonnes :

- Les 16 premiers caractères coïncident avec ceux du reçu J1.
- « grand livre identique » porte sur `balls`, `levels`, `by_q_p`, `nodes`, `leaves`, `sum_m`, `max_m`,
  `leaf_dominance_tests`, `pair_tests`, `triple_tests`, `line_hits`, `quad_tests`, `judged`, `extended`, `weighted`,
  `max_shell` et `stalled_leaves`, à 1 et 4 fils.
- Autres contrôles : `skipped_bbox` égal à l'identité de l'arbre complet ; `skipped_bbox`, `preskipped_bbox` et
  `filter_tests` égaux entre 1 et 4 fils.

Un premier passage a été fait avant le retrait de l'arène A5 et avant les retouches de forme (`differentiel_j2.txt`).
Ses sorties et celles des deux passages finaux sont identiques octet pour octet (sha256 `5757c4ae…` pour les trois
fichiers) : mêmes dumps, et mêmes compteurs `preskipped_bbox` et `filter_tests`.

### 5.2 Niveaux exacts (trou V4 du dump)

- Le dump n'écrit pas le niveau. La sonde hors dépôt `levelhash.cpp` calcule une empreinte FNV-1a de `cat.level`
  (signe et mots de num puis de den, en ordre de rang). Elle est compilée contre la bibliothèque de chaque version.
- Résultat : identique entre `568d45297` (4 fils) et J2 (4 fils et 1 fil) sur les 10 entrées (`niveaux_j2.txt`).
  Exemples : trame 02 à K = 5, 1 099 581 niveaux, `5d48e1de66ea7038` ; à K = 10, 4 908 695 niveaux,
  `6edfe520fd17e3b4`.

### 5.3 Portes

`ctest --test-dir build -L gate` : **8 sur 8 vertes** (`ctest_gate.txt`, 432 s).

- `mhgp10_catalogue_oracle` : 161 contrôles, 0 écart, 14 132 boules.
- `mhgp10_tower_oracle` : 73 contrôles, 0 écart.
- Les autres (unit, head vs sklearn, level_collision, points_cover, batch_equivalence, mreach_border) sont vertes.

La compilation passe sans avertissement sous `-Wall -Wextra -Wpedantic -Werror`, en build par défaut, en
`x86-64-v3` et sous sanitizers.

**ASan et UBSan** (`-DMHGP10_SANITIZE=ON`, `-fno-sanitize-recover=all`, 2 fils ; `sanitizers_j2.txt`).

- Entrées : `lidar02_quarter_x_nonneg_y_nonneg` (5 286 sites) et `syn_shells_density_x1` (8 000), à K = 5 et 10.
- Résultat : code 0, aucun message, dumps identiques à la référence (`4939fead5753116e`, `be5d19647049e109`,
  `6f903b08d471cad8`, `0bb0bc9c17a7f481`).

### 5.4 Mutants (hors patch)

Fautes : `mutants_j2.diff`, relatif au code J2 final ; copies construites dans `/tmp/j2work/mut{1,2,3}`. Juge :
`mutants_j2.sh`. Sorties : `mutants_j2.txt`.

| mutant | faute injectée | dumps (quart 01, syn_shells_x2 ; K = 5 et 10) | grand livre | oracle T2 | verdict |
| --- | --- | --- | --- | --- | --- |
| M1 « D retiré, garde seule » | seule la tranche S0 exclut ; le reste de Y n'est jamais compté (listes plus larges) | identiques (des listes plus larges restent sûres) | **différent** : nœuds 224 977 → 1 595 953 sur le quart à K = 5 | code 0 (0 écart) | **tué par le grand livre du différentiel** |
| M2 « facteur d'échelle » | `h2 = h` au lieu de `2h` dans D-loc (dominance trop large, listes trop étroites) | **différents** : 210 424 → 6 662 boules | **différent** | **code 1**, 110 écarts sur 161 | tué partout |
| M3 « bord bas de la pré-ignorance fermé » | `penv.hi <= lo` au lieu de `<` (ignorance à tort quand l'enveloppe parente touche la face basse) | identiques sur ces données LiDAR et synthétiques | **différent** : nœuds 224 977 → 224 913 | **code 1**, 45 écarts sur 161 (grilles coplanaires) | **tué par l'oracle T2 et par le grand livre** |

Leçons :

- **M1.** Il n'est vu que par le grand livre (nœuds, feuilles, Σm), comme le prévoyait la conception (« égalité des
  listes »). Comparer le grand livre fait donc partie de la porte.
- **M3.** Il reproduit M-C3 de la conception : même compte de 45 écarts à l'oracle. Une boule perdue exige un support
  entier sur un plan dyadique, ce qui arrive sur les grilles de l'oracle, pas sur ces nuages.

## 6. Mesures

### 6.1 Étage des boîtes, `catalogue_stages.t_boxes`

1 fil, 3 répétitions en alternant « avant », « après » et « après v3 » pour chaque entrée et chaque K ; médiane,
avec [min, max].

- « avant » : `568d45297`, Release, sans `-march`.
- « après » : J2, même build.
- « après v3 » : J2 avec `-DMHGP10_MARCH=x86-64-v3`.
- Données brutes : `mesure_j2.tsv` (colonnes : binaire, entrée, K, répétition, t_boxes, catalogue_s, t_assemble,
  boules).
- Machine partagée, charge moyenne 1,3 à 2,2 pendant la campagne : ce sont des ordres de grandeur. La mesure fiable
  se fera sur G4.

| entrée | K | sites | avant (s) | après (s) | gain | après v3 (s) | gain v3 | catalogue entier avant → après → v3 (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| trame 02 entière | 5 | 45 845 | 10,340 [10,321–10,518] | 6,313 [6,207–6,366] | ×1,64 | 5,830 [5,814–5,953] | ×1,77 | 11,141 → 7,014 → 6,552 |
| trame 02 entière | 10 | 45 845 | 37,894 [37,731–38,435] | 25,472 [25,436–26,140] | ×1,49 | 23,732 [23,726–24,323] | ×1,60 | 40,961 → 28,479 → 26,712 |
| trame 00 entière | 5 | 39 885 | 10,482 [10,451–10,671] | 6,468 [6,443–6,715] | ×1,62 | 6,035 [6,000–6,103] | ×1,74 | 11,197 → 7,140 → 6,726 |
| trame 00 entière | 10 | 39 885 | 39,161 [39,156–39,526] | 26,973 [26,854–27,066] | ×1,45 | 25,197 [25,113–25,468] | ×1,55 | 42,298 → 29,908 → 28,169 |
| quart 01 (`lidar01_quarter_x_neg_y_neg`) | 5 | 8 074 | 1,693 [1,688–1,711] | 0,980 [0,974–0,981] | ×1,73 | 0,900 [0,898–0,913] | ×1,88 | 1,797 → 1,082 → 0,995 |
| quart 01 (`lidar01_quarter_x_neg_y_neg`) | 10 | 8 074 | 6,032 [6,006–6,093] | 3,911 [3,907–3,933] | ×1,54 | 3,630 [3,601–3,637] | ×1,66 | 6,387 → 4,255 → 3,976 |

Lecture :

- L'étage des boîtes gagne ×1,62 à ×1,73 à K = 5 et ×1,45 à ×1,54 à K = 10 ; avec `x86-64-v3`, ×1,74 à ×1,88 et
  ×1,55 à ×1,66.
- Le filtre gagne ×3,4 à ×3,8 (§ 6.2). Le reste de l'étage, surtout la feuille (non touchée), fixe maintenant le
  temps.
- Nombre de boules identique d'une exécution à l'autre, pour chaque entrée et chaque K.
- Pour situer la trame 02 à K = 5 : la session G4 p2 mesurait 7,98 s de boîtes à 1 fil avant J2. Le codespace est
  ici environ 30 % plus lent, et ce facteur ne se transpose pas tel quel.

### 6.2 Filtre seul (tics TSC à 2,445 GHz, 1 fil)

- Sondes hors dépôt : `rdtsc` autour de `filter_node`, construites par `sonde_mkprobe.sh` depuis le code final,
  ses variantes d'ablation et `568d45297`. Banc alterné : `sonde_bench.sh`. Sorties : `sonde_rdtsc.txt`.
- Médiane de 5 répétitions (quart, K = 5), de 3 (quart, K = 10) et de 2 (trame 02).
- Le coût par test est le temps de tout le filtre (sélection, Y, noyau, sortie) divisé par le nombre de tests :
  `guard_tests + dominance_tests` pour la base, `filter_tests` pour J2.

| entrée | K | base (Gtic) | J2 (Gtic) | J2 v3 (Gtic) | base → J2 | base → J2 v3 | tics par test base / J2 / J2 v3 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| quart 01 | 5 | 2,424 | 0,722 | 0,533 | ×3,36 | ×4,55 | 24,0 / 8,9 / 6,6 |
| quart 01 | 10 | 6,913 | 1,811 | 1,050 | ×3,82 | ×6,58 | 21,6 / 6,7 / 3,9 |
| trame 02 | 5 | 14,199 | 4,199 | 3,167 | ×3,38 | ×4,48 | 24,0 / 8,6 / 6,5 |
| trame 02 | 10 | 40,408 | 10,493 | 6,151 | ×3,85 | ×6,57 | 22,1 / 6,7 / 3,9 |

Lecture :

- Le cœur tourne à ~3,26 GHz ; un tic TSC vaut donc ~1,33 cycle. La base coûte ~30 cycles par test, conforme aux
  « environ 33 cycles par test » du plan ; J2 ~9 à 12 en build par défaut, ~5 à 9 en `x86-64-v3`, tout le filtre
  compris.
- Le build de base compilé en `x86-64-v3` ne gagne rien (2,489 Gtic contre 2,424) : le gain `-march` vient de la
  forme sans branchement, que GCC vectorise en AVX2.
- Part du filtre dans t_boxes, trame 02, 1 fil : 55 % → 27 % à K = 5, 43 % → 17 % à K = 10.

### 6.3 Ablations (quart 01, Gtic du filtre, médiane)

| variante (code final moins un élément) | K = 5 | K = 10 |
| --- | ---: | ---: |
| J2 final | 0,722 | 1,811 |
| sans pré-ignorance (A6) | 0,848 | 2,081 |
| sans réservoir par insertion (A4 : vecteur de paires, `nth_element`, tri) | 0,783 | 1,873 |
| sans `noinline` | 0,834 | 2,155 |
| comptage uniforme sur Y, sans tranche S0 (A3′) | 0,799 | 2,034 |
| J2 final, `x86-64-v3` | 0,533 | 1,050 |
| comptage uniforme, `x86-64-v3` | 0,451 | 0,948 |
| arène par profondeur (A5), mesurée sur une variante antérieure avec arène : avec / sans | 0,687 / 0,693 | 1,754 / 1,819 |

### 6.4 Pour mémoire : l'assemblage

`t_assemble` à 1 fil, relevé dans la même campagne (médianes, avant / après) :

| entrée | K = 5 (s) | K = 10 (s) |
| --- | --- | --- |
| trame 02 | 0,455 / 0,446 | 1,974 / 1,973 |
| trame 00 | 0,409 / 0,412 | 2,023 / 1,983 |
| quart 01 | 0,057 / 0,064 | 0,251 / 0,241 |

L'assemblage n'est pas touché par J2 : ce sont toujours les grands tableaux en `std::vector` initialisés en série
(V9 du plan). À 1 fil, il pèse 7 % de l'étage des boîtes après J2 à K = 5 et 8 % à K = 10. Sa part grandit avec le
nombre de fils.

## 7. Risques et limites

1. **Transposition.** Les mesures viennent d'un codespace AMD EPYC 7763 (Zen 3, AVX2, sans AVX-512), chargé par
   d'autres sessions, avec g++ 13.3. La VM G4 a un EPYC 9B45 (Zen 5, AVX-512) et g++ 11.4. Les rapports peuvent y
   bouger de ±30 %. Les ordres de grandeur valent, pas les pourcentages.
2. **Build de G4.** Le worker configure avec le seul `-DCMAKE_BUILD_TYPE=Release`. Il obtient donc le noyau scalaire
   sans branchement, soit le gain « défaut » ci-dessus. Le gain `-march` ne vaut que si la session passe
   `-DMHGP10_MARCH=x86-64-v4` (ou v3).
   - La porte ISA x86-64-v4 n'a pas pu être jouée ici (pas d'AVX-512 sur ce codespace). Il faut la jouer sur G4 avant
     d'y utiliser l'option : même différentiel, dumps et grand livre.
3. **`[[gnu::noinline]]`.** L'attribut est reconnu par GCC et Clang. Son effet (−13 à −16 % sur le filtre) vient
   de l'allocation des registres dans `process`, récursive et chargée : il dépend du compilateur, pas de
   l'exactitude.
4. **Boîtes cubiques.** La forme D-loc prend le côté h de l'axe 0 pour les trois axes. C'était déjà le cas du calcul
   de stagnation. Une découpe non cubique (la découpe binaire, écartée par la conception CPU) exigerait un h par axe.
   Le mutant M2 montre qu'une erreur de facteur d'échelle est tuée.
5. **Sortie anticipée après S0.** C'est un branchement par site. Il est bien prédit parce que les listes suivent
   l'ordre de Morton : les sites gardés et exclus viennent par plages. Le gain de cette tranche est mesuré en scalaire
   (−10 à −11 % sur le filtre). En `x86-64-v3`, le comptage uniforme serait meilleur de 10 à 15 % sur le filtre et de
   1 à 5 % sur t_boxes. On garde une seule forme, sans code conditionné par l'ISA.
6. **Grand livre.** `guard_tests` et `dominance_tests` disparaissent du JSON. Toute comparaison de grands livres
   entre versions doit exclure ces deux clés et les deux nouvelles.
7. **Hors périmètre.**
   - Assemblage (V9 du plan : grands tableaux en `std::vector` initialisés en série) : non touché. Son temps à 1 fil
     est relevé au § 6.4 pour mémoire.
   - J0 (`ledger_version`, `--list-hash`) : non fait. L'égalité des listes n'est donc pas prouvée nœud par nœud par
     une empreinte. Elle l'est par les lemmes 1 à 5, et corroborée par le grand livre (nœuds, feuilles, Σm, m max
     identiques) et par les dumps.
   - Les fixtures F-DYA, F-TRI et F-L64 et le lot C (feuille) ne sont pas concernés.
8. **Après J2, l'étage des boîtes est dominé par la feuille.**
   - Trame 02 à 1 fil, build par défaut : le filtre ne pèse plus que ~27 % de t_boxes à K = 5 et ~17 % à K = 10.
   - La suite est J3 (feuille), puis, pour le filtre, une passe commune aux huit frères. La sélection du réservoir
     pèse maintenant de l'ordre de 25 à 30 % du filtre, d'après une mesure par étages d'une variante intermédiaire.

## 8. Reproduire

```bash
cd /workspaces/E-HGP/build/v9-open-worktree && git archive 568d45297 morsehgp3D_v10 | tar -x -C <dir>
cd <dir> && git apply /workspaces/E-HGP/build/v10-j2/J2.patch
cmake -S morsehgp3D_v10 -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel 4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir build -L gate
/workspaces/E-HGP/build/v10-j2/differentiel_j2.sh build/mhgp10_catalogue      # 10 entrees, 1 et 4 fils
/workspaces/E-HGP/build/v10-j2/mesure_j2.sh 3 out.tsv avant=<bin 568d45297> apres=build/mhgp10_catalogue
# option A7 : cmake ... -DMHGP10_MARCH=x86-64-v3 (ou x86-64-v4 sur G4, porte ISA a jouer la-bas)
```

Fichiers de ce reçu, tous dans `build/v10-j2/`, avec leurs empreintes dans `SHA256SUMS` :

- `J2.patch` ;
- `differentiel_j2.sh`, `differentiel_j2.txt`, `differentiel_j2_final.txt`, `differentiel_j2_v3.txt` ;
- `niveaux_j2.txt`, `levelhash.cpp` ;
- `ctest_gate.txt`, `sanitizers_j2.txt` ;
- `mutants_j2.diff`, `mutants_j2.sh`, `mutants_j2.txt` ;
- `mesure_j2.sh`, `mesure_j2.tsv`, `mesure_j2.txt` ;
- `sonde_mkprobe.sh`, `sonde_bench.sh`, `sonde_rdtsc.txt` ;
- binaires :
  - `mhgp10_catalogue.j2` (`a3881a57…`, J2 par défaut) ;
  - `mhgp10_catalogue.j2v3` (`aad29bcb…`, J2 en x86-64-v3) ;
  - `mhgp10_catalogue.j2c` (premier passage du différentiel, variante encore munie de l'arène A5) ;
- arbres :
  - `orig/` : copie vierge de `568d45297` ;
  - `morsehgp3D_v10/` : copie patchée ;
  - `build/`, `build-v3/`, `build-base/` : constructions Release, J2, J2 en v3 et `568d45297`.
