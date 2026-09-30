# Conception v10 — hiérarchies de points et têtes de clustering (CLUSTER_v2)

```text
phase=conception_v10_hors_registre
sous-systeme=hierarchies_de_points_et_tetes_de_clustering
backend=reference_cpu (tete O(n) sur CPU ; aucune voie GPU propre)
profile=quantized_u18_input_only
mode=benchmark_only pour les tetes ; exact_relative_to_tower pour la topologie des points
public_status=not_claimed
GCP non utilise (conception ; sondes locales nice -n 19, <= 1 fil)
```

Auteur : session de conception du 28 septembre 2026, révision de `CLUSTER_v1.md` après la critique adverse
(`design/crit_CLUSTER/`). Base lue : worktree `build/v9-open-worktree` à `ce8a649dd` ; lentilles L01, L04, L06–L15 ;
conceptions sœurs `ARCH_v1.md`, `arch_v2/part1.md` (brouillon ARCH_v2), `TOWER_v1.md`, `GEN_v1.md`, `EVAL_v1.md`.
Sondes nouvelles de cette révision : `design/cluster_v2/` (empreintes dans `cluster_v2/SHA256SUMS`). Tous les
chiffres de sonde sont des chiffres de **développement** : aucun n'est un résultat et aucun n'établit une pente.

**Préséance.** TOWER_v1 prévaut pour la tour et ses sorties ; EVAL_v1 prévaut pour le protocole de banc ; ce document
prévaut pour la définition des hiérarchies de points, des têtes et de la règle de choix du producteur. Toute
divergence est déclarée à l'endroit où elle apparaît.

---

## 0. Résumé exécutif

**L'objet ne change pas.** Pour chaque ordre K, la hiérarchie de points canonique C∩X : x entre dans $L_K$ au niveau
$D_K(x)$ (carré de la distance à son K-ième voisin, x et ses copies compris) et appartient à la composante de
$L_K(a)$ qui le contient. Partitions emboîtées en a (C2) et en K (C3). Distance-cœur d'HDBSCAN avec `min_samples = K`,
mais connexité de la multicouverture exacte.

**Ce qui change dans la v2.**

1. **Construction par graphe-chemin (nouvelle, prouvée, vérifiée).** Le dendrogramme de points d'un ordre se lit sur
   l'index d'intervalles que la tour publie déjà (ordre des feuilles, jonctions `jrank`, `entry_node`, TOWER § 4.7
   et § 5.6). On trie les points par position de feuille, on prend le maximum des jonctions entre voisins, puis un
   Kruskal à n − 1 arêtes et n activations. Coût $O(n\log n+L_K)$, **indépendant des $H_K$ nœuds** et de
   l'algorithme de T2 (séquentiel ou Borůvka P13). La « voie fusionnée » de la v1 disparaît. Vérification :
   520 nuages (250 sur grilles dégénérées), 1 319 ordres, 36 044 coupes fermées, 4 variantes, 0 écart contre l'oracle
   indépendant du critique, branché sur le prototype de TOWER (§ 4.1).
2. **Témoins E1 corrigés (B1, M1).** Deux atteignabilités mutuelles exactes, α = 1 (HDBSCAN standard) et α = 2
   (rayon $\max(r_K(x),r_K(y),d/2)$, **égale à la tour à K = 1**). La géométrie n'est revendiquée que si la tour bat
   les deux (test d'intersection-union), à K = 2 et à K = 3. À K = 2, l'atteignabilité α = 1 est la liaison simple
   (lemme prouvé). α entre dans la grille de `hdb_dev` (budget symétrique).
3. **Condensation N-aire déclarée divergente de sklearn (B2).** Lemme des plateaux sûrs, prouvé : si, pour chaque
   plateau, la masse hors plus gros item est < mcs, toute binarisation donne la même condensation. Sinon, la porte
   compare à une référence N-aire. Fait aggravant, mesuré : sur la fixture `line5` (5 points alignés), sklearn n'est
   **pas équivariant par permutation**. Le point du milieu rejoint l'amas de gauche ou de droite selon l'ordre
   d'entrée ; la condensation N-aire le rend bruit de façon canonique.
4. **Contrat aligné sur TOWER (M3, M4).** Aucun index global de clés, aucun `find_ball`. Entrées C∩X lues dans
   l'étage Q de la tour. $N_K(x)$ est un **multiensemble**, départagé par `SiteIdx`. K = 1 et les atomes lourds sont
   définis, avec un plancher λ fini. Les entrées à doublons suivent la livraison par étapes d'ARCH_v2 (refus
   `duplicate_positions` côté tour tant que la porte pondérée n'est pas verte ; MR les traite nativement).
5. **Régime LiDAR (M2).**
   - Sur la tour du contrat (K5 ou K10), la tête coûte, estimé, ≈ 3–5 ms de mur à K5 et ≈ 4–8 ms à K10 sur G4
     W48, tous ordres compris. C'est ≤ 4 % du total estimé (catalogue + tour) : le goulot reste la tour.
   - La voie « clustering seul », tour à $K_{\max}=K_P$, porte **301 745 boules à K2** sur 08/000200. Ce chiffre
     est dérivé exactement du reçu L13 : 4,7 fois moins qu'à K5 (1 407 885). Elle est plausible sous 100 ms
     seulement avec le GPU du catalogue, ou avec une constante CPU ≤ ≈ 6 µs/boule (hypothèse ×26 d'ARCH).
   - MR coûte 10 à 100 fois moins que la tour la moins chère.
   - L'ordre de la tête LiDAR est $K_P$, gelé sur `dev`.

**Verdict sur la critique.** Les 2 bloquants et les 7 majeurs sont fondés ; B2 est aggravé. Les 13 constats mineurs
sont fondés, dont un favorable (descentes moins coûteuses) ; le quatorzième est un constat de correction. Les preuves
C1–C5, le résolveur et les valeurs FULL des fixtures sont confirmés par la critique. Réponses point par point : § 1.

**Règle de choix, sans complaisance (§ 13).**

- La tour n'est désignée comme producteur du clustering que si elle bat **les deux** témoins MR_α **et**
  `hdb_dev`.
- À égalité, le producteur retenu est le moins cher : MR. La tour reste alors le producteur de la topologie exacte
  (LiDAR, Zoltan).
- Pilote de développement (§ 13.6 ; K = 2, n = 400, 64 nuages, vraie tour C∩X par Γ₂) : la tour et MR_2 sont
  indiscernables, |Δ ARI_s| ≤ 0,009 dans les 6 cellules, et l'écart tour − HDBSCAN standard y est l'effet α.
- Prévision écrite d'avance, qui en découle : c'est l'issue « non-infériorité » qui est attendue. La v10 battrait
  alors HDBSCAN standard par sa **tête** (échelle ẑ, mcs = √n, politique de bruit), pas par la géométrie de la
  tour. Le document de résultat devra le dire en première phrase.

---

## 1. Réponses à la critique

### 1.1 Bloquants et majeurs

| Id | Verdict | Vérification faite ici | Correction (section) |
| --- | --- | --- | --- |
| B1 — E1 confond la géométrie et le rapport d'échelle entrée/fusion | **fondé** | CSV du critique resynthétisé : Δ(α2 − α1) de −0,077 (K5, `none`, 0 %, 0-16) à +0,075 (K5, `bounded2`, 30 %, 16-0) ; signe inversé par la politique de bruit | témoins MR_1 **et** MR_2, lemme d'identité tour ≡ MR_2 à K = 1, C5 pour α = 2 (§ 3.3) ; H3 en intersection-union sur α, à K ∈ {2, 3}, publiée par politique de bruit ; α dans la grille de `hdb_dev` ; P1 refondée sur un pilote réel (§ 13) |
| B2 — G2 fausse dès K ≥ 3 (plateaux structurels) | **fondé, aggravé** | fixture `line5` : sklearn 1.9.1 (brute, kd_tree, ball_tree) rattache x₂ à gauche ou à droite selon l'ordre d'entrée ; N-aire : x₂ bruit, identique sous les 3 permutations (`cluster_v2/nary_plateau_fixture.py`) | G2 contre une référence N-aire exacte sur les mêmes coordonnées entières ; comparaison sklearn seulement sous le lemme des plateaux sûrs (§ 5.3), cas comptés ; divergence déclarée (§ 5) |
| M1 — K = 2 trivial pour MR | **fondé** | preuve en deux lignes (§ 3.3, lemme L2) | T1-MR_1 à K = 2 déclarée « liaison simple + tête » ; E1 confirmatoire à K = 2 **et** K = 3 ; chiffres L14 « ms = 2 » renommés ms = 1 |
| M2 — LiDAR : voie fusionnée absente d'ARCH, incompatible avec P13, tour omise, ordre non fixé, voie K2 non analysée | **fondé** | TOWER § 5.8 publie déjà `entry_node`/`entry_level` et l'index d'intervalles ; tailles de catalogue par $K_{\max}$ dérivées du reçu L13 (`cluster_v2/catalogue_by_kmax.log`) | voie fusionnée **supprimée** ; construction par graphe-chemin, indépendante de l'algorithme T2 (§ 4.1) ; tableau LiDAR tour + tête (§ 11.1) ; ordre de tête = $K_P$ ; voie « clustering seul » chiffrée (§ 11.2) |
| M3 — TowerView ≠ ARCH/TOWER (clé, index, comparaisons exactes, R5) | **fondé** | lecture de TOWER § 4.7, § 5.6, § 5.8 et ARCH_v2 § 5.2–5.4 | `TowerOrderView` = sous-ensemble en lecture seule d'`OrderForest` ; aucune clé ni index global ; `entry_pos`/`entry_eq` demandés à l'étage Q ; clés unifiées entières ; doubles de niveau recalculés pour ≤ n rangs par ordre ; départage par `SiteIdx` (§ 4.1, § 10.3) |
| M4 — K = 1, μ ≥ K, N_K sur sites distincts, λ(0) = +∞ | **fondé** | TOWER § 2.3 et § 2.9 (boules de rayon nul, règle « une seule position ») | $N_K(x)$ multiensemble ; K = 1 et μ ≥ K : entrée au rang 0 via la boule de rayon nul ; plancher $r_{\mathrm{floor}}$ (§ 3.1, § 6.1) ; fixture `heavy_atom` |
| M5 — la parcimonie favorise la tour | **fondé** | — | alignement sur EVAL § 7.2 (P parmi les configurations de tour) ; décision du producteur par coût : à égalité, MR (§ 13.4) |
| M6 — OP11 surqualifié (« deux racines ») | **fondé** | oracle du critique rejoué (`crit_CLUSTER/check_fixtures.py`) : repli « mutant » = `0123 \| 4` à 54 ; FULL = une racine | OP11 restreint : contradiction informative = fusion retardée de `cx_fold_k2_n6` (190/7 → 55/2) ; `cx_fold_k3_n5` ne sert qu'à tuer le mutant exact `gabriel_fold_topology` (§ 3.5, § 14) |
| M7 — E0 échouera pour cause de quantification | **fondé** | `crit_CLUSTER/quant_e0.py` : 3/24 écarts flottant brut contre grille | E0 devient un diagnostic ; la porte est G2 : C++ ≡ référence Python N-aire **sur les mêmes coordonnées u18** ; nuages G1 sur toute l'étendue $2^{18}$, sans filtre « sans ex æquo » (§ 12) |

### 1.2 Mineurs

| Constat | Verdict | Correction |
| --- | --- | --- |
| Reçu v1 : 360 nuages et 22 684 niveaux (pas 420 et ≈ 26 000), graines non journalisées, `checks_T1: 0` trompeur, T4b testé aux seuls niveaux de Π | fondé | chiffres corrigés (§ 3.4) ; la campagne v1 est déclarée **non rejouable** ; la campagne v2 journalise commande, graine et boîte ; G3 teste C5 à l'union des niveaux critiques de Π et de MR_α (§ 12.1) |
| A priori `bounded(1,5)` contredit par les données et la règle | fondé | recalcul : `bounded(1)` 0,8106 > `none` 0,8067 > `bounded(1,5)` 0,8064 > `bounded(2)` 0,7787 > `full` 0,7529 ; cellules bruitées = 5 exécutions sphériques ; **aucun a priori** ; choix par EVAL § 7.2 sur `dev` (§ 8) |
| C5 : zone aveugle composée = facteur 3 en rayon | fondé | énoncé harmonisé ; ajout du différentiel balayage ≡ chemin et du témoin BFS borné (§ 12.4) ; limites dites |
| Descentes : 1,0–1,4 MEB par point | fondé (favorable) | les descentes relèvent désormais de l'étage G de la tour ; chiffre du critique repris au § 11 |
| T2 : clé de lot non comparable entre ordres ; « max(d, s_lo) » | fondé | clés unifiées globales (rangs de la tour communs à tous les ordres, table globale des $D_k$) ; texte corrigé en « max(w, s_lo) » (§ 4.1.3, § 9.2) |
| `long double` non portable | fondé | plus de `long double` ; λ en binary64 pur, `det_log2`/`det_exp2` maison, ẑ arrondi au 1/16 (§ 6.2) |
| ẑ avec distances nulles | fondé | ẑ sur les **sites distincts** (distance ≥ 1 unité de grille), plancher inutile (§ 6.3) |
| Sélection epsilon : `>` strict chez sklearn | fondé | sémantique exacte d'`epsilon_search`/`traverse_upwards` reprise (§ 7) |
| CAS sur clé 128 bits dans Borůvka | fondé | deux passes `atomic_min` u64 (poids, puis arête empaquetée) ; aucun `__int128` atomique (§ 4.3) |
| `uniform` avec racine exclue : cible inatteignable | fondé | famille N1 d'EVAL (`uniform`) sortie de la moyenne confirmatoire de ce sous-système ; publiée avec `asc` vrai pour toutes les méthodes (§ 13.2) |
| Fixtures incomplètes (`entry_equal_level`, `parent_split_ce8a`) | fondé | `entry_equal_level` construite (`line5`, K = 2, nœud unique à 16) et **nécessaire** : 550 nuages aléatoires dégénérés ne tuent pas le mutant ; `parent_split` réécrite en dendrogramme abstrait à valeurs exactes (§ 12.2) |
| Vote T3 : égalité exacte de sommes binary64 | fondé | abstention sur quasi-égalité bornée par Neumaier, comptée (§ 4.4) |
| Coût de campagne non estimé | fondé | ≈ 4–5 h W8 en local, ≈ 30 min sur G4, estimés (§ 11.4) |
| « Vérifié correct, pour mémoire » | — | pris acte ; aucune modification |
| (implicite) sklearn `alpha` dépend de l'algorithme | fondé (sonde EVAL v2) | MR_α n'est jamais délégué à sklearn : producteur exact maison (§ 4.3) |

---

## 2. Contraintes héritées de l'audit (inchangées sauf mention)

| Constat (lentille) | Contrainte |
|---|---|
| Clustering du 28 sept. = repli des cofaces de Gabriel (E5, `false_in_general`) | toute tête consomme la forêt FULL ; porte « racine unique » et fixtures de repli sur tout consommateur |
| Descente « aucun enfant gros » (L08-F1, L11-F2) | condensation exacte, sortie au niveau de scission |
| Racines non seuillées (L08-F4, F9) | racine unique exigée ; racine exclue par défaut |
| Plateaux perdus (L08-F5) | plateaux N-aires atomiques, jamais binarisés (§ 5) |
| Oracle HDBSCAN à un paramètre ; ARI « bruit = classe » (L09, L14) | adversaires d'EVAL (`hdb_dev` décisif) ; ARI_s primaire |
| Réglages après les scores (L09-F7) | préenregistrement EVAL |
| Convention K ↔ min_samples (L14-F11) | K = `min_samples` sklearn (le point compte) |
| Chaîne Python/Fraction à 32k (L07, L08-F6) | C++20, une construction pour toutes les variantes |

Jetés : graphe de Gabriel comme topologie ; convention « gabriel » des facettes ; descente « tous petits » ; feuilles
par défaut ; échelle `log` ; revendication `cda636b5e` ; « monter K fragmente » (artefact E5) ; **voie fusionnée
de la v1** ; **résolveur propre à la tête** (celui de la tour sert) ; **a priori de politique de bruit**.

---

## 3. Objets et énoncés

### 3.1 Notations

- $X$ : sites distincts de $\lbrack0,2^{18})^{3}\cap\mathbb{Z}^{3}$, en ordre de Morton (`SiteIdx`), de poids
  $\mu_x\geq1$. Le multiensemble $\widetilde{X}$ contient $\mu_x$ copies de x. `PointId` n'est lu que par `api/`.
- Niveaux en **rayon carré** $a=r^{2}$ ; $L_K(a)=\lbrace y:\sum_{s:\lvert y-s\rvert^{2}\leq a}\mu_s\geq K\rbrace$.
- $d^{2}(x,y)\in\mathbb{N}$, $d^{2}<3\cdot2^{36}<2^{38}$, exact en u64 et en binary64.
- $N_K(x)$ : les K premiers éléments de $\widetilde{X}$ dans l'ordre lexicographique $(d^{2}(x,\cdot),\mathrm{SiteIdx})$,
  **copies comprises**. C'est un K-multiensemble. $D_K(x)$ est le carré de la distance de son K-ième élément.
  - Si $\mu_x\geq K$, alors $N_K(x)$ est formé de K copies de x et $D_K(x)=0$.
  - C'est la définition de TOWER § 5.6 ; la v1 prenait K sites distincts (défaut M4).
- $r_K(x)=\sqrt{D_K(x)}$. $X_K(a)=\lbrace x:D_K(x)\leq a\rbrace$ : points actifs.
- $T_K$ : forêt FULL d'ordre K (TOWER § 4.7), une racine, feuilles ordonnées en ordre de parcours ; l'intervalle
  du nœud v est $\lbrack\mathrm{lo}(v),\mathrm{lo}(v)+\mathrm{len}(v))$ ; `jrank[i]` est le rang du nœud qui crée la
  jonction entre les positions i et i+1.
- **Unités de rayon par producteur** : tour = rayon de boule ; MR_α = $\max(r_K(x),r_K(y),d/\alpha)$.
- **Plancher** $r_{\mathrm{floor}}$ : plus petit rayon strictement positif que le producteur peut produire sur la
  grille unité. Il vaut 1/2 pour la tour (deux sites à distance 1), $1/\alpha$ pour MR_α. Il ne sert qu'à dater
  la sortie d'un atome entré au niveau 0 (§ 6.1).

### 3.2 La hiérarchie C∩X

**Définition.** $\Pi_K(a)$ partitionne $X_K(a)$ par les composantes connexes de $L_K(a)$. La famille
$(\Pi_K(a))_{a\geq0}$ est la hiérarchie C∩X d'ordre K.

**Divergence déclarée avec la thèse.** Les amas discrets de la Déf. 8 se recouvrent dès K ≥ 2 ; C∩X est une
partition. Les amas discrets restent disponibles comme appartenance douce (§ 8.3), jamais comme hiérarchie.

**Amendement du 29 septembre 2026** (audit de la hiérarchie K-NN, `audits/audit_hierarchie_knn_20260929/`). La tête
v10-b du lot C se sert des amas discrets **comme entrée** d'une hiérarchie de points. L'entrée `cover` rattache
chaque point à une seule composante, celle de sa première boule couvrante (rayon $\alpha_K(x)$). Chaque niveau reste
donc une partition. Les amas discrets eux-mêmes, qui se recouvrent, ne sont toujours pas publiés comme hiérarchie :
« jamais comme hiérarchie » ne vaut plus que pour eux, en tant qu'ensembles. En position générale, la première boule
couvrante est une naissance d'ordre K. Une tête qui condense avec mcs > K ne lit donc pas $\alpha_K(x)$ : le point
sort à la fusion qui absorbe cette naissance (`src/tower/tower.hpp`).

**Second amendement du 29 septembre 2026** (audit continu, `audits/audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md`
§ 2 et § 4.2 ; registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, section V10). L'entrée `cover` n'est pas
« la sémantique des amas discrets » : elle en est une **laminarisation particulière**, avec perte d'appartenances.

- Au même niveau, les couvertures $D_r(C)=\lbrace x\in X:d(x,C)\leq r\rbrace$ de deux composantes peuvent se
  recouvrir. Fixture F1 : K = 2, sites alignés 0, 2, 4, rayon 1 (a = 1) ; composantes {1} et {3}, couvertures {0, 2} et
  {2, 4}. La première couverture donne le point 2 à un seul propriétaire, par l'ordre canonique du catalogue ; le
  binaire publie {0, 2} et {4}. Aucune partition ne contient les deux amas discrets : laminarité, fidélité à toutes
  les couvertures et affectation exclusive immédiate ne peuvent être exigées ensemble. L'énoncé contraire est
  `false_in_general`.
- Cette laminarisation est discontinue. Fixture F2 : K = 2, {0, 999, 2000} puis {0, 1001, 2000} ; la fusion
  gauche–milieu passe du rayon 499,5 au rayon 1 000 en `cover` (saut de 500,5 pour un déplacement de 2), de 999 à
  1 001 en `core`. `core` vérifie une borne de stabilité : un déplacement apparié d'au plus ε change ses rayons de
  fusion et d'entrée d'au plus 2ε (`proved_here` ; cas d'égalité gravé en F4 : 0, 1 puis −ε, 1 + ε). `cover` n'en
  hérite pas (`false_in_general`).
- Plusieurs ordres ne forment pas un arbre unique sur les points (F3, `false_in_general`) : sur 0, 20, 22, 50, 52, la
  coupe (K = 1, r = 10) contient {0, 20, 22} et la coupe (K = 2, r = 15) contient {20, 22, 50, 52}. Les tranches
  monotones (r croissant, K décroissant) restent emboîtées (`proved_here`) ; hors d'elles, rien ne garantit
  l'emboîtement, et une tête multi-K déclare sa règle de laminarisation.
- Les quatre faits sont lus sur les binaires natifs par la porte `mhgp10_regression_projection_facts`
  (`tests/regression/test_projection_facts.py`). « Jamais comme hiérarchie » vaut toujours pour les amas discrets en
  tant qu'ensembles. `core` est la restriction exacte et stable $C\cap X$, mais pas la cible des amas discrets de la
  Déf. 8 : $C\cap X\subseteq D_r(C)$, et core en retarde les points frontière (deux sites à distance d, K = 2 : couverts
  dès d/2, entrés en core à d). `cover` préserve cette participation précoce, au prix d'un propriétaire unique et de
  la discontinuité F2. Aucune de ces fixtures ne mesure une instabilité des sorties LiDAR : une condensation avec
  mcs > K peut effacer ces petites branches.

### 3.3 Énoncés (statut visé `proved_here`, § 14)

- **(C1) Rattachement.** Pour $a\geq D_K(x)$, x est dans la composante de $L_K(a)$ qui contient la région témoin
  $T_a(N_K(x))=\bigcap_{f\in N_K(x)}\bar{B}(f,\sqrt{a})$, qui est convexe, non vide et contient x. Toute K-partie F
  avec $\beta(F)\leq a$ et $\max_{f\in F}d^{2}(x,f)\leq a$ donne la même composante. Le départage des ex æquo est
  donc sans effet sur la partition. Preuve : v1 § 2.2, confirmée par la critique.
- **(C2, C3) Emboîtements.** $L_K(a)\subseteq L_K(b)$ pour $a\leq b$ ; $L_{K+1}(a)\subseteq L_K(a)$ et
  $D_{K+1}\geq D_K$. Donc $\Pi_K(a)$ raffine $\Pi_K(b)$ restreinte, et $\Pi_{K+1}(a)$ raffine $\Pi_K(a)$ restreinte.
- **(C4) K = 1.** $D_1\equiv0$ ; $\Pi_1(a)$ est la liaison simple au seuil de distance $2\sqrt{a}$.
- **(C5, α = 1).** $\mathrm{MR}^{1}_K(\varepsilon)$ partitionne les actifs ($r_K\leq\varepsilon$) par le graphe
  $\max(r_K(x),r_K(y),d(x,y))\leq\varepsilon$. Alors $\Pi_K(r^{2})\sqsubseteq\mathrm{MR}^{1}_K(2r)$ et
  $\mathrm{MR}^{1}_K(\varepsilon)\sqsubseteq\Pi_K(9\varepsilon^{2}/4)$. La zone aveugle composée vaut un facteur 3 en
  rayon. Preuve : v1 § 2.2, confirmée ; un contre-exemple 1D montre que $\Pi_K(\varepsilon^{2})$ ne suffit pas.
- **(C5bis, α = 2), nouveau.** $\mathrm{MR}^{2}_K(\varepsilon)$ utilise le graphe
  $\max(r_K(x),r_K(y),d(x,y)/2)\leq\varepsilon$. Alors $\Pi_K(r^{2})\sqsubseteq\mathrm{MR}^{2}_K(2r)$ et
  $\mathrm{MR}^{2}_K(\varepsilon)\sqsubseteq\Pi_K(4\varepsilon^{2})$.
  - *Preuve, premier sens.* Même chaîne que C5 : sites d'une même boule de rayon r, deux à deux à distance ≤ 2r,
    donc $d/2\leq r$ ; distances-cœurs ≤ 2r.
  - *Preuve, second sens.* Si $d(x,y)\leq2\varepsilon$ et $r_K(x),r_K(y)\leq\varepsilon$, tout z de [x, y] est à
    distance ≤ ε de x ou de y ; alors $\bar{B}(z,2\varepsilon)$ contient une boule de rayon ε centrée en x ou en y,
    qui contient K sites.
- **(L1) Identité à K = 1.** $\mathrm{MR}^{2}_1(\varepsilon)=\Pi_1(\varepsilon^{2})$ pour tout ε : entrées à 0 et
  fusion à d/2 des deux côtés. Donc la tour et MR_2 ont le **même dendrogramme** à K = 1. MR_1 a le même, avec des
  rayons doublés. Pour tout α, l'EOM est la même à K = 1 (λ multiplié par une constante).
  - Mesure (sonde EVAL v2, `eval_v2_probe/interleave_check.log`, n = 36, 6 graines) : rayon de fusion tour sur MR_2
    ≤ 1,26 (K = 2) et ≤ 1,35 (K = 3) ; MR_2 sur tour ≤ 1,11 ; fusions égales pour 46 % (K = 2) et 63 % (K = 3) des
    paires ; 100 % à K = 1.
- **(L2) MR_1 est trivial à K = 2.** $D_2(x)=\min_{y\neq x}d^{2}(x,y)$ (ou 0 si $\mu_x\geq2$). Donc
  $D_2(x)\leq d^{2}(x,y)$ et $D_2(y)\leq d^{2}(x,y)$ pour tout couple, d'où
  $\max(D_2(x),D_2(y),d^{2}(x,y))=d^{2}(x,y)$.
  - $\mathrm{MR}^{1}_2$ est la liaison simple, et HDBSCAN(ms = 2) a les mêmes étiquettes que HDBSCAN(ms = 1).
  - $\mathrm{MR}^{2}_2$ n'est **pas** trivial : $d^{2}(x,\mathrm{NN}(x))<4D_2(x)$.
  - Mesuré par le critique : mêmes comptes d'ex æquo et de multifusions à K = 1 et K = 2 sur 16/16 nuages.

### 3.4 Vérification (reçus)

- **Campagne v1 (C1–C5)** : `proto_cx/campaign.log` = **360 nuages**, **22 684 niveaux critiques**, 0 écart.
  - Graines et boîtes non journalisées : campagne **non rejouable** telle quelle.
  - Le compteur `checks_T1` est mort.
  - C5 n'y est testé qu'aux niveaux critiques de Π.
  - Elle est remplacée par la porte G3 (§ 12.1), dont les commandes sont journalisées.
- **Campagne v2 du graphe-chemin** (`cluster_v2/pathgraph_campaign.log` et premier lot) : 0 écart.
  - Volume : 520 nuages, 1 319 ordres, 36 044 coupes fermées, 4 variantes chacune (deux départages de $N_K$, deux
    choix de feuille).
  - Oracle : `crit_CLUSTER/my_oracle.py`, indépendant (MEB par balayage des supports, Γ_K par unions).
  - Tour jugée : prototype TOWER `tower_v10_proto_v2.py` (mêmes structures que TOWER § 4.7).
  - Boîtes 6, 11, 12 (270 nuages) et grilles de côté 2 et 3 (250 nuages).
- **Γ₂ du pilote E1** : 60 nuages et 338 coupes contre le même oracle (`cluster_v2/e1_pilot_k2.py selftest 60`).

### 3.5 Réfutations et contradictions (fixtures au § 12.2)

- **Raccourci de première couverture** : faux (v1, 32 écarts sur 3 874). Fixture `cx_firstcov_k3_n6`.
- **Repli des cofaces de Gabriel** : il change C∩X.
  - Contradiction informative : `cx_fold_k2_n6`, où {0,1,2} ∪ {4,5} fusionnent à 190/7 en FULL contre 55/2 sous le
    repli. Valeurs confirmées par l'oracle du critique.
  - `cx_fold_k3_n5` n'établit « deux racines » que pour le **mutant exact** `gabriel_fold_topology` : sommets = toutes
    les K-parties, arêtes = cofaces de Gabriel, points par $N_K(x)$.
  - Dans le repli réel de la v9 (atomes = ∂C, vote), x₄ rejoint la racine (critique, M6).
  - La fixture sert à tuer ce mutant ; elle ne dit rien d'autre.
- **Transport des unions-chemins d'un ordre à l'autre** : faux, et c'est nouveau.
  - Deux points actifs à l'ordre k peuvent être reliés par des arêtes-chemins d'ordre k+1 de poids ≤ a, via des
    points inactifs, sans être dans la même composante de $\Pi_k(a)$.
  - Contre-exemple à 5 points, a = 11/4 (`cluster_v2/phantom_transfer.log`, graine 2004).
  - Conséquence : la tête multi-ordres T2 compose les dendrogrammes $P_k$ et jamais les arêtes-chemins (§ 9.2).
    Fixture `phantom_transfer_n5`.

---

## 4. Producteurs de hiérarchies de points

### 4.1 Tour → dendrogramme de points par graphe-chemin

#### 4.1.1 Théorème PG (graphe-chemin)

**Hypothèses.**
- $T_K$ a une racine unique. Ses feuilles sont rangées dans l'ordre de TOWER § 5.5, où chaque sous-arbre est un
  intervalle.
- Propriété d'index d'intervalles (TOWER § 5.6, requête WA) : deux feuilles de positions p < q sont dans la même
  composante de la coupe fermée a si et seulement si $\max_{p\leq i<q}\mathrm{jrank}\lbrack i\rbrack\leq a$.
- Pour chaque point x, on choisit une position $p_x$ **dans le sous-arbre** de $t_x$ = `entry_node[x]`, nœud de
  $T_K$ vivant à la coupe fermée $D_K(x)$ qui contient la composante de $N_K(x)$. On prend $p_x=\mathrm{lo}(t_x)$.

**Construction.**
- On trie les points par $p_x$, avec départage par `SiteIdx` : $x_1,\ldots,x_n$.
- On pose $w_m=\max_{p_{x_m}\leq i<p_{x_{m+1}}}\mathrm{jrank}\lbrack i\rbrack$, ou $w_m=\bot$ si les deux
  positions sont égales.

**Énoncé.** Pour tout a et tous x, y actifs ($D_K\leq a$) avec x avant y :
x ∼ y dans $\Pi_K(a)$ ⟺ $\max_{i(x)\leq m<i(y)}w_m\leq a$.

**Preuve.**
- Pour $a\geq D_K(x)$, la composante de x est $\mathrm{WA}(t_x,a)$ par C1 et C2.
- La feuille en position $p_x$ est dans le sous-arbre de $t_x$. Son ancêtre vivant à a est donc aussi celui de
  $t_x$.
- Donc x ∼ y ⟺ les feuilles $p_x$ et $p_y$ sont dans la même composante à a ⟺ $\max\mathrm{jrank}\lbrack p_x..p_y-1\rbrack\leq a$.
- Les intervalles $\lbrack p_{x_m},p_{x_{m+1}})$, pour m de i(x) à i(y) − 1, partitionnent $\lbrack p_x,p_y)$, et
  le maximum global est le maximum des $w_m$. ∎

**Corollaires.**
- (i) Les points inactifs situés entre x et y dans l'ordre ne gênent pas : $w_m$ ne dépend que des positions.
- (ii) Le dendrogramme à plateaux atomiques de $(\Pi_K(a))_a$ est celui d'un Kruskal par lots sur n − 1 arêtes et
  n activations.
- (iii) Tout $p_x$ du sous-arbre de $t_x$ convient, y compris la feuille `Min(resolve(K, N_K(x)))`. La variante
  est testée.
- (iv) Une coupe ouverte à l'entrée (mutant `entry_open_cut` de la v1) ne change pas $p_x$ de sous-arbre. Ce n'est
  **pas** une faute dans cette construction. La faute pertinente est la clé d'égalité (§ 4.1.3).

#### 4.1.2 Algorithme (par ordre K ; les ordres sont des tâches indépendantes du pool, R6)

```text
entrées (lecture seule, TOWER § 4.7) : jrank[0..L-2], leaf_lo[], entry_node[], entry_level[], entry_pos[], entry_eq[]
1. p[x] := leaf_lo[entry_node[x]]                                      // O(n)
2. ordre := tri radix des x par (p[x], SiteIdx)                         // O(n)
3. w[m] := max jrank[p[x_m] .. p[x_{m+1}] - 1] (bottom si p égaux)     // max segmenté : UNE lecture séquentielle de jrank, O(L)
4. événements := {(clé(w[m]), arête m)} ∪ {(clé(D_K(x)), activation x)} ; tri radix u64 stable   // O(n)
5. balayage par lots de clé égale (DSU sur les rangs 0..n-1 de l'ordre) :
     a. pour chaque arête du lot : noter (racine, item pré-lot) des deux bouts ; unir
     b. pour chaque activation du lot : noter (racine finale, feuille x)
     c. grouper par racine finale ; items := items pré-lot non vides distincts ∪ feuilles entrantes
        ≥ 2 items → nouveau nœud (clé du lot, enfants triés par identifiant) ; 1 item → item ; 0 → aucun
6. contrôles O(n) : racine unique ; niveau(parent) > niveau(enfant) strictement ; n feuilles ; tailles sommées
```

- L'étape 3 est un maximum segmenté sur une partition de $\lbrack p_{x_1},p_{x_n})$. Elle est parallèle par tranches, avec une
  couture déterministe aux bords. Pour $n\ll L$, un arbre de maxima (`jtree` de TOWER) donne O(n log L) ; les deux
  sont exposés et doivent donner le même résultat (porte G11).
- Nœuds numérotés dans l'ordre des lots ; dans un lot, groupes triés par plus petit identifiant d'item. Les
  feuilles 0..n−1 sont en ordre de `SiteIdx`, donc la numérotation ne dépend que des positions (R1, R5).
- Mémoire : ≈ 70 o par point et par ordre, soit ≈ 28 Mo pour 40k points × 10 ordres. `jrank` est lu en place,
  sans copie.

#### 4.1.3 Clés unifiées exactes (tous ordres)

- Un rang de la tour r (`LevelRank`, commun à tous les ordres) a pour clé `r << 32`.
- Un niveau d'entrée entier D a pour clé `(pos(D) << 32) | (eq ? 0 : 1 + s)` :
  - pos(D) est le plus grand rang de niveau ≤ D ;
  - eq ⟺ D = niveau(pos(D)) : même lot que l'événement de la tour ;
  - s est le rang dense de D parmi les valeurs d'entrée **de tous les ordres** qui tombent dans le même intervalle
    ouvert entre deux rangs.
- ⊥ a pour clé 0 : le rang 0 est le niveau nul, et le lot 0 contient les entrées de K = 1 et des atomes lourds.
- Ordre des clés = ordre exact des niveaux ; égalité des clés ⟺ égalité exacte. Les clés sont **comparables entre
  ordres**, ce que la tête T2 exige.
- `pos` et `eq` sont calculés par l'étage Q de la tour, qui les calcule déjà pour sa requête WA d'entrée ; on
  demande qu'elle les **publie** (4 o + 1 bit par site et par ordre).
- À défaut, la tête les recalcule par dichotomie sur la table des rangs : filtre double certifié (erreur relative
  ≤ 3·2^-53 d'ARCH_v2 § 4.2, marge 2^-46), puis repli exact U192 × u128. Soit ≤ n·Kmax·log L comparaisons ; la
  table globale des D compte ≤ n·Kmax entrées.
- Les doubles de niveau (pour λ) ne sont calculés que pour les ≤ n rangs présents dans le dendrogramme de points, à
  50–100 ns chacun (TOWER § 5.8). Aucun rationnel n'est réduit dans le chronomètre.

#### 4.1.4 Invariants de la tête sur la sortie de la tour (O(n), toujours actifs)

- **I-H1 (entrée vivante).** $\mathrm{clé}(\mathrm{rank}(t_x))\leq\mathrm{clé}(D_K(x))<\mathrm{clé}(\mathrm{rank}(\mathrm{successor}(t_x)))$,
  ou $t_x$ racine. Ce contrôle détecte une coupe ouverte ou un décalage d'un rang dans le WA d'entrée de la tour.
- **I-H2 (monotonie verticale des entrées).** $D_{K+1}(x)\geq D_K(x)$ pour tout x (O(n·K)). Le préfixe
  $N_K(x)\subset N_{K+1}(x)$ n'est pas publié par la tour : il est contrôlé par le juge d'échantillon (§ 12.4).
- **I-H3 (dendrogramme).** Racine unique, n feuilles, niveaux strictement croissants vers la racine, tailles et
  masses sommées.
- Un échec rend `invariant_violated` avec sa raison (ARCH § 4.5), jamais un clustering.

#### 4.1.5 Contrat consommé (sous-ensemble d'`OrderForest`, TOWER § 4.7)

```cpp
namespace mhgp10::points {
struct TowerOrderView {                 // lecture seule, aucune copie
  Order K; u32 n_nodes, n_leaves;
  std::span<const LevelRank> rank;      // [n_nodes], non décroissant en identifiant
  std::span<const NodeIdx>   successor; // [n_nodes] (I-H1, balayage différentiel)
  std::span<const u32>       leaf_lo, leaf_n;          // [n_nodes]
  std::span<const LevelRank> jrank;     // [n_leaves - 1]
  std::span<const NodeIdx>   entry_node;               // [n_sites]
  std::span<const u64>       entry_level;              // [n_sites] = D_K(x), poids compris
  std::span<const u32>       entry_pos;                // [n_sites] DEMANDÉ à l'étage Q (sinon recalcul, § 4.1.3)
  std::span<const u8>        entry_eq;                 // [n_sites] DEMANDÉ (bit : D_K(x) = niveau(entry_pos))
  const Csr<NodeIdx>*        parents;                  // balayage différentiel (G11) seulement
};
struct TowerView {
  Order keff; u32 n_sites; u32 n_ranks;
  std::array<TowerOrderView, 11> order;
  double level_double(LevelRank) const;                // binary64 PUR, déterministe (§ 6.2)
  int compare(LevelRank, u64) const;                   // exact (secours de § 4.1.3)
  const FacetLocator& facets() const;                  // T3 seulement : locate_facet(K, F) (TOWER § 5.6)
};
}
```

- Si la tour ne publie pas l'index d'intervalles ou les entrées, la tête ne peut pas être exacte relativement à la
  tour. C'est un **refus** (`invariant_violated/tower_view_incomplete`, raison ajoutée à `reasons.def`), jamais un
  repli silencieux.
- Si P13 remplace le Kruskal séquentiel par un Borůvka intra-ordre, l'ordre des feuilles et `jrank` restent
  requis, puisque TOWER s'en sert pour ses propres ancres. Ils se dérivent après coup par un parcours parallèle de
  l'arbre : préfixes des tailles de sous-arbres, puis jonction = niveau du plus petit ancêtre commun de feuilles
  consécutives. **La tête ne dépend d'aucun algorithme de T2.**

### 4.2 Algorithme 2 : balayage de $T_K$ (différentiel, hors chemin produit)

C'est l'étape 5 de la v1 : balayage des nœuds de $T_K$ par identifiant, fusionné avec les points par clé, tenue de
`pnode`. Coût $O(H_K+n)$, séquentiel par ordre. Il est **exact** (preuve v1 (i)–(iii), confirmée par la critique) et
**distinct** du graphe-chemin (il lit `parents`, pas `jrank`). Il sert de différentiel à l'échelle (G11) : même tour,
deux algorithmes, dendrogrammes identiques octet pour octet.

### 4.3 Témoins MR_α (α ∈ {1, 2})

**Définition** (entiers exacts).
- $w_\alpha(x,y)=\max(\alpha^{2}D_K(x),\alpha^{2}D_K(y),d^{2}(x,y))<4\cdot3\cdot2^{36}<2^{40}$.
- Entrée de x : $\alpha^{2}D_K(x)$ ; rayon = $\sqrt{w}/\alpha$.
- $D_K$ compte les poids (`SiteTree::kth_distance`, ARCH_v2 § 5.1). Les atomes lourds ($\mu\geq K$) entrent à 0.

**Arbre couvrant.**
- Borůvka sur `SiteTree`. Ordre total des arêtes : $(w,\min\mathrm{SiteIdx},\max\mathrm{SiteIdx})$.
- Par tour, deux passes `atomic_min` sur u64 :
  - (1) poids minimal sortant par composante ;
  - (2) parmi les points qui l'atteignent, arête empaquetée `(min << 32) | max`.
- Sans verrou sur toute cible 64 bits, déterministe (le minimum ne dépend pas de l'entrelacement), conforme à R4.
- Élagage des nœuds : borne inférieure de $d^{2}$ sur la boîte, minimum de $\alpha^{2}D_K$ du nœud, drapeau
  « mono-composante ».
- ≤ ⌈log₂ n⌉ tours.

**Dendrogramme.** Kruskal par lots de w égal, plateaux N-aires. Toutes les MST ont la même suite de composantes, donc
le dendrogramme est unique.

**sklearn n'est jamais utilisé comme producteur MR_α.** Sa sémantique de `alpha` dépend de l'algorithme (sonde EVAL
v2 `sk_alpha_algo.log` : `brute` = simple changement d'échelle, 20/20 ; `kd_tree` = d/α seul, 20/20 différents). Il ne
sert que de référence externe à K où les plateaux sont sûrs (G2).

### 4.4 Atomes-facettes § 9.1 (tête T3, référence de fidélité)

- **Catalogue `gabriel`.** Cofaces de Gabriel σ = I ∪ U des naissances régulières d'ordre K+1, avec énumération
  par supports positifs pour les coquilles étendues (v1 § 2.4, inchangé).
- **Facettes** F = ∂C : les représentants des cellules (b, K), déjà résolus par l'étage G, exportés sur demande,
  plus les σ ∖ {z} pour z ∈ I (TOWER § 5.6).
- **Date et rattachement.** τ naît à β(τ), sa propre MEB, rationnelle exacte. Elle est rattachée par
  `locate_facet(K, τ)`, qui donne une position de feuille.
- **Même construction par graphe-chemin**, avec les facettes comme atomes de masse $m_\tau$ et activation à β(τ).
  La clé de β(τ) se calcule par dichotomie exacte rationnel/rationnel (U320). C'est lent, mais T3 n'est pas sur le
  chemin LiDAR.
- **Masses et vote** comme en v1 : $S_\tau$, $T_x$, $w_{x\tau}$, $m_\tau$.
  - Vote : **abstention** si
    $\lvert V_x(c_1)-V_x(c_2)\rvert\leq\varepsilon_{\mathrm{N}}(c_1)+\varepsilon_{\mathrm{N}}(c_2)$, où
    $\varepsilon_{\mathrm{N}}$ est la borne de Neumaier. L'abstention est comptée (`vote_near_ties`).
- **Mode `hgp_old_repro`** (n ≤ 40) : inchangé (v1 § 2.4), porte G7.

---

## 5. Condensation N-aire (Campello et al. ; divergence déclarée avec sklearn)

### 5.1 Algorithme

Il est inchangé par rapport à la v1 § 3 :
- descente en largeur ;
- enfants « gros » si leur masse est ≥ m ;
- continuation s'il y a un seul gros enfant, scission s'il y en a au moins deux, fin s'il n'y en a aucun ;
- les petits enfants sortent au λ du nœud ;
- un plateau N-aire est traité en **une** action.

Une correction : un atome qui atteint la file (masse ≥ m) sort au λ de son entrée **planchée**,
$\lambda(\max(r_{\mathrm{entrée}},r_{\mathrm{floor}}))$ (§ 6.1). Il ne sort jamais à +∞.

### 5.2 Divergence avec sklearn (déclarée)

- sklearn binarise tout plateau dans l'ordre de sortie de son arbre couvrant. Il crée alors des scissions de durée
  nulle ou rattache un petit item à l'un des gros selon cet ordre.
- La condensation N-aire est **canonique** : elle ne dépend ni de l'ordre d'entrée, ni de la MST choisie.
- Fixture `line5` (§ 12.2), mcs = 2, ms ∈ {1, 2}. sklearn 1.9.1 donne à x₂ l'étiquette de l'amas de gauche dans
  l'ordre d'origine et celle de l'amas de droite dans l'ordre inverse, avec les trois algorithmes. La N-aire rend
  x₂ bruit dans les trois permutations.
- À K ≥ 3 sur MR, les plateaux sont structurels : un point dont la distance-cœur domine crée un plateau à cette
  valeur. D'après le critique : 5 823 multifusions sur 16 nuages à K = 3 ; étiquettes identiques à sklearn dans
  6/16 exécutions (K = 3) et 3/16 (K = 5).

### 5.3 Lemme des plateaux sûrs (PL, `proved_here`)

**Énoncé.** Soit un nœud-plateau d'items $v_1,\ldots,v_m$ (m ≥ 3), de masses $s_i$, et $s_{\max}$ la plus grande.
Si $\sum_{i\neq\arg\max}s_i<m_{\mathrm{cs}}$, alors toute binarisation du plateau donne la même condensation que le
traitement N-aire : mêmes sorties, mêmes λ, mêmes continuations.

**Preuve.**
- *Cas $s_{\max}\geq m_{\mathrm{cs}}$.* Dans tout arbre binaire sur les items, chaque nœud a un côté qui contient
  $v_{\max}$ (gros) et un côté qui n'en contient pas, de masse ≤ $\sum_{i\neq\max}s_i<m_{\mathrm{cs}}$ (petit).
  Le petit côté sort au λ du plateau, et l'on continue dans le même cluster jusqu'à $v_{\max}$. En N-aire, le seul
  gros est $v_{\max}$ : continuation, et les autres sortent au même λ.
- *Cas $s_{\max}<m_{\mathrm{cs}}$.* Deux côtés disjoints ne peuvent pas être gros tous les deux : l'un ne contient
  pas $v_{\max}$ et pèse moins que mcs. Aucune scission n'a donc lieu, et tous les points sortent au λ du plateau,
  comme en N-aire.
- Les arbres condensés sont identiques. Les sélections EOM le sont aussi, puisque la règle d'égalité est la même
  (parent gardé, `>` strict pour les enfants chez sklearn). ∎

**Usage.** La porte G2 compare à sklearn seulement les exécutions dont **tous** les plateaux sont sûrs. Le taux de
plateaux sûrs est publié par K et par famille.

### 5.4 Stabilité, sortie, validateur

La stabilité suit la sémantique sklearn (`compute_stability`), sommée par Neumaier dans l'ordre de l'arbre, avec la
borne publiée. La sortie est `mhgp10_condensed_tree_v1`, avec validateur O(n + C) (v1 § 3). L'équivalence avec la
condensation ascendante de HGP-old (OP5) est inchangée.

---

## 6. Échelles λ, plancher, ẑ

### 6.1 Définitions

- $\lambda_z(r)=r^{-z}$, où r est exprimé dans les unités du producteur.
- **Plancher.** $\lambda(0):=\lambda(r_{\mathrm{floor}})$, avec $r_{\mathrm{floor}}=1/2$ (tour), $1/\alpha$ (MR_α).
  Comme $r_{\mathrm{floor}}$ minore tout rayon positif du producteur, λ(0) majore tous les autres λ, égalité
  possible. Les égalités sont tranchées par les clés, jamais par λ.
- Le plancher ne sert qu'aux atomes de masse ≥ mcs entrés au niveau 0 (K ≤ μ). **Déclaré** : c'est une
  convention, pas un théorème. Sans elle, la stabilité est infinie ; sklearn ne gère pas les poids.
- La densité multi-ordres $\lambda(k,r)=k/r^{z}$ ne sert qu'à T2.

### 6.2 Arithmétique déterministe (correction de la v1)

- **Niveau → double.** `level_double(rank)` doit être calculé par une suite fixe d'opérations IEEE binary64 :
  extraction des 64 bits de tête du numérateur et du dénominateur, conversions u64 → double, division, `ldexp`.
  Pas de `long double`, pas de contraction FMA (`-ffp-contract=off` ; CUDA `-fmad=false -prec-div=true
  -prec-sqrt=true`). C'est une exigence posée à TOWER et à GEN, qui fournissent cette fonction.
- $r=$ `sqrt` IEEE, correctement arrondi.
- **λ.**
  - z ∈ {1, 2, 3} : suites fixes de multiplications et de divisions.
  - z réel : $\lambda=\mathrm{det\_exp2}(-z\cdot\mathrm{det\_log2}(r))$, par des polynômes maison à ordre
    d'évaluation fixé, sans libm. Leur erreur est bornée dans le code (≤ 8 ulp visés) et vérifiée contre
    `mpmath` sur $10^{6}$ tirages.
  - Les résultats sont identiques octet pour octet sur toute plateforme IEEE : x86-64 SSE2, aarch64, CUDA avec
    les options ci-dessus.
- **Stabilités.** Neumaier dans l'ordre de l'arbre, jamais par fil.
- L'ordre des λ ne décide jamais la topologie.

### 6.3 Estimateur ẑ (préenregistré)

- Levina–Bickel avec la correction de MacKay–Ghahramani, k = 10 voisins, **sites distincts autres que x**.
  $T_j\geq1$ unité de grille, donc aucun logarithme de 0 ni plancher 1e-12.
- Sommes dans l'ordre des `SiteIdx`, logarithmes par `det_log2`.
- **Arrondi publié** : $z_{\mathrm{used}}=\mathrm{round}(16\hat{z})/16$, avec arrondi au pair.
  - Raison : bit-identité entre plateformes, et insensibilité de la tête aux fluctuations de ẑ en dessous de
    1/32, qui sont très en deçà de la variance de l'estimateur.
  - Garde : ẑ ∉ [0,5 ; 3,5] ⇒ saturation au bord, comptée (`zhat_clamped`).
- ẑ par cluster sélectionné : diagnostic seulement (v1 § 4.2).
- **Directive d'exposant** (mémoire utilisateur : z = dimension intrinsèque ; z = 1 pour l'équité face à HDBSCAN,
  z = 2 pour les surfaces LiDAR).
  - Banc synthétique : z ∈ {1, $z_{\mathrm{used}}$}. z = 1 est la ligne d'équité ; le choix se fait sur `dev`.
  - Régime LiDAR : **z = 2 fixé**. ẑ est publié par trame, et une trame où |ẑ − 2| > 0,5 est comptée
    (`zhat_lidar_off`) sans changer z.

---

## 7. Sélections

- **EOM** : $R(c)=\max(E(c),\sum R(c'))$. Racine exclue par défaut. Égalité → parent. Quasi-égalités comptées
  (`eom_near_ties`). La sélection est une antichaîne.
- **leaf** : feuilles du condensé.
- **epsilon** : sémantique **exacte** de sklearn (`_tree.pyx`, `epsilon_search` et `traverse_upwards`).
  - Un cluster sélectionné dont le rayon de naissance est **strictement** inférieur à ε est remplacé par son premier
    ancêtre de rayon de naissance **strictement** supérieur à ε.
  - La remontée s'arrête à l'enfant de la racine, sauf `allow_single_cluster`.
  - On dédoublonne ensuite.
  - ε est donné comme **quantile des rayons de fusion propres** du producteur (EVAL § 4.3), donc sans unité
    commune entre producteurs.
- **cut(a)** : coupe fermée à un niveau exact (DBSCAN*).
- **Étiquettes** : clusters numérotés dans l'ordre croissant de leur plus petit `SiteIdx`. Cela ne dépend que des
  positions (R5) : les étiquettes sont **identiques**, pas seulement équivalentes, sous renommage des `PointId` et
  sous permutation de l'entrée.
- Probabilités sklearn et GLOSH publiées.

---

## 8. Politique de bruit

### 8.1 Politiques

- `none`.
- `bounded(ρ)` (définition de L14) : $\mathrm{core}_5=\sqrt{D_{\max(K,5)}}$, $Q_{0,95}$ linéaire.
- `full`.
- Plus proche point classé : requête sur `SiteTree`, avec un drapeau « contient un classé » par nœud (calculé en
  O(n) de bas en haut). Aucun second arbre k-d.
- Égalité exacte de distance entre deux classés d'étiquettes différentes : **abstention** (le point reste bruit).
  C'est comptée (`fill_ties`).
  - Raison : cette règle ne dépend d'aucun ordre arbitraire, même pas du repère des coordonnées. Un départage par
    `SiteIdx` serait conforme à R5, mais dépendrait de l'ordre de Morton, donc de l'orientation du repère.
  - Cela tranche la question 7 de la v1 contre EVAL § 2.6, qui départage au plus petit indice. **Divergence
    signalée à EVAL.**

### 8.2 Choix de la politique primaire

- **Aucun a priori.** Le choix se fait par EVAL § 7.2 sur `dev` (bruit présent dans toutes les familles), sur la
  même grille pour la tour et pour `hdb_dev`.
- Recalcul du signal de la v1 (`proto_cx/fill_proxy.csv`, poids égal par niveau de bruit) :

| Politique | ARI_s, poids égal par niveau |
| --- | --- |
| `bounded(1,25)` | 0,8148 |
| `bounded(1)` | 0,8106 |
| `none` | 0,8067 |
| `bounded(1,5)` | 0,8064 |
| `bounded(2)` | 0,7787 |
| `full` | 0,7529 |

  Les cellules à 10 % et à 30 % ne comptent que 5 exécutions, toutes de la famille sphérique. Ce signal est donc
  **inexploitable** : il ne sert qu'à montrer que la politique domine les écarts (±0,15).
- Chaque adversaire est évalué avec la même politique que la tête comparée (EVAL `hdb_match_fill`).

### 8.3 Appartenance douce

Diagnostic : fraction des K voisins de x dans chaque cluster sélectionné ; $w_{x\tau}$ agrégés pour T3.

---

## 9. Têtes candidates (préenregistrées) et alternatives écartées

Paramètres communs : mcs = round(√n) en masse ; EOM, racine exclue, égalité → parent ; $z\in\lbrace1,z_{\mathrm{used}}\rbrace$ ;
politique de bruit choisie sur `dev`. Toutes les variantes sont publiées sur **la même construction**.

### 9.1 T1 — EOM-densité à K fixe

- K ∈ {2, 3}, choisi sur `dev` (EVAL § 7.2) ; K = 5 en variante publiée.
- Sources : tour (C∩X), MR_1, MR_2, sk.
- **Honnêteté sur K = 2** (lemme L2). Sur MR_1, T1 à K = 2 est la liaison simple munie de l'échelle $r^{-z}$ et
  de la politique de bruit, c'est-à-dire HDBSCAN(ms = 1). Les chiffres L14 « ms = 2 » et la sonde v1 « 0,795 à
  K = 2 » sont des chiffres ms = 1, obtenus avec `bounded(2)`, qui n'est pas le choix de la règle (§ 8.2).
- Nouveauté sur la tour seulement : connexité de la multicouverture exacte. La priorité (Blaser et al., arXiv
  2405.01214, bifiltration cœur et multicouverture) est à vérifier avant toute revendication.

### 9.2 T2 — tranche γ + EOM (axe K)

- $k(s)=\mathrm{clamp}(\lceil K_{\mathrm{hi}}(1-s/s_0)\rceil,2,K_{\mathrm{hi}})$, $s_0=c\cdot\mathrm{med}_x\sqrt{D_{K_{\mathrm{hi}}}(x)}$,
  c ∈ {1, 2, 4, 8} sur `dev`.
- **$K_{\mathrm{hi}}=5$** (v1 : 10). Raisons :
  - coût de campagne : tour K5 au lieu de K10, soit 5,3 fois moins de boules en uniforme 32k (§ 11.4) ;
  - T2 perdait de 0,016 à 0,037 contre T1 en développement : le gain espéré ne justifie pas K10.
- $\lambda=k(s)/s^{z}$.
- **Construction** : composition des dendrogrammes $P_k$ de chaque ordre, **jamais** des arêtes-chemins d'un autre
  ordre (contre-exemple `phantom_transfer_n5`).
  - Au passage de k+1 à k au rayon $s_b$, chaque point actif à l'ordre k est uni au représentant de son item de
    $P_k$ vivant à $s_b^{2}$ (par C3, la partition courante raffine $\Pi_k(s_b^{2})$).
  - On rejoue ensuite les lots de $P_k$ dans $(s_b^{2},s_{b'}^{2}\rbrack$.
  - Les clés unifiées (§ 4.1.3) ordonnent les événements de tous les ordres.
  - Sur MR : même code, avec les dendrogrammes MR_α de chaque ordre (texte v1 « max(d, s_lo) » corrigé en
    « max(w, s_lo) »).
- Rôle : second niveau publié. Tête candidate pour le bruit fort.

### 9.3 T3 — § 9.1 pondéré sur FULL (fidélité à la thèse)

Elle tourne sur la tour seulement, à K = 2, avec le catalogue `gabriel`, F = ∂C, dates `cech_birth` et z = ẑ (masses
et λ). Condensation en masse, vote avec abstention sur quasi-égalité, politique de bruit. À K = 1, T3 ≡ T1. Prévision :
T3 ≤ T1 (biais de degré de $S_\tau$). Son témoin E1 est `mr_devbest` (EVAL § 4.3).

### 9.4 Retirées ou écartées (inchangé, avec raison)

- **EOM intégrée sur K** (`measured_negative`, v1 : 0,722–0,777 contre 0,795).
- **Jointure OU multi-ordres** : fermeture par union refusée.
- **Intersection multi-ordres** : dominée par l'ordre le plus bruité.
- **Routage par votes** : C∩X est déjà une partition.
- **ToMATo par plus grand écart** : 0,55–0,59.
- **Feuilles par défaut** et **échelle log** : non.

---

## 10. Architecture C++ (couches `points/` et `head/` d'ARCH_v2)

### 10.1 Modules (budget : ≤ 3 000 lignes de produit, ≤ 3 000 lignes de portes)

| Fichier ARCH_v2 | Rôle | Dépend de |
|---|---|---|
| `points/dendrogram.hpp` | contrat `PointDendrogram` (ARCH_v2 § 5.4) et forme interne `AtomDendrogram` ; conversion O(n) | core |
| `points/keys.cpp` | clés unifiées (§ 4.1.3), table globale des $D_k$, secours exact | core, `TowerView` |
| `points/from_tower.cpp` | graphe-chemin (§ 4.1.2), invariants I-H1 à I-H3 | `keys`, `dendrogram` |
| `points/mreach.cpp` | MR_α : Borůvka `SiteTree`, dendrogramme (§ 4.3) | cloud, `dendrogram` |
| `points/zhat.cpp` | ẑ (§ 6.3) | cloud |
| `points/facets.cpp` | atomes-facettes T3 (§ 4.4) | `TowerView::facets`, `keys` |
| `head/condense.cpp` | § 5 | `dendrogram` |
| `head/select.cpp` | § 7 | `condense` |
| `head/scale.hpp` | λ, `det_log2`, `det_exp2` (§ 6.2) | core |
| `head/fill.cpp` | § 8 | cloud |
| `head/heads.cpp` | T1, T2, T3 ; une construction, toutes les variantes | tous |
| `head/validate.cpp` | validateurs O(n + C) | tous |

- Hors produit (`tests/`, `bench/`) : `sweep_ref.cpp` (algorithme 2, § 4.2) ; `ref/` Python MIT
  (`cx_oracle.py`, `pathgraph_ref.py`, `mreach_ref.py`, `condense_ref.py`, `hgp_old_semantics.py`,
  `vor_catalogue.py`).
- Liaison Python : l'API C d'ARCH (§ 11.3). La tête n'a pas d'ABI propre ; la C ABI de la v1 est retirée au profit
  de celle d'ARCH.
- `head/` ne voit que `points/dendrogram.hpp` (ARCH_v2 § 3). `points/` n'appelle aucune MEB (un seul résolveur,
  dans `tower/`).

### 10.2 Structures

```cpp
namespace mhgp10::points {
struct LotKey { u64 v; };                    // § 4.1.3 ; égalité <=> même niveau exact ; comparable entre ordres
struct AtomDendrogram {                      // N-aire, plateaux atomiques
  u32 n_atoms, n_nodes;                      // atomes 0..n_atoms-1 en ordre de SiteIdx ; nœuds ensuite, ordre des lots
  Buffer<LotKey> key;                        // [n_atoms + n_nodes]
  Buffer<double> radius;                     // unités du producteur ; plancher appliqué aux clés nulles
  Buffer<u32> parent;                        // kNone à la racine
  Csr<u32> children;                         // enfants triés par identifiant
  Buffer<double> atom_mass;                  // vide => poids des sites (u32) convertis
};
}
namespace mhgp10::head {
struct CondensedTree { /* v1 § 8.2, inchangé ; birth_key racine = +inf symbolique */ };
struct Selection { Buffer<u32> clusters; Buffer<i32> atom_label; u32 eom_near_ties, fill_ties, vote_near_ties; };
}
```

Tous les grands tableaux passent par `Buffer`, et donc par `MemoryBudget` (ARCH § 4.3).

### 10.3 Arithmétique et déterminisme

- **Exact** :
  - $d^{2}$, $D_K$, $w_\alpha$ en u64 ;
  - clés de lot en u64 ;
  - comparaison rang/entier par la fonction de la tour (filtre double certifié, puis U192 × u128) ;
  - aucune MEB dans `points/` ni dans `head/`.
- **Binary64 pur** : r, λ, masses, stabilités, ẑ, quantiles.
- **Équivariance** (R1, R5) :
  - la partition C∩X ne dépend pas du départage (C1) ;
  - les dendrogrammes (N-aires) ne dépendent pas de la MST ;
  - EOM : égalité → parent ;
  - étiquettes par plus petit `SiteIdx` ;
  - remplissage et vote : abstention.
- **Bit-identité quel que soit W** : écritures par case, max segmenté à couture fixe, Borůvka par `atomic_min`,
  balayage séquentiel par ordre.

---

## 11. Coûts (le régime LiDAR d'abord ; estimations, sauf mention « mesuré » ou « dérivé »)

### 11.1 LiDAR, tête sur la tour du contrat (trames sans sol, n ≈ 40–46 k sites, G4 W48)

**Volumes (mesurés, TOWER § 11.1)** : feuilles cumulées sur les ordres, $\sum_KL_K$ = 0,90 M (K5) et 4,41 M (K10)
sur ng00.

| Poste de la tête | K5, ordres 1–5 | K10, ordres 1–10 | Nature |
| --- | ---: | ---: | --- |
| entrées C∩X (WA d'entrée) | dans TOWER Q | dans TOWER Q (0,40 M requêtes) | déjà budgété par TOWER |
| clés unifiées (`entry_pos` publié) | < 0,5 ms | < 1 ms | O(n·K) |
| tri des points + max segmenté sur `jrank` | ≈ 1 ms | ≈ 2 ms | lecture séquentielle de 3,6 Mo / 17,6 Mo |
| tri des événements + Kruskal par lots | ≈ 1 ms | ≈ 1,5 ms | O(n α) par ordre, ordres en parallèle |
| doubles de niveau (≤ n rangs par ordre) | ≈ 0,5 ms | ≈ 1 ms | 50–100 ns par rang, parallèle |
| ẑ (k = 11) | ≈ 1,5 ms | ≈ 1,5 ms | ≈ 60 ms CPU, parallèle |
| condensation, EOM, étiquettes (ordre $K_P$) | ≈ 1 ms | ≈ 1 ms | O(n) |
| remplissage | < 1 ms | < 1 ms | parallèle |
| **Tête, mur** | **≈ 3–5 ms** | **≈ 4–8 ms** | ≈ 0,1–0,3 CPU·s |
| Tour seule (TOWER § 11.2, hors catalogue) | ≈ 70–95 ms | ≈ 0,40–0,65 s | estimé par TOWER |
| Catalogue (ARCH, selon la constante : 4 à 14,6 µs/boule) | ≈ 0,2–0,74 s CPU ; 60–110 ms GPU | ≈ 0,85–3,1 s CPU ; 0,22–0,44 s GPU | estimé par ARCH |

Lecture :

- **La tête ne peut ni tenir ni faire échouer le contrat LiDAR.** Elle pèse au plus ≈ 4 % du total estimé
  (catalogue + tour) à K5, même sur GPU, et ≈ 1 % à K10. Le contrat C(K5, 1 s), puis 100 ms, se joue dans le
  catalogue et dans l'étage G de la tour.
- Les dendrogrammes de points **de tous les ordres** ≤ $K_{\max}$ sont produits pour ≈ +1 ms par ordre. C'est le
  « tokenizer » multi-ordres de Zoltan.
- Coût marginal nul pour P13 : la tête lit `jrank`, quel que soit l'algorithme de T2 (§ 4.1.5).
- L'étalon des descentes du critique (1,02–1,42 MEB par point à 32k, K5 et K10) concerne désormais l'étage G de
  la tour : c'est une bonne nouvelle pour TOWER (0,40 M requêtes d'entrée à K10).

### 11.2 LiDAR, voie « clustering seul » (tour à $K_{\max}=K_P$)

**Catalogue dérivé exactement du reçu L13** (`cluster_v2/catalogue_by_kmax.log`), admission $p+q_{\min}\leq K_{\max}+1$.
Les totaux K5 et K10 retombent sur les reçus, ce qui contrôle la dérivation.

| Entrée | K1 | K2 | K3 | K5 | K10 |
| --- | ---: | ---: | ---: | ---: | ---: |
| LiDAR 08/000200 sans sol (45 845 sites) | 118 890 | **301 745** (6,6/site) | **575 614** (12,6/site) | 1 407 885 | 5 483 320 |
| uniforme 32k | 122 465 | 387 239 | 843 716 | 2 531 823 | 13 491 980 |
| huit amas 32k | 117 590 | 365 210 | 785 157 | 2 305 835 | — |

**Estimations à K2 sur G4.** Les constantes sont celles d'ARCH_v2 et de TOWER ; l'accélération effective ×26 à W48
est une hypothèse d'ARCH, non mesurée.

- Catalogue : 0,30 M × {4 ; 8 ; 14,6} µs = 1,2 / 2,4 / 4,4 CPU·s, soit **≈ 46 / 93 / 170 ms** de mur CPU ;
  **≈ 14–25 ms** en GPU, à l'échelle de la cible K5 d'ARCH (60–110 ms pour 1,31 M).
- Tour : ≈ 1,2–1,45 µs par boule (TOWER K5), soit ≈ 0,4 CPU·s et **≈ 15–25 ms**.
- Tête : ≈ 2–3 ms.
- **Total : ≈ 65–200 ms en CPU seul, ≈ 35–55 ms avec le catalogue sur GPU.** Sous 100 ms en CPU seul, il faut une
  constante de catalogue ≤ ≈ 6 µs/boule : (100 − 25 − 3) ms × 26 / 0,30 M.
- À K3 : environ le double.
- MR_α sur la même trame : Borůvka O(n log n), ≈ 30–60 ms mono-fil et **≈ 2–4 ms** à W48, soit 10 à 100 fois
  moins cher que la tour la moins chère.

Conséquences :

- l'ordre de la tête LiDAR est **$K_P$**, gelé sur `dev` (question 5 pour une variante propre au LiDAR) ;
- sur la tour du contrat, la tête est quasi gratuite ;
- si la tour ne sert **qu'au** clustering, la voie K_P est la plus économe des voies exactes, mais MR la domine en
  coût. Si H3 échoue (§ 13), le clustering LiDAR par défaut passe par MR, et la tour garde ses sorties
  topologiques.

### 11.3 Bancs synthétiques (tête seule, mono-fil local, estimé)

| n | ẑ + $D_K$ (k = 11) | Graphe-chemin (un ordre) | Condensé + sélection | Remplissage | MR_α (un α) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 8 000 | ≈ 10 ms | ≈ 2 ms | ≈ 2 ms | ≈ 2–5 ms | ≈ 40 ms |
| 16 000 | ≈ 20 ms | ≈ 4 ms | ≈ 4 ms | ≈ 4–10 ms | ≈ 80 ms |
| 32 000 | ≈ 40 ms | ≈ 8 ms | ≈ 8 ms | ≈ 8–20 ms | ≈ 160 ms |

Tour à $K_{\max}\leq3$ : moins de 30 ms à 32k sur G4 (TOWER § 11.2), ×10 en local. La porte de coût porte sur des
**compteurs déterministes** : événements, octets de `jrank` lus, opérations DSU, tours de Borůvka. Leur rapport par
doublement 8k → 16k → 32k doit rester ≤ 2,3.

### 11.4 Coût de campagne (constat mineur)

Plan de test d'EVAL (§ 3.4) : 1 990 scènes à 8k, 1 194 à 16k et 1 194 à 32k, plus celles à 500 et 2 000.

- Tour à $K_{\max}=5$ (T2 et variantes K5), estimation ARCH § 6.4 à W8 local (0,7 / 1,4 / 3,0 s) : ≈ 1,9 h.
- Tour à $K_{\max}=3$ seule : ≈ 0,6 h.
- `Kmax_run = 10` d'EVAL à n ≤ 8000, pour `tw_oracle_full` : ≈ +2 h.
- Têtes et MR_α (quatre MST par scène) : ≈ 0,5 h.
- `dev` : environ le quart.
- **Total ≈ 4–5 h en local W8, ≈ 30 min sur G4 (estimé).** À confirmer par les compteurs E6 avant le
  préenregistrement.

### 11.5 Voie GPU

Aucune. La tête est en O(n) ou en lecture séquentielle : un transfert n'y gagnerait rien. Seul le k-NN de ẑ peut
utiliser l'oracle de feuilles résident de la tour (TOWER § 13). Porte : bit-identité CPU/GPU des étiquettes quand la
tour tourne sur GPU (§ 6.2).

---

## 12. Portes, fixtures, juges, mutants

Codes de sortie exacts : 0 conforme, 1 désaccord du juge, 2 refus avant calcul, 3 plancher ou invariant violé,
4 mutant tué. Tout signal est un échec.

### 12.1 Portes

- **G0 — Structure.**
  - Validateurs `AtomDendrogram` et `CondensedTree` en O(n + C), avec la **stricte croissance des niveaux** vers la
    racine : elle tue `entry_key_strict`.
  - Invariants I-H1 à I-H3.
  - Planchers : `--min-atoms`, `--min-clusters`, `--min-multifusions`.
- **G1 — K = 1.**
  - Dendrogrammes tour ≡ MR_2 (niveaux identiques) ≡ MR_1 (niveaux ×4 en carré), ≡ référence N-aire, sur toute
    exécution.
  - Étiquettes ≡ sklearn(ms = 1, `kd_tree`) sur les exécutions à plateaux sûrs (§ 5.3), taux publié.
  - Nuages sur **toute l'étendue** $2^{18}$, n ∈ {500, 2 000, 8 000}. Plancher : ≥ 5 clusters.
- **G2 — MR_α ≡ référence N-aire exacte** (`ref/mreach_ref.py` + `ref/condense_ref.py`), **sur les mêmes
  coordonnées u18** : étiquettes et condensés identiques.
  - K ∈ {1, 2, 3, 5}, α ∈ {1, 2}, familles du banc à n = 2 000.
  - Contre sklearn : seulement les exécutions à plateaux sûrs, comptées.
  - Lemme L2 en porte : $\mathrm{MR}^{1}_2$ ≡ $\mathrm{MR}^{1}_1$ (dendrogrammes à plateaux identiques) sur toute
    exécution.
  - Mutants `min_samples_plus_one` et `mr_alpha_ignored` tués.
- **G3 — Oracle C∩X exhaustif** (T2 bornée, n ≤ 10, K ≤ 4 ; 200 nuages génériques et 200 dégénérés, commandes
  journalisées).
  - Le graphe-chemin C++ lit la tour C++, et le résultat est comparé à l'oracle Fraction à **chaque** niveau
    critique.
  - Contrôles aussi de C1 (toutes les facettes témoins), de C3, de C5 et de C5bis. Ceux-ci portent sur
    **l'union** des niveaux critiques de Π, de MR_1 et de MR_2, comme le demande le mineur T4b.
  - Oracles bornés T2 : ils établissent la vérité.
- **G4 — Fixtures de repli et racine unique** sur tout consommateur (T1, T2, T3 ; tour, MR_1, MR_2 ; K ∈ {1..4}).
- **G5 — Condensation dorée** : fixtures `cond_toy8`, `plateau_3tri`, `parent_split`, `line5` (plateau N-aire),
  `heavy_atom`. Aussi : égalité EOM → parent, racine exclue ou sélectionnable, seuil en masse.
- **G6 — Invariants § 9.1** : inchangés (v1).
- **G7 — Sémantique HGP-old** : inchangée (v1).
- **G9 — Équivariance et déterminisme.**
  - Permutation de l'entrée et renommage des `PointId` : étiquettes **identiques** (§ 7).
  - `--workers-gate` : sorties identiques octet pour octet pour W ∈ {1, 2, 7, 16}.
- **G10 — Échelle** (labels `scale8000` / `scale16000` / `scale32000`, familles uniform, eight_clusters, shells,
  filaments, bridge ; trames LiDAR) : juges du § 12.4.
- **G11 — Deux algorithmes, une réponse.**
  - Graphe-chemin ≡ balayage (§ 4.2) : dendrogrammes identiques octet pour octet, sur la même tour.
  - Aussi : max segmenté ≡ requêtes `jtree`.
  - Cas couverts : 8k/16k/32k et trames ng00/ng01/ng02, K5 et K10, tous les ordres, en local et sur G4.
- **G12 — Budget.** Compteurs déterministes par doublement (§ 11.3). Chronos par étape, séparés de ceux de la tour.
  Les temps n'ont valeur de contrat que sur G4 ; tout dépassement est publié.

### 12.2 Fixtures permanentes (coordonnées exactes, points nommés dans l'ordre)

| Nom | K | Points | Attendu exact |
|---|---|---|---|
| `cx_E5` | 2, 3 | A(0,0,7) B(0,9,6) C(1,4,0) D(0,0,1) E(4,1,2) | K=2 : CDE@18, ACDE@36, ABCDE@62. K=3 : D@18, CDE@22, ACDE@42, ABCDE@82 (confirmé, oracle du critique) |
| `cx_four_L11F1` | 2 | A(0,9,0) B(24,9,0) C(12,27,0) Z(12,0,0) | ABZ@225, ABCZ@468 |
| `cx_square_K2` | 2 | (0,0,0)(2,0,0)(2,2,0)(0,2,0) | PQRS@4, un seul nœud N-aire |
| `cx_fold_k2_n6` | 2 | (1,8,9)(1,8,11)(4,10,6)(6,2,12)(11,5,10)(12,8,10) | FULL : {0,1,2}∪{4,5} à 190/7 ; mutant repli : 55/2 |
| `cx_fold_k3_n5` | 3 | (2,6,6)(2,9,6)(5,7,2)(9,4,5)(9,10,8) | FULL : 01234@54 ; mutant exact `gabriel_fold_topology` : `0123 \| 4` définitif (tue le mutant, rien d'autre) |
| `cx_firstcov_k3_n6` | 3 | (2,4,4)(2,8,5)(2,9,1)(3,7,0)(6,9,6)(8,10,7) | à a = 18 = $D_3(x_4)$ : x₄ **seul** (`12 \| 4`), dans la composante de la facette isolée {1,4,5} (β = 11) ; la première couverture ({1,2,4}, β = 41/4) le mettrait avec {1,2} ; 124 à 73/4 (confirmé, oracle du critique) |
| `line5` / `entry_equal_level` (**nouvelle**) | 2 | (0,0,0)(1,0,0)(5,0,0)(9,0,0)(10,0,0) | nœuds {x₀,x₁}@1, {x₃,x₄}@1, **un seul** nœud {x₂, n01, n34}@16 ($D_2(x_2)=16$ = niveau de la fusion de la tour par le triplet {x₁,x₂,x₃}, centre x₂, r = 4) ; mutant `entry_key_strict` : deux nœuds à 16, G0 violé |
| `line5` / `plateau_nary_vs_binary` (**nouvelle**) | 1 (et MR K ∈ {1, 2}) | idem, mcs = 2, z = 1 | N-aire : {x₀,x₁} et {x₃,x₄} sélectionnés, x₂ bruit, sous toute permutation ; sklearn : x₂ rattaché selon l'ordre d'entrée (déclaré, jamais une porte d'égalité) |
| `phantom_transfer_n5` (**nouvelle**) | 1 → 2 | (8,9,2)(1,10,9)(7,2,2)(8,0,6)(9,3,5) | à a = 11/4 : x₀ et x₁ séparés dans $\Pi_1$, reliés par des arêtes-chemins d'ordre 2 ≤ a ; tue `t2_phantom_path` |
| `cond_toy8` | — | 8 facettes de masse 1 : paires à β1, quadruplets à β4, racine à β16, seuil 3 | stabilité 1,0 par enfant (λ = 1/r) |
| `plateau_3tri` | 2 | cofaces (0,1,3), (0,2,4), (0,3,4) à β = 4 | un nœud, toutes les facettes présentes |
| `parent_split` (**réécrite**) | — | 20 atomes unitaires : A = {0..4}@1, B = {5..9}@1, C = {10..19}@1, P = {A,B}@121/100, racine = {P,C}@10000 ; mcs = 2 ; z = 1 | E(A) = E(B) = 5/11 ; E(P) = 989/110 ; E(C) = 99/10 ; sélection {P, C}. Mutant `parent_loses_children_mass` : E(P) = 0, sélection {A, B, C} |
| `heavy_atom` (**nouvelle**) | 2, MR_1 | sites A(0,0,0) μ=3, B(10,0,0) μ=3, C(20,0,0) μ=1 ; mcs = 3 ; z = 1 | A et B entrent à 0 ; plateau à w = 100 {A, B, C} ; A et B nés à λ = 1/10, sortis au plancher λ(1) = 1 : E = 27/10 chacun ; C bruit ; mutant `lambda_infinite_at_zero` tué |

- Les nouvelles fixtures sont produites par `cluster_v2/entry_equal_level.py`, `nary_plateau_fixture.py` et
  `phantom_transfer.py`.
- `entry_equal_level` est **nécessaire** : le mutant `entry_key_strict` a survécu à 550 nuages aléatoires
  dégénérés (`pathgraph_mutants.log`). C'est l'exemple même d'une fixture d'égalité à graver.
- `heavy_atom` et les fixtures pondérées de la tour attendent la levée du refus `duplicate_positions` (ARCH_v2
  D-05). D'ici là, elles ne tournent que sur MR.

### 12.3 Mutants (`--inject=`, copies mutées hors produit, ARCH § 9.5)

| Mutant | Tué par |
|---|---|
| `condense_descend_all_small` | G1, G5 |
| `gabriel_fold_topology` | G4 (`cx_fold_k2_n6` : 190/7 ; `cx_fold_k3_n5`) |
| `parent_loses_children_mass` | G5 `parent_split` |
| `lambda_ignores_z` | G5 (fixture z = 3) |
| `lambda_infinite_at_zero` | G5 `heavy_atom` |
| `root_selectable` | G5 |
| `entry_at_first_coverage` | G3, `cx_firstcov_k3_n6` |
| `entry_key_strict` | G0, `entry_equal_level` |
| `path_w_min` | G3 (tué au premier nuage dégénéré, `pathgraph_mutants.log`) |
| `path_sort_by_D` | G3 (idem) |
| `path_leaf_wrong_order` (feuille prise dans l'ordre K−1 ; `leaf_prev_K` de la sonde) | G3 (idem) |
| `t2_phantom_path` | `phantom_transfer_n5` |
| `binarize_plateau` | G5 `line5`, `plateau_3tri` |
| `min_samples_plus_one` | G2 |
| `mr_alpha_ignored` | G1 (K = 1 : tour ≡ MR_2), G2 |
| `nk_distinct_sites` | G2 pondéré (MR) ; G3 pondéré après levée de D-05 |
| `fill_tie_smallest_label` | G9 |
| `facet_date_first_coface` | G6 |

### 12.4 Juges d'échelle (8k/16k/32k et trames ; jamais exhaustifs)

Ce qui est vu et ce qui ne l'est pas, sans complaisance :

1. **Invariants globaux** (toujours) : I-H1 à I-H3, racine unique, antichaîne EOM, conservation des masses,
   emboîtement horizontal à 16 niveaux tirés (O(n) par niveau par balayage des $w_m$).
2. **G11** (deux algorithmes sur la même tour) : il voit toute faute d'implémentation de l'un des deux. Il ne voit
   pas une faute de la tour elle-même.
3. **Tour** : juges propres de TOWER et différentiel v9 sur chaque trame (`tower_merkle_v10`, ARCH_v2 M7). C'est
   l'**indépendance à l'échelle** de l'objet consommé.
4. **Entrelacements** C5 (MR_1) et C5bis (MR_2) à 16 niveaux tirés, plus C3 entre ordres consécutifs : indépendants
   de la tour, bon marché. **Zone aveugle** : une erreur qui reste dans le facteur 3 (MR_1), ou dans le facteur 4
   composé (MR_2), n'est pas vue.
5. **Échantillon** de 256 points : $N_K(x)$ et $D_K(x)$ recalculés par force brute O(n) ; `entry_pos` et
   `entry_eq` recalculés en exact.
6. **Témoin BFS borné** (ARCH_v2 J-local), 64 nœuds tirés dont la population locale est ≤ 40 sites : chemin de
   (K+1)-parties de β ≤ a entre $N_K(x)$ et $N_K(y)$, pour deux enfants distincts. Il certifie qu'une fusion
   n'est **pas plus tardive** qu'annoncé. Il ne certifie pas qu'elle n'est pas plus précoce : seule la T2 bornée le
   voit.

---

## 13. Expériences et décision (EVAL_v1 prévaut pour le protocole)

### 13.1 Sources

- `tw` : C∩X, graphe-chemin.
- `mr` : MR_1, exact et atomisé.
- `mr2` : MR_2, exact et atomisé. **Nouveau, exigé par B1.**
- `sk` : sklearn standard, binarisé.

Correspondance des noms avec EVAL : T1 = A, T2 = C, T3 = D ; le « K par consensus » = B, diagnostic seulement.

### 13.2 Choix de P et de `hdb_dev` (EVAL § 7.2, amendé)

- P est l'argmax de J sur `dev` parmi les configurations **de tour**, avec les candidats du § 9. EVAL départage à
  0,002 près par simplicité.
- **Amendement demandé à EVAL (B1, budget symétrique).**
  - La grille G_dev de `hdb_dev` porte sur les sources `mr`, `mr2` et `sk`.
  - α est une option légitime d'HDBSCAN (paramètre `alpha` de McInnes). C'est aussi la source même de la
    confusion.
- La famille N1 `uniform` sort de la moyenne confirmatoire de ce sous-système : toute tête à racine exclue y vaut 0.
  Elle est publiée avec `asc` vrai pour toutes les méthodes.

### 13.3 H3 amendée : la géométrie (intersection-union)

- Pour K ∈ {2, 3} (les deux, quel que soit $K_P$), et avec la tête de P aux autres paramètres :
  - $H3_K$ est rejetée **si et seulement si** Δ(tw − mr) **et** Δ(tw − mr2) ont chacune une borne basse de l'IC
    > 0 et une p-valeur < 0,05, dans le sens favorable.
  - C'est un test d'intersection-union : sa p-valeur est le maximum des deux, et il n'y a pas de correction à
    l'intérieur de la famille.
- Si P est T2 (multi-ordres), les témoins sont T2 sur `mr` et sur `mr2`, avec le même $K_{\mathrm{hi}}$ et le même c.
  $H3$ est alors unique (pas d'indice K).
- Si P est T3 (facettes), les témoins sont `mr_devbest` et `mr2_devbest` (EVAL § 4.3), et la phrase le déclare.
- Correction de Holm entre $H3_2$ et $H3_3$, et avec les hypothèses H1 et H2 d'EVAL.
- **Publication par politique de bruit** : la politique primaire décide. `none`, `bounded(ρ*)` et `full` sont
  publiées. Si une autre politique donne un Δ significatif de signe contraire, la phrase le dit d'emblée
  (« dépend de la politique de bruit »).
- Métriques d'arbre (DP, BNF1, EVAL E5) : même comparaison, publiée, non confirmatoire.

### 13.4 Règle de choix du producteur (« le choix le plus pertinent »)

La décision est calculée par `decide.py`, dans cet ordre :

1. **G — géométrie établie** : $H3_{K_P}$ rejetée **et** H1 (tw_P contre `hdb_dev`) rejetée.
   - La tour est le producteur du clustering.
   - La revendication « la multicouverture exacte améliore le clustering » est autorisée à $K_P$.
2. **N — non-infériorité** : l'IC de Δ(tw_P − mr_P) et celui de Δ(tw_P − mr2_P) sont tous deux inclus dans
   [−0,01 ; +∞[, sans G.
   - Le producteur par défaut du clustering est **le moins cher** : la configuration de `hdb_dev` sur sa source
     (MR ou sk).
   - La tête de tour est publiée comme variante à topologie exacte (LiDAR, Zoltan), déclarée équivalente à la
     marge près.
3. **D — dégradation** : borne haute < −0,01 contre l'un des deux témoins.
   - MR par défaut. Le clustering par la tour est déconseillé ; la tour garde ses sorties topologiques.
4. Tout autre cas (IC trop larges) : **indécis**, publié tel quel. MR par défaut, au titre de la parcimonie par
   coût.

**« Bat HDBSCAN »** (EVAL R1) :

- Dans le cas G, la revendication porte sur la tour.
- Dans les cas N et D, `hdb_dev` **est** la meilleure tête sur MR : la v10 ne peut pas le battre par construction.
  La seule revendication possible est alors « la tête v10 (échelle ẑ, mcs = √n, politique de bruit) bat HDBSCAN
  standard » (H2a–H2d). C'est une revendication de **tête**, pas de géométrie, et la phrase générée doit le dire.

### 13.5 Expériences de développement (préfixe « dev », aucune revendication)

- E0-diag : C++ contre le proxy L14 sur coordonnées **quantifiées** (diagnostic ; la porte est G2).
- E1-dev : tw contre mr contre mr2, à K ∈ {1, 2, 3, 5} et z ∈ {1, ẑ}, pour chaque politique de bruit.
- E2 (z), E3 (K, T2), E4 (bruit), E5 (arbre) et E7 (T3 contre `mr_devbest`), comme dans EVAL § 7.3.
- E6 (coûts) : tête séparée de la tour, 8k/16k/32k ; trames LiDAR à K5 et K10 et voie K_P sur G4 (scripts gardés,
  `TERMINATED` certifié, fenêtre demandée).
- E8 (SemanticKITTI « things », protocole ALPINE) : hors portes, question 5.

### 13.6 Pilote E1 à K = 2 (développement, `cluster_v2/e1_pilot_k2.csv`)

**Méthode.**
- Tour : C∩X exacte d'ordre 2 par Γ₂. Sommets = paires à d²/4, arêtes = triplets à leur MEB (binary64 dans le
  pilote). Autotest contre l'oracle Fraction : 60 nuages, 338 coupes, 0 écart.
- Témoins : MR_1 et MR_2 exacts en entiers.
- Tête T1 : z = ẑ, mcs = 20, EOM N-aire, racine exclue (fonction `t1` du critique).
- Données : banc v9, 8 familles, n = 400, 8 groupes, niveau medium, bruit 0 et 30 %, graines 2026092800–03 (déjà
  brûlées). Soit 64 nuages × 3 politiques de bruit.

**Résultats** (ARI_s moyen ; Δ apparié, IC bootstrap à 95 % ; victoires-défaites) :

| Remplissage | Bruit | tw | mr1 | mr2 | Δ(tw − mr1) | Δ(tw − mr2) | Δ(mr2 − mr1) |
| --- | --- | ---: | ---: | ---: | --- | --- | ---: |
| `none` | 0 % | 0,836 | 0,882 | 0,830 | −0,046 [−0,072 ; −0,020] 6-19 | +0,006 [−0,007 ; +0,017] 15-8 | −0,052 |
| `none` | 30 % | 0,713 | 0,703 | 0,713 | +0,010 [−0,019 ; +0,039] 20-12 | +0,000 [−0,022 ; +0,022] 15-16 | +0,010 |
| `bounded(1)` | 0 % | 0,859 | 0,884 | 0,863 | −0,025 [−0,046 ; −0,004] 8-17 | −0,004 [−0,017 ; +0,007] 11-12 | −0,021 |
| `bounded(1)` | 30 % | 0,715 | 0,691 | 0,718 | +0,023 [−0,005 ; +0,051] 24-8 | −0,003 [−0,032 ; +0,025] 14-18 | +0,026 |
| `bounded(2)` | 0 % | 0,883 | 0,894 | 0,892 | −0,011 [−0,035 ; +0,012] 14-10 | −0,009 [−0,023 ; +0,005] 8-10 | −0,002 |
| `bounded(2)` | 30 % | 0,567 | 0,555 | 0,567 | +0,012 [−0,018 ; +0,042] 24-8 | −0,000 [−0,034 ; +0,034] 16-16 | +0,012 |

**Lecture.**
1. La tour et MR_2 sont **indiscernables** : |Δ| ≤ 0,009 dans les 6 cellules, tous les IC contiennent 0,
   victoires et défaites équilibrées.
2. L'écart tour − MR_1 **reproduit** l'écart MR_2 − MR_1, par exemple −0,046 contre −0,052 (`none`, 0 %) et
   +0,023 contre +0,026 (`bounded(1)`, 30 %). Il change de signe avec le bruit, comme l'avait mesuré le critique.
3. B1 est donc confirmé sur une vraie tour : à K = 2, sur ce banc, l'effet « tour contre HDBSCAN » est l'effet α.

**Portée.**
- n = 400, sous les tailles d'intérêt : exploratoire, aucune pente, aucune revendication.
- K = 2 seulement : Γ₃ coûterait $\binom{400}{4}$ arêtes en Python.
- Graines de développement déjà brûlées.
- MEB en binary64.

### 13.7 Prévisions écrites d'avance

| Id | Prévision | Fondement |
| --- | --- | --- |
| P1a | $H3_2$ et $H3_3$ **non rejetées** : \|Δ(tw − mr2)\| ≤ 0,01 au global, IC contenant 0 | L1 (identité à K = 1), C5bis (rapport de rayon mesuré ≤ 1,36), pilote § 13.6 (6/6 cellules) |
| P1b | Δ(tw − mr1) ≈ Δ(mr2 − mr1), de signe dépendant de la politique de bruit : négatif sans bruit ni remplissage, positif à 30 % | pilote § 13.6 ; `alpha_confound.csv` |
| P1c | Issue N (ou indécise) : producteur par défaut du clustering = MR ; la valeur de la tour pour le clustering plat n'est pas établie | P1a |
| P2 | tw_P bat `hdb_match` (z = 1, même remplissage) de +0,03 à +0,06 : c'est un effet de tête (ẑ, mcs) | L14 ; v1 |
| P3 | Δ(tw_P − `hdb_dev`) : IC contenant 0 | `hdb_dev` dispose des mêmes têtes et de α |
| P4 | T2 < T1 au global ; T2 ≥ T1 sur hierarchical et à 30 % de bruit | v1 (`gamma_proxy.csv`) |
| P5 | T3 ≤ T1 | biais de degré de $S_\tau$ |
| P6 | Coûts du § 11 exacts à un facteur 2 près (G4 seulement ; compteurs en local) | TOWER § 11.2, ARCH_v2 |

Si P1a se vérifie, le document de résultat commencera par : « la multicouverture exacte n'améliore pas le
clustering plat sur ce banc ; le gain sur HDBSCAN standard vient de la tête. »

---

## 14. Obligations de preuve (registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, à inscrire avant le code)

| Id | Énoncé | Statut visé | Élément |
| --- | --- | --- | --- |
| OP1 | C1 : composante de x à $a\geq D_K(x)$ = celle de $N_K(x)$ (multiensemble), pour toute facette témoin | `proved_here` | § 3.3 ; G3 |
| OP2 | C2, C3 : emboîtements horizontal et vertical | `proved_here` | § 3.3 |
| OP3 | C5 (α = 1) : $\Pi_K(r^{2})\sqsubseteq\mathrm{MR}^{1}_K(2r)$, $\mathrm{MR}^{1}_K(\varepsilon)\sqsubseteq\Pi_K(9\varepsilon^{2}/4)$ | `proved_here` | § 3.3 |
| OP3b | C5bis (α = 2) : $\Pi_K(r^{2})\sqsubseteq\mathrm{MR}^{2}_K(2r)$, $\mathrm{MR}^{2}_K(\varepsilon)\sqsubseteq\Pi_K(4\varepsilon^{2})$ ; L1 : égalité à K = 1 | `proved_here` | § 3.3 ; G1, G3 |
| OP4 | L2 : $\mathrm{MR}^{1}_2$ = liaison simple ; HDBSCAN(ms = 2) ≡ HDBSCAN(ms = 1) en étiquettes | `proved_here` | § 3.3 ; G2 |
| OP5 | Condensation descendante N-aire ≡ ascendante par lots de HGP-old | `proved_here` | v1 § 3 ; G7 |
| OP5b | PL : plateaux sûrs ⇒ N-aire = toute binarisation | `proved_here` | § 5.3 ; G2 |
| OP5c | « Les étiquettes de sklearn HDBSCAN sont équivariantes par permutation » | `false_in_general` | fixture `line5` |
| OP6 | Résolveur (terminaison, composante préservée, refus si le catalogue est incomplet) | renvoi à TOWER PO-T4, PO-T11 | — |
| OP7 | Théorème PG (graphe-chemin) | `proved_here` | § 4.1.1 ; `pathgraph_campaign.log` ; G3 |
| OP7b | Balayage (algorithme 2) exact ; donc égal au graphe-chemin | `proved_here` (corollaire) | § 4.2 ; G11 |
| OP8 | T2 : filtration croissante ; composition des $P_k$ exacte par C3 | `proved_here` | § 9.2 |
| OP8b | « Les unions-chemins d'ordre k+1 sont valides à l'ordre k » | `false_in_general` | `phantom_transfer_n5` |
| OP9 | Invariants § 9.1 sous F = ∂C | existant | inchangé |
| OP10 | Antichaîne EOM | existant (Prop. 12.1) | inchangé |
| OP11 | « Le repli par les seules cofaces de Gabriel préserve C∩X » | `false_in_general` | `cx_fold_k2_n6` (190/7 contre 55/2) ; `cx_fold_k3_n5` ne vaut que pour le mutant exact déclaré |
| OP12 | « Composante de première couverture puis ancêtre vivant = C∩X » | `false_in_general` | `cx_firstcov_k3_n6` |
| OP13 | T2-MR = γ-linkage publiée | `open` | — |
| OP14 | EOM intégrée sur K perd en développement | `measured_negative` | `proto_cx/h2_proxy*.csv` |
| OP15 | Plancher $\lambda(0)=\lambda(r_{\mathrm{floor}})$ | convention déclarée (pas un théorème) | § 6.1 ; `heavy_atom` |

---

## 15. Risques

1. **H3 non rejetée (probable, § 13.7).** La tour n'est pas justifiée par la qualité du clustering plat, et le
   produit de clustering par défaut passe par MR. Parade : règle écrite d'avance ; valeur de la tour ailleurs
   (topologie exacte, multi-ordres, verticales).
2. **La politique de bruit domine les écarts** (±0,15) et inverse le signe de l'effet α. Parade : ARI_s, même
   politique pour tous, H3 publiée par politique.
3. **Divergence avec sklearn sur plateaux** : un lecteur attend l'égalité avec HDBSCAN. Parade : lemme PL,
   `line5`, taux de plateaux sûrs publié.
4. **Dépendance au contrat de la tour** (`jrank`, `leaf_lo`, `entry_*`). Parade : refus explicite ; ces champs
   sont déjà dans TOWER § 4.7 ; `entry_pos` et `entry_eq` sont demandés.
5. **Bit-identité de λ** (libm, FMA, CUDA). Parade : § 6.2, porte G9 sur deux plateformes dès qu'aarch64 ou le GPU
   est disponible.
6. **ẑ global faux sur les scènes à dimensions mêlées** (LiDAR). Parade : ẑ par cluster en diagnostic, famille
   `mixed_dim` d'EVAL.
7. **Coût de campagne** (§ 11.4). Parade : compteurs E6 mesurés avant le gel ; $K_{\mathrm{hi}}=5$ pour T2.
8. **Entrées pondérées** : refus côté tour jusqu'à D-05. Parade : MR les traite ; fixtures prêtes.
9. **Ordre de tête LiDAR choisi sur des données synthétiques.** Parade : question 5 ; E8.
10. **Dérive de code** (6 EOM en v9). Parade : une seule condensation, une seule sélection ; les références Python
    sont des juges, pas des variantes.

---

## 16. Questions ouvertes pour l'utilisateur

1. **Si le cas N se réalise** (tour non inférieure mais pas meilleure) : acceptez-vous que le clustering par défaut
   de la v10 passe par l'atteignabilité mutuelle, la tour restant le producteur de la topologie exacte (LiDAR,
   Zoltan) ? C'est la règle préenregistrée ici.
2. **« Battre HDBSCAN »** : dans les cas N et D, battre `hdb_dev` est impossible par construction. La revendication
   se réduit à « la tête bat HDBSCAN standard » (apparié, défauts, thèse). Cela vous convient-il ?
3. **Politique de bruit visée** : couverture complète (instances LiDAR) ou abstention (bancs à bruit uniforme) ?
   Faute de réponse, le choix est fait sur `dev` par J.
4. **α dans `hdb_dev`** : l'ajouter rend l'adversaire plus fort, puisque c'est un paramètre standard d'HDBSCAN.
   Je le préenregistre. Objection ?
5. **Ordre de la tête LiDAR** : $K_P$ du banc synthétique (défaut), ou un choix propre au LiDAR sur des trames `dev`
   SemanticKITTI (E8, données locales, seuls scores et digests versionnés) ?
6. **Voie « clustering seul » à $K_{\max}=K_P$** (≈ 0,30 M boules à K2 sur 08/000200) : faut-il en faire un mode
   produit avec son propre contrat de temps (par exemple C(K2, 100 ms) sur GPU), ou la laisser comme mesure E6 ?

---

## Annexe A — Reçus

### A.1 Sondes de cette révision (`build/v10-persist/design/cluster_v2/`)

Python 3.12.1, scikit-learn 1.9.1, numpy 2.5.3, `nice -n 19`, un fil ; worktree v9 à `ce8a649dd`. Empreintes :
`cluster_v2/SHA256SUMS`.

| Fichier | Contenu | Commande |
| --- | --- | --- |
| `pathgraph_check.py`, `pathgraph_first_lot.log`, `pathgraph_campaign.log` | théorème PG contre l'oracle du critique sur le prototype TOWER : 520 nuages, 1 319 ordres, 36 044 coupes, 0 écart | `python3 pathgraph_check.py <graine0> <essais> <nmin> <nmax> <boîte>` (chaque commande est écrite dans son journal) |
| `pathgraph_mutants.py`, `pathgraph_mutants.log` | `w_min`, `sort_by_D`, `leaf_prev_K` tués ; `key_strict` survivant sur 550 nuages | `python3 pathgraph_mutants.py 300 150 4 7 3` etc. |
| `entry_equal_level.py` | fixture `line5` à K = 2 : un nœud contre deux sous mutant | `python3 entry_equal_level.py` |
| `nary_plateau_fixture.py` | `line5` à K = 1 et 2 : N-aire contre sklearn sous 3 permutations | `python3 nary_plateau_fixture.py` |
| `phantom_transfer.py`, `phantom_transfer.log` | contre-exemple au transport des unions-chemins | `python3 phantom_transfer.py 2000 200 5 8 10` |
| `catalogue_by_kmax.py`, `catalogue_by_kmax.log` | catalogues par $K_{\max}$ dérivés des reçus L13 | `python3 catalogue_by_kmax.py` |
| `e1_pilot_k2.py`, `e1_pilot_k2.csv`, `e1_pilot_k2.cmd`, `e1_pilot_summary.py`, `e1_pilot_summary.log` | pilote E1 à K = 2 (§ 13.6) et autotest Γ₂ ; résumé avec IC bootstrap (graine 1) | `python3 e1_pilot_k2.py selftest 60` ; `python3 e1_pilot_k2.py run 400 4 0,0.3` ; `python3 e1_pilot_summary.py` |

### A.2 Sondes reprises

- **Critique** (`design/crit_CLUSTER/`, `SHA256SUMS`) : `alpha_confound.csv`, `nary_vs_sklearn.log`,
  `descent_steps.log`, `quant_e0.py`, `my_oracle.py`, `check_fixtures.py`.
- **EVAL v2** (`design/eval_v2_probe/`) : `interleave_check.log`, `sk_alpha_algo.log`.
- **v1** (`design/proto_cx/`) : `campaign.log` (non rejouable, § 3.4), `fill_proxy.csv`, `h2_proxy*.csv`,
  `gamma_proxy.csv`, `fold_*`, `firstcov_check.py`.

Ces fichiers sont hors dépôt. Ils doivent être copiés dans `morsehgp3D_v10/receipts/cluster_design_20260928/`, avec
le commit, à l'ouverture de la v10. Les chiffres cités ne valent qu'avec ce reçu.
