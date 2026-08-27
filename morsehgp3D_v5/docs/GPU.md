# MorseHGP3D v5 — plan du port GPU (point 2)

Cadre inchangé : `public_status=not_claimed`, aucune cible CUDA livrée à ce
jour. Ce document dit **quoi** porter, dans **quel ordre**, et **comment on le
prouve** ; rien n'y est un résultat.

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

- **Même objet** : le multiensemble des candidats post-RLE (clés primitives,
  représentants de niveau, arités) produit par le device doit être
  **bit-identique** à celui du chemin CPU sur les mêmes entrées — c'est la
  porte, et la seule.
- **Exactitude** : toute décision reste entière ; le device n'a pas de
  `__int128` → arithmétique 128 bits **portable** (`src/core/dint.hpp`,
  limbes u64, prouvée égale à `__int128` sur CPU par `mhgp5_dint_gate`) ; les
  formes q3/q4 sont dupliquées en variantes device (`src/lanes/device_forms.hpp`)
  prouvées égales aux formes de production sur tous les triangles/tétraèdres
  de petits nuages et sur les fixtures u16 extrêmes.
- **Le flottant reste un filtre** : la borne certifiée par seed et la séquence
  FMA figée valent sur device à condition d'imposer l'arrondi au plus proche
  et d'interdire toute contraction hors de la séquence (`-fmad=false` ou
  `__fmaf_rn`/`__fma_rn` explicites) ; le repli exact est obligatoire.
- **Ce qui ne passe pas sur device (première version)** : les niveaux q4
  exacts (U192/U320) et `cmp_2p2_jb2` — le device rend les **candidats** (clé
  primitive + seed + complétion) ; le CPU forme le niveau q4 et exécute le
  cœur de seed exact sur les seuls candidats remontés, ou bien le device
  n'exécute que les filtres certifiés et laisse au CPU les replis.
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
5. **Lotissement et mesure** : regrouper les rectangles d'un ouvrier en lots
   de plusieurs dizaines de milliers de seeds par lancement (l'ordre
   d'émission est préservé : un ouvrier traite ses rectangles en séquence),
   flux asynchrone, puis banc apparié CPU 48 fils / GPU sur `eight_clusters`
   et `scanline_single_pass` 50 000 ; reçu ; jamais un claim.
