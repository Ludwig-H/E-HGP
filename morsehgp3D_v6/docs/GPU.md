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
48 fils, un travail indépendant par boule, arithmétique 100 % entière
(AxisBounds i128, `BallKey::power` MHGP6_HD), fil de fer ≈ 1,7 Go H2D /
2 Go D2H à sorties de taille fixe — contre 20-40 Go de covers qui bornaient
les lanes v5 (kernel 1-4 % du mur, −19,5 % sur terrain seulement). Plafond
d'Amdahl si l'étage tombe à ~1 s : uniform 49,3 → ~36 s (−26 %), gain sur
toutes les familles. Les lanes (série L) restent subordonnées au reçu C5 et
à la fermeture de l'exposant q4 scanline.

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
  cibles de compression à démontrer par types + static_assert).
- C3 `k_prefilter` (un thread par boule, DFS à pile bornée ≤ 49 + garde →
  refus par boule remonté en refus du RUN ENTIER — § 5.5 : statut par boule,
  réduction déterministe au plus petit index global, lots et préfixes jetés,
  pool fatal fermé ; fixture peigne Morton à pile 49).
- C4 `k_census` (BallData bit-identique, ordre DFS = ordre de pile du
  scalaire — le scalaire empile gauche puis droite donc visite droite puis
  gauche ; mutant d'inversion des pushes ; payload compact autosuffisant
  ~92 o/boule reconstruit hôte, jamais la copie du BallData 224 o).
- C5 pilote `mhgp6_cuda` + contrats 50k digest-égalité quatre familles +
  reçu G4 = le reçu de gain qui conditionne les séries F (prepare_fold
  device, contrat canonique keys/ev_fid) et L (lanes). Exigences § 5.5 :
  petits cas adversariaux champ par champ AVANT les digests 50k, nouvelle
  source v6 épinglée, répétitions et coûts H2D/D2H mesurés — jamais le reçu
  GPU v5 historique ; nouveau pin + nouveau GO auditeur obligatoires.
