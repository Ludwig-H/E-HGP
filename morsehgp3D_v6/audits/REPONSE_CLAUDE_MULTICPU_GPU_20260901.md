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

### § 5.10 — harnais de profil (commit séparé après vos six contre-lectures ; « livré » ne s'écrit qu'au pin)

Le profil reduce est refondu selon votre correctif minimal, point par point :

- record `ReduceProfile` PAR K stocké dans `ForestResult`, imprimé APRÈS
  `run_pipeline` (`%.3f`, somme, RÉSIDUEL aux mêmes bornes, intervalles —
  les recouvrements réduction/réduction et A/réduction se lisent, jamais
  soustraits ; horizons exacts documentés : `begin` après le déplacement
  initial, `liberation` = ev_fid + FidState seulement, colonnes d'intern
  sélectives), avec durée de digest par K, inflight demandé ET pics
  observés ;
- chronomètre armé DÈS L'ENTRÉE de `reduce_fold` (l'init couvre les
  allocations FidState/scratch/reserve + le warmup de prefetch) ;
- sonde de vivacité sous `MHGP6_PROFILE_LIVENESS` SÉPARÉE (sa pollution
  cache/mur sort de l'attribution par défaut), et mesure DEUX-PHASES :
  activer les fids du lot → relever le PIC INTRA-LOT → décrémenter →
  relever la FRONTIÈRE inter-lots — les deux valeurs publiées (votre
  relevé « un lot éteint peut publier zéro » fermé) ;
- frontières renommées : `post_remplissage` (copie des clés dans
  parents/born) vs `materialisation_tri_copie` (tris + copie profonde +
  niveaux + seen) ;
- plus AUCUNE impression avant les `mark` : `profil_intern` (qui fuyait
  dans `t_merge_ms`) et `profil_vivantes` (dans `t_reduce_ms`) passent par
  le record ;
- mode diagnostic `--fold-join=1` (`fold_join_before_next_k`) : B(K) joint
  AVANT A(K+1) — `fold_inflight=1` n'isole pas B, acté ; objet identique,
  seul l'ordonnancement change ;
- mur de référence = Release NON instrumenté, gravé en doctrine dans le
  record.

Smoke local (uniform 2000, inflight=1, join=1) : fenêtres couvrantes
(résiduel 0,000), lignes par K sans entrelacement. Balayage local apparié
T × inflight à suivre en diagnostic non décisionnel.

Vos contre-lectures `9cafe7b6`/`7ec81064`/`71528f8a` intégrées dans la foulée :

- **plus aucune I/O dans les workers** : les records sont DRAINÉS dans
  `RunResult::fold_profiles[K]` sous le verrou de publication (copie seule)
  et imprimés par `print_run` APRÈS le retour de `run_pipeline` ;
- **deux pics séparés** : `pic_reduce_actif` strictement autour de
  `reduce_fold` (RAII — le chevauchement B×B se prouve), l'ancien pic
  renommé `pic_workers_b` (cycle de vie : réduction + digest + attente de
  publication + callback) ;
- **résiduel à bornes identiques** : `mur_local = end − begin` (mêmes bornes
  que les fenêtres) ; `t_reduce`/`t_partition` restent des compteurs
  séparés ;
- **intervalles A par K** (`a_debut`/`a_fin` : expansion + préparation) —
  la concurrence A/B se LIT désormais dans la trace ;
- **cible de build explicite `mhgp6_profile`** (les macros par
  `CMAKE_CXX_FLAGS` ne signent pas un binaire) + **porte d'identité**
  `mhgp6_profil_identite` (objet + digests identiques normal/profil ×
  join 0/1, structure valide, `profil_kind`/`fold_join` signés) ;
- **contrat d'échec CAUSAL** : votre relevé « le CLI ne print jamais après
  un refus » est exact — la porte COMPILÉE `mhgp6_profil_contrat_echec`
  inspecte le `RunResult` directement (refus ⟹ `fold_profiles` vides, pic
  nul) ; `invalidate_provisional` couvre le vecteur ; le check stdout du
  gate Python est requalifié « surface CLI seulement » ;
- `duree_digest_k_ms` renommé (c'était une durée), `FidState` n'est plus
  « une ligne de cache » sans preuve ;
- **scène de panne NON VACUEUSE** (votre `ac6b4bc1` : le budget 4 Kio
  refuse avant `fold_profiles.assign`) : porte
  `mhgp6_profil_contrat_echec_k2` — mutant `fold-inject-a-failure-k2`
  activé AVANT tout run (cache statique des sites), callback K=1 exigé (le
  vecteur a existé et son record K1 fut rempli en vol), panne A à K=2, puis
  profils et pic constatés EFFACÉS au retour terminal (code 4, convention
  de `mhgp6_contrat_echec_fold_k2`).

Et vos `01bd14a9`/`e32262d3` (le handoff) :

- **vivacité enregistrée et exercée** : cible `mhgp6_profile_liveness`
  (deux macros) passée en 3e argument à la porte — `profil_kind`
  exactement `reduce_v2+liveness`, un `profil_vivantes` par K,
  `pic_intra_lot > 0` sur la fixture, frontière ≤ pic ;
- **`fold_join` causal dans la porte** : chaîne
  `a_debut ≤ a_fin ≤ reduce_interne_debut ≤ reduce_interne_fin` par K ;
  sous join=1, `reduce_interne_fin(K) ≤ a_debut(K+1)` ET
  `pic_reduce_actif == pic_workers_b == 1` ; ensembles de K cohérents entre
  lignes forêt/cardinalités/reduce/intern (l'ensemble exact K1..kmax reste
  à la porte exact-K du juge), temps d'intern contrôlés aussi ;
- **builds discriminés** : zéro ligne `profil_*` exigée dans les sorties du
  binaire normal — passer `mhgp6_profile` aux deux arguments échoue
  (contre-épreuve exécutée : DESACCORD) ;
- **scène K2 sous jonction** : `fold_join_before_next_k=true` et le
  callback vérifie le profil K1 STRICTEMENT non vide — la copie précède
  prouvablement A(K=2), le récit ne dépasse plus la preuve ;
- **vocabulaire de preuve fini** : la porte atteste une PROJECTION
  DÉTERMINISTE NOMMÉE (jamais « l'objet » — reformulé partout),
  `duree_digest_foret_k_ms` (la fenêtre ne couvre que `digest_forest_v4`),
  colonnes `fusion_et_lib_parts`/`remap_et_lib_pools` (les libérations
  nommées plutôt que séparées artificiellement), l'hypothèse mémoire de
  `FidState` déclarée comme hypothèse de dimensionnement, jamais un
  diagnostic mesuré.

Votre `9041c191` (horizons) intégré aussi : schémas de colonnes EXACTS par
ligne (`profil_intern` vide ne passe plus), plancher PAR K (Python sur les
`%.3f`, porte C++ sur les doubles bruts — un record K par défaut ne passe
plus même isolé), join=0 documenté PERMISSIF (aucune preuve de
chevauchement — l'imposer rendrait la fixture sensible au scheduler), et le
vocabulaire des horizons réécrit partout : « I/O d'impression du profil »
(le callback et la sonde `/proc/self/statm` restent dans les workers),
`begin` après le déplacement initial, `liberation` = ev_fid + FidState
seulement, `init` sans la croissance de scratch, colonnes d'intern
sélectives, vivacité = TROIS parcours des incidences, `sizeof(FidState)==32`
sans claim de ligne de cache, recouvrements « réduction/réduction et
A/réduction » (jamais « B/B et A/B »).

NOTE D'EXPLOITATION : l'exploitant vient de donner son feu vert pour la
session de MESURE G4 dès que la suite GPU (série C) sera prête — la demande
de GO frais avec nouveau pin vous parviendra à ce moment, conformément à
votre § 6 (preuves CPU locales d'abord, fold CPU visible dans le mur bout
en bout).

Votre arbre de décision pré-enregistré pour la matrice fils × inflight ×
join est adopté tel quel : join=1 améliore le mur à travail B stable ⟹
budget de workers/affinité d'abord ; `materialisation_tri_copie` domine B
isolé ⟹ instruire `CompactDelta` ; le coût vient du recouvrement
A/réduction ⟹ borner la concurrence avant le layout. Aucun facteur de gain
ni « memory-bound » ne sera revendiqué avant.

Vos `2142c798`/`7724e730` (bornes honnêtes) intégrés aussi :

- champs renommés `reduce_interne_debut/fin` et `mur_reduce_interne` — la
  fenêtre couvre le CORPS INTERNE de `reduce_fold` (destructeurs de
  `FoldPrepared`/`Stage`, digest, publication, callback et sonde RSS HORS
  fenêtre) ; le claim est le recouvrement A/RÉDUCTION, jamais l'étage B
  complet ;
- porte Python : projection ATTESTÉE nommée (digest_all + digest_forest_K*
  + lignes `cardinalites K=`), K uniques et croissants, PLANCHER de durées
  cumulées strictement positives (des records par défaut de bonne
  cardinalité ne passent plus) ;
- `profil_intern` renommé honnêtement : `alloc_empreintes`,
  `offsets_diffusion`, `intern_tri` — sans séparation artificielle des
  passes ;
- `#error` si `MHGP6_PROFILE_LIVENESS` sans `MHGP6_PROFILE_REDUCE` ;
  `fold_join` signé dans `temps_fold_mur_ms` (build normal compris) ; la
  télémétrie RSS (`/proc/self/statm` par K sous le verrou de publication)
  est DÉCLARÉE dans la sortie — à désarmer ou signer pour un vrai run de
  débit.

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

## 5. Coutures § 5.13, § 5.14 et § 5.15 — toutes intégrées et falsifiées

Réponse aux trois contre-lectures du 1er septembre (`18b28700`, `63248cb3`,
`b1e5463e`). Chaque couture est fermée dans le commit qui porte cette note ;
les preuves locales sont rejouées au même HEAD.

- **§ 5.13.1 signatures** : chaque record du pilote publie `signature_cpu=`
  et `signature_device=` — sha256 d'une projection canonique
  `mhgp6_parite_v1` couvrant exactement le tuple comparé ; une incohérence
  projection/tuple est un PLANCHER (code 3). Le juge unique
  `morsehgp3D_v6/tests/pilote_juge.py` (porte stub, runner en mode
  `--fichier`, validateur — importé, jamais réimplémenté) ferme records,
  ABBA/BAAB, octets, chronos, lots ; 22 contre-fixtures intégrées tuées.
- **§ 5.13.2 seuils du profil** : 0.0051/0.006, scène historique gravée
  dans `tests/profil_contre_fixture.py` (tuée), scène aux arrondis honnêtes
  restée verte, porte `mhgp6_profil_contre_fixture`.
- **§ 5.13.3 profil** : `CONF_SPECS=aucun`, troisième passage `rotation8`
  (rotation cyclique fixe de 8, runner + validateur la recalculent), note
  GO marquée supersédée dans sa seconde moitié.
- **§ 5.13.4 runner** : `taskset -c` dérivé de la topologie et attesté
  (demandé + effectif), `--repeat=4 --ordre=cpu-device --min-lots=2`
  explicites, axes `SESSION_*` pilotant `MAX_RUN_SECONDS` /
  `GUEST_SHUTDOWN_MINUTES` avant tout garde-fou (bloc profil déplacé en
  amont, surcharge d'environnement = profil `custom`).
- **§ 5.14.1 pin** : `profils/g4_serie_c_v1.env` et
  `morsehgp3D_v6/tests/pilote_juge.py` dans les DEUX listes normatives
  (même ordre), répertoire parent matérialisé, 13 refus de pin au selftest ;
  préflight v5 CONDITIONNEL (sauté quand conf/bench/gpu-v5 = `aucun`,
  jamais un masquage de plancher — le bloc disparaît, planchers par lignée
  exercée).
- **§ 5.14.2 budget** : enveloppe exceptionnelle 7 h / 415 min retenue,
  route device créditée 1,0× la route CPU avant mesure ; le calcul du
  profil canonique est GRAVÉ dans le selftest (estimateur réel extrait du
  cycle de vie : 13 552 s pour une fenêtre de 20 995 s) ; le commentaire
  5 h aligné (déficit 1 257 s au modèle courant).
- **§ 5.14.3 juge** : mode `--fichier` appelé par le runner APRÈS chaque
  famille et AVANT la suivante (verdict gravé `.juge.txt`, troncature au
  refus) ; fermeture des chronos (étage ≥ somme des six composantes ± 0,4 ;
  murs enveloppants ± 0,1 ; « six composantes à 1000 sous un étage à 1 »
  tuée) ; stabilité signature/`nb_total`/octets/lots entre répétitions.
- **§ 5.14.4 reçu** : inventaire exact AVANT `ctest -V` (listage `-N`,
  code jamais neutralisé) ; `FORBIDDEN` non appliqué au transcript CTest
  agrégé (le `REFUS` légitime de `mhgp6_pilote_refus_n` acté) ; deux
  libellés de résumé CTest admis ; `nounits` sur les instantanés
  `nvidia-smi` ; juge épinglé ; K=1..10 exacts et flottants finis pour
  l'attribution ; hashes des binaires exécutés gravés (un seul par phase) ;
  `matrice_resume`/`gpuv6_resume` copiés au reçu durable ; affinité par
  (socket, core) dans le cpuset autorisé, masque RECALCULÉ par le
  validateur pour matrice et attribution.
- **§ 5.15.1 liaison littérale** : dès que `profil == profil_canonique`,
  TOUS les axes communs sont comparés au canon (axes série C et durées
  compris ; axe optionnel absent d'un ancien canon ignoré) —
  `matrice_timeout` 60→61 sous un nom canonique est refusé (scène au
  selftest) ; la commande d'attribution est vérifiée comme celle de la
  matrice (taskset, binaire `mhgp6_profile`, arguments du point).
- **§ 5.15.2 identité pilote/device** : en-tête parsée exactement et liée
  à l'entrée attendue (famille, n, graine, fils) et
  `lot_effectif = min(lot, nb_total)` (5 contre-fixtures) ; le validateur
  lie nom/SM de l'en-tête au build et exige le même UUID au build et aux
  instantanés avant/après chaque pilote ;
  `-DCMAKE_CUDA_ARCHITECTURES=120` passé explicitement et REFUS CMake de
  toute autre valeur.
- **§ 5.15.3 fail-fast** : une troncature matrice/attribution SAUTE tout le
  bloc GPU v6 (build compris, cause publiée, scène `/bin/false` au
  selftest) ; le code de `ctest -N` n'est plus neutralisé.
- **§ 5.15.4 harnais** : `selftest_cycle_vie_v6.sh` porté à l'inventaire
  repo-relatif de 13 fichiers dans deux répertoires — 53/53 vertes AVANT
  commit (les 31 rouges étaient bien la cascade du manifeste obsolète).

Preuves au HEAD de cette note : 117/117 portes `gate` v6 en Release
(l'unique rouge intermédiaire `mhgp6_profil_identite` était la contention
d'un selftest concurrent — repassé seul puis en suite complète propre) ;
selftest campagne TOUT vert (nominal, G4, décision, série C + 23
falsifications série C, fail-fast ×3, budget 13 552 s) ; selftest cycle de
vie 53/53. Il reste : l'export propre du SHA candidat puis l'accusé bref le
gravant, conformément à votre § 5.12/§ 5.15.

GCP non utilisé par cette livraison.

## 6. Premier départ série C refusé par la garde invitée — marge de boot corrigée

Le premier lancement au SHA accusé `5d886db1` (reçu immuable
`receipts/session_g4_20260901_5d886db16c1e_1788286152/`, ~6 min SPOT) a été
arrêté PAR LA GARDE elle-même : pin, préflight budgétaire (13 552 s /
20 995 s), `maxRunDuration=25200` recertifié et démarrage verts, puis
l'armement du coupe-circuit invité a échoué à la relecture. Arithmétique :
VM démarrée 18:09:12 UTC, échéance GCE = +25200 s = 01:09:12 ; `shutdown -P
+415min` armé après ~5 min d'attente SSH/OS Login = 01:09:28, soit 16 s
APRÈS l'échéance GCE — l'assertion `scheduled_epoch <= gce_deadline_epoch`
de `start_and_verify.sh` a refusé, arrêt d'urgence certifié `TERMINATED`
sur la génération exacte, reçu écrit. Le fail-closed a fonctionné exactement
comme conçu ; la faute était la marge : 415 min ne laissent que 300 s entre
le boot et l'échéance GCE, en dessous du délai réel d'armement.

Correctif minimal, gardes INCHANGÉES : `SESSION_INVITE_MINUTES=405`
(24 300 s, sous le plafond exceptionnel 415 de votre § 5.12) — 900 s de
marge de boot, ~3× le délai observé ; fenêtre utile 20 395 s pour 13 552 s
estimés ; borne du selftest budgétaire realignée. Nouveau SHA à graver dans
un accusé mis à jour avant la relance.

GCP : une session avortée par la garde (reçu ci-dessus), VM certifiée
`TERMINATED` ; relance prévue au nouveau SHA.

## 7. Deuxième départ interrompu par un redémarrage du conteneur — arrêt certifié, aucun reçu possible

Le départ au SHA accusé `b97f20ea` (génération VM
`2026-09-01T11:23:08.753-07:00`, garde invitée armée `+405` et RELUE du
premier coup, portes VM v6=117 avec préflight v5 sauté, plans annoncés
conf=0/matrice=48/attrib=4/gpuv6=6) a été interrompu vers 18:45 UTC par un
REDÉMARRAGE DU CONTENEUR de développement (cause externe au protocole) :
superviseur local et `WORK` sous `/tmp` perdus, runner distant tué par le
HUP de la session SSH, aucun reçu durable possible. À la reprise du
conteneur (~6 min plus tard) : `describe` = `RUNNING`, puis
`stop_and_verify.sh` — VM ARRÊTÉE ET CERTIFIÉE `TERMINATED`, aucune autre
VM `project=e-hgp` active. Coût : ~30 min SPOT sans données. Les
coupe-circuits (GCE 25200 s, invité 405 min) auraient borné la dérive même
sans reprise. Relance au même SHA, journal déplacé hors de `/tmp`
(`/workspaces/.ehgp-session-logs/`, survit au conteneur) ; la perte du
`WORK` en cas de nouveau redémarrage reste une limite assumée du
superviseur local — les reçus partiels restent sur le disque de la VM.

GCP : un describe en lecture puis un arrêt certifié ; relance annoncée.

## 8. Réponse au § 5.16 et audit adversarial post-session du protocole série C

### 8.1 Reçu `1788293187` — contrôles manuels et lecture

Vos deux contrôles manuels sont CONFORMES (script rejoué sur les quatre
statuts d'attribution : égalité exacte des 12 jetons, aucun doublon ;
`arch_compilees=120` dans les quatre en-têtes, device/SM égaux au build).
La lecture factuelle du reçu est dans
`NOTE_CLAUDE_RECU_SERIE_C_G4_20260901.md` : 58/58 runs, kernels 154 ms
pour 21,6 M de boules, gain de mur −10 % bridé par le wire hôte (2,6 s) et
la reconstruction hôte (4,1 s), matrice concordante à 1 % sur trois
passages (join=1 +35 %, inflight 2 optimal, digest +23 %), reduce insensible
aux fils et dominé par `materialisation_tri_copie` (31–36 %). Aucune
promotion de statut.

### 8.2 Audit adversarial multi-agents du code livré (b1e5463e..b97f20ea)

Six lecteurs à dimensions distinctes (validateur↔runner, juge du pilote,
robustesse shell, budget/temps, vacuité des selftests, C++/CMake), trois
sceptiques par constat (lecture, reproduction, doctrine), un critique de
complétude : 16 constats, 12 survivants, 4 réfutés, 2 constats du critique.
Tous les survivants sont fermés dans le commit qui porte cette note, chacun
avec sa falsification :

- **errexit muet dans le runner** (2 majeurs, les seuls capables de brûler
  une campagne payée sans reçu) : `cpulist="$(cpu_list_for …)"` et
  `inv_names="$(grep … | sort)"` tuaient le runner sous `set -e` avant les
  troncatures gravées « topologie illisible » et « inventaire vide » —
  substitutions neutralisées (`|| true`), scènes runner « lscpu vide » et
  « ctest -N vide » ajoutées (troncature gravée, manifeste distant écrit,
  bloc GPU sauté).
- **relation invité/GCE à l'égalité = 0 s de budget d'armement** (votre
  § 5.16 et le premier départ) : `GUEST*60 + 300 + 480 <= MAX_RUN` désormais
  refusé avant toute garde (480 s = 600 s certifiables moins la tolérance
  systemd de 120 s) ; défaut d'invité 470→465 min ; frontières 467/468 min
  sous 28800 s gravées au selftest du cycle de vie ; contre-calendrier (p)
  porté à MAX=5280 à cutoff inchangé ; commentaire du profil corrigé (316 s
  après l'échéance SÛRE, budget 600/480 s, marge observée 573 s).
- **liaison d'objet du pilote** (critique + § 5.16) : `digest_all` des
  records lié aux bras `--digest` de la matrice pour la même entrée ET à la
  fixture d'égalité `GPUV6_OBJET_DIGESTS` du profil (quatre familles 50k
  gravées depuis le reçu ; uniform:50000 = v5ref du reçu d98f47296d67 = contrat
  GPU v5 du 27 août) ; `arch_compilees` comparé par le juge (`--arch=120`
  passé par le runner et le validateur) ; mutants `86`, graine, `parite=NON`,
  hex64 tués.
- **égalité d'argv normalisée** (§ 5.16.1) : vecteur exact pour la matrice,
  l'attribution et la commande complète du pilote (seul le chemin du
  binaire est libre) ; mutants « suffixe contradictoire », « doublon »,
  « argument en plus » tués.
- **vacuité des selftests** : falsifications ajoutées pour plan≠profil
  (trois plans série C), bit-identité `digest_all` entre bras `--digest`,
  invariance du grand-livre entre points, frontière 0.006/0.005 de la somme
  d'attribution (refus/acceptation), résumé ctest ≠ inventaire, instantané
  `nvidia-smi` avec unités, cardinal du masque ≠ fils, verdict du juge
  supprimé ; la matrice du selftest passe à 5 points (rotation8 = rotation
  de 3, non triviale) dont `uniform:50000 --digest` pour exercer la liaison
  pilote↔matrice.
- **contre-fixture du profil non causale** : dents isolées (somme seule,
  fermeture seule, frontière honnête) avec le message exigé — un seuil
  régressé à 0.009 rougit désormais.
- **re-validation d'un reçu immuable** (critique) : le validateur refuse
  d'écrire ses résumés dans un répertoire portant `SHA256SUMS` ;
  `revalidate_v6_receipt.sh` re-juge une COPIE du reçu avec le profil
  canonique tiré de `git show <source_commit>` et compare les résumés — le
  reçu `1788293187` re-validé sous le validateur corrigé : 58/58, résumés
  identiques, reçu intact.

Réfutés (non retenus) : bornes inférieures des chronos à 0.0 (l'étage
peut être nul sans candidats), plancher de `h2d_octets_index` (la taille
exacte n'est pas transportée), champs inconnus acceptés (grammaire
extensible voulue), pilote factice sans code non nul (la troncature
« pilote non nul » est exercée par `/bin/false` ailleurs).

Preuves au HEAD de cette note : selftest campagne 120/120, cycle de vie
55/55, intégration v6 + sûreté GCP 83/83, juge 27/27 contre-fixtures,
portes `gate` v6 rejouées. Dette hors périmètre (préexistante au parent
`b1e5463e`, hors CI) : 5 tests `tests/gcp/test_phase5_*`/`test_phase15_*`.

Reste ouvert, avant toute session facturable : la reprise persistante du
superviseur (WORK, clé et artefacts hors `/tmp`, commande de récupération
testée, selftest tuant le superviseur après le handshake) — livraison
séparée ; puis nouvel accusé explicite pour tout quatrième départ.

GCP non utilisé par cette livraison.
