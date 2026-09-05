# Comparaison mono C/D du noyau MEB — préparation seulement

Révision 3 : `prepared_manifest_v3.json` est l'autorité de cette préparation du runner. Les révisions 1 et 2, leurs sources et leurs reçus sont conservés sans modification dans `revision1/` et `revision2/`. Leurs manifestes à la racine restent historiques et ne décrivent pas les sources courantes. La contre-fixture de revue a montré qu'un reçu v1 pouvait épingler un autre exécutable que la sortie de son build ; ce crosswire est désormais refusé, même si la copie étrangère a des octets identiques.

La révision 3 ajoute HEAD/porcelain avant et après, UTC au début et à la fin de chaque essai, et une allowlist matérielle (modèle CPU, CPU logiques, affinité, MemTotal, noyau, architecture, charge). `host_shared=true` reste explicite. Une évolution documentaire du worktree est rapportée, sans invalider à elle seule les sources épinglées ; une dérive de ces sources reste bloquante. `LD_PRELOAD`, `LD_AUDIT` et `LD_LIBRARY_PATH` non vides sont refusés avant processus, sans journaliser leurs valeurs ; leurs seuls états booléens font partie du snapshot. Aucun dump d'environnement.

`runner.py` ne lance aucun moteur et n'écrit aucun reçu **sans `--execute`**. Le futur D n'est pas créé par cette préparation. Ne pas ajouter `--execute` avant la revue et le GO explicite de root.

Plan fixe : C puis D, uniforme, n=8000, coord=65536, seed=3, s=8, tour candidate K1..10, CSR, digest activé, aucune archive. Un seul CPU logique (affinité CPU6), `threads=1`, `fold-inflight=1`, `fold-join=1`. Chaque processus est limité à 600 s et à 26 GiB d'espace virtuel via RLIMIT_AS ; ce dernier n'est pas un plafond physique RSS. Le proxy de payload nommé reste 16 GiB. Les cinq caps silent sont explicités, dont un milliard de supports MEB. Aucun GCP.

Le CLI C est `build/v7_c_qualification/mhgp7`, SHA-256 `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`. Ses sources sont celles du reçu historique `receipts/release_c_20260904` : jamais celles du futur worktree D. Le runner vérifie les pins du manifeste, du résumé, de l'inventaire binaire, des sources et de la liaison de build C. Pour D, la source est explicite (`--candidate-source-root`, défaut v7 courant) et un reçu de build validé est **obligatoire à l'exécution**.

Prévisualisation, seule commande destinée à cette phase :

```bash
python3 -B build/v7_meb_paired/runner.py --candidate /chemin/futur/mhgp7_D --candidate-source-root /chemin/sources_D --output /chemin/recu_neuf
```

Après GO uniquement, fournir aussi `--candidate-build-receipt /chemin/build_D.json --execute`. Le répertoire de résultat est create-only ; un résultat existant n'est jamais repris ni écrasé. Le runner reprend les implémentations épinglées de `parse_completion`, `classify`, `run_process`, `source_snapshot`, `atomic_json` et du digest chaîné des bancs existants. Il ajoute uniquement la vérification des flags mono, des caps et des chronos. Le groupe de processus possédé est drainé aussi sur censure/interruption ; un reçu d'essai est écrit avant chaque lancement. Une dérive de sources, helpers, binaires ou reçus de build interdit l'accord, y compris si elle est découverte après une sortie nominale.

Un accord exige deux observations complètement validées, puis l'égalité des dix digests chaînés, de toutes les cartes, des compteurs généraux, de tous les champs `silent_K2..10` (supports MEB, queries, chaînes, ajouts compris), des limites silent et des trois caps de payload. Les chronos et RSS ne participent pas à l'égalité. Refus mathématique et censure sont conservés séparément ; une paire incomplète n'a **ni champ equality, ni ratio, ni conclusion SLO**. Une divergence ou un reçu invalide ne donne pas de gain. Les coûts complets incluent le digest ; les temps d'étages, du silent, du fold, du CLI et du processus restent séparés. Deux runs froids ordonnés ne constituent ni un gain statistique ni une qualification 1 s/100 ms. Le format demeure `normalized_horizontal_h0_candidate`, `public_status=not_claimed`. L'absence de création de threads est une porte produit distincte, pas une mesure fournie par ce runner.

## Reçu de build D requis

JSON `schema=mhgp7-mono-meb-build-v1`, `status=completed`, avec les champs suivants. Ce reçu doit être produit par le build réel, pas rempli à partir du seul binaire après coup.

| Champ | Valeur et contrôle |
| --- | --- |
| `source_root` | Chemin absolu des sources D désignées au runner. |
| `sources_before`, `sources_after` | Deux objets identiques `{chemin_relatif: sha256}` couvrant `CMakeLists.txt` et tous les fichiers de `src/`, `cli/`, `oracle/`. Ils doivent également égaler `candidate_sources(source_root)` au lancement. |
| `binary` | Objet `path`, `sha256`, `bytes`, exactement le binaire D demandé ET le chemin résolu `build_dir/mhgp7`, exécutable. |
| `compile_database`, `cmake_cache` | Objets `path`, `sha256`, pointant `build_dir/compile_commands.json` et le cache du même build D. Le cache doit déclarer `CMAKE_BUILD_TYPE:STRING=Release`, `MHGP7_ENABLE_CUDA:BOOL=OFF` et `CMAKE_HOME_DIRECTORY:INTERNAL=source_root`. |
| `compile_command` | Entrée exacte du `compile_commands.json` épinglé pour `cli/mhgp7.cpp`. Champs CMake usuels `directory`, `file`, `command`, éventuellement `output`. |
| `build_command` | Tableau `['cmake', '--build', build_dir, '--parallel', '2', '--target', 'mhgp7']` ; parallélisme 1..8 accepté, aucun autre target. |
| `build_exit_code` | Entier 0, obtenu après le build réel. |

Le contrôle de compilation exige le compilateur direct `c++`, `g++` ou `clang++`, les flags uniques `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror`, `-c` vers le CLI et `-o` vers `CMakeFiles/mhgp7.dir/` dans le build. Toute option supplémentaire, macro TESTING/PROFILE, wrapper ou response-file est rejetée jusqu'à revue explicite. La commande doit appartenir à la base compilée épinglée. Le snapshot initial doit aussi correspondre exactement aux pins des liaisons C/D vérifiées : un changement dans l'intervalle n'est pas adopté comme nouvelle baseline. Les permissions exécutables et les empreintes sont revérifiées aux frontières des deux runs. Les helpers sont exécutés depuis les mêmes octets que ceux qui viennent d'être hashés, sans seconde lecture ni pycache. Cette liaison documentée aux frontières source/build n'est pas une attestation hermétique ; la limite est conservée dans le reçu.

## Selftests légers

`python3 -B build/v7_meb_paired/selftest.py` et la variante `-O` n'exécutent jamais MorseHGP. Les douze méthodes couvrent préparation sans effet, parsing nominal, mutations de mono/caps/archives, refus/censure, dérives source/helper/binaire, perte d'essai en interruption, create-only, liaison D obligatoire, crosswire hors cible, base/cache étrangers, permissions, flags instrumentés, correspondance du snapshot initial, lecture unique des helpers, allowlist matérielle et refus d'injection LD sans exposition de sa valeur. Une minuscule sonde locale vérifie le drainage d'un descendant ignorant TERM, sous timeout d'une seconde. Les fixtures synthétiques sont explicitement issues du test de banc existant ; elles ne sont pas des mesures n=8000.
