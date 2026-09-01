# GPU v6 — plan de port, doctrine et pistes fermées

Cadre : `phase=exploration_v6_hors_registre`, `public_status=not_claimed`.
Doctrine (ARCHITECTURE.md § Parallélisme) : aucun kernel n'entre au build
produit sans reçu de gain G4 ; option CMake `MHGP6_ENABLE_CUDA` OFF par
défaut, la CI GitHub ne la construit jamais ; syntaxe hôte vérifiée au stub
CUDA local avant toute session (mémoire de session du 1er septembre).
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
  contractuel v5 requalifié + confinement de panne réparé — porte
  `mhgp6_executor_pool`, mutants pool-serial / pool-drop-exception /
  pool-close-fatal-missing).
- C2 index résident (`GpuCloudIndex` SoA, digest des octets téléversés).
- C3 `k_prefilter` (un thread par boule, DFS à pile bornée ≤ 49 + garde →
  refus par boule, jamais une troncature).
- C4 `k_census` (BallData bit-identique, ordre DFS = ordre de pile du
  scalaire).
- C5 pilote `mhgp6_cuda` + contrats 50k digest-égalité quatre familles +
  reçu G4 = le reçu de gain qui conditionne les séries F (prepare_fold
  device, contrat canonique keys/ev_fid) et L (lanes).
