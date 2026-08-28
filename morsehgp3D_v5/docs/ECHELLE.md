# MorseHGP3D v5 — Passage à l'échelle : dizaines de millions de points sur une G4

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`. Ce document
est une **conception chiffrée**, pas un claim : chaque chiffre cite un reçu ou
une mesure locale (ratios), et les théorèmes à prouver sont listés avant le
code (§ 6). Rédigé le 28 août 2026 à partir des reçus G4 n° 9–11, des mesures
locales du 28 août et des audits v3/v4 cités au § 2.

## 1. Les deux objectifs

1. **Principal — objet complet, `smax = 11`, K = 1..10** (dix forêts, profil
   d'entrée u16 inchangé), sur des nuages de 10 M à 30 M points.
2. **Secondaire — K = 1..5** (`smax = 6`, $h_q = 7 - q$), même code, profil
   **nommé** dans les reçus (jamais confondu avec l'objet complet ; un préfixe
   exact de la tour, § 3.3).

Machine cible (lecture seule du 28 août 2026) : `g4-standard-48` = 48 vCPU,
**180 Gio** de RAM, 1 × RTX PRO 6000 (96 Go), un disque persistant NVMe de
**100 Go** ; aucun SSD local attaché. Sessions gardées de **8 h au plus**
(`maxRunDuration`), donc tout calcul plus long doit être **reprenable**
(statut `incomplete_continuation`, flux sur disque persistant survivant à
l'arrêt).

## 2. Ce que la v3 et la v4 avaient déjà établi

- **v4, audit bloquant `C829` (17 août 2026)** — « la prochaine limite est la
  taille de l'objet, pas le prédicat » : 391 événements et 247 nœuds de fusion
  par point à 8000, ~11,7 G événements à 30 M ; il faut **nommer le produit**
  (`full_symbolic_stream` / `connectivity_hierarchy` / `query_or_labels`),
  préflight de cardinalité avant allocation, index 64 bits, plus de `ev_k`
  résident, séparer connectivité et rendu, garder `SpherePlateau` comme unité.
- **v4, contre-audit `772A8D`** — bornes de Poisson fermées : q2 seule impose
  en espérance $2K_{\max}(K_{\max}-1)$ facettes nées par point, soit **180 par
  point pour K = 10** (5,4 G à 30 M) et **40 pour K = 5** (1,2 G à 30 M) ;
  produit recommandé `facet_hierarchy_stream` (K-MST exact au niveau des
  facettes, streamé, output-sensitive), et un produit orienté requêtes (Th. 2
  du manuscrit : composantes de $L_K(r)$) comme seul candidat à un SLO court.
- **v3, réponses « route 50 k puis 10 M » (13 août 2026)** — un halo k-NN ne
  rend pas les tuiles indépendantes ; une coquille peut avoir $\Theta(n)$
  labels ; les événements inter-tuiles et les lots de même niveau exigent une
  **fusion globale** ; peuvent rester résidents : points, index, état du fold
  par ordre, fronts bornés ; les supports se streament.
- **doctrine produit** (`docs/GPU_G4_ARCHITECTURE.md`) : à 10–30 M, nuage et
  arbre résidents seulement si le préflight le démontre ; ancres, frontières,
  candidats et sorties streamés par plages Morton.

La v5 reprend ces conclusions telles quelles ; ce document ne les contredit
sur aucun point et y ajoute des mesures qui fixent les constantes.

## 3. Lois d'échelle mesurées

### 3.1 Par ordre et par point (reçu G4 n° 11, `uniform` 200 000, 48 fils)

| grandeur | total | par point | dont K = 10 |
|---|---:|---:|---:|
| boules uniques survivantes | 90 M | 450 | — |
| événements (K = 1..10) | 89,8 M | 449 | 105 |
| facettes (K = 1..10) | 562 M | 2811 | 912 |
| deltas | 62,8 M | 314 | 83 |
| attachements (naissances) | 468 M | 2340 | 787 |

Mémoire (paliers `rss_mb`) : 14 Go après génération, 34 Go après census
(boules 224 o), **66 Go au maximum du fold** (trois ordres résidents, ~11 Go
par ordre). Temps : 254 s (génération 69, préfiltre 31, census 19, mur du
fold 115 dont reduce séquentiel 1,4 µs par événement et digest 66 s cumul).

### 3.2 En fonction de l'ordre maximal (`uniform` 32 000, 8 fils, ratios)

| `smax` | K max | boules/pt | mur | pic du fold |
|---:|---:|---:|---:|---:|
| 5 | 4 | 49 | 25 s | 1,0 Go |
| 7 | 6 | 122 | 50 s | 2,4 Go |
| 9 | 8 | 243 | 96 s | 5,1 Go |
| 11 | 10 | 425 | 211 s | 8,8 Go |

Les cardinalités d'un ordre K sont **identiques** quel que soit `smax` ≥ K + 1
(vérifié à 32 000 sur `uniform` et `scanline` : K = 4 → 703 116 événements et
1 943 993 facettes dans les quatre runs) : la restriction est un préfixe exact
de la tour (§ 3.3). Le coût croît en ≈ K².

### 3.3 Le préfixe exact

La mort par profondeur $\left\vert I_B \right\vert \ge h_q = s_{\max} - q + 1$
ne tue que des boules dont tous les événements ont $K = q + d - 1 \ge s_{\max}$ ;
les ordres $K \le s_{\max} - 1$ sont donc inchangés. Un profil K = 5 est
l'objet complet **restreint** aux cinq premiers ordres, avec les mêmes
digests par ordre (`digest_forest[K]`) — la conformité du profil se prouve par
égalité de ces digests avec ceux d'un run `smax = 11` à 8000–32 000.

### 3.4 Facettes vivantes dans le fold (instrument `profil_vivantes`, 28 août)

Dans le fold d'un ordre trié par niveau, une facette n'est vivante (première
incidence passée, dernière à venir) que brièvement. Pic de facettes vivantes
par point, K = 10 : `uniform` 8000 16,8, 16 000 18,1 (+8 % par doublement) ;
`eight_clusters` 8000 13,3 ; `scanline` 8000 2,5. Fraction du total : K = 10
2,2 %, K = 4 10 %, K = 1 93 % (les singletons). **C'est la mesure qui rend un
fold à état borné possible** : à 10 M, ≈ 40 facettes vivantes par point
(marge ×2,2) contre 912 au total pour K = 10.

### 3.5 Rayons des boules survivantes (32 000, `smax = 11`)

Rayon maximal : `uniform` 22 unités (2,2 pas moyens), `scanline` 84 (2,4 pas),
`eight_clusters` **149 (7,4 pas, 23 % du domaine — les vides inter-amas)**. Un
halo par copie est donc impraticable sur les familles à vides ; la conception
laisse l'index **résident** et ne tuile que le travail (§ 4).

## 4. Architecture retenue (décision du 28 août, après trois conceptions et trois critiques adverses — `docs/analyses/echelle_20260828/`)

**Cœur : fold streamé par ordre à état borné**, alimenté par un **amont
streamé par seaux Morton du centre des boules** ; le profil nommé
`prefixe_k5` est le premier échelon livrable ; une **clé d'ex aequo
explicite** des événements est le préalable commun. Le tuilage spatial *du
fold* (halo $R_{\max}$, journal de mort par cellule) est **rejeté** : halo
dépendant des données (23 % du domaine sur `eight_clusters`), amplification
de lecture ×3 à ×27, ~5 To d'E/S à 10 M — alors qu'une passe de comptage
PREMIÈRE/DERNIÈRE par ordre, par tri externe, obtient l'oubli des facettes
sans géométrie ni halo. La table des comptes en RAM envisagée au § 4.2
initial (64 Go à 10 M) est remplacée par ce comptage externe.

Ce que l'objet exige et que le plan ne touche pas : mêmes lanes, même RLE
(clé, arité minimale, plus petit niveau), même census complet sous
`shell_cap`, même tri stable par niveau exact, même union-find séquentiel
(« la racine de `first` absorbe »), mêmes fid par tri global des clés, mêmes
plateaux par quotient. Seule la **résidence** change.

### 4.1 Produit nommé

- `payload = mhgp5-forests-stream-v1` (flux des deltas par ordre, dans l'ordre
  des lots de niveau, avec certificat de reconstruction de la partition) et
  digest `mhgp5-digest-stream-v1` ; le digest v4 reste l'autorité de
  conformité tant que le résident existe (≤ 1 M) et un **convertisseur** flux
  → tableaux v4 (hors produit) prouve l'égalité aux tailles où les deux
  existent. Output-sensitive : ≈ 1,6–1,8 To de clés développées à 10 M pour
  K = 10 (parents ≈ 600 clés/pt), ≈ 0,15 To pour K = 5.
- Statuts : `complete_regular | unsupported_degeneracy | resource_exhausted/{requires_tiling, live_state, disk, index, seau} | invalid_input | invariant_violated | incomplete_continuation` (reprise à une frontière de phase, jamais sur une prédiction de temps) ; `public_status=not_claimed`.

### 4.2 Résident, streamé, externe (`uniform`, 48 fils ; base reçu 200 k, cardinalités +10–15 % à 10 M)

| poste | 1 M | 10 M | 30 M |
|---|---:|---:|---:|
| index radix résident | 0,17 Go | 1,7 Go | 5,1 Go |
| rectangles WSPD vivants (3 lanes, 242/pt × 16 o) | 3,9 Go | **39–50 Go** → par vague | 116–150 Go → jamais résidents |
| boules censusées (aujourd'hui résidentes) | 100 Go | 1,0 To → SSD par seau | 3,0 To |
| événements tous K (runs SSD, ×2 pour le tri externe) | 72 Go | 720 Go | 2,2 To |
| comptage PREMIÈRE/DERNIÈRE (SSD, transitoire, par ordre) | 62 Go | 620 Go | 1,9 To |
| fold K = 10 : facettes vivantes (22/pt attendu, ×2,5 majoré) | 2,5 Go | 17–32 Go | 55–106 Go |
| fold K = 10 : enregistrements de composantes | **à instrumenter (L0)** | ? | ? |
| RAM de pointe, objet complet (`inflight` = 1) | ~15 Go | ~110–140 Go | ~165–200 Go (non garanti) |
| RAM de pointe, `prefixe_k5` | ~6 Go | ~60 Go | ~90 Go |
| SSD requis, complet (K par K, runs effacés) | 0,4 To | **≥ 4 To** | ≥ 12 To |
| SSD requis, `prefixe_k5` | 0,05 To | ~0,5 To | ~1,5 To |
| temps, complet | ~25 min | 6–7,5 h | 17–20 h (3 sessions) |
| temps, `prefixe_k5` | ~5 min | 1,5–2 h | 5–6 h |

`eight_clusters` complet à 10 M : 8–9 h (hors session, exposant ~1,55 sur la
génération) ; `scanline` : **non extrapolable** (lane q4 en exposant ~3 entre
32 k et 200 k — c'est le verrou de la famille LiDAR, et c'est la lane GPU
résidente qui doit le lever). Toute ligne 10 M / 30 M est une extrapolation ;
les seules mesures opposables seront les reçus 1 M puis 10 M.

### 4.3 Les trois étages

1. **Amont streamé par seaux Morton du centre** : le seau d'une boule est une
   fonction pure de la `BallKey` ($c = -B/(2A)$ exact en i128, écrêté à la
   boîte u16 ; T7), donc le RLE par seau est globalement exact et la fusion
   k-aire des seaux triés restitue l'ordre global (`digest_balls` v4) ; les
   seaux sont des plages de l'arbre radix à effectif de positions égal (T8),
   jamais fonction de la charge ; le census lit l'index résident (pas de
   halo) ; magasin de boules SSD partitionné par $(q, d)$ ; expansion en une
   passe vers dix runs par ordre ; tri externe par ordre.
2. **Fold streamé à état borné** : clé d'ex aequo explicite (niveau exact,
   `BallKey`, rang d'émission intra-boule ; T2) ; passe PREMIÈRE/DERNIÈRE par
   empreinte 64 bits (minorant du pic exact, marge déclarée ; T4) ; table des
   facettes vivantes libérée **au lot** de la dernière incidence (T3, avec
   `root_key` figée à la création et `canon_key` = min mise à jour aux unions,
   contre-fixtures « absorbée ⇒ oubliable » réfutée) ; arène des
   enregistrements de composantes bornée par compteur instrumenté (T6) ;
   indices et positions d'incidence en u64 ; deltas émis en flux.
3. **Reprise** : manifeste atomique (pin, tailles, sha256 des runs, ordre et
   lot courants), reprise à une frontière de phase ; comptabilité à chaque
   frontière (aucune écriture sans espace pour ancien état + temporaire +
   fusion + manifeste + marge).

## 5. Modèle de temps

Constantes du reçu 200 k (48 fils) avec dérive de latence du reduce
(1,4 → 2,2 µs par événement à 10 M, +8 % par doublement) ; voir la table du
§ 4.2. Hypothèses à mesurer avant tout engagement : débit du disque (le
disque persistant de 100 Go actuel est mesuré à ~290 Mo/s par
`gcp-migration/deploy.sh`, insuffisant), constante du reduce à état borné,
surgénération aux frontières de seaux.

## 6. Théorèmes à prouver avant le code (énoncés, `docs/analyses/echelle_20260828/synthese.txt` § 3)

- **T1 préfixe exact** : pour $s \le s'$ et tout $K \le s - 1$, événements,
  ordre et `forest[K]` identiques entre `smax = s` et `smax = s'` (livré :
  porte de préfixe ; corollaire : `digest_balls` et `digest_all` ne sont pas
  comparables entre profils).
- **T2 clé totale** : (niveau exact, `BallKey`, rang d'émission intra-boule)
  est un ordre total qui coïncide avec le tri stable actuel sur l'ordre
  d'entrée (RLE puis émission) ; mutants `tiebreak-cell-local`,
  `tie-by-sorted-T`.
- **T3 dernière incidence** : après le lot de sa dernière incidence, l'état
  d'une facette n'est lu que par `find`, `canon` et les clés ; avec
  `root_key`/`canon_key` portées par les enregistrements, la suite des deltas
  est inchangée quand la facette est libérée ; contre-fixtures (chemin de 3
  points à K = 1, deux tétraèdres partageant une face à K = 2) ; mutant
  `free-on-absorb`.
- **T4 libération jamais anticipée** : la marque DERNIÈRE par empreinte est
  ≥ la dernière position de chaque clé confondue ; le pic par empreintes est
  un minorant ; le préflight emploie une marge déclarée et calcule le pic à
  la granularité du lot.
- **T5 ordre intra-lot sans catalogue** : `fid(x) < fid(y)` ⟺
  `key(x) < key(y)` ; `post_list` triée par racine ≡ triée par `root_key` ;
  `(facet_keys, final_canon_fid)` est une fonction du flux de deltas ;
  invariant sans état `output = min(parents ∪ born)`.
- **T6 enregistrements de composantes** : borne à prouver ou majorant
  instrumenté avec refus `resource_exhausted/live_state`.
- **T7 exact-once par seau**, **T8 équilibre déterministe des seaux**,
  **T9 identité de masse par ordre** (comptés = expansés = longueur du run ;
  $\sum (q + d) = K + 1$ par événement ; fusions = facettes − composantes
  finales ; attachements = $\sum \left\vert \text{born} \right\vert$).

## 7. Portes

Égalité résident / streamé aux deux digests (v4 par convertisseur, flux) à
8 k / 16 k / 32 k (quatre familles + `terrain`, $s$ = 6/8/10,
contre-familles), seaux ∈ {1, 2, 3, 7, 31}, fils ∈ {1, 8, 48}, tailles de
run, `inflight` ∈ {1, 2, 3}, frontière de run au milieu d'un niveau égal,
coupure + reprise après chaque phase ; 200 k puis 400 k et **1 M** sur G4 ;
invariants globaux à 10 M / 30 M (T9 par K, violations nulles, K = 1 :
facettes = $n$, fusions = $n - 1$, idempotence de la partition par rejeu des
deltas sur un union-find frais, planchers par K, porte de préfixe,
invariance du digest de flux au découpage et aux fils, équivariance par
permutation) ; juge d'échantillon ($10^4$ par K, graine gravée : census par
balayage des seaux, niveau par `obigint`, ré-expansion, incidences recomptées
par un parcours différent, K = 1 par Borůvka à arithmétique distincte) ;
mutants à code 4 : `prefix-hq-off`, `tiebreak-cell-local`, `tie-by-sorted-T`,
`free-on-absorb`, `free-record-early`, `canon-not-min-on-union`,
`root-key-mutable`, `bucket-by-anchor`, `no-clamp`, `merge-order`,
`last-mark-shifted`, `pic-per-incidence`.

## 8. Ordre des livraisons

| # | livraison | objet | digest v4 |
|---|---|---|---|
| L0 | instrument `vivantes` promu en compteur de réception (pic par lot, enregistrements, singletons) + reçu 32 000 puis 200 k | inchangé | inchangé |
| L1 | **livré en partie (28 août)** : profil nommé `prefixe_k5` / `prefixe_k4` dans la sortie et porte de préfixe par digests (K = 4, 5 ; trois familles ; mutant tué) — restent : clé d'ex aequo explicite, indices u64, mutants T2 | inchangé | bit-identique |
| L2 | fold résident à état borné (PREMIÈRE/DERNIÈRE en RAM, table des vivantes, arène, convertisseur flux → tableaux v4) | inchangé | bit-identique par convertisseur |
| L3 | payload et digest de flux, manifeste atomique, statuts SSD | inchangé | second digest déclaré |
| L4 | amont streamé : seaux Morton du centre, RLE par seau, magasin de boules, expansion en runs, tri externe | inchangé | `balls` v4 par fusion k-aire |
| L5 | préflight par rôle (`MemAvailable`, cgroup, `statvfs`, débit mesuré) ; jalon **1 M** G4 : résident ≡ streamé | — | — |
| L6 | 10 M : `prefixe_k5` d'abord, puis objet complet | — | flux + invariants + juges |
| L7 | 30 M : `prefixe_k5` (une session) puis complet en sessions gardées avec reprise | — | idem |

Le GPU (lane résidente sur device, `GPU.md`) est un chantier **parallèle**
visant la lane q4 de `scanline`/`eight_clusters` et la génération à 10 M ; il
ne change ni l'objet ni ce plan.

## 9. Questions à trancher par l'utilisateur

1. **Profil des premiers reçus 10 M** : `prefixe_k5` d'abord (~1,5–2 h,
   ~60 Go, ~0,5 To de disque) avant l'objet complet ? `prefixe_k4` utile ?
2. **Disque** : accord pour attacher un SSD à la VM via les scripts gardés
   (≥ 1,5 To pour 10 M `prefixe_k5` / complet K par K, ≥ 4 To avec rétention
   complète ; 30 M complet ≥ 12 To) — débit **mesuré** au préflight.
3. **Sessions** : une mini-session (200 k + 1 M, ≤ 2 h) pour L5, une session
   ≤ 8 h pour 10 M, 30 M complet en 2–3 sessions avec reprise.
4. **Contrat de payload** : le digest de flux comme autorité au-delà de 1 M ;
   les `batch_levels` (240 Go à 10 M) restent-ils dans le contrat ?
5. **Familles** : `uniform` puis `eight_clusters` (10 M complet hors session
   → `prefixe_k5`) ; `terrain` à profiler à 200 k avant engagement.
