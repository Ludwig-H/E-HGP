# Tranche S3 — arbre d'ordre K seul et rattachement des boules de W_K : compte rendu d'implémentation

5 octobre 2026, de 05 h 49 à 07 h 25 UTC (heures lues par `date -u`), reprise du travail interrompu le 4 octobre au
soir (conteneur redémarré à 05 h 36 UTC, `/tmp` vidé, tout reconstruit). Worktree `build/v11-impl-s3`, détaché à
`f98aeed67`, non rebasé (checkout partiel, `receipts/` absent). Rien n'est indexé, commité ni poussé. Constructions et
essais sous `/tmp/v11-s3/`, au plus deux cœurs par construction. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only (défaut de compilation ; u24 joué localement, u18 laissé à G4)
public_status=not_claimed
```

Autorités suivies, dans l'ordre : `DECISIONS_UTILISATEUR.md`, `CRITIQUE_ET_PLAN_REVISE.md`,
`SPECIFICATION_FINALE.md` (§§ 2.1–2.4, 3.2, 4, 7.1–7.2, 8.3–8.5, 9.1), puis le contrat L0 commité sur `origin/main`
(`5adf6a59f` : `MATHEMATIQUES.md` § 10, `docs/SORTIES.md`, oracle `reference/hgp11_ref/supports.py`) et les gardes de
l'auditeur (`de4ab58a8`, `aef7182b3`), lus par `git show` sans toucher au worktree. S3 ne publie ni population ni
compte stocké (décisions 4 et 5, qui relèvent de S6 et S7) : elle fournit l'arbre, le rattachement, les rôles, les
traces strictes et les branches.

## 0. En bref

- `build_order(FullDomain&&, Order k, MemoryBudget&, FullParams, Pool*, OrderTimings*, u64* attach_ns)` construit la
  seule forêt d'ordre K par la mise en place **non concurrente** de `build_full`, sans verticales ; `build_full`
  n'est pas modifié. Identité I10 avec `build_full(...).order(K)` : conforme sur 796 comparaisons bornées, aux trois
  tailles d'intérêt (8 000, 16 000, 32 000 points) et sur les trois trames à K5, plus ng00 à K10.
- Journal des graines : un champ `SeedLog* seed_log = nullptr` et quatre lignes gardées dans `cell` et
  `regular_cell`. **Aucun octet de sortie ne change** sans journal : 58 cas avant/après identiques (dumps
  `MHGP11FUL1` et `MHGP11PH` par sha256, sorties JSON hors durées), trames comprises. Temps de
  `mhgp11_full_bench` (ng00, K5, meilleur de 12 passes, deux séries) : écarts de −2,1 % à +1,7 % sur l'étage forêt,
  de signe variable, dans le bruit d'une machine partagée : aucun ralentissement visible (§ 6).
- Balayage du lemme D après `finish()` : `WindowAttachment` sur toute la fenêtre faible, contrôles I1 à I4 et
  « naissance forte » dans le produit, garde `UINT32_MAX` des traces publiées (refus `tower_capacity`).
- **Différentiel local contre l'oracle L0** (hors CTest, prototype de `mhgp11_tower_attach_fraction`) : la sortie
  JSON de la sonde, alignée sur `Supports.canonical`, est **identique** sur les 210 nuages de l'oracle (951 ordres,
  15 062 boules, 12 441 nœuds), en u21 voie sérielle et lots W4, en u24, et sous ASan + UBSan.
- 35 portes S3 enregistrées, toutes conformes localement (u21) ; 14 portes rapides conformes en u24 ; portes S3
  conformes sous ASan + UBSan Debug. Neuf mutants `tower` (les six du § 8.5 et trois nouveaux) tués localement.

## 1. Reprise

Le prédécesseur avait écrit `order_tree.hpp/.cpp`, `attachment.cpp`, `seed_log.hpp`, le journal dans
`forest_plateau.cpp`, les tests (`order_tree_*`, `attach_*`), six mutants et les sections de `FULL_FORESTS.md` et
`PROVENANCE.md`, sans rapport. Je l'ai relu en entier, reconstruit et rejoué : tout compilait sous `-Werror` et les
portes passaient. Rien n'a été jeté. J'ai ensuite :
1. intégré les apports de L0 et de l'auditeur (a) à (e) (§ 4) ;
2. réécrit la sortie JSON de `attach_probe` au format du vidage canonique de l'oracle et joué le différentiel local ;
3. ajouté les portes `mhgp11_tower_attach_capacity` et `mhgp11_tower_attach_scale{8000,16000,32000}`, l'empreinte
   `attache=` dans la ligne du juge, la voie sérielle `--serial`, et D2/E5 dans la porte d'export ;
4. ajouté trois mutants et vérifié les neuf mutants S3 ;
5. refait toutes les mesures perdues avec `/tmp` : avant/après à l'octet, temps, échelle, trames, K10, ASan, u24 ;
6. corrigé une erreur de compilation que seul le profil u24 révélait (largeurs différentes du numérateur et du
   dénominateur de niveau dans la sonde) ;
7. mis à jour `FULL_FORESTS.md` et `PROVENANCE.md` (numéros de ligne des sources vérifiés sur `f98aeed67`).

## 2. Fichiers (worktree `build/v11-impl-s3/morsehgp3D_v11/`)

| Fichier | État | Contenu |
| --- | --- | --- |
| `src/tower/order_tree.hpp` | nouveau | `BallRole`, `WindowAttachment`, `OrderTree`, `build_order` (exposés par `tower.hpp`). |
| `src/tower/order_tree.cpp` | nouveau | `build_order`, `order_forest` (contextes de la boucle non concurrente de `build_full`), `SeedLog::make`. |
| `src/tower/seed_log.hpp` | nouveau | `in_window` (fenêtre faible), `published_traces` (garde `UINT32_MAX`), `SeedLog`, `attach_window`. |
| `src/tower/attachment.cpp` | nouveau | Balayage du lemme D, rôles, branches, contrôles I1 à I4 et « naissance forte ». |
| `src/tower/forest_internal.hpp` | modifié | Déclaration de `SeedLog`, champ `SeedLog* seed_log = nullptr` de `ForestBuilder`. |
| `src/tower/forest_plateau.cpp` | modifié | Inclusion de `seed_log.hpp` ; `open`/`add` gardés par `seed_log != nullptr` dans `cell` et `regular_cell`. |
| `src/tower/module.cmake`, `src/tower/tower.hpp` | modifiés | Deux sources ; quatre `using` (`OrderTree`, `WindowAttachment`, `BallRole`, `build_order`). |
| `tests/tower/order_tree_support.hpp` | nouveau | Nuages bornes (fixtures, témoins D2 et E5), identité des forêts, juge E2, branches recomptées. |
| `tests/tower/order_tree_test.cpp` | nouveau | Groupes `identity`, `same_params`, `refusals`. |
| `tests/tower/attach_test.cpp` | nouveau | Groupes `fixtures` (1, 4, 7, 8, D2, E5), `capacity`, `e1e2`. |
| `tests/tower/order_tree_fault.cpp` | nouveau | Panne de chaque allocation. |
| `tests/tower/attach_judge.cpp` | nouveau | Juge d'échelle : I10, I1 à I4, E2, voie sérielle, empreinte `attache=`. |
| `tests/tower/attach_probe.cpp` | nouveau | Sonde : JSON canonique (format de l'oracle) et bloc d'incidences fortes. |
| `tests/tower/attach_export_gate.py` | nouveau | Porte d'export (bibliothèque standard, sans `assert`, testée sous Python 3.10.21 `-S -B` et `-O`). |
| `tests/tower/tests.cmake`, `tests/tower/public_header_test.cpp` | modifiés | Portes S3 ; groupe `order_tree` du parapluie. |
| `tests/mutants/tower.json` | modifié | Neuf mutants ajoutés, plancher 123 → 132 (le seul changement du texte existant). |
| `docs/FULL_FORESTS.md`, `docs/PROVENANCE.md` | modifiés | Section `build_order` et rattachement ; section de provenance S3 en fin de fichier. |

Taille : 2 290 lignes nouvelles (dont 484 de produit) ; `git diff --stat` des fichiers suivis : 9 fichiers, 327
insertions, 7 suppressions. `docs/SORTIES.md`, `MATHEMATIQUES.md` et `README.md` ne sont pas touchés.

Scripts et journaux de la session (hors dépôt) : `build/v11-persist/sortie_supports/impl_s3/` (différentiel oracle,
avant/après, temps, mutants, portes d'échelle, LiDAR, export, sorties CTest). Ils n'écrivent aucune coordonnée LiDAR.

## 3. Choix et conception

**`build_order`.** Validation (`1 ≤ k ≤ min(kmax, n)`, `ForestParallel::validate`), puis la mise en place de la
boucle non concurrente de `build_full` (`forest_vertical.cpp:281-356`) : table de populations non liée, espaces census,
mémo, `ForestParallel`, et le seul constructeur d'ordre K, sans `RegularVerticalSeeds`. Les drapeaux
`concurrent_orders` (S11), `parallel_verticals` et `reuse_regular_verticals` sont **refusés**
(`parameter_out_of_range`), pas ignorés. Les contextes et le constructeur sont rendus avant le balayage, le journal
avant le transfert du domaine. Refus : `parameter_out_of_range`, `memory_budget`, `tower_capacity`, `tower_invariant`,
domaine intact, réservations rendues, diagnostics publiés au succès seulement.

**Journal.** `SeedLog` tient trois tampons admis avant allocation : boules des cellules, décalages, graines.
`cell` et `regular_cell` ouvrent la cellule (`BallIdx` strictement croissant, sinon `tower_invariant`) et consignent
chaque graine de naissance rendue (jamais une racine du DSU ni un `top`). Seul le fil qui applique les cellules
écrit, dans la voie sérielle comme dans la voie par lots (`ForestParallel::flush` applique les jonctions en ordre
après la résolution parallèle, les coquilles étendues entre deux vidanges). Les traces consignées sont strictes :
`cell` exige déjà un niveau initial `< λ_b`, `resolve_job` aussi.

**Balayage (lemme D, § 7.2 de la spécification).** W_K recalculée depuis le catalogue par la fenêtre faible
`p+q−1 ≤ K ≤ p+m` ; naissances (K ≥ 2) rattachées à leur nœud, de même rang, et fortes ; cellules du journal groupées
par rang, un seul `advance(r−1)` par plateau ; pour chaque graine, nœud u de la coupe ouverte, règle du parent, même
a(u) pour toutes les traces (T3) ; rôle par les rangs ; branches publiées pour le rôle fusion, compactées dans le
journal. Contrôles : I1 (chaque boule une fois, naissances = `births()` à K ≥ 2, aucune à K = 1, naissance forte),
I2 (rôles, vie à la coupe fermée), I3 (les branches distinctes des fusions couvrent les arêtes), I4 (six registres de
la forêt). Tout écart rend `tower_invariant` sans résultat.

**Juge E2.** Port de test de `ball_nodes` (`bench/points_export.cpp:178-214`) avec la fenêtre au lieu du prédicat
fort ; deux K-parties par boule (premières et dernières), niveau initial `≤ λ_b`, ancêtre **fermé** au rang de la
boule ; branches recomptées par descentes neuves des traces strictes de `build_cell`. À l'échelle, mémo et census
comme `ball_nodes`, tirage à graine fixe (`std::mt19937_64`).

**Sonde.** Mode requêtes : une ligne JSON par requête, clés triées, au format de `Supports.canonical(k, ids)` :
`format` (`hgp11_attach_probe`), `version`, `k`, `n`, `sites` (ordre lexicographique), `ids`, `nodes` (`level`,
`parent`, `children`, `kind`, `post`, `balls`, `birth_center`) et `balls` (`node`, `level`, `center`, `role` en
français, `p`, `m`, `qmin`, `components`, `prior`, `strict_traces`), plus `s_star` (S* en coordonnées). Les fractions
sont réduites et écrites comme `str(fractions.Fraction)` par une petite arithmétique d'entiers naturels propre à la
sonde (le numérateur de niveau a jusqu'à 8B+12 bits) ; les boules sont triées par (postordre du nœud, rang, centre
exact par `num::compare_centers`). Mode `--incidences` inchangé : bloc de forêt et bloc d'incidences fortes aux
conventions de `write_order`.

## 4. Apports de L0 et de l'auditeur intégrés

**(a) Lemme D sans `β(F) ≤ ℓ(r_b−1)`.** Le produit ne contient aucune garde sur cette inégalité : le balayage ne lit
que des nœuds et leurs ancêtres. Le témoin D2 est gravé dans `mhgp11_tower_attach_fixtures` (rattachements, rôles,
branches, traces, plus : boule de AB absente de `Cat_2`, MEB de AB au niveau 64, niveau 41 de rang `r_b−1` de la
boule ABC, enfants des fusions 5 et 6), dans les nuages bornés de `mhgp11_tower_attach_e1e2` et
`mhgp11_tower_order_identity`, et dans la porte d'export. Le mutant `garde_beta_coupe_ouverte` (l'inégalité devenue
garde du journal) n'est tué que par D2 parmi les fixtures.

**(b) Lemme W.2.** Les descentes et le journal lisent le vrai Γ_K ; rien n'est filtré à W_K. Le témoin E5 est gravé
comme D2 (fusion de niveau 83886/3563 à trois enfants 5, 6 et 8 ; boule de AC hors de `Cat_2`, MEB de AC au niveau
33/2 ; enfants des fusions 7, 8 et 9). Le code propre à S3 ne résout aucune trace : le filtre « graine résolue dans
le seul W_K » n'y est pas exprimable. Le mutant `graine_resolue_dans_w_k` est donc posé dans la descente partagée
(`src/tower/descent.cpp` : refus du pas intérieur par une boule hors de `Cat_K`, c'est-à-dire hors de W_K pour une
K-partie) ; il est tué par `mhgp11_tower_attach_fixtures`, au témoin D2 (premier refus), E5 le tuant aussi.

**(c) Garde de capacité.** `published_traces(end − begin)` refuse `tower_capacity` au-delà de `UINT32_MAX` avant
toute conversion ; `components` en est majoré. Aucun plafond de coquille n'est imposé au constructeur de FULL. Le
cas réel (coquille de 150 sites à K8, au moins C(69,8) = 8 361 453 672 traces) est hors de portée : la garde est
jouée directement sur valeurs synthétiques (`mhgp11_tower_attach_capacity` : `UINT32_MAX` admis, `2^32`, C(69,8) et
`2^64−1` refusés), et le mutant `traces_u32_sans_garde` est tué.

**(d) Juge E2.** Le juge garde `initial ≤ λ_b` et l'ancêtre fermé ; le journal garde `initial < λ_b`. « Une
naissance est toujours forte, une boule faible n'est jamais une naissance » est contrôlé dans le produit (I1), dans
le juge d'échelle et dans `mhgp11_tower_attach_e1e2` (26 276 boules faibles jugées, aucune naissance).

**(e) Sortie de la sonde.** Alignée sur le vidage canonique (§ 3). Le différentiel lui-même n'est pas câblé
(consigne) ; le script local `impl_s3/attach_vs_oracle.py` en est le prototype : il projette la sortie de l'oracle
sur les clés de la sonde (sans `s_star`) et compare les dictionnaires. Résultat :
`attach_vs_oracle conforme cas=951 boules=15062 noeuds=12441 roles={"fusion": 5768, "interne": 2736, "naissance": 6558}`,
en u21 (W1 et W4), en u24 (W1) et sous ASan + UBSan (W4).

## 5. Portes jouées (construction Release u21 `/tmp/v11-s3/b21`, `MHGP11_MODULES=tower`)

`ctest -R '^mhgp11_tower_(order|attach|public_header)'` avec `MHGP11_DATA_DIR` : **35/35 conformes** (283 s) ; les trois `mhgp11_tower_attach_scale*`, passées ensuite à `--serial --identity`, rejouées conformes (87 s).

| Porte | Ligne ou compte exact |
| --- | --- |
| `mhgp11_tower_public_header_order_tree` | 9 contrôles (carré à K2 par les noms publics) |
| `mhgp11_tower_order_identity` | `order_identity compared=796 narrowed=126 attachments=733 balls=22948` ; 6 045 contrôles (plancher 6 000) ; 10,9 s |
| `mhgp11_tower_order_same_params` | `order_same_params compared=467` ; 2 082 contrôles (plancher 2 000) |
| `mhgp11_tower_order_refusals` | `order_refusals pic_exact=538` ; 103 contrôles (cinq refus `memory_budget`, un succès au pic exact) |
| `mhgp11_tower_attach_fixtures` | 823 contrôles : 104 boules gravées (fixtures 1, 4, 7, 8, D2, E5, voies sérielle et lots) et 20 contrôles de témoins |
| `mhgp11_tower_attach_capacity` | 12 contrôles |
| `mhgp11_tower_attach_e1e2` | `attach_e1e2 boules=45896 naissances=17216 fusions=15886 internes=12794 passageres=3196 fusions_3plus=4588 etendues=5884 faibles=26276` ; 662 015 contrôles |
| `mhgp11_tower_order_fault` | 268 pannes injectées sur neuf configurations (22 à 35 allocations chacune), toutes refusées `memory_budget`, budget rendu, aucun diagnostic publié |
| `mhgp11_tower_attach_export` (et `_opt`) | `attach_export_verdict conforme cas=60 noeuds=270799 incidences=647622 octets=9687200` |
| `mhgp11_tower_order_identity_scale8000` | `attach_judge_verdict conforme k=5 n=8000 boules=395667 naissances=164842 fusions=108813 internes=122012 branches=273654 traces=737362 noeuds=273655 e2=0 identite=1 attache=845bdf1deb8c7bb3 entree=3be1324202d28360` |
| `mhgp11_tower_attach_scale8000` | même ligne, `identite=1` aussi : I10 par la voie sérielle sans Pool, même empreinte `attache=` |
| `mhgp11_tower_attach_e1e2_scale8000` | même ligne avec `e2=395667 identite=0` (toutes les boules) |
| `mhgp11_tower_order_identity_scale16000` | `... n=16000 boules=819004 naissances=340057 fusions=225041 internes=253906 branches=565097 traces=1530768 noeuds=565098 e2=0 identite=1 attache=efb95876c7aa639b entree=3469c29b4c34b7e3` |
| `mhgp11_tower_attach_scale16000` | même ligne (voie sérielle) |
| `mhgp11_tower_order_identity_scale32000` | `... n=32000 boules=1690045 naissances=700184 fusions=463577 internes=526284 branches=1163760 traces=3165977 noeuds=1163756 e2=0 identite=1 attache=9b07979d0029104e entree=aea3dec129cef911` |
| `mhgp11_tower_attach_scale32000` | même ligne (voie sérielle) |
| `mhgp11_tower_attach_e1e2_scale32000` | même ligne avec `e2=2000 identite=0` |
| `mhgp11_tower_order_identity_lidar_ng00_k5` | `... k=5 n=39885 boules=789886 naissances=341081 fusions=235401 internes=213404 branches=576483 traces=1350288 noeuds=576371 e2=0 identite=1 attache=8d4ad62e754ba32d entree=975c390e5912fabe` |
| `mhgp11_tower_order_identity_lidar_ng01_k5` | `... n=35551 boules=652958 naissances=283207 fusions=195173 internes=174578 branches=478385 traces=1102505 noeuds=478265 e2=0 identite=1 attache=72a8bf3900de9b61 entree=6b918ef47e9ae56e` |
| `mhgp11_tower_order_identity_lidar_ng02_k5` | `... n=45845 boules=832386 naissances=361326 fusions=248676 internes=222384 branches=610022 traces=1393952 noeuds=609376 e2=0 identite=1 attache=04381e7f08032a30 entree=6e11fa8bc5ee6432` |
| `mhgp11_tower_attach_e1e2_lidar_ng0{0,1,2}_k5` | mêmes lignes avec `e2=20000 identite=0` |
| `mhgp11_tower_attach_export_lidar_ng00_k5` (et `_opt`) | `attach_export_verdict conforme cas=1 noeuds=576371 incidences=1705735 octets=23186944` |
| `mhgp11_tower_attach_export_lidar_ng01_k5` (et `_opt`) | `... noeuds=478265 incidences=1416180 octets=19266136` |
| `mhgp11_tower_attach_export_lidar_ng02_k5` (et `_opt`) | `... noeuds=609376 incidences=1807322 octets=24575400` |
| `mhgp11_tower_order_identity_lidar_ng00_k10` (`long`) | `attach_judge_verdict conforme k=10 n=39885 boules=2117675 naissances=979350 fusions=659261 internes=479064 branches=1638612 traces=3831491 noeuds=1638573 e2=2000 identite=1 attache=719d801e659bebe9 entree=975c390e5912fabe` (52,6 s à W8 ; 65 s et 3,4 Go de RSS à W4) |

Les comptes de forêt et de traces aux trois tailles et sur les trames sont ceux que le prédécesseur avait gravés ;
l'empreinte `attache=` est nouvelle. Hors porte, E1 = E2 a aussi été joué une fois sur **toutes** les boules des trois
trames (789 886, 652 958 et 832 386, moins de 10 s chacune) : aucun écart. Les portes de trames gardent le tirage de
20 000 boules (juge d'échantillon, règle du dépôt).

Autres contrôles : `python3 -S -B tools/check_style.py --root morsehgp3D_v11` → `style_ok fichiers=433` ;
`run_mutants.py --check` → `manifeste_ok module=tower mutants=134 plancher=132` ; `check_docs.validate` sur les deux
documents modifiés : seuls restent les liens vers `receipts/`, absents du checkout partiel, sur des lignes
antérieures à la tranche. Suite `fast` complète du module : § 8.

## 6. Avant/après : octets et temps (binaires de la base `f98aeed67` contre le worktree)

**Octets** (`impl_s3/compare_bench.py`) : `mhgp11_full_bench` (modes 0, 2047 et 16379, W1 et W4) et
`mhgp11_points_export` (ordres 1..K) sur carré, D2, E5, tétraèdre, boîte de 600 points, 2 000 points uniformes
u18, 3 000 points en grappes, puis les trames ng00 à ng02 à K5 (16379 et 2047 à W4, export à W4) :
`compare_bench conforme cas=58 identiques=58`. Dumps identiques par sha256, mêmes codes, mêmes sorties JSON hors
clés de durée. Les exécutables eux-mêmes diffèrent forcément (nouveau champ, nouvelles lignes gardées).

**Temps** (`impl_s3/time_bench.py`, diagnostic local, pas un reçu) : ng00, K5, mode 16379, trois passes par
processus, quatre processus par binaire alternés, machine partagée (charge 4 à 10) :

| Fils | Binaire | forêt, meilleur | forêt, médiane | FULL, meilleur | FULL, médiane |
| --- | --- | ---: | ---: | ---: | ---: |
| W1 | base | 5 578,8 ms | 5 933,1 ms | 13 509,6 ms | 14 335,6 ms |
| W1 | courant | 5 625,9 ms | 6 009,2 ms | 13 521,8 ms | 14 417,8 ms |
| W4 | base | 1 442,1 ms | 1 517,7 ms | 3 456,6 ms | 3 645,5 ms |
| W4 | courant | 1 453,0 ms | 1 695,1 ms | 3 500,3 ms | 3 996,8 ms |

Seconde série, 25 minutes plus tard (même protocole, W4 puis W1, machine plus chargée) :

| Fils | Binaire | forêt, meilleur | forêt, médiane | FULL, meilleur | FULL, médiane |
| --- | --- | ---: | ---: | ---: | ---: |
| W4 | base | 2 141,1 ms | 2 266,9 ms | 5 435,8 ms | 5 626,1 ms |
| W4 | courant | 2 177,4 ms | 2 539,1 ms | 5 343,9 ms | 6 411,7 ms |
| W1 | base | 5 904,9 ms | 6 747,8 ms | 14 854,7 ms | 16 979,2 ms |
| W1 | courant | 5 779,5 ms | 6 089,1 ms | 14 015,8 ms | 14 732,1 ms |

Lecture : les écarts des meilleurs temps changent de signe d'une série à l'autre (forêt +0,8 % puis −2,1 % à W1,
+0,8 % puis +1,7 % à W4 ; FULL +0,1 % puis −5,6 % à W1, +1,3 % puis −1,7 % à W4) ; les médianes varient de 10 à
15 % avec la charge des autres agents. **Aucun ralentissement n'apparaît** à la résolution de cette machine, ce
qu'attend le changement lui-même : sans journal, un test `seed_log != nullptr` par cellule et un par graine,
toujours faux et donc bien prédits, et huit octets de plus dans `ForestBuilder` (objet de pile). Ce n'est pas un
reçu : la mesure à W48 se refait sur G4.

## 7. Mutants

`mhgp11_mutants_tower_manifest` rejoué **avant** (base `f98aeed67` : `manifeste_ok module=tower mutants=125
plancher=123`) et **après** (`manifeste_ok module=tower mutants=134 plancher=132`) : les motifs existants de
`forest_plateau.cpp` restent uniques.

Vérification locale (`impl_s3/mutate_local.py` : copie des sources sous `/tmp/v11-s3/mut/src`, construction
incrémentale des seules cibles de la porte, porte jouée par CTest et jugée comme `run_mutants.py`, restauration
exacte). Témoins sans mutation verts ; **9 tués sur 9**, tous par le verdict `code` :

| Mutant | Porte | Cause observée |
| --- | --- | --- |
| `rattachement_coupe_ouverte` | `mhgp11_tower_attach_fixtures` | refus `tower_invariant` dès `carre_k1` (T3 et vie de l'interne) |
| `branches_coupe_fermee` | `mhgp11_tower_attach_e1e2` | refus `tower_invariant` (I3 : enfants non couverts) |
| `journal_racine_dsu` | `mhgp11_tower_attach_fixtures` | branches fausses (`components` 1 au lieu de 2) |
| `fenetre_forte` | `mhgp11_tower_attach_fixtures` | refus `tower_invariant` (jonctions faibles hors fenêtre) |
| `journal_premiere_graine` | `mhgp11_tower_order_identity` | refus `tower_invariant` (I4 : somme des traces) |
| `role_rang_egal_interne` | `mhgp11_tower_attach_fixtures` | refus `tower_invariant` (lemme B) |
| `traces_u32_sans_garde` | `mhgp11_tower_attach_capacity` | `2^32` traces admises (`obtenu 0, attendu 23`) |
| `garde_beta_coupe_ouverte` | `mhgp11_tower_attach_fixtures` | refus `tower_invariant` au témoin D2 seulement |
| `graine_resolue_dans_w_k` | `mhgp11_tower_attach_fixtures` | refus `tower_invariant` au témoin D2 (premier touché) |

Ce n'est pas la campagne `run_mutants.py` (copie et construction complètes par mutant, 134 mutants) : elle reste
pour G4 (`mhgp11_mutants_tower`).

## 8. Sanitiseurs, u24 et suite rapide

**ASan + UBSan, Debug** (`/tmp/v11-s3/basan`, `MHGP11_SANITIZE=ON`, `MHGP11_MODULES=tower`, cibles limitées aux
tests S3, `forest`, `forest_parallel`, `pipeline`, à la sonde et à l'export) : toutes les portes S3 conformes
(`order_identity` 482 s, `same_params` 382 s, `attach_e1e2` 140 s, `attach_export` 181 s), différentiel oracle
conforme, aucune erreur de sanitiseur ; portes `forest`, `forest_parallel` et `pipeline` construites conformes, sauf
`mhgp11_tower_pipeline_equivalence`, arrêtée par son délai de 600 s (CTest à deux tâches, machine chargée) sans
erreur de sanitiseur. Rejouée seule hors CTest (`./mhgp11_tower_pipeline equivalence`), elle est conforme :
`pipeline_equivalence orders=960 pipelines=192`, 4 519 contrôles, aucun échec, en 1 506 s. Les autres échecs de
cette passe sont des portes dont l'exécutable n'était pas construit (cibles limitées), pas des verdicts.

**Release u24** (`/tmp/v11-s3/b24`, `MHGP11_COORD_BITS=24`) : 14/14 portes rapides S3 conformes (mêmes lignes
`cas=60` d'export) ; différentiel oracle conforme. La construction u24 a révélé une erreur de la sonde (déduction
`auto` sur deux largeurs), corrigée.

**Suite `fast` complète du module `tower`** (u21, `ctest -L fast -j 2`, 372 s) : **269 conformes sur 273**,
dont `mhgp11_style` et `mhgp11_mutants_tower_manifest` (et leurs jumelles). Les quatre échecs sont dus à
l'environnement, pas à la tranche :
- `mhgp11_tower_full_paired_protocol` et `_opt` lisent
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`, absent du checkout partiel
  (`FileNotFoundError`) ;
- `mhgp11_tower_full_campaign` et `_opt` (modèle Python pur, `native0`, aucun binaire de la tranche) dépassent leur
  délai de 120 s sur la machine chargée ; jouée seule, la porte rend sa ligne exacte
  `full_campaign_verdict conforme attempts483 schedules412 interrupted1 checks39532 native0` en 146 s, et 119 s sur
  les sources de la base.

## 9. Écarts à la spécification, et pourquoi

1. **Capacités du journal majorées avant le parcours** (spec : admises après la classification, `C` = nombre de
   `kinds = 2`). Le majorant compte toute boule de W_K hors naissance régulière, naissances étendues comprises ; il
   se calcule en une passe sur le catalogue, sans toucher au constructeur, et reste admis avant allocation.
2. **Paramètre de diagnostic `u64* attach_ns`** ajouté en fin de signature de `build_order` (étage `attach` de la
   ligne d'état de `docs/SORTIES.md` § 3), facultatif, publié au succès seulement.
3. **Drapeaux refusés plutôt qu'ignorés** : `parallel_verticals` et `reuse_regular_verticals` rendent
   `parameter_out_of_range` comme `concurrent_orders`. L'appelant (S7) doit les effacer du masque 16 379.
4. **`attach_scale.py` et `attach_oracle.py`** (liste de fichiers du § 9.1) ne sont pas écrits : le juge d'échelle
   est natif (`attach_judge.cpp`, recomptes I1 à I4, I10, E2) ; le différentiel contre l'oracle est la future porte
   `mhgp11_tower_attach_fraction`, à câbler avec L0, dont le prototype est hors dépôt.
5. **Noms et contenus des portes d'échelle** : `mhgp11_tower_order_identity_scale*` porte I10 et I1 à I4 sur la
   voie par lots W3 ; `mhgp11_tower_attach_scale*` porte les mêmes contrôles sur la voie sérielle sans Pool et exige
   la même empreinte `attache=`, ce qui grave l'indépendance du rattachement à la voie et au nombre de fils (I8
   restreint à S3).
6. **Identité des boules dans la sonde** : centre exact et niveau, comme le vidage de l'oracle (contrat L0), au lieu
   de « S* en indices d'entrée » (spec § 8.3) ; S* reste publié en coordonnées (`s_star`).
7. **Mutant « graine résolue dans le seul W_K »** posé dans `descent.cpp`, code partagé avec FULL (voir § 4 b).
8. **Profils** : u18 non joué localement (consigne) ; u24 joué.

## 10. Ce que le contrat L0 devra documenter (fichiers non touchés ici)

- `docs/SORTIES.md` :
  - `build_order` **refuse** (`parameter_out_of_range`) `concurrent_orders`, `parallel_verticals` et
    `reuse_regular_verticals` ; la façade les efface du masque 16 379 avant l'appel (§ 1, « Moteur ») ;
  - la tour peut refuser `tower_capacity` quand une cellule a plus de `UINT32_MAX` traces strictes (§ 6 et § 8,
    ordre des refus), sans plafond de coquille imposé à FULL ;
  - l'étage `attach` de la ligne d'état est le diagnostic `attach_ns` de `build_order` (balayage et contrôles).
- `docs/MATHEMATIQUES.md` § 10.5, « Réalisation (E1) » : contrôles faits dans le produit (I1 à I4, naissance forte,
  même a(u) pour toutes les traces), majorant des capacités du journal, et identité avec `build_full(...).order(K)`
  gardée par I10 ; rappel que le journal ne consigne que des traces de niveau initial `< λ_b`.
- `README.md` (v11) et `docs/DEVELOPPEMENT.md` : état de S3 après sa qualification G4 (portes, mutants, lignes
  gravées) ; aucun temps revendiqué.
- `reference/README.md` : plan du différentiel `mhgp11_tower_attach_fraction` (projection de `Supports.canonical`
  sur les clés de la sonde, `s_star` ignoré, labels `oracle fast`, profils u18, u21, u24) ; le script
  `impl_s3/attach_vs_oracle.py` en donne la forme et le résultat local (951/951).
- `docs/ARCHITECTURE.md` : la ligne `tower` de L0 cite déjà `build_order` et `WindowAttachment` ; rien à ajouter.

## 11. Ce qui doit tourner sur G4

- Matrice : GCC Release u18, u21 et u24 (portes S3 `fast`, les trois `scale*`, les trames `lidar` et K10 `long`),
  ASan + UBSan sur `tower` (dont `mhgp11_tower_pipeline_equivalence`, arrêtée par délai ici), TSan ciblé sur
  `mhgp11_tower_order_identity`, `mhgp11_tower_attach_e1e2` et les portes d'échelle en voie par lots (le journal est
  écrit par le seul fil d'application ; TSan doit le confirmer).
- Campagne complète `mhgp11_mutants_tower` (134 mutants, plancher 132).
- Rejeu de l'avant/après à W48 (octets et temps de `mhgp11_full_bench`), puis la mesure appariée de L2 sous la règle
  écrite (`build_order` contre `full`, trois trames, W1 et W48).
- Après l'intégration de L0 : la porte `mhgp11_tower_attach_fraction` (u18, u21, u24).
