# morsehgp3D_v10 — Architecture et plan (conception ARCH_v2)

28 septembre 2026. Sous-système : ARCHITECTURE ET PLAN. Révision d'`ARCH_v1.md` après la critique adverse
(2 bloquants, 10 majeurs, 16 mineurs). Document de conception, pas un état du dépôt : aucun fichier suivi n'a été
modifié. GCP non utilisé.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference   (cuda_g4 à partir de V10-4b, ou plus tôt si la décision D-K5 du § 18 l'exige)
profile=quantized_u18_input_only
mode=conception_v10
public_status=not_claimed
```

Sources lues pour cette révision, en plus de celles d'ARCH_v1 :

- la critique et ses preuves `design/crit_ARCH/` (`fnv_words.py`, `t2cost.py`, `lidar02_K5_dom3_M16.json`,
  `raw00_K5_dom3_M16.json`) ;
- les conceptions sœurs `GEN_v1.md`, `TOWER_v1.md`, `CLUSTER_v1.md` et `EVAL_v1.md` (même dossier), avec
  lesquelles ce document est maintenant aligné (§ 0.3) ;
- `AGENTS.md` du worktree v9 à `ce8a649dd`, § « Ouverture v9 » (décisions datées de l'utilisateur) et
  § « Contrat principal actif » ;
- `gcp-migration/start_and_verify.sh`, `stop_and_verify.sh`, `tower_session_v9.py` ;
- `morsehgp3D_v9/tests/chain/euler_scale_gate.cpp` (protocole Kmax+2 de la v9) ;
- l'échafaudage non suivi `build/v9-open-worktree/morsehgp3D_v10/` (≈ 3,5 k lignes), lu pour cohérence
  seulement.

Artefacts produits pour cette révision, sous `design/arch_v2/` :

- `widths.py` : table des largeurs du § 17.5 ;
- `lattice_prune_check.py` : lemme P3c, 20 000 essais, 0 écart.

La recette des worktrees clairsemés (§ 20.2) a été exécutée dans un clone jetable sous le scratchpad
(29 Mo). Aucune écriture dans le dépôt partagé.

Convention : « mesuré » renvoie à un artefact cité ; « estimé » est un modèle ; « supposé » est une hypothèse à
mesurer, avec la session qui la mesurera.

---

## 0. Résumé exécutif

### 0.1 Ce qui ne change pas

Les cinq décisions structurantes d'ARCH_v1 tiennent. La critique ne remet pas en cause les couches ; ses défauts
sont locaux.

1. **Générateur sensible à la sortie** : boîtes de centres à listes certifiées (CBLE, lentille L13), durci par
   GEN_v1 (lemme Z entier, identité de boule par support canonique S\*, oracle de feuilles). Le WSPD et `s`
   disparaissent.
2. **Couches strictes, un seul chemin** : un ordonnanceur, un algorithme de tour quel que soit W, une liste
   blanche de paramètres publics, un seul résolveur (dans `tower/`, partagé par les consommateurs).
3. **Vérification hors du produit** : juges et oracles dans `tests/`, mutants par copies mutées. Le produit ne garde
   que des invariants O(sortie).
4. **Tête de clustering sur contrat abstrait** à deux producteurs (tour C∩X et atteignabilité mutuelle), même code
   de tête : c'est ce qui rend l'expérience E1 honnête.
5. **Le LiDAR comme régime de complexité.** Budgets, octets par boule et portes de coût sont posés d'abord sur les
   trames SemanticKITTI sans sol. Les trames brutes entières ont désormais leurs propres budgets (§ 6), avec un
   statut à confirmer par l'utilisateur (question 1).

### 0.2 Réponses à la critique

| Id | Constat | Verdict | Correction (section) |
| --- | --- | --- | --- |
| B1 | Six worktrees complets (1,81 Go chacun) ne tiennent pas dans 6,3 Go libres | **fondé** | worktrees **clairsemés** (`--no-checkout` + `sparse-checkout --cone`, 29 Mo mesurés), budget disque par rôle, porte disque avant tout build (§ 20) |
| B2 | Euler à kcat = kmax + 2 ne juge pas le catalogue publié : un mutant « dominateur à K−1 » reste vert | **fondé** | lemme de transfert P2b et protocole J-KM2 : Euler sur cat(kmax+2) **et** égalité canonique restrict(cat(kmax+2), adm(kmax)) = cat(kmax) du produit. Port du protocole v9 `euler_scale_gate.cpp`, perdu par ARCH_v1. Exemple du mutant traité (§ 12.5) |
| M1 | Contrat principal = trame brute entière, sans budget | **partiellement fondé** | la décision datée du 21 septembre (20:10 UTC), qui prévaut en cas de conflit (AGENTS.md, § Ouverture v9), place les contrats de temps « principalement » sur les trames sans sol. Les trames brutes ne sont donc pas le contrat principal des chronos, mais elles restent un régime publié : budgets de temps, RSS et octets, portes de correction et de mémoire (§ 6.4–6.6, § 12.6), question 1 à l'utilisateur |
| M2 | Porte K5 CPU ≤ 1 s fondée sur 8 µs/boule non mesurés | **fondé** | constantes mesurées et supposées séparées (§ 6.3) ; seuil de rentabilité c\* publié par scénario (§ 6.4) ; calibration G4 C0 dès V10-0 ; porte de temps remplacée par un verdict de contrat mesuré, plus une décision D-K5 chiffrée (§ 18) |
| M3 | Table des largeurs fausse (droite q4 / boîte, élagage `SiteTree`, Gordan, clé étendue) | **fondé** | table refaite (`arch_v2/widths.py`) : lemme Z en i128 ; le test par paramètres de droite (133 bits) est interdit ; Gordan et coplanarité avec un centre q3 passent en U256 (134 bits) ; identité par S\* ; élagage exact en i128 par minimisation séparable sur le réseau (P3c, vérifié) ; `SiteTree` réduit aux requêtes entières (§ 17.5) |
| M4 | Multiplicités sans obligations de preuve | **fondé** | obligations P2w, P5w, P7w, P17 (Euler pondéré ouvert), T2 sur copies étiquetées ; livraison par étapes : refus explicite `duplicate_positions` tant que la porte pondérée n'est pas verte (§ 5.1, D-05) |
| M5 | Statut dépendant de W sous budget mémoire | **fondé** | budget **logique** déterministe, arènes par ouvrier et marge de tranches réservées à l'ouverture de session hors budget logique, aucune réservation logique dans une région parallèle (R7, lemme P19, § 4.4) |
| M6 | Digest moteur FNV-1a sur mots : collisions triviales | **fondé** (reproduit : `fnv_words.py` rend `True`) | SHA-256 partout, par arbre de Merkle à tranches fixes et en parallèle, hors chronomètre (§ 12.8) |
| M7 | Ordres K ≥ 2 sans juge d'échelle sur les trames du contrat | **fondé** | différentiel v9 sur **chaque** trame mesurée, K5 et K10, sans sol et brute ; juge local borné (BFS sur Γ_K) ; cohérence points–verticales sur tous les sites ; matrice de couverture (§ 12.4, § 12.6) |
| M8 | Portes p95 sur 3 trames d'une séquence | **fondé** | contrat défini sur les 30 trames de test d'EVAL (10 séquences) ; blocage de données déclaré ; sur les 3 trames de conception, on ne publie que des diagnostics (§ 16, § 18) |
| M9 | Budget CI non mesuré | **fondé** | distributions de n fixées (T2 CI n ≤ 9), oracles CI réduits, mesure à la sortie de V10-0 puis gel ; budget de job 15 min, `fast` ≤ 5 min (§ 13) |
| M10 | Pas de reprise si le contrôleur G4 est perdu | **fondé** | répertoire de session persistant, `--lifecycle-state-file`, `--handoff-file`, `--guard-mark-dir`, commande de reprise sans redémarrage, contraintes de clé OS Login, politique SPOT (§ 15) |

Les 16 constats mineurs sont traités un par un en annexe F, avec trois compléments.

### 0.3 Alignement avec les conceptions sœurs

Ce document est le cadre ; en cas de divergence, voici qui prévaut.

- **GEN_v1 prévaut** pour l'algorithme du générateur, ses prédicats et son format de sortie. ARCH_v2 adopte son
  format de `Catalogue`, l'identité S\*, l'oracle de feuilles et le lemme Z. Il corrige un point : les octets par
  boule doivent compter la table des niveaux (§ 5.6).
- **TOWER_v1 prévaut** pour la tour. ARCH_v2 adopte ses étages P/G/M/T/Q, son `OrderForest` et ses invariants
  I1–I10. Ses statuts `catalogue_incomplete/*` deviennent des raisons de `invariant_violated` (§ 4.5). Son
  « mode certifié » Kmax+2 devient le juge J-KM2 hors produit (§ 12.5).
- **CLUSTER_v1 prévaut** pour les têtes et les producteurs de points. Ses modules se rangent dans `points/` et
  `head/`. Son `resolve.hpp` est retiré, comme il l'accepte lui-même au § 15 : les entrées C∩X viennent de l'étage Q
  de la tour.
- **EVAL_v1 prévaut** pour les protocoles de banc et de performance. ARCH_v2 adopte ses frontières de chronomètre
  B0/B2, ses 30 trames de test, sa définition du contrat C(K, T) et son schéma unique `mhgp10_run_v1`, qui remplace
  `mhgp10_probe_v1`. Une divergence est déclarée : l'objectif « ≤ 32 o par boule » d'EVAL (L07-F6) est incompatible
  avec une sortie FULL (ancres, contributions, index d'intervalles). Le budget d'octets normatif est celui du § 5.6.

### 0.4 Cibles chiffrées, sans complaisance

- **C(K5, 1 s) en CPU seul sur trames sans sol** : plausible, pas acquis. Il tient si la constante du catalogue,
  ramenée à l'hôte local, reste sous un seuil c\* compris entre 7,8 et 21 µs par boule selon deux inconnues
  (vitesse par fil du G4 et accélération à 48 fils) et selon le coût de la tour (§ 6.4). Le prototype mesuré est à
  14,6 µs, la cible d'ingénierie GEN à ≤ 4 µs. La session de calibration C0 tranche les deux inconnues **avant**
  d'écrire le générateur de production.
- **C(K10, 1 s)** : hors de portée du CPU seul, sauf si la constante descend au niveau du modèle GEN (1,6 µs) dans
  le scénario optimiste : sur une trame de 60 k sites, c\* vaut 0,9 à 1,6 µs en pessimiste et 1,8 à 3,3 µs en
  optimiste. La voie retenue est le GPU pour le catalogue, puis pour l'étage G de la tour.
- **Trames brutes entières (régime secondaire publié)** : K5 à 2,82 M boules mesurées (22,9 par site) ; le CPU seul
  n'y tient 1 s que dans les scénarios favorables. K10 brut (≈ 11,4 M boules, estimé) exige le GPU pour le
  catalogue et pour la tour.
- **100 ms** : cible de recherche. Budget par étage publié, aucune promesse.
- **RSS** : ≤ 0,4 Go à K5 et ≤ 1,2 Go à K10 sur trame sans sol ; ≤ 0,7 Go et ≤ 2,5 Go sur trame brute (v9 : 4,5 à
  6,4 Go à K10 sans sol).
- **Code** : ≈ 12 k lignes de produit CPU, 2,5 k GPU et 8 k de tests (v9 : 33,5 k + 33,8 k).

---

## 1. Des constats d'audit aux exigences d'architecture

| Constat v9 (lentille) | Exigence v10 | Mécanisme de contrôle |
| --- | --- | --- |
| Générateur à 0,44 n² sur amas, cubique sur coquilles (L03, L07, L13) | générateur par centres, linéaire en la sortie sur les familles adverses | porte de coût sur compteurs déterministes à 8k/16k/32k (§ 12.7) |
| Triple recensement, sceau 1/64 (L02, L12) | le catalogue recense une fois et publie ses populations en CSR | aucun recensement hors de `catalogue/` (`check_v10.py`) |
| 3 index, 4 types de clé, 2 espaces de rangs (L12) | un index par type de requête, chacun avec un seul propriétaire : `LeafOracle` pour les centres rationnels (tour), `SiteTree` pour les requêtes entières (points, témoin) ; un espace `SiteIdx` ; une identité de boule (S\*) | identifiants forts ; `check_v10.py` refuse une requête rationnelle hors de `LeafOracle` |
| 38 leviers, défaut différent de la configuration mesurée (L12) | liste blanche de paramètres publics ; constantes internes dans `tuning.hpp`, chacune avec un reçu | `check_v10.py` |
| Voie de tour choisie par le nombre de fils (L04, L12) | un algorithme ; statut et sorties indépendants de W (R1, R7) | portes W1/W2/W8 (W48 sur G4), y compris sous budget serré |
| 12 sites de création de fils (L12) | un `sched::Pool` par `Session` | `check_v10.py` |
| Mutants compilés dans le produit, 21 sites morts (L06, L12) | copies mutées au configure ; le configure échoue si un extrait a disparu | `cmake/mutants.cmake` |
| 224 o par boule, RSS 6,4 Go à K10 (L04, L07, L12) | rangs de niveaux `u32`, niveau exact recalculé depuis S\*, budget d'octets § 5.6 | porte de budget (§ 12.9) |
| Statut dépendant de l'ordonnancement (critique M5) | budget logique déterministe (§ 4.4) | porte « statut identique à W1 et W8 sous budget serré » |
| 10 enums de statut (L12) | 5 statuts, raisons énumérées, `invalid_input` à la seule frontière | table X-macro unique |
| Clustering hors moteur, sur cofaces Gabriel (E5) (L01, L06, L08, L14, L15) | tête C++ lisant la tour ; porte E5 ; porte K=1 = HDBSCAN hors égalités | § 12 |
| Angle mort Kmax−1 et Kmax (audit B, critique B2) | juge J-KM2 avec transfert de restriction | § 12.5 |
| CI rouge 40 fois, lecteur d'archives dans la CI (L06, L12) | CI mesurée, rouge = arrêt, vérification de forme des seuls reçus du commit | § 13 |
| Sonde v1 → v30 (L05, L12) | un schéma additif `mhgp10_run_v1` (EVAL) | § 15 |
| Reçus sous `/tmp` perdus, KITTI versionné (L15) | reçus hors `/tmp`, depuis un commit poussé ; aucun octet de données | § 14, § 16 |
| Chronomètre de composant présenté comme contrat (L06, L07) | contrat B2 résident sur les 30 trames d'EVAL, B0 froid publié à côté | § 15, EVAL § 12 |
| Contrôleur G4 perdu, VM restée RUNNING (incident du 27 août) | session persistante, reprise sans redémarrage | § 15.3 |
| README et PASSATION en journal (L12, L15) | README ≤ 80 lignes, PASSATION ≤ 300 lignes réécrite | `check_v10.py` |

---

## 2. Arborescence

```text
morsehgp3D_v10/
  README.md                  état courant, construction, commandes (≤ 80 lignes)
  PASSATION.md               acquis, carte, chantiers ouverts (≤ 300 lignes, réécrite)
  CMakeLists.txt
  cmake/
    run_expect.cmake         port épinglé de v9 (code exact, signal refusé, EXPECT_LINE même exécution)
    gates.cmake              mhgp10_gate(), labels, planchers
    mutants.cmake            copies mutées au configure, validation des extraits, enveloppe « code 4 »
    warnings.cmake           -Wall -Wextra -Wpedantic -Werror, refus de -ffast-math et -Ofast
  include/mhgp10/
    mhgp10.hpp               EN-TÊTE PUBLIC UNIQUE
    mhgp10_c.h               ABI C
  src/
    core/     ids.hpp, status.hpp, reasons.def, result.hpp, buffer.hpp, csr.hpp, budget.hpp/.cpp,
              alloc.hpp (déclaration seule), tuning.hpp
    sched/    pool.hpp/.cpp, primitives.hpp (scan, tri stable, concaténation par ordinal)
    arith/    wide.hpp (U192/U256/U320 signés et non signés), level.hpp, predicates.hpp (MHGP10_HD),
              zonogon.hpp (lemme Z, MHGP10_HD), widths.hpp (static_assert)
    cloud/    prepare.cpp (validation u18, tri Morton, multiplicités), site_tree.hpp/.cpp (requêtes ENTIÈRES)
    catalogue/ center_tree.cpp, leaf_enum.hpp (HD) + .cpp, shells.cpp (voie rare U256), ranks.cpp,
              leaf_oracle.hpp/.cpp (lemme O), euler.cpp (invariant I2), catalogue.hpp (contrat)
    tower/    prepare.cpp (P), resolve.hpp (HD) + .cpp (G), minima.cpp (M), edges.cpp (T1), kruskal.cpp (T2),
              intervals.cpp (Q), quotient.cpp (Gordan, U256), invariants.cpp, tower.hpp (contrat)
    points/   dendrogram.hpp (contrat), from_tower.cpp, mreach.cpp (témoin), zhat.cpp
    head/     condense.cpp, select.cpp, fill.cpp, heads.cpp, head.hpp (contrat)
    api/      session.cpp, build.cpp, c_abi.cpp
    io/       canon.cpp (sérialisations canoniques), sha256.cpp (portable + SHA-NI), merkle.cpp, u32le.cpp
    gpu/      (V10-4b) *.cu, device_session.cpp ; stub sans CUDA
  alloc/
    alloc_std.cpp            définition produit de mhgp10::detail::raw_allocate (sans initialisation)
  cli/mhgp10.cpp             tower | cluster | digest
  bench/
    probe.cpp                LA sonde (lignes mhgp10_run_v1) ; lie tests/judges
    families.hpp             familles v4/v9 portées bit à bit
    proto/cble.cpp           prototype L13 commis tel quel (sha256 d'origine 9dab902a…), pour la calibration C0
    synthetic/               banc de clustering (EVAL, CLUSTER)
    run_campaign.py
  python/mhgp10/__init__.py  liaison ctypes + numpy
  tests/
    support/alloc_poison.cpp définition de test de raw_allocate (empoisonnement 0xA5)
    bigint/                  BigInt/BigRat de test, volontairement lents
    unit/  oracle/  judges/  fixtures/  gates/  head/  v9diff/  mutants/mutants.json
  experimental/              hors bibliothèque (MHGP10_EXPERIMENTAL=OFF)
  tools/
    check_v10.py  receipt.py  run_schema.py  fetch_frames.py  sync_push.sh  disk_gate.sh
  data/MANIFEST.json         trames : sha256, tailles, recette (AUCUN octet)
  docs/  SPEC_V10.md PREUVES_V10.md ARCHITECTURE.md PROTOCOLE_MESURE.md BANC.md PROVENANCE.md FAUSSES_PISTES.md
  receipts/<chantier>_<YYYYMMDD>/
  audits/
```

Hors du dossier, les fichiers suivants, créés par la seule phase V10-0 avec l'accord de l'utilisateur sur
`AGENTS.md` :

- `.github/workflows/morsehgp3d-v10.yml` ;
- `gcp-migration/v10_session.py`, `v10_worker.sh`, `v10_recover.py`, `v10_selftest.py`, `v10_target.json` ;
- une section « Ouverture v10 » dans `AGENTS.md` ;
- une section v10 dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`.

Différences avec ARCH_v1 : `SiteTree` ne répond plus qu'aux requêtes entières ; les requêtes à centre rationnel
passent par `catalogue/leaf_oracle` ; `tower/` suit les étages de TOWER_v1 ; `alloc/` et `tests/support/` remplacent
le commutateur d'empoisonnement ; `bench/proto/cble.cpp` sert à la calibration ; `disk_gate.sh` est nouveau.

---

## 3. Couches et dépendances

```text
L0  core, sched            (sched dépend de core)
L1  arith                  (core)
L2  cloud                  (core, sched, arith)         SiteTree : requêtes entières seulement
L3  catalogue              (L0–L2)                      CenterTree transitoire ; LeafOracle vit jusqu'à la fin de Q
L4  tower                  (L0–L3)                      seul résolveur du dépôt
L5  points                 (L0–L4)                      mreach n'utilise que L0–L2
L6  head                   (L0–L1, points/dendrogram.hpp seulement)
L7  api                    (tout)                       seule couche qui voit PointId
--  io                     (core, arith, api) — bibliothèque séparée, jamais liée par L0–L6
--  gpu                    (backend de L3/L4, mêmes en-têtes HD)
--  alloc                  une définition de raw_allocate choisie à l'édition de liens
```

Règles, vérifiées par `check_v10.py` sur les `#include` et les symboles :

- une couche n'inclut que des couches inférieures ; `head/` ne voit que `points/dendrogram.hpp` ;
- `src/` n'inclut jamais `tests/`, `bench/`, `experimental/`, `audits/` ;
- une seule définition de `resolve` (dans `tower/`) ; `points/` et `head/` n'appellent aucune MEB ;
- une requête à centre rationnel (`knn_closed`, `lookup`, `nearest_strict_intruder`) n'existe que dans
  `catalogue/leaf_oracle` ;
- chaque couche a un en-tête contrat ; l'implémentation est en `.cpp` ; les fonctions `MHGP10_HD` vivent dans
  `predicates.hpp`, `zonogon.hpp`, `leaf_enum.hpp` et `resolve.hpp`, sans conteneur standard ni allocation.

Pourquoi deux index et pas un : le `SiteTree` d'ARCH_v1 répondait aussi aux centres rationnels, ce qui exigeait
un élagage boîte contre boule ouverte à environ 2^191 (critique M3-b). Dans GEN et TOWER, les requêtes rationnelles
se font sur la liste certifiée de la feuille du centre : m ≤ 24 sites, recensement exact en i128, **sans élagage**
(lemme O, P18). Le `SiteTree` ne sert qu'à des requêtes en un site entier : entrées de mreach, ẑ, remplissage du
bruit. Ses distances sont entières (d² < 2^38, i64) et son élagage est trivialement exact. Si un jour une requête
rationnelle doit élaguer un pavé, le lemme P3c (§ 17.5) donne un test exact en i128.

---

## 4. Types fondamentaux (`src/core/`)

### 4.1 Identifiants

```cpp
namespace mhgp10 {
using u8 = std::uint8_t; using u16 = std::uint16_t; using u32 = std::uint32_t; using u64 = std::uint64_t;
using i32 = std::int32_t; using i64 = std::int64_t;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;
enum class PointId   : u32 {};   // identité externe ; seule api/ la manipule
enum class SiteIdx   : u32 {};   // rang des positions DISTINCTES dans l'ordre de Morton
enum class BallIdx   : u32 {};   // rang d'une boule dans l'ordre canonique (rang de niveau, S*)
enum class LevelRank : u32 {};   // rang dense des niveaux exacts distincts (0 = niveau nul)
enum class NodeIdx   : u32 {};   // nœud d'une forêt d'ordre K
enum class CellIdx   : u32 {};   // cellule (boule, K) de l'atlas (TOWER § 4)
using Order = u8;                // K ∈ [1, kmax], kmax ≤ 10 ; kcat ≤ 12 dans les juges
inline constexpr u32 kNone = 0xFFFFFFFFu;
}
```

Tous les index internes sont `u32`, y compris les décalages de CSR du catalogue : la critique a relevé que GEN
utilise `u64` pour `ids_begin`. Un dépassement est détecté **avant** l'écriture et rend
`resource_exhausted/index_overflow_u32`. Marge au pire cas publié : trame brute K10, ≈ 11,4 M boules × 8,1
identifiants ≈ 92 M, loin de 2^32. Uniforme 32k K10 : 13,5 M boules, ≈ 130 M identifiants.

### 4.2 Niveaux

- Un niveau est un rayon au carré rationnel exact. Il n'est stocké ni par nœud, ni par contribution, ni par boule :
  on stocke un `LevelRank`. `level_rep[rank]` désigne la boule représentante ; son niveau exact se recalcule depuis
  S\* (50 à 100 ns).
- Tri (GEN § 5.4) : clé `double` à erreur relative ≤ 3·2^-53, tri parallèle par (clé, S\*), bandes floues
  réparées par comparaison exacte U320, rangs denses. La clé `double` est **transitoire** : libérée après le tri.
  La tête recalcule λ depuis le niveau exact d'un rang, à la demande.
- Rang 0 : niveau nul (boules de rayon nul des sites, TOWER § 2.3).
- Sortie publique : `level_value(rank)` rend le rationnel exact non réduit ; la réduction canonique est faite par
  `io/canon.cpp`, hors chronomètre.

### 4.3 Tampons, CSR, allocation sans commutateur

```cpp
namespace mhgp10::detail { void* raw_allocate(std::size_t bytes); void raw_release(void*) noexcept; }
template <class T> class Buffer;   // sans initialisation par défaut ; octets comptés par MemoryBudget
template <class T> struct Csr { Buffer<u32> off; Buffer<T> val; std::span<const T> row(u32) const; };
```

`raw_allocate` est **déclarée** dans `src/core/alloc.hpp` et **définie** hors de `src/`, en deux endroits :

- `alloc/alloc_std.cpp` : `operator new` aligné, sans initialisation. C'est la définition liée par les exécutables
  produit, la sonde et la bibliothèque partagée Python ;
- `tests/support/alloc_poison.cpp` : même allocation suivie d'un remplissage à 0xA5. C'est la définition liée par
  les exécutables de tests.

CMake choisit l'objet à l'édition de liens. `check_v10.py` vérifie que ce symbole est défini dans ces deux fichiers
et nulle part ailleurs. Ce mécanisme remplace l'empoisonnement « en build de test » d'ARCH_v1, qui aurait exigé un
`#if` interdit par le § 9.4 (constat mineur). Le code de la bibliothèque testée est ainsi, octet pour octet, celui
du produit ; seul l'allocateur lié diffère.

### 4.4 Budget mémoire déterministe (correction de M5)

**Défaut d'ARCH_v1.** Il y en avait deux :

- les tampons étaient réservés par tâche. À W = 48, les réservations concurrentes dépassent celles de W = 1, si bien
  que la même entrée pouvait rendre `resource_exhausted` à W48 et `ok` à W1 ;
- le budget par défaut (80 % de la RAM) dépendait de la machine.

**Définitions.**

- `SessionConfig::data_budget_bytes = B` : plafond des **octets logiques**. Ce sont les tableaux dont la taille est
  une fonction déterministe de (entrée, paramètres) : catalogue, oracle de feuilles, tour, transitoires d'étage. La
  valeur 0 signifie « aucun plafond logique ». C'est le défaut, et il ne dépend plus de la machine.
- **Frais fixes de session** F(W) = W × (A + C) + F0. A est l'arène de travail d'un ouvrier, bornée statiquement
  par `M_hard = 128` et `kcat ≤ 12` (static_assert). La table H d'une feuille de 128 sites, 128² masques de
  128 bits, y occupe 256 Kio ; A vaut donc 1 Mio. C est la taille d'une tranche de sortie (256 Kio). F0 est la
  mémoire du pool. F(48) ≈ 60 Mo. `Session::open` réserve F(W) **hors de B** et le publie. Si F(W) dépasse la RAM
  disponible, il échoue d'emblée, avant toute entrée, par `resource_exhausted/session_overhead`.
- **R7** : aucune réservation logique dans une région parallèle, sauf par le **pool de tranches**.
  - Les tableaux dont la taille est connue après comptage sont réservés dans le fil principal, entre deux étages
    (compter → réserver → remplir).
  - Les sorties de taille inconnue (émissions des feuilles, représentants de la tour) s'écrivent à la suite dans la
    tranche courante **de l'ouvrier**, de C octets, tirée d'un pool. Une tâche note (ouvrier, décalage, longueur) de
    chacun de ses segments. En fin d'étage, la concaténation par ordinal (R2) recopie ces segments dans un tableau
    réservé par le fil principal, puis rend les tranches au pool.
  - Chaque tâche ajoute au compteur logique les octets **qu'elle a écrits**. Le pool compte les octets
    **physiques**.

**Règle d'issue d'un étage s.** L_{<s} désigne le total logique des étages antérieurs, déjà acquis.

1. **Aucune annulation à l'intérieur d'un étage.** Chaque tâche va à son terme ou à sa faute. Une tâche en faute
   (invariant, arène dépassée) garde les octets écrits jusqu'à la faute. Ce nombre est une fonction de la tâche
   seule, puisque la tâche est une fonction pure de ses entrées.
2. M_s est la somme, sur toutes les tâches de l'étage, des octets écrits. M_s est déterministe.
3. Si L_{<s} + M_s > B, l'issue est `resource_exhausted/memory_budget` (étage s). Sinon, s'il y a une faute de tâche,
   l'issue est celle du plus petit (K, ordinal). Sinon, on passe à l'étage suivant.
4. **Refus anticipé.** Pendant l'étage, le pool refuse une tranche seulement si ses octets physiques dépassent
   B − L_{<s} + W·C. Un ouvrier n'a qu'une tranche partielle, et les octets de toute tâche en vol sont au plus sa
   contribution finale à M_s. À tout instant, physique ≤ M_s + W·C. Un refus implique donc L_{<s} + M_s > B,
   c'est-à-dire l'issue 3. L'étage peut alors s'arrêter tout de suite : son issue est déjà certaine.

**Lemme P19.** Si `Session::open` a réussi, l'issue (statut, raison, étage, K, ordinal) est une fonction de
(multiensemble d'entrée, paramètres, B, A, C). Elle ne dépend ni de W ni de l'ordonnancement.

Preuve : les points 1 à 4 ci-dessus. Le dépassement d'arène (`resource_exhausted/scratch_arena`) est une propriété
de la tâche seule, puisque A ne dépend pas de W.

**Échec d'allocation réelle** (RAM épuisée, B = 0) : `std::bad_alloc`, attrapé à la frontière `api/`, rend
`resource_exhausted/allocation_failed`. Cette issue est déclarée **non déterministe** et exclue de R1. Les portes
fixent donc toujours un B explicite.

**Porte (§ 12.3)** : même entrée, même B serré (choisi pour échouer à l'étage de la tour) ; W ∈ {1, 2, 8} en local et
48 sur G4 ; lignes de statut JSON identiques octet pour octet.

**Honnêteté du compte** : pic logique compté ≥ 85 % du pic RSS au-delà du socle du processus et de F(W)
(§ 12.9). Les frais F(W) sont publiés à part.

### 4.5 Statuts et raisons (`status.hpp`, `reasons.def`)

```cpp
enum class Status : u8 { ok, invalid_input, unsupported_degeneracy, resource_exhausted, invariant_violated };
```

| Statut | Raisons | Émetteur |
| --- | --- | --- |
| `invalid_input` | `empty_input`, `size_mismatch`, `coordinate_out_of_domain`, `duplicate_point_id`, `kmax_out_of_range`, `k_out_of_range`, `parameter_out_of_range`, `fp_environment` (mode d'arrondi ≠ `FE_TONEAREST` à `Session::open`) | `api/` seulement |
| `unsupported_degeneracy` | `shell_quotient_budget` (coquille u > 16, boule publiée), `duplicate_positions` (transitoire, § 5.1) | catalogue, tour |
| `resource_exhausted` | `memory_budget`, `scratch_arena`, `session_overhead`, `allocation_failed`, `index_overflow_u32`, `wide_leaf` (m > 128), `device_unavailable` | toutes couches |
| `invariant_violated` | `arith_guard`, `census_mismatch`, `canonical_order_duplicate`, `rank_order`, `csr_bounds`, `euler_mismatch` (I2), `missing_key` (I5), `root_count` (I6), `forest_identity` (I7), `descent_not_decreasing` (I3), `pointer_not_strict` (I4), `window_empty` (I9), `vertical_naturality` (I8), `vertical_closed_cut` (I10), `nested_parallelism`, `leaf_unsplittable` | toutes couches |

- Pas d'exception à travers une frontière de couche. `MHGP10_CHECK(cond, Reason::x)` rend `invariant_violated`
  sans `assert`.
- Les `catalogue_incomplete/*` de TOWER sont des fautes de notre générateur, pas des propriétés de l'entrée : ce
  sont des raisons de `invariant_violated`.
- Le succès se publie `ok`, avec les champs `exactness = relative_to_catalogue`, `euler_checked_up_to`,
  `weighted_sites`, `extended_balls`. Jamais `exact` : la règle du dépôt s'applique.

### 4.6 Publication transactionnelle

`build_tower` rend `Result<Tower>`. L'objet n'existe que si tout a réussi : aucun ordre partiel, aucun préfixe.
L'issue d'un échec suit la règle du § 4.4 :

- l'étage le plus tôt en échec ;
- dans cet étage, `memory_budget` d'abord, puis la faute de tâche de plus petit (K, ordinal) ;
- à égalité, la plus petite raison dans l'ordre de `reasons.def`. Les tampons de travail sont libérés avant le retour.

---

## 5. Contrats entre couches

Les budgets d'octets sont des **contraintes** de conception. Chaque sous-système choisit sa disposition mais doit les
tenir (porte § 12.9).

### 5.1 Entrée et `SiteTable` (`cloud/`)

**Entrée LiDAR** (constat mineur). Les trames sont définies par EVAL § 12.1.

- Entrée = les **retours** du `.bin`. Trame brute : tous les retours. Trame sans sol : tous sauf ceux que le masque
  Patchwork++ épinglé classe « ground ».
- `PointId` = indice du retour dans le `.bin` (u32).
- Quantification au pas de 1 mm, origine au capteur moins 2^17 mm, arrondi au pair sur la valeur float32 exacte.
  Un retour hors de [0, 2^18) refuse la trame, et le refus est compté.
- **Site** = position quantifiée distincte. **Poids** = nombre de retours du site. Table site → `PointId` en CSR
  triée par `PointId` croissant. **Tous** les `PointId` sont conservés : on n'en choisit aucun.
- Forme canonique : l'identité d'un site est sa position. `id_digest` hache les listes de `PointId` triées.
- Mesuré : la trame brute 08/000000 compte 123 389 retours et 123 389 sites, donc aucune fusion à 1 mm (lu ici dans
  le manifeste v8 et par `numpy.unique`). Les trames sans sol 000000, 000100 et 000200 n'ont aucune fusion non plus
  (v8 R2).

```cpp
struct SiteTable {                    // ordre de Morton 54 bits = SiteIdx
  Buffer<u32> x, y, z;                // u18
  Buffer<u32> weight;                 // ≥ 1
  Csr<PointId> ids;                   // pour api/ et io/ seulement
  u64 n_points;                       // Σ weight
  bool weighted;                      // ∃ weight ≥ 2
};
class SiteTree {                      // arbre k-d implicite sur l'ordre de Morton, feuilles ≤ 16 (≈ n/8 nœuds × 32 o)
 public:                              // requêtes en un POINT ENTIER seulement ; d² < 2^38, i64 exact
  u64 kth_distance(Point q, u64 k) const;                    // D_k(q), poids compris
  void knn(Point q, u32 k, std::span<SiteIdx> out) const;    // départage par SiteIdx
  void within(Point q, u64 r2, Buffer<SiteIdx>& out) const;
};
```

**Multiplicités : livraison par étapes (D-05 révisée, critique M4).**

- L'objet v10 est défini sur le multiensemble : L_K compte les copies. C'est la spécification (§ 2 : les doublons
  sont agrégés en sites munis de multiplicités). La v9 refusait, et la famille filaments 32k produit déjà un doublon
  à 1 mm.
- La sémantique pondérée de TOWER (fenêtre basse p_w + q − 1, avec q compté en **positions** ; boules de rayon nul
  synthétisées depuis la table des poids ; quotient sur t-multiensembles) est validée par prototype :
  9 fixtures sur 9 et 300 nuages sans désaccord. Elle n'est pas encore prouvée. Ses obligations sont P2w, P5w, P7w et
  P17 (§ 19).
- **Tant que PO-T16 (P5w) n'est pas `proved_here` et que la T2 pondérée (≥ 200 sites de poids ≥ 2) n'est pas verte,
  une entrée à doublons rend `unsupported_degeneracy/duplicate_positions`.** Ce refus est explicite, compté et testé.
  Ce n'est jamais un calcul non jugé. La porte de sortie de V10-2 lève ce refus. Coût pour les contrats : nul,
  puisqu'aucune trame mesurée n'a de doublon à 1 mm.
- Le différentiel v9 n'existe que sur des entrées de poids 1 : l'exporteur refuse les autres.

### 5.2 `catalogue/` → `Catalogue` et `LeafOracle`

Objet (GEN § 1) : boules critiques bien centrées. `q_min ∈ {2,3,4}` est compté en positions sur la coquille
complète. Admission adm_k(b) :

- p + q_min ≤ k + 1 si aucune position de la coquille n'a un poids ≥ 2 ;
- p ≤ k − 1 sinon (sur-ensemble sûr, drapeau `weighted_shell`).

**Identité** : S\*, le plus petit support de cardinal q_min dans l'ordre lexicographique des `SiteIdx`. S\*
détermine la boule (GEN § 1.2). Ordre canonique : (rang de niveau, S\*). Son injectivité est l'obligation P16 ; elle
est vérifiée au balayage des rangs et un doublon rend `invariant_violated/canonical_order_duplicate`.

```cpp
struct Catalogue {                    // SoA, ordre canonique
  Buffer<LevelRank> rank;             // ........................................................ 4 o
  Buffer<u32> ids_off;                // CSR dans ids : I trié puis U trié (u32, garde de dépassement) 4 o
  Buffer<u8> n_interior;              // |I| en positions (≤ kcat − 1) ................................ 1 o
  Buffer<u8> n_shell;                 // |U| en positions (≤ 128) ..................................... 1 o
  Buffer<u8> qflags;                  // q_min (2 bits) | étendue | pondérée ........................... 1 o
  Buffer<SiteIdx> ids;                // ............................................. 4 o par identifiant
  Buffer<BallIdx> level_rep;          // par rang ............................................ ≤ 4 o par boule
  ExtendedSide ext;                   // coquilles étendues : S* en positions dans U (rare, ≈ 0,04 %)
  Order kcat;
};
```

Octets par boule : 11 fixes + 4 par identifiant + ≤ 4 de table des niveaux. Avec les comptes mesurés d'identifiants
par boule (GEN § 7), cela donne ≈ 34 o à K5 LiDAR (4,63 identifiants) et ≈ 48 o à K10 (8,10). La critique avait
raison : les 24 o d'ARCH_v1 et les 30,5 et 44,4 o de GEN oubliaient la table des niveaux.

**`LeafOracle`** (GEN § 8, lemme O = P18) : feuilles terminales, énumérées ou non, triées par code de Morton fin de
leur coin, listes certifiées L(Q) en CSR, et CSR feuille → boules centrées dans la feuille.

- API exacte : `locate(c)`, `knn_closed(c, k ≤ kcat)`, `nearest_strict_intruder(c, r², F)`, `lookup(c, r²)`.
- Mémoire : Σm ≈ 12,2 M identifiants (49 Mo) à K5 LiDAR, 26,4 M (106 Mo) à K10, plus les codes (≈ 9 Mo).
- Durée de vie : de la fin du catalogue à la fin de l'étage Q de la tour, puis libéré. Les entrées C∩X de tous les
  K sont calculées en Q.
- Le `CenterTree` et ses listes non terminales sont libérés à la fin de l'énumération.

### 5.3 `tower/` → `Tower` (TOWER_v1)

La sémantique est celle de v7/v9 : bloc par boule, fenêtre [max(1, p_w + q − 1), min(kmax, W, p_w + u_w)], lots
atomiques par niveau exact, naissances, continuations à contributions datées, multifusions N-aires, verticales à la
coupe fermée, une racine finale par K, quotient de Gordan sur les coquilles étendues ou pondérées.

L'algorithme suit les étages de TOWER :

- **P** : préparation, atlas, tables locales ;
- **G** : `resolve(K, F) → cellule terminale`, pur et parallèle ;
- **M** : minima fixes par sauts de pointeurs ;
- **T1** : hyperarêtes datées, préfiltre de forêt couvrante minimale ;
- **T2** : Kruskal par plateaux, séquentiel par ordre, les ordres en parallèle ;
- **Q** : index d'intervalles, ancres, contributions, verticales, entrées C∩X.

`OrderForest` suit TOWER § 4.7 : rang, CSR des parents, successeur, image verticale, intervalle de feuilles, index
d'intervalles, ancres par cellule, contributions, `entry_node` et `entry_level` par site.

Budget : ≤ 81 o persistants par boule au-delà du catalogue, pic ≤ 100 o par boule avec T1–T2 exécutés par moitiés
d'ordres (TOWER § 11.3).

### 5.4 `points/` → `PointDendrogram` (contrat de la tête)

```cpp
struct PointDendrogram {              // arbre N-aire de composantes, points en feuilles
  LevelTable levels;                  // niveaux distincts de CET arbre : rang → (exact, double)
  Buffer<u32> node_rank;  Csr<u32> children;  Buffer<u32> parent;      // plateaux jamais binarisés
  Buffer<u32> point_node;             // par point, dans l'ordre d'entrée de l'API (équivariant, R1-b)
  Buffer<u32> point_rank;             // niveau d'entrée : d_K(x)² (C∩X) ou distance-cœur (mreach)
  Buffer<u32> point_weight;           // multiplicité
};
```

Il y a deux producteurs.

- **`from_tower.cpp`** lit `entry_node` et `entry_level` de l'ordre K, calculés par l'étage Q (un seul résolveur),
  puis fusionne les rangs des nœuds et les niveaux d'entrée entiers par comparaison exacte (d² × dén contre num,
  ≤ 2^153, U192). Il ne calcule aucune MEB.
- **`mreach.cpp`** calcule les distances-cœurs par `SiteTree::kth_distance`, puis l'arbre de Kruskal exact de
  l'atteignabilité mutuelle par Borůvka sur `SiteTree` (CLUSTER § 2.3), avec des plateaux groupés.

### 5.5 `head/` → `Clustering`

Les têtes (T1 EOM-densité, T2 tranche γ, T3 § 9.1 pondéré), la condensation exacte, les sélections et les politiques
de bruit relèvent de CLUSTER_v1 § 3–7. Les hyperparamètres de la méthode figurent dans la liste blanche
`ClusterParams` (§ 9.1).

### 5.6 Budget d'octets consolidé (normatif)

Pour chaque colonne : octets par boule, puis total.

| Poste | K5 sans sol (1,31 M boules) | K10 sans sol (5,51 M) | K5 brute (2,82 M) | K10 brute (≈ 11,4 M, estimé) |
| --- | ---: | ---: | ---: | ---: |
| catalogue (§ 5.2) | 34 o — 45 Mo | 48 o — 265 Mo | 34 o — 96 Mo | 48 o — 550 Mo |
| oracle de feuilles | 44 o — 58 Mo | 21 o — 115 Mo | ≈ 27 o — 75 Mo | ≈ 20 o — 230 Mo |
| tour, pic (§ 5.3) | ≤ 100 o — 131 Mo | ≤ 100 o — 551 Mo | ≤ 100 o — 282 Mo | ≤ 100 o — 1,14 Go |
| sites (table, `SiteTree`) | — 3 Mo | — 3 Mo | — 8 Mo | — 8 Mo |
| **pic logique compté** (budget B) | **≈ 237 Mo** | **≈ 0,93 Go** | **≈ 0,46 Go** | **≈ 1,93 Go** |
| frais de session F(48) (hors B, § 4.4) | ≈ 60 Mo | ≈ 60 Mo | ≈ 60 Mo | ≈ 60 Mo |
| **RSS budgété** (porte, socle du processus et F(W) compris) | **≤ 0,4 Go** | **≤ 1,2 Go** | **≤ 0,7 Go** | **≤ 2,5 Go** |
| v9 mesuré (RSS) | — | 4,5–6,4 Go | — | ≈ 10 Go (selon la critique, non revérifié ici) |

Le pic transitoire de S3 (catalogue en tranches plus catalogue trié, soit environ deux fois le catalogue, plus
l'oracle) reste sous le pic de la tour : par exemple 0,65 Go contre 0,93 Go à K10 sans sol.

L'oracle de feuilles des trames brutes est extrapolé depuis les nœuds mesurés (1,55 M feuilles × 11,5). K10 brute est
extrapolé à 92 boules par site ; il sera mesuré à V10-1.

---

## 6. Pipeline et modèle de coût

### 6.1 Étages

| Étage | Couche | Travail | Parallélisme | Backend GPU |
| --- | --- | --- | --- | --- |
| S0 préparer | cloud | validation, tri Morton par base, sites et poids, `SiteTree` | tri parallèle | non (≈ 2 ms) |
| S1 arbre de centres | catalogue | gardes, dominateurs, coupe D′, k-DOP, découpe (GEN § 5.2) | frontière de 64W nœuds, puis une tâche par nœud | oui (BFS par niveau) |
| S2 feuilles | catalogue | paires, triplets (lemme Z), quadruplets, recensement, coquilles étendues (GEN § 5.3) | une tâche par feuille, arène locale | oui (un warp par feuille si m ≤ 32 ; m > 32 rendu au CPU, compté) |
| S3 rangs | catalogue | clé `double`, tri par échantillonnage, réparation exacte des bandes, rangs | tri parallèle | éventuellement |
| S4 P | tower | atlas, tables locales, invariant I2 | par boule | non |
| S5 G | tower | `resolve` des représentants | par (K, représentant) | oui (TOWER § 13) |
| S6 M | tower | minima fixes par sauts de pointeurs | par cellule | oui |
| S7 T1 | tower | hyperarêtes, préfiltre de forêt couvrante minimale (Borůvka) | par K et par arête | oui |
| S8 T2 | tower | Kruskal par plateaux | séquentiel par K, ordres en parallèle | non (plafond d'Amdahl) |
| S9 Q | tower | index d'intervalles, ancres, contributions, verticales, entrées C∩X | par requête | oui |
| S10 sortie | tower | CSR finaux, invariants O(sortie), libérations | par K | non |
| S11 points | points | dendrogramme de points (lecture des entrées de Q) | par point | non |
| S12 tête | head | condensation, sélection, remplissage | O(n log n) | non |

### 6.2 Volumes de référence (mesurés)

| Entrée | Sites | Boules K5 | Boules K10 | Source |
| --- | ---: | ---: | ---: | --- |
| 08/000000 sans sol | 39 885 | 1 306 696 (32,8 par site) | 5 512 670 (138) | R22 (v9) |
| 08/000100 sans sol | 35 551 | 1 095 926 | 4 383 302 | EVAL § 12.1 |
| 08/000200 sans sol | 45 845 | 1 407 885 (30,7) | 5 483 320 (119,6) | `crit_ARCH/lidar02_K5_dom3_M16.json`, GEN § 10.1 |
| **08/000000 brute** | **123 389** | **2 822 052 (22,9)** | ≈ 11,4 M (≈ 92 par site, **estimé**) | `crit_ARCH/raw00_K5_dom3_M16.json` |
| uniforme 8k / 16k / 32k | — | 0,59 / 1,23 / 2,53 M | 3,09 / — / 13,49 M | cble (L13), TOWER § 11.1 |
| huit amas 32k | — | 2,31 M | — | L13 |

Le catalogue est n·Θ(K³) à petite constante ; le LiDAR (surfaces) porte 2,3 à 3,5 fois moins de boules par site que
l'uniforme. La trame brute porte **moins** de boules par site que la trame sans sol (22,9 contre 30,7–32,8 à K5) :
le sol est une surface peu profonde. Mais elle a 3,1 fois plus de sites, donc environ 2,2 fois plus de boules.

### 6.3 Constantes : mesuré contre supposé (correction de M2)

| Quantité | Valeur | Statut | Source ou session de mesure |
| --- | --- | --- | --- |
| c_cs : catalogue K5 sans sol, cble `--dom=3 --M=16` | **14,6 µs CPU par boule** (20,53 CPU·s / 1 407 885) | mesuré sur le codespace (EPYC 7763, Zen 3), 2 fils, `nice 19`, charge ≈ 7,6 | `crit_ARCH/lidar02_K5_dom3_M16.{json,time}` |
| c_cs : catalogue K5 brute, idem | **14,9 µs** (42,09 / 2 822 052) | mesuré, même hôte | `crit_ARCH/raw00_K5_dom3_M16.{json,time}` |
| c_cs : feuille v2 de GEN, K5 / K10 | 16,1–24,1 / 21,0–22,8 µs | mesuré, deux passes, charge 5 puis 10 | `gen_probe/out/runs.jsonl` |
| artefact K5 LiDAR conservé par L13 (sans `--dom=3`) | 31,3 µs | mesuré | `out_lidar02_K5_M16.json` (L13) |
| « 18,5 µs » cité par L13 | — | **sans artefact : retiré** | — |
| c_eng : cible d'ingénierie | ≤ 4 µs | **objectif**, porte locale V10-1 sur compteurs | GEN § 10.3 |
| c_mod : modèle compteurs × coûts unitaires | 1,6–1,7 µs | **estimé, non mesuré** | GEN § 10.3 |
| ρ = coût par fil G4 (Zen 5) / coût par fil codespace (Zen 3) | [0,5 ; 1,0] | **supposé** | session C0 (§ 15.5) |
| S48 = mur(W1) / mur(W48) sur G4, 24 cœurs, SMT2 | [24 ; 34] | **supposé** (GEN prévoit 28–34) | sessions C0 et C1 |
| v9 moteur K5 : CPU·s / mur à W48 | ×41 (114,9 CPU·s en 2,77 s) | mesuré (R22) | ce n'est **pas** S48 : le temps CPU d'un fil SMT compte plein alors que le fil tourne à ≈ 60 % |
| tour v10 K5 / K10, W48 | 70–95 ms / 0,40–0,65 s | estimé (TOWER § 11.2) | V10-2 (local), C1 (G4) |
| tour v9 K5 / K10, W48 | 289–304 ms / 1 212 ms | mesuré (R22) | — |

ARCH_v1 confondait le rapport CPU·s / mur (×41, gonflé par le SMT) avec une accélération. Il supposait ×26 et
8 µs sans artefact. ARCH_v2 sépare trois grandeurs, toutes mesurables :

- la constante par fil (c1_G4 = ρ · c_cs) ;
- l'accélération de mur S48 ;
- le volume N, mesuré à l'avance.

Le mur du catalogue vaut alors c1_G4 · N / S48.

### 6.4 Seuils de rentabilité c\*

Pour un contrat de T secondes sur une trame de N boules, la constante locale maximale admissible est :

```text
c* = (T − T_tour − T_fixe) · S48 / (ρ · N)            (µs locaux par boule si N en millions et T en secondes)
```

On prend T_fixe = 0,05 s (préparation, rangs, sortie), et deux scénarios : **pessimiste** (ρ = 1, S48 = 24) et
**optimiste** (ρ = 0,6, S48 = 30). La plage donnée pour c\* va du T_tour haut au T_tour bas.

| Contrat, trame | N | T_tour supposé | c\* pessimiste | c\* optimiste | prototype mesuré | lecture |
| --- | ---: | --- | ---: | ---: | ---: | --- |
| K5 1 s, 08/000200 sans sol | 1,41 M | 0,10–0,30 s | 11,1–14,5 | 23,0–30,1 | 14,6 | le prototype passe en optimiste et échoue en pessimiste |
| K5 1 s, trame de 60 k sites (pire cas EVAL) | 2,0 M | 0,10–0,30 s | **7,8–10,2** | 16,3–21,3 | 14,6 | la cible d'ingénierie (≤ 4) passe dans tous les scénarios |
| K10 1 s, 08/000000 sans sol | 5,5 M | 0,40–0,65 s | 1,3–2,4 | 2,7–5,0 | 17,7–22,8 | CPU seul : seulement au niveau du modèle GEN et en optimiste |
| K10 1 s, trame de 60 k sites | 8,3 M | 0,40–0,65 s | **0,9–1,6** | 1,8–3,3 | 17,7–22,8 | GPU requis |
| K5 1 s, 08/000000 brute | 2,82 M | 0,20–0,65 s | 2,6–6,4 | 5,3–13,3 | 14,9 | CPU seul : cible d'ingénierie et tour v10 rapide requises |
| K10 1 s, 08/000000 brute | ≈ 11,4 M | 0,85–1,4 s (×2,1) | ≤ 0,2 ou négatif | ≤ 0,44 ou négatif | — | GPU requis pour le catalogue **et** pour la tour |

C'est cette table, et non un chiffre unique, qui fonde les décisions du § 18 : la calibration C0 fixe ρ et S48, la
fin de V10-1 fixe c_cs, puis la décision D-K5 s'applique.

### 6.5 Budgets par étage sur LiDAR (régime prioritaire)

Mur du catalogue sur G4, en CPU seul à W48, pour les trois hypothèses de constante. Chaque case donne le mur en
scénario pessimiste puis en scénario optimiste.

| Trame, K | N | H-proto (mesuré 14,6 µs à K5 ; ≈ 20 µs à K10) | H-eng (4 µs) | H-mod (1,6–1,7 µs) |
| --- | ---: | ---: | ---: | ---: |
| sans sol K5 (08/000200) | 1,41 M | 0,86 / 0,41 s | 0,24 / 0,11 s | 0,09 / 0,05 s |
| sans sol K10 (08/000000) | 5,5 M | 4,6 / 2,2 s | 0,92 / 0,44 s | 0,39 / 0,19 s |
| brute K5 (08/000000) | 2,82 M | 1,75 / 0,84 s | 0,47 / 0,23 s | 0,19 / 0,09 s |
| brute K10 | ≈ 11,4 M | — | 1,9 / 0,91 s | 0,81 / 0,39 s |

Totaux de bout en bout (B2) :

| Cas | Catalogue | Tour | Reste | **Total B2** | v9 mesuré (R22) |
| --- | --- | --- | --- | ---: | --- |
| K5 sans sol, H-eng, CPU | 0,11–0,24 s | 0,07–0,30 s | 0,03 s | **0,21–0,57 s** | CPU 2,18–3,00 s ; GPU chaîne 0,76–0,98 s, mur 1,5–1,9 s |
| K5 sans sol, H-proto, CPU | 0,41–0,86 s | 0,07–0,30 s | 0,03 s | **0,51–1,19 s** | idem |
| K10 sans sol, H-eng, CPU | 0,44–0,92 s | 0,40–0,65 s | 0,05 s | **0,89–1,62 s** | CPU 6,1–8,6 s ; GPU chaîne 2,3–3,0 s |
| K10 sans sol, catalogue GPU (GEN : 30–100 ms, projection) + tour CPU | 0,03–0,10 s | 0,40–0,65 s | 0,05 s | **0,48–0,80 s** | — |
| K10 sans sol, catalogue et étage G sur GPU (TOWER : tour 50–80 ms, projection) | 0,03–0,10 s | 0,05–0,08 s | 0,05 s | **0,13–0,23 s** | — |
| K5 brute, H-eng, CPU | 0,23–0,47 s | 0,15–0,65 s | 0,05 s | **0,43–1,17 s** | — |
| K10 brute, catalogue et G sur GPU | 0,06–0,20 s | 0,10–0,20 s | 0,08 s | **0,24–0,48 s** (projection) | — |

Les projections GPU de GEN et TOWER ne valent qu'après reçu G4 : la v9 a montré des écarts ×10 entre projection et
mur. Elles orientent l'ordre des travaux, elles ne fondent aucune porte.

### 6.6 Budgets de mémoire

Ce sont ceux du § 5.6 : RSS ≤ 0,4 Go (K5 sans sol), ≤ 1,2 Go (K10 sans sol), ≤ 0,7 Go (K5 brute), ≤ 2,5 Go (K10
brute), ≤ 2,5 Go (uniforme 32k K10, local). Tous tiennent sur le codespace (31 Go) et sur G4 (185 Go). L'objectif
d'EVAL de ≤ 32 o par boule n'est pas adopté (§ 0.3).

### 6.7 Budgets synthétiques (pentes et bancs)

Les portes de coût synthétiques portent sur des **compteurs déterministes** (GEN § 11.8 ; TOWER `meb_calls`,
`knn_steps`, `pointer_rounds`, `msf_edges`, `wa_queries`), identiques à 2, 8 ou 48 fils. Les temps locaux restent des
diagnostics (hôte partagé, charge 5 à 13). Temps mesurés du prototype v2, 2 fils, en local (GEN § 10.2) :

- coquilles 8k / 16k / 32k : 1,46 / 2,65 / 5,11 s (v9 : 57,5 / 179,9 / 1 438,5 s pour q3/q4 seul) ;
- filaments : 1,87 / 4,03 / 7,82 s ;
- grille 20³ / 25³ / 32³ : 5,8 à 29,6 s, soit 48 à 77 µs par boule, le pire régime dégénéré mesuré.

Le banc de clustering à K ≤ 3 et 32k (≈ 0,8 M boules) coûte, en local, de l'ordre de 10 s de catalogue avec le
prototype et environ 1 s visé.

### 6.8 100 ms à K5

Budget par étage à tenir **simultanément** :

- catalogue ≤ 40 ms, soit ≥ 35 M boules/s (projection GPU de GEN : 10–40 ms) ;
- étage G ≤ 20 ms ;
- T2 ≤ 15 ms (plafond d'Amdahl de TOWER : 7–14 ms) ;
- Q et le reste ≤ 25 ms.

C'est une cible de recherche, qui suppose un processus résident (le contexte CUDA seul coûte 121–166 ms). V10-4b
publie la décomposition mesurée contre ce budget, sans promesse.

---

## 7. Parallélisme et déterminisme

### 7.1 `sched::Pool`

```cpp
class Pool {                                      // créé UNE fois par Session : W−1 fils + l'appelant
 public:
  explicit Pool(unsigned threads);
  unsigned threads() const;
  template <class F> Status for_each(size_t n, size_t grain, F&& f);          // f(i) écrit des cases indexées par i
  template <class F> Status for_each_ordered(std::span<const u32> order, F&& f);  // lancement par coût décroissant
};
// primitives.hpp : exclusive_scan, sort_stable (ordre total exigé), concat_by_ordinal, ChunkPool (§ 4.4)
```

### 7.2 Règles normatives R1–R7

- **R1 (reformulée : invariance et équivariance).** Soit l'entrée une suite ((x_i, id_i))_i.
  - (a) Toute sortie indexée par un identifiant interne (`SiteIdx`, `BallIdx`, `LevelRank`, `NodeIdx`, `CellIdx`) et
    tout digest sont des fonctions du multiensemble {(x_i, id_i)} et des paramètres. Ils sont **invariants** par
    permutation de la suite, par W et par backend.
  - (b) Les tableaux indexés par l'ordre d'entrée de l'API (`point_node`, `point_rank`, étiquettes) sont
    **équivariants** : permuter l'entrée les permute de la même façon.
  - (c) Renommer les `PointId` par une bijection σ ne change que `id_digest` et les sorties indexées par `PointId`,
    exactement par σ.
  - (d) L'issue (statut, raison, étage) suit le lemme P19 (§ 4.4) ; `allocation_failed` est exclu.
- **R2** : un producteur parallèle écrit dans des cases indexées par un ordinal déterministe ; la concaténation se
  fait par préfixes dans l'ordre des ordinaux (GEN : ordre de Morton des tâches ; TOWER : ordre des cellules).
- **R3** : aucune réduction flottante dont l'ordre dépend de l'ordonnancement ; le flottant ne sert que de filtre
  certifié ou de mesure sommée dans l'ordre de l'arbre (tête).
- **R4** : tables de hachage pour l'appartenance seulement ; résolution par le plus petit ordinal.
- **R5** : départages par `SiteIdx` dans le moteur, jamais par `PointId` ni par ordre d'arrivée. La tête départage
  les égalités de remplissage selon la règle écrite de CLUSTER § 6 ; son résultat reste équivariant.
- **R6** : pas de parallélisme imbriqué (`invariant_violated/nested_parallelism`).
- **R7** (nouvelle) : aucune réservation logique dans une région parallèle hors du `ChunkPool` (§ 4.4).

### 7.3 Aucun fil ailleurs ; un algorithme quel que soit W

`std::thread`, `std::async`, `pthread_create` et `#pragma omp` sont interdits hors de `src/sched/`. Le GPU utilise un
flux par `Session`. L'étage T2 exécute le même code à W = 1 et à W = 48 : les ordres sont des tâches du pool. Un
balayage intra-ordre (P13), s'il est adopté après ablation, **remplace** T2 pour tous les W dans le même commit.

---

## 8. Dépendances

| Dépendance | Produit | Tests et juges | Banc | Raison ou épingle |
| --- | --- | --- | --- | --- |
| C++20, `__int128` | oui | oui | — | g++ 11.4 (VM G4 et CI) et g++ 13.3 (local) ; clang 18 pour la CI assainie |
| entiers larges maison (`arith/wide.hpp`) | oui | oui | — | U192/U256/U320, jugés contre BigInt de test (et `cpp_int` si Boost est présent) |
| BigInt/BigRat de test | non | oui | — | arithmétique volontairement autre et lente |
| Boost | non | optionnel | — | `BOOST_ROOT=build/v7_boost_gate` en local ; jamais requis en CI ; requis par l'exporteur v9 (`tests/v9diff`, hors build par défaut) |
| CUDA 12.9 | option, OFF par défaut | porte `device` | — | V10-4b ; `nvcc --fmad=false` sur les unités de filtre (P14) |
| **Python** | non | tests de la tête et du banc | oui | **CI : Python 3.12 via `actions/setup-python`** (scikit-learn 1.9.1 exige ≥ 3.11 ; numpy 2.5 exige ≥ 3.12) ; épingles égales au local mesuré : Python 3.12.1, numpy 2.5.3, scikit-learn 1.9.1. **VM G4 (Python 3.10)** : aucun test Python du banc ; les scripts de worker restent compatibles 3.10 et n'utilisent que la bibliothèque standard |

Aucune dépendance tierce dans `src/`. HGP-old n'est jamais importé ni versionné dans la v10 (licence non commerciale).

- Il peut être **exécuté** hors build, localement, comme oracle de correction sur de petits nuages : jamais en CI,
  résultats et divergences connues déclarés (clique du manuscrit contre chemin du code).
- Toute porte « reproduit la sémantique HGP-old » se réimplémente en MIT **en salle blanche depuis le manuscrit**
  (Parties I–II), jamais depuis la lecture de son code (constat mineur).

---

## 9. Discipline : un chemin, pas de levier sans ablation, pas de mutant dans le produit

### 9.1 Paramètres publics fermés (liste blanche, API corrigée)

| Struct ou argument | Champs | Remarque |
| --- | --- | --- |
| `SessionConfig` | `threads`, `backend`, `data_budget_bytes` | 0 fil = matériel ; 0 octet = pas de plafond logique (§ 4.4) |
| `TowerParams` | `kmax` | 1 ≤ kmax ≤ 10 |
| `point_hierarchy(…, Order K)`, `mreach_hierarchy(…, Order K)` | `K` | 1 ≤ K ≤ kmax (tour) ; 1 ≤ K ≤ 16 (mreach) |
| `IntrinsicDimParams` | `k` (défaut 10) | Levina–Bickel ; ajouté à la liste blanche (constat mineur) |
| `ClusterParams` | `min_cluster_size`, `z`, `selection`, `allow_single_cluster`, `noise`, `fill_radius_factor` | hyperparamètres de la MÉTHODE, préenregistrés |

`HierarchyParams` d'ARCH_v1 est supprimé : il était déclaré dans la liste blanche et jamais utilisé. Il n'y a ni
paramètre `s`, ni mode `verify`, ni levier booléen. Ajouter un champ modifie la liste blanche de `check_v10.py`, donc
un diff visible.

### 9.2 Constantes internes (`src/core/tuning.hpp`)

Ce sont M(K), `M_hard = 128`, T = 6, la réserve 3K, la stagnation 3 (GEN § 6), les grains du pool, les feuilles du
`SiteTree`, l'arène A et la tranche C. Chaque constante cite le reçu d'ablation ou de calibration qui l'a fixée. M(K)
est calibrée une fois sur G4 (session C1). `getenv` est interdit dans `src/`.

### 9.3 Alternatives : `experimental/`

Une alternative entre dans `src/` si et seulement si une ablation appariée la justifie. Le protocole est celui
d'EVAL § 12.7 :

- digests égaux ;
- blocs ABBA ;
- borne haute de l'IC du Δ relatif < 0 ;
- gain médian ≥ 2 % ;
- entrées : 30 trames de test, ou 3 + 6 trames de conception et de développement en phase de développement, plus
  8k/16k/32k pour la pente.

Elle devient alors le seul chemin, et l'ancien est supprimé dans le même commit. Un résultat neutre ou négatif prend
une ligne dans `docs/FAUSSES_PISTES.md`, avec son reçu.

### 9.4 Limites mécaniques (`tools/check_v10.py`, en CI)

- `src/` : les seules directives conditionnelles admises sont `#pragma once`, `#if MHGP10_ENABLE_CUDA` (dans `gpu/`
  seulement), `#ifdef __CUDACC__` (dans les en-têtes HD seulement, pour `MHGP10_HD`) et le refus
  `#ifdef __FAST_MATH__ / #error`. Aucun identifiant `mutant`, `inject` ni `MHGP10_TEST`.
- `mhgp10::detail::raw_allocate` est défini exactement dans `alloc/alloc_std.cpp` et
  `tests/support/alloc_poison.cpp`.
- Aucun fil hors de `src/sched/` ; aucun `getenv` ; aucune sortie standard hors de `cli/` et `bench/`.
- Graphe d'inclusion conforme au § 3 ; une seule définition de `resolve` ; aucune requête rationnelle hors de
  `leaf_oracle`.
- Fichier de `src/` ≤ 600 lignes.
- Aucune date ni étiquette de session dans les commentaires de `src/`.
- Structs publiques égales à la liste blanche.
- Chaque extrait de `tests/mutants/mutants.json` présent exactement une fois dans son fichier.
- `README.md` ≤ 80 lignes ; `PASSATION.md` ≤ 300 lignes ; chaque reçu cité existe.
- `receipts/` : extensions `json`, `jsonl`, `csv`, `txt`, `md`, `SHA256SUMS` ; au plus 2 Mo par reçu ; aucun nuage.
- `data/` ne contient que `MANIFEST.json` ; tout fichier dont le sha256 figure dans le manifeste est refusé partout
  dans le dépôt (L15-06).

### 9.5 Mutants hors produit

Format `tests/mutants/mutants.json` : `name`, `file`, `find`, `replace`, `suite` (sous-commande d'un exécutable de
tests), `expect_code` du juge (1 ou 3), `expect_line` (ligne `cause=` exacte).

`cmake/mutants.cmake` fait trois choses :

1. au configure, toujours : vérifie que chaque `find` apparaît une fois exactement, sinon `FATAL_ERROR` ;
2. avec `-DMHGP10_BUILD_MUTANTS=ON` : copie `src/`, applique le remplacement, construit la bibliothèque mutée en
   build unitaire et l'exécutable de la suite ;
3. enregistre la porte `mhgp10_mutant_<nom>`, qui lance `cmake -P run_mutant.cmake`. Cette enveloppe exécute la
   suite et **rend 4** (« mutant tué », convention du dépôt) si et seulement si la suite rend `expect_code` avec
   `expect_line`. Elle rend 0 si le mutant survit, et 3 si la suite a échoué autrement (cause différente, signal).

La porte attend le code 4. Le code 4 garde ainsi le sens qu'il a dans tout le dépôt (constat mineur) : il n'est
jamais un statut du produit.

Les pannes de ressource se testent sans injection, par le budget logique public : un B minuscule doit rendre
`resource_exhausted/memory_budget` sans sortie partielle, avec une ligne identique à W1 et à W8.

---

## 10. CMake

- `cmake_minimum_required(VERSION 3.22)` (la VM a CMake 3.22.1). Les `.cu` reçoivent `-std=c++20` par expression
  génératrice, sans `CUDA_STANDARD`.
- `CMAKE_CXX_STANDARD 20`, extensions OFF, `-Wall -Wextra -Wpedantic -Werror`. Refus de configurer avec
  `-ffast-math` ou `-Ofast`.
- Options : `MHGP10_BUILD_TESTS=ON`, `MHGP10_SANITIZE=OFF`, `MHGP10_TSAN=OFF`, `MHGP10_ENABLE_CUDA=OFF`,
  `MHGP10_BUILD_MUTANTS=OFF`, `MHGP10_EXPERIMENTAL=OFF`, `MHGP10_DATA_DIR` (cache hors Git), `MHGP10_V9_SOURCE`
  (arbre v9 à `ce8a649dd`, vide par défaut).
- Cibles :
  - `mhgp10` (statique, `src/` sans `io/` ni `gpu/`), `mhgp10_io`, `mhgp10_cuda` (option) ;
  - `mhgp10_alloc_std`, `mhgp10_alloc_poison` (objets) ;
  - `mhgp10_c` (partagée) ;
  - exécutables `mhgp10` et `mhgp10_probe` ;
  - **cinq** exécutables de tests à sous-commandes : `mhgp10_unit`, `mhgp10_oracle`, `mhgp10_judge`,
    `mhgp10_gates`, `mhgp10_v9diff` (option). Chacun est en build unitaire (une unité de traduction) pour le temps
    de CI.

```cmake
# mhgp10_gate(NAME <n> COMMAND <cible> <args...> CODE <0|1|2|3|4> [LINE "<ligne exacte>"]
#             LABELS <labels...> [TIMEOUT <s>])
# ctest -> cmake -P run_expect.cmake : code EXACT, signal = échec, LINE lue dans la MÊME exécution.
# Codes : 0 conforme ; 1 désaccord d'un juge ; 2 refus (invalid_input, unsupported_degeneracy,
# resource_exhausted — la ligne JSON de statut précise lequel) ; 3 invariant violé ou plancher non atteint ;
# 4 mutant tué (enveloppe de mutant seulement). PASS_REGULAR_EXPRESSION seul est interdit.
```

| Label | Contenu | Où |
| --- | --- | --- |
| `fast` | unitaires, fixtures, T2 réduite (§ 12.1), oracles réduits, déterminisme, tête K=1 réduite | CI, avant chaque poussée |
| `sanitize` | sous-ensemble de `fast` sous ASan/UBSan | CI |
| `gate` | toutes les portes de correction, y compris les longues (≤ 15 min à W8) | sortie de phase, nuit |
| `oracle` | T2 et oracles bornés complets | sortie de phase |
| `judge` | juges d'échelle (§ 12.4), dont J-KM2 | sortie de phase |
| `mutant` | enveloppes de mutants | hebdomadaire, sortie de phase |
| `scale8000` / `scale16000` / `scale32000` | portes exécutées réellement à cette taille, avec planchers | sortie de phase |
| `lidar` | trames du manifeste présentes dans `MHGP10_DATA_DIR` (enregistrées seulement si présentes) | sortie de phase, G4 |
| `device` | égalité hôte/appareil | G4 |

---

## 11. API C++, CLI, liaison Python

### 11.1 En-tête public unique (`include/mhgp10/mhgp10.hpp`)

```cpp
namespace mhgp10 {
enum class Backend : u8 { cpu, cuda };
struct SessionConfig { unsigned threads = 0; Backend backend = Backend::cpu; u64 data_budget_bytes = 0; };
struct Ledger;                                             // ≤ 32 champs : compteurs déterministes | temps
class Session {                                            // possède Pool, budget, arènes, contexte device
 public:
  static Result<Session> open(const SessionConfig&, Ledger* = nullptr);   // publie F(W), device_init_ms
};
struct InputView { std::span<const u32> xyz; std::span<const u32> ids; };   // 3n u18 ; n PointId
struct TowerParams { Order kmax; };
struct IntrinsicDimParams { u32 k = 10; };
Result<Tower> build_tower(Session&, InputView, TowerParams, Ledger* = nullptr);
Result<PointDendrogram> point_hierarchy(Session&, const Tower&, Order K);
Result<PointDendrogram> mreach_hierarchy(Session&, InputView, Order K);
Result<double> estimate_intrinsic_dimension(Session&, InputView, IntrinsicDimParams = {});
Result<Clustering> cluster(Session&, const PointDendrogram&, const ClusterParams&);
// Accès en lecture : Tower::order(K) → OrderView (spans), Tower::level_value(rank), Tower::population(ball, I, U)
}
```

### 11.2 CLI `mhgp10`

```text
mhgp10 tower   --in cloud.u32le [--ids ids.u32le] --kmax 5 [--threads N] [--backend cpu|cuda]
               [--data-budget BYTES] --out tower.mhgp10
mhgp10 cluster --in cloud.u32le --K 2 --source tower|mreach --mcs 45 --z 1 [--selection eom|leaf]
               [--noise abstain|bounded_fill|full_fill] --out labels.i32le
mhgp10 digest  --tower tower.mhgp10        (engine_digest, tower_digest_v10, catalogue_digest, id_digest)
```

Codes de sortie, alignés sur la convention du dépôt :

- 0 : `ok` ;
- 2 : refus, c'est-à-dire `invalid_input`, `unsupported_degeneracy` ou `resource_exhausted` ;
- 3 : `invariant_violated`.

Les codes 1 et 4 ne sont jamais émis par le produit : ils sont réservés aux juges et à l'enveloppe de mutant. La
sortie standard porte une ligne JSON `{"status":…,"reason":…,"stage":…,"order":…}`, que les portes lisent comme LINE
exacte.

### 11.3 Liaison Python minimale

ABI C (`mhgp10_c.h`, ≈ 12 fonctions, protocole compter puis remplir, CLUSTER § 8.1) et `python/mhgp10/__init__.py`
en ctypes + numpy. Pas de pybind11. La quantification u18 des données du banc est faite par le banc (EVAL § 3.8),
jamais par la bibliothèque. HDBSCAN reçoit les mêmes coordonnées quantifiées.

---

## 12. Vérification : ce qui prouve que la tour est juste et que le chronomètre est honnête

Principe : garder ce qui établit la vérité à petite taille (oracles), ce qui détecte une faute d'implémentation à
l'échelle (invariants globaux, juges d'échantillon, différentiel indépendant, mutants), et ce qui rend le chronomètre
honnête. Jamais de vérification exhaustive à l'échelle, jamais de juge O(n³) ni de tableau par paire.

### 12.1 Oracles bornés (établissent la vérité)

| Oracle | Portée et arithmétique | CI (`fast`) | Sortie de phase (`oracle`) |
| --- | --- | --- | --- |
| T2 Γ_K (port de `census_tower_oracle.hpp` v9) | partitions des K-parties à chaque coupe ouverte et fermée, chronologie (naissance, continuation, multifusion), verticales ; BigRat. **Multiensembles par copies étiquetées** : un doublon est un point distinct à la même position, et la MEB d'une partie se calcule sur ses **positions distinctes**. C'est ce qui lève le rejet des supports affinement dépendants (`support_ball` v9, `mini_t2.py`) | 200 nuages, **n ∈ [4, 9]**, K = 1..n, dont 40 avec doublons (poids ≤ 3) | ≥ 5 000 nuages, n ≤ 14, 20 % avec doublons, W1 et W8 ; planchers de TOWER § 12.1 |
| Catalogue brut | tous les sous-ensembles de ≤ 4 sites, recensement complet, kcat = 1..min(12, n) ; prédicats i128 **réécrits indépendamment** (Cramer, pas de Gram), signes nuls et quasi-bords confirmés en BigRat | 60 nuages, **n ≤ 16** | ≥ 500 nuages, n ≤ 60 |
| C∩X brut | d_K(x) et composante de L_K par Γ_K | 50 nuages, n ≤ 9 | 1 000 nuages, n ≤ 12 |
| Second langage | Python/Fraction : `tower_proto.oracle_partitions` et `mini_t2.py` (`audit_v9/L06_tests_oracles`). **Pas** `tower_degen.py`, qui est une tour prototype et non un oracle (constat mineur) | — | une fois, 500 nuages, reçu |
| HGP-old | exécuté hors build (§ 8), petits nuages, sémantique déclarée | — | optionnel, reçu |

Coûts de référence, pour fixer les distributions. L'oracle Python mesuré ici par `crit_ARCH/t2cost.py` (même
générateur, grille {0..9}³) met environ 1,7 s à n = 9 pour K = 1..5 et 3,9 s à n = 10 pour K = 1..5 ; la critique
mesure 18,6 s à n = 12 pour K = 6. La v9 en C++ (`cpp_int`) met 2,16 s pour 2 nuages à n = 14. Le coût en C++ croît
comme Σ_K C(n, K) : de n = 14 à n = 9, il est divisé par environ 30, ce qui estime la T2 CI à 200 nuages × ≈ 30 ms ≈ 6 s.
Cette estimation n'est pas une mesure : le budget est gelé à la sortie de V10-0, sur mesure (§ 13).

Générateur des nuages de la T2 : grilles {0..2}³, {0..3}³, {0..4}³ ; génériques u18 ; coins u18 ; quasi-plans ;
cosphériques ; kmax tiré dans [1, n] ; `PointId` épars et inversés ; entrée permutée.

### 12.2 Fixtures permanentes

Voir l'annexe A. Chaque fixture est un JSON gravé aux coordonnées exactes, avec sa provenance et son attendu. Toute
nouvelle contradiction devient une fixture et une ligne du registre **avant** de continuer.

### 12.3 Portes de déterminisme et d'équivariance

- `engine_digest` égal pour W ∈ {1, 2, 8} (et 48 sur G4) ;
- invariance par permutation de l'entrée (R1-a) ; équivariance des tableaux par point (R1-b) ; bijection de
  `PointId` (R1-c) ;
- **statut sous budget serré identique à W1 et à W8** (P19) ;
- backend CPU = CUDA (V10-4b).

### 12.4 Juges d'échelle (détectent une faute d'implémentation)

| Juge | Principe | Indépendance | Où |
| --- | --- | --- | --- |
| K=1 = EMST | la forêt d'ordre 1 (niveaux d²/4, plateaux N-aires) est égale, par `tower_merkle_v10` restreint à K=1, au dendrogramme à plateaux d'un EMST exact entier | Borůvka entier sur `SiteTree` (`tests/judges`), jamais le code de la tour | 8k/16k/32k ; **chaque trame mesurée**, sans sol et brute |
| **J-KM2** (§ 12.5) | Euler sur cat(kmax+2) pour K ≤ kmax **et** restriction canonique égale au catalogue du produit | transfert par définition (P2b) + identité du nerf | 8k/16k/32k K5 (six familles), 8k K10 ; chaque trame mesurée K5 et K10 |
| boîtes de centres | pour une boîte Q tirée (stratifiée : près des sites, dans les vides, sur les surfaces), tout support d'une boule admissible centrée dans Q est à distance ≤ d_K(q0) + diam(Q) du centre q0 de Q ; on énumère brutalement les ≤ 4-sous-ensembles de cette boule (i128 réécrit), on confirme en BigRat et on recense sur TOUT le nuage ; puis on compare au catalogue filtré par « centre dans Q » | autre lemme (borne de rayon), autre énumération, autre auteur (rôle D) | ≥ 200 boîtes par entrée ; au moins 30 % des boîtes tirées là où les boules à p ∈ {kmax−2, kmax−1} sont denses |
| I5 descente | toute sphère admissible visitée par une descente est au catalogue | gratuit (produit) | toutes |
| cohérence points–verticales | pour **tous** les sites x et tous les K ≥ 2, à a = D_K(x) : WA_{K−1}(image(entry_K(x)), a) = WA_{K−1}(entry_{K−1}(x), a) (TOWER § 12.4) | relation entre ordres ; échoue si un ordre est faux sans que l'autre le soit | toutes |
| **juge local borné** | pour ≥ 1 000 graines par (entrée, K) : BFS sur Γ_K(a) depuis N_K(x), MEB rationnelle de test indépendante, arrêt à 5 000 K-parties ; si la BFS se termine, sa composante (K-parties et points couverts) égale celle de la tour | arithmétique et algorithme de test, pas de catalogue | K = 2..kmax, dont ≥ 30 % des graines à K ∈ {kmax−1, kmax} ; plancher : ≥ 60 % de BFS terminées ; 8k/16k/32k et trames |
| **différentiel v9** | `catalogue_digest` et `tower_merkle_v10` égaux à ceux de la v9 relancée par exporteur (TOWER § 12.5) | implémentation indépendante (WSPD et lignée v7) | uniforme, terrain, huit amas 8k/16k/32k K5 (K10 à 8k et 16k) ; **chaque trame mesurée** (3 de conception, 30 de test, sans sol et brute), K5 et K10, épinglé une fois (session P9) |
| structure | I6, I7, I8, I10 rejoués hors produit, validateur structurel complet | test | toutes |
| consommateur = tour (**corrigé**) | à des coupes a tirées, le dendrogramme de points et la forêt d'ordre K ont le même nombre de composantes **contenant au moins un point entré** (D_K(x) ≤ a). Sans cette restriction, la porte est fausse : à K=2, deux points à distance d, L_2 a une composante sur [d²/4, d²) alors qu'aucun point n'est entré (constat mineur) | code distinct de `points/` | toutes ; égal à l'oracle C∩X brut sur petits nuages |
| tête K=1 = HDBSCAN | sur des nuages **sans égalité** (d² deux à deux distincts, vérifié en O(n²) pour n ≤ 2 000 ; aucun écart relatif de stabilité < 10^-9) : étiquettes égales à `HDBSCAN(min_samples=1, min_cluster_size=m)` à permutation près, EOM et leaf. Les nuages à égalités passent par une porte séparée qui ne compare que les partitions aux coupes hors niveaux d'égalité : v10 garde les plateaux N-aires, scikit-learn binarise arbitrairement et compare en `>` strict (constat mineur, P12) | scikit-learn 1.9.1 | ≥ 20 nuages, plusieurs m |
| mreach + tête = HDBSCAN | même règle pour min_samples ∈ {2, 3, 5, 8} : c'est ce qui autorise l'oracle 2D rapide du banc | scikit-learn | ≥ 20 nuages |
| E5 sur la tête | toute tête consommant la tour rend une racine sur E5, le témoin à 5 points et le 4-points L11 | fixtures | CI |

### 12.5 Protocole Kmax+2 et lemme de transfert (correction de B2)

**Pourquoi Euler seul ne juge pas les ordres kmax−1 et kmax.** La contribution d'une boule à χ(L_K) est le
coefficient de t^{K−1} dans t^p · Σ_{T ⊆ U, c ∈ conv T} (t−1)^{|T|−1} (v9 `euler_scale_gate.cpp`). Elle peut être non
nulle dès K = p + 1, quel que soit q_min. Juger l'ordre K exige donc toutes les boules de p ≤ K − 1, y compris celles
de q_min = 4, c'est-à-dire p + q_min ≤ K + 3. D'où kcat ≥ K + 2 : le catalogue du produit (kcat = kmax) ne permet
Euler que pour K ≤ kmax − 2.

**Défaut d'ARCH_v1 (critique B2, fondé).** Construire cat(kmax+2) « dans le juge » exécute les gardes et les
dominateurs au seuil kmax + 2, et non au seuil kmax du produit. Une faute **relative au seuil** mord à des p
différents dans les deux constructions. Le mutant « dominateur à K−1 » en est l'exemple :

- au seuil 10 (produit), il exclut un site qui a exactement 9 dominateurs, donc le site de coquille d'une boule
  admissible à p = 9, dont la fenêtre commence à p + q_min − 1 ≥ 10 : la tour publiée est fausse à l'ordre 10 ;
- au seuil 12 (juge), il ne touche que les boules à p = 11, qui n'agissent qu'aux ordres ≥ 12, hors du domaine
  d'Euler.

Euler reste vert, le produit est faux. C'est l'angle mort v9 que le § 12.5 d'ARCH_v1 prétendait fermer. La v9
possédait pourtant le correctif : `euler_scale_gate.cpp` exigeait « l'égalité EXACTE, clé par clé, du catalogue K5
avec la restriction p + q_min ≤ 6 du catalogue K7 », sur une famille. ARCH_v1 l'avait perdu.

**Lemme P2b (transfert).** Soit adm_k l'admission du § 5.2 et cat_k = {b critique bien centrée : adm_k(b)}.

- Pour k ≤ k', adm_k(b) ⇒ adm_{k'}(b) : les deux clauses sont monotones en k.
- Donc {b ∈ cat_{k'} : adm_k(b)} = cat_k.

L'égalité découle des **définitions** et ne dépend pas de l'algorithme (seuils de gardes, dominateurs, M, T).

**Protocole J-KM2** (juge `mhgp10_judge kmax_plus_2`, par entrée, à K5 et à K10) :

1. C = catalogue du produit à kcat = kmax. C'est la construction que les passes chronométrées exécutent : leur
   `engine_digest` couvre le catalogue.
2. C' = catalogue à kcat = kmax + 2 : même code, autres seuils (kcat ≤ 12 est interne, D-29).
3. Euler sur C' pour K = 1..kmax, entrée non pondérée. En cas d'échec : code 1, `cause=kmax_plus_2.euler`.
4. `catalogue_digest(restrict(C', adm_kmax)) == catalogue_digest(C)`. Le digest hache (niveau réduit, S\*, I, U,
   poids, q_min), jamais les rangs, qui diffèrent entre C et C'. En cas d'échec : code 1,
   `cause=kmax_plus_2.restriction`, suivi de la première clé qui diffère.

**Ce que J-KM2 couvre.**

- Une faute relative au seuil apparaît à des p différents dans C et C', et (4) échoue. Le mutant « dominateur à K−1 »
  est tué ainsi : C' restreint contient les boules à p = 9 correctes, C ne les a pas.
- Une faute indépendante du seuil apparaît identiquement dans C et C'. Elle est vue par (3) si elle change une somme
  d'Euler à un ordre ≤ kmax, sinon par le juge des boîtes, par le juge local, par I5 et par le différentiel v9.
- L'invariant produit I2 (Euler sur C pour K ≤ kmax − 2, O(sortie)) reste actif : il est gratuit.
- Entrée pondérée : (3) ne s'applique pas tant que P17 (Euler pondéré) est ouvert. Le juge l'écrit
  (`euler=not_applicable_weighted`) ; (4) s'exécute toujours.

**Coût** : C' pèse environ (12/10)³ ≈ 1,7 fois C en boules à K10, et (7/5)³ ≈ 2,7 fois à K5. C'est un coût de
référence, jamais chronométré. Sur G4, il s'ajoute aux ≈ 35 min de références d'EVAL § 12.9.

**Mutants attachés** : `dominator_threshold_k_minus_1`, `guard_threshold_k_minus_1`, `admission_p_plus_q_le_k_plus_2`.
Ils sont tués à l'échelle par J-KM2 (uniforme 8k K5) et à petite taille par l'oracle brut.

### 12.6 Matrice de couverture à l'échelle (correction de M7)

Question : sur une trame du contrat, qu'est-ce qui juge l'ordre K ?

| Mécanisme | K = 1 | 2 ≤ K ≤ kmax − 2 | K ∈ {kmax − 1, kmax} | Nature |
| --- | --- | --- | --- | --- |
| EMST exact | forêt complète | — | — | complet, indépendant |
| I2 (Euler produit) | catalogue | catalogue | — | nécessaire seulement |
| J-KM2 | catalogue | catalogue | **catalogue** | Euler + transfert |
| boîtes de centres | catalogue | catalogue | catalogue | échantillon, indépendant |
| I5 | complétude locale | idem | idem | gratuit |
| cohérence points–verticales | — | forêt (tous les sites) | forêt (tous les sites) | global, relationnel |
| juge local borné | — | forêt (échantillon) | forêt (≥ 30 % des graines) | échantillon, indépendant |
| **différentiel v9** | complet | **complet** | **complet** | implémentation indépendante ; la v9 a elle-même un angle mort à kmax−1 et kmax, donc un désaccord déclenche une enquête (fixture minimale, registre), jamais un verdict automatique en faveur de l'une ou de l'autre |
| racine unique, identité de forêt, naturalité | toutes | toutes | toutes | invariants |

Toute optimisation de V10-4 (balayage intra-ordre, GPU) se juge à trois niveaux :

- digest égal à la référence v10 jugée ;
- épingle v9 de la trame ;
- juge local et cohérence réexécutés sur la nouvelle voie.

Une faute de tour aux ordres ≥ 2 ne peut donc plus se propager en silence sur les trames du contrat.

### 12.7 Portes de coût (familles adverses)

- Synthétiques (EVAL § 12.8 d) : 19 familles, 8k → 16k → 32k, K5 et K10. Les pentes par doublement doivent vérifier
  p_boules ≤ 1,15 et p_travail ≤ p_boules + 0,15, sur les compteurs déterministes. Les coquilles, deux plans et huit
  amas en font partie, ainsi que la famille à sortie Ω(n²) de la v7 (la porte y est relative à la sortie).
- LiDAR cumulé (EVAL § 12.8 c) : 1, 2, 4 puis 8 trames recalées, avec p_travail − p_boules ≤ 0,15.
- Trame brute contre sans sol : **diagnostic publié, pas une porte**. Rapports mesurés par boule à K5, brute contre
  sans sol : tests de garde 495 contre 389 (×1,27) ; paires 23,4 contre 21,1 ; triplets 45,5 contre 41,8 ;
  quadruplets 37,1 contre 38,8. Un seuil à 1,3 serait fragile ; la distribution diffère (sol plan).
- Une entrée qui dépasse le budget logique rend `resource_exhausted` au lieu de tourner indéfiniment.

### 12.8 Digests (correction de M6)

FNV-1a sur mots de 64 bits est abandonné. La critique l'a démontré et c'est reproduit ici : inverser le bit 63 dans
deux mots donne un digest identique, et une seule inversion ne change que le bit 63 de la sortie. Tous les digests
sont désormais SHA-256, calculés **hors chronomètre** par `io/`.

| Digest | Contenu | Usage |
| --- | --- | --- |
| `engine_digest` | Merkle SHA-256 des tableaux du moteur dans l'ordre canonique : chaque tableau est sérialisé en petit-boutiste et coupé en tranches **fixes** de 1 Mio ; les tranches sont hachées en parallèle ; hash de tableau = SHA-256(nom, type, longueur, hashes des tranches) ; digest = SHA-256 des hashes de tableaux dans un ordre fixé. Les niveaux sont identifiés par (rang, `level_rep` → coordonnées de S\*), sans calcul de PGCD. Les `PointId` en sont exclus | déterminisme (W, backend, permutation), honnêteté du chronomètre, mutant « étage sauté » |
| `catalogue_digest` | GEN § 7 : SHA-256 sur (niveau réduit, S\*, I, U, poids, q_min) en ordre canonique | J-KM2, différentiel v9 |
| `tower_digest_v10` | TOWER § 5.7 : sérialisation linéaire canonique, niveaux réduits | épingles, reçus |
| `tower_merkle_v10` | TOWER § 5.7 : sans numérotation | comparaisons entre implémentations (v9, GPU) |
| `id_digest` | listes de `PointId` par site | R1-c |

Implémentation : `io/sha256.cpp` en version portable, plus une voie SHA-NI choisie à l'exécution
(`__attribute__((target("sha,sse4.1")))` et `__builtin_cpu_supports`, sans `#if`). Test unitaire : vecteurs NIST, et
égalité des deux voies sur 10^4 tampons aléatoires. Débit : OpenSSL fait 1,5 Go/s sur un fil du codespace (SHA-NI
présent, mesuré ici). Avec le Merkle parallèle, le coût est inférieur à 0,1 s pour 1 Go sur G4.

### 12.9 Honnêteté du chronomètre et de la mémoire

- Chaque passe chronométrée est **digest-égale** (`engine_digest`) à la référence jugée de la même (trame, K). Cette
  référence a passé EMST, J-KM2, le juge local, la cohérence, la structure et l'épingle v9. Une passe à digest
  différent est invalide, et la trame échoue au contrat (EVAL § 12.5).
- La sonde publie B0 (processus froid), B2 (résident, contrat), les phases B3, et F(W) séparément (EVAL § 12.2).
- Le pic logique compté doit valoir au moins 85 % du pic RSS au-delà du socle.
- Le mutant `skip_stage_resolve` (étage G réduit à zéro requête) est tué par la porte de digest. Avec SHA-256, cette
  porte a un sens.

### 12.10 Mutants initiaux (≈ 26 à la sortie de V10-2)

- **Catalogue** : garde à K−1 ; **dominateur à K−1** ; dominateur non strict ; boîte fermée au lieu de demi-ouverte ;
  signe du lemme Z inversé ; dédoublonnage des coquilles étendues coupé ; q_min non recalculé ; admission
  p + q ≤ K + 2 ; intérieur non strict ; poids de multiplicité ignoré.
- **Tour** (TOWER § 12.3) : `window_lo`, `binarize`, `drop_continuations`, `first_rep_only`, `vertical_open`,
  `wa_open_as_closed`, `knn_ignores_weight`, `pointer_nonstrict`, `lookup_misses_extended`, `euler_skip_last`.
- **Tête** : descente « aucun enfant gros » (défaut v9) ; racine sélectionnable par défaut ; masse du parent perdue à
  la scission.
- **Arithmétique et infrastructure** : retenue perdue dans la multiplication U192 ; marge du filtre de niveau 2^-50 ;
  arrondi de P3c par troncature au lieu de plancher (N négatif) ; compteur logique alimenté par les octets physiques
  du pool, tué par la porte « statut identique à W1 et W8 sous budget serré » ; taille de tranche Merkle dépendante
  de W, tuée par la porte de déterminisme.

---

## 13. CI GitHub (`.github/workflows/morsehgp3d-v10.yml`)

Déclencheurs : `push` sur `main` et `pull_request` touchant `morsehgp3D_v10/**`, `gcp-migration/v10_*`,
`tools/check_docs.py` ou le workflow lui-même.

| Job | Machine | Contenu | Budget |
| --- | --- | --- | --- |
| `gates` | ubuntu-22.04, **g++ 11**, CMake **3.22.1** (pip), comme la VM G4 ; **Python 3.12 par `actions/setup-python`** ; `ccache` en cache | configure (valide aussi `mutants.json`), build Release, `ctest -L fast`, tests Python de la tête et du banc (normal et `-O`), `check_v10.py`, `tools/check_docs.py`, `receipt.py check --changed` (§ 14), `v10_selftest.py` (faux gcloud) | `timeout-minutes: 20` ; cible ≤ 15 min, **mesurée** à la sortie de V10-0 |
| `sanitize` | ubuntu-24.04, clang 18, Debug + ASan/UBSan | `ctest -L sanitize` | idem, en parallèle |
| `mutants` | `workflow_dispatch` et hebdomadaire | `-DMHGP10_BUILD_MUTANTS=ON`, `ctest -L mutant` | ≤ 45 min ; non bloquant par commit, bloquant en sortie de phase |

**Budget mesuré, pas déclaré (correction de M9).** La v9 mettait 17,5 min à construire en Release sur 2 vCPU,
pour 33 k lignes et 169 exécutables. La v10 vise 12 k lignes et 5 exécutables de tests en build unitaire ;
l'estimation (5 à 8 min de build) reste à mesurer.

- **Sortie de V10-0** : trois exécutions du job, durée de chaque étape publiée dans un reçu.
- Le `timeout-minutes` et la liste des tests du label `fast` sont alors gelés à la valeur mesurée plus 30 %.
- `fast` ≤ 5 min. Au-delà, on déplace un test vers `gate`, avec son nom dans la passation. On ne descend jamais la T2
  CI sous 100 nuages ni sous n = 9.

Il n'y a ni secret, ni écriture GCP, ni GPU, ni donnée KITTI. Les tests Python installent `numpy==2.5.3` et
`scikit-learn==1.9.1`.

Règle : **CI rouge = arrêt des poussées**, sauf le correctif ou le revert, dans l'heure, par l'agent qui a poussé en
dernier.

---

## 14. Reçus

Un chiffre publié (README, PASSATION, message de commit, audit) cite un reçu ; sinon il n'existe pas.

```text
morsehgp3D_v10/receipts/<chantier>_<YYYYMMDD>/
  RECEIPT.json   (mhgp10_receipt_v1)   runs.jsonl (mhgp10_run_v1)   SUMMARY.md (≤ 60 lignes)   SHA256SUMS
```

`tools/receipt.py` a trois commandes.

- `make` refuse un arbre sale sur `src/`, `alloc/`, `cli/`, `bench/`, `cmake/`, un commit non poussé, ou une sonde
  construite hors de son propre répertoire (`build/receipt_<nom>/`, supprimé après le reçu).
- `check --changed <base>..<tête>` (CI) ne vérifie que les reçus **ajoutés ou modifiés** dans l'intervalle poussé, et
  seulement leur forme : cohérence de `SHA256SUMS`, tailles, extensions, JSON lisible, présence du champ `schema`.
  Aucun reçu historique n'est relu en CI : c'était la cause de la CI rouge v9 (constat mineur). Un reçu est immuable
  après son commit.
- `replay` reconstruit au commit dans un worktree clairsemé détaché, rejoue les commandes et compare les digests
  (jamais les temps).

Emplacement : jamais `/tmp`. Les sessions G4 écrivent sous `/workspaces/.ehgp-sessions/v10.<horodatage>/`, puis le
reçu expurgé est copié dans `receipts/`. Les binaires exécutés, leurs sha256 et la recette de build sont dans le reçu.

---

## 15. Protocole G4

### 15.1 Réutilisé tel quel

`gcp-migration/start_and_verify.sh` et `stop_and_verify.sh` :

- double coupe-circuit ;
- `maxRunDuration` entre 30 s et 8 h ;
- label `project=e-hgp` ;
- arrêt ciblé par génération ;
- `TERMINATED` certifié.

Cible épinglée dans `gcp-migration/v10_target.json` (projet, zone, instance, `g4-standard-48`, SPOT, action STOP).
Faits VM (L05) : EPYC 9B45 24c/48t, 185 Go, RTX PRO 6000 Blackwell sm_120, pilote 580.173.02, nvcc 12.9.41, g++ 11.4,
CMake 3.22.1, Python 3.10.

### 15.2 Composants v10

- `v10_session.py` (≤ 500 lignes, inerte sans `--execute`) : paquet depuis un **commit** poussé (`git archive`,
  reconstruit octet pour octet à la réception, comme `require_committed_protocol` v9) ; données depuis le cache,
  vérifiées contre `data/MANIFEST.json` ; démarrage gardé ; recertification ; téléversement ; worker ; rapatriement ;
  arrêt ciblé.
- `v10_worker.sh` (sur la VM, Python 3.10 et bibliothèque standard seulement pour ses auxiliaires) : build Release
  (et CUDA si le plan le demande), `ctest -L fast`, `-L device` si CUDA, plan, archive **incluant les binaires
  exécutés et leurs sha256**. La v9 excluait `output/build` : binaire perdu, session Nsight inutilisable.
- `v10_recover.py` (§ 15.3), `v10_selftest.py` (tout le cycle contre un faux gcloud, en CI), plan JSON par session.

### 15.3 Persistance et reprise après perte du contrôleur (correction de M10)

L'arrêt « dans `finally` » ne protège ni d'un SIGKILL ni d'un redémarrage du conteneur. C'est l'incident du 27 août :
trap perdu, `/tmp` effacé, VM restée RUNNING une heure. ARCH_v2 s'appuie sur les garanties que les scripts gardés
offrent déjà, et que `tower_session_v9.py` utilisait.

**Répertoire de session persistant** `/workspaces/.ehgp-sessions/v10.<UTC>.<suffixe>/` (mode 0700, survit au
conteneur) :

| Fichier | Rôle |
| --- | --- |
| `session.env` | `GCP_PROJECT_ID`, `GCP_ZONE`, `GCP_INSTANCE_NAME` (depuis `v10_target.json`), commit, sha256 du paquet et du plan, `maxRunDuration`, minutes de la garde invitée |
| `key/id_ed25519{,.pub}` | clé de session (§ 15.4) |
| `lifecycle.txt` | `start_and_verify.sh --lifecycle-state-file` : `start_may_have_been_requested` → `targeted_running` (génération) → `targeted_stopping` / `targeted_stopped` / `targeted_stop_failed` |
| `handoff.json` | `--handoff-file` : témoin de génération publié atomiquement |
| `guardmarks/` | `--guard-mark-dir` : `guest_guard_pending`, `double_guard_verified` |
| `journal.jsonl` | chaque étape du contrôleur, ajoutée puis `fsync` |
| `controller.pid` | présence d'un contrôleur vivant |
| `v10_recover.py` | copie du script de reprise **au commit de la session**, faite au lancement |
| `capture/` | rapatriement, binaires compris |

Au lancement, le contrôleur imprime la commande de reprise :
`python3 /workspaces/.ehgp-sessions/v10.<…>/v10_recover.py /workspaces/.ehgp-sessions/v10.<…>`.

**`v10_recover.py`** (≤ 250 lignes ; sémantique de `recover_v6_session.sh`, audit série C § 5.18.6) :

1. Ne démarre **jamais** la VM. Refuse si `controller.pid` désigne un processus vivant.
2. Lit `lifecycle.txt` et `handoff.json`.
   - Si aucune génération n'est connue et que l'état vaut `start_may_have_been_requested` : blocage, code 71. Le
     script imprime les deux commandes manuelles, variables de cible comprises (`gcloud compute instances describe
     … --format='value(status,lastStartTimestamp)'`, puis `GCP_PROJECT_ID=… GCP_ZONE=… GCP_INSTANCE_NAME=…
     stop_and_verify.sh --yes --expected-last-start-timestamp <valeur lue>`). Sans ces variables,
     `stop_and_verify.sh` vise une cible par défaut et refuse (incident du 27 août).
3. Si la génération g est connue, lit l'état de la cible.
   - TERMINATED avec `lastStartTimestamp` = g : l'arrêt est certifié.
   - RUNNING avec g : si `double_guard_verified` est présent et que l'échéance de garde laisse plus de 10 min, le
     script rapatrie `output/` en lecture seule, au plus 5 min, avec une clé éphémère OS Login de TTL ≤ 20 min.
     Puis, dans tous les cas, il exécute `stop_and_verify.sh --yes --expected-last-start-timestamp g` avec les
     variables de `session.env`.
4. Écrit un reçu `…_reprise_<epoch>`, dont le statut est forcé à `partial_or_invalid` (jamais une décision). Un
   journal perdu y est déclaré tel.

`v10_selftest.py` tue le contrôleur par SIGKILL à six moments, contre le faux gcloud :

- avant la demande de démarrage ;
- après la demande ;
- après la publication de la génération ;
- pendant le worker ;
- pendant la capture ;
- pendant l'arrêt.

Il exige soit `TERMINATED` certifié par la reprise, soit le blocage 71 quand aucune génération n'est connue.

### 15.4 Clé OS Login (contraintes de `start_and_verify.sh`)

- `GCP_SSH_KEY_FILE` : ED25519 privée, **non chiffrée**, générée par session dans `key/`. Mode **0600**, répertoire
  0700. Le contrôleur refuse avant tout appel GCP si ces modes ne sont pas respectés : selon la critique, un échec
  v9 venait d'une clé en 0644.
- Inscription unique dans OS Login juste avant le démarrage, avec une expiration restante comprise entre
  `maxRunDuration` et `maxRunDuration + 660 s` (`SSH_KEY_TTL_SLACK_SECONDS`), sinon le démarrage est refusé. Le
  contrôleur passe `--ttl = maxRunDuration + 300 s`, soit 65 min pour 3 600 s (la v9 passait 70 min pour 3 600 s).
- Après l'arrêt certifié : `os-login ssh-keys remove`, au mieux, journalisé.

### 15.5 Sessions planifiées

| Session | Phase | Contenu | Durée |
| --- | --- | --- | --- |
| **C0 calibration** | V10-0 | `bench/proto/cble` (commis) à K5 et K10 sur les 3 trames de conception et la trame brute 08/000000, à W1, W24 et W48 ; moteur v9 K5 à W1 et W48 sur 08/000000. En local, au même commit : cble à W1 sur les mêmes entrées. Sorties : ρ, S24, S48, c1_G4 (§ 6.3) | `maxRunDuration` 3 600 s ; ≤ 25 min utiles |
| C1 catalogue | sortie de V10-1 | catalogue v10 à W1 et W48, mêmes entrées ; calibration de M(K) ∈ {12, 16, 24, 32} ; J-KM2 sur les 4 trames | 3 600 s |
| **P9 épingles v9** | dès que les trames existent | chaîne v9 (`ce8a649dd`, exporteur canonique) K5 et K10 sur chaque trame mesurée, sans sol et brute ; `PINS.json` avec reçu | ≤ 2 h (estimé : 33 trames × 2 variantes, ≈ 12 s sans sol et ≈ 35 s brute par trame et par paire de K, soit ≈ 30 min, plus le build) |
| contrat CPU | V10-4a | plan EVAL § 12.3, bras `cpu48` (et `cpu24`) | ≤ 3 h, découpable K5 puis K10 |
| contrat GPU | V10-4b | bras `gpu` et `cpu48` | idem |

Avant toute session, trois conditions : `ctest -L gate` vert en local au commit ; syntaxe CUDA vérifiée localement
(nvcc extrait, compilation seule) ; `v10_selftest.py` vert.

### 15.6 SPOT : préemption et rupture de stock

- **Préemption** : reçu `incomplete`, jamais une revendication. Les lignes ne se fusionnent entre sessions que si le
  sha256 du binaire, le type d'hôte et le pilote sont identiques (EVAL § 12.9) ; sinon, le plan recommence.
- **Rupture de stock au démarrage** : démarrage refusé, reçu `start_refused_stockout`, cible **inchangée**. Changer
  de zone exige un commit revu de `v10_target.json`. Après deux ruptures consécutives, le plan est reporté et
  l'utilisateur prévenu.
- Une seule session SPOT utile à la fois.

### 15.7 Frontière du contrat de temps

Elle est définie par EVAL § 12.2 :

- **B2** (résident) : du tableau des sites en mémoire hôte à la tour FULL canonique en mémoire hôte, transferts
  compris. C'est le contrat.
- **B0** : processus froid, publié dans la même phrase.

Il reste à confirmer par l'utilisateur (question 3) que cette frontière est la bonne.

---

## 16. Données (correction de M8)

- Aucun octet KITTI dans le dépôt. `data/MANIFEST.json` donne, pour chaque trame : séquence, index, variante (sans
  sol, brute), retours, sites, sha256 du `.bin` et du masque, recette (préparateur, Patchwork++ `3e6903a1`).
- Cache hors Git : `MHGP10_DATA_DIR`, par défaut `/workspaces/.ehgp-data/v10/` (≈ 2 Mo par trame, ≈ 70 Mo pour 33
  trames et leurs deux variantes).
- **Trames de test** : EVAL § 12.1, soit 30 trames, 3 par séquence, pour s ∈ {00…07, 09, 10}, tirées par sha256 et
  gelées dans `perf/FRAMES_v1.toml`. **Trames de conception** : 08/000000, 000100, 000200.
- **Constat** (recherche faite ici) : aucune donnée brute SemanticKITTI n'est présente sur le codespace. Seules les
  trois trames de conception existent, sous forme dérivée dans l'historique v8.
- **Blocage de données déclaré.** Le contrat C(K, T) d'EVAL (p95 sur 30 trames de 10 séquences) ne peut pas être
  évalué tant que l'utilisateur ne fournit pas les 30 scans bruts, ou n'autorise pas le téléchargement de l'archive
  velodyne de KITTI odometry (≈ 80 Go). Cette archive tiendrait dans `/tmp` (231 Go libres, volatil) le temps d'en
  extraire les 30 scans, pas dans `/workspaces` (question 2).
- Jusque-là, sur les trois trames de conception, on ne publie que des **diagnostics** : maximum sur trames et passes,
  jamais un p95. Aucune phrase de contrat n'est écrite.

---

## 17. Arithmétique (correction de M3)

### 17.1 Repères

Trois repères, repris de GEN § 4.1 :

- **global** : sites entiers u18 ;
- **arbre** : X = 2^T x avec T = 6, bornes de boîtes entières, étendue S = 2^24 ;
- **feuille** : origine locale.

Les bornes ci-dessous sont calculées dans le repère **global**, le pire cas. Le repère local de GEN ne peut que les
réduire.

### 17.2 Lemme Z : droite des centres q4 contre la boîte, en i128

Énoncé. Soient trois sites x_i, x_j, x_k non alignés, et f_j(c) = |x_i − c|² − |x_j − c|², une forme affine en c.

- La droite Λ des points équidistants des trois sites rencontre la boîte fermée Q̄ = [l, h] si et seulement si
  0 ∈ Φ(Q̄), avec Φ(c) = (f_j(c), f_k(c)).
- Φ(Q̄) est le zonogone Φ(m) + Σ_a [−½, ½] γ_a, où γ_a = (2(x_j − x_i)_a (h_a − l_a), 2(x_k − x_i)_a (h_a − l_a)).
  Il est de dimension 2 parce que les sites ne sont pas alignés.
- Test exact : pour tout a tel que γ_a ≠ 0, avec n_a = (−γ_a,2, γ_a,1), il faut
  |n_a · (Φ(l) + Φ(h))| ≤ Σ_b |n_a · γ_b|.

Preuve. Φ est affine, donc Φ(Q̄) est un zonogone. Ses arêtes sont parallèles aux générateurs non nuls. Un point y
appartient si et seulement si, pour chaque normale d'arête, sa projection tombe dans l'intervalle de support
n · Φ(m) ± ½ Σ |n · γ_b|. Et Φ(l) + Φ(h) = 2Φ(m).

Largeurs dans le repère de l'arbre : membre gauche ≤ 2^102,2, membre droit ≤ 2^100,6. C'est de l'i128, même en
global. Ce test remplace :

- le préfiltre flottant du prototype à marge 1e-3 (32 à 65 M `line_hits` par trame) ;
- le test par paramètres de droite, qui atteint 2^133,1 et est **interdit** (critique M3-a).

Porte V10-0 : 10^6 couples (triplet, boîte) tirés contre BigRat. On y inclut des boîtes tangentes, des droites
parallèles à une face, des sites quasi alignés et les coins u18.

### 17.3 Voie rare U256 (Gordan, q_min étendu, plateaux)

Les tests d'appartenance d'un centre **q3** (N ≤ 2^95, D ≤ 2^76,2) à un plan, à un segment ou à un triangle d'autres
sites de coquille atteignent 2^134,3. C'est le cas du recalcul de q_min et de S\* sur coquille étendue, et du
quotient de Gordan « c ∉ conv(supp A) ». Ces tests passent donc en **U256 signé**, par **une seule** implémentation,
sans chemin i128 rapide. Les tests avec un centre q4 (≤ 2^115,9) et avec un milieu q2 tiennent en i128, mais la voie
rare les traite aussi en U256 : la simplicité prime, puisque cette voie concerne ≈ 0,02 à 0,04 % des boules LiDAR.

C'est précisément là que le test de plateau i128 de la v7 rendait des réponses fausses à 18 bits (L12).

### 17.4 Lemme P3c : élagage exact d'un pavé entier contre une boule ouverte rationnelle

Énoncé. Soient un centre c = a + N/D, avec a un site de la coquille et D > 0, et g(z) = D|z − a|² − 2N · (z − a),
de sorte que z est strictement intérieur si et seulement si g(z) < 0. Sur un pavé entier [L, H], relatif à a :

- min sur les points **entiers** de g = Σ_i min_{t ∈ [L_i, H_i] ∩ ℤ} (D t² − 2 N_i t) ;
- chaque minimum est atteint en ⌊N_i/D⌋ ou en ⌊N_i/D⌋ + 1, bornés à [L_i, H_i].

Preuve : g est séparable ; chaque terme est une parabole convexe, dont le minimum entier est à l'entier le plus
proche du sommet, borné à l'intervalle.

- Largeur : ≤ 2^115,9, donc i128.
- Vérifié par `arch_v2/lattice_prune_check.py` : 20 000 pavés et centres tirés, 0 écart avec la force brute.
- Ce lemme n'est pas sur le chemin du produit (§ 3) : il sert si un élagage rationnel devient nécessaire, et il
  réfute l'affirmation « il faut 2^191 ».
- Mutant attaché : arrondi par troncature au lieu du plancher.

### 17.5 Table des largeurs (gravée en `widths.hpp` par `static_assert`, calculée par `arch_v2/widths.py`)

| Quantité (repère global u18, arbre T = 6) | log2 max | Type |
| --- | ---: | --- |
| garde, dominance, distance site–boîte (repère de l'arbre) | 51,2 | i64 |
| lemme Z : n · (Φ(l) + Φ(h)) / Σ \|n · γ\| | 102,2 / 100,6 | i128 |
| centre q3 : N relatif / D | 95,0 / 76,2 | i128 |
| centre q4 : N relatif / D | 76,8 / 57,4 | i128 |
| centre dans [l, h) : 2^T (N + D a) / l · D | 101,6 / 100,2 | i128 |
| recensement D\|z − a\|² − 2 N · (z − a) | 115,3 | i128 |
| orientation avec centre q4 (tétraèdre strict) | 115,9 | i128 |
| orientation ou coplanarité avec centre q3 (Gordan, q_min étendu) | 134,3 | **U256** (voie rare) |
| niveau q3 : num / dén | 112,8 / 77,2 | u128 |
| niveau q4 : num / dén | 155,1 / 114,8 | U192 / u128 |
| comparaison de niveaux (produit croisé maximal) | 269,8 | U320 |
| C∩X : d² · dén contre num | 152,3 | U192 |
| élagage de pavé (P3c), somme des 3 axes | 115,9 | i128 |
| droite q4 / boîte par paramètres | 133,1 | **interdit** (on utilise le lemme Z) |
| identité d'une boule | — | S\* : quatre `SiteIdx`, aucune arithmétique |

La clé de coquille étendue d'ARCH_v1, « i128 + u32 », était fausse (critique M3-d) : un centre réduit comporte trois
numérateurs de ≤ 95,6 bits et un dénominateur de ≤ 76,9 bits. Elle est remplacée par S\* (GEN § 1.2).

### 17.6 Flottant et arrondi

- Aucune décision du générateur ni de la tour ne dépend d'un flottant sur CPU (GEN § 4.4).
- Seule la clé de tri des niveaux est flottante : erreur ≤ 3·2^-53, bandes réparées en U320.
- `Session::open` vérifie `fegetround() == FE_TONEAREST`, sinon `invalid_input/fp_environment` (constat mineur ; la
  v4 coupait ses filtres dans ce cas).
- `-ffast-math` est refusé au configure, et `__FAST_MATH__` provoque un `#error`.
- Côté appareil (P14) : les unités de filtre sont compilées avec `nvcc --fmad=false`, parce que nvcc contracte en FMA
  par défaut. Toute autre option exige des bornes prouvées sous contraction. Aucun filtre flottant n'est activé avant
  P14.

---

## 18. Plan de phases

### 18.1 Vue d'ensemble

```text
             J0      J2        J6          J11         J16          J24
V10-0  [fondations + C0]
V10-1           [catalogue + oracles + J-KM2 + C1] → décision D-K5
V10-2           [tour sur doublure brute] → [tour sur catalogue réel + diff v9 + T2 pondérée]
V10-3  [tête + banc + mreach (dès J0)] ··········→ [branchement tour, E1]
V10-G                    [catalogue GPU, si D-K5 l'ouvre] ·····→
V10-4a                                              [CPU W48 sur G4 : diagnostic ou contrat selon les données]
V10-4b                                                          [GPU]
V10-5                                                     [campagne préenregistrée]
```

Les durées sont indicatives, en jours-agent, pour 4 à 5 agents. Deux doublures de test découplent les pistes : le
catalogue brut borné laisse avancer la tour, et le témoin mreach laisse avancer la tête et le banc.

### 18.2 Phases, portes d'entrée et de sortie

**V10-0 — Fondations et calibration (J0–J2).**

Entrée : cette conception acceptée ; accord de l'utilisateur pour la section « Ouverture v10 » d'`AGENTS.md` et la
cible de `CLAUDE.md` ; disque vérifié (§ 20.3).

Livrables :

- arborescence, CMake, `run_expect.cmake` porté (PROVENANCE) ;
- `core` (budget logique, `ChunkPool`, statuts), `sched`, `arith` (entiers larges, lemme Z, P3c, `widths.hpp`),
  `cloud` (`SiteTable`, `SiteTree`) ;
- `io` (SHA-256 portable et SHA-NI, Merkle) ; `alloc/` et `tests/support/` ;
- en-têtes contrats de toutes les couches (types seulement) ; BigInt/BigRat de test ;
- `check_v10.py`, `receipt.py`, `run_schema.py`, `disk_gate.sh`, `sync_push.sh` ;
- workflow CI ; `v10_session.py`, `v10_worker.sh`, `v10_recover.py`, `v10_selftest.py` ;
- `bench/proto/cble.cpp` commis ; squelettes de README, PASSATION, SPEC_V10, PREUVES_V10 ; section v10 du registre.

Sortie :

- CI verte, **durées mesurées sur trois exécutions et gelées** (§ 13) ;
- `arith` contre BigInt : 10^6 opérations tirées, coins u18, collision (0,1,0)/(0,0,65536), refus de 262 144 ;
- lemme Z et P3c contre BigRat ou force brute ;
- `SiteTree` contre force brute sur 1 000 requêtes ;
- `sched` bit-identique à W ∈ {1, 2, 8} ;
- porte P19 (budget serré, statut identique à W1, W2 et W8) ;
- vecteurs NIST SHA-256, voie SHA-NI égale à la voie portable ;
- `v10_selftest.py` vert, reprise par SIGKILL comprise ;
- **reçu de la session C0** (ρ, S24, S48, c1_G4 du prototype) ;
- mutants tués : plage de coordonnées, retenue U192, marge de filtre, signe du lemme Z, arrondi de P3c, compteur
  logique du pool ;
- P1, P3, P3a, P3b, P3c, P4, P16 et P19 `proved_here` dans PREUVES_V10.

**V10-1 — Catalogue exact, oracles, J-KM2 (J2–J6).**

Entrée : V10-0 close ; P1, P2, P2b et P18 écrits ; note de réouverture formelle de la piste v3 « cellules de
centres » (`PISTES_FERMEES.md` : lemme, fixtures, porte de coût distincte).

Sortie :

- oracle catalogue brut ≥ 500 nuages dégénérés, kcat = 1..12, zéro désaccord (entrées pondérées comprises : le
  catalogue les traite, seule l'API les refuse encore) ;
- comptes par (q, p) et coquilles étendues égaux à la v9 sur les 14 entrées L13 ;
- `catalogue_digest` égal à l'exporteur v9 sur uniforme, terrain, huit amas 8k et sur les 3 trames de conception
  K5/K10 ;
- **J-KM2 vert** à 8k/16k/32k K5 (uniforme, huit amas, terrain, deux plans, coquilles, filaments), à 8k K10, et sur
  les 3 trames de conception et la brute 08/000000 à K5 et K10 ;
- juge des boîtes vert ; portes de coût vertes (§ 12.7) ;
- **travail déterministe par boule sur LiDAR K5 et K10 ≤ celui du prototype v2** (GEN § 10.1 : 389 et 300 tests
  d'arbre par boule, 62,8 et 63,4 dominances de feuille, etc.). Temps local publié comme diagnostic seulement ;
- déterminisme, permutation, relabel ; 10 mutants de catalogue tués, dont `dominator_threshold_k_minus_1` par J-KM2 ;
- **reçu de la session C1**, puis décision D-K5.

**Décision D-K5** (écrite dans la PASSATION avec son reçu, à la sortie de V10-1). On prédit, pour une trame de
60 k sites en `cpu48` :

```text
B2(K5) = c1_G4 · 2,0 M / S48 + T_tour + 0,05 s,   avec T_tour ∈ [0,10 ; 0,30]
```

- Si la borne haute est ≤ 0,8 s : C(K5, 1 s) est poursuivi en CPU seul (V10-4a).
- Si la borne basse est > 1,0 s : la piste **V10-G** (catalogue GPU, rôle E) s'ouvre immédiatement, en parallèle de
  V10-2.
- Entre les deux : les deux pistes, V10-G en second.

Pour K10, V10-G s'ouvre dans tous les cas, sauf si la borne haute prédite en CPU seul est ≤ 0,8 s, ce qu'aucun
scénario du § 6.4 ne donne.

**V10-2 — Tour FULL, T2, différentiel v9 (J2–J11).**

Entrée : V10-0 close pour démarrer sur la doublure ; V10-1 close pour sortir. P5, P5w, P6, P7, P7w, P8 et P9 écrits.

Sortie :

- T2 ≥ 5 000 nuages, K = 1..min(10, n), doublons (≥ 200 sites de poids ≥ 2) et cosphériques compris, planchers
  atteints ;
- fixtures de l'annexe A ; EMST à 8k/16k/32k et sur les trames ; racine unique ; I2 ; J-KM2 au niveau tour ;
  cohérence points–verticales ; juge local borné (planchers) ;
- différentiel v9 : uniforme, terrain, huit amas 8k/16k/32k K5, 8k/16k K10, et les trames de conception K5/K10,
  sans sol et brute. Toute différence devient une fixture et une ligne du registre ;
- déterminisme W1/W8 et P19 ; RSS et octets par boule dans le § 5.6 (reçu local) ; 10 mutants de tour tués ;
- **PO-T16 (P5w) `proved_here` : le refus `duplicate_positions` est levé dans le même commit.**

**V10-3 — Hiérarchies de points, têtes, banc (J0–J13).**

Entrée : piste mreach et tête dès la clôture de V10-0 ; branchement de la tour quand la T2 de V10-2 est verte. P10,
P11 et P12 écrits.

Sortie :

- tête K=1 = HDBSCAN sur nuages sans égalité, et porte séparée pour les nuages à égalités ;
- mreach + tête = HDBSCAN pour min_samples ∈ {2, 3, 5, 8} ;
- consommateur = tour (version corrigée) ; E5 et témoin à 5 points ; oracle C∩X brut ;
- **E1 exécutée sur les graines de développement**, reçu, décision au registre (D-35).

**V10-G — Catalogue GPU (piste ouverte par D-K5).**

Entrée : D-K5 ; P14 écrit (entier pur sur l'appareil, ou filtres à bornes prouvées avec `--fmad=false`).

Sortie : égalité des multiensembles d'enregistrements (S\*, I, U, rang) contre le CPU sur les trames et à
8k/16k/32k ; feuilles de m > 32 rendues au CPU et comptées ; reçu G4.

**V10-4a — Performance CPU sur G4 (J11–J24).**

Entrée : V10-2 close, D-K5 écrite.

Travail : profil par étage, constantes (arbre de centres, feuilles), résolution dédoublonnée. Balayage intra-ordre
seulement si T2 dépasse 30 % de B2 (P13 écrit avant le code).

Sortie :

- (i) **si les 30 trames de test sont présentes** : C(K5, 1 s) et C(K10, 1 s) évalués en `cpu48` selon EVAL § 12.6
  (p95 ≤ T, maximum ≤ 1,2 T, 100 % des passes digest-égales, aucun refus), **verdict publié quel qu'il soit** avec
  B0. Chaque trame mesurée a son épingle v9 (P9) et sa référence jugée. RSS dans le § 5.6. Trames brutes
  homologues : B2, B0 et RSS publiés (régime secondaire), correction jugée à l'identique ;
- (ii) **sinon** : la phase se clôt en **diagnostic** sur les 3 trames de conception et la brute 08/000000 (maximum
  sur trames et passes, jamais un p95). Le contrat reste OPEN avec la raison « données » ;
- dans les deux cas : toutes les passes digest-égales à la référence jugée ; portes de compteurs locales vertes.

**V10-4b — GPU.**

Entrée : 4a close, ou V10-G avancée ; profil `cpu48` propre montrant un étage > 30 % de B2 ; P14.

Travail : noyaux HD (feuilles, arbre, étage G, M, T1, Q), session résidente, repli CPU par élément, compté.

Sortie : CPU = GPU au digest sur toutes les trames mesurées et à 8k/16k/32k ; contrats selon EVAL (mêmes cas (i) et
(ii)) ; décomposition mesurée contre le budget 100 ms (§ 6.8) et note de décision.

**V10-5 — Campagne finale contre HDBSCAN (J16–J24).**

Entrée : V10-3 close (E1 faite) ; préenregistrement commis avant tout calcul sur les graines d'évaluation.

Protocole EVAL § 3–7 : familles, niveaux, bruit, n ∈ {2k, 8k, 16k, 32k} ; adversaires HDBSCAN par défaut, apparié et
`hdb_dev` ; métrique ARI_ss, etc.

Sortie : résultats publiés avec reçu, **quel qu'en soit le signe** ; ligne `experimental_target` du registre à jour ;
README formulé selon D-35.

### 18.3 Règle de revendication (D-35)

« Bat HDBSCAN » ne s'écrit qu'avec un reçu, des graines d'évaluation disjointes, l'adversaire `hdb_dev` réglé sur
`dev` et la même politique de bruit pour tous (EVAL § 6.6). « Grâce à la tour » exige en plus qu'E1 donne un IC de
Δ(tour − mreach) strictement positif. Sinon, le livrable recommandé est la tête sur mreach, et la tour est présentée
pour ce qu'elle apporte d'établi : objet exact, verticales, niveaux exacts.

### 18.4 Sortie commune à toutes les phases

- PASSATION réécrite ; reçu de sortie (labels exécutés listés) ; registre à jour ;
- aucun élément OPEN qui cite un reçu exécuté sans le marquer ;
- `docs/implementation_status.toml` non touché (exploration hors registre).

---

## 19. Obligations de preuve

| Id | Énoncé | Avant | Statut visé | Fixtures et juges |
| --- | --- | --- | --- | --- |
| P1 | Gardes (K sites quelconques) et dominateurs (K sites strictement plus proches sur toute la boîte), pondérés ; propriété par boîte demi-ouverte (GEN § 2.2–2.5) | V10-1 | proved_here | oracle brut, mutants garde/dominateur |
| P2 | Le catalogue cat_kmax suffit pour π0 de L_K, K ≤ kmax (sites distincts) | V10-1 | proved_here (repris) | oracle brut, T2 |
| **P2b** | Transfert : adm_k monotone, donc restrict(cat_{k'}, adm_k) = cat_k (§ 12.5) | V10-1 | proved_here | J-KM2, mutant `dominator_threshold_k_minus_1` |
| **P2w** | Multiensembles : cat_kmax plus les boules de rayon nul (une par site, fenêtre [1, min(kmax, w_s)]) suffisent ; fenêtre basse p_w + q − 1, avec q compté en **positions** (TOWER § 2.9) | V10-2 | proof_obligation → proved_here | paire (3,3), triangle (3,1,1), octaèdre pondéré, point de poids K, continuation a×2 + b (annexe A) |
| P3 | Table des largeurs (§ 17.5), filtre de niveau à marge 2^-46 | V10-0 | proved_here + static_assert + selftest | coins u18 |
| **P3a** | Lemme Z (§ 17.2) | V10-0 | proved_here | 10^6 cas contre BigRat |
| **P3b** | Voie rare U256 : tout test à centre q3 hors du recensement passe en U256 | V10-0 | proved_here | triangles quasi plats de côté ≈ 2^18 |
| **P3c** | Élagage exact d'un pavé entier (§ 17.4) | V10-0 | proved_here | `lattice_prune_check.py`, porte C++ |
| P4 | Aucun préfiltre flottant dans le générateur ; seules des formes exactes (GEN § 4) | V10-1 | proved_here | mutant de signe du lemme Z |
| P5 | Tour depuis le catalogue (TOWER PO-T1–T8, T12, T13) | V10-2 | conditional_theorem → proved_here | T2, fixtures |
| **P5w** | = PO-T16 : Th. 2 sur multiensembles, quotient pondéré, lots pondérés | V10-2 | proof_obligation → proved_here (lève `duplicate_positions`) | T2 pondérée ≥ 200 sites de poids ≥ 2 |
| P6 | Descente (PO-T4) | V10-2 | proved_here | rayon égal, portail |
| P7 | Lots atomiques (PO-T6, PO-T8) | V10-2 | proved_here | E5, A–E, ABCZ |
| **P7w** | Lots avec copies : une cellule dont la coquille porte une position de poids ≥ 2 peut être une continuation (exemple de la critique : a×2, b diamétral, K = 2 → continuation de contribution {b}) | V10-2 | proved_here | fixture annexe A |
| P8 | Verticales à la coupe fermée, naturalité (PO-T9) | V10-2 | conditional_theorem → proved_here | inert_ball |
| P9 | Continuations : la voie générale les traite ; un catalogue régulier non pondéré n'en produit pas | V10-2 | proved_here | ABCZ |
| P10 | C∩X (PO-T11) | V10-3 | proved_here | oracle C∩X brut |
| P11 | d_K(x)/2 ≤ α_K(x) ≤ d_K(x), entrelacement de facteur 2 | V10-3 | proved_here | — |
| P12 | À K=1, masses unitaires, z = 1, **sans égalité de d² ni de stabilité**, la condensation v10 coïncide avec HDBSCAN(min_samples=1). Avec égalités : règle écrite (plateaux N-aires côté v10) et comparaison hors niveaux d'égalité | V10-3 | proved_here + porte | porte K=1 |
| P13 | Balayage intra-ordre = lots (si utilisé) | V10-4a | proof_obligation → proved_here | T2 |
| P14 | Filtres sur l'appareil certifiés sous `--fmad=false`, ou entier pur ; repli exact | V10-G/4b | proved_here | porte `device` |
| P15 | Préenregistrement (engagement) | V10-5 | — | — |
| **P16** | Ordre canonique total : (rang, S\*) est injectif, parce que S\* détermine la boule (GEN § 1.2) | V10-0 | proved_here | invariant `canonical_order_duplicate` |
| **P17** | Euler pondéré : n·[K=1] devient au moins #{sites de poids ≥ K}, plus des termes de coquille multiensemble | — | **open** (O-W2) ; I2 et l'étape (3) de J-KM2 sont désactivés et déclarés sur entrée pondérée | — |
| **P18** | Lemme O : la liste certifiée d'une feuille contient N_k(c) fermé pour tout c de la feuille et tout k ≤ kcat (GEN § 2.7) | V10-1 | proved_here | juge `knn_closed` contre force brute |
| **P19** | Déterminisme de l'issue sous budget logique (§ 4.4) | V10-0 | proved_here | porte W1/W2/W8 à budget serré |
| **P20** | Transfert du digest : `engine_digest` couvre catalogue et tour, donc une passe digest-égale à une référence jugée calcule le même objet, à collision SHA-256 près | V10-0 | proved_here (trivial, écrit) | mutant `skip_stage_resolve` |

---

## 20. Travail en parallèle avec des agents (correction de B1)

### 20.1 Rôles et propriété des dossiers

| Rôle | Possède | Ne touche pas sans l'intégrateur |
| --- | --- | --- |
| Intégrateur | `include/`, `CMakeLists.txt` racine, `cmake/`, `README`, `PASSATION`, `SPEC_V10`, en-têtes contrats | — |
| A — géométrie | `src/arith`, `src/cloud`, `src/catalogue`, `bench/families.hpp`, `bench/proto/` | `src/tower` |
| B — tour | `src/tower`, `tests/v9diff` | `src/catalogue` |
| C — clustering | `src/points`, `src/head`, `python/`, `bench/synthetic`, `docs/BANC.md` | `src/tower` |
| D — vérification | `tests/bigint`, `tests/oracle`, `tests/judges`, `tests/fixtures`, `tests/mutants`, `tests/support` | le code produit (indépendance des juges) |
| E — infrastructure puis GPU | `.github/`, `tools/`, `gcp-migration/v10_*`, `bench/probe.cpp`, `alloc/`, `src/io`, `src/gpu` | le reste |

D écrit les juges **sans lire l'implémentation** jugée (seulement SPEC et PREUVES).

### 20.2 Worktrees clairsemés

Mesures :

- un checkout complet d'`origin/main` pèse 1,81 Go (68 233 fichiers, dont v8 1 045 Mo, v7 420 Mo, Zoltan 124 Mo) ;
- `/workspaces` a 6,3 Go libres sur 63 Go (90 %) ;
- six worktrees complets saturent le disque (critique B1, fondée).

Recette. Elle a été exécutée ici dans un clone jetable du scratchpad : **29 Mo** au lieu de 1,81 Go (fichiers racine
compris, qui sont surtout des PDF).

```bash
git -C /workspaces/E-HGP fetch origin
tools/disk_gate.sh 1500                                   # refuse si moins de 1,5 Go libres (§ 20.3)
git -C /workspaces/E-HGP worktree add --no-checkout --detach /workspaces/E-HGP/build/v10-wt-<rôle> origin/main
git -C /workspaces/E-HGP/build/v10-wt-<rôle> sparse-checkout set --cone \
    morsehgp3D_v10 tools gcp-migration .github docs/math
git -C /workspaces/E-HGP/build/v10-wt-<rôle> checkout --detach origin/main
```

Remarques :

- `sparse-checkout set` dans un worktree active `extensions.worktreeConfig=true` dans la configuration **partagée**
  du dépôt (observé dans le clone de test). C'est sans effet sur les autres worktrees pour git ≥ 2.20 ; le local a
  git 2.53. L'arbre principal reste complet.
- Les documents hors du cône (`docs/SPECIFICATION_MORSEHGP3D.md`, sources v7 et v9) se lisent sans checkout, par
  `git show origin/main:<chemin>`, ou dans l'arbre v9 existant `build/v9-open-worktree`, en lecture seule. Cet arbre
  sert aussi de `MHGP10_V9_SOURCE` : un seul arbre v9 pour tous les rôles.
- L'intégrateur peut travailler dans `build/v9-open-worktree/morsehgp3D_v10`, où existe déjà un échafaudage non
  suivi : aucun checkout supplémentaire.
- Jamais de branche. Jamais `git add -A`. Toujours vérifier `git diff --cached --quiet` avant `git add`. Jamais
  `git worktree prune` ni `gc --prune=now` : des commits d'agents orphelins sont à conserver.

### 20.3 Budget disque par rôle et porte disque

| Poste | Par rôle | Remarque |
| --- | ---: | --- |
| worktree clairsemé | ≈ 30–40 Mo | mesuré à 29 Mo, plus la v10 elle-même |
| build Release (bibliothèque, 5 exécutables de tests, sonde) | ≤ 150 Mo | les arbres Release v9 font 68 à 141 Mo pour 169 exécutables |
| build ASan/UBSan | ≤ 300 Mo | `v9-asan` : 224 Mo |
| build `receipt_<nom>` | ≤ 150 Mo, temporaire | supprimé après le reçu |
| build des mutants (rôle D seulement) | ≤ 600 Mo, temporaire | environ 26 bibliothèques unitaires ; supprimé après l'exécution |
| **total stable par rôle** | **≤ 0,5 Go** | |
| **pic, 5 rôles + D en mutants + cache de données + 2 sessions G4** | **≈ 3,5 Go** | sur 6,3 Go libres |

- `tools/disk_gate.sh <Mo>` s'exécute avant tout configure et avant toute session. Si
  `df --output=avail /workspaces` est inférieur au seuil (1 500 Mo pour un build, 3 000 Mo pour une session G4), il
  refuse avec la liste des builds **du rôle** les plus anciens. Un agent ne supprime jamais le build d'un autre.
- Gros fichiers transitoires (catalogues dumpés, sorties de sonde brutes) : dans le scratchpad `/tmp` (231 Go libres,
  volatil), jamais dans `/workspaces`. Jamais de reçu dans `/tmp`.
- Le nettoyage des 22 Go de `build/` (arbres v4 à v8 et audits v9 anciens, selon les critères du 23 septembre)
  libérerait une marge confortable. Il demande l'accord de l'utilisateur (question 6).

### 20.4 Poussées et auditeur

- Commits petits, porte `fast` verte en local, puis `tools/sync_push.sh`. Le script refuse un index non vide ou un
  arbre sale hors des dossiers du rôle, fait `fetch`, `rebase origin/main`, reconstruit, relance `fast`, puis pousse
  `HEAD:main`. Il s'arrête au premier conflit.
- Contrats gelés à la sortie de V10-0. Aucun WIP poussé.
- L'auditeur travaille dans `morsehgp3D_v10/audits/` et pousse sur `main`. Il reçoit les questions par
  `QUESTION_CLAUDE_*`. Ses dépôts se lisent avant toute dépense G4.

---

## 21. Documents

| Document | Contenu | Règle |
| --- | --- | --- |
| `README.md` | ce qu'est la v10, cadre, statut en une ligne, construction, trois commandes | ≤ 80 lignes |
| `PASSATION.md` | cadre ; acquis (résultat → reçu → porte) ; carte ; chantiers ouverts ; état G4 et sessions ; décisions D-K5 et E1 ; pièges | ≤ 300 lignes, réécrite à chaque phase |
| `docs/SPEC_V10.md` | objet autonome, dont les multiensembles ; divergences argumentées avec la thèse | ≤ 800 lignes |
| `docs/PREUVES_V10.md` | P1–P20 (§ 19) | une section par lemme |
| registre global | section v10 (annexe D) | à jour AVANT tout changement de statut |
| `docs/PROVENANCE.md` | ports : `run_expect.cmake`, `cble.cpp`, `census_tower_oracle.hpp`, `euler_scale_gate.cpp` (protocole), fixtures | un port non listé est un défaut |
| `docs/PROTOCOLE_MESURE.md` | renvoie à EVAL § 12 ; sessions C0, C1, P9 ; reprise | |
| `docs/BANC.md` | protocole préenregistré | commis avant les graines d'évaluation |
| `docs/FAUSSES_PISTES.md` | une ligne par piste close (WSPD indexé par arêtes, repli Gabriel E5, FNV sur mots, …) | pointeur vers reçu ou fixture |

---

## 22. Tailles de code estimées

| Couche | Lignes (cible) | v9 équivalent |
| --- | ---: | ---: |
| core + sched (budget, pool de tranches) | 1 000 | dispersé |
| arith (larges, lemme Z, P3c, largeurs) | 1 300 | ≈ 1 000 |
| cloud | 700 | 3 index |
| catalogue (dont oracle de feuilles, voie U256) | 2 000 | 17 534 |
| tower | 2 300 | 6 858 |
| points + head | 2 400 | Python |
| api + ABI C + io (SHA-256, Merkle) | 1 300 | — |
| cli + sonde | 900 | 1 200 |
| **Produit CPU** | **≈ 11 900** | 33 490 |
| gpu | 2 500 | 6 341 |
| tests C++ | 7 500 | 33 768 |
| Python (liaison, banc, tête, outils, G4 dont reprise) | 3 300 | ≈ 15 000 |

---

## 23. Risques

1. **Constante du catalogue** (§ 6.3, § 6.4) : le prototype à 14,6 µs ne passe C(K5, 1 s) sur la pire trame que si
   ρ · 24 / S48 ≲ 0,55. La cible d'ingénierie (≤ 4 µs) n'a pas de reçu. Parade : C0 d'abord, D-K5, V10-G ouverte tôt.
2. **K10 en CPU seul** : exclu dans presque tous les scénarios. Le GPU est nécessaire, pour le catalogue puis pour
   l'étage G.
3. **Trames brutes** : K10 brut à ≈ 11,4 M boules (estimé) demande aussi la tour sur GPU ; c'est non chiffré en
   mesure.
4. **Aucune borne de pire cas** pour l'arbre de centres (grille 32³ : 48 à 77 µs par boule). Parade : portes de coût
   adverses, budget logique, `wide_leaf`.
5. **Balayage séquentiel T2** : plafond de 7–14 ms (K5) et 20–40 ms (K10) estimés ; il faut P13 pour 100 ms.
6. **E1 négative** : la tour pourrait ne rien apporter face à mreach à tête égale (D-35).
7. **Multiplicités** : P5w non prouvé ; le refus transitoire est accepté.
8. **Différentiel v9** : l'exporteur exige Boost (`BOOST_ROOT` local ; sur la VM, en-têtes empaquetés dans la session
   P9). La v9 a elle-même l'angle mort Kmax : un désaccord déclenche une enquête, pas un verdict.
9. **Données** : les 30 trames de test manquent ; le contrat est bloqué (§ 16).
10. **GPU** : `__int128` émulé sur l'appareil, FP64 faible ; les projections ×10 de la v9 invitent à la prudence.
11. **Disque** : 6,3 Go libres ; la porte disque et le clairsemé tiennent 3,5 Go de pic, sans marge pour un
    imprévu de 3 Go.
12. **Rechute de méthode** : parée par les contrôles mécaniques de `check_v10.py`, pas par la documentation.

---

## 24. Questions ouvertes pour l'utilisateur

1. **Régime contractuel des chronos** : je lis la décision datée du 21 septembre (20:10 UTC), qui prévaut selon
   `AGENTS.md` § Ouverture v9, comme ceci : les contrats 1 s et 100 ms portent sur les trames **sans sol** ; les
   trames brutes entières sont un régime secondaire, publié et jugé sans être une porte de phase. Faut-il plutôt que
   les trames brutes soient aussi contractuelles ? Mesuré : 2,82 M boules à K5 sur 08/000000 brute.
2. **Données** : fournir, hors Git, les 30 scans bruts des séquences 00–07, 09 et 10 (liste EVAL `FRAMES_v1`), ou
   autoriser le téléchargement temporaire de l'archive velodyne de KITTI odometry (≈ 80 Go dans `/tmp`) pour en
   extraire ces scans.
3. **Frontière du contrat** : valider B2 résident (EVAL § 12.2) comme contrat, avec le mur froid B0 publié à côté.
4. **Revendication clustering** (D-35) : si E1 ne montre aucun gain de la tour à tête égale, le livrable « bat
   HDBSCAN » est-il la tête sur mreach ?
5. **Ouverture v10** : autoriser la section « Ouverture v10 » d'`AGENTS.md` et la mise à jour de la cible de
   `CLAUDE.md` (encore v5).
6. **Disque** : autoriser un nettoyage de `build/` (22 Go) selon les critères du 23 septembre, pour les arbres v4 à v8
   et les builds d'audit v9 de plus de sept jours non cités ?
7. **Multiplicités** : accepter qu'une entrée à doublons soit refusée explicitement (`duplicate_positions`) jusqu'à
   la sortie de V10-2, sachant qu'aucune trame LiDAR mesurée n'a de doublon à 1 mm ?

---

## Annexe A — Fixtures permanentes

| Fixture | Source | Garde |
| --- | --- | --- |
| E5 (5 points) | v9 `tests/fixtures/regressions/gabriel_point_set_counterexample.json` | repli Gabriel faux ; toute tête |
| régression silencieuse A–E : A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0), Kmax=2 | v9 `docs/HERITAGE_V7_V8.md` § 4 | attaches silencieuses |
| témoin 5 points K=2 : (4,11,5), (5,2,5), (8,2,2), (11,3,1), (11,5,1) | L06 `min_witness.py` | consommateur à 2 racines |
| 4 points non Gabriel | v9 `061d1ac9c` | connexité, convention de facettes |
| ABCZ (continuation) | `audits/b_full_continuation_origin_20260927` | continuation conservée |
| carré de côté 2 (e_K = (1, −3, 1, 1)), carré K2 à quatre parents | v9 `full_ball_tower_gate.cpp:128-151` | multifusion N-aire |
| coquille à 7 points | idem | p + u ≤ smax n'est pas une admission |
| pair, square, growth_redundant, inert_ball, support3/4, u16/u18_tetra, u18_corners, portal_equal, actual_equal_radius_descent, inert_ball_doubled_lot, present_but_wrong_rank, post_seed_square_partial | idem, l. 569-573 | tour |
| triangle rectangle AB/ABC ; courbe des moments facette 048 ; MEB K7 | registre v9 (L01) | niveaux, MEB |
| contre-exemples Euler/Kmax+2 à 13 et 23 points | v9 `ETAT_COURANT.md:1569-1580` | Euler nécessaire seulement |
| tétraèdre sans face q3 acceptée (K3) ; 75 cosphériques ; racine sur borne intérieure ; carré coplanaire ; quadrilatère AC/BD à p = Kmax−1 | L03 | générateur |
| cube à 4 diagonales ; {0,5,10,11} K2 ; rangées ; m² feuilles q2 | L02 | q2 |
| collision (0,1,0)/(0,0,65536) ; coordonnée 262 144 refusée | L15 | domaine u18 |
| jouet de condensation à 8 facettes ; parent scindé K2 (`ce8a649dd`) ; plateau à trois cofaces ; triangle Gabriel isolé | L08, L11 | tête |
| pondérées : paire (3,3), triangle aigu (3,1,1), octaèdre à poids mixtes, point de poids K, tous identiques | GEN F8, TOWER § 12.8 | multiplicités |
| **continuation pondérée** : a de poids 2, b de poids 1, diamétraux, p = 0, K = 2 : {a,a} naît au niveau 0 ; la cellule (ab, 2) est une continuation de contribution {b} au niveau \|ab\|²/4 | critique M4-b | P7w |
| **J-KM2** : plus petit nuage (tiré par l'oracle brut) sur lequel `dominator_threshold_k_minus_1` change cat(kmax) sans changer Euler sur cat(kmax+2) restreint aux ordres ≤ kmax | critique B2 | P2b |
| droite q4 tangente à une face de boîte ; triangle quasi plat de côtés ≈ 2^18 | § 17.2, § 17.3 | P3a, P3b |
| consommateur K=2 à deux points (composante sans point entré) | constat mineur | porte consommateur = tour |

## Annexe B — Schéma de run

Le schéma unique est `mhgp10_run_v1`, défini par EVAL § 12.4. Il remplace `mhgp10_probe_v1` d'ARCH_v1. ARCH ajoute
les champs suivants, de façon additive :

- `memory.data_budget_bytes`, `memory.session_overhead_bytes` (F(W)), `memory.peak_logical_bytes` ;
- `status` (ligne JSON du § 11.2, étage compris) ;
- `digests.engine` (Merkle SHA-256), `digests.catalogue`, `digests.tower_v10`, `digests.id` ;
- `judges` : références seulement (`emst`, `kmax_plus_2.euler`, `kmax_plus_2.restriction`, `boxes`, `local`,
  `coherence`, `v9_pin`) ;
- `calibration` : sessions C0 et C1 (`rho`, `s24`, `s48`, `c1_us_per_ball`).

Règle : additif ; la version ne change que si le sens d'un champ change ; au plus 3 versions sur la vie de la v10.

## Annexe C — Schéma `mhgp10_receipt_v1`

Champs obligatoires :

- identité : `schema`, `name`, `date`, `commit`, `tree_clean` ;
- construction : `recipe {cmake, compiler, flags}`, `binaries [{path, sha256}]` ;
- environnement : `host {cpu, threads, ram_gb, gpu, python, numpy, sklearn}`, `inputs [{sha256, n_returns, n_sites,
  source}]` ;
- exécution : `commands [str]` (ordre réel), `labels_run [str]`, `runs [{command_index, run_line_sha256, status}]`,
  `summary {…}` ;
- statut : `public_status: "not_claimed"`, `gcp {used, session_dir, generation, terminated_certified}`.

Taille ≤ 2 Mo avec `runs.jsonl` ; aucun octet de nuage.

## Annexe D — Section v10 du registre des preuves (lignes initiales)

| Énoncé | Statut initial | Porte ou fixture |
| --- | --- | --- |
| composantes de Γ_K = π0(L_K(a)) | theorem_external (Th. 2) | T2 |
| catalogue critique suffisant (P2) ; transfert de restriction (P2b) | proved_here | oracle brut ; J-KM2 |
| catalogue et boules de rayon nul suffisants pour les multiensembles (P2w, P5w, P7w) | proof_obligation | T2 pondérée, fixtures pondérées |
| lemme des boîtes de centres, pondéré (P1) ; lemme O (P18) | proof_obligation → proved_here | oracle brut, juge des boîtes, mutants |
| largeurs 18 bits, lemme Z, voie U256, élagage P3c (P3–P3c) | proved_here | selftests, static_assert |
| tour depuis le catalogue, cas régulier (P5) ; extension non régulière | conditional_theorem ; proof_obligation | T2 |
| descente, lots, continuations (P6, P7, P9) | proof_obligation → proved_here | T2, ABCZ |
| verticales, naturalité (P8) | conditional_theorem | T2, cohérence points–verticales |
| identité d'Euler par le nerf, sites distincts | proved_here (nécessaire seulement) | I2, J-KM2 |
| identité d'Euler pondérée (P17) | open | — |
| K=1 = EMST, niveau d²/4 | proved_here | juge EMST |
| C∩X (P10) ; entrelacement de facteur 2 (P11) | proof_obligation | oracle C∩X brut |
| condensation = HDBSCAN à K=1 sans égalités (P12) | proof_obligation → proved_here | porte scikit-learn |
| déterminisme de l'issue sous budget (P19) ; ordre canonique total (P16) | proved_here | portes |
| repli sur les seules cofaces Gabriel préserve π0 | false_in_general | E5, témoin à 5 points |
| coût linéaire en la sortie du générateur par centres | heuristic / experimental_target | portes de coût adverses |
| C(K5, 1 s) en CPU seul sur trames sans sol | experimental_target (D-K5) | reçus C0, C1, contrat |
| la tête sur la tour bat la tête sur mreach (E1) ; la v10 bat HDBSCAN | experimental_target | reçus E1, V10-5 |

## Annexe E — Décisions (identifiants ; une étoile marque une décision modifiée par cette révision)

- D-01 base neuve, ports explicites seulement.
- D-02 générateur par boîtes de centres, WSPD et `s` abandonnés.
- D-03\* deux index à propriétaire unique : `LeafOracle` (centres rationnels, tour) et `SiteTree` (requêtes
  entières) ; `CenterTree` transitoire ; un espace `SiteIdx`.
- D-04\* niveaux en rangs `u32`, niveau exact recalculé depuis S\*, clé `double` transitoire.
- D-05\* multiplicités dans l'objet, refus explicite `duplicate_positions` jusqu'à PO-T16.
- D-06 quotient de plateau sans plafond silencieux, U256.
- D-07\* cinq statuts ; `catalogue_incomplete` devient `invariant_violated/*`.
- D-08 publication tout-ou-rien, issue du plus petit (étage, K, ordinal).
- D-09 un `Pool` par `Session`.
- D-10\* déterminisme par ordinal, R1 en invariance et équivariance, R7.
- D-11 un algorithme de tour quel que soit W.
- D-12\* liste blanche de paramètres (sans `HierarchyParams`, avec `IntrinsicDimParams`).
- D-13 pas de mode `verify` ; J-KM2 hors produit.
- D-14\* mutants par copies mutées, enveloppe « code 4 ».
- D-15 pas de Boost dans le produit.
- D-16 liaison Python par ABI C et ctypes.
- D-17\* CMake 3.22, CI g++ 11, Python 3.12 en CI.
- D-18 cinq exécutables de tests en build unitaire.
- D-19 labels par contenu.
- D-20\* digests SHA-256 (moteur par Merkle, canoniques) ; FNV abandonné.
- D-21\* reçus depuis un commit poussé, hors `/tmp` ; CI qui ne vérifie que la forme des reçus modifiés.
- D-22\* schéma unique `mhgp10_run_v1` (EVAL).
- D-23\* session G4 depuis un commit, binaires capturés, répertoire persistant, reprise sans redémarrage.
- D-24 contrat B2 résident sur trames distinctes (à confirmer).
- D-25 documents courts, pas de journal.
- D-26 `experimental/` et promotion par ablation.
- D-27 tête sur contrat abstrait ; un seul résolveur (tour).
- D-28 mreach et juge EMST par Borůvka entier.
- D-29 kmax ≤ 10, catalogue interne ≤ 12.
- D-30 pas de mode « tranche K seule ».
- D-31\* budget logique déterministe et frais de session publiés.
- D-32 GPU à en-têtes HD, repli par élément.
- D-33\* worktrees clairsemés, porte disque, propriété par dossier, doublures.
- D-34 CI rouge = arrêt.
- D-35 règle de revendication liée à E1.
- D-36 banc préenregistré.
- D-37\* CPU d'abord, mais GPU ouvert par D-K5.
- D-38 aucun octet KITTI.
- D-39\* rôles des tailles, et trames brutes comme régime secondaire publié.
- D-40 limites mécaniques de fichiers.
- **D-41** protocole J-KM2 (Euler sur kmax+2 et restriction).
- **D-42** calibration C0 avant le générateur de production.
- **D-43** matrice de couverture et épingle v9 par trame mesurée.
- **D-44** lemme Z en i128 ; test par paramètres de droite interdit.
- **D-45** allocation par substitution à l'édition de liens.

## Annexe F — Constats mineurs de la critique

| Constat | Verdict | Traitement |
| --- | --- | --- |
| Octets du catalogue sous-estimés (26 et non 24 ; niveaux, `base` omis ; `anchor` par ordre ; `SiteTree` 2n nœuds contre feuilles ≤ 16) | fondé | § 5.2 (11 o fixes + 4 par identifiant + ≤ 4 de niveaux) ; atlas et ancres selon TOWER (`base` global, `anchor` par ordre via `by_k`) ; `SiteTree` ≈ n/8 nœuds (§ 5.1) |
| `mhgp10_gate CODE <0\|1\|2\|3>` sans codes 4 et 5 ; conflit avec la convention « 4 = mutant tué » | fondé | CLI : 0/2/3 ; 1 réservé aux juges ; 4 réservé à l'enveloppe de mutant ; `CODE <0..4>` (§ 9.5, § 10, § 11.2) |
| `#if` interdit, mais empoisonnement « en build de test » | fondé | substitution à l'édition de liens (§ 4.3) |
| `receipt.py check` sur tous les reçus en CI | fondé | `--changed`, forme seulement (§ 14) |
| scikit-learn 1.9.1 et numpy 2.5 exigent Python ≥ 3.11 et ≥ 3.12 ; runner et VM en 3.10 | fondé | `setup-python` 3.12 ; aucun test Python du banc sur la VM (§ 8, § 13) |
| Porte « consommateur = tour » fausse à la lettre | fondé | restreinte aux composantes contenant un point entré (§ 12.4) |
| Égalité « K=1 = HDBSCAN » fragile aux égalités | fondé | nuages sans égalité et porte séparée (§ 12.4, P12) |
| Mode d'arrondi non gardé ; FMA de nvcc | fondé | `fp_environment`, `--fmad=false` (§ 17.6) |
| Feuilles non bornées à côté 1 ; m ≤ 32 pour le GPU | fondé | T = 6, `M_hard = 128` puis `wide_leaf` (GEN) ; m > 32 rendu au CPU et compté (§ 6.1) |
| Entrée LiDAR : sites ou retours ; quel `PointId` | fondé | § 5.1 : retours, `PointId` = indice du retour, tous conservés ; différentiel v9 limité au poids 1 |
| `tower_degen.py` n'est pas un oracle | fondé | § 12.1 |
| R1 : équivariance et non invariance | fondé | § 7.2 |
| HGP-old en salle blanche | fondé | § 8 |
| Obligations manquantes : ordre canonique, transfert, élagage, Euler pondéré | fondé | P16, P2b, P3c, P17 (§ 19) |
| 18,5 µs sans artefact | fondé | retiré ; mesures régénérées citées (§ 6.3) |
| Incohérences d'API (`HierarchyParams`, `k` de ẑ, `Session::open` sans `Ledger`) | fondé | § 9.1, § 11.1 |
| (complément, issu de M3-b) `SiteTree` « arithmétique exacte i128 » | fondé pour ARCH_v1 | le `SiteTree` ne fait plus que des requêtes entières ; P3c montre qu'un élagage exact en i128 existe (§ 3, § 17.4) |
| (complément, issu de M9) oracle brut à n ≤ 60 impossible en CI | fondé | n ≤ 16 en CI, n ≤ 60 en sortie de phase (§ 12.1) |
| (complément, issu de M5) frais dépendant de W dans le budget | fondé | F(W) hors du budget logique (§ 4.4) |
