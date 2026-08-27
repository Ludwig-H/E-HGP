# Réponse de Claude à l'audit du pin `9762daaf` (ETAT_COURANT du 27 août 2026)

- **Date :** 27 août 2026
- **Audit :** [`ETAT_COURANT.md`](ETAT_COURANT.md) (pin fonctionnel audité `9762daaf`)
- **Corrections :** commit `338a9ef8` (`main`), portes rejouées localement ; reçus à venir sur ce pin (campagne locale 8000/16000/32000 relancée, session G4 à suivre)
- **Cadre :** `phase=exploration_v5_hors_registre`, `public_status=not_claimed` ; rien ici ne promeut un statut

## P0 — contrat structurel du fold

Fermé. `validate_fold_events` (`src/forest/fold.hpp`) s'exécute **avant** toute
allocation ou tri, distinctement de la garde de capacité : `q in [2, 11]`,
`d <= 9`, `q + d <= 11`, un même `K = q + d - 1` dans l'appel, identifiants
distincts (support et intérieur), `active_mask < 2^q`. Refus `invalid_input`
avec l'index fautif. `tests/fold_contract_gate.cpp` (`mhgp5_fold_contract`) :
rejets permanents `q11+d1` (votre probe), `q12`, `d10`, `q1`, `q0`, `K`
mixtes, doublon support/intérieur, doublon dans le support, bit de masque haut
(seul et parmi des valides) ; limites positives `q11+d0` (masque plein),
`q2+d9`, vide. Rejoué vert sous `MHGP5_ENABLE_SANITIZERS` (ASan + UBSan,
`build/v5-asan`) avec `mhgp5_api_guard_gate`.

## P0 — plafond de coquille

Fermé. `kShellCapProfile = 12` ; `validate_run_options` refuse
`shell_cap` hors `[4, 12]` avant tout calcul (`invalid_input`). `api_guard` :
`13`, `32`, `SIZE_MAX` refusés à zéro callback et sans payload ; `12` accepté.
Une coquille effectivement supérieure au plafond reste `resource_exhausted`,
jamais tronquée (inchangé).

## P0 — livraison CUDA

Fermé au niveau de ce que l'hôte peut prouver ; **aucun résultat CUDA n'est
annoncé** tant qu'un build `nvcc` frais et les sorties complètes du témoin ne
sont pas rapatriés.

- Masquage de `D2` : les copies doubles s'appellent `F0..FQ`.
- Oracle arithmétique : tous les cas sont désormais **dans** le contrat exact
  de `dint.hpp` — mode A `|x| < 2^62` avec `b` sur tout `i64`, mode B
  `|x| < 2^78` avec `|b| < 2^40` ; l'oracle est l'`__int128` natif, sans
  débordement possible ; le repli « concordance hôte/device » a disparu.
- CMake : options GCC bornées à `CXX` (`$<COMPILE_LANGUAGE:CXX>`, generator
  expressions citées) ; la cible CUDA reçoit
  `-fmad=false;--expt-relaxed-constexpr;-Xcompiler=-Wall,-Wextra,-Werror`.
- Vérification sans `nvcc` : le fichier est passé à `g++ -fsyntax-only` avec
  `__CUDACC__` et un stub CUDA (`<<<>>>` retirés) — syntaxe hôte et corps des
  kernels acceptés. Ce n'est pas un build `nvcc` ; la session G4 le fera.

## P1

- **Compteurs des seeds mortes** : le témoin compare verdict **et** compteurs
  de certification pour **toutes** les seeds ; mutant
  `witness-no-warp-correction` (drapeau hôte passé au kernel : la correction
  intra-warp au franchissement de `h3` est désactivée) → porte en code 4,
  registre à 48 mutants (`mutants_gate.py` vert).
- **Protocole G4** : un témoin non conforme **ou** `nvcc` absent refuse
  immédiatement les phases 1 et 2 (`exit 3`, statut gravé) ; `gpu_witness.txt`
  contient `nvcc --version`, `nvidia-smi`, `uname -m`, la commande, la
  configuration et le build ; `validate_v5_campaign.py` exige le lot
  arithmétique et les **deux** familles avec planchers (1000 seeds) et
  `desaccords=0`, pas la sous-chaîne finale ; `selftest_campagne_v5.sh` à
  huit scénarios (témoin en échec ⟹ aucune phase suivante ; sous-chaîne OK
  avec un désaccord ⟹ refusé) : `PROTOCOLE CONFORME`.
- **Exception dans `on_forest`** : `BgJoiner` RAII + `std::exception_ptr` ;
  l'exception est capturée dans le fil d'arrière-plan, le fil est joint, puis
  elle est relancée par `run_pipeline` dans le fil appelant. Contrat écrit
  dans `run.hpp` ; `api_guard` : callback qui lève à `K = 2`, exception
  propagée avec son message, pipeline arrêté à `K = 2` (deux callbacks).
- **`partition << 26 | tid`** : remplacé par deux tableaux `{ev_part (u8),
  ev_fid (u32)}` — injectif sans borne par partition, `kFoldTidMask` retiré ;
  aucun refus tardif ajouté.
- **Métrique `intern`** : n'inclut plus `merge`. `par_gate` : la vérification
  de `rle_workers` et `fold_workers` reste à ajouter (elle est dans la même
  file que le rejeu des reçus ci-dessous).

## Preuves et reçus

- Reçus déjà **postérieurs** au fold/pipeline/RLE, commis avant cet audit :
  `receipts/conformite_v4/campagne_v5_d08913ac7936_20260827.txt` (12/12
  conformes, local) et `receipts/campagne_g4_v5_20260827_fold_parallele`
  (12 conformités + 4 contrats 50 000 à `9762daaf`, `digest_all` identiques à
  la session 1). Comme vous le demandez, les 12 cas appariés sont **rejoués
  sur `338a9ef8`** (campagne locale en cours) et une session G4 suivra avec
  le témoin en phase 0.
- Documentation : arbitrages V1–V4 migrés dans `docs/MATHEMATIQUES.md` et
  `docs/ARCHITECTURE.md` (statuts inchangés) ; la décision sur les
  applications verticales est gravée en `ARCHITECTURE.md` § 7.1.

## Ce qui reste ouvert de mon côté

- `par_gate` : ajouter `rle_workers` / `fold_workers`.
- Boost absent localement (troisième autorité `cpp_int`) ; `mutants_gate.py`
  compte encore les mentions en commentaire comme des sites.
- Point 2 (GPU) : le kernel `k_scan` n'est prouvé que par le témoin (à
  compiler sur G4) ; la lane q3 par lots (`src/gpu/q3_lane_batched.hpp`) est
  prouvée égale à la production sur CPU.
