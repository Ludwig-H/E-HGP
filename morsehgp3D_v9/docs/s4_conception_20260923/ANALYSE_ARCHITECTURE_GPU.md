# Port GPU des voies q3/q4 des survivants (atlas, q3, q4) : analyse d'architecture

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (+ cuda_g4 pour S1–S3)
profile=quantized_u18_input_only
mode=analyse_architecture
public_status=not_claimed
```

**GCP non utilisé.** J'ai seulement lu le worktree `build/v9-open-worktree` (HEAD `a6d08f05f`) : aucun build, aucun ctest, aucune mesure. Les chiffres viennent des reçus R13 et `q34_survivor_phases`. Les débits de la section 6 sont des **projections** : il leur faut un reçu G4.

## 1. Le patron fixé par S1–S3

- **Un en-tête portable** (`witness_filter.hpp`, `certificate.hpp`, `MHGP9_HD`) reprend l'arithmétique du produit (i64/i128), ses arrêts et ses compteurs. Un groupe abstrait de 32 voies a deux réalisations : `HostGroup` (séquentiel) et `WarpGroup` (un warp).
  - `ballot2` suivi de `nth_set_bit` retrouve la position d'arrêt du balayage séquentiel, ce qui rend `uniform_tests` exact.
  - La compaction se fait dans l'ordre du balayage (`for_set`).
- **Chaque warp a son ardoise de `capacity` sites.** Un dépassement **met l'arête en attente** : l'arête entière retourne au moteur CPU, sans compteur. C'est une décision de mémoire, jamais une décision géométrique.
- **Warps persistants :** chacun prend l'arête suivante sur un compteur atomique. Les totaux par warp vivent en mémoire partagée, écrits par la voie 0.
- **Garde d'entrée côté hôte** (`validate_certificate_input` : liens d'échappement, feuilles à un seul rang, domaine u18). C'est la seule garantie de terminaison et de bornes du noyau.
- **Juges :**
  - la porte hôte compare arête par arête et compteur par compteur, avec des mutants de comparateur, des planchers et une ardoise réduite ;
  - `check_certificate_batch` vérifie les identités de voies et de travail ;
  - `judge_certificate_filter` recalcule sur CPU chaque décision rendue, dans les préflights G4 ;
  - les condensés de C sont épinglés.
- **Calibration S3 (R13, 08/000000/K5).** Environ 351 ms de mur CPU W48 (≈ 16,8 s de fil) sont remplacées par 189 ms d'appareil, transferts compris. **Le gain n'est que de ×1,9.**
  - Par arête, un warp met ≈ 139 µs, un fil CPU ≈ 8 µs : un warp vaut environ 1/17 de fil.
  - Cause : les marches d'arbre sont **uniformes**. Les 32 voies suivent le même nœud, soit environ 540 M pas dépendants.
  - Ordre de grandeur (projection, non mesuré) : ≈ 800 cycles par pas, latence L2 non masquée avec 8 warps par SM et 250 registres.
- Autre contrainte : `FAUSSES_PISTES.md` ferme « GPU par petits lots synchrones avec reconstruction hôte ». La suite doit éviter les allers-retours et les reconstructions côté hôte.

## 2. (a) Données résidentes

**Aujourd'hui, rien ne reste sur l'appareil.**
- `run_filter_batch` et `run_certificate_batch` allouent chacune l'index (nœuds, échappements, `rank_points`), le téléversent et le libèrent au retour.
- Les survivants reviennent à l'hôte (`Q34FilterBatch`), puis sont renvoyés à S3.
- L'ardoise de S3 (plages, formes) est écrasée à chaque arête.
- Le CPU reconstruit ensuite 708 686 covers (`Engine::certified_edge`, `rebuilt_covers`).

| donnée | taille | proposition |
| --- | --- | --- |
| index plat, échappements, `rank_points` | ≈ 4 Mo | un seul envoi par chaîne (`GpuPreparation` l'aplatit déjà pendant q2), puis une **session d'appareil** qui le garde de S2 à S4 |
| `spatial_order` (rang → ID) | 4n octets | **nécessaire, absent aujourd'hui** (voir ci-dessous) |
| survivants S2, masques S3 | ≈ 10 octets par arête | restent sur l'appareil ; l'hôte ne reçoit que les attentes et le ledger |
| plages du cover par arête vivante | ≈ 8 octets par plage | S3 calcule exactement `Q34EdgeCover::ranges()`, avec les mêmes compteurs. Les écrire dans une arène (arête → offset, compte ; réservation atomique par la voie 0), avec `cover_sites` par arête pour router les arêtes lourdes. La reconstruction CPU disparaît |
| formes des sites du cover | 24 octets par site | ce sont celles de S3 (`fc` mis à l'échelle 2^20, `fx`, `fy`) : même base et même échelle que `Q4LocalGeometry::form` / `Q4LocalCell`. Les 266 M tests de points de l'atlas (K5) et 1,47 G (K10) les recalculent. **Ne pas les stocker globalement** (≈ 6 Go à K5, 21 Go à K10) : les recalculer dans l'ardoise à partir des plages, en parallèle sur les voies |

**Pourquoi `spatial_order` est indispensable :** tous les départages se font sur les ID d'origine.
- clés de propriétaire (`edge_key`, `minmax`) ;
- test canonique q4 `y<x` ;
- égalité de racines départagée par `a<b` ;
- tri des supports ;
- **orientation de l'arête** : le cover trie `edge_ids` par ID, et la normale `d×u` de `Q4FamilySeed` change de signe si l'on échange a et b, ce qui renverse le balayage et ses compteurs d'entrées et de sorties. La base du prouveur, elle, est invariante par cet échange ; c'est pourquoi S3 n'en avait pas besoin.

Réutiliser les cellules du prouveur S3 (comptes uniformes aux profondeurs 2 à 6, même plan, mêmes formes) pour amorcer l'atlas serait un **nouvel algorithme** : il faudrait un théorème puis une requalification CPU. Ce n'est pas un port.

## 3. (b) Répartition du travail

**Observation clé : ces marches peuvent se paralléliser sans changer leurs compteurs.** Les marches par liens d'échappement (cover, graines q3, graines q4, domaine positif, décomposition du cover) prennent des décisions **locales au nœud** : un nœud est visité si et seulement si tous ses ancêtres ont été scindés. Une marche parallèle par frontière reproduit donc le même ensemble visité et les mêmes compteurs :
- descendre uniformément jusqu'à avoir au moins 32 sous-arbres ;
- donner ensuite une voie par sous-arbre, bornée par son lien d'échappement ;
- concaténer dans l'ordre des sous-arbres (`merged_ranges` est recalculé après la concaténation).

Ce levier s'applique aussi à S3.

**Plus fort encore pour les voies : tout vit dans le cover.** La lentille Z (graines q3 et q4, boîte de complétion) est contenue dans la boule |z−m|² ≤ 3D/4, elle-même contenue dans le cover. Un balayage des sites du cover en ordre de rang suffit donc :
- l'ordre préfixe des feuilles est l'ordre des rangs, donc la **séquence de graines est identique** ;
- la boîte de complétion et `completion_count` sont identiques (ce sont des fonctions de l'ensemble Z).

On supprime ainsi ≈ 231 M pas dépendants à K5 : graines q3 77,8 M, graines q4 51,5 M, domaine 51,0 M, décomposition 51,0 M. Les compteurs de marche sont redéfinis, d'où une variante CPU (section 5).

| sous-étape | volume K5 (K10) | répartition | pile / ardoise |
| --- | --- | --- | --- |
| atlas | 570 k atlas ; 4,48 M cellules (23,6 M) ; 329 M visites (1,78 G) | un warp par arête. La récursion des cellules reste uniforme et en ordre DFS, ce qui préserve `node_budget=4096`, `leaf_sites=32` et `max_depth=7`. La frontière du parent est classée en parallèle sur les voies, compaction ordonnée par ballot, arrêt saturant à K−1 par `nth_set_bit` | pile explicite de ≤ 8 cadres ; cellules ≤ 4096 × ~40 octets ; arène de frontières en ordinaux u32 du cover |
| centre q3 et descente dans l'atlas | 9,3 M graines (33 M) | une voie par graine : `q3_center` en i128, `scaled_floor`, ≤ 7 niveaux | registres |
| census q3 (feuille exacte ou cover) | 2,1 M feuilles (54 M tests) ; 2,7 M census globaux (112 M bornes i128) | un warp par boule, balayage coopératif avec ballot et `nth_set_bit` pour l'arrêt à K−1 (compteurs exacts) ; coquille = compte plus empreinte | aucune : remplace la pile GlobalBoxes de 55 cadres × 40 octets par fil, qui partirait en mémoire locale |
| q4 LiveOnly | 5,8 M graines (26 M) ; 85 M sites actifs (526 M) ; 28 M événements (148 M) | un warp par graine, graines en ordre de rang, feuilles vivantes en DFS ≤ 7. Dans un balayage : voir la liste sous le tableau | résumé ≤ 4096 u32 ; événements ≤ sites actifs de la feuille |
| arêtes lourdes | queue de distribution | un **bloc par arête** (groupe de 128 : ballots de warps, préfixe en mémoire partagée), routé par `cover_sites` de S3 ; sinon CPU **pendant** l'appel, l'hôte étant inactif | — |

Détail du balayage q4, dans un warp :
- une voie par site : `side`, découpage i128 ;
- compaction des événements ;
- **tri par rang** : la voie i compte les e_j < e_i. L'ordre (racine, ID) est total, donc la séquence triée est unique ;
- préfixes des entrées et sorties, donc profondeur de chaque groupe en parallèle ;
- à l'intérieur d'un groupe, les présentations restent séquentielles jusqu'à la première émise.

**Mise en attente :** arène de frontières pleine, tampon d'événements plein, tampon d'émission de l'arête plein, ou arène globale pleine. Avec une ardoise de C = 8192 sites, un warp occupe ≈ 0,6 Mo, soit ≈ 2 Go pour 3000 warps.

**Noyau fusionné ou étages séparés ?**
- **Fusionner** atlas, q3 et q4 dans un seul warp par arête garde l'atlas dans l'ardoise. Pas d'arène d'atlas globale (les frontières retenues à K10 se comptent en Go) et l'ordre d'émission par arête est celui du moteur.
- **Risque : les registres.** S3 seul utilisait 250 registres ; `0b41e4c86` le ramène à 128, non mesuré sur G4. Il faudra mettre les routines i128 en `__noinline__` et vérifier avec ptxas.
- **Repli :** deux noyaux (atlas + q3, puis q4), avec une arène d'atlas par tranche d'arêtes.

## 4. (c) Sortie

**La chaîne ne consomme que cinq champs.** `Presentation` (`tower_chain.cpp:279`) garde clé, arité, support, profondeur et **taille** de coquille. Les ID de coquille ne sont pas utilisés. En aval, la chaîne revérifie profondeur et taille : égalité au sein d'une clé, puis census du catalogue (`chain_census_depth/shell_mismatch`).

**Enregistrements de taille fixe** (≈ 104 octets) : arité u8, 4 ID de support triés, profondeur, taille de coquille, clé 5×i128.
- K5 : 850 k enregistrements, ≈ 88 Mo.
- K10 : 4,63 M enregistrements, ≈ 480 Mo.

**Calculer la clé sur l'appareil.** La clé est une fonction du support (`make_q3`/`make_q4` puis pgcd `primitive`).
- La reconstruire sur l'hôte coûte un pgcd i128 par candidat, de l'ordre de la microseconde. Estimation non mesurée : ≈ 0,2–0,5 s à K10 sur 48 fils, et c'est la piste fermée de « reconstruction hôte ».
- Sur l'appareil, les décisions (census, positivité) se prennent sur les coefficients non réduits : le signe est invariant par diviseur positif, et les bornes < 2^117 valent avant réduction.
- La réduction primitive ne se fait que pour les candidats émis : pgcd binaire, puis division par soustractions successives.

**Pas de passe comptage / préfixe / remplissage pour l'émission.** Elle doublerait le coût dominant ; on la garde pour les étages bon marché (comme le motif drapeau / préfixe / dispersion de S2). À la place :
- chaque warp accumule les enregistrements de l'arête dans son ardoise ;
- en fin d'arête, une seule réservation par `atomicAdd` dans l'arène globale ;
- la publication est donc **atomique par arête**, jamais un préfixe ; si la place manque, l'arête est mise en attente, sans enregistrement ni compteur.

**Déterminisme :** le multiensemble ne dépend pas de l'ordonnancement. Chaque enregistrement porte (ordinal d'arête, ordinal d'émission) ; un tri radix sur l'appareil donne une sortie bit à bit identique quel que soit le nombre de warps. La chaîne retrie de toute façon (`gather_presentations`).

**ID de coquille :**
- l'émulation hôte les produit, et la porte hôte les compare ;
- l'appareil ne rend que le compte et une empreinte indépendante de l'ordre (somme et xor d'un mélange des ID), pour le juge des préflights.

## 5. (d) Portes d'exactitude

Le moteur doit d'abord être restructuré ; la qualification se fait donc en deux temps.

**1. La variante CPU calcule le même objet.** Nouvelles options du moteur, coupées par défaut :
- atlas au niveau des sites, sans budget Z ;
- `WspdQ3CensusMode::ScalarCover` dans la chaîne (mode déjà existant) ;
- graines et domaine positif tirés du cover.

La porte exige, par arête, le même multiensemble trié (arité, clé, support, profondeur, ID de coquille) que le moteur actuel, sur les survivants réels de trois trames à K2/3/5/10. Elle exige aussi les six condensés épinglés par C, et un reçu de coût CPU.

Argument pour l'atlas : **sans budget Z, la classification par blocs est égale à la classification par sites.**
- Les bornes de bloc sont conservatrices.
- Un bloc ambigu descend jusqu'aux singletons, testés exactement.
- Les feuilles étaient de toute façon raffinées complètement.
- Seuls changent la forme de l'arbre (cellules Deep plus tôt) et les compteurs.
- Les sites des fragments restent en ordre de rang, donc les compteurs du census de feuille (arrêt précoce) sont inchangés.

**2. Le port a les mêmes compteurs que la variante.**
- `gpu/lanes_q34.hpp`, avec `HostGroup` et un `BlockGroup` de 128.
- Porte hôte par arête : multiensemble, et compteurs logiques champ par champ.
- **Liste d'exemptions explicite :**
  - `sort_comparisons`, `shell_sort_comparisons` et `hull_sort_comparisons`, qui dépendent de l'implémentation libstdc++ ;
  - les champs d'octets `peak_*`, `retained_*` et de capacité, déjà déclarés non logiques.
- Mutants causaux tués (`--inject`) :
  - départage du propriétaire ;
  - `y<x` canonique ;
  - signe du découpage ;
  - propriété de bord du groupe ;
  - K−2 au lieu de K−1 ;
  - `nth_set_bit` décalé d'une unité ;
  - échange a↔b.
- Planchers non nuls : attentes, ExactLeaf, `whole_atlas_skips`, `boundary_skips`, `groups_without_support`, `clipped_inside`.
- Ardoise réduite : sont mises en attente exactement les arêtes au-delà de la capacité, les autres restent identiques.

**3. Juge sur l'appareil.**
- `judge_lanes_filter` dans les préflights : `Engine::certified_edge` recalcule chaque arête décidée ; multiensemble et compteurs sommés doivent être égaux.
- Préflight à ardoise réduite.
- Identités `check_*` et `validate_completion`.
- Condensés de la chaîne.
- Les `logic_error` actuels deviennent un statut `fault`, qui aboutit à `kInvariantViolated`.

**4. Arithmétique.** Le code d'appareil utilise déjà i128 (`cell_outside`, `box_xi`). Il faut éviter la division i128 :
- dans `scaled_floor`, le quotient vaut −2 à 2 après le test `twice_den` : des comparaisons suffisent ;
- les planchers de `PreparedPower` disparaissent avec ScalarCover ;
- pgcd binaire pour les clés ;
- un selftest arithmétique hôte/appareil aux bornes prouvées (2^117).

## 6. (e) Débit attendu

**Référence CPU (R13, W48) :** phase des survivants 731 ms à K5 (≈ 30 s de fil), 2 922 ms à K10 (≈ 130 s de fil).

**Copie directe au patron S3** (marches uniformes, pas de données résidentes). Avec un warp ≈ 1/17 de fil et 1504 warps : ≈ 0,34 s à K5, ≈ 1,5 s à K10. **Gain ≈ ×2 seulement.**

**Avec la restructuration** (marches remplacées par des balayages du cover, formes recalculées dans l'ardoise, plages résidentes, 16 warps par SM, arêtes lourdes routées) :
- volume ≈ 60–100 M pas de warp à K5, balayages parallèles sur les voies : atlas ≈ 10–20 M ballots, census ≈ 25 M, ≈ 3 M balayages q4 ;
- si un warp atteint 1/3 à 1/6 de fil : **K5 ≈ 50–150 ms, K10 ≈ 0,2–0,6 s** ;
- transferts : 2–4 ms à K5, 10–20 ms à K10 ;
- plus de reconstruction de cover.

**Effet sur la chaîne :** K5 passerait de 2,20 à ≈ 1,6 s. La tour (0,75 s) et les certificats (0,21 s) restent, donc les survivants seuls n'amènent pas à 1 s. La marche parallèle par frontière réduirait aussi S3.

## 7. Parties difficiles et restructurations nécessaires

1. **`Q4LocalAtlas` / `Q4LocalFragment`.** Graphe de `shared_ptr` (fragment parent, géométrie, cover qui possède l'index), `build()` récursif, frontières `std::vector<size_t>` d'ID de nœuds, union étiquetée `Q4LocalPartitionResult`, comptabilité d'octets. À remplacer par une arène plate par warp : cellules indexées et frontières d'ordinaux u32.
2. **Budgets séquentiels.**
   - `z_test_budget=512` est un préfixe séquentiel des tests sur toute la frontière d'entrée ; il bloque le parallélisme sur les voies. Solution : variante sans budget Z, requalifiée.
   - `node_budget` dépend de l'ordre de création DFS, `stop_after` de l'ordre du balayage. Solution : DFS uniforme et `nth_set_bit`.
3. **Census GlobalBoxes.** DFS de l'index **global**, bornes i128, pile de 2,2 Ko par fil, divisions de `PreparedPower`. À remplacer par ScalarCover coopératif.
4. **Quatre marches de l'index global par arête.** Les remplacer par des balayages du cover, ou par des marches parallèles exactes.
5. **`std::sort` avec comparateurs compteurs.** Le tri par rang est sans ambiguïté car l'ordre est total ; les compteurs de comparaisons sont à exempter.
6. **Consommateurs `std::function` et exceptions.** Les remplacer par des enregistrements et des codes de statut.
7. **Sémantique des ID d'origine et orientation de l'arête.** Il faut `order[]` sur l'appareil.
8. **Pression sur les registres.** S3 seul utilisait 250 registres ; mettre les routines i128 hors ligne et prévoir le découpage en étages.

**Suite proposée :**
- **S4.0** — session d'appareil (index et `order[]` résidents, survivants et masques gardés sur l'appareil, plages exportées par S3), plus marches parallèles exactes dans S3 ; reçu G4.
- **S4.1** — variantes CPU et leur qualification d'objet.
- **S4.2** — en-tête portable et porte hôte.
- **S4.3** — noyau, arène à publication atomique par arête, juge dans les préflights, reçu G4.

Fichiers lus :
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gpu/certificate.hpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gpu/filter_runner.cu`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen/pipeline/wspd_q34.cpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/chain/tower_chain.cpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen/lanes/q4_local.cpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen/lanes/q4_local_partition.cpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen/lanes/q3_ball_census.cpp`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/g4_tower_r13_20260923/vm/probe_0.stdout`
- `/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/receipts/g4_tower_r13_20260923/vm/probe_2.stdout`
