# GPU v6 — plan de port, doctrine et pistes fermées

Cadre : `phase=exploration_v6_hors_registre`, `public_status=not_claimed`.
Doctrine (ARCHITECTURE.md § Parallélisme) : aucun kernel n'entre au build
produit sans reçu de gain G4 ; option CMake `MHGP6_ENABLE_CUDA` OFF par
défaut, la CI GitHub ne la construit jamais ; le stub local est une preuve
de syntaxe et de logique **C++ hôte** — jamais une preuve de compilation
nvcc ni de comportement device (§ 5.8 ; mémoire de session du 1er
septembre).
Conception complète : `audits/NOTE_CLAUDE_CONCEPTION_MULTICPU_GPU_20260901.md`
(verdict d'étage chiffré au reçu
`receipts/session_g4_20260901_d98f47296d67_1788245493/`).

## Verdict d'étage (mesuré, jamais déclaré)

Porter d'abord PRÉFILTRE+CENSUS (série C) : 28,6 % du mur uniform 50k à
48 fils, un travail indépendant par boule, arithmétique 100 % entière. Fil
de fer HONNÊTE au reçu 50k (21 622 480 candidats, contrat census_all v1) :
H2D = 2 421 717 760 o de boules + 2 162 248 000 o de sentinelles + index ≈
4,59 Go déclarés ; D2H ≈ 2,16 Go à sorties de taille fixe — contre
20-40 Go de covers qui bornaient les lanes v5 (kernel 1-4 % du mur,
−19,5 % sur terrain seulement). Plafond d'Amdahl si l'étage tombe à ~1 s :
uniform 49,3 → ~36 s (−26 %), gain sur toutes les familles. Les lanes
(série L) restent subordonnées au reçu C5 et à la fermeture de l'exposant
q4 scanline.

## F0 — piste fermée : le REDUCE du fold ne se porte pas

Idée : réduire sur device l'union-find de l'étage B (25-28 s cumulés à 48
fils, 73 % du fold). Cause de fermeture : `reduce_fold` est une chaîne de
lots séquentielle (lots le plus souvent d'UN événement), au canon min-fid et
à l'ordre d'émission des deltas CONTRACTUEL — le digest hache `r.deltas`
dans l'ordre d'émission, trié par fid de racine union-find, et
`unite_canon` rend la racine dépendante de tout l'HISTORIQUE des unions
(pas de la seule partition) : un CC parallèle (type ECL-CC) ne préserve ni
les frontières de lots ni les racines, donc pas le digest. Ce qui survit :
le précalcul parallèle amont (rôles, `first_contact`, compteurs) et l'aval
FIFO du design « cœur DSU minimal » (chantier CPU). Ne rouvrir que sur un
théorème d'équivalence + fixture, jamais sur un benchmark.

## Livraisons (chacune : code + porte + mutants)

- C1 ✅ pool d'exécuteurs (`src/gpu/executor_pool.hpp`, hôte pur, port
  contractuel v5 requalifié) + confinement de panne CÔTÉ WORKER : la version
  initiale (confinement dans un wrapper producteur) portait une course
  causale — le worker pouvait dépiler le travail suivant sur l'exécuteur
  empoisonné avant la fermeture (sonde auditeur : 199/200,
  `REPONSE_AUDITEURS_MULTICPU_V6_20260901.md` § 5.6) ; corrigée en donnant
  la fermeture au worker (admission fermée + file annulée AVANT la
  notification du ticket fatal), fixture permanente scénario 10 ; instant
  d'activation = le retrait sous `mu_`, contre-preuve par hook
  `pre_activate` + closer (scénario 13). Portes `mhgp6_executor_pool*` à
  scènes-signature SÉLECTIVES, quatre mutants : pool-serial /
  pool-drop-exception / pool-worker-resume-after-fatal /
  pool-activate-after-unlock.
- C1 ✅ témoin device `src/gpu/device_witness.cu` — SOCLE ARITHMÉTIQUE
  PARTIEL seulement (§ 5.8) : DI128 (port du lot 1 v5, div_by_4 incluse) +
  `__int128` natif device avec QUOTIENT/RESTE (§ 5.4) ; preuve d'écriture
  par sentinelle, attestation de branche `di_mulhi_branch()` (stub =
  portable, nvcc = intrinsèque), verdict mutant PAR PRIMITIVE (désaccords de
  somme = retenues attendues comptées par l'hôte). Il ne compile ni
  `BallKey::power`, ni `AxisBounds`, ni la division plancher : ces ports
  arrivent avec C3 et la décision wire (division device vs minimisateurs
  précalculés hôte). Stub hôte exécutable
  (`tests/cuda_stub.hpp` + `tests/device_witness_stub.cpp`, kernels en
  boucles séquentielles) : portes locales `mhgp6_device_witness_stub*` —
  nominal 0, trois dents à 4 (carry ; skip-arith `witness-skip-write` ;
  skip-native `witness-skip-native-write`, tableaux séparés) et la
  contre-fixture composée skip+carry GRAVÉE à 1 (les oracles tranchent,
  jamais un 4 aveugle) — AVANT toute session, jamais un reçu device ; les
  mêmes portes `gpu` réelles n'existent que sous `MHGP6_ENABLE_CUDA` (G4).
  Aucun nvcc exercé à ce jour, aucun GO G4 ouvert.
- C2 index résident (`GpuCloudIndex` SoA, digest de la sérialisation
  canonique de chaque tableau — tailles, racine, version — jamais les octets
  bruts d'un struct ABI ; § 5.4 : formats wire versionnés champ par champ,
  cibles de compression à démontrer par types + static_assert). Contrat
  détaillé : § « Wire série C v1 » ci-dessous.
- C2 (code) : `src/gpu/wire.hpp` — construction + sérialisation canonique +
  digest, refus fail-closed ; porte `mhgp6_wire` (aller-retour bit-exact,
  t1 contre balayage exhaustif, trois refus hors-domaine, digest gravé) +
  mutants `gpu-index-drop-node` / `wire-t1-plus-one`.
- C3/C4 (code) : `src/gpu/census_kernels.cuh` — `k_prefilter` et `k_census`,
  un fil par boule, pile ≤ 49 + garde → statut par boule → refus du RUN
  entier ; ordre de pile du scalaire (gauche puis droite empilés). Porte
  STUB `mhgp6_census_device_stub` : bit-identité boule à boule contre le
  scalaire de production sur les candidats RÉELS (deux familles), 4 mutants
  par drapeaux (`gpu-range-add-le`, `gpu-stack-shallow`,
  `gpu-swap-push-order` — divergence d'ORDRE à multiset égal —,
  `gpu-census-nonstrict`) ; porte DEVICE jumelle
  `mhgp6_census_device` (mêmes exigences, `MHGP6_ENABLE_CUDA`, arch
  compilée signée dans le binaire).
- C5 (code) : couture `RunOptions::prefilter_census_override` (nullptr =
  route CPU strictement inchangée ; refus transactionnels mappés, jamais un
  préfixe) ; route partagée `src/gpu/pilot.hpp` ; porte STUB
  `mhgp6_pilot_stub` : pipeline COMPLET, OBJET IDENTIQUE (tous digests,
  cartes, totaux) CPU vs route série C sur 2 familles × 2 tailles, refus du
  run entier sous mutants ; pilote `cli/mhgp6_cuda.cu` (les deux routes,
  parité exigée, coûts wire/H2D/kernels/D2H séparés, `--repeat`, parsing
  exact, arch + device signés) + son stub de syntaxe/logique. Records
  (§ 5.13-5.15) : chaque répétition publie les DEUX signatures d'une
  projection canonique `mhgp6_parite_v1` couvrant EXACTEMENT le tuple
  comparé (`digest_all`, `digest_balls`, `digest_postprefilter`, forêts par
  K, cartes par K, `total_events`, `emitted`) — le validateur recalcule
  l'égalité lui-même, le booléen `parite=` n'est qu'une redondance, et une
  incohérence projection/tuple est un PLANCHER (code 3). Le juge unique
  `tests/pilote_juge.py` (porte stub, runner G4 en mode `--fichier` après
  CHAQUE famille, validateur) ferme : en-tête exacte liée à l'entrée
  (famille, n, graine, fils, lot), cinq records ABBA/BAAB, formules
  d'octets (112/100/100 par boule), `lot_effectif = min(lot, nb_total)`,
  chronos finis, étage device ≥ somme de ses six composantes (± 0,4 ms
  d'arrondi %.1f), murs enveloppant leurs sous-étages, stabilité de
  l'objet/volumes entre répétitions, `arch_compilees` comparé à l'attente
  (120 sur G4). L'architecture est CONTRACTUELLE :
  `CMAKE_CUDA_ARCHITECTURES=120` exigée (refus CMake sinon). LIAISON D'OBJET
  (audit post-session) : le `digest_all` des records doit égaler celui des
  bras `--digest` de la matrice pour la même entrée ET la fixture d'égalité
  `GPUV6_OBJET_DIGESTS` du profil (gravée depuis le reçu
  `session_g4_20260901_b97f20ea4b8f_1788293187`) — un pilote calculant un
  autre objet à parité interne intacte n'est plus vert. Un reçu durable se
  re-juge sans être modifié par `gcp-migration/revalidate_v6_receipt.sh`. Le reçu de
  gain 50k+ vient d'une session G4 (profil de campagne, nouveau pin,
  nouveau GO auditeur) — jamais d'un gate ni du reçu GPU v5 historique.

## Wire série C v1 (`gpu_wire_v1`) — le contrat AVANT tout kernel (§ 5.4/§ 6 item 3)

Doctrine : chaque format est versionné et sérialisé CHAMP PAR CHAMP en
petit-boutiste explicite — jamais un memcpy de struct ABI ni son padding. Le
digest d'un téléversement hache, par tableau : un tag, la taille, puis les
octets sérialisés ; le digest d'index chaîne les tableaux, la racine, les
comptes et la version. Toute valeur hors domaine au moment de la
sérialisation est un REFUS transactionnel (`invalid_input`), jamais une
troncature.

**Décision division (tranchée avec § 5.11)** : la seule division du chemin
est `t1 = floor_div128(-b[i], 2a)` (AxisBounds) — fonction de la boule
seule, HISSÉE CÔTÉ HÔTE. Le wire porte SIX CANDIDATS u32 par boule
(clamp_domaine(t1) et clamp_domaine(t1+1) par axe, rabattus sur
[0, 65535]) — jamais un `t1` i64 (rétrécissement non gardé, contre-exemple
a=1, b=−2^70) ; construction BORNÉE sans i128 signé intermédiaire
(1cb08aa8) : b>0 ⟹ 0/0 ; b=0 ⟹ 0/1 ; b<0 ⟹ q=|b|/(2a) en u128, saturé —
couvre b=INT128_MIN. Pour toute boîte incluse dans le domaine, rabattre
domaine puis boîte == rabattre boîte (parabole convexe) : la division ET le
+1 disparaissent du device. Le device n'exécute QUE de l'add/mul/cmp
`__int128` — le socle prouvé par `mhgp6_device_witness`.

**Dataflow (tranché, § 5.11)** : `census_all` v1 — le census tourne sur TOUS
les candidats (le préfiltre du reçu 50k n'en tue ~1 % : une compaction
stable des survivants n'achèterait presque rien et compliquerait le
transport de l'indice global ; elle reste une option documentée si des
familles à forte mortalité apparaissent). Volumes HONNÊTES au reçu 50k
(21 622 480 candidats) : H2D = 2 421 717 760 octets (112 o/boule) + index ;
D2H = 9 + 91 = 100 o/boule utiles (compte+statut préfiltre ; ids + statut +
comptes + cand census) ≈ 2,16 Go ; lot vif = 112 + 9 + 91 = 212 o/boule,
424 en double tampon. `cand_idx = base + gid` n'est exact QUE parce
qu'aucune compaction n'a lieu — gravé au contrat.

**Frontière D2H (f3704e99)** : sorties PRÉREMPLIES de sentinelles par
l'hôte ; VALIDATEUR CENTRALISÉ (`validate_ball_out` : statut connu,
n_int ≤ 9, n_shell ≤ 12, cand_idx global attendu, tous les ids dans
[0, n_upos)) exécuté AVANT toute reconstruction — une écriture omise ou
corrompue ne devient jamais une borne de lecture ; mutants
`gpu-skip-ball-write` et `gpu-nshell-overdomain`.

**`GpuCloudIndexWire` (H2D une fois, résident)** — tableaux SoA :

| tableau | type wire | octets/élément | contenu |
|---|---|---|---|
| `node_left`, `node_right` | i32 | 4 + 4 | NodeRef (négatif = feuille, encodage CPU conservé) |
| `node_first`, `node_last` | i32 | 4 + 4 | plage upos du sous-arbre |
| `node_box` | u16 × 6 | 12 | boîte SERRÉE tlo/thi (coordonnées u16 par profil ; hors domaine ⟹ refus) |
| `upos` | u16 × 3 | 6 | position unique (ordre Morton) |
| `wsum` | u32 | 4 | préfixe des multiplicités (n < 2^30 ⟹ u32 exact, débordement ⟹ refus) |

En-tête : version (`gpu_wire_v1`), `n_nodes`, `n_upos`, `root` (i32),
digest. Coût : 28 o/nœud + 10 o/upos ≈ **38 o/upos** (< la cible de 60,
démontrée par les types wire ci-dessus, pas par le sizeof des structs CPU —
`RadixNode` fait 120 o). La feuille n'a PAS d'entrée nœud (encodage négatif).

**`GpuBallIn v1` (H2D par lot)** — par boule, 112 o : `a, b[3], c` en
5 × (u64 lo, u64 hi) = 80 o (forme i128 sérialisée explicitement) ; SIX
CANDIDATS u32 = 24 o (mots 10..12 : `w[10+i] = c0[i] | c1[i] << 32`) ;
`h` u64 = 8 o (> 0 ; seuil du préfiltre, `interior_cap = h − 1` au census).

**Sorties D2H (SoA, par lot)** — préfiltre 9 o VIFS par boule : `count`
u64 (exact au statut ok) + `status` u8 (`ok` | `at_least_h` |
`stack_overflow`) ; census 91 o VIFS : `ids` i32 × 21 (intérieur [0, 9),
coquille [9, 21) ; indices upos, ordre = ORDRE DE PILE DU SCALAIRE — gauche
puis droite empilés, droite puis gauche visités, pile ≤ 49 + garde) +
`status` u8 (`ok` | `interior_overflow` | `shell_overflow` |
`stack_overflow`) + `n_int` u8 (≤ 9) + `n_shell` u8 (≤ 12) + `cand_idx` u32
(= base + gid, exact PARCE QUE census_all — aucun octet réservé fantôme).
Jamais la copie du `BallData` CPU (224 o, 4,79 Go à 21,4 M) : la
reconstruction (clé, niveau, arité, multiplicités) est HÔTE.

**Budget VRAM (déclaré, refusé, jamais dépassé silencieusement)** : index +
lot vif 112 + 9 + 91 = 212 o/boule + tableaux d'erreurs. Le pilote v1
travaille en LOT UNIQUE séquentiel (le double tampon — 424 o/boule — est une
OPTIMISATION NON IMPLÉMENTÉE, documentée seulement) ; `--lot` est
inoffensif (`lot_eff = min(lot, nb_total)`), la frontière multi-lots est
EXERCÉE par porte (`--lot=17`, plancher `lots > 1`, mutant
`gpu-lot-base-reset`). Aucune compaction en v1 (census_all) ; la compaction
stable resterait l'option documentée si des familles à forte mortalité de
préfiltre apparaissent.

**Erreurs** : statut PAR BOULE puis réduction déterministe au plus petit
index global ⟹ refus du RUN ENTIER (lots et préfixes jetés, callbacks
muets, pool fermé via `DeviceFatalError`) ; `cudaError` ⟹ pareil.
