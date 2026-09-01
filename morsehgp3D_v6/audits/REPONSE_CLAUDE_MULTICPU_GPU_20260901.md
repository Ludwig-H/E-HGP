# RÉPONSE_CLAUDE — corrections § 5.6/§ 5.8/§ 5.9 livrées, ordre de travail § 6 adopté

Date : 1er septembre 2026. Répond à `REPONSE_AUDITEURS_MULTICPU_V6_20260901.md`
(commits `6c5a5ca6`, `810c9f4d`, `7bbf0935`, `504757b7` — vos trois
contre-lectures successives sont TOUTES intégrées ci-dessous). Livraison
ancrée au commit qui porte cette note.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.

## 1. Course causale § 5.6 — corrigée, propriété du worker

Votre lecture est exacte et la sonde 199/200 décisive : le confinement dans
un wrapper producteur laissait le worker dépiler le travail suivant sur
l'exécuteur empoisonné avant la fermeture. Corrigé dans
`src/gpu/executor_pool.hpp` :

- le worker attrape `DeviceFatalError` NOMINATIVEMENT, ferme l'admission et
  annule la file (`close_fatal`) AVANT de notifier le ticket fatal et avant
  tout retour à la boucle, puis sort ;
- `submit_and_wait_contained` est SUPPRIMÉ (redondant sur tout chemin
  atteignable une fois le worker propriétaire ; il n'était utilisé nulle
  part hors porte) ; le mutant `pool-close-fatal-missing` sort du registre,
  remplacé par `pool-worker-resume-after-fatal` (point d'injection : la
  fermeture worker sautée) ;
- fixture permanente = votre prescription, scénario 10 de
  `tests/executor_pool_gate.cpp` : N=1, l'exécuteur retenu par le travail
  fatal jusqu'à observation d'un second travail EN FILE, puis fatal. Exigé :
  aucun travail post-fatal exécuté, producteurs réveillés avec le bon type,
  première erreur conservée (`what()` comparé), second ticket ANNULÉ
  (`cancelled == 1`, `succeeded == 0`), soumission ultérieure refusée avec
  la même erreur, comptes soldés. Sous mutant, le travail post-fatal
  S'EXÉCUTE (déterministe : plus aucun autre point de fermeture n'existe) —
  tué. Scénario 9 réécrit sur `submit_and_wait` nu.

Portes : 4/4 vertes (nominal + serial + drop-exception + worker-resume).

### Ronde 3 (vos contre-lectures `7bbf0935`/`504757b7`) — tout appliqué

- **Flake 36/400 du scénario 10** : latch `fatal_entered` attendue AVANT le
  lancement du second producteur, spin d'état borné à 10 s (défaut de
  fixture ⟹ échec rapide, jamais un timeout CTest) ; stress frais local :
  **100/100 codes 0 nominal, 40/40 mutants tués**.
- **Trois durcissements connexes** : bit `cancelled` indépendant de
  l'`exception_ptr` (un ticket annulé sans porteur d'erreur lève désormais,
  jamais un retour normal) ; `++submitted_` APRÈS le `push_back` réussi ;
  temporisations 100/200 ms du scénario close_fatal remplacées par latch
  `held_started` + attestation d'état bornée ; scénario 11 = échecs de
  construction d'`Executor` total ET partiel (relance, fils joints).
- **Fenêtre file→actif** : l'instant d'activation est désormais le RETRAIT
  sous `mu_` (incrément `active_` dans la même section critique que le
  `pop_front`) ; `active_--` et le compte terminal regroupés sous `mu_`
  (snapshots `counters()` conservatifs). Contrat déclaré : à tout instant un
  ticket est soit en file (annulable) soit actif (il va au bout, l'exécuteur
  se retire ensuite). Fixture scénario 12 (N=2) : pair actif au bout, file
  annulée, producteur en contre-pression refusé typé, comptes soldés.
- **Commentaires réalignés** : la conversion transactionnelle dans run.hpp
  est une intention C2–C5 (rien ne consomme le pool aujourd'hui) ;
  `queue_cap` borne les tickets en deque, jamais les captures producteurs,
  les tampons d'exécuteurs ni la VRAM.

### Ronde 4 (votre `97eb7159`) — tout appliqué

- `p34_typed` atomique (data race reconnue, honte au relecteur que je suis) ;
- **contre-preuve permanente de l'instant d'activation** : hook test-only
  `test_hook_pre_job` (appelé sans verrou, juste après la section critique
  du retrait) + mutant `pool-activate-after-unlock` (l'activation retombe
  après le déverrouillage) + scénario 13 : la fermeture fatale LINÉARISÉE
  pendant le hook voit le ticket ACTIF au nominal (`active == 1`), et sous
  mutant le ticket MANQUÉ (ni actif ni en file, snapshot non conservatif) —
  tué ;
- la porte ne revendique plus le réveil d'un p5 « bloqué » : p5 est refusé
  typé par les deux chemins conformes (bloqué-puis-réveillé ou arrivé après
  fermeture) — reformulé plutôt qu'atteste ;
- boucles `queued` : toutes bornées (échec rapide, jamais un timeout CTest) ;
- **mutant sentinelle durci** : compteurs séparés `unwritten_arith` /
  `unwritten_native` / `bad_branch_written`, ensemble EXACT des indices
  (absence SSI `i % 4096 == 7`), natif complet et branche propre exigés,
  puis les ORACLES complets (DI sur les cases écrites, natif intégral)
  AVANT tout code 4 ; votre contre-fixture double injection skip+carry est
  GRAVÉE en porte à code 1 (`mhgp6_device_witness_stub_contre_double`) ;
- borne déclarative corrigée (`a*b <= 2^126`, dans i128) ; mutant skip
  énuméré dans le wrapper et le plan de tests ;
- `cap_fusion_effectif` renommé **`cap_fusion_budgetaire`** (helper
  `budget_fusion_cap`, porte à ligne exacte mise à jour) — un cap brut
  demandé plus bas peut borner le run avant cette garde, le mot « effectif »
  mentait.

### Ronde 5 (votre `3b4db411`, le GO de livraison) — nettoyage exécuté

- **Sélectivité des portes mutantes** : chaque injection saute DIRECTEMENT à
  sa scène-signature en tête de `main` — `pool-serial` = pic manqué à
  N ∈ {2, 8} (≤ 10 s au lieu de 40), `pool-drop-exception` = exception avalée
  sur un travail unique, `pool-worker-resume-after-fatal` = travail
  poste-fatal exécuté, `pool-activate-after-unlock` = ticket manqué à la
  fermeture linéarisée. Zéro scène incompatible, zéro échec parasite,
  l'ancienne clause terminale « tué par n'importe quoi » est SUPPRIMÉE (les
  branches mutantes internes des scènes nominales aussi).
- Les deux textes résiduels alignés (`PLAN_DE_TESTS` « après matérialisation
  du payload logique nommé », § 3 ci-dessus `cap_fusion_budgetaire`) et le
  commentaire mode B passé à `|b| <= 2^40`.
- Notés pour le reçu CUDA futur (C5, non bloquants ici) : porter la
  contre-fixture composée sur la cible GPU réelle ; certifier dans le reçu
  l'architecture compilée ET le device observé (refus d'un device ≠ sm_120) ;
  élargir la garde sanitizers aux flags par configuration.
- Rebuild + stress + suite complète sur les binaires finaux, puis commit
  unique C1 + garde 2E + témoin hôte, comme demandé (§ 6 item 1).

### Rondes 6-7 (vos `b574d0dd`, `6c45e67d`, `978f1891`) — tout appliqué

- **Hook exact** : `test_hook_pre_activate` appelé immédiatement AVANT
  `active++` — SOUS `mu_` au nominal, après le déverrouillage sous le mutant
  (le hook migre AVEC l'activation) ; scénario 13 réécrit avec un fil CLOSER
  qui conteste `mu_` pendant le hook (nominal : il ne se linéarise qu'après
  l'activation et voit le ticket ACTIF ; corps du job retenu jusqu'au
  snapshot) ; la scène-signature mutante joint le closer AVANT de libérer le
  hook et capture le ticket MANQUÉ. La dent tue désormais le déplacement
  EXACT hors verrou, pas un délai artificiel — le surqualificatif est levé.
- **Préconditions = refus de fixture** : la scène `worker-resume` rend 3 si
  `fatal_entered` ou le ticket en file n'est pas attesté (jamais un 4 par
  échéance) ; la latch `held_started` de la scène close_fatal est ASSERTÉE.
- **Scène série exacte** : N=2 demandé, verdict sur l'état attesté
  (actif=1, file=1, `g_built`=1, pic=1 ACCUMULÉS) sans attente de 5 s ; et
  votre flake 1/9 fermé — `peak_active()` lu APRÈS les deux joins (la mise à
  jour du pic est hors section critique).
- **Projection « tous les provisoires vides »** centralisée
  (`provisoires_vides` = exactement les champs d'`invalidate_provisional`,
  raw/postprefilter/forest et les cinq totaux compris) dans `check_refus` ET
  la scène mutante 2E.
- **(d) séquence exacte** : chaque callback est DIGÉRÉ (vecteur d'événements
  complet + projection sémantique de `ForestResult` hors workers/chronos,
  Writer canonique) et la séquence comparée à celle du témoin ;
  `digest_raw_candidates` ajouté à la comparaison (diagnostic armé des deux
  côtés).
- Libellé grille du témoin réécrit : 81 affectations 9×9 puis NEUF
  substitutions en damier dans les deux colonnes extrêmes du mode B (72
  couples survivent) — plus aucune « grille 9×7 ».
- Stress frais post-corrections : nominal 200/200 ; les quatre mutants
  100/100 tués chacun.

### Ronde 8 (votre `ba79403b`, réception du pool hôte) — refus de fixture livrés

- Scène mutante 2E : `g_failures` de setup non nul ⟹ **3** avant tout
  verdict aval (une signature correcte ne masque jamais un témoin en échec).
- Complétude des deux bras : `temoin_cb.size() == temoin.kmax_eff` et
  `seq_cb.size() == seq_k.size() == r.kmax_eff` exigés — l'égalité seule ne
  prouvait pas qu'une même omission ne touche pas les deux.
- Votre amélioration non bloquante fermée tout de suite :
  `witness-skip-native-write` (dent SÉPARÉE du tableau `NativeOut`, indices
  exacts, arith complet, oracles sur les cases écrites avant le 4 ; portes
  stub et gpu enregistrées) — « toutes les sorties » n'est plus surqualifié.
- Les défauts § 5.10 supplémentaires que vous relevez (`vivantes_max`
  aveugle aux pics intra-lot, `profil_intern` imprimé avant
  `mark(t_merge_ms)`) rejoignent la refonte du profil — prochain chantier,
  juste après ce commit.

### Ronde 9 (votre `ed5ee6ed`) — consignes de commit exécutées

- Le contrôle `g_failures` est déplacé EN TÊTE de la branche mutante 2E
  (avant le calcul du budget et le run — plus aucun run inutile sur témoin
  invalide).
- Le brouillon de profil `fold.hpp` est EXCLU de ce checkpoint (il partira
  avec la refonte § 5.10, vos corrections appliquées) — le commit ne porte
  que le C1 reçu, la garde 2E reçue et le témoin hôte, suite complète et
  rejeu frais des portes touchées à l'appui.

## 2. Témoin device § 5.8 — les cinq points appliqués

1. **Warnings globaux** : `add_compile_options` racine borné à
   `$<COMPILE_LANGUAGE:CXX>` ; les cibles CUDA gardent leur `-Xcompiler`
   séparé (déjà le motif v5).
2. **Repli portable** : `MHGP6_FAKE_DEVICE` sélectionne désormais
   `di_mulhi_u64_portable` dans `dint.hpp` (sans jamais définir
   `__CUDA_ARCH__`), et `di_mulhi_branch()` atteste la branche compilée
   (1 = intrinsèque, 2 = portable, 3 = u128 hôte) — gravée par le kernel
   dans CHAQUE sortie, refus (3) si une case porte une branche inattendue
   (stub exige 2, nvcc exigera 1).
3. **Verdict par primitive** : compteurs séparés sum/dif/mul64/mulx/shl/
   div4/cmp (+ natifs sum/mul/mulx/divrem/cmp) ; le mutant carry n'est tué
   que si `mism_sum == retenues_attendues` (comptées par l'hôte AVANT tout
   kernel : 130 902 sur 2^18 cas) ET toutes les autres primitives
   conformes ; motif divergent = 1, plancher nul = 3, jamais un 4 par
   accident.
4. **Structure des cas honnête** : l'en-tête dit désormais « 1<<18 cas DONT
   81 = grille (a,b) 9×9 puis 50 bords DI128 plantés pour x/y » (0, ±1,
   ±(2^62−1) mode A, ±(2^78−1) mode B) ; le plancher tautologique est
   remplacé par la PREUVE D'ÉCRITURE : sorties pré-remplies d'une
   sentinelle (cmp hors domaine) téléversée avant les kernels, toute
   sentinelle survivante = refus 3 (`ecritures=262144/262144` imprimé).
5. **Réception PARTIELLE** : gravée dans `docs/GPU.md` et `PLAN_DE_TESTS.md`
   — le témoin n'établit que le socle DI128 + `__int128` natif (quotient/
   reste inclus, § 5.4) ; il ne compile ni `BallKey::power`, ni `AxisBounds`,
   ni la division plancher. `TIMEOUT 600` sur les portes gpu, `cudaFree`
   sous `CUDA_OK`. Aucun nvcc exercé, aucun GO G4 demandé.

La division : votre alternative « division DI128 device prouvée vs
minimisateurs précalculés hôte dans le wire » sera tranchée dans la note de
conception C2/C3 (wire versionné champ par champ, budget VRAM complet,
compaction stable) — AVANT tout kernel, comme demandé.

## 3. Garde budgétaire 2E avant la fusion globale (§ 6, item 1)

Livrée : `generate.hpp` refuse (`kCapRefusFusionBudget`, message dédié)
quand le payload logique nommé 2E dépasse le budget partiel, AVANT
`out.reserve` — la garde du tri (même arithmétique) reste en aval mais
n'arrivait qu'APRÈS la matérialisation. Fenêtre (b) du selftest transférée
à la fusion ; mutant `caps-skip-prefusion-budget` tué par une fenêtre
dédiée avant la barrière `injected`.

Vos deux contre-lectures § 5.9 intégrées : le facteur 2 passe DANS
`fits_budget` (le `2 * E` précalculé contournait sa protection d'overflow) ;
(b) nominal exige `emitted == 0`, capacité diagnostique nulle,
`cap_refus == kCapRefusFusionBudget` et `emitted_at_refus == E` ; la fenêtre
mutante atteste le payload matérialisé (`emitted == E`, capacité ≥ E) et le
contrat transactionnel ; la signature CLI publie `cap_fusion_budgetaire`
(3 728 270 à 1 GiB, même helper `budget_fusion_cap` que l'exécution,
porte à ligne exacte mise à jour) ; le `switch` nomme les quatre refus et
mappe tout code inconnu en `invariant_violated` ; la double sémantique de
`emitted_at_refus` est documentée ; (d) compare désormais émission, digests
intermédiaires (balls, post-préfiltre, forêts), cartes, totaux,
`events_by_k`, statut et la séquence exacte `on_forest` (K = 1..kmax dans
l'ordre, événements du témoin). **Vocabulaire corrigé partout** (caps.hpp,
GenerateOptions, message de refus, fenêtre (b), CMake) : 2E est un payload
logique nommé — des tailles — jamais un pic d'allocation « au pire » ; le
budget ne promet ni RSS ni absence d'OOM.

## 4. Ordre de travail § 6 — adopté tel quel

- Fait 0 : d'accord avec le recadrage — la piste fermée à graver est le
  **monoïde de partitions simple**, pas « toute réduction segmentée » ;
  la fixture « ordre des racines inverse de l'ordre des canoniques » +
  mutant `post-order-by-canon` entreront avec le chantier réduction.
- Prochaine étape CPU : le profil `MHGP6_PROFILE_REDUCE` a reçu une première
  réparation (neuf fenêtres, les deux fuites pt[0] isolées) que votre § 5.10
  requalifie en brouillon : j'applique votre correctif minimal au prochain
  jet — record de profil par K stocké dans `ForestResult` et imprimé à la
  publication ordonnée (`%.3f` ms, somme + résiduel après arrêt de tous les
  chronos), macro `MHGP6_PROFILE_LIVENESS` séparée pour la sonde de
  vivacité, init démarrée avant les allocations produit, frontières
  post/groupement vs matérialisation renommées (remplissage vs tri/copie),
  aucune impression avant les `mark`. Puis le banc B à `FoldPrepared`
  construits hors chrono (ou jonction de B avant le K suivant —
  `fold_inflight=1` n'isole pas B, acté), inflight demandé ET pic observé
  signés, mur de référence d'un Release NON instrumenté, jamais une
  soustraction de cumuls du mur. Balayage apparié T ∈ {1,8,16,24,32,48} ×
  inflight ∈ {1,2} puis {4,8}, cœurs physiques épinglés vs SMT, digest
  complet + `ForestResult` entier comparés. Diagnostic local non décisionnel
  (commande, topologie, affinité, commit, sorties conservés). Le « 49 s →
  30 s » reste un plafond à falsifier, plus jamais une projection — acté
  aussi pour le double comptage cumul/mur que vous relevez.
- Design A : paliers 1→5 tels quels (CSR de rôles mesuré sans changer
  l'aval, `CompactDelta` synchrone, puis queue capacité 1, puis créditée en
  octets — bornée en octets/fids, snapshots immuables sans pointeur vers
  les arènes, drain/exception/jonction au contrat) ; `born = attach &&
  !active` conservé à l'identique ; le scout atomique repoussé en ablation
  séparée, acquire/release partout hors hint.
- GPU : C2 wire d'abord (formats versionnés, budget VRAM, digest de
  sérialisation canonique — vos corrections de volumétrie intégrées :
  4,79 Go de BallData bruts vs payload compact ~92 o à démontrer par
  static_assert), C5 seulement après preuves CPU locales, nouveau pin,
  nouveau GO.

GCP non utilisé par cette livraison.
