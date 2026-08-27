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
3. **Option CMake `MHGP5_ENABLE_CUDA`** (OFF par défaut, sm_120 comme le
   produit, `-fmad=false` pour qu'aucune contraction n'altère la séquence
   figée du filtre flottant), cible `mhgp5_device_witness`
   (`src/gpu/device_witness.cu`) — **écrit, pas encore compilé** : aucun
   `nvcc` local, la compilation et l'exécution ont lieu sur G4 en **phase 0**
   de `gcp-migration/v5_campaign_remote.sh` (run `gpu_witness`, exigé à
   code 0 par `validate_v5_campaign.py` ; `nvcc` absent ⇒ code 2 gravé, jamais
   un vert de complaisance). Le témoin exécute sur le device (a) 2^18 tirages
   d'arithmétique DI128 plus les bords, (b) le scan q3 **warp-par-seed** (les
   32 fils balaient les sites, `__ballot_sync`/`__popc` comptent les
   intérieurs, la sortie anticipée à h3 est reproduite en retirant la
   contribution des sites au-delà du h3-ième intérieur) sur toutes les seeds
   des ancres survivantes d'`uniform` et `eight_clusters` à n = 400 ; les
   verdicts mort/vivant et les compteurs de certification sont comparés bit
   à bit au scan shaped CPU (lui-même égal à la lane de production).
   Plancher : 1000 seeds par famille.
4. **Kernel q3** (warp-par-seed) + porte d'égalité device/CPU post-RLE sur
   G4 ; puis **kernel q4** (W_4, cœur de seed avec repli CPU, complétions).
5. Mesure par banc apparié CPU 48 fils / GPU sur `eight_clusters` 50 000 ;
   reçu ; jamais un claim.
