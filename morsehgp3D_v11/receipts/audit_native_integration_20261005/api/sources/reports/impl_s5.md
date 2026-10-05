# Tranche S5 — façade `api`, `Session` et `mhgp11 --sortie=full` : compte rendu d'implémentation

4 octobre 2026, 21 h 55 UTC (heure lue par `date -u`). Worktree `build/v11-impl-s5` (checkout partiel détaché à
`f98aeed67`). Rien n'est commité ni poussé ; aucune écriture hors du worktree, de `/tmp/v11-s5/` et de ce rapport.
**GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (u24 construit et joué localement aussi)
public_status=not_claimed
```

Autorité suivie : `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 3.5, 3.6, 4, 5, 6.2, 6.6, 6.7, 8.5, 8.6, 9.1-S5). Le brouillon L0 de `docs/SORTIES.md`
(`build/v11-impl-l0`, lu à 21 h 51) a servi à aligner les noms ; rien n'en est copié ni ne dépend de lui.

## 1. Fichiers

Chemins relatifs à `morsehgp3D_v11/`.

| Fichier | État | Rôle |
| --- | --- | --- |
| `src/api/api.hpp` | nouveau | En-tête public (seul inclus par `cli/`) : `OutputKind` (`full`), `SessionParams`, `Session` (`make`, `budget`, `pool`, `workers`, `close`), `CloudView`, `FullRequest`, `Request` (= `std::variant<FullRequest>`), `Stage`/`RunReport`, `compute`, `Product`, `valid_grid_step`, `valid_origin_coordinate`, `Provenance`, `kFullFileName`, `kManifestSchema`, `publish`, `tree_k_sha256`. Tout est `noexcept`, `Result`/`Outcome`. |
| `src/api/internal.hpp` | nouveau | Interne : paramètres fixes du moteur (`catalogue_params`, `full_params`, `kEngineMask = 16379`), `write_full`, `full_manifest`, `check_provenance`. |
| `src/api/selftest.hpp`, `selftest.cpp` | nouveaux | Auto-test F5 séparé en mesure (`measure_float_environment`) et jugement (`judge_float_environment`) ; `environment_selftest`. |
| `src/api/session.cpp` | nouveau | `Session::make` (Pool, puis budget, puis F5), `close` (`MemoryBudget::released`), noms des sorties et des étages. |
| `src/api/compute.cpp` | nouveau | Paramètres du masque 16379 ; `compute(full)` : `prepare_cloud`, positions répétées, K > n, `build_index`, `prepare_full_domain` (Pool), `build_full` ; rapport d'étages. |
| `src/api/write_full.cpp` | nouveau | Port octet pour octet de `serialize`/`birth_sphere` (`bench/full_probe.cpp`) et de `word`/`integer` (`bench/whole_input.hpp`) sur `io::FileWriter`. |
| `src/api/manifest.cpp` | nouveau | JSON déterministe du manifeste, `tree_k_sha256`, contrôle des décimaux, `publish` (création, écriture, manifeste, `commit`). |
| `src/api/module.cmake` | nouveau | Sources du module. |
| `src/core/reasons.def` | modifié | `MHGP11_REASON(environment_selftest, invariant_violated, api)` ajouté **en fin de table**. |
| `tests/core/status_test.cpp` | modifié | Copie gravée de la table : 26 lignes, plancher 89, dernière raison `environment_selftest` (sinon `mhgp11_core_unit_reasons` échoue). |
| `docs/ARCHITECTURE.md`, `cmake/modules.cmake` | modifiés (une ligne chacun) | Ligne `api` de la table : rôle « façade publique `api/api.hpp` et `Session` », dépendances **explicites** `core num sched cloud io index catalogue tower` au lieu de « tous » (voir § 4, écart 1). |
| `cli/cli.cmake` | nouveau | Cible `mhgp11_cli`, `OUTPUT_NAME mhgp11`, liée à `mhgp11` ; incluse avant CTest par `CMakeLists.txt` (inchangé). |
| `cli/mhgp11.cpp` | nouveau | Exécutable : options du § 5, ordre des refus, une ligne JSON, publication puis `retract()` si la ligne échoue. |
| `bench/mhgp11_formats.py` | nouveau | Lecteur bibliothèque standard : manifeste (ordre des clés, types, forme canonique à l'octet), dossier publié (inventaire, tailles, sha256, décodage `MHGP11FUL1` par `bench/full_semantic.py`, comptes recoupés), positions des `PointId` d'un `MHGP11FUL1`. |
| `tests/api/tests.cmake`, `api_support.hpp`, `session_test.cpp`, `session_fault.cpp`, `selftest_test.cpp`, `selftest_fault.cpp` | nouveaux | Portes `mhgp11_api_*`. |
| `tests/cli/tests.cmake`, `cli_support.py`, `cli_contract.py`, `cli_full_identity.py`, `cli_full_invariance.py`, `fenv_preload.cpp` | nouveaux | Portes `mhgp11_cli_*` ; sonde de référence `mhgp11_full_bench`, ou, hors construction complète, la même source sous le nom `mhgp11_cli_full_reference`. |
| `tests/mutants/api.json` (9 mutants), `tests/mutants/cli.json` (11 mutants) | nouveaux | Mutants du § 8.5 et causaux ajoutés. |
| `docs/PROVENANCE.md` | modifié | Section neuve en fin de fichier : « Façade api et exécutable mhgp11 (tranche S5) ». |

Non modifiés, volontairement : `bench/full_probe.cpp`, `bench/points_export.cpp` et leurs portes (aucun octet de
sortie existant ne change), `CMakeLists.txt`, `docs/SORTIES.md`, `docs/MATHEMATIQUES.md`, `README.md`,
`docs/implementation_status.toml`.

## 2. Choix

**Session** (`session.cpp`). Ordre : `sched::make_pool` (`parameter_out_of_range` hors de 1..256,
`session_overhead`), puis le compte du budget sous `guarded` (`memory_budget`), puis l'auto-test
(`environment_selftest`), comme l'étape 3 du § 5. Le budget est tenu par `unique_ptr` pour que la `Session` se
déplace (`Result<Session>`) ; une Session déplacée est vide et `close()` y rend un succès. `close()` rend
`MemoryBudget::released()` : `budget_not_released` (code 3) tant qu'un `Product`, ou n'importe quel `Buffer` de la
Session, vit encore.

**Auto-test F5** (`selftest.cpp`). Quinze témoins calculés sur des opérandes `volatile`, jugés sur les motifs
binaires (zéro signé confondu, car `2^53 - 2^53` vaut `-0` sous l'arrondi vers moins l'infini) :
- **T** : exceptions flottantes toutes masquées (MXCSR bits 7 à 12 sur x86, `fegetexcept()` de la glibc pour le x87).
  Une exception démasquée ferait d'une opération inexacte un `SIGFPE` au lieu d'une valeur, et F6 admet des seuils
  jusqu'à `2^-1074` (exception « opérande dénormal »). Le contrôle précède toute opération : la mesure ne calcule
  rien si une exception est démasquée.
- **M** : mode d'arrondi parmi les quatre modes IEEE.
- **F2** : six noyaux entiers exacts (sommes, différences, produits, conversion d'un `i64`), plus la précision :
  `((2^53 + 1) - 2^53)` vaut 0 ou 2, jamais 1 (précision étendue refusée).
- **F3** : huit opérations à résultat normal (addition, soustraction, multiplication, trois divisions dont une près de
  `2^998`, conversions d'un `u64` et d'un `i64`) dont le résultat doit être l'un des **deux voisins** binaire64 du
  résultat exact : la fidélité, pas le mode.
- FTZ et DAZ sont admis (aucun témoin dénormal ; la doctrine et les portes existantes les jouent déjà).

**Calcul** (`compute.cpp`). Paramètres fixes = ceux de `bench/points_export.cpp` = masque 16379 de
`bench/full_probe.cpp` avec `16 256 0 4294967295`. Ordre des refus : `k` hors de 1..12, nuage (`prepare_cloud`),
`multiplicity_unsupported` **avant** K > n (cas « trois points confondus, K > sites »), puis calcul. Le rapport
d'étages (durée, pic réservé par `restart_peak`) n'est écrit qu'en cas de succès.

**Écrivain `MHGP11FUL1`** (`write_full.cpp`). Port octet pour octet : magie de 10 octets sans bourrage, mots u64,
entiers signe-magnitude au nombre de mots **du type du profil** (`num::to_wide`, jamais réduit), mêmes expressions
`i128` pour les centres. Écriture par paquets de 512 mots sur la pile vers `FileWriter::u64s` ; la première erreur est
gardée. Le premier essai a donné le même sha256 que la sonde.

**Manifeste** (`manifest.cpp`). Clés en ordre fixe, sans espace, terminé par `\n`, entiers par `std::to_chars` ;
il ne contient que des littéraux ASCII, des empreintes hexadécimales et des décimaux contrôlés, donc
`json.dumps(obj, separators=(',', ':')) + '\n'` le réécrit à l'octet (contrôle du lecteur). Ni temps, ni fils, ni
chemin. Exemple (petit nuage de 5 points, K = 3) :

```json
{"schema":"ehgp.v11.output.v1","output":"full","status":"complete","public_status":"not_claimed","coord_bits":21,"k":3,"parameters":{"budget_bytes":null,"grid_step":null,"origin":null},"inputs":[{"name":"points","bytes":60,"sha256":"7e04…"},{"name":"ids","bytes":20,"sha256":"df95…"}],"files":[{"name":"full.mhgp11ful1","format":"MHGP11FUL1","version":1,"bytes":4954,"sha256":"81c6…"}],"tree_k_sha256":"a149…","counts":{"sites":5,"points":5,"orders":[{"k":1,"births":5,"nodes":8,"edges":7,"root":7},{"k":2,"births":6,"nodes":9,"edges":8,"root":8},{"k":3,"births":4,"nodes":6,"edges":5,"root":5}]}}
```

`counts` de `full` (fixés ici, le § 8 du brouillon L0 les renvoie à S5) : `sites`, `points`, puis par ordre `k`,
`births`, `nodes`, `edges`, `root` (les en-têtes d'ordre de `MHGP11FUL1`). `budget_bytes` vaut `null` sans
`--budget` ; `grid_step` est la chaîne de `--pas` ; `origin` est un **tableau de trois chaînes** (les trois
décimaux de `--origine`), ou `null`. `version` vaut 1 (suffixe de la magie). `tree_k_sha256` suit exactement § 6.6
(`MHGP11TK`, `u64 K`, `u64 N`, par nœud `parent`, `rank`, `birth_key`, nombre d'enfants en `u32`, puis les enfants),
sur l'ordre K de la tour.

**Publication** (`publish`). Contrôle de la provenance (`parameter_out_of_range` avant toute création), `create`,
écriture, manifeste en dernier, `commit` (io). Pour `full`, l'étage `output` est vide (le produit est la tour) ;
`write` couvre fichier, empreinte de l'arbre, manifeste, synchronisations et renommage.

**CLI** (`cli/mhgp11.cpp`, seulement `"api/api.hpp"`).
- Options : `--clé=valeur`, table de 12 noms connus ; forme, inconnue, répétée, `--sortie` absente, inconnue ou non
  livrée (`supports`, `points`, `plat`), option propre à `plat` (`--mcs`, `--z`, `--selection`), obligatoire absente,
  chemin vide, valeur hors domaine : `parameter_out_of_range`, avant tout effet. Entiers : chiffres ASCII seuls,
  jeton entier, borne avant conversion (leçons de R2 `cli_options.hpp`, aucune ligne portée). `--pas` et
  `--origine` contrôlés puis recopiés tels quels.
- L'état de la sortie standard est lu **avant les options**, sans rien ouvrir : ouverte en écriture, et fichier
  régulier égal (même périphérique, même inode) à l'un des fichiers nommés par `--points=` ou `--ids=` dans `argv`.
  Si elle n'est pas utilisable, toute ligne de refus va sur la sortie d'erreur, options comprises : aucune ligne n'est
  jamais écrite dans une entrée. Étape 2, avant que le processus n'ouvre quoi que ce soit : sortie fermée ou en
  lecture seule (`output_unwritable`), sortie égale à une entrée (`output_conflict`), puis
  `io::OutputDirectory::plan(D, {points, ids})`.
- Étapes 3 à 8 dans `execute` : Session, `io::read_u32le`, `compute`, `publish`, puis libération des entrées et du
  produit, puis `Session::close`. Un refus après la publication (fin de session) retire le dossier.
- Ligne de succès, **après** la publication :
  `{"phase":"mhgp11","output":"full","status":"ok","reason":"none","coord_bits":21,"k":…,"workers":…,"sites":…,"stages_ns":{"cloud","index","domain","tree","attach","output","write","total"},"peaks_bytes":{"cloud","index","domain","tree","output","write"},"counts":{"nodes","births","edges"},"manifest_sha256":"…"}`,
  où `cloud` = lecture + préparation (aligné sur le brouillon L0), `attach` et `output` nuls pour `full`, `counts`
  = totaux sur les ordres 1..K.
- Ligne de refus : `{"phase":"mhgp11","output":"full"|null,"status":…,"reason":…,"stage":"options|plan|session|read|compute|publish|close|report","coord_bits":…}` ;
  `output` est `null` tant que les options ne sont pas lues ; `stage` rend l'ordre des refus vérifiable.
- `SIGPIPE` ignoré : un tube sans lecteur rend `EPIPE`. Si la ligne de succès ne passe pas (`fflush`, `ferror`) :
  `retract()`, puis ligne de refus `output_unwritable`/`report` sur la sortie d'erreur, code 2.
- Codes : `exit_code` (0, 2, 3) ; aucun autre.

**Lecteur** (`bench/mhgp11_formats.py`). Réutilise `full_semantic.decode` sans le modifier ; ajoute le schéma du
manifeste, l'inventaire exact du dossier (fichiers du manifeste + `manifeste.json`, `D.pending` absent), les recoupements
de tailles, sha256 et comptes, et `full_point_ids`.

## 3. Portes jouées (résultats exacts, locaux)

Constructions sous `/tmp/v11-s5/`, au plus 2 cœurs (`-j 2`), machine partagée (charge 7 à 10).

**Release u21, tous modules** (`b21`) :
- `mhgp11_api_session_*` : `released` 370 contrôles, `product_alive` 12, `refusals` 50, `equivalence` 217,
  `tree_digest` 165, `provenance` 46 ; inventaire `inventaire_ok tests=6`. Planchers gravés à ces valeurs.
- `mhgp11_api_session_fault_*` : `session` 25 contrôles, `starvation` 167 contrôles, ligne
  `starvation allocations=80 refus=80` (chaque allocation du calcul refusée tour à tour rend `memory_budget`, sans
  réservation laissée ; planchers 19 et 60, car le nombre d'allocations dépend de la libstdc++).
- `mhgp11_api_selftest_modes` 81 contrôles (4 modes × 4 états FTZ/DAZ), `mhgp11_api_selftest_judge` 66.
- `mhgp11_api_selftest_fault_none|upward|ftz_daz` : `selftest_probe none`, code 0 ;
  `mhgp11_api_selftest_fault_inexact|underflow|denormal` : `selftest_probe environment_selftest`, code 3.
- `mhgp11_cli_contract` : `cli_contract_verdict conforme refus54 temoins3` (54 refus : 30 à l'étape des options,
  dont une option fausse avec la sortie standard sur une entrée ; 9 au plan, dont sortie standard fermée et sortie
  standard égale à une entrée ; 1 code 3 à la session, par bibliothèque préchargée ; 12 à la lecture et au calcul ;
  2 après la publication, `/dev/full` et tube sans lecteur, dossier retiré ; 3 témoins, dont la recopie de `--pas`,
  `--origine` et `--budget` dans le manifeste).
- `mhgp11_cli_full_identity` : `cli_full_identity_verdict conforme attempts503 refusals7`,
  `cli_full_identity_ok controles=3493`, contre `mhgp11_full_bench` lui-même dans la construction complète
  (31 nuages : 11 fixtures du § 2.9, 6 petites boîtes, 6 positions génériques, 2 au bord haut du domaine,
  2 alignements ; K = 1..min(5, n) ; deux ordres d'entrée ; W1 et W4 ; K = n + 1 refusé par les deux programmes ;
  15 s).
- `mhgp11_cli_full_determinism` : `cli_full_determinism_verdict conforme runs30`, `controles=66` (uniform_u18 de
  1 200 points et 400 points d'une boîte de côté 12 ; K = 1, 3, 5 ; W1, W2, W4, répétition, permutation ; 27 s).
- `mhgp11_cli_full_relabel` : `cli_full_relabel_verdict conforme runs12`, `controles=48` (PointId non denses, 0 et
  0xFFFFFFFF compris : seuls les mots `PointId` changent, à l'image près ; `tree_k_sha256` et comptes identiques).
- Jumelles `_opt` : toutes conformes. Les scripts passent aussi sous `python3 -S -B` et leur grammaire est celle de
  Python 3.10 (`ast.parse(..., feature_version=(3, 10))`).
- Échelle et LiDAR joués une fois, hors CTest, à W4 : `--uniform=8000 --k=5` → `cli_full_identity_verdict conforme
  attempts2 refusals0` (59 s) ; `--data=lidar_ng00 --k=5` → même ligne (2 min 16 s). Sur ng00 à K5, le fichier
  `full.mhgp11ful1` fait 300 883 482 octets et le manifeste 1 047 ; l'étage `write` (sha256 au fil de l'écriture et
  `fsync` de 300 Mo) a pris 2,09 s sur ce Codespace chargé. Diagnostic local, non reçu, aucune revendication.
- Bancs inchangés : `mhgp11_tower_full_bench_io` (`full_io_verdict conforme attempts427 successes413 refusals14`),
  `mhgp11_tower_points_export_width`, `mhgp11_tower_full_bench_semantic`, `mhgp11_tower_public_header_*`,
  `mhgp11_core_unit_reasons` : conformes.
- `python3 -S -B tools/check_style.py --root morsehgp3D_v11` : `style_ok fichiers=442` ; `--units api` 13,
  `--units cli` 5, `--units core` 34.
- `run_mutants.py --check` : `manifeste_ok module=api mutants=9 plancher=9`,
  `manifeste_ok module=cli mutants=11 plancher=11`.
- Suite rapide complète `ctest -L fast` (760 portes) : voir § 3 bis.

**Mutants, vérification locale** (hors dépôt, `/tmp/v11-s5/tools/mutate_local.py` : mutation **en place** dans le
worktree, construction incrémentale des seules cibles de la porte, porte jouée par `ctest`, restauration exacte du
fichier vérifiée par `diff` contre une copie). Les 20 mutants sont tués, tous par le verdict `code` :

| Manifeste | Mutant | Porte |
| --- | --- | --- |
| api | `session_sans_liberation` | `mhgp11_api_session_product_alive` |
| api | `selftest_omis` | `mhgp11_api_selftest_fault_inexact` |
| api | `selftest_pieges_ignores`, `selftest_noyaux_entiers_omis`, `selftest_precision_etendue_admise` | `mhgp11_api_selftest_judge` |
| api | `multiplicite_apres_k` | `mhgp11_api_session_refusals` |
| api | `tree_k_sans_enfants` | `mhgp11_api_session_tree_digest` |
| api | `provenance_non_controlee`, `pas_nul_admis` | `mhgp11_api_session_provenance` |
| cli | `option_hors_sortie_admise`, `k_treize_admis`, `budget_nul_admis`, `budget_ignore`, `entree_sur_sortie_standard_admise`, `sortie_standard_non_controlee`, `retrait_omis`, `pas_non_recopie` | `mhgp11_cli_contract` |
| cli | `fils_ignores`, `ecriture_full_permutee`, `manifeste_taille_fausse` | `mhgp11_cli_full_identity` |

Ce n'est **pas** la campagne `run_mutants.py` (copie propre et construction complète par mutant, environ dix minutes
de bibliothèque à un cœur chacune) : elle reste à jouer sur G4 (`mhgp11_mutants_api`, `mhgp11_mutants_cli`).

**ASan + UBSan, Debug** (`basan`, `-DMHGP11_MODULES=core;api;cli`, donc toute la bibliothèque) : `ctest -L fast`,
134 portes, toutes conformes (`mhgp11_cli_contract` après mise à jour de la ligne attendue ; sous sanitizer, le cas
préchargé n'est pas joué). Ligne de panne identique : `starvation allocations=80 refus=80`. Rejoué après les
dernières retouches du CLI : voir § 8.

**Release u24** (`b24`, `-DMHGP11_COORD_BITS=24 -DMHGP11_MODULES=api;cli`) : `ctest -L fast`, 33 portes, toutes
conformes, mêmes lignes de verdict (`attempts503 refusals7`, `runs30`, `runs12`) ; le contrat, rejoué après les
dernières retouches, rend `cli_contract_verdict conforme refus54 temoins3`. La sonde de référence y est
`mhgp11_cli_full_reference` (même source que `mhgp11_full_bench`).

### 3 bis. Suite rapide complète u21

`ctest -L fast -j 2` sur `b21` (760 portes, avant les deux dernières retouches du CLI) : **757 conformes**, une
sautée (`mhgp11_support_lidar_sentinel`, sans `MHGP11_DATA_DIR`), trois en échec, toutes étrangères à S5 :
- `mhgp11_tower_full_paired_protocol` et `_opt` : `FileNotFoundError` sur
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`, absent du checkout partiel (même
  constat que S4) ;
- `mhgp11_tower_full_campaign_opt` : délai de 120 s dépassé sous une charge de 7 à 8 sur 8 cœurs (porte Python sans
  natif) ; rejouée seule, la porte prend 115 à 120 s et l'une ou l'autre jumelle dépasse tour à tour. Aucun fichier
  qu'elle lit n'est touché par S5.
Après les retouches, les 62 portes `api`, `cli`, `core`, `style` et manifestes de mutants de `b21` ont été rejouées :
toutes conformes (`refus54 temoins3`, `attempts503 refusals7`, `runs30`, `runs12`, `style_ok fichiers=442`, dix
manifestes `manifeste_ok`).

## 4. Écarts à la spécification, et pourquoi

1. **Table des modules : dépendances explicites de `api`.** La ligne `api` disait « tous ». `check_style` impose que
   `MHGP11_DEPS_api` égale cette liste, et `CMakeLists.txt` refuse la configuration si un module de la fermeture
   manque (`points`, `head`, et bientôt `supports`, sont absents). Un module `api` présent ne se configurait donc
   pas. J'ai écrit la liste explicite des modules qu'`api` inclut (`core num sched cloud io index catalogue
   tower`), ce que la table et `check_style` admettent, **sans ajouter `supports`**. Le rôle devient « façade
   publique `api/api.hpp` et `Session` » (la règle `[module]` exige `api/api.hpp`).
   **Conflit à résoudre à l'intégration** : le brouillon L0 garde « tous » et ajoute `supports points head` à
   `MHGP11_DEPS_api` ; avec `src/api/` présent, la configuration échoue tant que ces trois modules n'existent pas.
   Chaque tranche qui fait inclure un module par `api` l'ajoute à la ligne (S7 : `supports`, S9 : `points`,
   S10 : `head`), ou « tous » revient quand tous existent.
2. **Ordre des raisons.** Consigne du lanceur suivie : `environment_selftest` est ajoutée maintenant, en fin de
   table, et S6 ajoutera `support_shell_capacity` puis `supports_invariant` après elle. Le brouillon L0 (§ 3 de
   `SORTIES.md`) annonce l'ordre inverse (S6 livrée en L1 avant S5 en L2). L'ordre ne départage que deux refus au
   même K, mais `tests/core/status_test.cpp` le grave : l'intégrateur doit trancher et mettre la copie gravée à jour
   (26 lignes ici, 28 après S6).
3. **Signature de `publish`.** `publish(Session&, const Product&, io::OutputDirectory&, const Provenance&,
   RunReport*)` : le `Request` n'est plus passé, le produit garde le sien (aucune incohérence possible).
   `Provenance` porte aussi `budget_bytes` (optionnel) et `origin` en trois chaînes.
4. **`Request` et `OutputKind` réduits à `full`.** Pas de types morts pour les sorties non livrées ; S7, S9 et S10
   ajoutent leurs requêtes. Le CLI refuse `supports|points|plat` avant tout effet.
5. **Auto-test plus large que « F2 et F3 » au sens strict** : il refuse aussi une exception flottante démasquée
   (hypothèse implicite de F3 : une opération rend une valeur). La faute injectée des portes est réelle :
   `feenableexcept` dans la sonde, et une bibliothèque préchargée (`LD_PRELOAD`) pour l'exécutable. Aucun crochet dans
   le produit.
6. **Contrôle de la sortie standard** (ajout, constat R2 de la contre-lecture de S4) : son état est lu avant les
   options, et refusé à l'étape 2, avant le plan du dossier : sortie fermée ou en lecture seule →
   `output_unwritable` ; sortie égale à une entrée → `output_conflict`. Toute ligne de refus va alors sur la sortie
   d'erreur, même à l'étape 1. Le § 5 de la spec ne le prévoyait pas.
7. **Clé `stage` dans la ligne de refus, clés `total` et `manifest_sha256` dans la ligne de succès, six pics** au lieu
   de trois ; `counts` de `full` = `nodes`, `births`, `edges` (totaux), au lieu de `nodes`, `balls`, `supports`.
8. **Entrées gardées pendant le calcul et la publication** (empreintes de provenance) : 16 octets par point de plus
   que la sonde, qui rend l'entrée avant FULL.
9. **Code 3 du contrat** : le seul chemin provocable sans crochet est l'auto-test, par préchargement. Sous ASan ou
   TSan, un préchargement précéderait le runtime du sanitizer : le cas n'y est pas joué, et la ligne attendue
   l'indique (`refus53` au lieu de `refus54`). `budget_not_released` après publication n'est pas provocable sans
   crochet ; le chemin (retrait, code 3) est écrit mais non couvert.
10. **Porte de déterminisme rapide** à 1 200 points (et non 2 000 : 27 s au lieu de 42 s, jumelle `_opt` comprise
    dans le temps de la suite) ; la variante `mhgp11_cli_full_determinism_scale8000` (W1, W8, W48) est enregistrée
    pour G4.
11. **Sonde de référence** : `mhgp11_full_bench` quand les portes de la tour l'ont déclarée (construction complète) ;
    sinon (construction `-DMHGP11_MODULES=cli` de la campagne de mutants, ou `api;cli`) la même source
    `bench/full_probe.cpp` sous le nom `mhgp11_cli_full_reference`. Mêmes octets.
12. **Aucun `json.cpp` dans `io`** : le JSON est dans `api/manifest.cpp`, comme le demande la consigne (et la
    contre-lecture de S4).
13. **`tree_k_sha256` n'est recalculable depuis aucun fichier publié** : `MHGP11FUL1` n'a ni `LevelRank` ni `BallIdx`,
    et `MHGP11SP` sobre n'a pas `BallIdx`. Il ne sert qu'à apparier des sorties ; les portes le comparent entre
    fils, permutations et réétiquetages, et une unité le recalcule depuis la forêt en mémoire. Si une traçabilité
    vérifiable par un tiers est voulue (`Zoltan/FoundationModel/SPECIFICATION.md`, invariant « Traçabilité »), il
    faudra une empreinte sur des champs publiés (niveaux exacts réduits, centres) : question pour L0.
14. **`--pas` et `--origine` recopiés sans normalisation** : `0.0010` et `0.001` donnent deux manifestes différents.

## 5. Ce que le contrat L0 devra documenter

Dans `docs/SORTIES.md` :
- § 1 : forme exacte des entiers (chiffres ASCII seuls, sans signe, zéros de tête admis, jeton entier) ; grammaire des
  décimaux (`[0-9]+(\.[0-9]+)?`, au plus 64 octets, pas strictement positif, origine avec `-` initial possible) ;
  `--origine` = trois décimaux séparés par des virgules.
- § 3, étape 2 : le contrôle de la sortie standard **avant** le plan du dossier (fermée ou en lecture seule :
  `output_unwritable` ; même inode qu'un fichier nommé par `--points=` ou `--ids=` : `output_conflict`) ; son état
  est lu avant même les options, et toute ligne de refus va alors sur la sortie d'erreur.
- § 3, forme exacte de la ligne de refus : clés `phase`, `output` (`null` à l'étape des options), `status`, `reason`,
  `stage` (`options`, `plan`, `session`, `read`, `compute`, `publish`, `close`, `report`), `coord_bits` ; ligne de
  succès avec `total` et `manifest_sha256`, `peaks_bytes` à six clés, `counts` de `full`.
- § 3 : `SIGPIPE` ignoré ; après la publication, la ligne de refus `output_unwritable`/`report` va sur la sortie
  d'erreur ; le refus `close` (fin de session, `budget_not_released`) retire aussi le dossier.
- § 3, table des raisons : l'ordre réel de `reasons.def` (écart 2).
- § 8 : `counts` de `full` (`sites`, `points`, `orders[k, births, nodes, edges, root]`), `origin` en tableau de trois
  chaînes, saut de ligne final, forme canonique (`json.dumps` compact), `version` 1 de `MHGP11FUL1`.
- § 8 : la limite de `tree_k_sha256` (écart 13) est déjà écrite pour `MHGP11SP` ; l'écrire aussi pour `MHGP11FUL1`.

Dans `docs/ARCHITECTURE.md` : la ligne `api` (écart 1) ; § 4, F5 : contenu de l'auto-test (exceptions masquées, mode,
F2 sur six noyaux et la précision, F3 sur huit témoins, FTZ/DAZ admis) ; § 7.2 est déjà amendé par L0.

Dans `README.md` : l'exécutable `mhgp11 --sortie=full` livré par S5 (commande type, codes, dossier publié) et la
réserve « qualification G4 en attente ».

Dans le registre des preuves : rien (aucun énoncé mathématique nouveau ; l'auto-test n'a aucun rôle de preuve).

## 6. Ce qui doit tourner sur G4

Enregistré, avec ses labels :
- Matrice : GCC Release u21 et u24 (portes `fast` des unités `api`, `cli`, `core`, dont `mhgp11_cli_full_identity` en
  u24, qui lit `--bits` du profil), ASan/UBSan (unités `api`, `cli`), TSan (au moins `mhgp11_api_session_equivalence`,
  `mhgp11_cli_full_determinism`, qui exercent le Pool à W2 et W4), Clang si présent.
- Échelle : `mhgp11_cli_full_identity_scale8000`, `_scale16000`, `_scale32000` (K5, W8), `_scale32000_k10`
  (`long`), `mhgp11_cli_full_determinism_scale8000` (W1, W8, W48).
- LiDAR : `mhgp11_cli_full_identity_lidar_ng00_k5`, `_ng01_k5`, `_ng02_k5`, `_ng00_k10` (`long`).
- Mutants : `mhgp11_mutants_api` (9) et `mhgp11_mutants_cli` (11), labels `mutant long`.
- Mesure (L2, hors portes) : la ligne standard de `mhgp11 --sortie=full` sur ng00 à ng02, K5, W1 et W48, trois prises,
  à apparier avec `--sortie=supports` (règle de décision de L2). À relever : l'étage `write` de `full` (300 Mo,
  sha256 logiciel et `fsync`) pèsera plus que l'arbre ; il ne fait pas partie de la mesure `tree`.

## 7. Points ouverts

- Écarts 1 et 2 : conflits textuels certains avec L0 (`docs/ARCHITECTURE.md` l. 48, `cmake/modules.cmake`,
  `src/core/reasons.def`, `tests/core/status_test.cpp`).
- `budget_not_released` après publication : écrit, non couvert (aucun crochet permis).
- Les refus d'`io` sur des noms de fichiers constants (`parameter_out_of_range`, `output_conflict` pour un nom en
  double) restent des refus d'entrée (code 2) et non des invariants (remarque R5 de la contre-lecture de S4) : ils
  sont impossibles avec le seul nom `full.mhgp11ful1`, et les traduire demanderait une raison sans porte.
- Écart 13 (`tree_k_sha256` non recalculable) : à trancher en L0.

## 8. Rejeu final (après les dernières retouches du CLI)

Retouches : état de la sortie standard lu avant les options (aucune ligne de refus dans une entrée), étage `cloud` =
lecture + préparation et étage `output` vide pour `full` (alignement sur le brouillon L0), sonde de référence =
`mhgp11_full_bench` dans la construction complète, cas de contrat « option fausse, sortie standard sur une entrée ».

- Release u21 (`b21`) : 62 portes `api`, `cli`, `core`, `style`, manifestes de mutants : toutes conformes ;
  `cli_contract_verdict conforme refus54 temoins3`, `cli_full_identity_verdict conforme attempts503 refusals7`
  (contre `mhgp11_full_bench`), `cli_full_determinism_verdict conforme runs30`,
  `cli_full_relabel_verdict conforme runs12`, `style_ok fichiers=442`.
- ASan + UBSan Debug (`basan`) : 42 portes `api`, `cli`, `core`, `style` : toutes conformes ;
  `cli_contract_verdict conforme refus53 temoins3` (sans le cas préchargé), mêmes lignes pour l'identité, le
  déterminisme et le réétiquetage, `starvation allocations=80 refus=80`.
- Release u24 (`b24`) : contrat rejoué, `refus54 temoins3` ; le reste inchangé depuis le § 3.
- Mutants : les 20 rejoués en place sur `b21` avec le code final, tous tués par le verdict `code` ; sources
  restaurées à l'identique (`diff -r` contre une copie).
