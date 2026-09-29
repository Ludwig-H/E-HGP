# Reçu — frontières d'entrée des CLI et du catalogue (clé `entrees_cli`)

29 septembre 2026. Base `0bce6cc00` (= origin/main), copie privée `src/` (dépôt local de la copie, sans rapport avec le
dépôt principal). `phase=exploration_v10_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only`,
`mode=correction_frontieres`, `public_status=not_claimed`. GCP non utilisé. Worktree du dépôt non modifié ; fichiers de
l'auditeur indépendant lus, non touchés. Aucune graine de banc (ni test, ni test_v10b) : fixtures gravées aux
coordonnées exactes, graine fixe `20260929` pour les deux nuages aléatoires de contrôle.

Patch : `entrees_cli.patch` (chemins `morsehgp3D_v10/...`, `git apply` à la racine du dépôt principal ; vérifié par
`git apply --check` sur une extraction vierge de `0bce6cc00`).

## 1. Constats reproduits sur le build non modifié

Build `build-avant/` (sources `base/` = `git archive 0bce6cc00`), binaires bit à bit identiques aux références
`build/v10-wt/` (sha256 `mhgp10_catalogue` a3bbad50…, `mhgp10_tower` a2077628…, `mhgp10_cluster` e00e2431…,
`mhgp10_mreach_cluster` 1dab9a4f…). Script `avant/reproduire_constats.py`, sorties complètes
`avant/constats_avant.json` (commande, code, stdout, stderr, durée, taille de sortie) et `avant/constats_avant.txt`.

| Constat | Commande (fixture) | Avant |
| --- | --- | --- |
| (a) fin u32le incomplète | 4 points `(0,0,0) (8,0,0) (0,8,0) (0,0,8)` + r = 1..11 octets (préfixe du point `262144 1 2`), quatre sondes, `--k=2` | **44/44 acceptés, code 0**, `status=ok`, `n=4`, 16 octets d'étiquettes ; à r = 4 le mot `262144` hors u18 disparaît ; témoin r = 12 : point complet lu, refusé `coordinate_out_of_domain` (18 bits), admis par mreach (21 bits) |
| (b) export sans attaches | `mhgp10_tower` 3 points `(0,0,0) (2,0,0) (5,0,0) --k=2 --no-points --dump=F` | **SIGSEGV (−11)** |
| (c) `--repeat=0` | `mhgp10_tower` 4 points `--k=2 --repeat=0` ; aussi `--repeat=-1` | **SIGSEGV (−11)** les deux |
| (d) feuille trop petite | `mhgp10_catalogue` huit coins de `[0,262143]^3 --k=5 --leaf=M` | M = 2, 4, 5, 7 : **délai de 10 s dépassé** (tué, récolté) ; défaut et M = 8 : 0,01 s |
| (e) conversions u32 du nombre de boules | lecture de `generator.cpp` de la base | l. 764–768 boucle `for (u32 r …)` et `Ref{…, r}` ; l. 803 `const u32 nb = static_cast<u32>(refs.size())` ; aucune garde `index_overflow_u32` (`avant/constat_e_lecture.txt`) ; non reproductible sans 2^32 boules |
| (f) même frontière, relevé en reproduisant | `--k=abc`, `--repeat=x`, `--leaf=abc` ; fichier absent ; dossier | **SIGABRT (−6)** (`std::invalid_argument` non rattrapée) ; absent : code 2 muet ; dossier : code 2 sous la raison fausse `empty_input` |

## 2. Correctif

- **Lecteur commun** `src/cloud/u32le_input.hpp` (en-tête seul, utilisé par les quatre sondes) : le fichier est lu en
  entier ou refusé avant tout calcul. Taille non multiple de 12 octets : `size_mismatch` ; au moins `kNone` points :
  `index_overflow_u32` (avant lecture si la taille est connue) ; ouverture, erreur ou lecture incomplète :
  `input_unreadable` ; mémoire : `memory_budget`. Le fichier régulier est jugé sur sa taille annoncée puis sur les
  octets effectivement lus ; un tube se lit en flux et se juge à la fin. Le domaine des coordonnées et le fichier vide
  restent jugés par `prepare_cloud`, désormais sur tous les mots du fichier. Hôte petit-boutiste exigé par
  `static_assert`. Fonction pure `check_u32le_size`, jugée sans fichier.
- **Nouvelle raison** `input_unreadable` (statut `invalid_input`), ajoutée en fin de `reasons.def` : aucune raison
  existante n'est renumérotée.
- **Sondes** `mhgp10_catalogue`, `mhgp10_tower`, `mhgp10_cluster` : refus en une ligne JSON `{"status","reason"}`, code 2
  (les refus de `prepare_cloud` de la tour et de `cluster` étaient muets) ; `mhgp10_mreach_cluster` garde sa convention
  `refus <raison>` sur stderr. Une valeur d'option illisible (`std::stoi`/`stoul`/`stoull`/`stod`) est refusée
  `parameter_out_of_range` au lieu d'un SIGABRT.
- **Tour** : `--repeat < 1` refusé `parameter_out_of_range` avant toute lecture ; l'export `--dump` ne parcourt les
  attaches que si elles existent (`--no-points` : nœuds, parents, niveaux et verticales seuls).
- **Catalogue** (`build_catalogue`, donc tout appelant et pas seulement la CLI) : `leaf_size` ∈ ]0, K + 3[ refusé
  `parameter_out_of_range` avant calcul (§ 3) ; garde `check_ball_count(first.back())` avant l'allocation de
  l'assemblage : total ≥ `kNone` refusé `index_overflow_u32`. Le total borne chaque compte par fil, donc la boucle
  locale `u32 r` et `nb` tiennent en u32 ; `Catalogue::balls()`, les rangs et la tour (`nballs`) aussi.

## 3. Justification de la borne de feuille : M ≥ K + 3 (et non M ≥ K)

La liste certifiée d'une boîte contient la boule K-NN fermée de chacun de ses points (théorème C). Quand la boîte se
réduit à un point c, elle tend vers la boule K-NN fermée de c : en position générale, K sites hors du diagramme de
Voronoï d'ordre K, K + 1 sur ses faces, K + 2 sur ses arêtes, K + 3 en ses sommets (au plus quatre sites
cosphériques, au plus K − 1 strictement intérieurs). Avec une feuille M < K + 3, aucune boîte ne s'arrête par la taille
près de toute une strate de ce diagramme (volume pour M < K, faces pour M = K, arêtes pour K + 1, sommets pour K + 2) :
la subdivision y descend jusqu'à la stagnation sous la grille, et le nombre de nœuds croît avec l'étendue du nuage,
en cube, en carré, linéairement, en logarithme. Avec M ≥ K + 3, toute boîte assez petite s'arrête en position
générale ; la table par défaut (12 / 16 / 24 / 28) garde cette marge et plus. La borne « M ≥ K » de l'énoncé est
nécessaire mais pas suffisante : M = K reste une explosion (mesure ci-dessous).

Mesure (compteurs déterministes de l'arbre, binaire de base, K = 5, un fil, délai 20 s ; homothétie entière exacte
du même nuage, donc même combinatoire ; `avant/leaf_scaling.py`, `avant/leaf_scaling.json`) :

| Nuage | M | ×1 (étendue 255) | ×4 | ×16 | ×64 | ×256 | ×1024 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 40 points aléatoires | 2, 4, 5 (M ≤ K) | délai | | | | | |
| 40 points aléatoires | 6 (K + 1) | 5 250 589 | délai | | | | |
| 40 points aléatoires | 7 (K + 2) | 125 955 | 165 647 | 206 741 | 248 053 | 289 873 | 331 725 |
| 40 points aléatoires | 8 (K + 3) | 12 627 | 12 715 | 12 801 | 12 839 | 12 835 | 12 833 |
| 40 points aléatoires | défaut 16 | 173 | 173 | 173 | 173 | 173 | 173 |
| 8 coins du cube | 2, 4, 5 | délai | | | | | |
| 8 coins du cube | 6 | 419 423 | 1 674 831 | 6 699 583 | délai | | |
| 8 coins du cube | 7 | 166 267 | 667 627 | 2 673 499 | 10 697 419 | délai | |
| 8 coins du cube | 8, défaut | 1 | 1 | 1 | 1 | 1 | 1 |

K + 1 croît linéairement, K + 2 logarithmiquement (+ 41 000 nœuds par facteur 4) en position générale ; K + 3 est
constant à l'arrondi des boîtes près. Le cube (dégénéré : quatre sites cocycliques par face, huit cosphériques) croît
linéairement à K + 2 le long de ses axes, où la boule limite compte 8 = K + 3 sites. Aucune valeur de `--leaf` du dépôt
n'est touchée : la calibration J2c n'a mesuré que M ≥ K + 5 (`calibre_m.sh`).

## 4. Portes ajoutées

- `mhgp10_regression_cli_input_frontiers` (`gate;regression;fast`, Python nu, sans `assert`, code exact via
  `run_expect.cmake`), `tests/regression/test_cli_input_frontiers.py` : **80 contrôles, plancher exact** (code 3 si le
  nombre diffère). Restes 0 à 12 octets sur les quatre sondes, fichier absent et dossier (60) ; dump sans attaches
  égal au dump complet privé de ses lignes `point`, structure 2 ordres / 8 nœuds / 6 attaches (4) ; `--repeat` 0, −1,
  illisible, témoin 2 passes (4) ; `--leaf` 2, K − 1, K, K + 2, illisible refusés, témoins K + 3 et défaut à 27 boules,
  dumps identiques (8) ; `--k=abc` sur les quatre sondes (4). Chaque délai dépassé (60 s) est un échec, processus tué.
- `mhgp10_unit` (`gate fast`), `test_input_frontiers` : `check_ball_count` à des cardinaux artificiels (0, 1,
  `kNone − 1` admis ; `kNone`, 2^32, 2^40, 2^64 − 1 refusés `index_overflow_u32` / `resource_exhausted`, aucune boule
  allouée) ; `check_u32le_size` (restes 1..11, vide, `kNone − 1` et `kNone` points) ; `build_catalogue` refuse
  M ∈ {1, 2, K − 1, K, K + 2} avant calcul ; à M = K + 3 le catalogue d'un nuage aléatoire u18 de 40 points est
  identique, champ par champ, à celui de la feuille par défaut, alors que l'arbre diffère (14 017 contre 207 nœuds).

Échec sur l'ancien build, succès sur le nouveau :

| Porte | Build non modifié | Build corrigé |
| --- | --- | --- |
| `test_cli_input_frontiers.py` | **code 1, 68 échecs sur 80** (les 12 témoins passent : pas de vert par vacuité) ; `avant/porte_sur_build_avant.txt` | code 0, 80/80 en 1,5 s (`apres/ctest_gate.txt`, `apres/porte_python_O.txt` sous `python3 -O`) |
| `mhgp10_unit` | ne compile pas : `check_ball_count was not declared`, `input_unreadable is not a member of Reason` (`avant/unit_nouvelle_porte_sur_base.txt`) | `unit_ok` |

RESULTATS_A_COMPLETER
