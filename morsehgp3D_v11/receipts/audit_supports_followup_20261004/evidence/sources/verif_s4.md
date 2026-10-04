# Contre-lecture de la tranche S4 « io »

4 octobre 2026, 20 h 05 UTC (`date -u`). Contre-lecteur indépendant. Worktree lu : `build/v11-impl-s4` (diff non
commité sur `1bf4be68f`). **Rien n'a été modifié dans ce worktree** : `git status` montre toujours les 12 mêmes
entrées. Constructions dans `/tmp/v11-impl-s4-verif/`, avec au plus 3 cœurs. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## Verdict

**Aucun constat bloquant.** Le module est correct sur tout ce que j'ai pu rejouer :
- SHA-256 conforme ;
- lecture entière ou refus, avec admission avant allocation ;
- transaction de dossier avec `RENAME_NOREPLACE`, orphelin jamais retiré ;
- aucune exception, aucun `operator new` ;
- portes, mutants et style verts.

Il reste trois points à corriger : un plancher faux sous root, un manque du port (le retrait après `commit` quand la
sortie standard échoue), et le JSON déterministe, prévu dans S4 et non livré. Viennent ensuite quelques remarques.

## 1. Portes rejouées (résultats exacts)

| Contrôle | Résultat |
| --- | --- |
| Release u21, `-DMHGP11_MODULES=io`, `ctest -L fast -j 3` | **27/27** |
| `sha256_oracle.py` sous `python3 -S` | `io_sha256 comparaisons=1242`, `mhgp11_io_sha256_ok controles=1244`, code 0 |
| Le même sous `PYTHONOPTIMIZE=1 python3 -S` | `mhgp11_io_sha256_ok controles=1244` |
| `tools/check_style.py --root …/morsehgp3D_v11` | `style_ok fichiers=420` |
| `run_mutants.py --check` (io.json) | `manifeste_ok module=io mutants=20 plancher=20` |
| Campagne complète des mutants, `--jobs 3` | `mutants_ok module=io mutants=20 tues=20 dont_signal=0 dont_delai=0 dont_construction=0 plancher=20` (3 min 23 s) |
| Debug ASan+UBSan, io seul (`-fsanitize=address,undefined -fno-sanitize-recover=all`) | **27/27** |
| Clang 18 Release, io seul | **27/27**, aucun avertissement |
| Release u21, tous modules, `ctest -L fast -j 3` | **726 portes : 723 passées, 2 échecs, 1 sautée** |

Détail de la suite complète :
- Les 2 échecs sont `mhgp11_tower_full_paired_protocol` et `_opt`. Ils lèvent `FileNotFoundError` sur
  `receipts/qualification_performance_20261003/baseline_source_manifest.json`. Ce fichier est absent du checkout
  partiel mais présent dans `1bf4be68f` (vérifié par `git cat-file -e`). Ces échecs ne viennent pas de io.
- La porte sautée est `mhgp11_support_lidar_sentinel`, faute de `MHGP11_DATA_DIR`.

Sondes de banc :
- `mhgp11_tower_full_bench_io`, `_full_bench_semantic`, `_points_export_width` et les autres portes `*_probe` et
  `full_bench` passent (6 sur 6).
- Aucun fichier hors de `src/io` et `tests/io` n'inclut `io/io.hpp`. La tranche ne touche donc à aucune sonde, et
  seul `docs/PROVENANCE.md` est modifié hors des nouveaux fichiers.

Épingles de `PROVENANCE.md`, toutes recalculées et **exactes** :
- R2 au commit `865f5e64…` : `u32le_input.hpp` `0ebd33a1…` et `cli_output.hpp` `ff7d96a5…`, sans modification
  locale ;
- `canonical_id.cpp` : `07f23c84…` ;
- `whole_input.hpp` : `95ee29ce…`, dernier changement en `80e77544e`.

L'erratum des lignes 18-45 et 49-221 de `canonical_id.cpp` est juste : `finalize` se termine à la l. 221.

`tools/check_docs.py` ne signale rien sur `PROVENANCE.md`. Ses 156 lignes de sortie sont des liens morts du checkout
partiel.

## 2. À corriger

### C1. `parent_unwritable` échoue sous root : plancher 3, mais 2 contrôles

`morsehgp3D_v11/tests/io/plan_test.cpp:144-157`.
- La branche root ne joue que `REQUIRE(s.ok())` et un `CHECK_EQ`, soit 2 contrôles pour un plancher de 3.
- Je l'ai vérifié par `sudo -n ./bio/mhgp11_io_transaction parent_unwritable` :
  `test parent_unwritable controles=2 echecs=0 plancher=3`, puis `PLANCHER non atteint`, **code 3**.
- Les autres exécutables io passent sous root (`mhgp11_io_unit`, `mhgp11_io_fault` : code 0).
- Sur G4, le worker est lancé par `gcloud compute ssh` sous OS Login, donc par un utilisateur ordinaire, et le défaut
  n'y apparaît pas. Il apparaîtrait dans un conteneur root.
- Correction : ajouter un contrôle à la branche root (par exemple `CHECK(r.ok())` avant le `create`), ou séparer les
  deux branches avec chacune son plancher. La réponse à la question 3 de l'implémenteur est donc : non, G4 ne tourne
  pas en root, mais la porte doit tenir dans les deux cas.

### C2. Port incomplet de `OutputSet` : pas de retrait après `commit`, donc la sortie standard en échec reste sans parade

Endroits : `docs/PROVENANCE.md:205`, colonne « Adaptations », qui dit « ni `rollback`/`release`/`finish` (rien
n'est jamais remplacé) », et `src/io/io.hpp:135-189`.

Dans R2, `rollback()` ne servait pas qu'à restaurer des originaux remplacés :
- `finish()` (`cli_output.hpp:430-445` du raccord) vide et contrôle la sortie standard **après** `commit()` ;
- si elle échoue alors que le code était 0 (par exemple `> /dev/full`), il défait la publication et rend 2 ;
- c'était la réparation de la frontière « jamais une ligne ok ni un code 0 avant que les sorties soient sûres ».

En v11, le § 5 de la spécification exige à la fois :
- une ligne JSON sur la sortie standard, écrite après la publication ;
- « Aucun refus ne publie de dossier ».

Si `fflush(stdout)` échoue après `commit`, le CLI de S5 n'a que deux mauvais choix :
- rendre 0 malgré une sortie standard perdue, ce qui contredit la frontière R2 ;
- rendre 2 en laissant `D` publié, ce qui contredit le § 5.

Correction proposée, peu coûteuse, puisque `publish()` contient déjà le chemin inverse
(`directory.cpp:216-223`) :
- exposer `Outcome OutputDirectory::retract() noexcept`, valable seulement après un commit réussi. Elle renommerait
  `D` en `D.pending` par `renameat2(RENAME_NOREPLACE)` sur le `parent_fd_` tenu, puis laisserait le destructeur
  retirer ce que l'objet a créé ;
- ajouter une porte (commit, retract, ni `D` ni `D.pending`) et un mutant.

À défaut, il faut arbitrer explicitement dans la spécification ce que fait le CLI quand la sortie standard échoue, et
corriger la phrase de `PROVENANCE.md`, qui donne une raison inexacte à l'abandon de `rollback`/`finish`.

### C3. Le JSON déterministe est absent alors que S4 le prévoit

Les livrables de S4 l'incluent explicitement :
- `slices_finales.json`, tranche `S4-io` : `files` contient `src/io/json.cpp`, et le `goal` dit « … JSON
  déterministe » ;
- § 3.1 de la spécification : rôle d'`io`.

L'implémenteur le renvoie à `api` (S5, `manifest.cpp`). Ce choix se défend, mais il change le contenu d'une tranche
dont S5 dépend. Je suis d'accord avec le renvoi, pour deux raisons :
- un seul document JSON existe, le manifeste ;
- S5 possède déjà `manifest.cpp`.

Il faut pourtant l'**inscrire** : mettre à jour `slices_finales.json` ou la note de passation de S5. Sinon,
l'implémenteur de S5 cherchera `io::json`. Les compteurs bornés et le formatage des entiers sans `printf` de locale
devront alors être portés par S5.

## 3. Remarques (sans correction exigée)

**R1. Le retour arrière quand le `fsync` du parent échoue** (`directory.cpp:216-223`).
- Le renommage a déjà rendu `D` visible. Le défaire fait **disparaître** un dossier qu'un lecteur a pu voir et
  commencer à lire. Cela affaiblit le « tout ou rien » que le code revendique ailleurs.
- L'autre choix serait de garder `D`, qui est complet et dont les fichiers sont synchronisés : seule la durabilité de
  l'entrée du parent est incertaine. On rendrait alors `ok`, ou un refus qui laisse `D`. Le § 5 interdit ce second
  cas, ce qui justifie le choix fait.
- Ce choix reste défendable. C3 et C2 se rejoignent : la même primitive `retract()` servirait aux deux.
- Ce chemin n'est pas couvert, et l'écart 8 de l'implémenteur le dit.
- Si le renommage de retour échoue lui aussi, `committed()` rend vrai, `commit` rend `output_unwritable` et
  `manifest_sha256()` reste à zéro. Ce triple état doit être documenté pour S5.

**R2. Le contrôle des entrées par chemin résolu est du code qu'aucun mutant ne peut tuer** (`directory.cpp:83-92`).
- Je confirme l'analyse : une entrée qui se résout sous `D` implique que `D` existe, car `realpath` échoue sur un
  composant absent.
- La porte `plan_inputs` décrit à tort son cas « lien physique » (`plan_test.cpp:104-107`). `ids.u32le` se résout
  dans la racine, pas sous `H`. Le refus vient donc de l'existence de `H`, pas d'une identité d'inode.
- Ce n'est pas faux, puisque le commentaire de `directory.cpp:19-21` l'assume. Le commentaire de la porte devrait
  pourtant dire « refusé par l'existence de H ».
- J'ai aussi cherché si l'inode gardait un objet par la sortie standard. Dans R2, `add_stdout()` empêchait une
  **sortie** de remplacer le fichier de la sortie standard. En v11, aucune sortie ne remplace un fichier existant,
  donc ce cas n'a plus d'objet.
- Un autre cas reste hors de `io` : `mhgp11 … >> ids.u32le` ajouterait la ligne JSON à une entrée. R2 ne le gardait
  pas non plus. On peut éventuellement le traiter dans `cli` (S5), en comparant l'inode de `fstat(1)` à celui des
  entrées.

**R3. Le déplacement d'`OutputDirectory` après un `create` n'est pas empêché** (`io.hpp:143`, `:155`).
- Les `FileWriter*` rendus pointeraient alors vers l'objet d'origine. Après sa destruction, ce serait un pointeur
  pendant, donc un comportement indéfini.
- Le contrat est documenté, mais rien ne le fait respecter. Deux options peu chères : une vérification qui rend le
  déplacement inopérant (`count_ != 0`, puis terminaison), ou une porte à arrêt anormal.

**R4. La mémoire de stdio n'est pas dans le budget** (`writer.cpp:63-72`).
- `fdopen` et `setvbuf(nullptr, _IOFBF, 64 Kio)` allouent par `malloc`, jusqu'à 9 × 64 Kio, soit environ 0,6 Mo
  hors `MemoryBudget`.
- La phrase « aucune allocation » du compte rendu vaut pour `operator new` seulement.
- C'est négligeable, mais l'affirmation doit être exacte. On pourrait aussi passer un tampon fixe membre à `setvbuf`.

**R5. Des refus d'appelant classés en refus d'utilisateur** (`directory.cpp:101-108`, `:183-185`).
- Un nom de fichier invalide, un nom en double ou plus de 8 fichiers rendent `parameter_out_of_range` ou
  `output_conflict`, donc le code 2.
- Or ces noms sont des constantes d'`api` : un tel refus serait une faute de programme, pas une faute de
  l'utilisateur.
- C'est acceptable dans `io`, mais `api` devrait traduire ces refus en invariant (code 3) plutôt que de les laisser
  passer pour des refus d'entrée.

**R6. Les portes dépendent de l'environnement.**
- `input_unreadable` exige un des trois fichiers sysfs candidats (`input_test.cpp:179-191`, `REQUIRE`). Il échoue
  dans un conteneur sans `/sys`. Sur G4, les fichiers sont présents.
- `input_sizes` crée deux fichiers creux de 48 Gio et 16 Gio sous `$TMPDIR` (`input_test.cpp:146-152`). Il faut
  pour cela un système de fichiers creux, sans `RLIMIT_FSIZE` posé par l'appelant. Le worker G4 ne pose que
  `ulimit -c 0`, donc c'est sans effet là-bas. À noter pour une configuration future.

**R7. `RLIMIT_FSIZE` à la place de `/dev/full`** (question 2 de l'implémenteur).
- J'approuve : dans une transaction de dossier, aucun fichier ne peut être un périphérique.
- `EFBIG`, avec `SIGXFSZ` ignoré, exerce exactement les mêmes `fflush`, `ferror` et `fclose` contrôlés, aux trois
  moments utiles. Root y est soumis aussi, et la limite ne concerne que le processus de la porte.
- Le mutant `manifeste_avant_donnees` est tué par la bonne observation : pas de manifeste dans `D.pending` après un
  échec.

**R8. Taille incohérente → `input_unreadable` plutôt que `size_mismatch`, tube ou périphérique refusés.**
- D'accord : la catégorie et le code de sortie sont les mêmes (`invalid_input`, code 2).
- Le refus du flux découle de la règle d'admission avant allocation (`ARCHITECTURE.md` § 7.1). Le § 5 dit d'ailleurs
  « fichiers réguliers ».

**R9. Les noms de fichiers diffèrent de la liste de S4.**
- Prévus : `output_dir.cpp`, `io_test.cpp`, `io_fault.cpp`, `io_probe.cpp`, `sha256_gate.py`, `transaction_gate.py`.
- Livrés : `directory.cpp`, `sha256_test.cpp`, `input_test.cpp`, `plan_test.cpp`, `transaction_test.cpp`,
  `fault.cpp`, `sha256_probe.cpp`, `sha256_oracle.py`. La porte de transaction est en C++, sans script Python.
- C'est sans conséquence, mais le tableau des écarts du compte rendu ne le mentionne pas.

**R10. `fsync` et la cible de 100 ms.**
- La spécification impose quatre niveaux de `fsync` : chaque fichier, le manifeste, `D.pending` et le parent.
- Sur le disque persistant de G4, chacun peut coûter quelques millisecondes. Il faut le mesurer dans l'étage
  `write` de la ligne JSON (S5) et ne pas le confondre avec le calcul.

## 4. Points vérifiés et conformes

- **SHA-256** : la rotation des noms dans `round_step` est juste. Le remplissage est correct aux frontières 55, 56,
  63, 64 et 65. Le différentiel contre `hashlib` passe sur 207 longueurs × 6 découpages, jusqu'à 1 Mo.
- **Lecture** :
  - le fichier est ouvert, puis `fstat`, sans allocation ;
  - les tailles sont contrôlées avant `admit(16 n)`, puis les quatre `Buffer` sont alloués ;
  - la lecture se fait par blocs de 4 096 points, avec le contrôle de fin `expect_end` ;
  - `/proc` (taille annoncée 0, contenu non vide) et sysfs (taille annoncée 4 096, contenu plus court) sont refusés ;
  - le tube est refusé sans blocage grâce à `O_NONBLOCK` ;
  - aucun refus ne laisse de réservation (pic nul pour les refus avant allocation).
- **Transaction** :
  - `plan` ne crée rien et prend son descripteur de parent avant tout ;
  - les `*at` passent par ce descripteur, ce qui résiste au déplacement du parent ;
  - `D` ou `D.pending` présent au sens de `lstat`, lien pendant compris, donne `output_conflict` ;
  - `mkdirat` est paresseux, avec `EEXIST` → `output_conflict` sans retirer le dossier étranger ;
  - les fichiers sont créés par `O_EXCL|O_NOFOLLOW` ;
  - l'ordre est : fichiers fermés et synchronisés, puis le manifeste, puis `fsync` de `D.pending`, puis `renameat2`
    `NOREPLACE` (`EEXIST`/`ENOTEMPTY` → conflit, autre erreur → `output_unwritable`, jamais `rename`), puis `fsync`
    du parent ;
  - le destructeur ne retire que ce que l'objet a créé.
- **Course entre deux appels sur le même `D`** : l'un des deux gagne au `mkdirat` ou au `renameat2`, et l'autre est
  refusé sans rien écraser.
- **Raisons** : `parameter_out_of_range`, `input_unreadable`, `output_unwritable` et `output_conflict` sont émises
  dans `src/io/` et provoquées dans `tests/io/`. `full_march_collector.py` n'est pas réenregistré.
- **Mutants** : les six mutants exigés par le § 8.5 sont présents et tués, et chaque motif cherché est unique.

## 5. Réponses aux questions de l'implémenteur

1. **JSON** : dans `api`, d'accord, mais il faut l'inscrire au plan de S5 (C3).
2. **`RLIMIT_FSIZE`** : oui (R7).
3. **Root sur G4** : non (OS Login). La porte doit quand même tenir sous root (C1).
