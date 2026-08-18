# PASSATION — état complet du chantier morsehgp3D_v4 au 18 août 2026

Document d'entrée pour la session qui reprend (Opus 5). Il relie les
trois plans — mathématique, implémentation, opérationnel — et pointe
vers les sources qui font foi. Rien ici ne remplace un document
normatif ; tout y renvoie. Pin de rédaction : `main@5c326c1`.

## 0. Où commencer, dans l'ordre

1. `AGENTS.md` (racine) puis `CLAUDE.md` — règles absolues (jamais de
   branche sans accord ; VM GCP uniquement par scripts gardés ; jamais
   de vérification exhaustive ; tailles d'intérêt 8000/16000/32000).
2. Ce document (vue d'ensemble et carte).
3. `docs/MATHEMATIQUES.md` — l'objet et les statuts de preuve.
4. `docs/ARCHITECTURE.md` — le plan initial (⚠ antérieur à une grande
   partie du pipeline réel ; le présent § 3 fait la mise à jour).
5. `audits/` en ordre chronologique inverse — le dialogue avec les
   auditeurs (qui poussent sur `main`) et ses réponses EXÉCUTÉES.
6. `receipts/` — chaque avancée a son reçu, ancré à un commit.

## 1. L'objet mathématique en bref

La hiérarchie HGP (manuscrit Hauseux, Défs 20–31, Th. 2–7 — pages PDF
35–134) : pour chaque ordre $K$, une forêt de fusion sur les
$(K-1)$-simplexes, dont les événements sont portés par des BOULES : une
boule $B$ contribue si son intérieur strict $I_B$ et sa coquille $U_B$
déterminent des plateaux sphériques. Réduction v4 : trois lanes de
génération suffisent —

- **q2** : boules diamétrales (paires) ;
- **q3** : circumboules de triangles aigus (l'arête max est l'ancre) ;
- **q4** : circumboules de tétraèdres (seed aigu + complétion).

Complétude sous seuils (`derive_v4`, § en tête de
`src/pipeline/ball_stream.hpp`) : un plateau pertinent pour
$K \leq 10$ a $\vert I_B\vert \leq K_{max} + 1 - q$ ; ses témoins de
fuseau $W_q(a,b)$ sont donc sous $h_q = s_{max} - q + 1$ : les filtres
$h_{coeur}/h_a/h_b$ ne perdent AUCUN plateau pertinent (Carathéodory
garantit un support minimal d'arité 2, 3 ou 4). Le rendu § 9.1 du
manuscrit : l'arbre est une partition laminaire des facettes, la
partition de l'unité $w_{x\tau} = S_\tau/T_x$ relie points et facettes.

Doctrine d'exactitude : entrée u16 quantifiée, TOUT prédicat décidé en
entier (i64/i128/U192/U320), aucun jitter, les dégénérescences
(plateaux cosphériques) sont réelles et traitées par quotient exact.
Le flottant n'existe que comme FILTRE certifié à repli exact (§ 2.4).

## 2. Les résultats acquis (théorème → code → porte)

Chaque ligne : l'énoncé, où il vit, la porte qui le grave. Les
dérivations complètes sont dans les reçus (`receipts/forest_20260817/`)
et les contre-audits.

### 2.1 Cover coefficient 3 et bornes de site

Le cover d'ancre $(a,b)$ contient tout $z$ avec
$\vert 2z - a - b\vert^2 \leq 3 D^2$ ; donc $\vert z-a\vert^2 < 2D^2$
(contre-audit `04c71a2` § 1, dérivation $((\sqrt{3}+1)/2)^2 < 2$).
Code : `rect_cover_handles` / `anchor_cover_from_handles`
(`src/events/edge_cover.hpp`). Portes : juges des petits n +
`--depth-gate`.

### 2.2 Cœur de seed (Jung)

Tout tétraèdre accepté a $R^2 \leq 3D^2/8$ ; un site $z$ avec
$P(z) < 0$ et $2P(z)^2 > J\,B(z)^2$ (où $J = D^2(3G - 2 E X)$) est
intérieur strict de TOUTE sphère admissible du seed — comptage
fail-open à $h_4$, l'ancre entière meurt par $W_4$. Comparaison exacte
en U320 (`cmp_2p2_jb2`). Code : lane q4 de `collect_candidate_balls`.
Portes : `--axial-pair-gate` + mutant `seed-core-nonstrict`.

### 2.3 Kernel affine par ancre (le chemin chaud actuel)

Par ancre : $u_z = 2z - a - b$, $q_z = \vert u_z\vert^2 - D^2$ —
entiers $< 2^{36}$, EXACTS en binaire64, remplis paresseusement au
premier seed (`LaneScratch::fill_affine_sites`). Par seed :
$N = W - G\,d$. Identité $L(z) = G q_z - 2\,u_z \cdot N = 4 P(z)$
(preuve : $P(a) = P(b) = 0 \Rightarrow d \cdot N = 0$), donc
$P = L/4$ exact pour Jung. Porte PERMANENTE `--q3-affine-gate` :
identité + divisibilité sur ~1,94 M de triples exhaustifs, témoin de
forte annulation ± gravé ($G = 2^{67} - 12345$, $L = +216577/-45565$).
Reçu : `ADDENDUM_KERNEL_AFFINE_20260818.md`.

### 2.4 Étage flottant certifié (borne PROUVÉE)

Séquence FIGÉE `affine_l_hat` (quatre arrondis, le doublement est
exact) ; erreur $< 6u\,M(z)$ avec $u = 2^{-53}$ ; seuil par seed
$E = 2^{-48} \cdot (G\,q_{max} + 2\vert N\vert_1 u_{max}) > 31u\,M$ —
marge $\times 5$, preuve REÇUE par les deux contre-audits `879B37`.
Décision : $\hat{L} < -E$ ou $> E$ certifie le signe, sinon repli
exact i128. Garde d'exécution : filtre coupé sous `__FAST_MATH__` et
si `fegetround() != FE_TONEAREST` (porte `--float-rounding-gate`).
Jung et `cmp_mu` n'utilisent PAS ce seuil (chantiers § 5.2).

### 2.5 Fold compact

Canonique par min-fid u32 (≡ min FacetKey car les fids suivent l'ordre
des clés), tables à époque pour les racines pré/post-lot, partition
finale DENSE (`facet_keys` + `final_canon_fid`, invariants
structurels), backend FIGÉ `build_forest_legacy` comme témoin. Porte
`--fold-compact-gate` (égalité complète, mutant `canonical-is-uf-root`).
`t_fold` 56,1 → 38,3 s à n=8000 ; l'internement (~26 s) est le poste
suivant (§ 5.3).

### 2.6 Parallélisme MESURÉ, jamais déclaré

Génération : tirage dynamique par rectangle (`run_rects`), compteurs
PAR LANE alimentés au point de création des `std::thread` ; aval :
`parallel_ranges` RETOURNE le nombre créé ; affinité CPU effective
publiée par `sched_getaffinity`. Sorties bit-identiques quel que soit
le découpage (tri stable + RLE ; fusion en ordre de tranche). Portes :
`--par-gate`, `--workers-gate` (mutants `parallel-hardcodes-one-worker`,
`parallel-ranges-one-worker`, `q3-one-worker`), validateur de campagne.

### 2.7 Poisson q2 et contrat de sortie

$E[N_j]/n \to 4$ par profondeur ; ~$2K(K{-}1)n$ facettes nées ;
`--q2-birth-gate` (l'égalité du second diamètre est un impossible-
théorème, donc une violation). Preflight `--output-preflight-only` +
plafond transactionnel `--max-output-bytes` (refus code 2 AVANT
allocation). ⚠ La GARDE DE CAPACITÉ du fold (casts u32/i32, audit
`5d274a1` § 7) reste À FAIRE — voir § 5.1.

## 3. Carte de l'implémentation

```
src/wspd/wavefront.hpp      arbre radix + WSPD, rectangles vivants par lane
src/events/                 formes exactes : q2/q3/q4 (instruction+event),
                            acute_seed, edge_cover (cover coef 3),
                            witness_count ; U192/U320 ; uabs/mul annotés
                            MHGP4_HD (préparation GPU)
src/pipeline/ball_stream.hpp  LE CŒUR : les trois lanes génératrices,
                            LaneScratch (sites affines paresseux),
                            étage flottant + garde d'arrondi, cœur de
                            seed Jung, sweep axial 2 côtés (opt-in GPU),
                            stats/timers/workers ; en-têtes = doctrine
src/forest/forest.hpp       fold compact + build_forest_legacy (FIGÉ)
src/forest/render.hpp       rendu § 9.1 (F_K^render, multiplicités)
src/forest/sphere_plateau.hpp  quotient exact des plateaux
bench/forest_probe.cpp      pipeline aval (RLE → préfiltre → census →
                            expansion → folds), TOUTES les portes CLI,
                            digest canonique, preflight
oracle/                     juge indépendant (arithmétique propre)
tests/                      selftests unitaires (obig, tree, forest…)
CMakeLists.txt              130 CTests — dont ~la moitié de portes
                            négatives à code EXACT (1/2/3/4)
```

Codes de sortie des portes : 1 = désaccord du juge, 2 = refus avant
calcul, 3 = plancher/invariant violé, 4 = mutant tué. Un CTest à
`PASS_REGULAR_EXPRESSION` est toujours doublé d'une porte à code.

Doctrine de test (v3, conservée) : planchers `--min-*` contre le
vert-par-vacuité ; fixtures gravées aux coordonnées exactes ; mutants
causaux tués ; équivariance par permutation ; oracles exhaustifs
bornés (n ≤ 12–14) qui ÉTABLISSENT la vérité ; jamais de
re-vérification de ce qu'un théorème garantit.

## 4. Performances connues (n=8000, smax=11, 4 fils, ce conteneur)

| famille | t_gen | notes |
|---|---|---|
| uniform | ~56-62 s | stable |
| eight_clusters | ~123-130 s | 2 658 325 événements ; jamais atteint avant v4 |

`t_fold` 38,3 s (batching 2, intern 21,5, reduce 17,8, partition 0,7).
Le cœur de seed domine la génération dense (~8,5 G d'évaluations i128
pour les sites certifiés négatifs — Jung exige l'exact). Le kernel
affine est CPU-neutre (bande ±3 s) : adopté pour la STRUCTURE (identité
gravée, contrat d'erreur serré, forme GPU warp-par-seed), pas pour la
constante — reçu `ADDENDUM_KERNEL_AFFINE` § mesures.

## 5. Chantiers ouverts, par priorité

1. **Garde de capacité du fold** (audit `5d274a1` § 7, ACCEPTÉ non
   exécuté) : refus `resource_exhausted` avant casts u32/i32
   (`events ≤ UINT32_MAX`, lots ≤ UINT32_MAX — sentinelle —, `nfid ≤
   INT32_MAX`), mutants `fold-u32-event-wrap`/`fold-i32-fid-wrap`/
   `fold-epoch-sentinel-collision` par bases artificielles.
2. **Intervalles de Jung** (audits E573888 § 1.2, 04c71a2 § 6) :
   certifier $\inf(2[P]^2) > \sup([J][B]^2)$ en flottant, repli
   `cmp_2p2_jb2` — le multiplicateur mesuré suivant (les 8,5 G
   d'i128). Puis schéma L/U à deux bornes (§ 5) et `cmp_mu` (borne
   propre ~$2^{114}$, l'ordre reste exact et transitif).
3. **Internement du fold en streaming** (~21 s à n=8000).
4. **GPU** (`src/gpu/device_compile_witness.cu` — compile-only,
   jamais encore compilé faute de nvcc ; plans
   `NOTE_CLAUDE_PLAN_GPU` + `NOTE_CLAUDE_PLAN_PARALLELISME_V2` :
   warp-par-seed, saturation par ballot, compaction vers passe exacte
   device ; l'oracle et le juge ne sont JAMAIS portés).
5. **Couches convexes q3** (`Q3ShallowHalfplaneIndex`) : DIFFÉRÉ par
   l'audit e27acfa § 2.3 — seulement si q3 domine encore après
   affine + intervalles, et sur les seules ancres lourdes.
6. Boule intérieure candidate $B(m, R-\delta)$ (réponse d'auditeur,
   dormant).

## 6. La campagne G4 « scale_threads » — protocole prêt, zéro reçu

Protocole (audits `9223888`/`b3a6eb4`/`66886c0`/`7d921ff`/`c9c3a48`,
tous EXÉCUTÉS) : `gcp-migration/session_scale_threads_g4.sh` →
préflight SIX gardes (dont les deux de `start_and_verify` modélisées,
constantes lues à la source, TTL dérivé du plafond, `--check-envelope`)
→ pin du protocole → inventaire « aucune VM » → démarrage gardé →
runner `v4_scale_threads_remote.sh` (argv haché NUL, workers mesurés,
digest canonique complet K=1..10) → validateur épinglé
`validate_v4_scale_threads.py` (SEULE autorité : liaison nom→argv→
identité imprimée, affinité effective, appariement par digest) →
arrêt certifié TERMINATED. Porte transactionnelle :
`selftest_scale_threads.sh` (15 refus causaux). Phases : `n32000`
(équivalence t1/t8/tmax), `n64000` (échelle, non appariée),
`court1h` (≤ 1 h, paires t8/tmax).

**Journal du 18 août** : cinq tentatives, zéro campagne exécutée,
CHAQUE VM certifiée TERMINATED — détail dans
`receipts/campagne_scale_threads_20260818/JOURNAL_TENTATIVES.md`.
Leçons durcies dans le code : préflight 4→6 gardes ; TTL dérivé ;
`terminationTimestamp` lu sous `resourceStatus.scheduling` ;
`europe-west4-a` préempte immédiatement (2×) → repli autorisé
`europe-west4-ai1a` (capacité prouvée) ; le bac à sable CCR bloque le
port 22 sortant (l'API passe, SSH jamais) → la campagne s'exécute
depuis un poste avec SSH (Codespace de l'utilisateur) ; un jeton
d'accès vit ~60 min (gcloud le met en cache — `rm access_tokens.db`
pour en frapper un neuf).

**Ligne prête** (depuis un poste avec auth + SSH, worktree propre sur
`main ≥ 5c326c1`) :

```bash
GCP_ZONE=europe-west4-ai1a GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a \
PHASE=n64000 RUN_TIMEOUT=600 BUILD_MARGIN=480 RETRIEVE_MARGIN=300 \
MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=55 SSH_KEY_TTL_MINUTES=66 \
./gcp-migration/session_scale_threads_g4.sh
```

Dernier état observé : le boot `-ai1a` est lent à monter `sshd`
(premier boot du disque) — si l'armement de la garde invité échoue,
relancer une fois (second boot plus chaud). ⚠ Conflit tranché pour
≤ 1 h seulement : la campagne est 100 % CPU sur une machine à GPU
(règle du 9 août) — un chemin gardé CPU-only reste une décision
UTILISATEUR.

## 7. Le processus de travail

- Les AUDITEURS poussent sur `main` ; leurs audits se LISENT et
  s'EXÉCUTENT avant toute dépense ; chaque réponse est un
  `REPONSE_CLAUDE_*`/`NOTE_CLAUDE_*` dans `audits/`, chaque livraison
  un reçu ancré au commit dans `receipts/` (immuable).
- Pousser régulièrement sur `main` (autorisé explicitement) ET sur la
  branche désignée de la session.
- Canal opérationnel GCP : branche `claude/g4-relais`, fichier
  `RELAIS.md` (sections en tête, jamais de secret).
- Jamais de jeton/clé dans le dépôt, les logs committés ou les
  messages ; les refus des scripts gardés sont finaux.
