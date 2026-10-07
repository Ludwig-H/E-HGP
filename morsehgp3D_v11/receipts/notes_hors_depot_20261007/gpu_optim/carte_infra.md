# Carte infrastructure et protocole GPU de la v11

Rédigée le 2026-10-06 vers 01:22 UTC (`date -u`), en lecture seule, sur `origin/main` `acb6a50b9`. Depuis `df904711a`, aucun fichier n'a changé sous `morsehgp3D_v11/src`, `bench`, `CMakeLists.txt` ni `gcp-migration/` (seul un audit a été ajouté). GCP non utilisé, aucun build, aucun commit.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 (voie mesurée, non qualifiée)
profile=quantized_u21_input_only
public_status=not_claimed
```

Légende des preuves : **[G4]** mesure sur la VM, avec le reçu cité ; **[src]** lecture du code ; **[est]** estimation raisonnée, jamais une mesure.

---

## 1. Ce qui existe

### 1.1 Option de construction

- `MHGP11_ENABLE_CUDA` vaut OFF par défaut (`morsehgp3D_v11/CMakeLists.txt:68`). Quand elle est active : `CMAKE_CUDA_ARCHITECTURES=120` par défaut, `enable_language(CUDA)`, `find_package(CUDAToolkit)`, et un contournement pour CMake 3.22 (`CMAKE_CUDA20_STANDARD_COMPILE_OPTION=-std=c++20`). L'option refuse ASan, UBSan et TSan. **Les avertissements `-Wall -Wextra -Wpedantic -Werror` ne s'appliquent qu'au C++** : le `.cu` est compilé sans `-Werror` [src].
- `src/catalogue/module.cmake:6-10` ajoute `leaf_batch_cuda.cu` et `leaf_batch_cuda_context.cu`, définit `MHGP11_HAVE_CUDA=1`, passe `--expt-relaxed-constexpr -lineinfo -Xptxas=-v` et lie `CUDA::cudart_static`. Le rapport `-Xptxas=-v` (registres, pile, débordements) n'est **ni conservé ni testé** [src].
- Dans le code device, ni `float` ni `double` (vérifié par grep sur `leaf_device*.hpp`, `leaf_batch*.cu`, `leaf_batch.hpp`). `-fmad` et FTZ sont donc sans objet aujourd'hui, mais rien ne l'impose pour un futur noyau [src].
- La voie GPU n'existe que dans la sonde `bench/full_probe.cpp`. Les bits de masque sont 16384 (feuille device jouée sur l'hôte), 32768 (lot de feuilles exécuté sur le Pool de l'hôte) et 65536 (lot sur CUDA). Les modes du banc sont 16379 (CPU), 49147 (lot hôte) et 81915 (GPU). **L'API et la CLI sont figées sur `kEngineMask = 16379`** (`src/api/internal.hpp:21`) : aucune sortie produit ne passe par le GPU [src].

### 1.2 Machine G4, d'après les reçus

| Fait | Valeur | Source |
| --- | --- | --- |
| GPU | RTX PRO 6000 Blackwell Server Edition, `compute_cap 12.0`, 97 887 Mio, 2 430 MHz SM max, 600 W | [G4] `gpu_g4/sessions/claudegpu3/prof_k5/gpu_profile.json` |
| Pilote | 580.178.04 ; `RmProfilingAdminOnly: 1`, donc ncu passe par `sudo -n`, disponible | idem |
| nvcc | `/usr/local/cuda/bin/nvcc`, 12.9 **V12.9.41**, installé sur l'hôte de la VM : ce n'est pas l'image `containers/cuda12.9-sm120.Dockerfile` (12.9.2, `CUDA_MODULE_LOADING=EAGER`) | [G4] `claudegpu5/gpu_k5_report.json` → `build` |
| CMake, compilateur, Python | CMake/CTest 3.22.1, g++ 11.4.0, Python 3.10.12 nu (sans numpy), Clang absent | [G4] `vm_facts` des reçus claudegpu3/5 |
| Hôte | 48 vCPU, 185 Go, AVX-512 ; TSan seulement sous `setarch -R` | idem |
| Construction CUDA de la sonde | configure 2,0 s, build 8,6 s | [G4] `claudegpu5/gpu_k5_report.json` |
| Nsight | nsys 2025.3.1 et ncu 2025.2.1.3, téléchargés sur la VM, épinglés par sha256 et taille (`bench/gpu_profile.py:31-35`) | [src] |

### 1.3 Sessions gardées

Le seul contrôleur est `gcp-migration/v11_session.py`, sur la cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`.

- **Durée.** La cible n'admet que `--max-run-seconds 4200` (`check_target` exige l'égalité avec le `maxRunDuration` configuré). Cela donne `guest_shutdown_minutes=55`. L'échéance du worker tombe 2 682 s après la génération de démarrage (claudegpu5 et claudegpu6 : 2 682,4 s et 2 682,6 s, calcul à partir de `worker_deadline_epoch` et `generation`) [G4]. La fenêtre utile d'environ 2 330 s vient de la note mémoire `voie-gpu-v11` et n'est pas recalculée ici.
- **Commandes admises par le plan** : `python3 {src}/morsehgp3D_v11/<script>.py`, `ctest …` ou `./mhgp11*`. Une construction CUDA passe donc par un script. `bench/gpu_ab.py --src` construit lui-même `b_cuda/mhgp11_full_bench`.
- **Python.** `python_packages: none` par défaut (Python 3.10 nu). `pinned` installe numpy 2.2.6, scipy 1.15.3, scikit-learn 1.7.2 et hdbscan 0.8.44, au prix de 600 s retenues sur la fenêtre.
- **Source.** `--commit` (preuve poussée) ou `--snapshot` (développement, pas une preuve). Le verrou est partagé avec la v10 : une seule VM active à la fois.
- **Durée réelle des bancs** (claudegpu6) [G4] :
  - K5 : 42 prises froides en 41 s et 6 processus chauds en 22 s, soit environ 63 s ;
  - K10 : 30 prises froides en 159 s et 6 processus chauds en 97 s, soit environ 256 s.
  - Le temps de banc n'est donc pas la contrainte. Le coût d'une session tient au démarrage, au téléversement et à la clôture.

### 1.4 Mémoire : contrat R7

- Chaque tableau device est un `DeviceArray`. Il réserve d'abord dans le `MemoryBudget` commun (`BudgetReservation`), puis appelle `cudaMallocAsync` sur le flux par défaut, dans le pool du périphérique. Le pool garde la mémoire rendue (`ReleaseThreshold = ~0`). Les sorties hôte sont admises avant les tableaux device (`leaf_batch_cuda.cu:119-140, 353-382`) [src].
- Trois grandeurs restent distinctes : le compte logique (budget), `device_bytes` (cumul des allocations) et la mémoire physique (`UsedMemHigh` et `ReservedMemHigh`, ajoutés en `57dd21be1`). **La mémoire physique n'a jamais été mesurée** : `device_pool_used_high` et `device_pool_reserved_high` valent `None` dans les rapports GPU6 [G4].
- Le contexte CUDA et les piles des fils ne sont pas comptés.
- Sur ng00, à froid, première prise : K5 donne `jobs = 353 456` et `device_bytes = 798 Mo`, dont environ 724 Mo pour les seules cases de 2 Kio par feuille (353 456 × 2 048) ; K10 donne `jobs = 530 259` et `device_bytes = 1,28 Go` [G4, `claudegpu6`].

### 1.5 Portes et mutants du chemin GPU

| Ce qui existe | Ce qui manque |
| --- | --- |
| `mhgp11_tower_full_leaf_lanes` (CTest) : 3 000 sites, K5/16 et K10/24, modes CPU, feuille hôte et lot hôte ; dump et registre identiques ; planchers `lot > sites`, `copiees > 0`, `rejouees > 0` à K10 | **Aucun mode 81915 dans une porte CTest**. La seule preuve native du GPU est `bench/gpu_ab.py`, joué sur G4 (372 prises froides et 84 processus chauds conformes, sessions 3 à 6) |
| Matrice G4 (`tools/g4_matrix.json`) : Release u18/u21/u24, empoisonnement, ASan/UBSan, TSan, mutants | **Aucune configuration `MHGP11_ENABLE_CUDA=ON`** dans la matrice ; la qualification finale (`claudefin*`, `clauderepriser*`) ne construit pas le GPU |
| 485 mutants tués sur 485 (reprise 3) | **Aucun mutant** sur `leaf_device.hpp`, `leaf_device_predicates.hpp`, `leaf_batch*.cpp/.cu`, `single_pass_batch.cpp` ni `leaf_queue.hpp` (`tests/mutants/catalogue.json` ne vise que 15 autres fichiers) |
| Repli exact `unresolved` → `leaf.cpp`, relu par l'auditeur (145 391 gardes Fraction/Gram, 18 426 contrôles scalaires CUDA) | **Repli jamais exercé en natif** : `non_resolues0` dans la porte de 3 000 sites, `unresolved = 0` sur les trois trames à K5 et K10 [G4] |
| Portes G4 natives demandées par l'auditeur : q3 extrême, q4 au seuil 2^20 et 2^20 + 1, préfixe obtus, coquille à qmin = 2 | **Non écrites** (README `gpu_g4`, « Limites et suite ») |
| — | `compute-sanitizer` (memcheck, racecheck, initcheck) jamais employé en v11, alors que CMake exclut ASan et TSan sous CUDA |
| — | Toute erreur CUDA devient `Reason::parameter_out_of_range` : une panne device ne se distingue pas d'un mauvais paramètre dans les reçus |

### 1.6 Références d'identité déjà stables

Les dumps FULL sont identiques à l'octet entre la sonde GPU (`00800dd88` → `22a6af6aa`, 4 octobre) et la CLI de la mesure finale (`38b76701b`, 5 octobre) [G4] :

| Trame | K5 FULL | K10 FULL |
| --- | --- | --- |
| ng00 | `3a2bfb4f9f48…` | `61a4245b91d9…` |
| ng01 | `5212a2ced81b…` | `838a447e0b92…` |
| ng02 | `78feb765e21c…` | `81f89995eacc…` (sonde GPU seulement) |

(`gpu_g4/sessions/claudegpu{3,5,6}/gpu_k*_report.json` → `identity` ; `qualification_finale/claudefinmesure/sorties_g4.json` → `calls`.) Ces empreintes peuvent servir de **références gravées**. Aujourd'hui, `gpu_ab.py` compare seulement à la première prise CPU de la même session.

---

## 2. Faits mesurés qui bornent l'intérêt du GPU (Amdahl)

Étages à W48 et K5, médianes de trois prises, d'après `claudefinmesure/sorties_g4.json` (CLI, masque 16379) [G4] :

| Trame | domain | tree | FULL hors écriture (cloud + index + domain + tree) |
| --- | ---: | ---: | ---: |
| ng00 | 221,6 ms | 163,6 ms | 388,8 ms |
| ng01 | 179,9 ms | 139,2 ms | 322,3 ms |
| ng02 | 230,5 ms | 150,7 ms | 385,4 ms |

Médianes à froid de la sonde GPU6, K5 et W48, CPU puis GPU [G4, `claudegpu6/gpu_k5_report.json` → `cold_medians_ms`] :

- **single_pass** : 129–154 ms sur CPU contre 84–106 ms sur GPU (l'exécuteur est mis à part) ;
- **domain** : 179–216 ms sur CPU contre 247–278 ms sur GPU. Domain moins single_pass vaut donc environ 57 ms sur CPU et 148–180 ms sur GPU : exécuteur, matérialisation des Level et préfixes, tous en série ;
- **forest** : 118–165 ms, identique sur les deux voies.

Conséquence [est] : même avec un exécuteur gratuit et parfaitement recouvert, FULL K5 reste au-dessus de la somme single_pass GPU (84–106) + reste du domaine CPU (environ 57) + tree (139–171), soit **environ 280–330 ms**. **Le contrat de 100 ms ne se ferme pas par la voie des feuilles. L'étage `tree`, entièrement sur CPU et sans aucune voie GPU, dépasse à lui seul 100 ms sur les 18 prises chaudes à W48** (audit `AUDIT_CONTRATS_…`, section « Mesure finale close »). Cela confirme le plafond de ×1,9 pour les feuilles seules (conception v10, § 5.4 de l'audit des transpositions).

---

## 3. Protocole A/B GPU standard

À appliquer à **chaque** changement de noyau ou de transport. Il prolonge `bench/gpu_ab.py` et `bench/ab_g4.py`.

1. **Source.** Preuve en `--commit` (poussé) ; `--snapshot` réservé au développement. Avant de lancer : `uptime -s` sur le codespace et contrôle des dossiers de session sans `DONE`. Sessions de `--max-run-seconds 4200`.
2. **Ordre du plan.**
   - Commande 0 : construction CUDA plus fumée sur 3 000 sites (`full_leaf_lanes` avec le mode GPU, timeout ≤ 300 s), pour qu'une panne de compilation n'emporte pas toute la fenêtre.
   - Puis A/B K5, A/B K10, et Nsight en dernier.
3. **Bras.** Un seul binaire CUDA, les modes choisis par masque : référence CPU 16379, variante GPU, et un **bras A/A** (16379 joué deux fois sous deux noms) pour mesurer le bruit dans la session. Ordre de Williams entre bras.
4. **À froid.** Un processus neuf par prise, **au moins 7 prises** par (trame, bras), sur les trois trames ng00, ng01 et ng02. Publier la médiane et l'intervalle de chaque prise (contexte, envoi, comptage, écriture, retour, Level), sans en déduire de total.
5. **À chaud.** Un processus par (trame, bras), **P = 8 passes**, dont on publie la **médiane des passes 2..P** (la meilleure passe seulement en complément). L'identité porte sur la passe P. Pour la qualifier passe par passe, ajouter à la sonde une empreinte du catalogue en mémoire à chaque passe, sans écrire de dump (proposition).
6. **Paramètres.** W48 en principal. W1 sur une trame pour le travail (CPU·s) et le chemin critique. K5 avec feuilles de 16, K10 avec feuilles de 24.
7. **Identité, exigée à chaque prise.**
   - sha256 du dump = **empreinte gravée** du § 1.6, et pas seulement la première prise CPU de la session ;
   - registre du catalogue égal ;
   - `unresolved`, `fill_jobs` et `copied_jobs` publiés ;
   - toute prise refusée rend la session non conforme.
8. **Décision.**
   - Rapport GPU/CPU par prise appariée (même rang du carré de Williams), puis médiane des rapports par trame.
   - Un gain n'est annoncé que si les **trois trames** vont dans le même sens et que l'écart dépasse le bruit A/A mesuré dans la même session.
   - Repère : entre deux sessions, la voie CPU chaude de ng00 a varié de 363,3 à 342,7 ms (S5 contre S6), environ 6 % [G4]. À W48, cinq prises ne tranchent pas sous 30 à 40 ms (audit des transpositions, point 10).
9. **Mémoire.** Publier `device_bytes`, `device_pool_used_high` et `device_pool_reserved_high` (jamais encore mesurés), le pic du budget logique et le RSS.
10. **Ressources des noyaux.** Conserver la sortie `-Xptxas=-v` (registres, pile, débordements) dans le rapport de construction. Référence ncu : 168 registres, pile locale de 3,2 Kio.
11. **Nsight à part.** Ne jamais chronométrer sous profileur. nsys sur trois passes de ng00 ; ncu `--set full` limité aux noyaux nommés, `--launch-count 2`. Rapports bruts ≤ 64 Mio rapatriés, empreintes dans `gpu_profile.json`.
12. **Reçu.** `receipts/<chantier>_<date>/` avec `check.py`, qui recalcule chaque tableau (modèle de `gpu_g4/check.py`, 553 contrôles), et arrêt `TERMINATED` certifié sur la cible exacte.

## 4. Portes à écrire pour chaque nouveau noyau

| # | Porte | Où | Code attendu |
| --- | --- | --- | --- |
| P1 | **Jumeau hôte** : le même code `__host__ __device__` exécuté par l'exécuteur du Pool ; `full_leaf_lanes` étendu au nouveau mode (dump et registre identiques, planchers non vides) | CTest local, label `fast` | 0 |
| P2 | **Mode GPU dans `full_leaf_lanes`**, enregistré seulement si `MHGP11_HAVE_CUDA` (mode 81915 ou successeur), comparé aux mêmes empreintes que P1 | CTest, label `gpu`, joué sur G4 | 0 |
| P3 | **Repli forcé** : un drapeau de test (« toutes les feuilles / une sur deux non résolues ») qui rend `unresolved > 0` et exige le même dump ; mutant « repli désactivé » tué | CTest local (hôte) et G4 (GPU) | 0, et 4 pour le mutant |
| P4 | **Mutants du chemin device** : `leaf_device.hpp`, `leaf_device_predicates.hpp`, `leaf_batch.cpp`, `single_pass_batch.cpp` (préfixes, ordinal, case pleine ou débordée, rang local, borne `kMaxSites`), tués par le jumeau hôte, donc sans GPU | `tests/mutants/catalogue.json` | 4 |
| P5 | **Fixtures numériques extrêmes** demandées par l'auditeur : q3 extrême, q4 au seuil 2^20 et 2^20 + 1, préfixe obtus, coquille à qmin = 2, en u21 et en u24 | CTest `oracle`, plus G4 avec le GPU | 0 |
| P6 | **Budget** : plafond = besoin − 1 → refus typé avant tout lancement ; panne d'allocation simulée ; retour au budget préexistant ; pics du pool publiés | CTest local (hôte) et G4 | 2 ou 3 |
| P7 | **Ouvriers** : sorties identiques à l'octet à W1, W4 et W48 avec le GPU | G4 | 0 |
| P8 | **compute-sanitizer** (`memcheck`, `racecheck`, `initcheck`) sur la porte de 3 000 sites, à K5 et K10 | Plan G4, commande dédiée | 0 |
| P9 | **Ressources ptxas** : registres, pile et débordements extraits du journal ; refus au-delà d'un plafond déclaré | Rapport de construction G4 | 0 ou 3 |
| P10 | **Identité gravée sur les trames** : les trois trames, K5 et K10, contre le § 1.6 | `gpu_ab.py` | 0 |

---

## 5. Pièges déjà payés

1. **CMake 3.22.1** ne connaît pas le dialecte CUDA20 : `claudegpu1` est morte à la génération (corrigé, `CMakeLists.txt:88-92`). Configurer en local avec la roue `cmake==3.22.1` avant toute session CUDA ; le codespace n'a pas nvcc.
2. **Garde fausse « feuilles ≤ sites »** : les feuilles se recouvrent (80 feuilles pour 9 sites), d'où `claudegpu2` refusée en entier. Remplacée par des plafonds de ressources (2^40 jobs, et 2^32 pour CUDA).
3. **Retour à 4 Go/s** vers des pages neuves (77 Mo en 19 ms), corrigé en pré-touchant les pages en parallèle (> 10 Go/s). Le retour coûte maintenant 2,3 ms à K5 [G4].
4. **Contexte à 78 ms** (`cudaFree(0)`) ; ouverture anticipée dans un fil. À froid, première prise de ng00 : prefetch 370 ms et `device_init` 237 ms, mur de 629 ms [G4, GPU6]. Le régime froid est dominé par le pilote.
5. **Barrière retirée = attente déplacée** : passer à des blocs d'un warp a fait monter l'attente mémoire de 1,19 à 3,03 cycles par instruction, pour 3 % de gain.
6. **Tri des feuilles par taille sans effet** : la taille ne prédit pas le travail (3,2 à 3,4 fils actifs sur 32).
7. **Format compact neutre sur le mur** : les rangs locaux par recherche linéaire ajoutent environ 30 ms au comptage à K10.
8. **Banc vert par vacuité** (prises ou passes absentes), corrigé au pin `22a6af6aa` ; la portée de l'identité reste la dernière passe.
9. **Porte Python avec numpy** : la matrice tourne en Python 3.10 nu (`claudequal1`). Toute porte d'identité GPU n'utilise que la bibliothèque standard ; la tester sous `python3 -S`.
10. **Redémarrages du codespace hors calendrier** (11:20 UTC le 4 octobre) : le pilote meurt, la VM continue ; `--recover` arrête la VM mais **perd les résultats**.
11. **Empreintes de route par profil** : des empreintes u21 imposées en u18 et u24 ont fait échouer `claudefina2` et `claudefinm`. Toute nouvelle porte d'empreinte se grave par profil.
12. **Ports GPU antérieurs** (v5, v6, v7, produit phase 15) : jamais plus d'environ 10 % de bout en bout ; code hôte à 88 % de l'étage en v6.

---

## 6. Opportunités chiffrées

Elles sont présentées dans `StructuredOutput` avec leur protocole. En résumé :

- **O1 Recouvrement CPU/GPU** de l'exécuteur avec la passe unique (lots en flux pendant le parcours). Exécuteur à chaud : 52–59 ms à K5 et 270–360 ms à K10, en série après le parcours.
- **O2 Portes et mutants du chemin GPU** (P2 à P4) : condition de toute optimisation, sans gain de temps.
- **O3 Feuille coopérative par warp (J3)** sur le comptage et l'écriture : comptage 37,6 ms et écriture 17,6 ms à K5, comptage 218 ms et écriture 113 ms à K10.
- **O4 Régime résident** (contexte et pool vivants entre trames) : de 0 à 237 ms de démarrage à froid par processus.
- **O5 Mémoire physique et cases** : 724 Mo de cases à K5.
- **O6 Étage `tree`** : aucune voie GPU, il faut d'abord mesurer et prototyper. 139–171 ms à W48.

## 7. Pistes écartées

Voir `StructuredOutput.rejected` : transferts (déjà marginaux), graphes CUDA, mémoire unifiée, proposition flottante suivie d'une recertification, census sur GPU (v6, v7), tout sur l'appareil matérialisé (v5), forêts sur GPU sans prototype, MPS ou plusieurs processus.

## 8. Questions ouvertes

Voir `StructuredOutput.open_questions`.
