# Domaine G : vérification par exécution de l'état gelé de la v11 (local, sans GCP)

```text
phase=exploration_v11_hors_registre (close) ; backend=cpu_reference ; profile=quantized_u21_input_only (+ contrôle u18)
public_status=not_claimed ; GCP non utilisé (aucune commande gcloud)
```

Notations : `S=/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_G` (tout ce que
j'ai construit ou écrit), `D=/workspaces/E-HGP/build/v11-full-data-20261002` (données, lues seulement).

## 0. Verdict en dix lignes

1. **Construction u21 Release propre.** 206 s à `-j6`, 0 avertissement sous `-Werror`, 1 087 portes enregistrées.
   CUDA n'est pas détecté : `MHGP11_ENABLE_CUDA=OFF` par défaut et pas de `nvcc`.
2. **`ctest -LE long` tel que documenté.** 1 038 portes en 43 min 15 s : **976 réussies, 60 sautées** (lidar, sans
   `MHGP11_DATA_DIR`) et **2 échecs par délai** (`mhgp11_tower_full_campaign` et sa jumelle `_opt`, 120 s).
3. **Les deux délais viennent de l'environnement.** Rejouées seules, les deux portes passent en 77,9 s et 77,7 s. La
   cause est la charge de la machine (charge moyenne de 14 à 28 venant des autres agents).
4. **Les 60 portes lidar passent** avec `MHGP11_DATA_DIR=$D` : 60/60 en 28 min 22 s. Les **26 portes `diff_v10`**
   passent aussi (absentes par défaut) : 26/26 en 33 s, contre les binaires figés de la v10 reconstruits depuis
   l'archive épinglée. Au total, les 1 038 portes non longues du profil u21 sont conformes au HEAD.
5. **Les 9 empreintes de l'audit final, § 3.4, sont reproduites à l'octet** : trois trames à K5 et K10, trois uniformes
   à K5. Les trois K10 sont obtenues par la voie CPU (feuilles 24, puis feuilles 16 via le CLI pour ng00) et égalent les
   valeurs produites par la voie GPU sur G4.
6. **`mhgp11 --sortie=full` donne un fichier identique** au vidage de la sonde, pour ng00 (`cmp` à K5, SHA-256 à K10). Les
   sorties `supports`, `points` et `plat` rendent le code 0 et partagent le même `tree_k_sha256`. Leurs comptes égalent
   les lignes gravées des portes.
7. **« Uniforme u18 » désigne la donnée, pas le profil.** Les empreintes gravées s'obtiennent avec le build u21. Un
   build u18 donne d'autres octets, mais la **même empreinte sémantique** (lecteur strict du dépôt) sur l'uniforme 8 000
   et sur ng00 K5 : même objet, autre sérialisation.
8. **Python.** L'oracle de référence passe sous `python3 -S` et `-S -O` avec les lignes gravées exactes. L'analyse
   statique des 113 scripts lancés par les portes ne trouve numpy et sklearn que dans les deux différentiels S9/S10,
   de label `long`.
9. **`tools/check_docs.py` (racine) échoue au HEAD.** On compte 213 liens morts, tous dans `morsehgp3D_v10/receipts/`,
   aucun dans la v11. `check_polyhedron_order_k_counterexamples.py` passe (3 cas).
10. **Dette observée.**
    - Les portes lidar sont sautées en silence sans la variable d'environnement.
    - La donnée et l'archive v10 sont hors dépôt.
    - Une porte lit `receipts/`.
    - Aucune porte ne grave les empreintes FULL du § 3.4.
    - Le délai de 120 s est trop juste hors G4.
    - Les 49 portes `long` (dont 530 mutants) ne sont pas rejouées ici, comme demandé.

## 1. Environnement et état du dépôt

- **Dépôt.** HEAD `e968aba8d`. `git diff ac081a06f HEAD` est **vide** sur `src/`, `cli/`, `cmake/`, `CMakeLists.txt`,
  `tests/`, `reference/` et `bench/full_probe.cpp`. Seuls PASSATION, README, AUDIT_FINAL, NOTE_CLOTURE, DEVELOPPEMENT et
  des reçus ont changé : le moteur exécuté est donc exactement le moteur gelé `ac081a06f`.
- **Arbre de travail.** Celui de `morsehgp3D_v11` était propre au départ. Pendant ma session, d'autres acteurs ont
  modifié `PASSATION.md` et `README.md`, et créé `docs/AUDIT_GEANT_V11.md`, `receipts/conception_v11_20261002/` et
  `receipts/audit_geant_v11_20261007/` (notes et rapports d'autres lecteurs). Je n'y ai pas touché.
- **Aucune écriture de mes exécutions dans le dépôt ni dans `$D`.** Vérifié par `find -newer` sur des marqueurs posés
  avant chaque campagne : rien d'autre que les fichiers des autres acteurs. Pas de `__pycache__` nouveau.
- **Machine.**
  - AMD EPYC 7763, **4 cœurs × 2 SMT = 8 vCPU**, 31 Gio.
  - Charge externe forte et variable : charge moyenne de 3 à 29 pendant mes mesures, venant d'autres agents (dont un
    rendu ffmpeg et chromium).
- **Outils, contre ceux de G4.**
  - GCC 13.3.0 (G4 : GCC 11.4).
  - CMake 3.28.3 (G4 : 3.22).
  - Python trouvé par CMake : 3.12.1 avec site-packages, numpy présent (G4 : 3.10 nu).
  - Pas de `nvcc`.
- **Données.** `$D/manifest.json` recoupé : coordonnées et identifiants des 6 cas conformes à leurs SHA-256 (ng00
  39 885, ng01 35 551 et ng02 45 845 sites ; uniformes de 8 000, 16 000 et 32 000). Le manifeste étiquette les 6 cas
  `quantized_u18_input_only`.

## 2. Construction

```bash
cmake -S /workspaces/E-HGP/morsehgp3D_v11 -B $S/build-v11 -DCMAKE_BUILD_TYPE=Release     # 6,9 s
cmake --build $S/build-v11 -j6                                                        # 3 min 25,9 s ; user 14 min 21 s
```

**Configuration.** Elle écrit :
- `unites testees = core;num;sched;cloud;io;index;catalogue;tower;supports;points;head;api;cli;reference`, `bits = 21` ;
- `portes diff_v10 de la reference absentes (MHGP11_V10_FROZEN_DIR='' ...)` ;
- `1087 portes enregistrees`.

**Construction.** Code 0 et **0 avertissement** (`grep -ci warning` = 0).

**CUDA.** Non détecté et non tenté : l'option est OFF par défaut et la configuration ne cherche pas `nvcc`. La voie GPU
n'est donc jugée ici que par l'égalité de ses empreintes G4 avec la voie CPU locale (§ 4).

**Constructions annexes.**
- **u18, sonde et CLI seulement.**
  `cmake -S … -B $S/build-v11-u18 -DCMAKE_BUILD_TYPE=Release -DMHGP11_COORD_BITS=18 && cmake --build … -j2 --target mhgp11_full_bench mhgp11_cli`.
  Durée 2 min 9,6 s, 0 avertissement, 1 087 portes enregistrées.
- **v10 figée.**
  - Archive `$D/v10_frozen_c764.tar.gz` : son SHA-256 `79fa4c63…` égale `tests/tower/v10_frozen_manifest.json`, et les
    34 fichiers sur 34 sont conformes.
  - Extraction dans `$S/v10src`, puis
    `cmake -S $S/v10src/morsehgp3D_v10 -B $S/build-v10frozen -DCMAKE_BUILD_TYPE=Release` et
    `cmake --build … -j2 --target mhgp10_catalogue mhgp10_tower` : 28 s, 0 avertissement.
- **Référence seule.**
  `cmake -S … -B $S/build-v11-ref -DMHGP11_MODULES=reference -DMHGP11_V10_FROZEN_DIR=$S/build-v10frozen` : 135 portes,
  dont 26 `diff_v10`.

## 3. Portes CTest

### 3.1 Comptes par label (registre u21, 1 087 portes ; un test peut porter plusieurs labels)

| Label | Portes | | Label | Portes |
|---|---:|---|---|---:|
| unit | 552 | | lidar | 76 (60 non `long`) |
| oracle | 265 | | mutant | 39 (13 campagnes `long`, 26 contrôles de manifeste) |
| fast | 921 | | long | 49 |
| scale8000 | 28 | | diff_v10 | 0 par défaut (26 avec `MHGP11_V10_FROZEN_DIR`) |
| scale16000 | 16 | | `-LE long` | 1 038 |
| scale32000 | 18 | | ni fast ni long | 117 |

Interprète Python : 506 portes (463 non `long`). Les 49 portes `long`, non jouées :
- 13 campagnes de mutants ;
- `reference_full`, soit 16 tranches et la somme ;
- 4 `points_vs_python` et 4 `head_vs_python` ;
- 11 portes K10 ou d'échelle K10.

### 3.2 Suite demandée

```bash
ctest --test-dir $S/build-v11 -LE long --no-tests=error --output-on-failure -j4
```

**Résultat.** 43 min 15 s de mur, 81 min de CPU. Code CTest 8 :
« 99% tests passed, 2 tests failed out of 1038 ».
- **976 Passed** ;
- **60 Skipped** : toutes les portes lidar non `long`, par le précontrôle `REQUIRE_DIR_ENV`. Trois portes uniformes en
  font partie (`mhgp11_num_roots_cost_uniform_u18_n{8000,16000,32000}_k5`) : elles lisent les uniformes dans le même
  dossier ;
- **2 Timeout** :

```text
131/1038 Test  #556: mhgp11_tower_full_campaign .....................***Timeout 120.00 sec
782/1038 Test  #557: mhgp11_tower_full_campaign_opt .................***Timeout 120.01 sec
```

**Sortie des deux échecs.** Aucune : le script n'écrit que sa ligne de verdict finale, et il est tué avant.

**Temps par label.**

| Label | sec·proc | Portes |
|---|---:|---:|
| fast | 1 992 | 921 |
| oracle | 888 | 248 |
| unit | 292 | 552 |
| scale8000 | 555 | 28 |
| scale16000 | 497 | 16 |
| scale32000 | 1 033 | 17 |

Les portes d'échelle sont en `RUN_SERIAL` et font l'essentiel du mur : la suite « rapide » du README prend 43 min en
local.

### 3.3 Diagnostic des deux délais

La porte est un modèle Python pur, sans natif : `full_campaign_test.py`, « simulated children only ».

**Rejouée hors CTest** (charge 10 à 14) :

```bash
python3 tests/tower/full_campaign_test.py
```

Elle donne la ligne exacte en 130,4 s de mur et 121,5 s de CPU utilisateur :
`full_campaign_verdict conforme attempts483 schedules412 interrupted1 checks39532 native0`.

**Rejouée par CTest, seule** (charge environ 5) :

```bash
ctest -R '^mhgp11_tower_full_campaign(_opt)?$'
```

Elle donne **Passed 77,92 s et 77,69 s**.

**Conclusion : dépendance à l'environnement, pas une faute.** C'est connu et non corrigé :
- `audit_native_integration_20261005/…/impl_s3.md` notait 146 s et 119 s seules ;
- `verif_s5.md` notait 85 s et 79 s, « délai de 120 s sous charge 10 à 17 » ;
- G4 la joue en 37,5 à 65,7 s (`receipts/full_pair_graph_20261003/graph2_failure/matrix.json`).

Marge locale à vide : environ 1,5×.

### 3.4 Portes lidar (60, non `long`)

```bash
MHGP11_DATA_DIR=$D ctest --test-dir $S/build-v11 -L lidar -LE long --no-tests=error --output-on-failure -j4
```

**100 % (60/60)**, 28 min 22 s : sentinelle, `roots_cost`, Euler, `order_identity`, `attach_e1e2/export`, juge
d'échantillon, registres et hiérarchie des supports, `points`, `plat`, route API des supports,
`cli_full_identity_lidar_ng0{0,1,2}_k5` (95 à 124 s chacune) et `cli_supports_lidar`, avec leurs jumelles `-O`.
Aucune écriture dans `$D`.

### 3.5 Différentiel contre la v10 figée

```bash
ctest --test-dir $S/build-v11-ref -L diff_v10 --no-tests=error --output-on-failure -j2
```

**26/26**, 33,3 s. Cela couvre :
- `reference_diff_v10` : 190 nuages, 640 tours, 26 530 lignes ;
- `_large` ;
- le refus ;
- 10 mutants de sérialisation tués (`admission_single`, `cover_last_ball`, `lexicographic_sites`, `merge_numbering`,
  `q3_form_from_support`, `rank_by_double`, `rank_last_ball`, `reduced_levels`, `support_last`,
  `weighted_flag_lost`) ;
- les jumelles `-O`.

Les dumps de catalogue et de tour de l'oracle sont identiques à l'octet à ceux de `mhgp10_catalogue` et `mhgp10_tower`
(`c764e121a`).

**Bilan CTest u21.** Les **1 038 portes non `long` sont conformes au HEAD**, à deux conditions : fournir
`MHGP11_DATA_DIR` pour les 60 portes lidar, et rejouer seules les 2 portes de campagne. Les 26 portes `diff_v10`,
absentes du registre par défaut, sont conformes elles aussi.

## 4. Empreintes de référence (audit final, § 3.4)

**Commande de la sonde.** C'est celle de `bench/gpu_ab.py` et de `ecart_v10_v11/measure.py`, voie CPU sans CUDA :

```bash
$S/build-v11/mhgp11_full_bench $D/<cas>.u32le $D/<cas>.ids.u32le <dump> <K> <feuille> 256 0 4294967295 8589934592 8 <masque> [passes]
```

Pilote `$S/tools/run_probe.py` : il hache le vidage puis l'efface. Résultats bruts dans `$S/fp/*.jsonl`.

| Entrée | K | Feuille, masque | SHA-256 obtenu (u21) | Gravé | Octets |
|---|---|---|---|---|---:|
| ng00 | 5 | 16, 802811 | `3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe` | identique | 300 883 482 |
| ng01 | 5 | 16, 802811 | `5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091` | identique | 255 161 594 |
| ng02 | 5 | 16, 802811 | `78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207` | identique | 328 855 058 |
| ng00 | 10 | 24, 802811 | `61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295` | identique | 1 457 125 554 |
| ng01 | 10 | 24, 802811 | `838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de` | identique | 1 169 527 226 |
| ng02 | 10 | 24, 802811 | `81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e` | identique | 1 469 069 530 |
| uniforme 8 000 | 5 | 16, 16379 | `f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf` | identique | 124 242 274 |
| uniforme 16 000 | 5 | 16, 16379 | `141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889` | identique | 255 097 522 |
| uniforme 32 000 | 5 | 16, 16379 | `a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b` | identique | 522 995 146 |

**9/9 reproduites à l'octet. Confirmations supplémentaires** :
- même empreinte après 10 passes à chaud (K5) et 3 passes à chaud (K10), sur le vidage de la dernière passe ;
- l'uniforme 8 000 rejoué deux fois.

**K10.**
- Les valeurs gravées viennent de `claudeg1`, voie GPU `868347`. Ma voie CPU (`802811`, feuilles 24) donne les mêmes
  octets.
- Le CLI ng00 K10 (masque 278523, feuilles 16) aussi : invariance à la voie et à la taille de feuille.
- `records.json` (`ecart_v10_v11`) ne contient **aucun** K10. Les K10 apparaissent dans les reçus dès le 4 octobre
  (`audit_gpu6_receipt_20261004`, `audit_gpu_scratch_20261004`) et le 5 octobre (`claudefinmesure`).
- La phrase du § 3.4 « mêmes valeurs dans records.json … voies CPU et GPU » ne vaut donc que pour K5.

**Uniformes « u18 ».**
- Ce sont des données dont les coordonnées tiennent dans 18 bits. Le reçu `ecart_v10_v11` les a jouées au **profil
  u21** (README du reçu), et le vidage porte `coord_bits=21`.
- Au **profil u18** (`$S/build-v11-u18`), les octets diffèrent : en-tête `coord_bits=18` et mots plus étroits. Valeurs
  obtenues :
  - 8 000 : `627d7fb5…e8b3`, 119 173 746 o ;
  - 16 000 : `6d5b636d…aa5a`, 244 688 146 o ;
  - 32 000 : `e0e07362…2109`, 501 653 050 o ;
  - ng00 K5 : `6d84f953…1502`, 288 549 482 o.
- Aucune de ces valeurs n'est gravée dans le dépôt.
- **L'empreinte sémantique** de `bench/full_semantic.py` (décodage strict : CSR, ordre canonique, naturalité
  verticale ; normalise profil et membres) est **identique entre u18 et u21** :
  - uniforme 8 000 K5 : `a45a9c64ed8a39f5…bdb0` ;
  - ng00 K5 : `fcca94600524d0dc…d276`.
- Ces deux valeurs figurent déjà dans les reçus du 3 octobre (`full_parallel_20261003/forest3`, où u21 et u24
  donnaient la même empreinte sémantique). L'identité des profils 18, 21 et 24 est donc établie sur ng00 K5.

## 5. Exécutable `mhgp11` sur ng00 K5 (W8)

```bash
$S/build-v11/mhgp11 --sortie=<full|supports|points|plat> --points=$D/lidar_ng00.u32le --ids=$D/lidar_ng00.ids.u32le \
  --dossier=$S/cli/<s>_ng00_k5 --k=5 --fils=8
```

| Sortie | Code | Fichier, format | Octets | SHA-256 du fichier | SHA-256 du manifeste | Mur total | Étages (s) |
|---|---|---|---:|---|---|---:|---|
| full | 0 | `full.mhgp11ful1`, MHGP11FUL1 v1 | 300 883 482 | `3a2bfb4f…a6fe`, **= sonde (`cmp` identique)** | `aabd6491…` (= G4 `claudefinmesure`, 5 oct.) | 5,07 s | domain 2,08 ; tree 0,91 ; write 2,03 |
| supports | 0 | `supports.mhgp11sp`, MHGP11SP **v2** | 24 377 352 | `6a3f8372…` (= préfixe gravé `mhgp11_route_ng00_file` u21) | `c544ce72…` | 3,71 s | domain 2,03 ; tree 1,07 ; attach 0,14 ; output 0,15 ; write 0,26 |
| points | 0 | `points.mhgp11pt`, MHGP11PT v1 | 36 763 152 | `657f2142…` | `363ffdb5…` | 4,28 s | domain 2,33 ; tree 0,97 ; output 0,22 ; write 0,57 |
| plat | 0 | `etiquettes.mhgp11et`, MHGP11ET v1 (mcs 20, z 1, eom) | 319 136 | `a52eb685…` | `aefd87d6…` | 3,40 s | domain 2,08 ; tree 0,68 ; output 0,26 ; write 0,11 |

**Points communs aux quatre sorties.**
- `tree_k_sha256 = a4425cb5cedd255d…`, égal à la valeur G4 du 5 octobre. C'est un recoupement entre FULL (`supports`)
  et `build_order` (`points`, `plat`).
- Manifestes : `public_status: not_claimed`, `status: complete`.

**Comptes**, conformes aux lignes gravées :
- `supports` : nœuds 576 371, boules 576 388, supports 576 388, coquilles étendues 62, rôle fusion 235 307 ;
- `points` : retardés 34 509, plateaux 37 684 ;
- `plat` : 1 457 clusters, 332 retenus, 3 140 points de bruit. La porte grave `retenus=996`, soit 3 appels × 332.

**CLI full ng00 K10** (`--k=10`).
- Code 0 ; fichier `61a4245b…`, identique à la sonde et à G4 ; manifeste `41cf379c…` et `tree_k` `c46e51af…`,
  identiques à G4 du 5 octobre.
- 31,8 s, dont **write 10,4 s pour 1,46 Go** : localement, l'écriture pèse un tiers du temps du CLI.

**Le CLI ne joue aucun mode de référence**, comme le dit la passation.
- Masque `kEngineMask = 278523`, soit 16379 + placement 262144.
- Feuilles de 16 à 256, sans cache de blocs ni GPU (`src/api/internal.hpp`).
- L'en-tête de `src/api/compute.cpp` dit encore « masque 16379 » : commentaire périmé, sans effet.

## 6. Temps locaux indicatifs (W8, AMD EPYC 7763 4C/8T)

**Ces temps ne décident rien. Seule G4 juge les temps.** La charge externe a beaucoup varié : à froid, charge de 20 à
28 ; à chaud, de 4,5 à 10.

| Prise | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| K5 CPU 802811 f16, froid, mur FULL (domain / forêts) | 3 624 (2 658 / 964) | 2 752 (1 907 / 842) | 4 096 (2 248 / 1 846) ms |
| K5, chaud : médiane des passes 2 à 10 (min) | 2 442 (2 383) | 1 985 (1 860) | 2 318 (2 256) ms |
| K5, CPU par passe | 16,6 s | 13,0 s | 15,7 s |
| K5, pic du budget / RSS max | 364 Mo / 0,70 Go | 314 Mo / 0,60 Go | 387 Mo / 0,74 Go |
| K10 CPU 802811 f24, froid (domain / forêts) | 30 038 (12 234 / 17 803) | 20 378 (9 010 / 11 366) | 26 569 (11 742 / 14 825) ms |
| K10, chaud : moyenne des passes 2 et 3 | 15 960 | 12 139 | 13 879 ms |
| K10, CPU par passe ; pic budget / RSS | 117 s ; 2,02 / 3,45 Go | 89 s ; 1,59 / 2,73 Go | 101 s ; 2,01 / 3,43 Go |
| CLI full K5, total (processus) | 5,05 s (5,07 s) | — | — |

**Uniformes K5, mode 16379, à froid** (charge environ 25) : 1 381, 2 649 et 6 356 ms pour 8 000, 16 000 et 32 000
sites.

**Repère G4 W48** (`claudeg1`).
- K5 CPU 802811 : 343 / 272 / 329 ms à froid, 314 / 255 / 313 ms à chaud. Le chaud local est environ 7,5 fois plus
  lent.
- K10 GPU : 1 782 / 1 336 / 1 536 ms à chaud. Ce n'est pas la même voie.

**Effet de la charge.** Le même K10 ng00 passe de 30,0 s (charge 28) à 16,6 s (première passe, charge 9).

## 7. Python

**Oracle de référence sous `python3 -S` et `-S -O`** (dans `$S/pyS`). Toutes les lignes sont égales aux `LINE`
gravées :

| Script | Ligne obtenue | Durée |
|---|---|---|
| `test_projection_contracts.py` | `projection_contracts_ok faits=5` | 0,12 s |
| `test_ref.py --suite=fast` | `reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029`, faits 15, écarts 0 | 14,5 s ; 12,1 s sous `-O` |
| `test_supports.py` | `reference_supports_ok nuages=210 ordres=951 boules=15062 supports=16943 noeuds=12441 coupes=48074`, faits 51, écarts 0 | 18,0 s ; 20,1 s sous `-O` |
| `test_supports.py --suite=primitives` | `… sites=24 supports=828 … refus=2` | 2,6 s |
| `test_ref.py --suite=absente` | code 2 | — |

**Tests hors CTest, sous `-S` et `-S -O`.**
- `tests/support/full_capture_reader_test.py` : `"checks": 55, "status": "ok"`.
- `tools/g4_prepare_host_test.py` : `g4_host_tools_verdict conforme checks87 native0 cloud0`.

Les autres `.py` hors registre sont des modules importés par des portes, ou des outils G4 (`full_v10_diff.py`).
`center_region_model_test.py` est appelé par `center_region_oracle.py`.

**Python 3.10 nu (G4).** Python 3.10 n'existe pas ici, d'où deux substituts :
- **Grammaire 3.10** : `ast.parse(feature_version=(3,10))` accepte les 201 fichiers `.py` de `tests/`, `reference/`,
  `bench/` et `tools/`.
- **Imports** : `modulefinder` lancé sous un venv sans paquets (`$S/venv_nu`) sur les 113 scripts lancés par les
  portes, imports paresseux compris (`$S/imports_nus.log`).
  - Seuls modules tiers : **numpy** et **sklearn.cluster**, uniquement dans `tests/points/points_vs_python.py` et
    `tests/head/head_vs_python.py`, via `bench/points_hierarchy.py`, `points_flat.py` et `points_radius.py`. Ces deux
    portes sont `long` : ce sont les différentiels S9 et S10.
  - Faux positifs : `hgp11_ref.Definition` et `hgp11_ref.Reference`, qui sont des classes.
  - Aucune porte non `long` ne dépend d'un paquet tiers.

**Contrôles racine.**
- `python3 tools/check_docs.py` : **code 1**, 22 s. « Documentation validation failed » : **213 liens locaux morts dans
  13 fichiers, tous sous `morsehgp3D_v10/receipts/`**.
  - Répartition : 3 dans `audit_continu_20260929`, 6 dans `audit_independant_20260930`, 4 dans
    `audit_independant_20261002`.
  - Ces fichiers ont été versés du 29 septembre (`56020cab6`) au 2 octobre (`aa8f0807a`).
  - Aucune erreur dans la v11. La CI racine, qui joue ce contrôle, est donc rouge au HEAD pour une raison hors v11.
- `python3 tools/check_polyhedron_order_k_counterexamples.py` : `polyhedron order-k counterexamples: PASS (3 cases)`,
  code 0, 0,08 s.

## 8. Affirmations de PASSATION et AUDIT_FINAL, relues par exécution

**Confirmées.**
- Empreintes du § 3.4 : 9/9.
- « Empreintes K5 stables du 3 au 7 octobre » : `records.json`, `claudeg1` et le HEAD donnent les mêmes valeurs.
- « Voie GPU identique au CPU à l'octet » : les K10 GPU de G4 égalent le CPU local.
- Le CLI prend 278523, feuilles 16, sans cache ni GPU.
- 530 mutants dans 13 manifestes.
- 385 `MHGP11_TEST`.
- `src/` : 21 899 lignes.
- `tests/` : 25 893 lignes C++ et 22 837 lignes Python, soit 48 730.

**Références de portes des mutants.** 528 mutants nomment une porte :
- 525 portes sont dans le registre u21 ;
- 1 porte (`mhgp11_core_poison`) n'existe que sous `MHGP11_POISON=ON`, option portée par le mutant ;
- 2 mutants sont de construction, sans porte.

Aucune référence pendante au HEAD, même si `--check` ne le vérifie pas (audit, § 9.2).

**Imprécisions.**
- K10 absent de `records.json` (§ 3.4).
- « 124 scripts Python de porte » : je compte 113 scripts distincts lancés en tête (`ARG0`) dans le registre u21. La
  définition de l'audit m'est inconnue : non reproduit tel quel.

**Non vérifiables ici.**
- Mutants (485/485 le 5 octobre ; 530 déclarés).
- ASan, UBSan, TSan.
- Profils u18 et u24 complets.
- Les temps G4.

## 9. Dette de qualification observée au HEAD

1. **Portes lidar sautées en silence.**
   - La commande du README et de la passation (`-LE long`) rend 60 « Skipped » et un résumé qui ne compte aucun échec.
   - Les données vivent hors dépôt, dans `build/` (volatile), avec `restore.py` et `restoration.json` ; le manifeste
     cite des chemins de la v8.
   - La v12 doit nommer et vérifier l'approvisionnement, ou faire échouer l'absence dans un mode « qualification ».
2. **`diff_v10` absentes par défaut** (26 portes).
   - L'archive figée n'existe que dans `$D`, hors dépôt. Elle est épinglée par `tests/tower/v10_frozen_manifest.json`
     et passe 26/26 une fois reconstruite.
3. **Une porte lit `receipts/`.**
   - `mhgp11_tower_full_paired_protocol` et `_opt` lisent
     `receipts/qualification_performance_20261003/baseline_source_manifest.json`, via `bench/full_baseline_source.py`.
   - Elles passent ici, mais échouaient en checkout partiel (`impl_s3`). Les autres mentions de `receipts/` dans
     `tests/` sont des commentaires.
4. **Aucune porte ne grave les empreintes FULL du § 3.4.**
   - `cli_full_identity*` compare le CLI à la sonde du même build : c'est relatif.
   - Seule la sortie `supports` a des préfixes gravés par profil (`tests/api/tests.cmake`).
   - Le « premier jalon » proposé pour la v12 repose sur des valeurs qui ne vivent que dans des reçus.
5. **Délai de 120 s de `mhgp11_tower_full_campaign` trop juste hors G4.**
   - Échec reproductible sous `-j4` et charge. Le défaut est noté depuis le 4 octobre et n'a pas été corrigé.
6. **Portes `long` non rejouées au HEAD** (49, conforme à la passation, § 2.3).
   - Les 13 campagnes de mutants (530), la référence complète et les K10.
   - Les différentiels S9 et S10 exigent numpy et sklearn : ils ne peuvent pas passer sous le Python nu de G4 sans
     plan `python_packages: pinned`.
7. **CI racine rouge.** `check_docs` : 213 liens morts, côté v10.
8. **Écart d'outils entre le local et G4.** GCC 13 contre 11, CMake 3.28 contre 3.22, Python 3.12 avec numpy contre
   3.10 nu : un vert local n'est pas un vert G4.
9. **Commentaire périmé** dans `src/api/compute.cpp` (masque 16379 contre 278523).

## 10. Pièges d'environnement rencontrés

- **Charge des autres agents** (jusqu'à 29 sur 8 vCPU) : deux délais CTest, et des temps à froid presque doublés.
  J'ai diagnostiqué en rejouant seul, et je n'ai retiré aucune porte.
- **Le Python trouvé par CMake voit numpy.** Une régression « Python nu » serait invisible localement. J'ai utilisé
  `python3 -S`, un venv sans paquets et une analyse statique.
- **8 vCPU = 4 cœurs physiques.** W8 sature le SMT.
- **`/workspaces` plein** (2,3 Go libres). Les vidages K10 font 1,2 à 1,5 Go : tout a été construit et écrit sous
  `/tmp` (`$S` pèse 1,3 Go).
- **Pas de `nvcc`** : la voie GPU n'est pas constructible ici.
- **Libellés trompeurs.** « uniforme u18 » désigne la donnée, et le manifeste de `$D` dit
  `quantized_u18_input_only` pour des cas joués au profil u21.
- **Trois commandes gardées.** Je n'ai lancé ni `pkill -f`, ni TSan, ni GCP. Les vidages ont été effacés après hachage,
  sauf ceux de ng00 K5 (sonde u21 et u18) et de l'uniforme 8 000 (u21 et u18), gardés dans `$S/fp/` pour les
  comparaisons.

## 11. Pièces (toutes sous `$S`)

| Thème | Pièces |
|---|---|
| Configuration et construction | `configure_u21.log`, `build_u21.log`, `configure_u18.log`, `build_u18.log`, `v10_configure.log`, `v10_build.log`, `configure_ref.log` |
| CTest | `ctest_main.log`, `ctest_lidar.log`, `ctest_diffv10.log`, `ctest_campaign_rerun.log`, `diag/full_campaign_direct.log` |
| Empreintes | `fp/records_u21.jsonl`, `fp/records_u18.jsonl`, `fp/records_warm.jsonl`, `fp/records_u21_extra.jsonl`, `fp/semantic_u8000.log`, `fp/semantic_ng00.log` |
| CLI | `cli/*_ng00_k5/` (fichiers et manifestes), `cli/*.stdout`, `cli/full_ng00_k10/manifeste.json` |
| Python | `pyS/reference_S.log`, `imports_nus.log`, `check_docs.log`, `check_poly.log` |
| Outils | `tools/run_probe.py`, `tools/semantic.py`, `tools/imports_nus.py` |
