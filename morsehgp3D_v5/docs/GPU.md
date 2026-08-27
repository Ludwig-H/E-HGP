# MorseHGP3D v5 — plan du port GPU (point 2)

Cadre inchangé : `public_status=not_claimed`. Ce document dit **quoi** porter,
dans **quel ordre**, et **comment on le prouve** ; une mesure y reste une
mesure. **État par étape** (distinguer source présente, compilation `nvcc`,
exécution conforme, mesure, pin du reçu) :

| cible | source | compilée (`nvcc`) | exécutée conforme | mesurée | reçu (pin) |
|---|---|---|---|---|---|
| témoin device (DI128 + scan q3 warp) | `src/gpu/device_witness.cu` | oui | oui, 0 désaccord | — | `campagne_g4_v5_20260827_temoin_device` (`24b3f164`) |
| lane q3 device | `src/gpu/q3_lane_device.cuh` | oui | oui, 1200 / 1200×4 fils / 8000×8 fils | temps kernel gravés, pas de banc apparié | idem |
| lane q4 device | `src/gpu/q4_lane_device.cuh` | oui | oui, 1200 / 1200×4 fils / 8000×8 fils (157 M complétions, 0 désaccord) | temps kernel gravés | `campagne_g4_v5_20260827_lane_q4_device` (`2e75cb42`) |
| pilote `mhgp5_cuda --gpu` (contrats 50 k) | `cli/mhgp5_cuda.cu` | oui | `uniform` et `terrain` : `digest_all` **identique** au contrat CPU ; `eight_clusters` et `scanline` : **abort** (`cudaMalloc : out of memory`, RSS 119 / 98 Go — paires q4 non bornées, corrigé en `a4e75d68`+, à recevoir) | GPU **plus lent** sur les familles régulières : `uniform` 87 s vs 78 s, `terrain` 41 s vs 23 s (génération 25,8 s vs 16,6 s, 31,4 s vs 13,7 s — coût hôte de matérialisation des lots ; kernels 15 s cumulés sur 48 fils) | idem (campagne `partial`) |
| mutant du témoin sur device | `--inject=witness-no-warp-correction` | à recevoir (protocole : run `gpu_mutant`, code 4) | — | — | — |

Lecture de la session `2e75cb42` : le lotissement fait tomber les lancements
q3 à 8000 de 645 636 à 542 (kernel 6,6 s → 24 ms) ; l'égalité de bout en bout
à 50 000 points avec les deux lanes device est établie sur `uniform` et
`terrain` ; mais sur ces familles régulières le device **coûte** plus qu'il ne
rapporte (les seeds y meurent tôt sur CPU, tandis que le lot copie tous les
sites de toutes les ancres). Le gain attendu est sur les familles denses
(`eight_clusters` : 94 + 88 s de lanes CPU à 48 fils), précisément celles qui
ont manqué de mémoire — bornes en sites (`2^20`) et en paires (`2^24`)
ajoutées, à mesurer. Suite prévue : un **exécuteur adaptatif** (une ancre va au
device seulement si son travail estimé — seeds × sites — dépasse un seuil,
sinon elle est scannée sur l'hôte dans le même pipeline ; les deux exécuteurs
étant prouvés égaux, l'objet ne change pas) et le recouvrement des transferts.

Ce qui n'a pas de reçu n'est pas reçu.

## 1. Ce que la mesure G4 a désigné (27 août 2026)

Sur `g4-standard-48` à 48 fils, K = 1..10 exact (reçu
`receipts/campagne_g4_v5_20260827/`) :

| famille 50 000 | génération | dont lanes q3 / q4 | fold (avant parallélisation) |
|---|---:|---:|---:|
| uniform | 16,8 s | 2,3 / 4,1 s | 114,8 s |
| eight_clusters | **190,9 s** | **94,0 / 87,6 s** | 103,5 s |

Le fold est traité (point 1 : partitionné, pipeliné). Le poste GPU est la
**génération sur covers denses** : par ancre survivante, un scan des sites du
cover pour chaque seed aigu (kernel affine `L = G·q − 2·u·N`, étage flottant
certifié, repli exact i128), le cœur de seed de Jung, et les complétions q4
(owner, exact-once, préfiltres, Cramer, centre strict, profondeur). Ce travail
est **régulier** (mêmes opérations sur tous les sites d'un cover) et sans
allocation : la forme voulue est *warp-par-seed* (un warp balaie les sites
d'un seed, réduction par ballot pour la profondeur), les ancres d'un rectangle
en blocs.

## 2. Contraintes non négociables

- **Même objet** : l'objet comparé est le **vecteur post-RLE** des candidats
  (trié par `ball_candidate_less`, dédoublonné par clé ; chaque élément = clé
  primitive, niveau exact, arité) produit par la lane device, qui doit être
  **égal élément par élément** à celui de la lane CPU sur les mêmes entrées ;
  à un fil, l'ordre brut d'émission est comparé aussi ; les compteurs de la
  lane (7 pour q3, 22 pour q4) doivent coïncider. C'est la porte, et la seule.
- **Exactitude** : toute décision reste entière. Arithmétique 128 bits
  **portable** (`src/core/dint.hpp`, limbes u64, prouvée égale à `__int128`
  sur CPU par `mhgp5_dint_gate` et sur device par le témoin) pour les formes
  et les scans ; `__int128` **est** employé en code device là où `wide.hpp`
  l'emploie déjà (`cmp_2p2_jb2`, cœur de seed q4), nvcc 12.9 avec GCC hôte
  le supportant — contrat de backend écrit dans `dint.hpp`. Les formes q3/q4
  sont dupliquées en variantes device (`src/lanes/device_forms.hpp`)
  prouvées égales aux formes de production sur tous les triangles/tétraèdres
  de petits nuages et sur les fixtures u16 extrêmes.
- **Le flottant reste un filtre** : la borne certifiée par seed et la séquence
  FMA figée valent sur device à condition d'imposer l'arrondi au plus proche
  et d'interdire toute contraction hors de la séquence (`-fmad=false` ou
  `__fmaf_rn`/`__fma_rn` explicites) ; le repli exact est obligatoire.
- **Ce qui reste à l'hôte** : les clés et niveaux exacts (réduction
  rationnelle, U192) des candidats émis — le device rend les verdicts (q3 :
  mort/vivant par seed ; q4 : cœur par seed, étage par complétion,
  profondeur par candidate) et l'hôte forme clé et niveau à partir de la
  forme recalculée. Le cœur de seed exact (`cmp_2p2_jb2`) s'exécute sur le
  device.
- **L'oracle n'est jamais porté.**
- **Pas de nvcc ici** : les kernels ne se compilent et ne s'exécutent que sur
  la G4, par sessions gardées ; tout ce qui peut être vérifié sur CPU
  (arithmétique, formes, logique du scan sous forme « device-shaped ») l'est
  avant chaque session.

## 3. Ordre des livraisons

1. **Arithmétique et formes portables** (CPU-vérifiables) : `dint.hpp`,
   `device_forms.hpp`, `mhgp5_dint_gate` — **livré** (0 désaccord contre
   `__int128`, mutant `dint-mulhi-dropped` tué).
2. **Scan q3 « device-shaped »** sur CPU : une fonction qui, pour un
   rectangle vivant, prend les tableaux plats (sites affines de l'ancre,
   seeds) et rend les candidats q3 exactement comme la lane q3 de
   `generate.hpp`, mais écrite comme un kernel (indices explicites, aucune
   allocation, réductions explicites) ; porte d'égalité post-RLE avec la lane
   de production à n = 400/1200 — **livré** (`src/gpu/q3_scan_shaped.hpp`,
   `mhgp5_q3_scan_shaped_gate` : 0 désaccord, mutant `q3-shaped-strict-flip`
   tué).
3. **Option CMake `MHGP5_ENABLE_CUDA`** (OFF par défaut, sm_120 fixé avant
   `enable_language`, `-fmad=false`, options GCC bornées à CXX et relayées par
   `-Xcompiler`), cible `mhgp5_device_witness` (`src/gpu/device_witness.cu`)
   — **prouvé sur G4** (session `24b3f164`, reçu
   `receipts/campagne_g4_v5_20260827_temoin_device`, RTX PRO 6000 Blackwell
   sm 12.0, CUDA 12.9.41, pilote 580.173) : 262 144 cas d'arithmétique DI128
   + bords, 0 désaccord ; scan q3 **warp-par-seed** (`__ballot_sync`/`__popc`,
   sortie anticipée à h3 reproduite jusque dans les compteurs des seeds
   mortes) sur 728 347 seeds (`uniform` 400) et 2 308 366 seeds
   (`eight_clusters` 400), 0 désaccord ; mutant `witness-no-warp-correction`
   tué. Le protocole de campagne exécute ce témoin en **phase 0** et refuse
   les phases CPU s'il n'est pas conforme ; le validateur exige les deux
   familles, leurs planchers et `desaccords=0`. Les deux sessions
   précédentes (`9762daaf`, `50fee05c`) ont été refusées — journaux non
   rapatriés, puis erreurs de compilation réelles (ponts `__int128` cachés à
   la passe device, `edge_key` hôte appelé depuis une fonction HD, sm_52 par
   défaut) — et sont gravées comme telles.
4. **Kernel q3** (`src/gpu/q3_scan_kernel.cuh`, partagé par le témoin et la
   lane) + **lane q3 device** (`src/gpu/q3_lane_device.cuh` : exécuteur par
   fil, tampons croissants, flux, un lancement par lot) — **prouvée égale à
   la production sur G4** (`mhgp5_q3_lane_device_gate`, run `gpu_lane` du
   même reçu) : `uniform` 1200 (176 245 candidats, 3,37 M seeds),
   `eight_clusters` 1200 à 4 fils (25,65 M seeds), `uniform` 8000 à 8 fils
   (1 425 821 candidats, 35,4 M seeds) — 0 désaccord vectoriel post-RLE, 0
   désaccord de compteurs (`q3_cert`). **Pas encore rapide** : un lot = un
   rectangle, soit 645 636 lancements pour 35,4 M seeds à 8000 (55 seeds par
   lancement, 6,6 s de kernel) ; le lotissement multi-rectangles est la
   livraison 5.
   Étage hôte (CPU, prouvé) : `q3_lane_batched.hpp`
   (`generate_q3_batched_with<Scan>`), `mhgp5_q3_lane_batched_gate`
   (égalité vecteur à vecteur, ordre brut à un fil, post-RLE à plusieurs).
   **q4 en forme de kernel, prouvé sur CPU** : `q4_core_shaped.hpp` (cœur de
   seed de Jung : `mhgp5_q4_core_shaped_gate`, 0 désaccord par seed sur
   508 979 / 936 824 / 326 836 seeds, replis exacts exercés par la famille
   cocirculaire), `q4_completion_shaped.hpp` (complétions + Cramer +
   bien-centrage + profondeur avec `Q4FormD` : `mhgp5_q4_completion_shaped_gate`,
   0 désaccord d'étage sur 3,99 M / 6,25 M / 2,61 M paires, formes `det`/`N'`
   identiques i128 ↔ DI128), `q4_lane_batched.hpp` (étage hôte complet :
   `mhgp5_q4_lane_batched_gate`, égalité post-RLE et **dix-neuf compteurs**
   avec la production, 1200 × trois familles, cocirculaire, ordre brut à un
   fil, 8000 à 8 fils). Mutants tués : `q4-shaped-jung-skip-kills`,
   `q4-shaped-once-flip`, `q4-batched-emit-deep`. Reste : l'exécuteur device
   q4 (kernel cœur warp-par-seed ; complétions thread-par-paire avec
   compaction ; profondeur warp-par-candidat) et sa porte sur G4.
5. **Lotissement, kernels q4, pilote CUDA et mesure** :
   - 5a **livré (CPU)** : `generate_q3_batched_with` / `generate_q4_batched_with`
     accumulent les rectangles de chaque ouvrier dans un lot vidé dès que le
     seuil `kSeedsPerLaunch = 2^16` seeds est atteint — testé **après chaque
     ancre** (l'unité atomique), donc **borne dure** d'un lot = seuil + seeds
     de la plus grosse ancre, mesurée et exigée par les portes
     (`vidages`, `max_lot_seeds`, `max_ancre_seeds`, `--min-flushes`) ; un
     seuil < 1 est refusé (code 2) ; une ancre q4 sans seed n'est pas
     matérialisée. **Ordre** : l'ordre *local* de chaque ouvrier (rectangles,
     ancres, seeds) est préservé ; à plusieurs fils, l'ordre brut global
     n'est pas spécifié (tirage dynamique, fusion par ouvrier) — seule la
     sortie post-RLE l'est, et c'est elle que les portes exigent (l'ordre
     brut n'est comparé qu'à un fil). Contrat structurel des lots
     (`validate_q3_batch_view`, `validate_q4_batch_view`,
     `validate_q4_results_view` : SoA de même taille, tranches dans les
     tableaux, indices de lentille, `x_site`/`skip` dans la tranche,
     émissions ordonnées/distinctes/de seeds vivants, somme des étages =
     complétions, limite `UINT32_MAX` gravée par vue synthétique) vérifié
     avant tout scan et toute émission (`mhgp5_batch_contract_gate`) ; les
     primitives parallèles capturent la première exception, joignent tous
     les fils puis relancent (`mhgp5_parallel_exception_gate`, quatre fils,
     vidage au seuil et vidage final) ; réserves device transactionnelles
     (temporaires puis échange, instance inutilisable après échec) ;
     comptage des paires q4 en `u64` refusé au-delà du domaine `u32` des
     kernels ; `lots` et `kernels` comptés séparément.
   - **Arithmétique device** : contrat retenu — `DI128` (`dint.hpp`) pour
     les formes et les scans ; `__int128` autorisé en code device **là où
     `wide.hpp` l'emploie déjà** (`cmp_2p2_jb2` via `mul_192x128_320`,
     pont `di_to_i128_hd`), nvcc 12.9 le supportant avec GCC hôte. Ce choix
     de backend est documenté et mesuré, jamais un claim ; la session
     `24b3f164` compile ce chemin (témoin q3) et la session `2e75cb42` le
     chemin q4.
   - 5b **écrit, en attente de G4** (session `2e75cb42`) :
     `src/gpu/q4_kernels.cuh` — `k_q4_core` (warp par seed, six compteurs par
     ballot, correction intra-warp au h4-ième témoin), `k_q4_complete` (bloc
     par seed vivant, étage `Q4Stage` par paire de la lentille), `k_q4_depth`
     (warp par paire candidate, `q4_power_d` par ballot) ;
     `src/gpu/q4_lane_device.cuh` — `Q4DeviceExecutor` (l'hôte calcule les
     offsets de paires des seeds vivants, compte les étages, compacte les
     candidates dans l'ordre de la production entre les kernels) ;
     `tests/q4_lane_device_gate.cu` (même contrat que la porte hôte : post-RLE
     et dix-neuf compteurs). Pilote `cli/mhgp5_cuda.cu` (`--gpu` : hooks
     `RunOptions::q3_override` / `q4_override`, sans mutants ; sans `--gpu` :
     témoin CPU du même binaire). La campagne construit et exécute les portes
     q4 device dans `gpu_lane` puis, en **phase 3**, les quatre contrats
     50 000 par `mhgp5_cuda --gpu` dont `digest_all` doit être **identique**
     au contrat CPU de la même famille — l'égalité de bout en bout à 50 k est
     ainsi jugée par le validateur, et les temps GPU/CPU sont gravés dans le
     même reçu (mesure, jamais un claim).
   - Reste après la session : lecture des temps (le lotissement et les
     transferts par lot sont la première marge), banc apparié CPU 48 fils /
     GPU sur `eight_clusters` et `scanline_single_pass`, et le port du fold
     n'est **pas** prévu (séquentiel par nature, déjà recouvert).
