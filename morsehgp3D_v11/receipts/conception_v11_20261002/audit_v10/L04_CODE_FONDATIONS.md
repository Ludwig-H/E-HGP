# L04 — Code des fondations de la v10 : audit pour la v11

```text
phase=exploration_v11_hors_registre (audit de la v10)
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_code_fondations
public_status=not_claimed
GCP non utilisé
```

Rédigé le 2 octobre 2026 à 07:01 UTC (heure lue par `date -u` au moment de l'écriture). Auditeur : lentille L04.
Lecture seule sur le dépôt et les dossiers privés ; compilations et calculs sous `/tmp/v11-audit/l04_code_fondations/`.

## 1. Périmètre lu

| Objet | Fichiers | Lignes |
| --- | --- | ---: |
| `src/core` | `types.hpp`, `reasons.def`, `status.hpp`, `buffer.hpp`, `buffer.cpp` | 309 |
| `src/sched` | `pool.hpp`, `pool.cpp`, `sort.hpp` | 213 |
| `src/arith` | `wide.hpp`, `geometry.hpp`, `geometry.cpp` | 408 |
| `src/cloud` | `cloud.hpp`, `cloud.cpp`, `site_tree.hpp`, `site_tree.cpp`, `grid32_primitives.hpp` | 520 |
| `tests/unit` | `unit_main.cpp`, `rank_search.cpp`, `grid32_primitives.cpp` | 687 |
| construction | `CMakeLists.txt`, `cmake/gates.cmake`, `cmake/run_expect.cmake` | 186 |

Les 22 fichiers ont été lus en entier, ligne à ligne, dans `/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10`.

- **Ancrage.** Chaque fichier est identique au commit `afb081774` (`git hash-object` du fichier égal à `git rev-parse afb081774:<chemin>`, 22 sur 22). Pendant l'audit, le HEAD de l'arbre de lecture est passé à `52687f8e5` (ouverture de la v11, 06:11 UTC) ; `git diff --stat afb081774 HEAD` est vide sur `src`, `cli`, `tests`, `cmake`, `CMakeLists.txt` et `reference` de la v10.
- **Usage réel.** Pour juger l'emploi des fondations, j'ai lu leurs sites d'appel : `src/catalogue/generator.cpp` (lignes 1–250 et 620–876), `src/catalogue/catalogue.hpp`, `src/tower/tower.cpp` (14–235, 436–556, 800–935, 1150–1166, 1596–1616), `src/tower/rank_search.hpp`, et les trois sondes de `cli/`.
- **Raccord R2.** `morsehgp3D_v11/docs/PROVENANCE.md` déclare que les fondations de la v11 seront portées depuis le raccord R2 privé (`build/v10-integration-r2/src`, commit `865f5e6`), jamais importé dans `main`. J'ai donc aussi lu ses versions de `pool.{hpp,cpp}`, `site_tree.{hpp,cpp}`, `reasons.def`, `fp_strict.hpp`, `u32le_input.hpp`, l'en-tête de `cli_output.hpp` et le début de son `CMakeLists.txt`, et comparé les deux arbres. Sont **inchangés** dans R2 : `wide.hpp`, `geometry.{hpp,cpp}`, `cloud.{hpp,cpp}`, `grid32_primitives.hpp`, `buffer.{hpp,cpp}`, `status.hpp`, `sort.hpp`, `gates.cmake`, `run_expect.cmake`. Mes constats sur ces fichiers valent donc aussi pour la source déclarée du port.
- **Pistes (jamais des preuves).** `receipts/audit_geant_developpeur_20260930/TRACKER.md` (MM1, PL1), `audits/audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md`, `receipts/development_frontier_precision_20260930/precision_math/AUDIT.md`, `receipts/pool_race_fix_20260929`, reçus G4 des sessions 4 et 5. Chaque point repris ci-dessous a été rejoué ou recalculé par moi ; quand ce n'est pas le cas, c'est dit.

## 2. Méthode

1. **Lecture** intégrale, puis relecture des sites d'appel pour chaque précondition trouvée.
2. **Constructions sous `/tmp`** (au plus 3 fils de compilation) : GCC 13.3 Release ; GCC ASan + UBSan + `MHGP10_POISON` ; GCC TSan (lancé par `setarch -R`) ; Clang 18 Release. Zéro avertissement dans les quatre. Exécution des trois portes unitaires dans chacune : toutes rendent le code 0 (`unit_ok`, `rank_search_ok`, `grille32_ok`).
3. **Avertissements masqués** : recompilation des fondations avec un jeu étendu sous GCC (`-Wconversion -Wsign-conversion -Wshadow -Wold-style-cast -Wcast-qual -Wuseless-cast -Wdouble-promotion -Wfloat-equal -Wnull-dereference -Wduplicated-cond -Wlogical-op -Wformat=2`…) et `-Weverything` sous Clang ; assainisseur d'entiers de Clang (`unsigned-integer-overflow`, `implicit-conversion`) ; ajout de `[[nodiscard]]` dans une copie pour révéler les résultats ignorés.
4. **Juge indépendant de l'arithmétique** : un harnais C++ (`harness.cpp`) appelle chaque prédicat de `geometry.hpp` ; un script Python (`oracle.py`) le juge par ses **propriétés de définition** en fractions exactes (équidistance, coplanarité, signe d'un déterminant rationnel, rayon), pas par une recopie des formules. Configurations : tous les triplets ordonnés de coins du cube, coins perturbés, tirages uniformes, cas quasi dégénérés. Rejoué à 18, 19, 20, 21, 24 et 32 bits, avec et sans UBSan.
5. **Démonstrations ciblées** : centre lointain pour `SiteTree` ; exception dans le `Pool` ; grain de $2^{63}$ ; tri parallèle contre `std::sort` ; `ulimit -v` ; options flottantes.
6. **Juges d'échantillon à l'échelle** sur les trames LiDAR du contrat (08/000000, 08/000100, 08/000200 sans sol : 39 885, 35 551 et 45 845 points) : requêtes de `SiteTree` contre la force brute exacte ; tour complète sous ASan + UBSan + tampons empoisonnés, sous TSan, et à 1, 2, 3 et 4 fils, comparée par empreinte au build de base.
7. **Compteurs déterministes** (machine partagée, charge de 10 à 18 sur 8 cœurs : aucun temps absolu local n'est une mesure) : appels de `parallel_for` par une copie instrumentée du `Pool`, défauts de page et commutations (`/usr/bin/time -v`), appels système (`strace -c`), comparaisons du tri, octets comptés par `MemoryBudget`. Les temps cités viennent des reçus G4.
8. **Mutants** : 18 mutations d'une ligne des fondations, appliquées à une copie, jugées par la seule porte `mhgp10_unit` ; puis les survivants de géométrie rejoués contre les portes d'oracle (réduites à 10 et 8 nuages) et, pour l'un, contre mon juge et sur un quart de trame.
9. **Raccord R2** : copie sous `/tmp`, porte `mhgp10_unit` reconstruite et exécutée (code 0), et mes démonstrations rejouées contre ses `pool.cpp` et `site_tree.cpp`.

Scripts, sources des harnais et journal d'exécution : `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l04_code_fondations/` (`JOURNAL_L04.txt` porte les sorties brutes, sections 1 à 41).

**Incidents de mon fait.** Un harnais de test (pas le code audité) contenait une boucle infinie ; j'ai arrêté ce seul processus, par son nom exact. Ma campagne de mutants contre les oracles est lente (un mutant fait boucler la tour jusqu'au délai de 600 s) ; une tentative d'arrêt a été refusée par le système de permissions, je l'ai laissée finir.

## 3. Verdict en bref

- **Juste dans son contrat.** À coordonnées u18, tous les prédicats exacts, le nuage, les requêtes de `SiteTree` (centre dans la boîte des sites) et le tri parallèle (ordre total) rendent le résultat exact. Aucun défaut ne rend faux un résultat publié de la tour : je n'ai trouvé **aucun constat bloquant** dans ce périmètre.
- **Contrats non tenus ou non gardés.** Le budget mémoire n'est raccordé à rien ; une `std::bad_alloc` tue le processus ; `SiteTree` rend des résultats faux pour un centre lointain, sans le dire ; la validité de plusieurs prédicats repose sur une convention que rien ne vérifie (forme du centre, retours de débordement ignorés) ; la porte unitaire ne juge pas la géométrie, et un défaut du mot de poids fort du comparateur de niveaux traverserait toutes les portes.
- **R2 corrige une partie** (exceptions du pool, réclamation saturante, garde de domaine de `SiteTree`) ; il ne touche ni au budget mémoire, ni à l'arithmétique, ni au tri, ni au coût de l'ordonnancement, et il ajoute une machinerie de refus d'options flottantes que la doctrine de la v11 rend inutile.
- **Pour 100 ms à 48 fils**, les fondations pèsent : sur G4, le catalogue K = 5 n'emploie qu'environ les trois quarts du temps de fil disponible, les phases hors boîtes coûtent 59 ms sur 171, la préparation en série (6,5 à 8,3 ms) n'entre pas dans les sommes publiées.

## 4. Constats

Gravités : **bloquant** (rend faux ou invalide un résultat ou un contrat), **majeur** (à traiter dans la conception de la v11), **mineur**, **info**. Chaque constat dit comment il a été établi : *lu*, *exécuté*, *mesuré* ; ce qui est seulement conjecturé est marqué. Sauf mention, le sujet est HEAD `afb081774` ; l'état du raccord R2 (`865f5e6`) est donné quand il diffère.

### C01 — Le budget mémoire n'est raccordé à rien (majeur)

- **Affirmation du code.** `src/core/buffer.hpp:1-2` : « tout grand tableau passe par `Buffer<T>`, réservé dans un `MemoryBudget` AVANT l'allocation (le budget est honnête par construction ; la v4 avait un plafond menteur) ». `PASSATION.md` (« Carte ») reprend « tampons comptés ».
- **Aucun plafond (lu).** Un `MemoryBudget` à limite finie n'est construit qu'en test (`tests/unit/unit_main.cpp:211`). Tous les `allocate` de `src/tower/tower.cpp` (lignes 85, 1339, 1342, 1346-1347, 1408) prennent le budget par défaut, illimité (`buffer.hpp:19`, `buffer.cpp:5-8`), qui est un état global. `peak()`, `used()` et `limit()` ne sont lus par aucun code produit ni aucune sonde. Aucune option de ligne de commande ne fixe un plafond.
- **Couverture (lu).** `Buffer<T>` porte le nuage (`cloud.hpp:20-22`) et dix tableaux déclarés dans `tower.cpp` (lignes 127, 704-705, 746-750, 753, 1341). Tout le catalogue est en `std::vector` (`UninitVector`, `catalogue.hpp:55` et `:69-78`) ; `generator.cpp` cite 38 fois `std::vector`, `tower.cpp` 55 fois ; `SiteTree` en a cinq (`site_tree.hpp:52-54`) ; `prepare_cloud` en alloue trois (`cloud.cpp:34, 39, 41`) ; le tri alloue n éléments à chaque appel (`sort.hpp:53`).
- **Mesuré** (`counted.cpp`, trame 02, K = 5, 4 fils) : pic compté après le catalogue 1 100 284 octets ; mémoire résidente au même instant 412 616 Kio. Le budget voit **0,26 %** du pic. En fin de tour le pic compté atteint 121 672 902 octets (les tampons de la tour).
- **Exécuté.** Sous `ulimit -v 300000`, `mhgp10_tower` sur la trame 02 finit par SIGABRT (code 134, `terminate called after throwing an instance of 'std::bad_alloc'`), sans ligne de statut. La raison `memory_budget` n'est rendue que si l'`operator new(nothrow)` d'un `Buffer` échoue.
- **R2.** `buffer.hpp` identique. R2 convertit `std::bad_alloc` en `memory_budget` aux points d'entrée (`generator.cpp:925-932`, `tower.cpp:1812-1819` de R2) : la raison signifie alors « l'allocateur a échoué », toujours pas « le plafond est atteint ».
- **Piste recoupée.** MM1 du TRACKER de l'audit géant (0,02 à 0,37 % du pic) : confirmé par ma mesure.
- **Conséquence pour la v11.** `ARCHITECTURE.md` § 1, règles 3 et 8, reprend la même promesse. Elle ne vaut que si (a) le budget appartient à la `Session`, sans instance globale illimitée ; (b) une porte échoue quand la part comptée du pic résident tombe sous un plancher ; (c) l'admission d'un étage se fait par formule, avant d'allouer, avec le même statut à 1 et à 48 fils. Sinon, ne pas écrire « honnête par construction ».

### C02 — Une exception dans une tâche tue le processus (majeur à HEAD ; corrigé dans R2)

- **Lu.** `src/sched/pool.cpp:28-37` exécute le corps sans garde. Dans un ouvrier, l'exception sort de `worker_loop` (`:39-57`) : `std::terminate`. Dans le fil appelant, elle sort de `parallel_for` (`:80`) alors que `current_` pointe encore sur le descripteur de la pile et que des ouvriers le lisent ; `tls_in_region` (`:29-36`) n'est pas restauré. `src/` et `cli/` ne contiennent aucun `try` (0 occurrence), alors que les corps parallèles font croître des `std::vector` (`generator.cpp:50-63`).
- **Exécuté** (`exc.cpp`). Exception levée dans un ouvrier : code 134 (SIGABRT). Dans le fil appelant : code 139 (SIGSEGV) en Release, `stack-use-after-scope` sous ASan.
- **Lu, non exécuté.** `pool.cpp:13-17` : si la création d'un fil lève `std::system_error`, les `std::thread` déjà créés sont détruits joignables (`std::terminate`).
- **R2** (`pool.cpp` de R2, lignes 64-78 et 100-135) : la première exception est capturée dans le descripteur, le compteur de tranches est porté à n, `parallel_for` attend tous les fils puis relance ; `make_pool` rend `resource_exhausted/session_overhead`. **Rejoué** : mes deux modes rendent la main, le second travail s'exécute en entier (1 000 tranches sur 1 000), ASan propre.
- **Piste recoupée.** PL1 du TRACKER.
- **Conséquence pour la v11.** Porter le `Pool` de R2, pas celui de HEAD. Réserve : « première exception » veut dire première dans le temps ; si deux raisons différentes peuvent se produire, la réduction doit choisir par ordinal de tranche pour rester déterministe.

### C03 — Grain de $2^{63}$ : une tranche exécutée deux fois (mineur ; corrigé dans R2)

- **Lu.** `pool.cpp:32` réclame par `fetch_add(grain)` ; après épuisement chaque fil ajoute encore `grain`, et le compteur reboucle modulo $2^{64}$ quand `n + P * grain` dépasse $2^{64}$.
- **Exécuté** (`grain.cpp`). `parallel_for(2, 2^63, …)` à 2 fils : la tranche `[0, 2)` s'exécute deux fois dans un même appel. Les grains du produit sont 1, 16, 32, 64, 256, 1 024, 4 096 et `kChunk` : le défaut n'est pas atteignable aujourd'hui.
- **R2.** Réclamation saturante par échange comparé ; rejoué : une seule exécution.

### C04 — `SiteTree` : requêtes à centre rationnel fausses hors de la boîte des sites (majeur)

- **Affirmation du code.** `src/cloud/site_tree.hpp:31-39` : « marge prouvée large (erreur < 1e-3 en u18) ; toutes les décisions sur les sites sont exactes », sans condition sur le centre.
- **Lu.** `site_tree.cpp:64` fixe `kMargin = 0.02`. `closed_ball` classe un site strictement intérieur sans clé exacte quand sa distance approchée passe sous `r2a - kMargin` (`:208-211`) et l'écarte au-dessus de `r2a + kMargin` (`:206`) ; `nearest` élague sur `W + kMargin` (`:150, :155, :158`). La validité repose sur une erreur absolue inférieure à 0,01, vraie seulement si le centre est près de la boîte.
- **Exécuté** (`far_center.cpp`). Sites a = (0, 0, 0), b = (100 000, 1, 0), c = (200 000, 0, 0) : centre circonscrit à 5·10⁹ du nuage. Sur 15 561 sites u18, `closed_ball` **perd 228 des 6 786 sites strictement intérieurs** ; `nearest(count = 2)` rend un mauvais site. Juge : `geom::side_key` sur tous les sites.
- **Mesuré** (`float_error_model.py`, modèle exact des doubles). Erreur absolue de la distance carrée approchée : 5 663 à ce rayon, 3 à 10⁸ ; elle croît comme $2^{-53} \vert c \vert^{2}$. Centre dans la boîte : 5,2·10⁻⁵ au plus à u18 sur 33 850 centres.
- **Prouvé** (ma dérivation, centre dans le cube, arrondi au plus proche, $u = 2^{-53}$, $L = 2^{B}$ : centre approché à $3{,}51 \, u L$ près, différence à $4{,}01 \, u L$ près, distance carrée à $28{,}6 \, u L^{2}$ près) : erreur inférieure à $2^{-12} \approx 2{,}44 \cdot 10^{-4}$ à u18, multipliée par 4 à chaque bit : 9,8·10⁻⁴ à u19, 3,9·10⁻³ à u20, **1,56·10⁻² à u21, au-dessus de 0,01**. La marge fixe vaut donc jusqu'à u20, pas au-delà.
- **Portée à HEAD.** Les appelants sont dans le domaine : `tower.cpp:887` et `:898` (centre d'une MEB certifiée, donc dans l'enveloppe convexe) et `:1610` (centre sur un site). Aucun résultat de la tour n'est touché. Juge d'échantillon sur trames entières (`frame_judge.cpp`) : 4 434 sphères sur la trame 00 et 4 416 sur la trame 02 (centre dans l'enveloppe), 0 désaccord ; `kth_distance` 3 000 sur 3 000, `within` 600 sur 600.
- **Coût structurel (compté, copie instrumentée, trames 00 et 02).** `nearest` centré sur un site, k = 5 : 21 à 24 nœuds visités, 35 à 44 sites balayés, 11 à 12 candidats, 5 clés exactes ; k = 10 : 26 à 29 nœuds, 52 à 64 sites, 10 clés exactes. `closed_ball` d'une sphère locale à centre dans l'enveloppe : 17 à 19 nœuds, 20 à 25 sites balayés, **2,2 clés exactes par sphère, soit exactement les sites de la coquille** ; tout intérieur est classé par le raccourci flottant. L'arithmétique exacte ne pèse donc presque rien dans ces requêtes ; le coût est le parcours.
- **Angle mort de la porte.** `tests/unit/unit_main.cpp:326-387` tire des centres circonscrits quelconques, donc parfois hors boîte, et passe : rien n'y vise le régime lointain.
- **R2.** Garde `filtered()` (centre dans le cube, bornes de représentation, `FE_TONEAREST`) et repli exact en O(n) par requête. **Rejoué** : mon contre-exemple passe (0 site perdu). Limites : marge fixe liée à u18, repli linéaire, hypothèses de compilation et d'arrondi.
- **Piste recoupée.** Constat G1 des auditeurs (29 septembre).
- **Conséquence pour la v11.** Le filtre de R2 n'est pas portable tel quel : (a) `ARCHITECTURE.md` § 3 exige que B = 21 et B = 24 compilent des mêmes sources, et 0,02 ne vaut plus à 21 bits ; (b) la doctrine F3 n'admet que des sommes de termes de même signe, or le filtre calcule la différence `x - cq` d'une coordonnée et d'un centre approché. Voir R2 de la section 8.

### C05 — Arithmétique exacte : juste à u18, avec deux bits de marge (info, acquis)

- **Exécuté** (`harness.cpp`, `oracle.py`). À 18 bits, 18 016 configurations (tous les triplets ordonnés de coins du cube, coins perturbés, tirages uniformes, cas quasi alignés et quasi coplanaires) : `center2`, `center3`, `center4`, `side`, `side_key`, `orient`, `orient_center` (forme q4), `orient_center_wide`, `strictly_inside_tetra`, `acute`, `is_midpoint`, `level2`, `level3`, `level4`, `compare` : **0 désaccord** avec le juge en fractions exactes ; sous UBSan (14 016 configurations), **0 rapport**. Même résultat à 19 et 20 bits (12 016 configurations chacun, sous UBSan).
- **Premiers débordements.** À 21 bits : `side_key` de forme q3 (1 486 désaccords sur 23 642, UBSan `geometry.hpp:93, 99, 100`), `orient_center` q4 (UBSan `geometry.hpp:120`), `level4` (70 sur 10 341 : `D * D` en `u128`, `geometry.cpp:88`). À 24 bits : `level4` 6 421 sur 10 344, `side_key` q3 11 385 sur 23 652, `orient_center` q4 3 261, `strictly_inside_tetra` 904. À 32 bits presque tout, à commencer par `dot` et `cross` en `i64` (`geometry.hpp:21-24`).
- **Budget recalculé.** Soit $M = 2^{B} - 1$ et des sites dans le cube $[0, M]^{3}$ ; $u = b - a$, $v = c - a$, $s = d - a$, $w = u \times v$. Trois lemmes : (L1) $\vert u_{i} \vert \leq M$ et $\vert u \vert^{2} \leq 3 M^{2}$ ; (L2) $\vert w_{i} \vert \leq M^{2}$ (double de l'aire d'un triangle projeté dans un carré de côté $M$) et $\vert w \vert^{2} \leq 3 M^{4}$ ; (L3) $\vert \det(u, v, s) \vert \leq 2 M^{3}$ (six fois le volume d'un tétraèdre inscrit dans le cube). Les bornes de (L2) et (L3) sont atteintes. Bits de magnitude nécessaires ($\log_{2}$ de la borne, **X** = le conteneur de la v10 ne suffit plus) :

| Quantité | Borne | Conteneur v10 | B=18 | B=20 | B=21 | B=24 | B=32 | Plus grand B sûr |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `dot` | $3 M^{2}$ | `i64` (63) | 37,6 | 41,6 | 43,6 | 49,6 | 65,6 **X** | 30 |
| `cross`, composante | $M^{2}$ | `i64` (63) | 36,0 | 40,0 | 42,0 | 48,0 | 64,0 **X** | 31 |
| `orient` | $2 M^{3}$ | `i128` (127) | 55,0 | 61,0 | 64,0 | 73,0 | 97,0 | 41 |
| `center3` : $N_{i}$ | $12 M^{5}$ | `i128` | 93,6 | 103,6 | 108,6 | 123,6 | 163,6 **X** | 24 |
| `center3` : $D$ | $6 M^{4}$ | `i128` | 74,6 | 82,6 | 86,6 | 98,6 | 130,6 **X** | 31 |
| `center4` : $N_{i}$ | $9 M^{4}$ | `i128` | 75,2 | 83,2 | 87,2 | 99,2 | 131,2 **X** | 30 |
| `center4` : $D$ | $4 M^{3}$ | `i128` | 56,0 | 62,0 | 65,0 | 74,0 | 98,0 | 41 |
| `side_key` q3 | $90 M^{6}$ | `i128` | 114,5 | 126,5 | 132,5 **X** | 150,5 **X** | 198,5 **X** | **20** |
| `side_key` q4 | $66 M^{5}$ | `i128` | 96,0 | 106,0 | 111,0 | 126,0 | 166,0 **X** | 24 |
| `orient_center` q4, somme | $39 M^{6}$ | `i128` | 113,3 | 125,3 | 131,3 **X** | 149,3 **X** | 197,3 **X** | **20** |
| `orient_center_wide` q3 : $cc_{i}$ | $18 M^{5}$ | `i128` | 94,2 | 104,2 | 109,2 | 124,2 | 164,2 **X** | 24 |
| `orient_center_wide` q3, somme | $54 M^{7}$ | `Wide<4>` (256) | 131,8 | 145,8 | 152,8 | 173,8 | 229,8 | 35 |
| `is_midpoint` q3 | $36 M^{5}$ | `i128` | 95,2 | 105,2 | 110,2 | 125,2 | 165,2 **X** | 24 |
| `level3` : numérateur | $27 M^{6}$ | `I192` (192) | 112,8 | 124,8 | 130,8 | 148,8 | 196,8 **X** | 31 |
| `level3` : dénominateur | $12 M^{4}$ | `u128` (128) | 75,6 | 83,6 | 87,6 | 99,6 | 131,6 **X** | 31 |
| `level4` : numérateur | $243 M^{8}$ | `I192` | 151,9 | 167,9 | 175,9 | 199,9 **X** | 263,9 **X** | 23 |
| `level4` : dénominateur | $16 M^{6}$ | `u128` | 112,0 | 124,0 | 130,0 **X** | 148,0 **X** | 196,0 **X** | **20** |
| `compare`, produit croisé | $3888 M^{14}$ | `Wide<5>` (320) | 263,9 | 291,9 | 305,9 | 347,9 **X** | 459,9 **X** | 22 |

- **Recoupement.** Longueurs maximales observées à 18 bits : $N$ de q3 92 bits, $D$ de q3 75, `side` q3 113, $D$ de q4 56, `side` q4 94, `level4` 148 et 112 : sous les bornes, et à 1 bit près pour celles qui sont atteintes. La table prédit exactement les premiers échecs observés (21 bits : les trois lignes à 20).
- **Lecture.** Le moteur est exact jusqu'à **u20**, pas seulement u18. Les bornes écrites en tête de `geometry.hpp:4-8` (différences majorées par $2^{19}$) sont justes mais lâches. Mes bornes recoupent, à 1 à 3 bits près, l'audit de largeur du 30 septembre (`precision_math/AUDIT.md`), qui n'emploie pas (L2) ni (L3).
- **Conteneurs par profil** (bornes mécaniques terme à terme, sans les lemmes, telles qu'un calcul `constexpr` par expression les produirait ; `budget_mecanique.py`) :

| Quantité | B=18 | B=21 | B=24 | B=32 |
| --- | --- | --- | --- | --- |
| `side_key` q3 | `i128` (115,8) | `Wide<3>` (133,8) | `Wide<3>` (151,8) | `Wide<4>` (199,8) |
| `side_key` q4 | `i128` (97,2) | `i128` (112,2) | `Wide<3>` (127,2) | `Wide<3>` (167,2) |
| `orient_center` q4 | `i128` (115,5) | `Wide<3>` (133,5) | `Wide<3>` (151,5) | `Wide<4>` (199,5) |
| `orient_center` q3 | `Wide<3>` (134,2) | `Wide<3>` (155,2) | `Wide<3>` (176,2) | `Wide<4>` (232,2) |
| centres q3 : $N$, $D$ | `i128`, `i128` | `i128`, `i128` | `i128`, `i128` | `Wide<3>`, `Wide<3>` |
| `level3` : numérateur, dénominateur | `i128`, `i128` | `Wide<3>`, `i128` | `Wide<3>`, `i128` | `Wide<4>`, `Wide<3>` |
| `level4` : numérateur, dénominateur | `Wide<3>`, `i128` | `Wide<3>`, `Wide<3>` | `Wide<4>`, `Wide<3>` | `Wide<5>`, `Wide<4>` |
| `compare` (q4 contre q4) | `Wide<5>` (269,1) | `Wide<5>` (311,1) | `Wide<6>` (353,1) | `Wide<8>` (465,1) |

- **Conséquence pour la v11.** Ces deux tables sont le budget à graver dans `num` (règle 10 de l'architecture). À B = 32, `mul` de `Wide<5>` par `Wide<4>` demande `Wide<9>`, que `wide.hpp:15` interdit, alors que la valeur tient sur 8 mots : il faut un produit borné explicite.

### C06 — L'exactitude des prédicats tient à des conventions que rien ne vérifie (majeur)

- **Retours de débordement ignorés (exécuté).** `arith::add`, `sub` et `resize` rendent `false` en cas de débordement (`wide.hpp:87-108, 127-138`) sans `[[nodiscard]]`. En l'ajoutant dans une copie, le compilateur désigne quatre sites : `src/arith/geometry.cpp:46, 69, 81, 83`. À u18 aucun ne déborde (C05) ; à u24 le résultat est un niveau faux, **sans signal** (6 421 `level4` faux sur 10 344).
- **Troncatures silencieuses (lu).** `Wide<1>::from_u128` jette le mot haut (`wide.hpp:19-24`), employé à `geometry.cpp:69` et `tower.cpp:998`. `level4` calcule $D^{2}$ en `u128` non signé, qui reboucle sans comportement indéfini, donc sans UBSan (`geometry.cpp:88`).
- **Forme du centre non typée (exécuté).** `geom::Center` (`geometry.hpp:26-29`) ne dit pas s'il vient de `center2`, `center3` ou `center4`. Or `orient_center` et `strictly_inside_tetra` ne valent que pour la forme q4 (commentaire `geometry.hpp:112-113`) et `level4` de même (dénominateur). Appliqué à une forme q3 à u18, `orient_center` rend **103 signes faux sur 13 768** (UBSan `geometry.hpp:120`). Les appelants respectent la convention : `generator.cpp:171-178` (sous `qgen >= 4`), `:214-217`, `:456-459`, `tower.cpp:215-216` ; rien ne l'impose.
- **Domaine mal borné (lu).** `cloud.hpp:16` et `cloud.cpp:28` acceptent jusqu'à 21 bits, et `site_tree.hpp:3` annonce 21 bits, alors que les prédicats ne sont exacts que jusqu'à 20. La seule garde est le refus `parameter_out_of_range` de `generator.cpp:636` et de `tower.cpp:1156`. `geometry.hpp` n'a aucun `static_assert` lié à `kCoordinateBits` (seul `generator.cpp:33` en porte un, pour le filtre des nœuds).
- **Raison morte.** `arith_guard` (`reasons.def:18`) n'est jamais émise.
- **R2.** Fichiers identiques.
- **Conséquence pour la v11.** `num::Int<bits>` (§ 3 de l'architecture) répond au premier point ; il faut en plus des types distincts par forme de centre (ou un budget par forme porté par le type), `[[nodiscard]]` sur toute opération qui peut refuser, et aucune conversion qui tronque.

### C07 — La porte unitaire ne juge pas la géométrie (majeur)

- **Exécuté** (`run_mutants.py`, 17 mutations d'une ligne, plus M18 ci-dessous ; juge = `mhgp10_unit` seul). Tués : arithmétique large (retenue du produit, emprunt), `SiteTree` (marge nulle, bande supprimée, troncature avant le tri exact, multiplicités ignorées), nuage (départage par `PointId`), `Pool` (tranche débordante), `Buffer` (libération), `Outcome::precedes`. **Survivent** : M2 `center3` avec $D$ doublé, M3 `center4` avec un signe faux, M4 dénominateur de `level3`, M5 signe de `orient_center_wide`, M6 `compare` sans produits croisés, M14 axes de Morton échangés.
- **Cause (lu).** `test_site_tree_rational` juge l'arbre par `geom::side_key` lui-même (`unit_main.cpp:364`) ; aucun test n'appelle `level2/3/4`, `compare`, `orient_center*`, `strictly_inside_tetra`, `acute`, `is_midpoint`. `README.md` attribue pourtant `src/arith` à la porte `mhgp10_unit`.
- **Exécuté, en aval** (`run_mutants_oracle.sh`, oracles réduits à 10 et 8 nuages). M2, M3, M4 et M6 sont tués par l'oracle du catalogue (31, 15, 30 et 31 échecs sur 41) et par celui de la tour (délai de 600 s pour M2 : la tour boucle ; 13, 17 et 17 échecs sur 25). M5 passe l'oracle du catalogue (0 échec sur 41 : le catalogue ne lit que l'égalité à zéro) et n'est tué que par la tour (4 sur 25). M14 (axes y et z de la clé de Morton échangés) **survit aux deux oracles** (0 échec sur 41 et sur 25) : ils comparent en coordonnées, donc la convention d'ordre des sites n'est gravée par aucune porte, alors que l'indice de site publié en dépend.
- **Angle mort de magnitude (lu, puis exécuté).** Les nuages des oracles ont des coordonnées au plus égales à 1 000 (`tests/oracle/test_catalogue_oracle.py`, fonction `clouds`) ; le contrôle de translation décale de 200 000 un nuage d'étendue 20 000. Aucune porte n'exerce un prédicat près de la borne u18. Démonstration : le mutant M18, qui fait ignorer au comparateur de niveaux son mot de poids fort (`wide.hpp:52`, entiers de 5 mots), **survit à tout** : `mhgp10_unit` (code 0), oracle du catalogue (0 échec sur 41), oracle de la tour (0 sur 25), et le dump de la tour sur un quart de trame LiDAR reste identique. Seul mon juge à grande magnitude le tue (1 désaccord sur 15 928 comparaisons de deux niveaux de tétraèdres, produits croisés de 260 bits). C05 comble ce trou pour HEAD.
- **Défauts de harnais (lu, exécuté).** `unit_main.cpp:401` rend le code 3 dès 1 000 échecs : un désaccord massif (M7, M8, M10) sort avec le code du « plancher ». `sched::parallel_sort` n'a aucune porte (aucune occurrence dans `tests/`).
- **R2.** Même angle mort : ses campagnes de mutants visent le pool, les filtres flottants et les juges, pas les prédicats.
- **Conséquence pour la v11.** Une porte `unit` de `num` avec un juge indépendant (fractions exactes, propriétés de définition) aux extrêmes de chaque profil B, 12 s dans ma version ; des mutants de code des prédicats dans `tests/mutants/` ; le code 3 réservé aux planchers.

### C08 — Ordonnanceur : juste, mais mal placé pour 100 ms à 48 fils (majeur)

- **Conception (lu).** Tous les ouvriers dorment sur une `condition_variable` entre deux régions (`pool.cpp:45`) ; chaque `parallel_for` les réveille tous (`:79`), chacun passe par l'unique mutex pour capturer le travail (`:44-49`) puis pour le rendre (`:53-54`), et l'appelant reprend ce mutex pour fermer (`:81-83`). Le corps est un `std::function` (`pool.hpp:31`). Tranches distribuées par un compteur atomique partagé ; aucune affinité de fils. R2 garde cette conception.
- **Courses et terminaison (exécuté).** TSan propre sur `mhgp10_unit` (dont 100 000 travaux courts enchaînés), sur mon test du tri, et sur la chaîne complète `mhgp10_tower` (quart de trame, 7 069 points, K = 5, 3 fils : 0 rapport, dump identique). La correction du 29 septembre (descripteur par appel, capturé sous verrou, `pool.cpp:67-78`) tient à la lecture : un ouvrier ne lit jamais un descripteur qu'il n'a pas capturé, et son dernier accès précède la sortie de l'appelant.
- **Déterminisme (exécuté).** Dumps de la tour identiques à 1, 2, 3 et 4 fils. La règle de `pool.hpp:3-5` (écriture à des positions fixées par l'ordinal) n'est pas suivie à la lettre : le catalogue accumule par fil (`locals[wk]`, `generator.cpp:681, 718-720, 739-743`) et c'est le tri canonique qui rétablit l'ordre.
- **Compté** (copie instrumentée, trame 02, K = 5, 4 fils, sans points) : 51 régions parallèles par construction, 69 789 tranches, 0 appel imbriqué, 1 région d'une seule tranche.
- **Mesuré sur G4** (reçu `g4_session4_j2c_20260929`, `cmd/001` et `cmd/005`, catalogue de la trame 02 à K = 5) :

| Fils | Mur (s) | CPU user + sys (s) | Défauts de page | Commutations volontaires | frontier (s) | boxes (s) | order (s) | assemble (s) | dont tri (s) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 4,040 | 3,95 + 0,09 | 145 264 | 3 | 0,0559 | 3,5085 | 0,1639 | 0,3071 | 0,1096 |
| 12 | 0,384 | — | — | — | 0,0166 | 0,2913 | 0,0270 | 0,0425 | 0,0172 |
| 24 | 0,259 | — | — | — | 0,0216 | 0,1805 | 0,0214 | 0,0277 | 0,0126 |
| 48 | 0,171 | 6,04 + 0,31 | 144 694 | 2 785 | 0,0266 | 0,1033 | 0,0138 | 0,0187 | 0,0072 |

- **Lecture.** (a) Les boîtes passent bien à l'échelle (×34 sur 24 cœurs à deux fils). (b) La frontière est **plus lente à 48 fils qu'à 12** (26,6 ms contre 16,6). (c) À 48 fils, les phases hors boîtes coûtent 59 ms sur 171. (d) Le taux d'emploi des fils est de 73 à 77 % (6,35 s de CPU pour 0,18 s de mur du processus, 0,171 s pour le catalogue, × 48). (e) 2 785 commutations volontaires, soit environ 59 mises en sommeil par ouvrier pour un seul catalogue ; localement, à 48 fils sursouscrits et sous `strace`, je compte 71 appels `futex` par région à corps triviaux (indicatif). (f) L'inflation du CPU utilisateur (×1,53) est celle du SMT, pas de l'attente active.
- **Borne.** À emploi parfait des fils, le catalogue seul coûterait 6,35 / 48 = 132 ms. Un ordonnancement parfait ne rendrait donc qu'environ 40 ms sur 171 ; le reste est du travail. Les 100 ms exigent les deux : moins de travail, et des phases de liaison parallèles.
- **Non mesuré.** Le coût mural d'un réveil à 48 fils. Ma machine était chargée (10 à 18 sur 8 cœurs) et mes micro-mesures locales ne valent rien ; je ne conclus pas sur ce point.

### C09 — Tri parallèle : exact, mais chaque élément est trié deux fois (mineur)

- **Lu.** `src/sched/sort.hpp:26-28` trie P blocs ; `:54-62` recopie chaque seau puis le **retrie** par `std::sort`, alors qu'il est formé de P suites déjà triées ; `:53` alloue n éléments à chaque appel, hors budget.
- **Exécuté** (`psort.cpp`). 180 cas (P = 2, 3, 4, 7, 8 ; n de 8 191 à 400 000 ; six motifs) : résultat identique à `std::sort` pour un ordre total ; TSan propre. Pour un ordre non total, le résultat dépend de P : c'est le contrat écrit (`sort.hpp:3-4`), et le seul appelant fournit un ordre total hors boule émise deux fois, refusée ensuite (`generator.cpp:773-776, 836`).
- **Mesuré** (compteur de comparaisons, n = 400 000). Rapport parallèle sur séquentiel : 1,63 à 1,88 à 8 fils, **1,88 à 2,93 à 2 fils**. À deux fils le tri parallèle fait donc plus de deux fois le travail du tri séquentiel.
- **G4.** 7,2 ms à 48 fils pour 1,4 million de références (×15,2 seulement).
- **Conséquence pour la v11.** Un tri par échantillonnage sans tri préalable des blocs, ou une fusion à P voies, ou un tri par base sur les bits de la clé ; tampon de travail pris dans le budget et réutilisé ; une porte.

### C10 — Préparation en série, absente des sommes publiées (majeur)

- **Lu.** `src/cloud/cloud.cpp:31-46` : contrôle du domaine, tri des `PointId`, clés de Morton et tri indirect, en série. `src/cloud/site_tree.cpp:9-61` : construction récursive en série. Les coordonnées des sites sont de plus recopiées en série par chaque couche, sous quatre formes (`u32` par axe dans le nuage ; `double` par axe dans `site_tree.cpp:15-22` ; `P3` en `i64` dans `generator.cpp:642-651`, deux fois, et dans `tower.cpp:1165-1166`).
- **G4** (reçu session 4). `prepare_s` vaut 6,5 à 8,3 ms pour la sonde de la tour (nuage, création du pool, `SiteTree`) et 3,1 à 3,7 ms pour celle du catalogue. **Mesuré ici** (`prep_bench.cpp`, minimum de 25 répétitions, machine chargée) : `prepare_cloud` 4,2 à 6,0 ms dont 2,7 à 3,4 ms de tri indirect ; `SiteTree` 6,7 à 8,0 ms. Un tri par base en série (cinq passes de 11 bits, `radix_bench.cpp`) rend la même permutation en 0,9 à 1,1 ms, soit 3 à 4,5 fois moins (indicatif).
- **Lu.** Les temps « catalogue + tour » publiés (0,204 à 0,254 s à K = 5) sont `catalogue_s + tower_s` (`cli/mhgp10_tower.cpp:106-107, 114-117`) : la préparation n'y figure pas. S'y ajoutent, dans `build_catalogue`, environ 8 ms hors de toute étape chronométrée (164,3 ms contre 156,6 ms d'étapes dans `cmd/012` ; 171,1 contre 162,4 dans `cmd/005`) : mise en place en série (`generator.cpp:641-651`) et libération des tableaux par fil au retour.
- **Conséquence pour la v11.** Environ 15 ms en série autour du calcul (préparation, mise en place, libération), soit 15 % d'un budget de 100 ms. Le contrat doit dire s'il part du fichier, des tableaux en mémoire ou du nuage préparé, et la préparation doit être parallèle (C15 montre qu'elle est juste).

### C11 — Mémoire : défauts de page à chaque passe, pics publiés gonflés (mineur)

- **Mesuré.** 201 159 défauts de page mineurs pour une construction (trame 02, K = 5, 4 fils, pic résident 400 844 Kio). Passes chaudes dans le même processus : + 120 855 puis + 61 094 défauts ; la mémoire n'est pas réutilisée d'une passe à l'autre. 42 `munmap` par construction, exécutés en série (`strace -c`).
- **G4.** Le même nombre de défauts (145 000) coûte 0,09 s de temps noyau à 1 fil et 0,31 s à 48 fils.
- **Pics publiés.** `cli/mhgp10_tower.cpp:88-110` garde le catalogue précédent vivant pendant la construction du suivant. Ici, le pic passe de 400 844 Kio (`--repeat=1`) à 710 964 puis 755 852 Kio (`--repeat=2`, `3`). Les pics des reçus de la sonde de la tour (769 172 Kio à K = 5 et 3 194 576 Kio à K = 10, à `--repeat=3`) surestiment donc une passe ; le catalogue seul à 48 fils pèse 424 676 Kio (`cmd/005`).
- **Conséquence pour la v11.** Des arènes de session réutilisées d'une trame à la suivante (un flux LiDAR rejoue la même forme de charge), et le pic d'une passe isolée dans les reçus.

### C12 — Statuts, raisons et codes de sortie : incohérences (mineur)

- **Raisons mortes (lu).** Jamais émises : `k_out_of_range`, `device_unavailable`, `arith_guard`, `window_empty`, `nested_parallelism`, `leaf_unsplittable` (`reasons.def:10, 16, 18, 19, 26, 27`). `reasons.def:2-3` renvoie à un dossier `api/` qui n'existe pas.
- **`Result` (lu).** `Result(Outcome)` avec la raison `none` est `ok()` avec une valeur par défaut (`status.hpp:72`), et sert de valeur d'attente (`cli/mhgp10_tower.cpp:84-85`) ; `value()` n'est pas gardé (`status.hpp:75-76`) ; `Outcome` n'est pas `[[nodiscard]]`.
- **Codes de sortie (lu).** L'en-tête de `cli/mhgp10_tower.cpp:9` dit « 0 conforme, 2 refus, 3 invariant violé ». Or un échec du catalogue rend 2 même pour `rank_order` ou `census_mismatch` (statut `invariant_violated`, `generator.cpp:836`) : `cli/mhgp10_tower.cpp:92-96`, `mhgp10_catalogue.cpp:62-66`, `mhgp10_cluster.cpp:125-129`. La course du pool du 29 septembre s'est d'ailleurs manifestée ainsi, comme un « refus ». R2 garde ce défaut (`print_refusal` rend toujours 2). Un échec de `prepare_cloud` rend 2 sans ligne de statut (`mhgp10_tower.cpp:72`, `mhgp10_cluster.cpp:109`).
- **Priorité non déterministe à l'intérieur d'un ordre (lu, non reproduit).** `status.hpp:58-63` promet « plus petit K, puis plus petite raison ». Elle n'est appliquée qu'entre ordres (`tower.cpp:1650-1655`). Dans un ordre, `tower.cpp:804-807` garde la première raison arrivée ; `generator.cpp:749-750` rend l'échec du premier ouvrier en défaut. Si un refus (`shell_quotient_budget`) et un invariant violé coexistent, le statut et le code dépendent de l'ordonnancement.

### C13 — Échafaudage déclaré, jamais employé (mineur)

- `types.hpp:21-25` : `SiteIdx`, `BallIdx`, `LevelRank` et `NodeIdx` n'ont **aucun** emploi hors de leur déclaration ; le moteur manipule des `u32` nus. Seul `PointId` sert (12 occurrences). L'argument de l'en-tête (`types.hpp:3-4` : un rang et un identifiant n'ont jamais le même type) n'est donc pas appliqué.
- `kMaxOrder` (`types.hpp:32`) est inutilisé ; le catalogue accepte K jusqu'à 12 (`generator.cpp:635`). `make_id` et `Buffer::allocate_zero` n'ont aucun emploi ; `Csr<T>` un seul (`cloud.hpp:22`) ; `MHGP10_CHECK` un seul fichier.
- `Pool::nested_calls()` n'est lu que par la porte unitaire, alors que `pool.hpp:5` dit que l'imbrication est « signalée ».
- `MHGP10_POISON` : aucune porte ne construit avec cette option. Je l'ai exercée : dump identique sous ASan + UBSan + tampons empoisonnés sur le quart de trame et sur la trame 01 entière (35 551 points), 0 rapport.
- `Buffer<T>` n'a ni `static_assert` sur `alignof(T)` (`buffer.hpp:70`), ni accès borné en build de test.

### C14 — Options flottantes : garde trop étroite à HEAD, machinerie trop lourde dans R2 (mineur)

- **Exécuté.** `CMakeLists.txt:20-22` ne refuse que la chaîne `fast-math` dans `CMAKE_CXX_FLAGS`. Passent : `-Ofast`, `CMAKE_CXX_FLAGS_RELEASE=-O3 -ffast-math`, `-funsafe-math-optimizations -ffinite-math-only`. `-march=native` passé par `CMAKE_CXX_FLAGS` active la contraction FMA (`-ffp-contract=off` n'est posé qu'avec `MHGP10_MARCH`, `:23-26`). Aucune garde dans les sources.
- **Mesuré.** Sous `-Ofast` comme sous `-march=native`, `mhgp10_unit` passe et le dump de la tour (quart de trame, K = 5) est identique au build de base : les marges absorbent ces écarts, sur cette entrée.
- **R2.** 486 lignes de `cmake/fp_flags.cmake`, une sonde, une garde par unité, une campagne de 122 mutants (délai 3 600 s) : c'est une liste noire d'options, par nature incomplète (R2 déclare lui-même quatre canaux hors contrat).
- **Conséquence pour la v11.** La doctrine du § 4 (usages justes sous tout mode d'arrondi, avec ou sans contraction) est la bonne réponse ; ne pas porter `fp_flags.cmake`.

### C15 — Nuage : juste (info, acquis)

- **Exécuté** (`morton.cpp`). `morton3` contre un entrelacement bit à bit indépendant : 2 000 000 de contrôles, 0 désaccord. `prepare_cloud` sur la trame 02 augmentée de 2 292 doublons et de `PointId` arbitraires : mêmes sites, poids et listes d'identifiants après permutation de l'entrée, clés strictement croissantes, CSR bien formé. Refus conformes (22 bits, coordonnée hors domaine).
- **Lu.** `cloud.cpp:30` contient une branche morte (`bits == 32`). Trois `std::vector` transitoires hors budget peuvent lever `std::bad_alloc`.

### C16 — Multiplicités : portées par le nuage, refusées par la tour (mineur)

- **Lu.** Le nuage regroupe les doublons en sites pondérés (`cloud.cpp:57-70`) et `SiteTree::kth_distance` compte les poids (`site_tree.cpp:229-268`). La tour refuse tout poids différent de 1 (`tower.cpp:1157-1158`, `multiplicity_unsupported`). `kth_distance` et `within` ne servent qu'à un témoin de test (`tests/head/mreach.cpp`).
- **Mesuré.** Les trois trames du contrat n'ont aucun doublon à 1 mm (39 885, 35 551 et 45 845 sites pour autant de points). Une trame qui en aurait un serait refusée.

### C17 — Primitives u32 : isolées et lentes (info)

- **Lu.** `grid32_primitives.hpp` n'est inclus que par sa propre porte. **Mesuré** : `morton96` (boucle de 32 tours sur 96 bits) coûte environ 20 fois `morton3` (214 contre 10 ns par clé, rapport seul significatif). C'est une référence, pas une primitive de production.

### C18 — Ordre des niveaux : un bon schéma d'emploi du flottant (info, acquis)

- **Lu.** `generator.cpp:773-836` trie par (approximation du niveau, support), répare en exact les bandes d'approximations voisines (seuil relatif $2^{-40}$ ; erreur relative de l'approximation annoncée sous $2^{-50}$, que je borne à $6 \cdot 2^{-53}$ et mesure à $2{,}84 \cdot 2^{-53}$ au plus sur 200 000 tirages), puis **vérifie en exact chaque paire de voisins** et refuse (`rank_order`) si l'ordre est faux. Le flottant ne décide rien que l'exact ne recontrôle. Coût sur G4 : 5,5 ms de comparaisons exactes et 4,6 ms de bandes à 48 fils.
- **Lu.** La représentation publiée d'un niveau (numérateur, dénominateur non réduits) dépend de la présentation qui a émis la boule ; `emitted_level` (`generator.cpp:187-221`) reproduit l'ordre d'énumération d'une version antérieure pour la fixer. C'est une complication héritée.

### C19 — Constructions, assainisseurs, avertissements (info)

- Quatre constructions sans avertissement (GCC Release, ASan + UBSan + POISON, TSan ; Clang 18). Portes unitaires au code 0 dans chacune.
- Jeu étendu de GCC sur les fondations : un seul avertissement (`wide.hpp:17`, conversion de signe sur la taille du tableau). `-Weverything` de Clang : neuf conversions de signe dans `sort.hpp` (lignes 27, 42, 58, 60), trois `switch` sans `default` (`status.hpp:20, 31, 42`), un destructeur de fin de programme (`site_tree.cpp:111`, tampon par fil). Assainisseur d'entiers de Clang : seuls les rebouclages voulus (`wide.hpp:27, 80`, `cloud.cpp:13`).
- `cmake/run_expect.cmake` (code exact, signal refusé, ligne attendue dans la même exécution) est juste à la lecture. `CMakeLists.txt` n'enregistre que des attentes à 0 : aucun refus (code 2 ou 3) n'est joué par une porte C++ directe. Une seule bibliothèque `mhgp10_core` regroupe toutes les couches (`CMakeLists.txt:38-47`) : aucune dépendance n'est imposée par la construction.

## 5. Ce qui est solide et mérite un port explicite en v11

Verdict par composant (« HEAD = R2 » : fichier identique dans les deux arbres).

| Composant | Verdict | Source à épingler | Ce qui fonde le verdict |
| --- | --- | --- | --- |
| `arith/wide.hpp` | **port tel quel**, plus `[[nodiscard]]` et aucune conversion qui tronque | HEAD = R2 | juge décimal de la porte unitaire ; mutants M7 et M8 tués ; relecture ligne à ligne |
| formules de `arith/geometry.{hpp,cpp}` | **port des formules, types à refaire** (`Int<bits>`, formes de centre typées) | HEAD = R2 | 18 016 configurations extrêmes contre un juge indépendant, 0 désaccord, 0 UBSan (C05) |
| budget de bits | **à graver** | tables de C05 | recoupé par l'expérience aux six largeurs |
| `core/status.hpp`, `reasons.def` | **port après élagage** | R2 (raisons ajoutées en fin de table) | modèle sain : statut fixé par la raison (X-macro), `Outcome`, `Result`, priorité `precedes` |
| `sched/pool.{hpp,cpp}` | **port de la version R2** pour la sémantique | R2 | descripteur par appel capturé sous verrou, réclamation saturante, capture et relance de la première exception, `make_pool` ; rejoué (C02, C03) ; mes tests d'exception et du tri sous TSan : 0 rapport |
| `cloud/cloud.{hpp,cpp}` | **port de la sémantique**, tri à refaire | HEAD = R2 | ordre total (Morton, `PointId`), sites pondérés, indice de site invariant par permutation et par renumérotation (C15) |
| sémantique des sorties de `SiteTree` | **à porter** ; filtre et construction à récrire | R2 comme témoin différentiel | sorties définies par les seules coordonnées (ex aequo par indice), indépendantes de la forme de l'arbre ; exact sur trames entières dans son domaine (C04) |
| requêtes entières `kth_distance`, `within` | **port** si les multiplicités sont gardées | HEAD = R2 | 3 000 et 600 contrôles sur trame, mutant M12 tué |
| `cmake/run_expect.cmake`, `gates.cmake` | **port tel quel** | HEAD = R2 | code exact, signal refusé, ligne attendue dans la même exécution |
| schéma « clé approchée, bandes réparées, contrôle exact de chaque voisin » | **à porter comme patron** | `generator.cpp:773-836` | le flottant n'y décide rien que l'exact ne recontrôle (C18) |
| `tower/rank_search.hpp` et sa porte | **port** | HEAD = R2 | 36 047 contrôles, tailles jusqu'à $2^{32} - 1$, propre sous UBSan |
| `grid32::squared_distance` | **port** si un profil large est ouvert | HEAD | porte dédiée, 212 684 contrôles |
| options `SANITIZE`, `TSAN`, `POISON`, `-Werror` GCC et Clang | **port**, avec une porte qui les exerce sur une trame | HEAD | quatre constructions propres ; dumps identiques sous assainisseurs |

Sont aussi à porter, comme **actifs de test** (écrits pour cet audit, dans le dossier de preuves) : le juge indépendant des prédicats (`harness.cpp`, `oracle.py`), la fixture du centre lointain (`far_center.cpp`), le juge d'échantillon sur trame (`frame_judge.cpp`), le lanceur de mutants sur copie (`run_mutants.py`), le test du tri (`psort.cpp`).

## 6. Ce qu'il ne faut pas refaire

1. **Annoncer un plafond mémoire sans le raccorder** : un budget par défaut illimité et global, 0,26 % du pic effectivement compté, et un commentaire qui dit « honnête par construction » (C01).
2. **Laisser lever dans une tâche parallèle** sans politique : des `std::vector` qui croissent dans les corps, aucun `try`, un processus tué par signal au lieu d'un refus (C02).
3. **Un filtre flottant à marge absolue fixe**, valable dans un domaine que l'interface ne vérifie pas (0,02, u18, centre dans la boîte), puis **un contrat sur les options de compilation** pour le protéger (486 lignes de CMake dans R2) (C04, C14).
4. **Des prédicats dont la validité dépend d'une convention d'appel** : forme du centre non typée, retours de débordement ignorables, conversions qui tronquent en silence, domaine accepté par le nuage (21 bits) plus large que celui de l'arithmétique (C06).
5. **Juger un prédicat par lui-même** (`side_key` contre `side_key`), n'exercer l'arithmétique qu'à petite magnitude, confondre le code du plancher et celui du désaccord massif (C07).
6. **Endormir tous les ouvriers entre deux régions** quand une construction en enchaîne une cinquantaine (51 comptées à 4 fils), garder en série la liaison entre les régions, et dimensionner le découpage sur le nombre de fils (C08).
7. **Trier deux fois** chaque élément et allouer un tableau de travail à chaque tri (C09).
8. **Sortir la préparation de la somme publiée** quand, avec la mise en place en série, elle pèse 15 % de la cible (C10).
9. **Publier un pic mémoire mesuré sur plusieurs passes** où deux générations de résultats coexistent (C11).
10. **Répartir le code de sortie entre les sondes** : le même statut rend 2 ici et 3 là (C12).
11. **Déclarer sans employer** : identifiants forts, raisons, constantes, compteurs, option d'empoisonnement sans porte (C13).
12. **Mettre l'entrée-sortie des sondes dans `core`** (R2 : `cli_output.hpp`, 448 lignes, sauvegardes par liens physiques) : la transaction « tout ou rien » se tient avec un fichier temporaire et un renommage.
13. **Une représentation de niveau qui dépend de l'ordre d'énumération** d'une version passée (C18).
14. **Une seule bibliothèque pour toutes les couches** : la construction n'impose alors aucune dépendance (C19).

## 7. Questions ouvertes

1. **Périmètre du contrat de 100 ms.** Part-il du fichier, des tableaux en mémoire ou du nuage préparé ? La préparation (6,5 à 8,3 ms sur G4) est publiée à part (`prepare_s`) et n'entre pas dans les sommes « catalogue + tour » ; la mise en place et la libération en série du catalogue (environ 8 ms) sont comptées dans `catalogue_s` mais dans aucune étape.
2. **Plafond mémoire.** Est-ce un contrat de la v11 (valeur, refus déterministe et indépendant du nombre de fils) ou seulement une mesure publiée ?
3. **Multiplicités.** Dédoublonner à l'entrée en publiant la correspondance, ou écrire la sémantique pondérée ? Aujourd'hui un doublon à 1 mm fait refuser la trame (C16).
4. **Conformité « octet pour octet » et niveaux.** Si la v11 doit égaler le binaire figé de la v10 sur les couples (numérateur, dénominateur) non réduits, elle doit porter la règle héritée d'`emitted_level`. Comparer les niveaux par leur valeur (produits croisés) ou les calculer depuis le support canonique supprimerait cette dette.
5. **Doctrine F3 et élagage spatial.** F3 interdit la soustraction entre approximations ; or élaguer une boîte autour d'un centre rationnel demande la différence entre une borne entière exacte et une coordonnée approchée. Faut-il une règle supplémentaire (borne inférieure certifiée, erreur absolue explicite, gonflée pour tout mode d'arrondi), ou un élagage en entiers exacts, dont le coût reste à mesurer ?
6. **Coût réel d'un réveil à 48 fils.** Non mesuré ici. Un micro-banc sur G4 (région vide, puis P tranches courtes, par nombre de fils) trancherait entre attente active bornée et sommeil.
7. **Fils et cœurs.** La VM a 24 cœurs à deux fils ; le CPU utilisateur passe de 3,95 à 6,04 s entre 1 et 48 fils. 24 fils épinglés font-ils mieux que 48 ? Le reçu de session 4 donne 0,259 s à 24 fils contre 0,171 s à 48 pour le catalogue, sans épinglage.
8. **Profils B = 21 et B = 24.** Les tables de C05 donnent les conteneurs ; reste le coût (`side_key` de forme q3 passe à 192 bits dès B = 21) et le filtre qui l'évite.
9. **Convention de l'ordre de Morton.** Elle n'est gravée par aucune porte (M14 survit partout) ; si l'indice de site est publié, il faut une fixture. Piste non contrôlée ici : le rapport L06 relève que l'entrée `cover` dépend du rang de Morton aux ex æquo.
10. **Format d'entrée.** Un fichier `.u32le` ne porte ni pas, ni origine, ni nombre de bits ; le domaine est fixé par la sonde.
11. **Portes de R2.** Je n'ai rejoué que `mhgp10_unit` de R2 et mes propres démonstrations ; les 82 portes annoncées par `PROVENANCE.md` restent à contrôler par qui portera.

## 8. Recommandations pour la v11

Par module de `morsehgp3D_v11/docs/ARCHITECTURE.md` § 2, dans l'ordre où je les traiterais.

**R1 — `num` : graver le budget, typer les formes, juger par un tiers.**
- Reprendre les tables de C05 comme budget normatif ; le calcul `constexpr` par expression donnera les bornes mécaniques (seconde table), les lemmes L2 et L3 n'étant utiles que pour gagner un mot à B = 20 ou B = 24.
- Trois types de centre (ou un paramètre de forme) : un prédicat ne doit pas pouvoir recevoir une forme hors de son budget.
- `[[nodiscard]]` sur toute opération qui peut refuser ; aucune conversion étroite implicite ; un produit borné explicite pour le comparateur de niveaux (à B = 32, 465 bits dans 8 mots).
- Porte `unit` : juge indépendant en fractions exactes aux extrêmes de chaque profil B compilé, sous UBSan, plus des mutants de code des prédicats. Le harnais de cet audit en est une base (12 s à 18 bits).

**R2 — `index` : plus de marge fixe, plus de précondition cachée.**
- Décision sur un site : clé de puissance exacte, précédée d'un filtre à borne **calculée** (somme des valeurs absolues des termes, facteur prouvé), sur le modèle d'`orient_center_filtered` (`tower.cpp:443-461`), dont l'analyse d'erreur est juste à la lecture. Un tel filtre vaut pour tout centre et tout profil B.
- Élagage des boîtes : borne inférieure certifiée de la clé sur la boîte, ou entiers exacts ; trancher la question 5 avant d'écrire.
- Centre sur un site : chemin entier exact, sans flottant (distances carrées inférieures à $2^{53}$ jusqu'à B = 24).
- Jamais de repli en O(n) par requête : le repli exact est par site.
- Construction : les sites sont déjà en ordre de Morton ; un arbre implicite sur cet ordre se construit en parallèle, sans `nth_element`, sans table de permutation ni copie des coordonnées en double. À comparer au k-d actuel (6,7 à 8,0 ms en série ici).
- Fabrique rendant `Result`, tampons comptés, tampons de requête par ouvrier détenus par la `Session` (aucun `thread_local`).

**R3 — `sched` : porter la sémantique de R2, mesurer avant de choisir la mécanique.**
- Porter du `Pool` de R2 : réclamation saturante, capture d'exception, refus à la création. Rendre la réduction des fautes déterministe (plus petit ordinal de tranche, puis `precedes`).
- Mesurer sur G4 le coût d'une région vide et la latence de prise en charge par nombre de fils (question 6), puis choisir entre sommeil et attente active bornée ; publier dans chaque reçu le nombre de régions, le taux d'emploi des fils (CPU sur mur × fils) et les commutations volontaires.
- Réduire le nombre de régions et paralléliser la liaison : la frontière en largeur, synchrone par niveau et dimensionnée par le nombre de fils, est le poste qui se dégrade (C08).
- `parallel_for` générique sur l'appelable ; partition statique par ordinal pour les boucles régulières, distribution dynamique réservée aux tâches irrégulières triées par charge.
- Tri : une seule passe de tri par élément, tampon de travail du budget, porte dédiée (C09).

**R4 — `core` : un budget réel ou pas de budget.**
- `MemoryBudget` détenu par la `Session`, sans instance par défaut ; admission d'un étage par formule avant allocation ; statut identique à 1 et à 48 fils sous budget serré (porte).
- Porte de véracité : part comptée du pic résident au-dessus d'un plancher, sur une trame.
- Arènes réutilisées d'une construction à la suivante ; plus de croissance de vecteur dans les corps parallèles.
- `Result` : pas de construction réussie à partir d'un `Outcome` ; accès à la valeur gardé ; `Outcome` `[[nodiscard]]`.
- Raisons : supprimer celles qui ne sont pas émises ; une raison n'entre dans la table qu'avec la porte qui la provoque.
- Identifiants forts : les employer dans les signatures (site, boule, rang, nœud) ou ne pas les déclarer.

**R5 — `cloud` et `io` : préparation dans le contrat de temps.**
- Tri par base des couples (clé de Morton, identifiant), contrôle des identifiants dans la même passe ; préparation chronométrée et publiée dans la somme.
- Une fonction unique statut → code de sortie pour toutes les sous-commandes ; une ligne de statut pour tout refus.
- Un format d'entrée auto-descriptif (pas, origine, bits, nombre de points).
- Une fixture qui grave la convention de l'ordre de Morton.

**R6 — portes des fondations.**
- Une porte par module avec juge indépendant, planchers, mutants sur copie (`tests/mutants/`), code 3 réservé aux planchers.
- Une porte « trame entière sous ASan + UBSan + tampons empoisonnés » et une porte « sortie identique à 1, 2, 4 et 8 fils », toutes deux par empreinte.
- Des refus joués par les portes C++ elles-mêmes (codes 2 et 3 attendus), pas seulement par les scripts.

## 9. Limites de cet audit

- **GCP non utilisé.** Tous les temps absolus viennent des reçus G4 existants, relus dans leurs fichiers bruts ; aucun n'a été rejoué.
- **Machine locale chargée** (10 à 18 sur 8 cœurs) : mes temps locaux ne sont donnés que comme rapports ou minima, et je ne conclus rien sur le coût mural des réveils.
- **Hors périmètre** : `generator.cpp` et `tower.cpp` n'ont été lus qu'aux sites d'appel des fondations ; les filtres flottants propres à la tour ont été relus, pas éprouvés.
- **K = 10** n'a pas été exécuté ici ; les mesures locales sont à K = 5.
- **R2** : seuls sa porte unitaire et mes démonstrations ont été rejoués.
- **Mutants contre les oracles** : oracles réduits (10 et 8 nuages au lieu de 40 et 24), suffisants pour tuer, pas pour qualifier.
- **Non reproduit** : la dépendance du statut à l'ordonnancement (C12) et l'échec de création de fil (C02) sont établis à la lecture seulement.

## 10. Preuves

Dossier `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l04_code_fondations/` :

| Fichier | Rôle |
| --- | --- |
| `JOURNAL_L04.txt` | sorties brutes, sections 1 à 41 |
| `harness.cpp`, `oracle.py` | juge indépendant des prédicats (C05, C06) |
| `budget_table.py`, `budget_mecanique.py`, `approx_error.py` | tables de C05, erreur de l'approximation des niveaux (C18) |
| `far_center.cpp`, `float_error_model.py`, `frame_judge.cpp`, `site_tree_stats.cpp` | `SiteTree` (C04) |
| `exc.cpp`, `grain.cpp`, `regions.cpp`, `wake.cpp` | `Pool` (C02, C03, C08 ; `wake.cpp` : mesure locale non concluante) |
| `psort.cpp` | tri parallèle (C09) |
| `counted.cpp` | octets comptés contre mémoire résidente (C01) |
| `prep_bench.cpp`, `radix_bench.cpp` | décomposition de la préparation, tri par base (C10) |
| `morton.cpp` | nuage (C15) |
| `run_mutants.py`, `mutants_unit.out` | mutants contre la porte unitaire (C07) |
| `run_mutants_oracle.sh`, `mutants_oracle.out`, `run_m18_oracle.sh`, `m18_oracle.out` | mutants contre les oracles (C07) |

Reçus G4 cités : `morsehgp3D_v10/receipts/g4_session4_j2c_20260929/results/cmd/` (`001`, `003`, `004`, `005`, `012`, `013`, `020` : `stdout` et `time.txt`), et `g4_session5_scale_20260929/vm_facts.txt` (AMD EPYC 9B45, 24 cœurs, 2 fils par cœur).
