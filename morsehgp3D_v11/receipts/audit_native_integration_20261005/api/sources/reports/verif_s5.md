# Contre-lecture de la tranche S5 « api, Session et `mhgp11 --sortie=full` »

Première contre-lecture le 4 octobre 2026 (instantané du worktree à 22 h 27, rapport écrit de 23 h 22 à 23 h 26 UTC,
interrompue avant son retour structuré). **Reprise et complétée le 5 octobre 2026 de 05 h 49 à 06 h 23 UTC**, après
le redémarrage du conteneur de 05 h 36 qui a vidé `/tmp`. Heures lues par `date -u`. Contre-lecteur indépendant.

Worktree lu : `build/v11-impl-s5` (diff non commité sur `f98aeed67` : 5 fichiers suivis modifiés, 7 entrées
nouvelles). **Rien n'a été modifié dans ce worktree.** Constructions et essais dans `/tmp/v11-s5-verif/`, au plus deux
cœurs par construction, machine partagée. Contrat de référence ajouté pour la reprise : **L0 commité**,
`origin/main` = `5adf6a59f` (`docs/SORTIES.md` § 3, 8, 9 et 10 ; `MATHEMATIQUES.md` § 10), réponses de l'auditeur
`de4ab58a8` et `aef7182b3` (D.1 à D.4) et son modèle `receipts/audit_supports_implementation_20261004/evidence/check_d2_signature.py`,
lus par `git show origin/main:…` sans rebaser le worktree. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (u24 construit et joué les 4 et 5 octobre)
public_status=not_claimed
```

Autorité suivie : `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 3.5, 3.6, 4, 5, 6.2, 6.6, 6.7, 8.5, 8.6, 9.1-S5) ; compte rendu `impl_s5.md` lu en entier ; depuis le 5 octobre,
le contrat L0 commité, qui prime sur la spécification pour ce qu'il fixe (signature `tree_k_sha256` version 2, état
`published_complete`, ordre des refus du § 3).

## 0. Reprise du 5 octobre : ce qui a été refait

- **Worktree inchangé depuis la première contre-lecture.** HEAD `f98aeed67` ; `git status --short` : les mêmes 12
  entrées (5 + 7) ; `--ignored` : 12 ; 593 fichiers sous `morsehgp3D_v11/` comme l'instantané du 4 octobre ; aucun
  fichier modifié après 22 h 24 UTC (`src/api/api.hpp`, le dernier touché, avant l'instantané de 22 h 27). Nouvel
  instantané à 05 h 50 (`/tmp/v11-s5-verif/snap_manifest_0550.sha256`, 1 009 fichiers du worktree), comparé en fin de
  reprise (§ 10).
- **Release u21 reconstruite à neuf** (`b21`, 812 portes, aucun avertissement, 4 min à deux cœurs). Les 27 portes
  `api` et `cli` de label `fast` passent et rendent **exactement** les lignes du 4 octobre : `refus54 temoins3`
  (`controles=270`), `attempts503 refusals7` (`controles=3493`), `runs30` (66), `runs12` (48), groupes
  `370/12/50/217/165/46`, `25/167`, `starvation allocations=80 refus=80`, `81/66`, sondes F5 `none` ×3 et
  `environment_selftest` ×3.
- **Campagne `cli`**, seul point non conclu du rapport interrompu : jouée, 11/11 (§ 7).
- **Campagne `api`** rejouée pour ancrer le reçu perdu (§ 7).
- **Cinq mutants de cru de plus** (§ 7), dont quatre survivent.
- **Alignement sur le contrat L0 commité** : jugé (§ 3), avec quatre sondes hors dépôt (signature version 2 recalculée
  par le modèle de l'auditeur sur les champs natifs, double échec après publication reproduit sans crochet, empreinte
  du manifeste après un commit refusé, fin de session en échec après publication).
- **Release u24 reconstruite** (`b24`, `-DMHGP11_MODULES=api;cli`) : 33/33 portes `fast`, mêmes lignes de verdict
  qu'en u21 ; elle sert aussi à donner les valeurs version 2 du profil 24 bits (§ 2).
- **Non refait**, car conclu le 4 octobre sur ce même worktree : suite `fast` complète (760 portes), ASan+UBSan,
  Clang, sondes d'identité indépendantes, ordre des refus, course, échelle et LiDAR. Seul `SIGXFSZ` (C2) a été
  rejoué, en une commande : même issue.

## Verdict

**Aucun constat bloquant** sur l'exactitude, le budget, la concurrence ou le déterminisme de ce que S5 calcule et
publie : `full.mhgp11ful1` est, octet pour octet, le dump de la sonde (503 tentatives en u21 et en u24, 40 tentatives
indépendantes, trame ng01 à K5, deux nuages contre une sonde construite depuis `origin/main`), aucun octet de sortie
existant ne change, et l'ordre des refus tient sur 14 cas indépendants. Campagnes de mutants : `api` 9/9, `cli`
11/11.

En revanche, **le contrat L0 commité depuis le lancement de la tranche n'est pas encore tenu**, sur trois points à
corriger **avant l'intégration** (§ 3, L1 à L3) :
- `tree_k_sha256` publié est la **version 1** (`BallIdx` des naissances), que `SORTIES.md` § 8 déclare « jamais
  publiée ». La version 2 se calcule avec ce que S5 a déjà en mémoire : nourri des champs natifs, le modèle de
  l'auditeur retrouve **à l'octet ses trois valeurs publiées** (singleton, paire à K1, ligne à K2), et 22 valeurs de
  plus sont prêtes à graver ;
- l'état **`published_complete`** n'existe ni dans le résultat de l'api ni dans la ligne de refus. Reproduit **sans
  crochet**, quatre fois sur quatre : sortie standard en échec puis retrait refusé, l'appel rend 2, le dossier reste
  publié et complet, et la ligne JSON ne le dit pas ;
- l'empreinte du manifeste est **perdue** quand une étape échoue après sa fermeture (`src/io/directory.cpp:230`
  l'affecte après la publication ; sonde : zéros après un commit refusé alors que le manifeste était fermé).

S'y ajoutent les trous de portes de la première contre-lecture (C1 à C6, § 4), toujours valables, et quatre
survivants nouveaux (ordre sortie standard puis dossier, sortie standard sur `--points`, clé `output` de la ligne de
refus, retrait annoncé sans publication). Tout se corrige en quelques dizaines de lignes.

## 1. Portes rejouées (résultats exacts, locaux)

| Contrôle | Résultat |
| --- | --- |
| **5 oct.** Release u21 reconstruite (`b21`, 812 portes), 27 portes `mhgp11_api_*` et `mhgp11_cli_*` de label `fast`, `-j 1` | **27/27**, lignes identiques à celles du 4 octobre (§ 0) |
| **5 oct.** Copie des sources (`mutown`, `-DMHGP11_MODULES=api;cli`, sonde `mhgp11_cli_full_reference`), 8 portes `mhgp11_cli_*` | **8/8** (témoin des mutants de cru) |
| **5 oct.** Release u24 reconstruite (`b24`, `-DMHGP11_MODULES=api;cli`), `ctest -L fast -j 2` | **33/33** : `refus54 temoins3`, `attempts503 refusals7`, `runs30`, `runs12`, `manifeste_ok` api 9 et cli 11, `style_ok fichiers=19` |
| Release u21, construction complète propre (`b21`, 812 portes enregistrées), `ctest -L fast -j 2` (4 oct.) | **760 portes : 756 conformes, 4 échecs étrangers à S5** (détail ci-dessous) |
| Portes `mhgp11_api_*` et `mhgp11_cli_*` de label `fast`, u21 (4 oct.) | **27/27** |
| `mhgp11_api_session` (six groupes, inventaire) | `controles=` 370, 12, 50, 217, 165, 46 ; `inventaire_ok tests=6` |
| `mhgp11_api_session_fault` | `session` 25 (plancher 19), `starvation` 167 (plancher 60), ligne `starvation allocations=80 refus=80` |
| `mhgp11_api_selftest` | `modes` 81, `judge` 66 ; sonde : `none`, `upward`, `ftz_daz` → code 0 ; `inexact`, `underflow`, `denormal` → `selftest_probe environment_selftest`, code 3 |
| `cli_contract.py` sous `python3 -S -B` | `cli_contract_verdict conforme refus54 temoins3`, `cli_contract_ok controles=270` |
| `cli_full_identity.py` sous `python3 -S -B` | `cli_full_identity_verdict conforme attempts503 refusals7`, `controles=3493`, contre `mhgp11_full_bench` (vérifié dans la commande enregistrée) |
| `cli_full_invariance.py --mode=relabel` sous `PYTHONOPTIMIZE=1 python3 -S -B` | `cli_full_relabel_verdict conforme runs12`, `controles=48` |
| `mhgp11_cli_full_determinism` | `runs30` (porte et jumelle conformes) |
| ASan+UBSan, Debug, `-DMHGP11_MODULES=core;api;cli` (`basan`), `ctest -L fast -j 2` (4 oct.) | **134 portes : 133 sans échec** (dont `mhgp11_support_lidar_sentinel`, sautée faute de `MHGP11_DATA_DIR`), **1 délai** : `mhgp11_cli_full_determinism_opt` à 600 s sous charge 15 à 17, alors que sa jumelle a passé en 519 s ; rejouée seule, charge 5 à 7 : **conforme en 224 s** |
| Release u24, `-DMHGP11_MODULES=api;cli` (`b24`), `ctest -L fast -j 2` (4 oct.) | **33/33** (sonde de référence `mhgp11_cli_full_reference`) |
| Clang 18, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only`, 11 sources S5 | aucun diagnostic (G4 n'a pas Clang) |
| `tools/check_style.py --root` | `style_ok fichiers=442` ; `--units api` 13, `cli` 6, `core` 34 (le compte rendu dit 5 pour `cli` : `fenv_preload.cpp` est venu après) |
| `run_mutants.py --check` | `manifeste_ok module=api mutants=9 plancher=9`, `manifeste_ok module=cli mutants=11 plancher=11` |
| Grammaire Python 3.10 (`ast.parse(feature_version=(3, 10))`), `assert`, imports | 5 scripts neufs et les 2 lecteurs réutilisés : conformes, aucun `assert`, bibliothèque standard seule |
| Épingles de la section S5 de `docs/PROVENANCE.md` | 6 sha256 recalculés, tous **exacts** (`full_probe.cpp`, `whole_input.hpp`, `points_export.cpp`, `full_semantic.py` à `f98aeed67` ; `cli_output.hpp`, `cli_options.hpp` au raccord R2 `865f5e6`) ; l'épingle de `full_probe.cpp` est aussi celle du port annoncé par L0 (`2d3a37cc…`) |
| `tools/check_docs.py` (racine) | rien sur les documents touchés ; 156 lignes, toutes des documents absents du checkout partiel |

Les 4 échecs de la suite rapide du 4 octobre :
- `mhgp11_tower_full_paired_protocol` et `_opt` : `FileNotFoundError` sur
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`, absent du checkout partiel. Après
  extraction de ce reçu depuis `f98aeed67` dans l'instantané du contre-lecteur, les deux portes passent.
- `mhgp11_tower_full_campaign` et `_opt` : délai de 120 s sous charge 10 à 17 (porte Python sans natif, aucun fichier
  S5). Rejouées seules : **conformes en 85 s et 79 s**.

## 2. Contrôles indépendants

**Identité à l'octet** (4 oct., `probes/identity_probe.py`, hors dépôt). Six nuages absents de la porte : uniforme de
2 000 points sur tout le domaine (K = 3 et 7), boîte de côté 10 riche en cosphériques (K = 3, 6, 9), boîte de côté 6 à
**K = 12**, 500 points au bord haut du domaine (K = 4 et 8), plan $z=0$, six grappes ; W1 et W5 ; PointId tirés sur
tout `u32`. **20/20 identiques en u21, 20/20 en u24** (sha256 du fichier = dump de la sonde = sha256 du manifeste).

**Échelle et LiDAR** (4 oct., script de la porte, hors CTest, W2) :
- `--uniform=8000 --k=5` → `cli_full_identity_verdict conforme attempts2 refusals0`, `controles=14` (1 min) ;
- `--data=lidar_ng01 --k=5` → même ligne, `controles=14` (2 min 56 s).

Aucune donnée ni coordonnée de trame n'est recopiée ici.

**Contre origin/main** (4 oct., `probes/pristine_compare.py`). Sondes `mhgp11_full_bench` et `mhgp11_points_export`
construites depuis `git archive f98aeed67`, comparées à celles de l'arbre S5 et à la sortie du CLI : uniforme de
3 000 points à K5 et boîte de côté 9 de 200 points à K7, W2. Dumps `MHGP11FUL1` identiques (et égaux au fichier du
CLI), fichiers `MHGP11PH` identiques, lignes JSON identiques une fois retirés les champs `*_ns` et `cpu_seconds`.

**Déterminisme au-delà de W4** (4 oct.) : 1 500 points, K5, W1, W48 et W256 → fichiers et manifestes identiques.

**Même enchaînement d'allocations que la sonde** (4 oct.). Sur 3 000 points à K3 : pic du CLI (max des étages index,
domaine, arbre) 44 801 356 octets contre `peak_reserved_bytes` 44 753 356 pour la sonde, et 12 580 608 contre
`reserved_after_bytes` 12 532 608 : l'écart vaut exactement $16n=48\,000$ octets, les entrées que le CLI garde
(écart 8 du compte rendu). Le moteur alloue donc la même chose que la sonde au masque 16 379.

**`tree_k_sha256` version 1 publié** (4 oct., `probes/tree_check.cpp`). Six nuages, K = 1 à 6 : la valeur du manifeste
égale l'empreinte version 1 de l'ordre K recalculée à la main, 36 cas sur 36. La valeur est juste au sens de la
spécification ; c'est la porte qui manque (C1), et c'est la version qui est périmée (L1). L'empreinte de l'ordre 1
d'un même nuage change entre kmax = 1, 2 et 3, car les rangs sont ceux de $\mathrm{Cat}_{\mathrm{kmax}}$ (R11).

**Signature version 2 sur les champs natifs** (5 oct., `probes/sig_dump.cpp` lié à `b21/libmhgp11.a`, et
`probes/sig_v2_check.py`). La sonde extrait de la tour calculée par `api::compute` les sites en ordre `SiteIdx`
(coordonnées), puis, par nœud de l'ordre K en numérotation canonique, `parent`, `rank`, `kind` (0 feuille de site à
K1, 1 naissance de boule, 2 fusion : déduit de `birth_key` et de l'ordre), la naissance (le `SiteIdx` à K1, sinon
$S^*$ = `catalogue().balls_data()[birth_key].support[0..qmin)`) et les enfants. Le hachage est fait par la fonction
`signature` du **modèle de l'auditeur**, pas par moi. Résultats :
- les trois valeurs publiées par l'auditeur sont **retrouvées à l'octet** : singleton $(3,2,1)$ à K1
  `8e991c2147dd5344a2012998e4052bddec912f533989cc8bef0376f26d7d12e6` ; paire $(0,2,0),(2,0,0)$ à K1
  `4878adc74b9cd5b03f6c5d95e60c95faeb0d4462e785ed01eeff093c214e5c46` ; ligne $(0,0,0),(1,0,0),(2,0,0)$ à K2
  `82f39093f987bd9dccaa10339251072d3c033e7f8dfa4dc34e0c7a17ba20a0a1`. La numérotation canonique du moteur, ses rangs,
  son $S^*$ et l'ordre de Morton tiennent donc les hypothèses du modèle ;
- 25 cas en tout (les trois, plus carré, triangle droit, `growth_ABCZ`, triangle aigu, témoin D2
  $A,B,C,Z,W$ de l'auditeur, et 40 points dans $[0,9)^3$, K = 1 à 4) : signature inchangée par permutation de l'entrée
  et réétiquetage (PointId près de $2^{32}-1$) ; listes d'enfants du moteur égales à celles que le modèle déduit des
  parents (donc croissantes) ; naissances d'abord (`kind` 0 à K1, 1 sinon), fusions ensuite ; $S^*$ croissant, d'arité
  2 à 4, dans les sites ; `sig_v2_check conforme lignes=25 ecarts=0` ;
- la valeur version 1 que S5 publie diffère dans les 25 cas (singleton : `569f648f…`) ;
- profil 24 bits (`sig_dump24`, lié à `b24/libmhgp11.a`) : pour les trois fixtures de l'auditeur, l'objet extrait
  (sites, nœuds, rangs, naissances, enfants) est identique à celui du profil 21 bits, et seule la signature change,
  par `coord_bits` : singleton `4ee1bfe286ebb0d15a9b19d649cb7233f83b790e4472d94ba9acf36252070996`, paire
  `eb765ff8c867e7fb001eccb6f618938de063d4d0526aa7b712d12197aea2f863`, ligne
  `e6d408f77aa2acebd8c06e86cc476c515bbf893bd208699657b5d6bd45228961` (modèle de l'auditeur avec `bits=24`).

**Double échec après publication, sans crochet** (5 oct., `probes/double_failure.py`). La sortie standard est un tube
**plein** (65 536 octets non lus) : la ligne de succès bloque. Dès que D apparaît, la sonde crée `D.pending`, puis
ferme le lecteur : l'écriture rend `EPIPE` (`SIGPIPE` est ignoré), le CLI appelle `retract()`, que `D.pending` fait
refuser. Issue, quatre fois sur quatre : code 2 ; sur la sortie d'erreur
`{"phase":"mhgp11","output":"full","status":"resource_exhausted","reason":"output_unwritable","stage":"report","coord_bits":21}`
puis le message français « dossier NON retire » ; **D reste publié et complet** (`mhgp11_formats.check_directory`
conforme). Rien dans la ligne JSON ne déclare `published_complete` ni l'empreinte du manifeste (L2). C'est une recette
de porte déterministe, sans crochet ni préchargement.

**Empreinte du manifeste après un commit refusé** (5 oct., `probes/manifest_digest_probe.cpp`, module `io` seul). Plan
de D, un fichier créé, D apparu avant le commit : `commit` → `output_conflict`, `committed()` faux, le manifeste a
pourtant été écrit et fermé ; `manifest_sha256()` rend **64 zéros** au lieu de
`sha256(manifeste) = 4bf48a37…c1be` (L3).

**Fin de session en échec après publication** (5 oct., `probes/close_after_publish.cpp`, niveau api). Un `Product`
encore vivant : `close()` → `budget_not_released` (code 3) ; `retract()` réussit et D disparaît. Même enchaînement
avec un `E.pending` créé avant le retrait : `retract()` → `output_conflict`, **E reste publié et complet**, l'empreinte
est gardée (le commit avait réussi). Le chemin est donc provocable sans crochet au niveau de l'api ; le CLI, lui, ne
le peut pas (son `Product` sort de portée avant `close()`), d'où la recommandation de L2 de porter cette fin d'appel
dans l'api.

**Ordre des refus** (4 oct., `probes/order_probe.py`, u21 et u24) :

| Cas (faute ou fautes) | Rendu | Conforme |
| --- | --- | --- |
| auto-test en faute (préchargement) + points absents | `environment_selftest`, `session`, code 3 | oui : session avant lecture |
| auto-test en faute + D existe | `output_conflict`, `plan` | oui |
| auto-test en faute + `--fils=0` | `parameter_out_of_range`, `options` | oui |
| ids tronqué + coordonnée hors domaine | `input_unreadable`, `read` | oui |
| hors domaine + PointId double + positions répétées | `coordinate_out_of_domain`, `compute` | oui |
| PointId double + positions répétées | `duplicate_point_id`, `compute` | oui |
| K = n + 1 et `--budget=1000` | `memory_budget`, `compute` | oui au sens d'`api.hpp` (préparation du nuage, étape 5) ; le § 3 de `SORTIES.md` commité ne le dit toujours pas (L4) |
| `--budget=79` (sous 16 n = 80) / `--budget=80` | `memory_budget` à `read` / à `compute` | oui |
| sortie standard fermée + D existe | `output_unwritable`, `plan`, ligne sur la sortie d'erreur | oui (non joué par la porte : mutant `plan_avant_sortie_standard` survivant) |
| sortie standard en **lecture seule** | `output_unwritable`, `plan` | oui (non joué par la porte, C3) |
| `--points=/dev/stdout`, sortie standard sur un fichier | `output_conflict`, `plan`, entrées intactes | oui (non joué par la porte : mutant `sortie_standard_entree_ids_seulement` survivant) |
| `--ids=` **lien symbolique** vers le fichier de la sortie standard | `output_conflict`, `plan`, ids intact | oui (non joué par la porte, C3) |
| sortie standard = entrée + D existe | `output_conflict` de la sortie standard d'abord | oui |
| `--fils=256`, `--budget=18446744073709551615`, `--k=005` | admis, code 0 | oui |

L'ordre réel de `io::read_u32le` (ouverture et `fstat` des deux fichiers, tailles, `index_overflow_u32`, admission de
16 octets par point, lecture complète) est **exactement** celui du § 3, étape 4, de `SORTIES.md` commité.

**Course** (4 oct.) : quatre appels simultanés sur le même D (3 000 points, K3) → un succès, trois `output_conflict` à
l'étape `publish`, aucun `D.pending` ; le dossier gagnant passe `mhgp11_formats.check_directory`.

**Limite de taille de fichier** (4 oct., rejouée le 5 oct.) : `ulimit -f 2` → l'exécutable est tué par `SIGXFSZ`
(code shell 153), laisse `D.pending/full.mhgp11ful1` tronqué à 2 048 octets, et l'appel suivant est refusé
`output_conflict` à l'étape `plan` jusqu'au retrait à la main. Avec `SIGXFSZ` ignoré (hérité par `trap '' XFSZ`), le
même appel rend proprement `output_unwritable` à l'étape `publish`, code 2, ni D ni `D.pending` (C2).

## 3. Alignement sur le contrat L0 commité (`origin/main` `5adf6a59f`)

### L1. `tree_k_sha256` : passer à la signature version 2 (SORTIES § 8 ; réponse D.2 de l'auditeur)

État : `src/api/manifest.cpp:148-163` hache « `MHGP11TK`, `u64 K`, `u64 N`, par nœud `parent`, `rank`, `birth_key`,
nombre d'enfants, puis tous les enfants » ; `birth_key` est un `BallIdx` à K ≥ 2. Le contrat commité fixe la version 2
et précise que la version 1 « n'est jamais publiée ».

Correction attendue :
- `src/api/api.hpp:162-166` : déclaration et commentaire ; la fonction a besoin du domaine (géométrie et $S^*$) :
  par exemple `io::Digest tree_k_sha256(const FullDomain& domain, const OrderForest& forest)`.
- `src/api/manifest.cpp:148-163` : flux exact, petit-boutiste, sans bourrage :
  `MHGP11TK` ; `u64` 2, `kCoordBits`, K, n, N ; les 32 octets bruts de
  SHA-256(`MHGP11GX`, `u64 kCoordBits`, `u64 n`, puis `u32` x, y, z de chaque site dans l'ordre des `SiteIdx`) ; puis
  par nœud, dans l'ordre canonique : `u32 parent` (`kNone` à la racine), `u32 rank`, `u8 kind` (0 si ordre 1 et
  `birth_key != kNone`, 1 si ordre ≥ 2 et `birth_key != kNone`, 2 si fusion), `u8` arité a, les a `SiteIdx` de la
  naissance en `u32` (le site `birth_key` à K1 ; `support[0..qmin)` de la boule `birth_key` du catalogue sinon ; rien
  pour une fusion), `u32 child_count`, puis les enfants en `u32` (déjà croissants : `forest.children(node)`).
- `src/api/manifest.cpp:225` : `api::tree_k_sha256(tower.domain(), tower.order(product.k()))`.
- Porte : `tests/api/session_test.cpp:48-65` (`digest_by_hand`) réécrite en version 2, et `tree_digest`
  (`session_test.cpp:213-232`) complété par les **trois valeurs de l'auditeur, gravées** par profil (§ 2 : 21 bits
  `8e991c21…`, `4878adc7…`, `82f39093…` ; 24 bits `4ee1bfe2…`, `eb765ff8…`, `e6d408f7…` ; la signature contient
  `coord_bits`), au moins un témoin à fusion multiple et le témoin D2 (valeurs du § 2 de ce rapport, si
  l'implémenteur les retrouve par sa propre sérialisation) ; plus C1 (le champ publié comparé au calcul à la main).
- Mutants (`tests/mutants/api.json`) : `tree_k_version_un` (mot de version 1), `tree_k_sans_geometrie` (empreinte de
  géométrie omise), `tree_k_naissance_omise` (a = 0 pour une naissance de boule), `tree_k_ordre_un` (C1), chacun tué par
  `mhgp11_api_session_tree_digest`.
- Textes : `bench/mhgp11_formats.py:16-18` (la docstring parle des `BallIdx` ; la version 2 n'est pas recalculable
  depuis `MHGP11FUL1` faute de $S^*$, elle l'est depuis `MHGP11SP`, S7) ; commentaire d'en-tête de `manifest.cpp`.

### L2. État `published_complete` (SORTIES § 3 et § 9 ; réponse D.3 de l'auditeur)

État : `api::publish` rend `Result<io::Digest>` ; sur un refus, rien ne dit si D reste publié. Le CLI tente bien
`retract()` sur tout échec après publication (`cli/mhgp11.cpp:306-320`), mais quand le retrait échoue il ne l'écrit
qu'en français sur la sortie d'erreur ; la ligne JSON (`cli/mhgp11.cpp:241-259`) est celle d'un refus sans
publication. Reproduit sans crochet (§ 2).

Correction attendue :
- Résultat de l'api : `publish` (et la fin d'appel, ci-dessous) rend l'état de publication (`none` ou
  `published_complete`) et l'empreinte du manifeste fermé (`io::OutputDirectory::manifest_sha256`, après L3) ; sur un
  refus de `commit` avec `committed()` vrai (double échec de la synchronisation du parent), l'état est
  `published_complete`.
- Fin d'appel portée dans l'api : `Session::close`, puis sur refus `retract()`, puis l'état. Le CLI l'appelle ;
  une porte api peut alors provoquer le troisième chemin sans crochet (un `Product` vivant), ce que le CLI ne permet
  pas (`retrait_omis_fin_de_session` survit aujourd'hui).
- Ligne de refus : forme fixée en S5 avec `mhgp11_cli_contract`. Proposition : deux clés finales toujours présentes,
  `"publication":"none"` ou `"published_complete"` et `"manifest_sha256":null` ou l'empreinte ; sur la sortie standard
  si elle peut être écrite, sinon sur la sortie d'erreur. Trois chemins : (i) commit refusé avec `committed()` vrai
  (synchronisation du parent puis retour arrière en échec), et retrait du CLI refusé lui aussi, code 2 ; (ii) fin de
  session en échec, retrait refusé, **code 3** ; (iii) ligne standard en échec, retrait refusé, code 2. Si le retrait
  réussit, l'état est `none` : rien n'est publié.
- Portes : dans `mhgp11_cli_contract` (`after_commit_cases`, l. 250-262), le cas du tube plein avec `D.pending` créé
  quand D apparaît (§ 2) : code 2, `publication=published_complete`, empreinte = sha256 de `D/manifeste.json`, D
  conforme, entrées intactes ; et `publication=none` dans tous les autres refus. Dans `mhgp11_api_session`, un groupe
  `after_publish` : `Product` vivant → `budget_not_released`, code 3, D retiré ; même chose avec `D.pending` créé avant
  la fin d'appel → `published_complete` et l'empreinte. Le chemin (i) exige une panne de `fsync` sur le parent :
  porte de faute G4 (bibliothèque préchargée de test sur `fsync` et `syscall(SYS_renameat2)`, hors sanitizers comme
  `fenv_preload`), enregistrée avec son label.
- Mutants : `etat_publie_omis` (`cli`, état toujours `none`) et `retrait_omis_fin_de_session` (`api`), tués par ces
  portes ; mon survivant `retrait_sans_publication` (§ 7) est tué aussi dès que l'état dérive du retrait et que la porte
  exige `none` hors des doubles échecs.

### L3. Empreinte du manifeste conservée dès sa fermeture (SORTIES § 9, étape 4)

`src/io/directory.cpp:225-232` (`commit_steps`) n'affecte `manifest_sha256_` qu'après `publish()` (l. 230). Correction :
déplacer l'affectation juste après `write_manifest` (entre les l. 228 et 229) et corriger le commentaire
`src/io/io.hpp:172` (« zéros avant un commit réussi » → « dès la fermeture du manifeste, même si la publication échoue
ensuite »). Porte : `tests/io/transaction_test.cpp:103-131` (`noreplace`), après chacun des trois commits refusés
par un D apparu (dossier vide `D`, fichier `E`, `F` sans fichier de données) : `manifest_sha256()` égale `digest_of`
du manifeste `{}` alors que `committed()` est faux (plancher 17 → 20) ; c'est exactement la sonde du § 2.
Mutant `empreinte_manifeste_apres_publication` dans `tests/mutants/io.json` (l'affectation remise après
`publish()`), tué par `mhgp11_io_transaction_noreplace` ; plancher du manifeste 21 → 22. Aucune porte existante ne
dépend d'une empreinte nulle après échec (vérifié par recherche).

### L4. Ordre des refus et garde de la sortie standard (SORTIES § 3)

Le comportement suit le § 3 : sortie standard puis plan à l'étape 2 ; session à l'étape 3 ; ordre réel de
`read_u32le` à l'étape 4 ; fin de session (`budget_not_released`, code 3) **avant** la ligne d'état à l'étape 9, les
deux retirant le dossier. Restent :
- `cli/mhgp11.cpp:8-17` : le commentaire décrit huit étapes ; le réécrire sur les neuf du § 3, avec l'étape 4 complète
  (`input_unreadable` des ouvertures et des tailles, `index_overflow_u32`, `memory_budget`, puis `input_unreadable` de
  la lecture incomplète ou de l'octet de trop) et l'étape 9 (fin de session puis ligne, retrait).
- `src/api/api.hpp:102-107` : dire que `memory_budget` de la préparation du nuage précède le test K > n (cas
  `K = n + 1`, `--budget=1000`, § 2) ; et ajouter `memory_budget` à l'étape 5 du § 3 de `SORTIES.md` à l'intégration
  (la préparation du nuage doit précéder le test K > n, puisque les positions répétées, étape 5, ne se voient
  qu'après elle).
- Porte (`tests/cli/cli_contract.py:130-166`, `plan_cases`) : deux fautes « sortie standard fermée + D existe » →
  `output_unwritable`, `plan` (tue `plan_avant_sortie_standard`) ; sortie standard en ajout sur le fichier de
  **`--points`** → `output_conflict`, points intact (tue `sortie_standard_entree_ids_seulement`) ; et les deux cas de
  C3 ; juger aussi la clé `output` de la ligne de refus, `null` à l'étape des options (tue
  `ligne_refus_sortie_toujours_full`).
- Facultatif : à l'étape 4, deux cas à deux fautes au niveau du CLI (`--budget=1` et tailles incohérentes →
  `input_unreadable` ; fichiers creux de $2^{32}-1$ points et `--budget=1` → `index_overflow_u32`), déjà couverts au
  niveau du module par `mhgp11_io_unit_input_sizes`.

### L5. Ordre des raisons

Le tableau du § 3 de `SORTIES.md` prévoit `support_shell_capacity` et `supports_invariant` (S6) **avant**
`environment_selftest` ; S5 met `environment_selftest` en fin de table à `f98aeed67`. Le contrat laisse
`reasons.def` faire foi si l'intégration suit un autre ordre, à condition de mettre à jour au même commit le tableau
et la copie gravée de `tests/core/status_test.cpp`. Décision d'intégrateur ; sans effet observable pour S5
(`environment_selftest` n'est émise qu'à la création de la Session et ne se fusionne jamais avec une autre raison).

### L6. Déterminisme à W48 (SORTIES § 10, remarque)

Le § 10 demande des portes de déterminisme à W1, W4 et W48. La porte rapide joue W1, W2 et W4 sur 1 200 points ; W48
n'est joué que par `mhgp11_cli_full_determinism_scale8000` (G4). Ajouter W48 à la porte rapide ne coûte presque rien
(j'ai joué W48 et W256 à la main le 4 octobre : identiques).

## 4. À corriger (première contre-lecture, toujours valable)

### C1. `tree_k_sha256` du manifeste : aucune porte ne juge le champ publié

`src/api/manifest.cpp:225` écrit `api::tree_k_sha256(tower.order(product.k()))`. La fonction est jugée
(`mhgp11_api_session_tree_digest` la compare à une sérialisation à la main), mais **le champ publié ne l'est nulle
part** :
- `cli_full_identity.py:123-125` et `cli_full_invariance.py:104` ne comparent le champ qu'entre appels (fils, ordres
  d'entrée, réétiquetages), ce qu'une constante satisfait ;
- `mhgp11_formats.read_manifest` ne contrôle que sa forme (64 chiffres hexadécimaux) ;
- `tests/cli/cli_support.py:105-107` (`tree_digest_of`) n'est appelé par aucune porte.

Mutants de contrôle (4 oct.) : `tree_k_ordre_un` (ordre 1 au lieu de kmax) et `tree_k_constante` (`io::Digest{}`)
**survivent tous deux aux 27 portes api et cli**. Correction, à faire avec L1 : dans `tests/api/session_test.cpp`
(groupe `released`, l. 81 et suivantes, ou `tree_digest`), publier chaque produit et comparer le champ
`"tree_k_sha256"` du manifeste à la signature version 2 calculée à la main, pour K = 1 à 4 ; inscrire
`tree_k_ordre_un` dans `tests/mutants/api.json`. Retirer `tree_digest_of` ou s'en servir.

### C2. `SIGXFSZ` n'est pas ignoré : signal et `D.pending` orphelin sous une limite de taille de fichier

`cli/mhgp11.cpp:292` ignore `SIGPIPE`, pas `SIGXFSZ`. Sous `RLIMIT_FSIZE`, l'écriture du fichier tue le processus :
c'est un arrêt par signal, donc un échec, et le destructeur d'`OutputDirectory` ne retire pas `D.pending`. Les portes
de S4 jouent déjà ce chemin avec `SIGXFSZ` ignoré (contre-lecture S4, R7). Correction : `::signal(SIGXFSZ, SIG_IGN);`
à côté de `SIGPIPE`, plus un cas du contrat (`resource.setrlimit(RLIMIT_FSIZE, …)` dans `preexec_fn`, attendu
`output_unwritable`, étape `publish`, code 2, ni D ni `D.pending`) et un mutant qui retire la ligne. L'essai avec
`trap '' XFSZ` montre que rien d'autre n'est à changer (rejoué le 5 octobre).

### C3. Contrat : sortie standard par lien symbolique et en lecture seule non jouées

Le code est juste (§ 2), mais `tests/cli/cli_contract.py:130-166` (`plan_cases`) ne joue ni une entrée désignée par un
**lien symbolique** vers le fichier de la sortie standard, ni une sortie standard ouverte **en lecture seule**. Les
mutants `stdout_lstat` (`cli/mhgp11.cpp:181`, `::stat` → `::lstat` : la ligne JSON s'ajoute alors à l'entrée) et
`stdout_lecture_seule_admise` (`cli/mhgp11.cpp:175`, test `O_RDONLY` retiré : refus seulement après calcul et
publication, à l'étape `report`) **survivent**. Correction : deux cas dans `plan_cases` (`os.symlink` vers `ids.u32le`
avec la sortie standard en ajout sur `ids.u32le` → `output_conflict`, `plan`, ids intact ; sortie standard ouverte
par `open(..., 'rb')` sur un fichier qui n'est pas une entrée → `output_unwritable`, `plan`, ni D ni `D.pending`), et
les deux mutants dans `tests/mutants/cli.json`. Même endroit pour les cas de L4.

### C4. Paramètres du moteur : rien ne les juge, et `kEngineMask` est mort

`src/api/internal.hpp:21` déclare `kEngineMask = 16379`, que rien n'emploie. Le mutant `parametres_moteur_par_defaut`
(`FullParams{}` au lieu de `api_detail::full_params()`, `src/api/compute.cpp:76`) survit : même objet, donc mêmes
octets, mais voie lente. Ce n'est pas une faute d'exactitude ; c'est le contrat « paramètres fixes du masque 16 379 »
(spécification § 5 ; `SORTIES.md` § 1, « Moteur ») et la mesure de L2, qui compare l'étage `tree` de `full` et de
`supports`, qui en dépendent. Correction : un groupe unitaire qui décode `kEngineMask` comme `bench/full_probe.cpp`
(`main`, l. 349-388) et compare champ par champ `catalogue_params(k)` et `full_params()` ; ou bien publier dans la
ligne standard le masque recalculé depuis les paramètres et le comparer, dans `cli_full_identity.py`, au champ
`optimizations` de la sonde.

### C5. Code mort

- `api::stage_name` (`src/api/api.hpp:90`, `src/api/session.cpp:39-51`) : ni le CLI (qui écrit les clés en dur,
  `cli/mhgp11.cpp:274-282`) ni une porte ne l'appellent. L'employer pour les clés de la ligne standard, ou le retirer.
- `tree_digest_of` (`tests/cli/cli_support.py:105-107`), voir C1.

### C6. `session_fault.cpp` : le commentaire promet plus que le code

`tests/api/session_fault.cpp:6-8` annonce « la tour calculée ensuite est identique à celle d'avant les pannes » ; les
l. 153-156 comparent deux tours calculées **après** les pannes. Garder `kept` vivant (l. 131) et comparer à lui, ou
corriger le commentaire.

## 5. Remarques (sans correction exigée)

**R1. La garde K > n de `compute` est un mutant équivalent** (`src/api/compute.cpp:63`). Retirée, les 27 portes
passent : le moteur refuse K > n de lui-même, avec la même raison et à la même étape. Je n'ai trouvé aucun budget qui
la distingue : K ≤ 12 impose n ≤ 11 et, sur le nuage de 5 points du contrat, le pic de la préparation du nuage
(73 968 octets) dépasse celui du domaine (37 210 octets à K5), si bien qu'un budget qui laisse passer le nuage laisse
passer aussi l'index et le catalogue. La garde reste utile (elle épargne l'index et le catalogue) ; la dire
équivalente plutôt que lui chercher une porte.

**R2. `memory_budget` à l'étape 5** : devenu L4 (le § 3 de `SORTIES.md` commité ne le liste toujours qu'aux étapes 4
et 7).

**R3. Entrées gardées** : 16 octets par point de plus que la sonde, mesuré exactement (§ 2). Sans effet sur la règle
de L2 (même CLI des deux côtés), mais un `--budget` réglé sur la sonde peut refuser. Copier empreintes et tailles,
puis rendre les tampons d'entrée après `compute`, ramènerait l'empreinte à celle de la sonde.

**R4. La sortie d'erreur peut être une entrée.** `2>>ids.u32le` : un refus y ajoute son message (et la ligne JSON
quand la sortie standard est inutilisable). Cas d'appelant, non promis par `SORTIES.md` ; le même contrôle d'inode que
pour la sortie standard le fermerait.

**R5. Triple faute** (fin de session puis retrait en échec) : couverte désormais par L2 (code 3 et
`published_complete`).

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
groupe `publication` dans `tests/api/session_fault.cpp` le couvrirait à peu de frais. La chaîne est bornée par une
constante (K ≤ 12, décimaux de 64 octets au plus), donc permise hors budget par l'`ARCHITECTURE.md` § 7.1.

**R10.** L'auto-test F5 est plus large que « F2 et F3 » : il refuse aussi une exception flottante démasquée (écart 5).
Je l'approuve : sans ce contrôle, la mesure des témoins pourrait tuer le processus par `SIGFPE`. Les quinze témoins
sont justes sous les quatre modes (vérifiés à la main : bornes `lo`/`hi`, zéro signé, `(2^{53}+1)-2^{53}`), comme
l'auditeur l'a vérifié de son côté (`api/check.py` de son reçu). Le contrôle du mode est vacant sur x86 (deux bits
valent toujours l'un des quatre modes) ; il reste une défense en profondeur.

**R11. Pour S7.** `tree_k_sha256` dépend du kmax du catalogue (rangs denses de $\mathrm{Cat}_{\mathrm{kmax}}$, § 2) :
l'égalité entre `full` et `supports` n'existe que si les deux domaines sont préparés à kmax = K. C'est le cas pour
`full` (kmax = K) et pour `supports` (`build_order` sur $\mathrm{Cat}_K$) ; la porte d'égalité est celle de S7.

**R12.** Un refus écrit sur une sortie standard utilisable mais dont la vidange échoue (avant toute publication) est
perdu sans repli sur la sortie d'erreur (`refuse`, `cli/mhgp11.cpp:252-259`, ne contrôle pas `fflush`). Le contrat ne
l'exige pas ; un repli coûterait deux lignes.

**R13.** Un tube **déjà** sans lecteur au lancement est jugé inscriptible à l'étape 2 (seul `O_RDONLY` y est refusé) :
le calcul entier est fait, publié, puis retiré à l'étape 9 (cas `tube sans lecteur` de la porte, étape `report`).
C'est conforme au § 3 ; un `poll` de la sortie standard (`POLLERR` sur un tube sans lecteur) le refuserait dès
l'étape 2, sans calcul. Facultatif.

## 6. Intégration avec L0 et S6

- **Table des modules : résolue par L0.** `origin/main` a adopté la ligne `api` explicite de S5 (`core num sched cloud
  io index catalogue tower`, `ARCHITECTURE.md` et `cmake/modules.cmake`) et ajouté `supports` à
  `MHGP11_ALL_MODULES`. À l'intégration, les deux côtés portent la même ligne `api` : conflit textuel trivial. (Mon
  constat du 4 octobre sur le brouillon L0, qui citait `supports points head`, est caduc.)
- **`docs/PROVENANCE.md`** : L0 ajoute en fin de fichier « Sortie paramétrée : ports annoncés », S5 « Façade api et
  exécutable mhgp11 » au même endroit ; garder les deux sections. Les épingles concordent (`full_probe.cpp`
  `2d3a37cc…`) ; la ligne « annoncée » de `src/api/write_full.cpp` est remplacée par la ligne livrée de S5.
- **Ordre des raisons** : voir L5. Conflit textuel certain dans `src/core/reasons.def` et `tests/core/status_test.cpp`
  quand S6 arrive.
- **À écrire dans `SORTIES.md` à l'intégration de S5** (le contrat dit « forme exacte fixée en S5 »), en plus des
  points du compte rendu (`impl_s5.md` § 5, que j'approuve) : forme exacte de la ligne de refus (clés, `stage`,
  `output` `null` à l'étape des options, `publication` et `manifest_sha256` de L2) et de la ligne de succès (`total`,
  six pics, `counts` de `full`, `manifest_sha256`) ; `memory_budget` à l'étape 5 (L4) ; `SIGPIPE` et `SIGXFSZ` ignorés
  (C2) ; `counts` de `full` dans le manifeste (`sites`, `points`, `orders[k, births, nodes, edges, root]`), `origin`
  en tableau de trois chaînes, saut de ligne final, forme canonique `json.dumps` compacte ; `tree_k_sha256` version 2
  implémentée (L1) ; le chemin de la fin de session dans l'api (L2).

## 7. Mutants

**Campagnes `run_mutants.py --jobs 2`** (copie propre et construction Release u21 par mutant, témoin vert) :
- `api` (4 oct., 23 h 00 → 23 h 19) : `mutants_ok module=api mutants=9 tues=9 dont_signal=0 dont_delai=0
  dont_construction=0 plancher=9`, tous tués par le verdict `code` ; **rejouée le 5 octobre** (06 h 07 → 06 h 18) :
  même ligne, témoin vert, les neuf tués par le verdict `code` ; compte rendu `/tmp/v11-s5-verif/mutants_api.json`
  (manifeste `60dfe047…`, sources `a9fb2484…`, les mêmes que pour `cli`) ;
- `cli` (5 oct., 05 h 55 → 06 h 07) : `mutants_ok module=cli mutants=11 tues=11 dont_signal=0 dont_delai=0
  dont_construction=0 plancher=11`, témoin vert, les onze tués par le verdict `code` ; compte rendu
  `/tmp/v11-s5-verif/mutants_cli.json` (manifeste `43815b34…`, sources `a9fb2484…`).

**Mutants de cru, 4 octobre**, appliqués en place à une copie (construction incrémentale, 27 portes `api` et `cli` de
label `fast`, restauration vérifiée par empreinte ; témoin : 27/27) :

| Mutant | Endroit | Issue | Lecture |
| --- | --- | --- | --- |
| `tree_k_ordre_un` | `src/api/manifest.cpp:225` | **survit** | causal, C1 |
| `tree_k_constante` | `src/api/manifest.cpp:225` | **survit** | causal, C1 |
| `stdout_lstat` | `cli/mhgp11.cpp:181` | **survit** | causal, C3 |
| `stdout_lecture_seule_admise` | `cli/mhgp11.cpp:175` | **survit** | causal (ordre des refus), C3 |
| `k_sup_n_garde_omise` | `src/api/compute.cpp:63` | survit | équivalent, R1 |
| `parametres_moteur_par_defaut` | `src/api/compute.cpp:76` | survit | même objet ; contrat du moteur, C4 |
| `retrait_omis_fin_de_session` | `cli/mhgp11.cpp:308` | survit | chemin non provocable par le CLI ; L2 le rend jugeable |

**Mutants de cru, 5 octobre**, sur la copie `/tmp/v11-s5-verif/mutown` (`-DMHGP11_MODULES=api;cli`, construction
incrémentale, 8 portes `mhgp11_cli_*` de label `fast` ; témoin : 8/8 ; restauration vérifiée par empreinte, copie
identique aux sources du worktree en fin de série) :

| Mutant | Endroit | Issue | Lecture |
| --- | --- | --- | --- |
| `plan_avant_sortie_standard` | `cli/mhgp11.cpp:300-304` (plan du dossier avant les contrôles de la sortie standard) | **survit** | causal : ordre de l'étape 2 du § 3, L4 |
| `sortie_standard_entree_ids_seulement` | `cli/mhgp11.cpp:179` (seul `--ids=` comparé à la sortie standard) | **survit** | causal : la ligne irait dans le fichier des points, L4 |
| `ligne_refus_sortie_toujours_full` | `cli/mhgp11.cpp:253` (`output` jamais `null`) | **survit** | forme de la ligne de refus non jugée, L4 |
| `retrait_sans_publication` | `cli/mhgp11.cpp:308` (`committed()` retiré) | survit | message « retrait impossible » sur tout refus après le plan ; JSON et codes inchangés ; tué par L2 |
| `refus_toujours_sur_sortie_erreur` | `cli/mhgp11.cpp:253` | **tué** (`mhgp11_cli_contract`, `mhgp11_cli_full_identity` et jumelles) | témoin de sensibilité |

## 8. Ce qui est vérifié conforme

- **Écrivain `MHGP11FUL1`** : relu ligne à ligne contre `serialize`, `birth_sphere`, `word` et `integer` ; mêmes
  champs, même ordre, mêmes expressions `i128` des centres, mots du type du profil ; seul le transport change
  (`FileWriter`, paquets de 512 mots, première erreur gardée). Conforme à `SORTIES.md` § 5 (format inchangé).
- **Paramètres** : `catalogue_params` et `full_params` égalent ceux de `bench/points_export.cpp` et le décodage du
  masque 16 379 de `bench/full_probe.cpp` (`max_nodes` 0 et `ball_limit` `kNone` par défaut), confirmé par le même pic
  d'allocations (§ 2).
- **Session** : Pool, puis compte du budget, puis F5 (ordre de l'étape 3 du § 3) ; budget et Pool uniques ; compte
  partagé, donc un produit qui survivrait à la Session rend ses octets sans comportement indéfini ; `close()` rend
  `budget_not_released` (code 3).
- **Transaction** : plan avant toute ouverture de fichier par le processus (la sortie standard est jugée sans rien
  ouvrir), création de `D.pending` après le calcul, manifeste en dernier, fin de session **avant** la ligne d'état,
  `retract()` après un échec de la ligne standard (portes `/dev/full` et tube sans lecteur), course arbitrée par
  `renameat2(RENAME_NOREPLACE)`.
- **Manifeste** : clés dans l'ordre du § 8 de `SORTIES.md`, ni temps, ni fils, ni chemin ; forme canonique
  (`json.dumps` compact) ; identique à W1, W48, W256.
- **Signature version 2** : les champs dont elle a besoin existent dans le domaine et la forêt, et donnent les valeurs
  de l'auditeur (§ 2) ; enfants déjà triés par le moteur.
- **Portes** : codes exacts, planchers gravés (lignes exactes `refus54`, `attempts503 refusals7`, `runs30`, `runs12`,
  planchers des groupes unitaires égaux aux comptes), aucune ne repose sur `assert`, jumelles `-O` conformes.
- **Règles** : aucune exception ne traverse l'api (`noexcept` partout, `guarded` aux frontières), aucun flottant
  décisionnel (le seul flottant est l'auto-test), aucun crochet dans le produit (fautes réelles par `feenableexcept`
  et `LD_PRELOAD`), aucun octet de sortie existant changé, `docs/SORTIES.md`, `MATHEMATIQUES.md` et `README.md`
  intacts.

## 9. Pour la session G4

En plus de la liste du compte rendu (`impl_s5.md` § 6), que j'approuve :
- les portes et mutants ajoutés pour L1 à L4 et C1 à C3, une fois livrés, dont la campagne `mhgp11_mutants_io`
  (mutant `empreinte_manifeste_apres_publication`) ;
- la porte de faute du chemin (i) de L2 (synchronisation du parent puis retour arrière en échec), hors sanitizers ;
- `mhgp11_cli_full_identity_lidar_ng02_k5` (ni l'implémenteur ni moi ne l'avons jouée) ;
- TSan sur `mhgp11_cli_full_determinism` et `mhgp11_api_session_equivalence` (non joué ici) ;
- le coût des jumelles `_opt` des portes d'échelle et LiDAR (R7) à chiffrer avant de lancer `gcc_release`.

## 10. Ce que je n'affirme pas

- Aucune mesure de temps : les durées citées sont locales, sous charge, et ne qualifient rien.
- Ni TSan, ni K10, ni 16 000 ou 32 000 points, ni ng00 et ng02 n'ont été joués ici ; Clang n'a fait qu'une analyse
  syntaxique.
- La suite `fast` complète et ASan+UBSan n'ont pas été rejoués le 5 octobre : leurs résultats sont ceux du
  4 octobre, sur le même worktree (§ 0).
- Les valeurs version 2 du § 2 sont celles du modèle de l'auditeur appliqué à des champs que j'ai extraits ; seules
  les trois valeurs de l'auditeur sont des références indépendantes de mon extraction.
- Les sondes de 5 octobre vivent dans `/tmp/v11-s5-verif/probes/` (hors dépôt, perdues au prochain redémarrage).
- Worktree en fin de reprise (06 h 22 UTC) : 1 009 fichiers, empreintes **identiques** à l'instantané de 05 h 50 ;
  HEAD `f98aeed67` ; `git status --short` et `--ignored` : 12 entrées, inchangées ; aucun `__pycache__`.
- Aucun statut public n'en découle.
