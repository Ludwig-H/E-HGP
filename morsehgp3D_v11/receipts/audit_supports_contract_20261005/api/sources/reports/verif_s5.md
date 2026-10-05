# Contre-lecture de la tranche S5 « api, Session et `mhgp11 --sortie=full` »

4 octobre 2026, rédigé de 23 h 22 à HEURE_FIN UTC (heures lues par `date -u`). Contre-lecteur indépendant. Worktree lu :
`build/v11-impl-s5` (diff non commité sur `f98aeed67` : 5 fichiers suivis modifiés, 7 entrées nouvelles). **Rien n'a
été modifié dans ce worktree** : instantané complet pris à 22 h 27 (593 fichiers, empreintes dans
`/tmp/v11-s5-verif/snap_manifest.sha256`), identique au worktree à 22 h 35 et en fin de contre-lecture ;
`git status --short --ignored` inchangé (12 entrées). Constructions et essais dans `/tmp/v11-s5-verif/`, au plus deux
cœurs par construction, machine partagée (charge 4 à 17 pendant la contre-lecture). **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (u24 construit et joué aussi)
public_status=not_claimed
```

Autorité suivie : `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 3.5, 3.6, 4, 5, 6.2, 6.6, 6.7, 8.5, 8.6, 9.1-S5) ; compte rendu `impl_s5.md` lu en entier ; brouillon L0
(`build/v11-impl-l0`, `docs/SORTIES.md`, table des modules) lu pour l'alignement, sans en dépendre.

## Verdict

**Aucun constat bloquant.** Ce que l'implémenteur affirme se rejoue, et davantage :

- `full.mhgp11ful1` est, octet pour octet, le dump de la sonde : porte rejouée (503 tentatives, u21 et u24), plus
  40 tentatives indépendantes sur des nuages et des K hors de la porte (K = 3 à 12, W1 et W5, u21 et u24), la trame
  **ng01** à K5 (l'implémenteur avait joué ng00), et deux nuages contre une sonde construite depuis **origin/main**
  (`git archive f98aeed67`), pas depuis l'arbre S5.
- Aucun octet de sortie existant ne change : sur ces deux nuages, dumps `MHGP11FUL1`, fichiers `MHGP11PH` et lignes JSON
  (hors champs de temps) de `mhgp11_full_bench` et `mhgp11_points_export` sont identiques entre l'arbre S5 et
  origin/main.
- L'ordre des refus tient sur 14 cas indépendants, dont 9 à plusieurs fautes que la porte ne joue pas.
- Campagnes de mutants : `api` 9/9 tués ; `cli` RESULTAT_CLI.

Il reste des **trous de portes** : sur 7 mutants de mon cru, tous survivent ; 4 sont causaux (C1, C3), 1 est
équivalent, 1 ne change pas l'objet (C4), 1 suit un chemin que l'implémenteur annonce non couvert. Et un défaut de
robustesse : sous une limite de taille de fichier, l'exécutable meurt par `SIGXFSZ` et laisse un `D.pending` orphelin
(C2). Tout se corrige en quelques lignes.

## 1. Portes rejouées (résultats exacts, locaux)

| Contrôle | Résultat |
| --- | --- |
| Release u21, construction complète propre (`b21`, 812 portes enregistrées), `ctest -L fast -j 2` | **760 portes : 756 conformes, 4 échecs étrangers à S5** (détail ci-dessous) |
| Portes `mhgp11_api_*` et `mhgp11_cli_*` de label `fast`, u21 | **27/27** |
| `mhgp11_api_session` (six groupes, inventaire) | `controles=` 370, 12, 50, 217, 165, 46 ; `inventaire_ok tests=6` |
| `mhgp11_api_session_fault` | `session` 25 (plancher 19), `starvation` 167 (plancher 60), ligne `starvation allocations=80 refus=80` |
| `mhgp11_api_selftest` | `modes` 81, `judge` 66 ; sonde : `none`, `upward`, `ftz_daz` → code 0 ; `inexact`, `underflow`, `denormal` → `selftest_probe environment_selftest`, code 3 |
| `cli_contract.py` sous `python3 -S -B` | `cli_contract_verdict conforme refus54 temoins3`, `cli_contract_ok controles=270` |
| `cli_full_identity.py` sous `python3 -S -B` | `cli_full_identity_verdict conforme attempts503 refusals7`, `controles=3493`, contre `mhgp11_full_bench` (vérifié dans la commande enregistrée) |
| `cli_full_invariance.py --mode=relabel` sous `PYTHONOPTIMIZE=1 python3 -S -B` | `cli_full_relabel_verdict conforme runs12`, `controles=48` |
| `mhgp11_cli_full_determinism` | `runs30` (porte et jumelle conformes) |
| ASan+UBSan, Debug, `-DMHGP11_MODULES=core;api;cli` (`basan`), `ctest -L fast -j 2` | **134 portes : 133 sans échec** (dont `mhgp11_support_lidar_sentinel`, sautée faute de `MHGP11_DATA_DIR`), **1 délai** : `mhgp11_cli_full_determinism_opt` à 600 s sous charge 15 à 17, alors que sa jumelle a passé en 519 s ; rejouée seule, charge 5 à 7 : **conforme en 224 s** |
| Release u24, `-DMHGP11_MODULES=api;cli` (`b24`), `ctest -L fast -j 2` | **33/33** (sonde de référence `mhgp11_cli_full_reference`) |
| Clang 18, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only`, 11 sources S5 | aucun diagnostic (G4 n'a pas Clang) |
| `tools/check_style.py --root` | `style_ok fichiers=442` ; `--units api` 13, `cli` 6, `core` 34 (le compte rendu dit 5 pour `cli` : `fenv_preload.cpp` est venu après) |
| `run_mutants.py --check` | `manifeste_ok module=api mutants=9 plancher=9`, `manifeste_ok module=cli mutants=11 plancher=11` |
| Grammaire Python 3.10 (`ast.parse(feature_version=(3, 10))`), `assert`, imports | 5 scripts neufs et les 2 lecteurs réutilisés : conformes, aucun `assert`, bibliothèque standard seule |
| Épingles de la section S5 de `docs/PROVENANCE.md` | 6 sha256 recalculés, tous **exacts** (`full_probe.cpp`, `whole_input.hpp`, `points_export.cpp`, `full_semantic.py` à `f98aeed67` ; `cli_output.hpp`, `cli_options.hpp` au raccord R2 `865f5e6`) |
| `tools/check_docs.py` (racine) | rien sur les documents touchés ; 156 lignes, toutes des documents absents du checkout partiel |

Les 4 échecs de la suite rapide :
- `mhgp11_tower_full_paired_protocol` et `_opt` : `FileNotFoundError` sur
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`, absent du checkout partiel. Après
  extraction de ce reçu depuis `f98aeed67` dans **mon** instantané, les deux portes passent.
- `mhgp11_tower_full_campaign` et `_opt` : délai de 120 s sous charge 10 à 17 (porte Python sans natif, aucun fichier
  S5). Rejouées seules : **conformes en 85 s et 79 s**.

## 2. Contrôles indépendants

**Identité à l'octet** (`probes/identity_probe.py`, hors dépôt). Six nuages absents de la porte : uniforme de 2 000
points sur tout le domaine (K = 3 et 7), boîte de côté 10 riche en cosphériques (K = 3, 6, 9), boîte de côté 6 à
**K = 12**, 500 points au bord haut du domaine (K = 4 et 8), plan $z=0$, six grappes ; W1 et W5 ; PointId tirés sur
tout `u32`. **20/20 identiques en u21, 20/20 en u24** (sha256 du fichier = dump de la sonde = sha256 du manifeste).

**Échelle et LiDAR** (script de la porte, hors CTest, W2) :
- `--uniform=8000 --k=5` → `cli_full_identity_verdict conforme attempts2 refusals0`, `controles=14` (1 min) ;
- `--data=lidar_ng01 --k=5` → même ligne, `controles=14` (2 min 56 s).

Aucune donnée ni coordonnée de trame n'est recopiée ici.

**Contre origin/main** (`probes/pristine_compare.py`). Sondes `mhgp11_full_bench` et `mhgp11_points_export`
construites depuis `git archive f98aeed67`, comparées à celles de l'arbre S5 et à la sortie du CLI : uniforme de
3 000 points à K5 et boîte de côté 9 de 200 points à K7, W2. Dumps `MHGP11FUL1` identiques (et égaux au fichier du
CLI), fichiers `MHGP11PH` identiques, lignes JSON identiques une fois retirés les champs `*_ns` et `cpu_seconds`.

**Déterminisme au-delà de W4** : 1 500 points, K5, W1, W48 et W256 → fichiers et manifestes identiques.

**Même enchaînement d'allocations que la sonde.** Sur 3 000 points à K3 : pic du CLI (max des étages index, domaine,
arbre) 44 801 356 octets contre `peak_reserved_bytes` 44 753 356 pour la sonde, et 12 580 608 contre
`reserved_after_bytes` 12 532 608 : l'écart vaut exactement $16n=48\,000$ octets, les entrées que le CLI garde
(écart 8 du compte rendu). Le moteur alloue donc la même chose que la sonde au masque 16 379.

**`tree_k_sha256` publié** (`probes/tree_check.cpp`, lié à `b21/libmhgp11.a`). Six nuages, K = 1 à 6 : la valeur du
manifeste égale l'empreinte de l'ordre K recalculée à la main, 36 cas sur 36, et diffère de celle de l'ordre 1 dès
K ≥ 2. La valeur est juste ; c'est la porte qui manque (C1). Au passage : l'empreinte de l'ordre 1 d'un même nuage
change entre kmax = 1, 2 et 3, car les rangs sont ceux de $\mathrm{Cat}_{\mathrm{kmax}}$ (note pour S7, R11).

**Ordre des refus** (`probes/order_probe.py`, u21 et u24) :

| Cas (faute ou fautes) | Rendu | Conforme |
| --- | --- | --- |
| auto-test en faute (préchargement) + points absents | `environment_selftest`, `session`, code 3 | oui : session avant lecture |
| auto-test en faute + D existe | `output_conflict`, `plan` | oui |
| auto-test en faute + `--fils=0` | `parameter_out_of_range`, `options` | oui |
| ids tronqué + coordonnée hors domaine | `input_unreadable`, `read` | oui |
| hors domaine + PointId double + positions répétées | `coordinate_out_of_domain`, `compute` | oui |
| PointId double + positions répétées | `duplicate_point_id`, `compute` | oui |
| K = n + 1 et `--budget=1000` | `memory_budget`, `compute` | oui, au sens d'`api.hpp` (préparation du nuage, étape 5) ; le § 3 de `SORTIES.md` ne le dit pas (R2) |
| `--budget=79` (sous 16 n = 80) / `--budget=80` | `memory_budget` à `read` / à `compute` | oui |
| sortie standard fermée + D existe | `output_unwritable`, `plan`, ligne sur la sortie d'erreur | oui |
| sortie standard en **lecture seule** | `output_unwritable`, `plan` | oui (non joué par la porte, C3) |
| `--points=/dev/stdout`, sortie standard sur un fichier | `output_conflict`, `plan`, entrées intactes | oui |
| `--ids=` **lien symbolique** vers le fichier de la sortie standard | `output_conflict`, `plan`, ids intact | oui (non joué par la porte, C3) |
| sortie standard = entrée + D existe | `output_conflict` de la sortie standard d'abord | oui |
| `--fils=256`, `--budget=18446744073709551615`, `--k=005` | admis, code 0 | oui |

**Course** : quatre appels simultanés sur le même D (3 000 points, K3) → un succès, trois `output_conflict` à
l'étape `publish`, aucun `D.pending` ; le dossier gagnant passe `mhgp11_formats.check_directory`.

**Limite de taille de fichier** : `ulimit -f 2` → l'exécutable est tué par `SIGXFSZ` (code shell 153), laisse
`D.pending/full.mhgp11ful1` tronqué à 2 048 octets, et l'appel suivant est refusé `output_conflict` jusqu'au retrait à
la main. Avec `SIGXFSZ` ignoré (hérité par `trap '' XFSZ`), le même appel rend proprement `output_unwritable` à
l'étape `publish`, code 2, ni D ni `D.pending` (C2).

## 3. À corriger

### C1. `tree_k_sha256` du manifeste : aucune porte ne le juge

`src/api/manifest.cpp:225` écrit `api::tree_k_sha256(tower.order(product.k()))`. La fonction est jugée
(`mhgp11_api_session_tree_digest` la compare à une sérialisation à la main), mais **le champ publié ne l'est nulle
part** :
- `cli_full_identity.py:123-125` et `cli_full_invariance.py:104` ne comparent le champ qu'entre appels (fils, ordres
  d'entrée, réétiquetages), ce qu'une constante satisfait ;
- `mhgp11_formats.read_manifest` ne contrôle que sa forme (64 chiffres hexadécimaux) ;
- `tests/cli/cli_support.py:105-107` (`tree_digest_of`) n'est appelé par aucune porte.

Mutants de contrôle, appliqués à ma copie : `tree_k_ordre_un` (ordre 1 au lieu de kmax) et `tree_k_constante`
(`io::Digest{}`) **survivent tous deux aux 27 portes api et cli**. Correction : dans `tests/api/session_test.cpp`
(groupe `tree_digest`, l. 213-232, ou `released`), publier chaque produit et comparer le champ `"tree_k_sha256"` du
manifeste à `io::to_hex(digest_by_hand(product.full().order(k)))`, pour K = 1 à 4 ; inscrire `tree_k_ordre_un` dans
`tests/mutants/api.json` (porte `mhgp11_api_session_tree_digest`). Retirer `tree_digest_of` ou s'en servir.

### C2. `SIGXFSZ` n'est pas ignoré : signal et `D.pending` orphelin sous une limite de taille de fichier

`cli/mhgp11.cpp:292` ignore `SIGPIPE`, pas `SIGXFSZ`. Sous `RLIMIT_FSIZE`, l'écriture du fichier tue le processus :
c'est un arrêt par signal, donc un échec, et le destructeur d'`OutputDirectory` ne retire pas `D.pending`. Les portes
de S4 jouent déjà ce chemin avec `SIGXFSZ` ignoré (contre-lecture S4, R7). Correction : `::signal(SIGXFSZ, SIG_IGN);`
à côté de `SIGPIPE`, plus un cas du contrat (`resource.setrlimit(RLIMIT_FSIZE, …)` dans `preexec_fn`, attendu
`output_unwritable`, étape `publish`, code 2, ni D ni `D.pending`) et un mutant qui retire la ligne. L'essai avec
`trap '' XFSZ` montre que rien d'autre n'est à changer.

### C3. Contrat : sortie standard par lien symbolique et en lecture seule non jouées

Le code est juste (§ 2), mais `tests/cli/cli_contract.py:130-166` (`plan_cases`) ne joue ni une entrée désignée par un
**lien symbolique** vers le fichier de la sortie standard, ni une sortie standard ouverte **en lecture seule**. Les
mutants `stdout_lstat` (`cli/mhgp11.cpp:181`, `::stat` → `::lstat` : la ligne JSON s'ajoute alors à l'entrée) et
`stdout_lecture_seule_admise` (`cli/mhgp11.cpp:175`, test `O_RDONLY` retiré : refus seulement après calcul et
publication, à l'étape `report`) **survivent**. Correction : deux cas dans `plan_cases` (`os.symlink` vers `ids.u32le`
avec la sortie standard en ajout sur `ids.u32le` → `output_conflict`, `plan`, ids intact ; sortie standard ouverte
par `open(..., 'rb')` sur un fichier qui n'est pas une entrée → `output_unwritable`, `plan`, ni D ni `D.pending`), et
les deux mutants dans `tests/mutants/cli.json`.

### C4. Paramètres du moteur : rien ne les juge, et `kEngineMask` est mort

`src/api/internal.hpp:21` déclare `kEngineMask = 16379`, que rien n'emploie. Le mutant `parametres_moteur_par_defaut`
(`FullParams{}` au lieu de `api_detail::full_params()`, `src/api/compute.cpp:76`) survit : même objet, donc mêmes
octets, mais voie lente. Ce n'est pas une faute d'exactitude ; c'est le contrat « paramètres fixes du masque 16 379 »
(§ 5) et la mesure de L2, qui compare l'étage `tree` de `full` et de `supports`, qui en dépendent. Correction : un
groupe unitaire qui décode `kEngineMask` comme `bench/full_probe.cpp` (`main`, l. 349-388) et compare champ par champ
`catalogue_params(k)` et `full_params()` ; ou bien publier dans la ligne standard le masque recalculé depuis les
paramètres et le comparer, dans `cli_full_identity.py`, au champ `optimizations` de la sonde.

### C5. Code mort

- `api::stage_name` (`src/api/api.hpp:90`, `src/api/session.cpp:39-51`) : ni le CLI (qui écrit les clés en dur,
  `cli/mhgp11.cpp:274-282`) ni une porte ne l'appellent. L'employer pour les clés de la ligne standard, ou le retirer.
- `tree_digest_of` (`tests/cli/cli_support.py:105-107`), voir C1.

### C6. `session_fault.cpp` : le commentaire promet plus que le code

`tests/api/session_fault.cpp:6-8` annonce « la tour calculée ensuite est identique à celle d'avant les pannes » ; les
l. 153-156 comparent deux tours calculées **après** les pannes. Garder `kept` vivant (l. 131) et comparer à lui, ou
corriger le commentaire.

## 4. Remarques (sans correction exigée)

**R1. La garde K > n de `compute` est un mutant équivalent** (`src/api/compute.cpp:63`). Retirée, les 27 portes
passent : le moteur refuse K > n de lui-même, avec la même raison et à la même étape. Je n'ai trouvé aucun budget qui
la distingue : K ≤ 12 impose n ≤ 11 et, sur le nuage de 5 points du contrat, le pic de la préparation du nuage
(73 968 octets) dépasse celui du domaine (37 210 octets à K5), si bien qu'un budget qui laisse passer le nuage laisse
passer aussi l'index et le catalogue. La garde reste utile (elle épargne l'index et le catalogue) ; la dire
équivalente plutôt que lui chercher une porte.

**R2. `memory_budget` à l'étape 5.** La préparation du nuage peut refuser `memory_budget` avant le test K > n
(`api.hpp:102-107` le dit ; cas du § 2). Le § 3 du brouillon `SORTIES.md` ne liste `memory_budget` qu'aux étapes 4 et
7 : l'ajouter à l'étape 5.

**R3. Entrées gardées** : 16 octets par point de plus que la sonde, mesuré exactement (§ 2). Sans effet sur la règle
de L2 (même CLI des deux côtés), mais un `--budget` réglé sur la sonde peut refuser. Copier empreintes et tailles,
puis rendre les tampons d'entrée après `compute`, ramènerait l'empreinte à celle de la sonde.

**R4. La sortie d'erreur peut être une entrée.** `2>>ids.u32le` : un refus y ajoute son message (et la ligne JSON
quand la sortie standard est inutilisable). Cas d'appelant, non promis par la spécification ; le même contrôle d'inode
que pour la sortie standard le fermerait.

**R5. Triple faute.** Si la fin de session échoue après la publication (`budget_not_released`) et que `retract()`
échoue aussi, l'appel rend 3 et D reste publié. Le § 9 du brouillon `SORTIES.md` ne connaît que deux doubles échecs,
rendus 2 : ajouter ce cas, ou dire que le code 3 l'emporte.

**R6. Marge de temps sous sanitizer.** En Debug ASan sous forte charge, `mhgp11_cli_full_determinism` a pris 519 s pour
un délai de 600 s, et sa jumelle a dépassé ; seule, la jumelle passe en 224 s. G4 joue ASan en Release, donc plus
vite ; à surveiller.

**R7. Coût G4 des jumelles.** Les portes `mhgp11_cli_full_identity_scale8000`, `_scale16000`, `_scale32000`,
`_lidar_ng0{0,1,2}_k5` et `mhgp11_cli_full_determinism_scale8000` sont des portes Python sans label `long` : chacune a
une jumelle `_opt` qui rejoue tout le calcul natif (sonde et CLI, deux ordres d'entrée ; dix appels pour le
déterminisme), soit le double du travail en série (`RUN_SERIAL`). `gcc_release` a `timeout_seconds` 1 200 et la
matrice `budget_seconds` 1 380 (`tools/g4_matrix.json`) : à chiffrer avant la session, ou mettre ces portes en `long`
(sans jumelle) ou dans une configuration à part.

**R8.** `--budget=18446744073709551615` est admis et publié `"budget_bytes":18446744073709551615`, alors qu'il vaut
`kUnlimited` ; sans conséquence, mais deux manifestes pour un même budget illimité.

**R9. Panne mémoire dans `publish` non jouée.** `mhgp11_api_session_fault_starvation` refuse tour à tour chaque
allocation de `compute`, pas celles de `publish` (la chaîne du manifeste, `src/api/manifest.cpp:206-207`). Le chemin
passe par `guarded` (`memory_budget`, `D.pending` retiré par le destructeur), mais aucune porte ne le provoque ; un
groupe `publication` dans `tests/api/session_fault.cpp` le couvrirait à peu de frais.

**R10.** L'auto-test F5 est plus large que « F2 et F3 » : il refuse aussi une exception flottante démasquée (écart 5).
Je l'approuve : sans ce contrôle, la mesure des témoins pourrait tuer le processus par `SIGFPE`. Les quinze témoins
sont justes sous les quatre modes (vérifiés à la main : bornes `lo`/`hi`, zéro signé, `(2^{53}+1)-2^{53}`). Le contrôle
du mode est vacant sur x86 (deux bits valent toujours l'un des quatre modes) ; il reste une défense en profondeur.

**R11. Pour S7.** `tree_k_sha256` dépend du kmax du catalogue (rangs denses de $\mathrm{Cat}_{\mathrm{kmax}}$, § 2) :
l'égalité entre `full` et `supports` n'existe que si les deux domaines sont préparés à kmax = K, ce que le § 4 du
brouillon `SORTIES.md` sous-entend (« des rangs de deux appels ne se comparent pas »).

## 5. Intégration avec L0 et S6

- **Table des modules.** Vérifié : l'arbre S5 avec le `cmake/modules.cmake` du brouillon L0 (`MHGP11_DEPS_api` = … `supports
  points head`) ne se configure plus (`CMake Error at CMakeLists.txt:226 : le module 'supports' est requis`). Le
  brouillon d'`ARCHITECTURE.md` dit que `check_style` ne contrôle que les dossiers présents, mais `CMakeLists.txt`
  refuse toute fermeture qui cite un module absent. Garder la liste explicite de S5 tant que `supports`, `points` et
  `head` manquent, ou rendre `CMakeLists.txt` tolérant aux modules planifiés : décision d'intégrateur.
- **Ordre des raisons.** S5 met `environment_selftest` en 25 (consigne du lanceur), S6 met `support_shell_capacity` et
  `supports_invariant` en 25 et 26 dans son propre arbre, le brouillon L0 annonce S6 d'abord puis S5. Conflit textuel
  certain dans `src/core/reasons.def` et `tests/core/status_test.cpp`, quel que soit l'ordre retenu. Sans effet
  observable pour S5 : `environment_selftest` n'est émise qu'à la création de la Session et ne se fusionne jamais
  avec une autre raison. Mettre ensuite la table du § 3 de `SORTIES.md` à l'ordre retenu.
- **À écrire dans `SORTIES.md`, en plus des points du compte rendu** (`impl_s5.md` § 5, que j'approuve) : R2
  (`memory_budget` à l'étape 5), R5 (triple faute), `SIGXFSZ` ignoré comme `SIGPIPE` une fois C2 corrigé, et la
  sortie standard en lecture seule (`output_unwritable`, étape 2).

## 6. Mutants

**Campagnes `run_mutants.py --jobs 2`** (copie propre et construction Release u21 par mutant) :
- `api` : `mutants_ok module=api mutants=9 tues=9 dont_signal=0 dont_delai=0 dont_construction=0 plancher=9`, témoin
  vert, tous tués par le verdict `code` (23 h 00 → 23 h 19) ;
- `cli` : LIGNE_CLI.

**Mes mutants**, appliqués en place à ma copie `/tmp/v11-s5-verif/mut` (construction incrémentale, 27 portes `api` et
`cli` de label `fast`, restauration vérifiée par empreinte ; témoin sans mutation : 27/27) :

| Mutant | Endroit | Issue | Lecture |
| --- | --- | --- | --- |
| `tree_k_ordre_un` | `src/api/manifest.cpp:225` | **survit** | causal, C1 |
| `tree_k_constante` | `src/api/manifest.cpp:225` | **survit** | causal, C1 |
| `stdout_lstat` | `cli/mhgp11.cpp:181` | **survit** | causal, C3 |
| `stdout_lecture_seule_admise` | `cli/mhgp11.cpp:175` | **survit** | causal (ordre des refus), C3 |
| `k_sup_n_garde_omise` | `src/api/compute.cpp:63` | survit | équivalent, R1 |
| `parametres_moteur_par_defaut` | `src/api/compute.cpp:76` | survit | même objet ; contrat du moteur, C4 |
| `retrait_omis_fin_de_session` | `cli/mhgp11.cpp:308` | survit | chemin annoncé non couvert (écart 9 du compte rendu) |

## 7. Ce qui est vérifié conforme

- **Écrivain `MHGP11FUL1`** : relu ligne à ligne contre `serialize`, `birth_sphere`, `word` et `integer` ; mêmes
  champs, même ordre, mêmes expressions `i128` des centres, mots du type du profil ; seul le transport change
  (`FileWriter`, paquets de 512 mots, première erreur gardée).
- **Paramètres** : `catalogue_params` et `full_params` égalent ceux de `bench/points_export.cpp` et le décodage du
  masque 16 379 de `bench/full_probe.cpp` (`max_nodes` 0 et `ball_limit` `kNone` par défaut), confirmé par le même pic
  d'allocations (§ 2).
- **Session** : Pool, puis compte du budget, puis F5 ; budget et Pool uniques ; compte partagé, donc un produit qui
  survivrait à la Session rend ses octets sans comportement indéfini ; `close()` rend `budget_not_released` (code 3).
- **Transaction** : plan avant toute ouverture de fichier par le processus (la sortie standard est jugée sans rien
  ouvrir), création de `D.pending` après le calcul, manifeste en dernier, `retract()` après un échec de la ligne
  standard (portes `/dev/full` et tube sans lecteur), course arbitrée par `renameat2(RENAME_NOREPLACE)`.
- **Manifeste** : clés dans l'ordre du § 6.6, ni temps, ni fils, ni chemin ; forme canonique (`json.dumps` compact) ;
  identique à W1, W48, W256.
- **Portes** : codes exacts, planchers gravés (lignes exactes `refus54`, `attempts503 refusals7`, `runs30`, `runs12`,
  planchers des groupes unitaires égaux aux comptes), aucune ne repose sur `assert`, jumelles `-O` conformes.
- **Règles** : aucune exception ne traverse l'api (`noexcept` partout, `guarded` aux frontières), aucun flottant
  décisionnel (le seul flottant est l'auto-test), aucun crochet dans le produit (fautes réelles par `feenableexcept`
  et `LD_PRELOAD`), aucun octet de sortie existant changé, `docs/SORTIES.md`, `MATHEMATIQUES.md` et `README.md`
  intacts.

## 8. Pour la session G4

En plus de la liste du compte rendu (`impl_s5.md` § 6), que j'approuve :
- les portes et mutants ajoutés pour C1, C2 et C3, si l'implémenteur les livre avant ;
- `mhgp11_cli_full_identity_lidar_ng02_k5` (ni l'implémenteur ni moi ne l'avons jouée) ;
- TSan sur `mhgp11_cli_full_determinism` et `mhgp11_api_session_equivalence` (non joué ici) ;
- le coût des jumelles `_opt` des portes d'échelle et LiDAR (R7) à chiffrer avant de lancer `gcc_release`.

## 9. Ce que je n'affirme pas

- Aucune mesure de temps : les durées citées sont locales, sous charge, et ne qualifient rien.
- Ni TSan, ni K10, ni 16 000 ou 32 000 points, ni ng00 et ng02 n'ont été joués ici ; Clang n'a fait qu'une analyse
  syntaxique.
- Les campagnes de mutants ont tourné sur mon instantané, identique au worktree.
- Aucun statut public n'en découle.
