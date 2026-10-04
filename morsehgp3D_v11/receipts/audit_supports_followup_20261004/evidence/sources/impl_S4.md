# Tranche S4 « io » — compte rendu d'implémentation

4 octobre 2026, 19 h 48 UTC (`date -u`). Worktree `build/v11-impl-s4` (checkout partiel de `origin/main` à
`1bf4be68f`). Rien n'est commité ni poussé. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (construction locale par défaut ; u18 et u24 non rejoués)
public_status=not_claimed
```

## 1. Ce qui est livré

Module `io` (§ 3.4, § 4, § 6.7 de la spécification), dépendances `core` et `cloud` seulement (`cloud` sert à
`check_cloud_sizes`, seule autorité de la borne `kNone`).

| Fichier | Rôle |
| --- | --- |
| `morsehgp3D_v11/src/io/io.hpp` | En-tête public : `Digest`, `Sha256`, `to_hex`, `kMaxMessageBytes`, `InputFiles`, `check_u32le_sizes`, `read_u32le`, `FileWriter`, `OutputDirectory`, `kMaxOutputFiles` (8), `kManifestName` (`manifeste.json`), `kMaxFileName` (64). |
| `morsehgp3D_v11/src/io/sha256.cpp` | Port du SHA-256 de `morsehgp3d/src/cpu/contract/canonical_id.cpp`. |
| `morsehgp3D_v11/src/io/input.cpp` | Port de `u32le_input.hpp` (R2) et du second fichier de `bench/whole_input.hpp`. |
| `morsehgp3D_v11/src/io/writer.cpp` | `FileWriter`, port de l'étape 3 d'`OutputSet` (R2). |
| `morsehgp3D_v11/src/io/directory.cpp` | `OutputDirectory`, port des étapes 1, 4 et 6 d'`OutputSet`, adaptées à la transaction de dossier. |
| `morsehgp3D_v11/src/io/module.cmake` | Sources du module. |
| `morsehgp3D_v11/tests/io/tests.cmake` | Portes (ne réenregistre pas `full_march_collector.py`). |
| `morsehgp3D_v11/tests/io/io_support.hpp` | Aides de test : dossier jetable, lecture et inventaire de fichiers. |
| `morsehgp3D_v11/tests/io/sha256_test.cpp`, `input_test.cpp` | Exécutable `mhgp11_io_unit`. |
| `morsehgp3D_v11/tests/io/plan_test.cpp`, `transaction_test.cpp` | Exécutable `mhgp11_io_transaction`. |
| `morsehgp3D_v11/tests/io/fault.cpp` | `mhgp11_io_fault` (`operator new` remplacé). |
| `morsehgp3D_v11/tests/io/sha256_probe.cpp`, `sha256_oracle.py` | Différentiel contre `hashlib`, Python nu. |
| `morsehgp3D_v11/tests/mutants/io.json` | 20 mutants, plancher 20. |
| `morsehgp3D_v11/docs/PROVENANCE.md` | Section « Module io » : une ligne par fichier porté, avec source, sha256, adaptations, portes et mutants. La phrase d'annonce de la l. 42 est mise à jour. |

Sources épinglées :
- `build/v10-integration-r2/src/morsehgp3D_v10/src/cloud/u32le_input.hpp`, au commit `865f5e6`, sha256
  `0ebd33a164ab0eeb5a7ba4b9c1b059b7338c95b93798d586006a875b376ffda3` ;
- `.../src/core/cli_output.hpp`, sha256 `ff7d96a52f5c60ceb77e78d644c80b4338e0ffb37c2cc3444eef092bb2f972c7` ;
- `morsehgp3d/src/cpu/contract/canonical_id.cpp`, sha256
  `07f23c844b38534f5cf7bd69d7b5bc4a74aa878707b0d5ad61874b012785567a` (`main` `1bf4be68f`, inchangé depuis `375a46590`) ;
- `bench/whole_input.hpp`, sha256 `95ee29ceda8d47d18c92fda9b83409e3da80ddd5dfc8ae0c2137ad532cd963c2`.

### Contrat réalisé

**`read_u32le(points, ids, budget)`.**
- `open(O_RDONLY|O_CLOEXEC|O_NONBLOCK)`, puis `fstat`, sans aucune allocation. Le fichier doit être régulier.
- `check_u32le_sizes` contrôle les tailles (`input_unreadable`, puis `index_overflow_u32`).
- `admit(16 n)`, puis quatre `Buffer`.
- Lecture par blocs de 4 096 points sur la pile, décodage petit-boutiste par décalages et SHA-256 au fil de la lecture.
- Lecture incomplète : `input_unreadable`. Un octet de plus après la taille annoncée : `input_unreadable` aussi.
- Deux fichiers vides sont admis ; le nuage vide est ensuite refusé par `prepare_cloud` (`empty_input`), comme dans
  R2 et comme l'ordre du § 5.

**`OutputDirectory::plan(D, entrées)`.** Rien n'est créé. Refus, dans cet ordre :
1. syntaxe du chemin → `parameter_out_of_range` ;
2. parent absent ou non dossier → `output_unwritable` ;
3. entrée résolue sous `D` ou `D.pending` → `output_conflict` ;
4. `D` ou `D.pending` présent au sens de `lstat`, lien pendant compris → `output_conflict` ;
5. parent non inscriptible → `output_unwritable`.

**`create(nom)`.**
- Nom `[a-z0-9_.]+`, au plus 64 octets, sans point initial, différent du manifeste, et au plus 8 fichiers ; sinon
  `parameter_out_of_range`, sans rien créer.
- Nom en double : `output_conflict`. Ce refus ne bloque pas la publication.
- `mkdirat(D.pending)` paresseux ; `EEXIST` donne `output_conflict`.
- Fichier ouvert en `openat(O_EXCL|O_NOFOLLOW)`, puis `fdopen` et `setvbuf` à 64 Kio.
- Toute erreur d'entrée-sortie est définitive.

**`commit(manifeste)`.**
1. Chaque fichier : `fflush`, `ferror`, `fsync` et `fclose`, tous contrôlés.
2. Puis `manifeste.json`, écrit **après** les fichiers, avec les mêmes contrôles.
3. `fsync(D.pending)`.
4. `syscall(SYS_renameat2, …, RENAME_NOREPLACE)`. `EEXIST` ou `ENOTEMPTY` donne `output_conflict` ; toute autre
   erreur, `EINVAL` et `ENOSYS` compris, donne `output_unwritable`. Il n'y a jamais de `rename`.
5. `fsync` du parent. S'il échoue, on tente de défaire en renommant `D` en `D.pending` sans remplacement, et on rend
   `output_unwritable`.

**Destructeur sans commit.** Il retire les fichiers que l'objet a créés, puis `D.pending` **seulement si l'objet l'a
créé**. Il ne touche jamais à `D`, ni à un `D.pending` étranger.

## 2. Portes jouées (résultats exacts)

Construction locale sous `/tmp/v11-impl-s4/`, avec au plus 3 cœurs.

**1. Release u21, module io seul** (`-DMHGP11_MODULES=io`, `/tmp/v11-impl-s4/bio`).
- `ctest -R "mhgp11_io|mhgp11_style"` : **25/25** passées.
- Comptes de contrôles gravés en planchers :

  | Groupe | Contrôles |
  | --- | ---: |
  | `sha256` | 27 |
  | `hex` | 3 |
  | `input` | 32 |
  | `input_sizes` | 31 |
  | `input_unreadable` | 25 |
  | `input_budget` | 9 |
  | `plan_syntax` | 17 |
  | `plan_parent` | 5 |
  | `plan_conflicts` | 16 |
  | `plan_inputs` | 17 |
  | `create_names` | 28 |
  | `parent_unwritable` | 3 |
  | `commit` | 27 |
  | `noreplace` | 17 |
  | `orphan` | 6 |
  | `discard` | 7 |
  | `write_failure` | 19 |
  | `starvation` | 15 |

- `mhgp11_io_sha256` : `io_sha256 comparaisons=1242`, `mhgp11_io_sha256_ok controles=1244`. Le résultat est le même
  sous `python3 -S` et sous `PYTHONOPTIMIZE=1 python3 -S`.

**2. Campagne de mutants complète** :
`run_mutants.py --manifest tests/mutants/io.json --jobs 3`
→ `mutants_ok module=io mutants=20 tues=20 dont_signal=0 dont_delai=0 dont_construction=0 plancher=20`.

| Unité ciblée | Mutants |
| --- | --- |
| SHA-256 | `sha_tronque`, `sha_longueur_en_octets` |
| Lecture | `lecture_tronquee_acceptee`, `fin_non_controlee`, `type_non_regulier_admis`, `controle_apres_allocation`, `admission_omise`, `lecture_gros_boutiste` |
| Plan et nommage | `conflit_ignore`, `orphelin_ignore`, `nom_reserve_admis`, `nom_double_admis` |
| Publication et abandon | `renommage_ecrasant`, `pending_non_retire`, `publication_defaite_au_destructeur`, `manifeste_avant_donnees` |
| Écriture | `ecriture_non_controlee`, `empreinte_ecriture_omise`, `bourrage_toujours`, `mots_gros_boutistes` |

Les six mutants exigés par le § 8.5 y figurent tous.

**3. Manifeste** : `run_mutants.py --check` → `manifeste_ok module=io mutants=20 plancher=20`. Il a été rejoué après
la dernière modification.

**4. Style** : `python3 tools/check_style.py --root <worktree>/morsehgp3D_v11` → `style_ok fichiers=420`. La règle
s'applique sur tout l'arbre, y compris `[raison_morte]` et `[raison_sans_porte]` pour les quatre raisons d'io.

**5. ASan + UBSan, Debug, module io seul** (`/tmp/v11-impl-s4/basan`) : `ctest -L fast` → **27/27** passées.

**6. Suite rapide complète, Release u21, tous modules** (`/tmp/v11-impl-s4/b21`) : `ctest -L fast -j 3` → 726
portes.
- Les 25 portes io, dont `mhgp11_mutants_io_manifest` et sa jumelle, passent.
- 2 échecs, **antérieurs et étrangers à io** : `mhgp11_tower_full_paired_protocol` et `_opt`. Ils échouent par
  `FileNotFoundError` sur `morsehgp3D_v11/receipts/qualification_performance_20261003/baseline_source_manifest.json`,
  car le checkout partiel du worktree n'a pas `receipts/`. Le fichier existe bien dans `1bf4be68f`.
- `mhgp11_support_lidar_sentinel` est sauté, comme prévu (pas de `MHGP11_DATA_DIR`).

**7. `tools/check_docs.py`** (racine du worktree) : aucune ligne sur `morsehgp3D_v11/docs/PROVENANCE.md`. Les 155
écarts signalés sont des liens morts du checkout partiel (`tests/fixtures`, `third_party`, `receipts`).

Ces résultats sont **locaux**. Ce ne sont ni une session G4 ni un reçu (§ 8.1, § 8.8). La matrice G4 reste à jouer :
u18, u24, TSan, poison, et `-DMHGP11_MODULES=io` sous Clang.

## 3. Écarts à la spécification, et pourquoi

1. **`/dev/full` remplacé par `RLIMIT_FSIZE`** (§ 8.6).
   - Dans une transaction de dossier, aucun fichier de `D` ne peut être `/dev/full`.
   - La porte `write_failure` provoque donc le refus d'écriture par le noyau : `setrlimit` à 512 octets, `SIGXFSZ`
     ignoré, ce qui donne `EFBIG`.
   - Trois cas sont couverts : échec au vidage final, échec pendant l'écriture, échec sur le manifeste. C'est le même
     chemin de code (`fflush`, `ferror`, `fclose` contrôlés) que celui que `/dev/full` exerçait dans R2.
2. **Identité par inode des entrées** (§ 3.4, § 5 étape 2).
   - Dans R2, l'inode servait à comparer une sortie existante à une entrée.
   - Ici, aucune destination n'existe jamais, puisque `D` et `D.pending` sont refusés s'ils existent, lien pendant
     compris. Une entrée « sous `D` », par lien symbolique ou par lien physique, implique que `D` existe.
   - J'ai gardé le contrôle par chemin résolu, qui est la règle écrite du CLI, et la porte `plan_inputs` exerce les
     liens symboliques et physiques. Ce contrôle est **recouvert** par celui de l'existence, si bien qu'aucun mutant
     ne peut le tuer seul. Je n'en ai pas inscrit, et je le dis dans `directory.cpp` et dans `PROVENANCE.md`. Un
     contrôle d'inode n'aurait été que du code mort.
3. **JSON déterministe** : rôle d'io dans la table du § 3.1, absent de l'esquisse du § 4. Je ne l'ai **pas** écrit :
   le manifeste est produit par `api` (S5, `manifest.cpp`). io fournit `to_hex` et `FileWriter::digest`/`size`. Voir
   la question 1.
4. **Taille incohérente → `input_unreadable`**, comme `bench/whole_input.hpp` et la ligne « Lecture » du § 5.
   - R2 rendait `size_mismatch` (raison de `cloud`).
   - Les fichiers vides sont admis, et `empty_input` vient de `prepare_cloud`, comme R2 et le § 5 étape 5. `whole_input`
     les refusait par `input_unreadable`.
5. **Tube et périphérique refusés** (`input_unreadable`) au lieu d'être lus en flux comme dans R2. Sans taille annoncée,
   aucune admission avant allocation n'est possible, et `ARCHITECTURE.md` § 7.1 interdit l'agrandissement par
   doublement. Le § 5 dit d'ailleurs « fichiers réguliers ».
6. **Ajouts à l'esquisse du § 4**, tous petits et exercés par des portes :
   - `Sha256::finish()` est `const` : il calcule sur une copie et peut être rappelé, ce qui supprime l'exception
     « finalisé deux fois » de la source ;
   - `Sha256::update(std::string_view)` et `Sha256::bytes()` ;
   - `kMaxMessageBytes`, `to_hex` et `check_u32le_sizes`, cette dernière pure et testée sans fichier ;
   - `OutputDirectory::committed()` et `manifest_sha256()`, utiles à `publish` (S5) ;
   - `FileWriter::name()`.
7. **Renommage** : `syscall(SYS_renameat2, …, 1u)` avec une constante d'ABI. Ni la macro de la libc ni son
   enveloppe ne sont utilisées, ce qui évite de dépendre de `_GNU_SOURCE` et de glibc ≥ 2.28. Sans `SYS_renameat2` à
   la compilation, ou avec `EINVAL`/`ENOSYS` à l'exécution, le refus est explicite (`output_unwritable`). Ce chemin
   n'est **pas** exercé localement : ext4 et tmpfs supportent `RENAME_NOREPLACE`.
8. **Défaire après un `fsync` du parent en échec.** Ce code n'est pas couvert : aucune injection n'est possible sans
   crochet dans le produit, ce qu'interdit la règle 6. Si le renommage de retour échoue lui aussi, `D` reste publié,
   et complet, avec un refus `output_unwritable`. Cette limite est écrite dans `directory.cpp`.
9. **Erratum de citation** : le SHA-256 occupe les lignes 18-45 (constantes) et 49-221 de `canonical_id.cpp`, pas
   49-188 (§ 3.4). `finalize` va jusqu'à la l. 221.
10. **Déplacement d'un `OutputDirectory`** permis seulement avant le premier `create`, car les `FileWriter*` rendus
    pointent dans l'objet. Aucune allocation n'est faite : tous les chemins tiennent dans des tableaux fixes et
    `realpath` écrit dans un tampon. La porte de panne le vérifie : zéro `operator new` dans le plan, la création et
    le commit, et zéro `operator new` qui lève dans tout le module.
11. **Branche root de `parent_unwritable`** : non exercée ici (uid 1000). Sous root, les droits ne refusent rien ; la
    porte attend alors le refus au plus tard à la création de `D.pending` dans `/proc`. Il faut la vérifier sur G4 si
    la session tourne en root.
12. **`docs/ARCHITECTURE.md` § 7.2** dit encore que la visibilité atomique n'est pas promise. Je ne l'ai pas modifié :
    c'est l'amendement S0 (§ 6.7, dernière phrase). Le code livre la garantie plus forte, « tout ou rien » par un seul
    renommage.

## 4. Lecture critique (consigne de l'utilisateur : ses réponses ne sont pas intangibles)

Rien dans cette tranche ne va contre un choix de l'utilisateur. Deux points de la spécification méritaient d'être
discutés plutôt qu'appliqués à la lettre.

**(a) Le manifeste « écrit en dernier ».**
- Avec un renommage de dossier unique, aucun lecteur ne voit jamais un `D.pending` partiel : un orphelin est refusé,
  jamais lu. L'ordre n'est donc pas observable de l'extérieur après coup.
- Je l'ai rendu observable et causal autrement : un commit refusé ne laisse jamais de `manifeste.json` dans
  `D.pending` à côté de données en échec. La porte `write_failure` regarde le dossier avant la destruction, et c'est
  ce qui tue `manifeste_avant_donnees`.

**(b) Le contrôle par inode.** Voir l'écart 2 : la transaction de dossier le rend sans objet. L'écrire n'aurait
produit qu'un code mort, impossible à tuer par un mutant.

## 5. Questions

1. Le **JSON déterministe** (§ 3.1) doit-il vivre dans `io` (petit écrivain à capacité bornée, réutilisable par les
   quatre sorties) ou dans `api/manifest.cpp` (S5) ? Je penche pour `api`, tant qu'un seul document JSON existe.
2. Le remplacement de `/dev/full` par `RLIMIT_FSIZE` dans la porte `mhgp11_io_transaction_write_failure`
   convient-il ?
3. Les sessions G4 tournent-elles en root ? Si oui, la branche `/proc` de `parent_unwritable` sera la seule jouée.
