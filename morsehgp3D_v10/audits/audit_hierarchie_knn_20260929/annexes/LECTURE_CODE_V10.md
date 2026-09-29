# Lecture du code v10 : ce que calcule la chaîne, du nuage aux étiquettes, et comparaison avec HGP-old

```text
phase=exploration_v10_hors_registre ; backend=cpu_reference ; profile=quantized_u18_input_only ; public_status=not_claimed
GCP non utilisé. Dépôt non modifié, rien commité.
```

**Références.** Toutes les lignes renvoient au commit `8b8d66f6e`. Ses sources sont identiques octet pour octet à celles du binaire figé `v10-bench-8b8d66f6e` (sha256 comparés fichier par fichier). Les chemins sont relatifs à `morsehgp3D_v10/`.

- **Worktree partagé.** Il contient des modifications non commitées de `src/catalogue/generator.cpp` et `catalogue.hpp`, apparues pendant ma lecture (autre session). Elles ajoutent un chronométrage et un tri parallèle, avec le même ordre. Je ne les cite pas ; les numéros de ligne sont ceux de HEAD.
- **HGP-old.** Je l'ai lu comme une spécification, sans rien en recopier. Je le cite par `HGP-old/fichier:ligne`.
- **Thèse.** Les pages citées sont celles imprimées dans le manuscrit.

## 0. Résumé

1. **La tour est fidèle au modèle.** Chaque forêt d'ordre K est l'arbre de fusion exact de π0(L_K(a)), avec a = r² rationnel exact et des fusions N-aires par plateau. Je n'ai trouvé aucun défaut dans la chaîne nuage → étiquettes.
2. **En entrée `cover`, chaque point est porté par une naissance, et la tête n'utilise jamais α_K(x)².**
   - En position générale, la première boule couvrante b\*(x) a exactement K sites. C'est donc une naissance d'ordre K.
   - Le point est attaché à ce nœud-feuille, au niveau même de la naissance.
   - La masse d'une naissance est au plus K, toujours inférieure à mcs = √n. Tous les points sortent donc de leur amas aux λ des fusions qui absorbent leur naissance.
   - Mesure : 6000 points sur 6000 sont attachés à une naissance (3 scènes dev), contre 72 à 328 sur 2000 en entrée `core`.
3. **Unité de λ.** λ = niveau^(−z/2) = r^(−z), où r est le **rayon** des boules (et non son carré), en unités de grille (`src/head/head.cpp:11-14`).
4. **Pour 7 familles sur 8, z = ẑ revient à z = 3.** ẑ vaut environ 3 partout (filaments compris : 2,98), et 2,1 à 2,2 seulement sur `shells`. C'est l'échelle de densité K-NN ambiante de la thèse (Déf. 7, λ̂ = K/(n ω_p r^p)), la même que l'exposant `expZ = 3` du notebook de HGP-old.
5. **Loi de l'EOM en fonction de z (déduite du code).** Pour un parent né au rayon r_P, scindé au rayon r_s, dont les enfants (des feuilles) perdent leurs points aux rayons r_x < r_s, l'EOM retient les enfants si et seulement si E_masse[(r_s/r_x)^z] + (r_s/r_P)^z > 2.
   - Quand z → 0, la règle compare les durées de vie en log r.
   - Quand z → ∞, elle tend vers les feuilles.
   - Avec les rayons du diagnostic `shells` à K = 10 (r_e ≈ 1,5, r_s ≈ 2,0, r_P ≈ 5,5), le seuil vaut z\* ≈ 2,2. Cela explique l'échec de ẑ = 2,1 et le succès de z = 3 et 4.
   - Retenir les 8 coquilles à K = 10 demande donc un z **supérieur** à la dimension de la densité (2 pour une surface). C'est un choix de tête, pas une conséquence du modèle.
6. **HGP-old condense sur des faces, pas sur des points** (les faces sont les K-parties).
   - Il répartit la masse de chaque point sur ses faces (§ 9.1) et prend λ = r^(−expZ), avec expZ = 2 par défaut et 3 dans le notebook.
   - Sa racine est sélectionnable.
   - Il étiquette chaque point par un vote pondéré S_τ sur toutes ses faces retenues, ce qui est inclusif.
   - Ses bons résultats s'expliquent par :
     - la sémantique des amas discrets ;
     - un z de 2 ou 3 ;
     - un étiquetage inclusif ;
     - et, pour le notebook, une métrique indulgente : l'ARI y est calculé hors de tout point prédit bruit.
7. **Écarts doc ↔ code, sans effet mesurable sur le banc actuel.**
   - Remplissage borné : le code prend k = max(K, 5), alors qu'EVAL_v2 § 2.6 définit core_K.
   - Plancher de λ, arithmétique déterministe, arrondi de ẑ, numérotation des étiquettes, tête T3 : conçus, absents du code.
   - La conception CLUSTER_v2 excluait les amas discrets comme hiérarchie, ce que fait désormais l'entrée `cover`.
   - Multiplicités : le README les annonce acceptées, mais la tour les refuse, sous un code de raison erroné.

## 1. (h) Entrée, doublons, multiplicités

**Banc.** `quantize18` (`bench/synthetic/scenes.py:297-307`) calcule un pas isotrope h = étendue max / (2^18 − 1) et q = floor((x − min)/h + 1/2), rabattu dans [0, 2^18).
- Les doublons de position sont retirés pour toutes les méthodes : `np.unique(..., return_index)`, première occurrence gardée (l. 305-306).
- Les étiquettes vraies des points retirés sortent du score.
- Mesure : **0 doublon sur les 256 scènes dev.**

**Moteur.** `prepare_cloud` (`src/cloud/cloud.cpp:23-72`) regroupe les points de même position en un site de poids w (l. 60) et trie par (clé de Morton, PointId) (l. 38-46).
- Le commentaire de la l. 38 annonce un tri par (Morton, coordonnées, PointId) ; c'est équivalent, puisque Morton est une bijection des coordonnées.
- Le CLI numérote les PointId dans l'ordre d'entrée (`cli/mhgp10_cluster.cpp:79`) et diffuse l'étiquette d'un site à tous ses PointId (l. 181-182).

**Multiplicités.**
- Le catalogue les accepte (§ 2).
- La tour refuse tout w ≠ 1 (`src/tower/tower.cpp:1168-1169`), avec la raison `shell_quotient_budget`. Ce code est erroné : il s'agit d'une multiplicité, pas d'un budget de quotient.
- Deux incohérences latentes, inaccessibles aujourd'hui à cause de ce refus :
  - l'attache `core` compte des sites et non des poids (`nearest`, l. 1617) ;
  - `point_dendrogram` force `point_weight = 1` (l. 1821).

## 2. (a) Catalogue (`src/catalogue/`)

**Boules retenues.**
- Chaque boule est une sphère dont le centre c est dans l'intérieur relatif de conv(S), pour un support S de 2 à 4 sites affinement indépendants.
- I = intérieur strict, de poids p ; U = coquille complète, de poids u.
- q_min est le plus petit support. S\* est le support canonique : le plus petit, dans l'ordre lexicographique des indices de sites, parmi ceux de cardinal q_min (`support.hpp:56-98`, `generator.cpp:134-172`).
- Une boule est admise pour les ordres ≤ kmax si p + q_min ≤ kmax + 1 quand la coquille n'a pas de site pondéré, et si p ≤ kmax − 1 sinon (`generator.cpp:249`).
- La condition p + q_min ≤ kmax + 1 dit que le premier ordre où la boule agit, p + q_min − 1, est au plus kmax.
- Ce sont exactement les centres c ∈ conv(U), c'est-à-dire les seuls candidats aux points critiques des D_k.

**Génération.** Elle se fait par boîtes de centres : gardes et dominateurs (`generator.cpp:469-527`), filtres de feuille par masques (l. 281-452), refus `wide_leaf` (l. 573-577). Aucune décision n'est prise en flottant.

**Niveau.** Le rayon carré est un rationnel exact num/den (`arith/geometry.cpp:52-90`) :
- q2 : |b−a|²/4 ;
- q3 : |u|²|v|²|u−v|² / (4|u×v|²) ;
- q4 : |N|²/D².

**Ordre canonique.** Les boules sont triées par (niveau exact, S\*), avec S\* complété par kNone = 0xFFFFFFFF (`generator.cpp:666-681`).
- Le tri se fait d'abord en double, puis une réparation exacte trie à nouveau les bandes où deux doubles consécutifs diffèrent d'au plus 2^-40 relatif (l. 670-681).
- Un niveau décroissant est refusé (`rank_order`), une boule émise deux fois aussi (`census_mismatch`) (l. 700-703).
- Le rang est le rang dense des niveaux exacts distincts ; `cat.level[rank]` garde une représentation par rang (l. 695-716).

**Catalogue construit au plus grand K de la liste.** Le CLI construit un seul catalogue au plus grand K demandé (`cli/mhgp10_cluster.cpp:90-94`). Pour un ordre K plus petit, les boules en surplus n'ont pas de cellule d'ordre K, mais elles peuvent couvrir des points.
- Tout minimiseur de α_K(x) vérifie pourtant p + q_min ≤ K, par un argument de contraction : sinon, déplacer le centre vers une partie séparable et réduire le rayon donnerait une boule plus petite contenant x et K − 1 autres sites.
- La première boule couvrante et ses ex aequo sont donc admis à K. Le résultat ne dépend pas de kmax.

## 3. (b) Tour d'ordre K (`src/tower/tower.cpp`)

**Fenêtre d'une boule.** K ∈ [p + q_min − 1, p + m], où m est le nombre de sites de la coquille (l. 1242-1243). On pose t = K − p.
- **Coquille régulière** (m = q_min), l. 1385 et 566-579 :
  - naissance si K = p + m ;
  - jonction de m morceaux si K = p + m − 1, avec pour représentants I ∪ U∖{U[r]}, r = 0..m−1 (l. 1038-1046) ;
  - inerte sinon.
- **Coquille étendue** (l. 580-630) :
  - on énumère les t-parties de U séparables, c'est-à-dire telles que c ∉ conv(A) (Gordan) ; `center_in_closed_hull`, `support.hpp:17-51` ;
  - aucune partie séparable : naissance ;
  - sinon, les morceaux sont les composantes du graphe A ∼ A' ⇔ A ∪ A' séparable ; il y a jonction s'il y a au moins 2 morceaux, avec un représentant par morceau (sa première t-partie) ;
  - refus au-delà de 24 sites dans la coquille ou de 20 000 parties séparables (l. 21-22).
- **K = 1** : les n sites naissent au rang 0 (l. 1204, 1060-1063). Les boules q2 à intérieur vide (Gabriel) sont des jonctions : c'est la liaison simple au rayon d/2.

**Descente `resolve`** (l. 798-992). À partir d'une K-partie F :
1. MEB exacte de F : proposition en double, certificat exact, repli sur Welzl exact (l. 467-546).
2. Le niveau doit décroître strictement à chaque pas, sinon `descent_no_terminal` (l. 837-843).
3. Si au moins K sites sont strictement intérieurs, saut aux K plus proches du centre (l. 906-933).
4. Sinon, si la sphère est une boule du catalogue dont la fenêtre contient K :
   - arrêt sur sa naissance ou sur le mémo de sa cellule (l. 950-972) ;
   - sinon, premier représentant local (l. 978-988).
5. Semis H_K : une naissance régulière de K sites est trouvée directement par sa population (l. 818-832, 1414-1446).

Le résultat est une naissance dont la composante, au niveau de F, contient la région témoin de F.

**Kruskal par plateaux** (l. 1052-1127).
- Toutes les jonctions de même rang exact forment un lot.
- Les représentants sont rattachés à leur racine d'avant le lot, puis unis jonction par jonction.
- Chaque groupe final d'au moins 2 racines distinctes d'avant le lot devient **un** nœud N-aire au rang du lot. Ses enfants sont les nœuds sommets de ces racines, triés par identifiant.
- Les nœuds sont d'abord les naissances (ordre des boules), puis les fusions dans l'ordre de création.
- Le rang d'un nœud vaut rang catalogue + 1 ; le rang 0 est le niveau nul.
- La forêt doit avoir exactement une racine (l. 1123-1125).
- `ancestor(v, r)` rend le plus haut ancêtre de rang ≤ r, soit la coupe fermée (l. 1130-1137).

**Verticales K → K−1** (l. 1665-1754).
- Naissance : image d'une (K−1)-partie de la boule, résolue à l'ordre K−1 puis remontée au niveau de la boule.
- Fusion : image commune de ses enfants, contrôlée (`vertical_naturality`).
- Elles ne sont **pas calculées** sur le chemin du banc : `only_order = K` (CLI l. 131) les désactive (tower.cpp l. 1670). La tête ne voit qu'une seule tranche en K.

## 4. (c) Attaches des points

**`core`** (l. 1607-1634).
- Une requête donne les kq plus proches sites de x, avec la clé exacte d² puis l'indice ; x compte, à distance 0.
- D_K(x) est la clé du K-ième (l. 1625).
- La K-partie N_K(x) est résolue, puis `ancestor` est pris au rang « nombre de niveaux ≤ D_K » (l. 1626-1630).
- Le nœud obtenu est la composante de L_K(D_K(x)) qui contient x. Le départage des ex aequo n'a pas d'effet (C1 de CLUSTER_v2 § 3.3).
- `point_level` vaut D_K(x) (l. 1631).

**`cover`** (l. 1542-1605).
- Une boule est couvrante si son poids est au moins K (l. 1557).
- `first[x]` est le plus petit indice, dans l'ordre canonique, d'une boule couvrante qui contient x (l. 1569-1579), obtenu par un minimum atomique, donc déterministe.
- Le nœud est `ancestor(resolve(K premiers sites de I ∪ U), rang(b\*) + 1)` (l. 1559-1566, 1592-1603).
- `point_cat_rank` vaut rang(b\*) + 1 (l. 1604), soit le niveau α_K(x)².
- α_K(x) = min{r : d(x, L_K(r)) ≤ r} est exactement l'instant de première couverture de la Déf. 8 et du Th. 2 de la thèse (p. 21, p. 60). On a d_K/2 ≤ α_K ≤ d_K.
- **Mesuré** (3 scènes dev, n = 2000, K = 3 ou 5) : α²/D_K est compris entre 0,25 et 0,94, de médiane 0,29 à 0,38.
- **Fait structurel.** En position générale, la boule b\* compte exactement p + m = K sites (argument de minimalité, § 2). C'est donc une naissance d'ordre K, et x s'attache au nœud-feuille de b\*, au niveau même de la naissance.
  - Mesuré : 2000 points sur 2000 sur les trois scènes, avec des niveaux publiés en nombre égal aux nœuds.
  - Une naissance porte au plus K points, moins que mcs = √n (au moins 45). **La tête ne lit jamais α² : la sortie d'un point est le λ de la fusion qui absorbe sa naissance.**
- **Départage.** Les ex aequo au niveau minimal sont départagés par S\* (indices de Morton). Mesuré : 0 point ex aequo sur 6000.
- **Couverture multiple ignorée.** Dès K ≥ 2, un point peut être couvert par plusieurs composantes qui ne sont pas encore fusionnées ; la hiérarchie ne garde que la lignée de la première.
- **K = 1** : l'attache `core` est utilisée (l. 1548-1549) ; les deux entrées coïncident.

## 5. (d) `point_dendrogram` (l. 1760-1824)

- **Niveaux calculés.** On prend les niveaux exacts des nœuds et des points : rangs du catalogue en `cover`, entiers D_K en `core` (l. 1777-1786).
- **Tri.** Il est exact : double, avec comparaison exacte si l'écart relatif est ≤ 1e-9 (l. 1789-1793). Le comparateur rend l'ordre exact.
- **Niveaux publiés.** Ce sont des doubles de **r²**, strictement croissants. Un nouveau rang n'est créé que si le niveau exact diffère **et** si son double dépasse le dernier publié (l. 1801). Deux niveaux distincts de même double partagent donc un rang : la tête les voit simultanés, avec λ égaux.
- **Structure.** Les parents sont ceux de la forêt ; les enfants sont reconstruits triés par identifiant. `point_node` est la forêt ; `point_weight` vaut 1.
- **Nœuds sans point.** Tous les nœuds de la forêt sont gardés, y compris ceux de masse nulle : 50 à 63 % mesurés. Ils sont inoffensifs, car toujours « petits ».
- **Graphe-chemin.** La construction de CLUSTER_v2 § 4.1 n'est pas implémentée. L'attache directe est équivalente pour la condensation : élaguer les sous-arbres de masse nulle et contracter les nœuds unaires ne change rien à cette dernière.
- **Contrôles** (`points/dendrogram.cpp:5-31`) : structure CSR, une racine, et rang d'entrée compris entre le rang du nœud et celui de son parent.

## 6. (e) Condensation (`src/head/head.cpp:18-105`)

**Masse et λ.** mass(v) est la somme des poids des points du sous-arbre (l. 20-32), et λ(niveau) = niveau^(−z/2) = **r^(−z)**, avec +∞ pour un niveau nul (l. 11-14).

**Descente à partir de la racine** (cluster né à λ = 0, l. 67), pour chaque nœud v atteint dans le cluster c :
- Les points attachés à v sortent de c à leur propre λ (l. 74-80).
- λ_v est calculé au niveau de v (l. 82) ; les enfants de masse ≥ mcs sont « gros » (l. 85).
- **Au moins 2 gros enfants :** chacun ouvre un cluster né à λ_v, et la stabilité de c reçoit masse(u)·(λ_v − naissance(c)). Les petits enfants sont abandonnés : tous leurs points sortent de c à λ_v (l. 86-95, `drop_subtree` l. 50-64).
- **Un gros enfant :** il continue c, et les petits sortent à λ_v (l. 99-100).
- **Aucun gros enfant :** c se termine, et tous ses points restants sortent à λ_v.

**Stabilité.** Elle vaut Σ w·(λ_sortie − λ_naissance(c)), avec des sommes naïves en double.

**Comparaison avec HDBSCAN.** C'est la sémantique de sklearn, généralisée aux fusions N-aires. Sur les plateaux, l'écart est déclaré (CLUSTER_v2 § 5.2). La porte `mhgp10_head_condensation_vs_sklearn` juge la tête sur un arbre d'atteignabilité mutuelle à K = 1 et 2 (`tests/head/`), pas sur la tour.

## 7. (f) EOM, feuilles, étiquettes, vote

- **EOM** (l. 116-142) :
  - on descend du plus grand indice au plus petit ;
  - une feuille est retenue ;
  - pour un nœud interne, si la somme des meilleurs enfants dépasse **strictement** sa stabilité, on garde les enfants ; en cas d'égalité, on garde le **parent** ;
  - la racine n'est jamais retenue sans `allow_single` (l. 125, 146-147), et le banc passe toujours `allow_single` = 0 ;
  - les descendants d'un amas retenu sont ensuite désélectionnés.
- **Feuilles :** les amas condensés sans enfant (l. 144).
- **Étiquette d'un point** (l. 148-168) : c'est l'identifiant du premier ancêtre retenu du cluster d'où il sort, et −1 sinon. Les identifiants suivent l'ordre des indices condensés (descente LIFO), et non le plus petit `SiteIdx` que demande CLUSTER_v2 § 7.
- **`--label=vote`** (CLI l. 149-175) : le point prend l'étiquette de la **première** boule couvrante, par niveau, dont la composante est dans un amas retenu. Les boules non retenues sont sautées.
  - Ce n'est pas le vote pondéré § 9.1 de HGP-old (§ 12).
  - Il couvre plus de points qu'une descente simple : un point sorti tôt, mais couvert plus tard par un amas retenu, est étiqueté.

## 8. (g) Réglages du banc (`bench/synthetic/`)

- **mcs** = round(√n), où n est le nombre de sites après dédoublonnage (`run_test.py:89-90`).
- **ẑ** (`methods.py:93-101`) : Levina–Bickel à k = 10, point exclu.
  - Pour chaque point, m̂⁻¹ = (1/(k−1)) Σ_{j<k} log(T_k/T_j) ; les valeurs non finies ou ≤ 0 sont écartées.
  - ẑ = 1/moyenne(m̂⁻¹) (MacKay–Ghahramani). Il est global par scène, calculé sur la grille, sans arrondi ni saturation.
  - Mesuré sur 256 scènes dev (bruit 0 / bruit 0,1) :

    | Famille | ẑ |
    | --- | --- |
    | spherical | 3,05 / 3,08 |
    | anisotropic | 2,94 / 2,97 |
    | heteroscedastic | 3,06 / 3,07 |
    | unbalanced | 3,04 / 3,04 |
    | **shells** | **2,12 / 2,24** |
    | bridge | 3,04 / 3,06 |
    | hierarchical | 3,00 / 3,09 |
    | **filaments** | **2,98 / 3,04** |

  - `INTRINSIC_DIMENSION['filaments'] = 1` (`scenes.py:65-68`) ne correspond donc pas à ce que voit l'estimateur à k = 10 : le tube a une épaisseur de 0,18. La piste « ẑ locale » pour les filaments ne donnera pas z ≈ 1 à cette échelle.
- **b(ρ)** (`methods.py:73-90`) :
  - core(p) est la distance au **max(K, 5)**-ième voisin, point compris (`run_test.py:99`, et le même `max(k, 5)` dans `kcover_dev.py:73`) ;
  - un point de bruit prend l'amas c du plus proche point classé si core(p) ≤ ρ·Q95_c ;
  - Q95_c est calculé sur les membres de c avant remplissage (quantile de type 7) ;
  - une seule passe ; les égalités sont tranchées par l'ordre de cKDTree.
  - Le remplissage est identique pour la tour et pour sklearn.
- **ARI_s** (`metrics.py`) : le bruit vrai et le bruit prédit deviennent des singletons.

## 9. Choix arbitraires

1. Le départage des niveaux égaux se fait par S\*, c'est-à-dire par les indices de Morton. Il fixe la numérotation des nœuds et le choix entre boules couvrantes ex aequo (0 cas mesuré).
2. Le départage de N_K(x) par indice est sans effet sur la partition.
3. La même composante est une fusion N-aire par plateau, et ses enfants sont triés par identifiant. L'ordre des amas condensés, donc des étiquettes, en découle.
4. Les niveaux publiés en double fusionnent les niveaux exacts indiscernables (l. 1801).
5. Pour l'EOM, une égalité donne le parent, « gros » veut dire masse ≥ mcs, et la racine est exclue.
6. En `cover`, l'affectation est dure, à la première couverture.
7. Le remplissage prend k = max(K, 5), départage par cKDTree, en une seule passe.
8. ẑ est calculé à k = 10, globalement, sans arrondi.
9. `quantize18` garde la première occurrence d'une position, dans l'ordre d'entrée.

## 10. Divergences entre la documentation et le code

| Doc | Code | Portée |
| --- | --- | --- |
| README « Doublons = multiplicités, jamais un refus » ; README « À venir : catalogue, tour, producteur C∩X, banc » | La tour refuse w ≠ 1 (`tower.cpp:1168-1169`), avec une raison erronée ; tout ce qui était « à venir » existe | README périmé |
| SPEC § 5 et § 6 : seulement C∩X ; portes cover et régression absentes | Entrée `cover` implémentée (c'est la meilleure tête) | SPEC en retard |
| CLUSTER_v2 § 3.2 : amas discrets « jamais comme hiérarchie » | `cover` en fait une hiérarchie | Remplacé par CLUSTERING (29/09), non amendé dans CLUSTER_v2 |
| CLUSTER_v2 § 4.1 et CONCEPTION § 6.5 : graphe-chemin, clés de lot exactes, rayon | Attache directe, niveaux doubles de r² | Équivalent pour la condensation |
| CLUSTER_v2 § 6.1 et CONCEPTION § 6.6 : plancher r_floor = 1/2 | λ(0) = +∞ (`head.cpp:12`), comme EVAL_v2 § 2.5 : les deux documents se contredisent | Sans effet : mcs > 1 |
| CLUSTER_v2 § 6.2-6.3 : det_exp2 et det_log2, Neumaier, ẑ arrondi au 1/16 et saturé à [0,5 ; 3,5] | `std::pow`, sommes naïves, ẑ brut | Reproductibilité inter-plateformes |
| CLUSTER_v2 § 7 : étiquettes par plus petit `SiteIdx`, sélection eps, probabilités, compteurs de quasi-égalité | Absents | — |
| EVAL_v2 § 2.6 : core_K = d_K, départage par coordonnées lexicographiques ; CLUSTER_v2 § 8.1 : core_{max(K,5)}, abstention sur égalité | core_{max(K,5)}, départage cKDTree | Les deux documents se contredisent ; le code suit CLUSTER pour k et aucun des deux pour les égalités |
| CLUSTER_v2 § 9.2-9.3 : têtes T2 (tranche γ) et T3 (§ 9.1 sur facettes) | Absentes | T3 est l'analogue de HGP-old ; non mesurable en l'état |
| CLUSTERING § 2 : « HGP-old … étiquette par vote de couverture » | Vote v10 = première boule couvrante retenue ; HGP-old = argmax pondéré S_τ sur toutes les faces | Pas le même vote |
| CLUSTERING § 2 : « ψ = t^(−p) moins bon que ẑ (mesuré) » | ẑ ≈ 3 = p sur 7 familles sur 8 | Seul `shells` (et le bruit) peut porter l'écart. À confirmer par la grille z ∈ {1, ẑ, 2, 3, 4} en cours |

## 11. Écarts au cadre « sur-niveaux de l'estimateur K-NN »

1. **La tour est conforme :** π0(L_K) est exact, sans restriction de Gabriel (le contre-exemple E5 est traité).
2. **Appartenance.**
   - `core` est C∩X : l'ensemble de niveau de Hartigan évalué aux données.
   - `cover` suit la Déf. 8 et le Th. 2 au niveau d'entrée, mais force une partition. Or les amas discrets forment un recouvrement dès K ≥ 2, et les couvertures suivantes sont ignorées.
3. **Échelle λ.**
   - L'excès de masse de Campello n'estime ∫(f − λ_min) que si λ est l'échelle de densité : z = p = 3 en ambiant, ou z = d, la dimension de la densité (2 sur une surface).
   - z = 1 est la convention de HDBSCAN (1/ε), pas la densité K-NN.
   - z > d n'est plus un excès de masse : cela repondère vers les amas denses, donc vers les feuilles.
4. **En `cover`, le niveau d'entrée n'agit pas.** La masse vit sur les naissances ; la stabilité ne dépend que des niveaux de fusion et du nombre de points que chaque naissance couvre en premier.
5. **Une seule tranche en K :** les verticales ne sont pas utilisées.
6. **Éléments extérieurs au modèle :** b(ρ) (à l'échelle sklearn d_{max(K,5)}), mcs = √n, ẑ global.

## 12. HGP-old : ce qu'il calcule réellement

**Faces et arêtes.**
- Il calcule itérativement le Delaunay d'ordre k (`_geometry_binding.cpp:147-405`). Les (K+1)-parties sont les unions de deux K-parties adjacentes ; ce sont des parties de Voronoï d'ordre K+1, comme le notait déjà A9-72.
- Le poids d'une (K+1)-partie est le r² de sa MEB (l. 436-456), élevé ensuite à expZ/2, soit r^expZ (`hypergraph.py:119-128`), avec un ordre égal à `min_samples − 1` (l. 70-77).
- Les nœuds sont les K-parties, appelées faces. Des arêtes relient les faces consécutives de chaque (K+1)-partie, au poids r^expZ (`_cython.pyx:880-894`). Une face n'apparaît qu'à sa première arête.

**Masses § 9.1.**
- S_τ = Σ_{σ⊃τ} r_σ^(−expZ) (`_cython.pyx:841-848`), T_x = Σ_{τ∋x} S_τ, et m_τ = S_τ Σ_{x∈τ} 1/T_x (`core.py:205-215`).
- Chaque point répartit une masse 1 sur ses faces.

**Condensation.**
- Kruskal sur les faces, puis une condensation **par composante** (`core.py:224-272`).
- Condensation ascendante avec λ = 1/(r^expZ + 1e-12) (`_cython.pyx:481-483`). Une composante est éligible si sa masse fractionnaire est ≥ mcs (l. 538).
- Création de feuilles, extension, puis parent N-aire (l. 539-618) ; les lots d'égalité se font à ε près, avec ε = 0 par défaut (l. 469-479).

**Sélection et étiquettes.**
- EOM : une égalité garde le parent, et **les racines sont sélectionnables** (`clustering.py:199-226`).
- Les faces sont prises dans les sous-arbres retenus (`clustering.py:358-363`, 632-637).
- Un point prend l'argmax_c de Σ_{τ∋x, ℓ(τ)=c} S_τ ; il est bruit s'il n'a aucune face étiquetée (`core.py:297-324`). Un remplissage 1-NN est optionnel (l. 365-381).

**Paramètres.**
- Défauts : K = 2, `min_samples` → K+1, mcs = round(√n), expZ = 2 (`core.py:41-60, 141-149`).
- Notebook (`HGP-clusterer Colab.ipynb`, cellule 10) : K = 5, `min_samples` = 6, **expZ = 3**, mcs = 50. HDBSCAN y est lancé avec `allow_single_cluster=True`.
- Cellule 12, `compute_metrics` : l'ARI est calculé seulement sur les points non-bruit à la fois dans la vérité et dans la prédiction.

**Défauts.**
- `min_samples > K+1` corrompt silencieusement les calculs : le pas de lecture K+1 ne correspond plus aux lignes (`hypergraph.py:70-77` avec `_cython.pyx:774-777`), comme A9-73.
- Le backend geogram ajoute une gigue N(0, 1e-5) (`core.py:173-176`).
- Le commentaire « λ ∝ 1/r » est faux : le code utilise r^(−expZ).

**Correspondance avec v10.**
- Même objet Γ_K, mais restreint aux arêtes Delaunay et Voronoï (sensible à E5).
- Faces et masses douces, contre des points à masse unité à la première couverture.
- Même famille d'exposants : z = expZ.
- Racine sélectionnable, contre exclue.
- Vote pondéré inclusif, contre lignée plus remplissage.

**Ce qui explique ses bons résultats (argumenté, non mesuré ici).**
1. Des amas discrets par les faces, soit une première couverture souple. A9-76 l'avait déjà mesuré : le gain venait de la première couverture (0,5517), non du vote § 9.1 (0,4511).
2. Un z de 2 ou 3 : d'après la loi du § 0.5, il favorise la scission de groupes proches. v10-b l'a retrouvé de fait, puisque ẑ ≈ 3.
3. Un étiquetage inclusif : un point est étiqueté si une seule de ses faces est retenue. C'est l'analogue du vote v10 et de b(1,5), qui mesurent +0,03 à +0,04.
4. Une métrique hors bruit prédit dans le notebook, qui récompense l'abstention et n'est pas comparable à ARI_s.

La racine sélectionnable n'a pas d'effet sur 8 groupes. Les masses douces de face n'ont jamais été mesurées en v10, puisque T3 n'est pas implémentée.

## 13. z et l'EOM : mesures illustratives

J'ai appliqué `condense_dev.py` aux arbres `cover` exportés, avec mcs = 45. La vérité compte 8 groupes.

| Scène | z = 1 | z = 2 | z = 3 et 4 |
| --- | --- | --- | --- |
| `shells` medium K3 | 8 | 8 | 8 |
| `spherical` hard 0,1 K5 | 3 (1422 / 241 / 241) | 8 | 8 |
| `filaments` hard K3 | 3 (1250 / 500 / 250) | 3 | 9 (500, 481, 250, 216, 154, 91, 61, 56, 45) |

- Le nombre d'amas retenus croît avec z, comme le prévoit la loi du § 0.5.
- Sur `filaments` hard, aucun z ne donne 8 × 250 : la hiérarchie à K = 3 mélange fusions et scissions.
- Prévision à vérifier sur la grille en cours : pour `hierarchical`, où la vérité est le parent de 3 sous-amas, augmenter z dégradera le score.

## 14. Mesures faites (légères, graines dev seulement, binaire figé, 2 fils)

- `zhat_dev.py` : ẑ sur les 256 scènes dev, et comptage des doublons retirés (0).
- `exp/run_exp.py` sur `shells` medium 0 n2000 K3, `spherical` hard 0,1 n2000 K5 et `filaments` hard 0 n2000 K3 :
  - égalités de première couverture : 0 sur 6000 points ;
  - points attachés à une naissance : 6000 sur 6000 en `cover`, contre 72 à 328 sur 2000 en `core` ;
  - α²/D_K entre 0,25 et 0,94 ;
  - nœuds de masse nulle : 50 à 63 %.

Aucune graine `test` ou `test_v10b` n'a été générée.

## Fichiers de travail

Tout est dans `/workspaces/E-HGP/build/v10-persist/audit_hier/lecteur_code_v10/` :
- `zhat_dev.py`
- `exp/run_exp.py`
- `exp/resultats_exp.log` (sorties brutes)
- `exp/*.tree.*` (arbres exportés)
- `head_src/` (copie HEAD du générateur)
- `these_35_134.txt` (extraction texte de la thèse)