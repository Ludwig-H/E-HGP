# Lecture des audits v3 du 16 août 2026 (tranche A) — rapport pour la conception V4

Fichiers lus intégralement (12), tous datés du 16 août 2026 UTC, dossier `morsehgp3D_v3/audits/`.
Contexte : dialogue entre « Claude » (développeur du prototype) et un ou deux auditeurs humains/IA (« l'auditeur », « l'autre auditeur », « contre-audit »). Cadre constant : `phase=exploration_v3_hors_registre`, `profile=quantized_u16_input_only`, `public_status=not_claimed`.

Ordre logique approximatif de la journée (par chaîne de commits) :
`fc63408/f614b74` → `5a225f3` → rétractations `b62d1f0` → audit consolidé `79e73b6` → descente ciblée `2880328` → `a6171d1` (rétractations télémétrie) → arbitrage `d38cc11`/`8870e6f` → `PairFrame` `566a05e`/`08b7007` → audits `f62d986`/`0d7c08b` → consensus → q2 réel `5eefe084`/`972c20b` → audits q2 → arbitrage court WSPD.

---

## 1. Fichier par fichier

### 1.1 `ARBITRAGE_COURT_WSPD_CAP_ET_SCISSION_20260816.md`

- **Qui/quoi** : l'auditeur arbitre entre sa propre réponse et celle d'un autre auditeur à la question de Claude « pourquoi le front appelé WSPD est-il devenu quadratique ? » (`QUESTION_CLAUDE_WSPD_QUADRATIQUE_20260816.md`).
- **Diagnostic à deux causes** : dans `combined_prefilter_probe`, (1) un rectangle n'était terminal que si `separated ET sous_cap`, (2) la récursion scindait le facteur **le plus peuplé**, pas celui de plus grand diamètre géométrique.
- **Preuve du quadratique par le cap** : si chaque terminal vérifie `|A|,|B| <= C`, alors `binom(n,2) = somme_R |A_R||B_R| <= #R * C^2`, donc `#R >= binom(n,2)/C^2`. Le cap seul suffit à imposer un nombre quadratique de tuiles.
- **La scission par population** retire l'invariant géométrique de la preuve de packing de Callahan–Kosaraju : retirer le cap sans corriger la scission ne rend pas la WSPD linéaire.
- **Architecture à restaurer** : `terminal dès que les deux nœuds sont bien séparés ; scinder le facteur de plus grand diamètre ; le cap est une propriété du scheduler aval ; un dépassement est une continuation, jamais une nouvelle définition du RectId`.
- **Expérience discriminante demandée** : 4 variantes (arrêt × facteur scindé), compteurs `pure_wspd_terminals / capacity_refined_tiles / population_split_terminals` sur `n=8000,16000,32000` et `s=6,8,10` — les tailles et séparations exactes de la feuille de route V4.

### 1.2 `AUDIT_APRES_972C_Q2_CORE_CENSUS_RELATION_Q3Q4_20260816.md`

- **Qui/quoi** : l'auditeur relit `972c20b` (mesure carriers) et `5eefe084` (q2 CoreDepth), répond à la question Q3 de Claude sur l'extension du théorème endpoint à q3/q4. Verdict : jalon reçu sous le nom `q2 CoreDepth end-to-end on real geometry`, **pas** `q2 sparse support generator end-to-end`.
- **Formules gravées** : `Phi(a,b,z) = (a-z)·(b-z)`, `H = -Phi`, `W2(a,b) = {z : Phi<0}` ; décisions de bloc `Phi_max<0 → ALL_OPEN`, `Phi_min>0 → OUTSIDE_CLOSED`, `Phi_min=0 → SHELL_POSSIBLE`, sinon `MIXED`.
- **Lemme endpoint généralisé** : pour `q ∈ {2,3,4}`, l'appartenance au cœur universel exige `H>0` ; si `C ∩ A ≠ ∅`, le triplet `(p,b,p)` donne `H=0`, donc **aucun certificat `ALL_Wq` sûr sur `A×B×C` ne peut réussir quand `C` recouvre `A` ou `B`** — indépendant du coefficient `4/3` et de toute monotonie de `H²`. Condition d'implémentation : le classifieur doit garder explicitement `H_min>0` **avant** tout test carré `c_q·H_min² > upper(E·X)`.
- **Certificat `NONE_W3/NONE_W4`** confirmé : si `H_max<=0` → NONE immédiat ; sinon `4·H_max² <= E_min·X_min → NONE_W3` et `3·H_max² <= E_min·X_min → NONE_W4` (sûr uniquement parce que le carré est pris après restriction à `H>0`).
- **Ce qui manque à q2** : `EdgeKey` en vrais `PointId`, `BallKey` diamétrale, `I_B`/`U_B` exacts, rang/niveau, déduplication `BallKey` (fixture du **carré exact u16** : deux diagonales → même centre, même rayon carré, une seule `BallKey`), continuation réellement sérialisée shell inclus.
- **Codec, 4 réserves** : off-by-one `b_node > dom_ep` (doit être `>=`), lane/rect non liés à l'exécution, masse `lower` non recomputée, `tree_digest` absent.
- **Mesure carrier** : `6843/7140` paires (≈96 %) portent un carrier **faible** (owner maximal faible, sans tie-break canonique) — ne pas l'utiliser comme mesure de la source exacte q3/q4 ; exiger `carrier_weak` vs `carrier_canonical`, fixture annulaire `632 canonical carriers, 0 Wq witnesses`.
- **Ambiguïté ABI signalée** : la masse de `relation_frontier` dans le majorant — option A (tag sur span compté dans MIXED) vs option B (`upper = lower + mixed_mass + relation_mass_upper`) — non tranchée ; l'oubli créerait un faux `CORE_CLEAR`.

### 1.3 `AUDIT_ARBITRAGE_D38_8870_SCHEDULER_PAIRFRAME_20260816.md`

- **Qui/quoi** : arbitrage entre deux contre-audits (`d38cc11`, `8870e6f`) portant sur le pin `a6171d1`, avant l'extraction `PairFrame`. Les deux convergent.
- **Borne de profondeur du radix LBVH reçue** : `MortonKey` 48 bits utiles, tie-break par indice ; `internal_depth <= 48 + ceil(log2 n)`, `leaf_depth <= 49 + ceil(log2 n)` ; pour `n <= 50000 < 2^16` : `<= 64/65`. À typer `RadixLBVHDepthBound-48bitMorton-indexTie`, jamais promue comme propriété générale.
- **Deux quadratiques dans le scheduler CPU** : (1) `frontiere.erase(begin+best)` sur `std::vector` : `somme_t Θ(F_t)` jusqu'à `Θ(n²)` par état ; (2) même bucketisé, le **rescan de la masse de frontière** `mf = Σ population(h)` à chaque tour reste `somme_t Θ(F_t)`. Solution : **masse exacte maintenue incrémentalement**.
- **Lemme du ledger incrémental** : état `(L, M, U=L+M)` avec `L <= N_q(p) <= U` ; split d'un parent `C` : retirer `m(C)` de `M`, enfants `ALL → L`, `NONE → rien`, `MIXED → M` ; `L` non décroissant, `U` non croissant, tests terminaux `O(1)`. **Attention à la saturation** : `L` peut être saturé à `h_q`, mais un `U` saturé ne peut plus redescendre — transporter `frontier_candidate_mass_exact` (non saturée).
- **Lemme de raffinement par batch** : remplacer simultanément un sous-ensemble `S` de parents d'une antichaîne par tous leurs enfants préserve antichaîne, partition de masse et invariant, indépendamment de l'ordre. ABI GPU : `SPLIT_WITNESS_BATCH(state, spans[0:B])`.
- **Buckets** `floor(log2 population) ∈ {0..15}` : bonne politique CPU (sélection `O(1)` à facteur 2 du max), mais **hors ABI de preuve** — un record de continuation doit être reprenable sous une autre politique avec les mêmes identités (gate `cap under policy A / resume under policy B == uncapped under policy C`).
- **`NONE_Wq` de `8870e6f`** reçu comme complément orthogonal : `Hmax=-phi_min`, `Emin=dist²(A,Z)`, `Xmin=dist²(B,Z)`, `q3 NONE si 4Hmax²<=EminXmin`, `q4 NONE si 3Hmax²<=EminXmin` ; exact sur trois singletons.
- Résidus listés : `U4_closed → upper_open`, `kActiveAll → kAllCarrierCoreClear`, `refine_depth_max` supprimé, `pending`/`cap_hits` unifiés.

### 1.4 `AUDIT_ARBITRAGE_F62D_Q1_Q2_CAPS_NONE_PAR_LANE_20260816.md`

- **Qui/quoi** : l'auditeur (commit `0d7c08b`) répond aux deux questions de `NOTE_CLAUDE_PAIRFRAME_ORDONNANCEUR_20260816.md`, après relecture de `08b7007` et du contre-audit `f62d986`.
- **Q1 (poids de lane)** : refusé comme cap. Le coût `CoreDepth` est `W_core <= P·M` (`P=pair_mass`, `M=masse ponctuelle candidate mixte`). Un poids fixe `poids(q)` supposerait `coût aval q = constante(q)·coût du cœur`, ce qui est faux : q4 peut porter `Θ(n)` carriers pour une seule arête (fixture annulaire 632 carriers / 0 témoins). Un poids peut ordonner une file, jamais certifier une ressource. Contrat vectoriel : `point_predicate_eval_cap / mixed_handle_cap / relation_handle_cap / continuation_byte_cap`, puis budgets par étage aval (BallForm/census q2 ; carriers q3 ; existence, énumération, Jung/axial q4), chacun en `count → preflight → fill → validate → publish`.
- **Q2 (élision des `NONE`)** : règle **lane-spécifique**. q3/q4 : `NONE_OPEN` du cœur éliminable (le bord de `W3/W4` n'est pas le shell final). q2 : `H_max<0 → OUTSIDE_CLOSED` éliminable ; `H_max<=0` avec égalité possible → `SHELL_POSSIBLE`, à transférer vers un `BallCensusLedger` ou couvert par un census global indépendant. Les spans relationnels (`MIXED_ENDPOINT`, `OVERLAP_A/B/BOTH`) ne sont **jamais** des `NONE` : un `z ∈ A` peut redevenir témoin après restriction endpoint.
- **Provenance compacte** : en production, remplacer les handles `NONE` par `ElidedNoneProof {point_mass, span_count, Digest128, classifier_schema}` lié à `cloud_epoch/tree_digest/rect_id/lane/endpoints` ; le juge petit n garde la liste complète.
- **Test de cap sans overflow** : `core_tile_fits = (mixed_mass==0) || (pair_mass <= eval_cap / mixed_mass)` ; fixture bloquante `P=64, M=256, F=1, cap=64` (annonce 64, vrai travail 16 384).
- **Sûreté globale de vague** : un cap par état ne borne pas `Σ resident_handles`, `Σ continuation_bytes`, `next_wave_state_count` — préflight global `count → exclusive scan → fill` obligatoire sur GPU.
- 10 gates gravées (G1–G10), dont G5 shell q2 (`H_max=0` avec vrai point `H=0` : `NONE_OPEN=true, OUTSIDE_CLOSED=false`), G9 narrowing (`> UINT32_MAX` refusé avant cast).

### 1.5 `AUDIT_CONSENSUS_APRES_F62_0D7_PAIRFRAME_20260816.md`

- **Qui/quoi** : consensus entre les audits `f62d986` et `0d7c08b` — « il n'existe pas de désaccord mathématique à arbitrer ».
- **P0 confirmé** : `pair_mass*frontier_width` borne des handles, pas le travail ; l'exactification exécute jusqu'à `p*M` lectures de statut quel que soit le nombre de spans factorisant ces `M` points.
- **Trois masses distinctes consolidées** : `L` (preuves `ALL_OPEN`), `M_mix` (spans MIXED ordinaires), `M_rel` (spans relationnels, surborne autorisée) ; `U = L + M_mix + M_rel` ; `core_point_eval_upper = pair_mass * (M_mix + M_rel)` — omettre `M_rel` lors du raccord q2 réel réintroduirait le sous-budget.
- **Optimisation reçue** : l'exactification initialise `count <- L` et ne scanne que les candidats MIXED/relationnels — jamais de rescan des preuves `ALL` par paire.
- **Mutant causal du majorant** : le mauvais algorithme à tuer est `U_bad' = min(h, U_bad - parent_pop + possible_children_pop)` ; fixture `20 + 5` (le span de 5 devient NONE, celui de 20 reste possible ; `raw upper = 20 >= 10` mais `stored bad = 5 < 10` → faux `CORE_CLEAR`).
- **Codec** : `ValidContinuationRoundTrip-v0` reçu ; `FailClosedContinuationCodec-v1` non reçu (préflight, magic/schema/époque/tree digest, antichaîne, doublons, masses recomputées, fin exacte du tampon).
- **Coût de sélection batchée** : `O(number_of_bucket_words + B)`, pas `O(1)`.
- Synthèse en cinq « banalités » : `un handle n'est pas un point ; un poids de lane n'est pas un budget ; un NONE_OPEN q2 n'est pas nécessairement sans shell ; un booléen de budget n'est pas un batch count ; un round-trip valide n'est pas un décodeur fail-closed`.

### 1.6 `AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md`

- **Qui/quoi** : audit consolidé après le pivot pair-major (`5a225f3` fonctionnel, rétractations `b62d1f0`, proposition `79e73b6`). Verdict : première architecture du dossier « qui puisse raisonnablement devenir un générateur exact et physiquement sparse ».
- **Rétractations reçues** : (1) une ancre `W4`-vivante peut avoir `Θ(n)` carriers (régions disjointes par le signe de `H` : `W4 intérieur : H>0`, `carrier aigu : H<0`) — donc aucun cap carriers ∝ `h4`, aucune capacité GPU sur la moyenne, backend collectif/spillable obligatoire ; (2) le cœur de Jung sert aux **intérieurs permanents** (rejet), pas à trouver l'apex.
- **Deux P0** : masque endpoint relationnel (un `z ∈ A` jeté du parent est irrécupérable ; règle : `jamais crédité dans lower_open, conservé dans upper, rejoué après restriction`) ; juge par cardinalité `sparse>=brute` insuffisant (une incidence vraie manquante compensée par une surnuméraire d'ancre morte) — comparer les identités `Target_q4 = {(EdgeKey(a,b), PointId(x))}` avec `missing=0, duplicate=0, false_symbolic_triple=0`.
- **Seuils** : sous `smax=11` : `h2=10`, `h3=9`, `h4=8` — paramétrés par lane.
- **Distinction `upper_open`/`upper_closed`** requise ici (sera **supersédée** pour le cœur q3/q4 par l'audit 1.7 : seul q2 identifie frontière du cœur et shell final).
- **Court-circuit q4** : le premier `ALL_CARRIER` arrête la recherche **existentielle** d'activation, jamais l'énumération complète (`CarrierExistenceFrontier` ≠ `CarrierEnumerationRoot`) ; le certificat doit transporter un vrai `PointId` disjoint de `A ∪ B`.
- **Architecture des trois générateurs** : q2 `PairFrame → Midball/W2 reject_at_10 → exactification → BallForm diamètre → census I_B/U_B → BallKey/RLE → fold` (aucun facteur carrier) ; q3 `W3 reject_at_9 → CarrierBlock (owner canonique, triangle strictement aigu, trois PointId distincts) → miniboule Gram/Cramer → census` — **output-sensitive, familles à sortie quadratique existantes, aucune promesse sous-quadratique** ; q4 `W4 reject_at_8 → existence carrier → énumération complète par arête → Jung kill → Q4SeedAxisTopR4-LBVH → owner parmi les six arêtes → primary carrier → positivité tétraédrique → BallKey → census`.
- **Invariants gravés** : exact-once des paires (LCA unique + induction WSPD), antichaîne de témoins (populations additives, remplacement atomique), monotonie sous restriction endpoint (ALL reste ALL, NONE reste NONE, MIXED rejoué), preuve indépendante de la politique de split (cap → `PENDING_RESOURCE`, jamais `DEAD`/`ACTIVE` par défaut), provenance q4 par règle totale sur vrais `PointId`.
- **Trois notions de parcimonie** à ne jamais confondre : représentation sparse (masse factorisée), travail sparse (peu de jobs physiques), sortie sparse (peu de `BallEvent`) — la troisième est **fausse sur certaines familles adversariales**.

### 1.7 `AUDIT_CONSTRUCTIF_FC634_F614_JONCTION_WSPD_LBVH_20260816.md`

- **Qui/quoi** : audit des commits `fc63408` (carriers conditionnés par vivacité) et `f614b74` (classifieur conjoint `W4 × carrier` lancé depuis la racine — constaté inerte : `dead_w4=0` depuis `(root,root,root)`, ce qui est normal et localise le raccord dans les terminaux WSPD).
- **Contre-famille exacte carriers** : `a=(-R,0,0)`, `b=(R,0,0)`, `D=4R²` ; pour `x=(0,u,v)`, `s=u²+v²` : `E=X=R²+s`, `H=R²-s`. Si `R²<s<3R²` : `E<D`, `X<D`, `H<0` → `abx` carrier aigu, `x ∉ W2`. Anneau entier `X_R = Θ(R²)` carriers, `0` témoin `W4`.
- **Fixture u16 permanente `live_anchor_many_carriers_annulus`** : `a=(1000,1000,1000)`, `b=(1020,1000,1000)`, `x=(1010,1000+u,1000+v)` avec `100<u²+v²<300` → **exactement 632** points ; `W4_count=0`, `canonical_carriers=632`. Tue `carrier-cap-proportional-to-h`, `carrier-average-used-as-cap`, `live-anchor-implies-local-degree-bounded`.
- **Énoncé probabiliste correct** : sous Poisson homogène, `E[#carriers | paire W4-vivante] = (c_d / v_4,d) * (h_4+1)/2`, valeur de référence `≈ 29,335` en dimension 3 pour `h_4=8`. `O(h)` **en espérance homogène uniquement**, `Θ(n)` au pire cas.
- **Contre-fixture Jung (tétraèdre régulier)** : sommets alternés de `{0,2}³` : `a=(0,0,0)`, `b=(0,2,2)`, `x=(2,0,2)`, `y=(2,2,0)` ; circumcentre plan de la face `c0=(2/3,2/3,4/3)`, rayon du cœur `|ab|/4 = sqrt(8)/4`, mais `||y-c0|| = 4/sqrt(3) > sqrt(8)/4` — l'apex est loin du cœur. Mutant permanent `q4-apex-source-limited-to-jung-core`.
- **`W4`-vivant ne borne pas `|ab|`** : `two_lines` = paires très longues vivantes car le fuseau traverse un vide ; la complétude des ancres reste portée par `CKPairTape/WSPD exact-once`, jamais par un k-NN local non certifié.
- **`bloc_tout_w4` par coins** : justifiable par convexité séparée (cône circulaire convexe en `e=z-a` d'angle `< π/2` à `b,z` fixés ; `W4(a,b)` convexe en `z`) — reçu sous réserve d'oracle causal (la branche vaut zéro sur les familles testées).
- **Bug u16** : `acute_owner_gateway_probe.cpp` stocke en `struct P3 { short x,y,z; }` — les valeurs `32768..65535` se replient ; `two_lines` produisait une coordonnée `-1` encodée en Morton u16. Correctif : `int32_t` + garde `0<=coord<=65535` avant Morton ; le tri doit utiliser les mêmes coordonnées que la géométrie.
- **Pipeline recommandé** : `WSPD terminal + ledger W4 hérité → jointure bloc → microtuiles d'arêtes → un seul parcours LBVH fusionné par arête (compte W4 exact avec arrêt à 8 + carriers exacts + overflow explicite) → cover LBVH partagé dans `||2z-a-b||² <= 4D` → top-k axial par seed → owner6/primary/positivité/BallKey/fold` ; bascule `carrier_count * cover_mass > switch_budget` → backend collectif `EdgeCenterShallowCut`.

### 1.8 `AUDIT_PAIRFRAME_08B7007_CAP_TUILE_CONTINUATION_20260816.md`

- **Qui/quoi** : audit `f62d986` du commit `08b7007` (squelette `PairFrame` abstrait). Verdict : extraction reçue, contrat de ressources non reçu.
- **P0 (le contre-exemple d'origine)** : `cout_tuile = pair_mass * frontier_width` dans `pair_frame.hpp`, alors que l'exactificateur de `pair_frame_probe.cpp` lit `pair_mass * frontier_candidate_mass_exact` prédicats. Fixture : `pair_mass=64`, un span mixte de population `256`, `frontier_width=1`, `cap=64` → coût annoncé `64`, réel `16 384` (facteur 256 ; jusqu'à ~`50 000` sur la cible). Correctif : deux caps (`frontier_handle_cap`, `exact_point_eval_cap`), init `count=lower`, test par division.
- **P1 batch** : `witness_budget_available = (budget>0)` puis `budget -= faits` a posteriori — `budget=1, batch=8` produit 8 scissions et un budget négatif. Correctif : `ActionRecord{kind, witness_count}` avec `B_effectif = min(B_demandé, budget_restant, spans_scindables)`.
- **P1 codec** : round-trip sur octets valides reçu ; manquent longueurs, préflight `na/nm`, magic, fin exacte, confrontation `schema_version`/`cloud_epoch`, validation de handles/antichaîne/doublons, frontière relationnelle, authentification LBVH. Header recommandé `ContinuationHeader{magic, schema, header_bytes, payload_bytes, cloud_epoch, tree_digest, crc}` + `expected<CoreContinuation, DecodeError>`.
- **Reçus positifs** : correction `08b7007` du majorant (`lower_open_sat` uint8 + `frontier_candidate_mass_exact` uint32, `upper_open_sat = min(h_q, lower + masse)`) mathématiquement juste ; invariant `L<=N<=U` maintenu ; buckets et reprises croisées ; lemme de batch.
- **P1 mutant** : le mutant du majorant meurt pour « drop children » (faute grossière) au lieu de la vraie perte d'information au-dessus du seuil — fixture `20+5` requise.
- **P2 télémétrie** : `masse_candidate` incrémentée avant distinction ALL/MIXED ; gate `mode_politiques` compare `g_c.terminal_checks` de la **dernière** politique ; `selector_scan_items` sous-compte les lots.

### 1.9 `AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md`

- **Qui/quoi** : audit du commit `2880328` (descente ciblée du plus gros span indécis dans le ledger `W4`). Verdict positif : « probe CPU de branch-and-bound exact », aucune fermeture fausse.
- **Preuve de sûreté** : invariant `L <= N_q(p) <= U` avec `U = L + Σ population(spans possibles)` (spans relationnels comptés — `U` lâche mais jamais trop petit) ; raffinement préservant (enfant `ALL → L`, `NONE → retiré`, `MIXED → frontière`) ; verdicts `L >= h_q → PRUNED_BY_UNIVERSAL_DEPTH` et `U < h_q → CORE_CLEAR` ; la garde `frontiere.size() > 4*kCapFrontiere` est **fail-open** (aucune masse perdue).
- **Mesures (terrain)** : `feuilles : 1 901 072 nœuds, dead_w4=8 262, active_edge=1 969, surcouverture=1 758` ; `grossière seule : 3 461 098 / 3 120 / 447 / 61 276` ; `ciblée : 1 578 364 / 8 262 / 1 969 / 1 758`. Lecture : le majorant grossier est correct mais indécis, les feuilles décident mais détruisent la factorisation, la ciblée récupère le pouvoir de décision (`identiques aux feuilles`) pour ~17 % de nœuds en moins.
- **Deux formulations refusées** : (1) « profondeur gouvernée par le seuil » — contre-famille : 7 témoins dans `W4` et tous les AABB internes à cheval sur la frontière → travail `Θ(n)` même sur arbre équilibré ; formulation correcte : `raffinement piloté par le seuil ; arrêt anticipé dès verdict ; aucune borne déterministe indépendante de n au pire cas` ; (2) `upper<h_q` n'est pas « vivante exactement » mais `CORE_CLEAR` (q3/q4 : ni carrier, ni positivité, ni shallow-ness de la circumboule propre ne sont prouvés — q2 seule exception, census restant dû).
- **Supersession explicite** : le `upper_closed` persistant demandé par l'audit consolidé (1.6) pour le cœur q3/q4 est retiré ; contrat : `CoreDepthLedger q3/q4 : lower_open, upper_open` ; `BallCensusLedger : I_B, U_B`.
- **Politique de scheduling v0** gravée : `lower>=h_q → PRUNED ; upper<h_q → CORE_CLEAR ; pair_mass<=cap → EXACTIFY ; sinon SPLIT_WITNESS / SPLIT_ONE_ENDPOINT / PENDING_RESOURCE`.
- **ABI GPU** : `PairFrameSoA / LaneStateSoA / slab de NodeHandle / continuation spillable` ; vague `classify → segmented_reduce → decide_action → count → exclusive_scan → fill`.

### 1.10 `AUDIT_Q2_5EEFE_972C_DECISION_ORACLE_VS_GENERATEUR_20260816.md`

- **Qui/quoi** : second auditeur sur `5eefe084`/`972c20b`. Reçoit `q2_pairframe_probe` comme `Q2PairFrameDecisionOracle-v0`, **pas** comme `SparseQ2HGPGenerator-v1`.
- **P0 de parcimonie** : `partitionne(A,B,cap_rect)` matérialise physiquement tous les rectangles dans `g_rects` : au pire `Θ(n²/cap_rect)` — avec `n=50 000` et `cap_rect=64` : **≈39 millions de PairFrame initiaux** avant tout certificat. La sortie `Resultat::morte` alloue `n*n` octets. Acceptable en oracle borné `n<=400`, jamais en modèle produit. La WSPD partitionne toutes les paires en **`O(s³ n)` rectangles** en dimension 3 pour séparation fixée.
- **P0 d'identité** : `g_pid[rank]` construit puis **inutilisé** — sorties indexées par rang Morton (`rang_a*n+rang_b`) ; le juge parcourt les mêmes rangs, donc vert même en cas de confusion rang/identité ; pas d'exact-once par `EdgeKey` (l'écrasement de cellule masque une double émission) ; paires `D=0` (positions dupliquées) non filtrées alors que `PROPOSITION.md` impose `D=||b-a||²>0` pour un support q2 propre — les autres multiplicités doivent être conservées.
- **Non-crédit endpoint q2 confirmé par le prédicat lui-même** : `C ∩ A ∋ p` → le produit d'AABB contient `(a=p, z=p)` → `Phi(p,b,p)=0` → `Phi_max>=0` → jamais `ALL_OPEN` sous la stricte `Phi_max<0`. (Cet auditeur ne généralise pas à q3/q4 ; l'audit 1.2 le fait par le lemme `H>0`.)
- **P0 census** : le cap reçu borne le compte ouvert (`pair_mass * mixed_open_candidate_mass`) mais pas le futur census shell (`pair_mass * shell_possible_point_mass`) ; forme `PROPOSITION.md` : `L_open>=10 → fermeture ; U_closed<=9 → petit packet exact rejouable pour intérieur + shell par paire` (sous `smax=11`).
- **P0 codec supplémentaires** : `pair_mass=1234` sérialisée arbitraire acceptée ; validation d'antichaîne `O(F²)` = déni de service potentiel → `max_continuation_handles` ou validation linéaire par ordre d'Euler.
- **Split endpoint jamais exercé sur géométrie** : `endpoint_scindable=false` dans la descente q2 — le verrou principal du pair-major (héritage + replay relationnel + scission endpoint partagée) reste non testé sur géométrie réelle.
- **96 % carrier** : mesuré sur **toutes** les paires, avant vivacité `W3/W4`, owner canonique et activation — à renommer `weak_acute_carrier_incidence_count` ; ne préfigure pas la charge des producteurs.
- **Directive finale** : conserver le probe comme oracle petit `n`, écrire à côté `CKPairTape → BallEvent` ; « étendre le probe jusqu'à 50 000 points reviendrait à perfectionner une matrice quadratique ».

### 1.11 `AUDIT_RECEPTION_A617_TELEMETRIE_NONE_WQ_PAIRFRAME_20260816.md`

- **Qui/quoi** : réception du commit `a6171d1` où Claude rétracte ses deux claims (profondeur bornée par le seuil ; `LIVE_EXACT`). Feu vert pour commencer `PairFrame`, avec réserve sur la télémétrie.
- **Chiffres de la rétractation** : `uniform, n=120, r4=8 : frontier_peak=112, refine_depth_max=100, refine_steps=213 187, cap_hits=0` — aucune petite borne `O(r4)`.
- **Compteurs mal nommés** : `frontier_peak` est une **masse logique d'IDs** (`Σ population(h)`), pas un nombre de spans ; `refine_depth_max` compte des tours de boucle (branches incomparables incluses), pas une profondeur ; `refine_steps` inclut les tests terminaux ; **deux chemins de cap distincts** (cap initial de construction vs cap `4*kCapFrontiere` de descente) dont un n'alimente ni `cap_hits` ni `continuation_mass` — `cap_hits=0` ne prouve rien. Correctifs : `frontier_witness_mass_peak` + `frontier_span_peak`, `witness_splits_total/max_per_state`, `witness_tree_depth_max`, `ContinuationReason::{INITIAL_FRONTIER_CAP, TARGET_REFINEMENT_CAP, ENDPOINT_SPLIT_CAP, OUTPUT_CAP}` et compteurs communs.
- **Théorème `NONE_OPEN_Wq`** (le texte le plus complet) : `H=(z-a)·(b-z)`, `E=||z-a||²`, `X=||b-z||²` ; `Hmax=-phi_min`, `Emin=dist²(A,Z)`, `Xmin=dist²(B,Z)` avec `gap(I,J)=max(0, I.lo-J.hi, J.lo-I.hi)`, `dist²=Σ gap²` ; cœurs `W2 : H>0 ; W3 : H>0 et 4H²>EX ; W4 : H>0 et 3H²>EX` (`c3=4`, `c4=3`) ; **si `Hmax<=0` ou `c_q·Hmax²<=Emin·Xmin`, aucun triplet de `A×B×Z` n'est dans `W_q`** ; exact aux singletons ; fail-open. Largeurs u16 : `Emin,Xmin < 2^34`, `Hmax < 2^35`, `c_q·Hmax² < 2^72`, `Emin·Xmin < 2^68` — `i128` suffit, ni racine ni division.
- **Fixture analytique du fuseau** : `a=(-R,0,0)`, `b=(R,0,0)`, `z=(0,u,v)`, `s=u²+v²` : `E=X=R²+s`, `H=R²-s` ; `z ∈ W2 ⟺ s<R²` ; `z ∈ W3 ⟺ s<R²/3` ; `z ∈ W4 ⟺ s<(2-sqrt(3))R²`. Anneau `(2-sqrt(3))R² <= s < R²` = `W2\W4`, invisible au certificat `NONE_W2`. Fixture u16 : `a=(900,1000,1000)`, `b=(1100,1000,1000)`, `R=100`, `z=(1000,1060,1000)`, `s=3600` ; `(2-sqrt(3))·10000 ≈ 2679,49 < 3600 < 10000` → `W2 oui, W3 non, W4 non`.
- **Diagnostic clé** : `frontier_witness_mass_peak ≈ n` mesure surtout la faiblesse volontaire du majorant (qui ne possède que `NONE_W2` via `Hmax<=0`), pas une limite intrinsèque du branch-and-bound — un singleton dans `W2\W4` restait compté dans `upper_open`.
- **Vocabulaire** : le shell HGP n'est **pas** réservé à q2 — toute BallForm finale q2/q3/q4 a son shell `U_B` ; ce qui est propre à q2 est l'identification `frontière de W2 = shell de la boule diamétrale`. Résidu `U4_closed`/`kActiveAll` à renommer (`U4_open`, `kAllCarrierCoreClear`).
- Métrique de duplication à publier avant pair-major : `ledger_refreshes_total / unique_pair_state_keys`.

### 1.12 `AUDIT_REPONSE_5A225_PAIR_MAJOR_FRONTIER_20260816.md`

- **Qui/quoi** : réponse de l'auditeur à la question de Claude (`NOTE_CLAUDE_GATEWAY_TERNAIRE`, § 8) : faut-il figer `(A,B)` ou le raffiner par tâche `(A,B,C)` ? Réponse : **ni l'un ni l'autre — fausse dichotomie**.
- **Solution** : `PairState(RectId, A, B, W4ThresholdState, AcuteExistenceState, JointWitnessFrontier)` — `C` n'est plus un facteur de tâche mais une **antichaîne interne** ; une scission endpoint est décidée **une seule fois pour l'état entier** puis héritée. Le code actuel refait la même scission endpoint pour chaque branche `C` → coût d'un **produit d'arbres** (cause des « exposants rouges »).
- **P0 masque endpoint** (première formulation) : `L(A,B) = {z : ∀a,b, z≠a,b et z W4-intérieur}` exclut `A ∪ B` ; `U(A,B) = {z : ∃a,b…}` peut les contenir. Tant que l'endpoint n'est pas singleton : `MIXED_ENDPOINT`, contribue à `upper`, jamais à `lower`, rejoué après restriction. Gate minimale `A={a0,a1}, B={b}, z=a1` : parent → `z` dans `U4` seulement ; enfant `{a0}×B` → témoin ordinaire ; enfant `{a1}×B` → masqué.
- **P0 juge** : `total_sparse >= total_brute` accepte la compensation d'une incidence vraie manquante par une incidence d'ancre morte ; exiger identités + rejouer chaque `PairId` des blocs `DEAD_W4` (chacune >= 8 témoins) et la vivacité de chaque paire des `ACTIVE_ALL`.
- **Court-circuit existentiel q4** : un seul nœud `C` classé `ALL_CARRIER` active tout le bloc ; les autres branches carrier sont annulées (OR parallèle : `carrier_certified` bloque l'émission des jobs suivants). L'ancien test négatif « `(A,B)` figé n'élague que des feuilles » (ratio 1,1 point/bloc de `53815f`) mesurait l'énumération de toute la relation, pas la requête existentielle — il ne tranche pas la nouvelle architecture.
- **Fates** : `w4_lower_sat==8 → DEAD_W4 ; carrier impossible → DEAD_NO_CARRIER ; w4_upper_sat<8 et certificat → ACTIVE_EDGE_BLOCK ; pair_mass<=cap → EXACT_PAIR_TILE ; sinon REFINE`. Interdit qu'un job `(state,C)` décide un split endpoint — seul le réducteur d'état le peut. Clé d'état : `(RectId, ANodeKey, BNodeKey)` avec `duplicate_pair_state_keys=0`, `endpoint_split_replays=0`.
- **Preuves** : exact-once par induction ; héritage (`ALL_W4/NONE_W4/ALL_CARRIER/NONE_CARRIER` restent vrais sous restriction, `MIXED` rejoué) ; sûreté des morts ; complétude des actifs (aucun support perdu tant qu'un `MIXED` n'est pas décidé).
- **Chiffre** : surcomptage du proposer fail-open `11–18 %` accepté ; gate `two_lines` (`active_edge=0, seed3_emitted=0, pairid_expanded=0`) reçue.
- **q3 séparé** : le court-circuit existentiel est propre à q4 ; q3 garde les `CarrierBlock` énumératifs vers `Lane3View : pieds des droites, ellipse D/12, profondeur < 9`.

---

## 2. Synthèse transverse

### 2.a Résultats mathématiques établis

**Définitions et prédicats (u16, arithmétique entière exacte, `i128` suffisant).**

```text
Phi(a,b,z) = (a-z)·(b-z)        H(a,b,z) = -Phi(a,b,z) = (z-a)·(b-z)
E(a,z) = ||z-a||²               X(b,z) = ||b-z||²       D = ||b-a||²

W2(a,b) = { z : H > 0 }                       (intérieur strict de la boule diamétrale)
W3(a,b) = { z : H > 0 et 4H² > E·X }          (c3 = 4)
W4(a,b) = { z : H > 0 et 3H² > E·X }          (c4 = 3)

H = 0 : shell de la boule diamétrale (pour q2, shell canonique du support)
H < 0 : extérieur (côté carrier aigu)
```

**Classification de bloc sur AABB (extrema exacts de `Phi`)** :
`Phi_max < 0 → ALL_OPEN ; Phi_min > 0 → OUTSIDE_CLOSED ; Phi_min = 0 → SHELL_POSSIBLE ; sinon MIXED`.

**Certificat `NONE_OPEN_Wq` (théorème, § 6 de 1.11)** : avec
`Hmax = -phi_min`, `Emin = dist²(A,Z)`, `Xmin = dist²(B,Z)`,
`gap(I,J) = max(0, I.lo-J.hi, J.lo-I.hi)`, `dist² = Σ_k gap(·)²` :
si `Hmax <= 0`, ou si `c_q·Hmax² <= Emin·Xmin` (q3 : `4Hmax² <= EminXmin` ; q4 : `3Hmax² <= EminXmin`), aucun triplet de `A×B×Z` n'est dans `W_q`. **Exact aux singletons**, fail-open, sûr uniquement parce que le carré n'est utilisé qu'après restriction à `H>0` (le signe doit être conservé séparément : un test portant sur `H²` seul certifierait le mauvais cône). Bornes u16 : `Emin,Xmin < 2^34`, `Hmax < 2^35`, `c_q·Hmax² < 2^72`, `Emin·Xmin < 2^68`.

**Théorème de non-crédit endpoint (q2, q3, q4)** : tout `Wq` exige `H>0` ; si `C ∩ A ≠ ∅` (ou `∩ B`), le triplet `(p,b,p)` du produit d'AABB donne `H=0` → aucun certificat `ALL_Wq` sûr sur `A×B×C` quand `C` recouvre un endpoint. **Le non-crédit est un théorème ; la conservation (replay après restriction endpoint) est un mécanisme** — un `z ∈ A` peut redevenir témoin dans l'enfant qui ne le contient plus.

**Invariant du ledger de profondeur** : `L <= N_q(p) <= U` avec `L` = masse `ALL_OPEN`, `M` = masse exacte des spans possibles (relationnels inclus en surborne), `U = L + M` ; split : `m(C)` retiré de `M`, enfant `ALL → L`, `NONE → rien`, `MIXED → M` ; `L` non décroissant, `U` non croissant. **Saturation** : `L` saturable à `h_q`, mais `U` doit être dérivé (`upper_open_sat = min(h_q, lower + frontier_candidate_mass_exact)`) — un `U` maintenu saturé ne peut plus redescendre après des `NONE`. **Lemme de batch** : remplacer simultanément plusieurs parents d'une antichaîne par tous leurs enfants préserve antichaîne, masse et invariant, sans dépendance d'ordre.

**Fixture analytique du fuseau (plan médiateur)** : `a=(-R,0,0)`, `b=(R,0,0)`, `z=(0,u,v)`, `s=u²+v²` → `E=X=R²+s`, `H=R²-s`, et :
`z ∈ W2 ⟺ s < R²` ; `z ∈ W3 ⟺ s < R²/3` ; `z ∈ W4 ⟺ s < (2-sqrt(3))·R² ≈ 0,2679·R²`.

**Contre-exemples permanents** :
- *Anneau carriers* : `R² < s < 3R²` → `E<D`, `X<D`, `H<0` : carrier aigu hors de `W2`. Version u16 : `a=(1000,1000,1000)`, `b=(1020,1000,1000)`, `x=(1010,1000+u,1000+v)`, `100<u²+v²<300` → **632 carriers, 0 témoin W4**. Réfute toute borne déterministe `#carriers = O(h)` (vraie seulement **en espérance sous Poisson homogène** : `E[#carriers | paire vivante] = (c_d/v_{4,d})·(h_4+1)/2 ≈ 29,335` pour `d=3`, `h_4=8` ; `Θ(n)` au pire cas).
- *Tétraèdre régulier* : `a=(0,0,0)`, `b=(0,2,2)`, `x=(2,0,2)`, `y=(2,2,0)` ; `c0=(2/3,2/3,4/3)`, `|ab|/4=sqrt(8)/4`, `||y-c0||=4/sqrt(3)` : le cœur de Jung `B(c0,|ab|/4)` ne contient pas l'apex — il ne sert qu'aux **intérieurs permanents** (rejet des seeds).
- *Anneau `W2\W4`* : fixture u16 `a=(900,1000,1000)`, `b=(1100,1000,1000)`, `z=(1000,1060,1000)`, `s=3600` (`2679,49 < 3600 < 10000`) : dans `W2`, hors `W3` et `W4`.
- *Carré exact u16* : deux diagonales → même `BallKey` diamétrale → contrat de déduplication `SupportKey ≠ BallKey`.
- *`two_lines`* : paires très longues `W4`-vivantes (fuseau dans un vide) — réfute « la vivacité borne `|ab|` » et tout voisinage k-NN local comme source de complétude.

**WSPD et LBVH** :
- Cause exacte du quadratique : terminal seulement si `separated ET sous_cap` → `binom(n,2) <= #R·C²` → `#R >= binom(n,2)/C²` ; la scission par population retire l'invariant de packing. Référence : terminal sur séparation seule, scission du facteur de plus grand diamètre ; le cap appartient au scheduler aval (continuation, jamais nouveau `RectId`). La WSPD partitionne toutes les paires en `O(s³·n)` rectangles (dim 3, séparation fixée).
- Exact-once : chaque `PairId` non ordonné appartient à un unique `PairFrame` terminal (LCA unique + induction).
- Profondeur du radix LBVH : `internal_depth <= 48 + ceil(log2 n)` (Morton 48 bits + tie-break d'indice) ; `<= 64` pour `n <= 50 000` ; borne **typée** au backend, non générale.
- Monotonie sous restriction endpoint : `ALL` reste `ALL`, `NONE/OUTSIDE` reste, `MIXED`/`MIXED_ENDPOINT` rejoué.

**Seuils** : sous `smax=11` : `h2=10`, `h3=9`, `h4=8` (`reject_at_10/9/8`), paramétrés par lane.

**Négatifs établis** : aucun seuil ne borne la profondeur ni le travail (`raffinement piloté par le seuil, aucune borne déterministe indépendante de n au pire cas`) ; `CORE_CLEAR ≠ LIVE_EXACT` pour q3/q4 (le cœur universel est un prune, pas la circumboule finale) ; la frontière de `Wq` (q3/q4) n'est **pas** le shell de la boule finale (identification vraie pour q2 seulement) ; la sortie q3 peut être quadratique (générateur output-sensitive obligatoire).

**Élision des `NONE`** (lane-spécifique) : q3/q4 `NONE_OPEN` du cœur éliminable (certificat stable sous restriction) ; q2 : seul `OUTSIDE_CLOSED` (`H_max<0`) inconditionnellement, `SHELL_POSSIBLE` transféré ou couvert par census global indépendant ; spans relationnels **jamais** éliminés ; provenance compacte `ElidedNoneProof{masse, span_count, digest, schema}` en production, liste complète chez le juge.

### 2.b Mesures chiffrées et décisions d'architecture

**Mesures** (toutes CPU, machine non spécifiée dans ces audits ; **aucun temps GPU/G4 mesuré** — SLO GPU marqué « ouvert » partout) :

| Mesure | Valeur |
|---|---|
| Descente `terrain` — feuilles | 1 901 072 nœuds ; dead_w4=8 262 ; active_edge=1 969 ; surcouverture=1 758 |
| — grossière seule | 3 461 098 ; 3 120 ; 447 ; 61 276 |
| — ciblée | 1 578 364 ; **mêmes fates que feuilles** (8 262 / 1 969 / 1 758) |
| `uniform, n=120, r4=8` | frontier_peak=112 ; refine_depth_max=100 ; refine_steps=213 187 ; cap_hits=0 |
| Contre-exemple cap | `P=64, M=256, F=1, cap=64` → travail réel 16 384 (facteur 256, jusqu'à ~50 000 sur cible) |
| Partition capée `n=50 000, cap_rect=64` | ≈ 39 millions de `PairFrame` physiques (`Θ(n²/cap_rect)`) |
| Carriers faibles | 6 843/7 140 paires (≈96 %) — mesure faible, non canonique, sur toutes les paires |
| Fixture annulaire | 632 carriers exacts, 0 témoin W4 |
| E[#carriers] Poisson homogène | ≈ 29,335 par paire W4-vivante (`d=3, h4=8`) |
| Surcomptage proposer fail-open | 11–18 % |
| Rampe carriers conditionnels | `n=200,400,800` — sous-quadratique sur la rampe testée uniquement |
| Cible d'échelle v3 | `n <= 50 000` (`uint32` pour les masses justifié par cette borne) |

**Décisions d'architecture consolidées** :

1. **Quotient pair-major** : `PairFrame` immuable `(RectId, a_node, b_node, pair_mass, geometry)` possédé par le rectangle WSPD + `Lane2/3/4State` autonomes ; `C` = antichaîne interne (`JointWitnessFrontier`), pas un facteur de tâche ; **une scission endpoint décidée une fois par état**, preuves héritées, `MIXED` rejoué. Partage autorisé : `PointStore`, ordre Morton/LBVH, partition neutre des paires, AABB, caches immuables. Partage interdit : fates, seuils, continuations, preuves, sorties.
2. **`CoreDepthLedger`** : `{threshold, lower_open_sat (uint8), frontier_candidate_mass_exact (uint32 non saturé), frontier, relation_frontier, credits, continuation}` ; `upper` dérivé. Cœur q3/q4 : `lower_open/upper_open` uniquement (pas d'`upper_closed` persistant) ; `BallCensusLedger` séparé pour `I_B/U_B`.
3. **Vague GPU** : SoA plates, `classify → segmented_reduce par état → decide (une action par état) → count → exclusive_scan → fill` ; batch de splits d'antichaîne autorisé (`B` borné par le budget **restant**) ; préflight **global** de vague (un cap par état ne borne pas la somme) ; aucune allocation dynamique par état ; overflow → continuation typée ou `resource_exhausted`, jamais une mort ni un actif.
4. **Caps physiques par étage**, jamais un `poids(q)` : `core_point_eval_cap` sur `pair_mass·(M_mix+M_rel)` (test par division, sans overflow), caps de handles séparés, `continuation_byte_cap`, budgets aval propres (q2 census ; q3 carriers ; q4 existence/énumération/Jung/axial). Reçus en vecteur d'unités physiques (`core_point_evals[q]`, `carriers_enumerated[q]`, `bytes_peak[q]`, `wall_time[q]`…).
5. **q4** : `CarrierExistenceFrontier` (court-circuit existentiel : premier `ALL_CARRIER` = activation, OR parallèle, annulation des autres jobs carrier) strictement séparée de `CarrierEnumerationRoot` (énumération complète obligatoire après activation) ; puis Jung (kill) → `Q4SeedAxisTopR4-LBVH` (apex) → owner6/primary/positivité → BallKey/census ; bascule vers backend collectif `EdgeCenterShallowCut` quand `carrier_count · cover_mass > switch_budget` (obligatoire à cause de l'anneau).
6. **Continuation** : sémantique indépendante de la politique (buckets = politique, `PolicyVersion` informatif) ; codec fail-closed avec header `magic/schema/longueurs/cloud_epoch/tree_digest/CRC`, recomputation de `pair_mass` et `lower`, liaison lane/rect attendus, validation linéaire de l'antichaîne, cap anti-DoS de handles ; gate transactionnelle universelle `capped + resume == uncapped` **par identités**.
7. **Buckets CPU** `floor(log2 population)` + masque de non-vides : sélection amortie ; masse de frontière **incrémentale** (jamais de rescan).

### 2.c Bugs, erreurs, rétractations documentés

**Rétractations mathématiques (commit `b62d1f0`)** :
1. `#carriers = O(h)` déterministe — **faux** (anneau 632 ; `Θ(n)` possible) ; vrai seulement en espérance Poisson homogène.
2. Le cœur de Jung `B(c0,|ab|/4)` comme source du 4e sommet — **faux** (tétraèdre régulier) ; il ne sert qu'au rejet par intérieurs permanents.

**Rétractations de claims (commit `a6171d1`, par Claude lui-même)** :
3. « Profondeur gouvernée par le seuil, pas par `n` » — rétracté (contre-famille frontière adversariale).
4. `upper<h_q` = `LIVE_EXACT` — rétracté ; renommé `CORE_CLEAR`.

**P0 de code** :
5. **WSPD quadratique** (`combined_prefilter_probe`) : cap de l'auto-jointure `h_a/h_b` fui dans le critère terminal + scission par population — `#R >= binom(n,2)/C²`.
6. **Cap de tuile en mauvaise unité** (`pair_frame.hpp`) : `pair_mass*frontier_width` (handles) au lieu de `pair_mass*frontier_candidate_mass_exact` (points) — dépassement d'un facteur = population du span (256 → 16 384 sur la fixture).
7. **Masque endpoint destructif** : les feuilles recouvrant `A/B` étaient jetées → un témoin légitime pour d'autres paires devenait irrécupérable après split endpoint (sûr pour `L`, faux pour `U` et l'héritage).
8. **Juge par cardinalité** `sparse >= brute` : compensation possible d'une incidence vraie manquante par une incidence d'ancre morte.
9. **Architecture triple-task** `Tache(A,B,C)` : la même scission endpoint refaite pour chaque branche `C` → coût d'un produit d'arbres (« exposants rouges »).
10. **Majorant saturé incrémental** : ne peut plus redescendre après des `NONE` → faux `CORE_CLEAR` possible (fixture `20+5`) ; corrigé par la masse exacte + dérivation (`08b7007`).
11. **Partition q2 physiquement quadratique** + matrice `n×n` : `Θ(n²/cap_rect)` rectangles matérialisés (≈39 M à 50k), `Resultat::morte` en `n²` octets — acceptés comme oracle `n<=400` seulement.
12. **Identités** : `g_pid` construit puis inutilisé (sortie par rang Morton), pas d'exact-once par `EdgeKey`, paires `D=0` non filtrées.
13. **Probe pas réellement u16** : stockage `short` (repli de `32768..65535`), `two_lines` avec coordonnée `-1` encodée en Morton u16 — clé de tri et prédicat sur deux nuages différents.
14. **Codec** : off-by-one `b_node > dom_ep` (accepte `b_node == dom_ep`, hors domaine `[0,dom_ep)`) ; lane/rect/tree digest non liés ; `pair_mass`/`lower` crus sans recomputation ; validation d'antichaîne `O(F²)` (DoS) ; lecture sans préflight de longueur dans la version initiale.
15. **Budget batch booléen** : `budget=1, batch=8` → 8 scissions, budget négatif.
16. **Second rescan quadratique** : recalcul de `mf = Σ population(frontière)` à chaque tour, même bucketisé.
17. **Télémétrie** : `frontier_peak` (masse logique, pas spans), `refine_depth_max` (pas une profondeur), `refine_steps` (inclut les tests terminaux), `noeuds` (mélange jobs et tests de coins), `masse_candidate` (inclut les `ALL`), deux chemins de cap sans ledger commun, gate `mode_politiques` comparant le compteur de la mauvaise politique, `selector_scan_items` sous-comptant les lots.
18. **Narrowing** : `CoreDepthLedger::sature` rabat `long long → uint32_t` sans garde.
19. **Mutant non causal** : le mutant du majorant mourait pour « drop children » au lieu de la perte d'information au-dessus du seuil.
20. **Shell invisible au juge ouvert** : le mutant `shell-jete` survivait nécessairement à un juge morte/vivante — la quantification u16 produit **beaucoup** d'égalités exactes `H=0` ; porte de cosphéricité séparée requise.
21. **Continuation q2 jamais exercée** : `PENDING` → `push_back` mémoire + estimation `24 + 4*largeur` excluant `shell_spans` ; pas de `encode → destroy → decode → resume` sur géométrie réelle.

### 2.d Pistes fermées dans cette tranche, et pourquoi

| Piste fermée | Cause |
|---|---|
| Cap de carriers `O(h)` / capacité GPU sur la moyenne | anneau 632 carriers, `Θ(n)` au pire cas |
| Cœur de Jung comme source de l'apex q4 | tétraèdre régulier : `||y-c0|| = 4/sqrt(3) > sqrt(8)/4` |
| Scheduler triple-task `Tache(A,B,C)` comme architecture produit | produit d'arbres ; conservé uniquement comme oracle `--scheduler=triple-task` |
| Cap dans le critère terminal WSPD + scission par population | quadratique prouvé `#R >= binom(n,2)/C²` ; perte de l'invariant de packing |
| `poids(q)` fixe comme cap de ressources | coût aval non proportionnel au cœur (q4 `Θ(n)` carriers) |
| `upper_closed` persistant dans le cœur q3/q4 | frontière de `Wq` ≠ shell final ; supersédé — `BallCensusLedger` séparé |
| Fate `LIVE_EXACT` q3/q4 | prouve seulement le passage du prune universel |
| Claim « profondeur/travail bornés par le seuil » | contre-famille adversariale, travail `Θ(n)` |
| Élision uniforme des `NONE` sur les trois lanes | shell q2 (`H=0`) perdu ; relationnels jamais élidables |
| Extension du probe q2 quadratique vers 50k | `Θ(n²/cap_rect)` frames + matrice `n²` ; le probe reste oracle petit n |
| Juge de complétude par cardinalité | compensation d'erreurs ; identités obligatoires |
| Voisinage k-NN local comme source de complétude des ancres | `two_lines` : paires longues vivantes ; complétude = WSPD exact-once |
| Descente systématique aux feuilles (CSR de points) | détruit la factorisation (1,9 M nœuds vs 1,58 M en ciblée, même décision) |
| Majorant grossier **comme politique terminale** | indécis quand la racine porte la masse (mais reste le bon état initial fail-open) |
| Dichotomie « figer `(A,B)` vs raffiner par tâche » | fausse dichotomie ; résolue par `PairState` + frontière interne |
| Domaine carrier dérivé de la frontière résiduelle du cœur | signes disjoints (`carrier : Phi>0` / `W2 : Phi<0`) ; racine d'énumération indépendante obligatoire |

### 2.e Ce que la V4 doit conserver / éviter absolument

**À conserver (acquis directement réutilisables)** :

1. **Toute la géométrie certifiée** : `Phi/H/E/X/D`, définitions `W2/W3/W4` avec `c3=4, c4=3`, classification de bloc par extrema (`ALL_OPEN / OUTSIDE_CLOSED / SHELL_POSSIBLE / MIXED`), certificat `NONE_Wq` (`Hmax=-phi_min`, `Emin/Xmin` par gaps d'axes, **signe d'abord, carré ensuite**), bornes de bits u16 (`i128` suffit, ni racine ni division — GPU-friendly).
2. **Le théorème de non-crédit endpoint** (valable q2/q3/q4 via `H>0`) et son corollaire mécanique : conservation + replay relationnel (`MIXED_ENDPOINT`, `OVERLAP_A/B/BOTH`).
3. **La WSPD de référence** : terminal sur séparation seule, scission du plus grand diamètre, `O(s³n)` rectangles, `RectId` exact-once propriétaire immuable des paires ; le cap est une affaire du scheduler aval (continuation). C'est exactement le cadre `s=6/8/10` de la feuille de route V4.
4. **Le quotient pair-major** : les deux produits à quotienter sont `endpoint × témoin` (PairFrame + frontière interne, une scission endpoint par état) et `carrier × apex` (géométrie axiale). Pour la V4, l'« élimination par h témoins zone cœur » = `CoreDepthLedger` (`L/U` dérivé, masse exacte incrémentale, antichaîne, batch) ; l'« élimination par `h_a/h_b` en dual-tree » doit intégrer d'emblée le masque relationnel (les témoins dans `A`/`B` sont relationnels, pas symétriques aux témoins externes).
5. **Les fixtures permanentes** : anneau 632 carriers, anneau `W2\W4` (`s=3600`), tétraèdre régulier (Jung/apex), carré (dédup `BallKey`), `two_lines`, fixtures d'égalité `H=0` (le u16 produit énormément d'égalités exactes), `20+5` (saturation), `64×256` (cap en points).
6. **La discipline de contrat** : caps physiques vectoriels par étage (`count → preflight → fill → validate → publish`), préflight global de vague, continuations typées fail-closed liées à `cloud_epoch/tree_digest/rect/lane`, gate `capped+resume == uncapped` par identités, juges par identités (`EdgeKey`/`(EdgeKey,PointId)` en **vrais `PointId`**, jamais en rangs Morton), séparation stricte politique/sémantique (reprise croisée entre politiques), trois notions de parcimonie distinctes.
7. **Séparations sémantiques** : `SupportKey ≠ BallKey` ; cœur (`lower_open/upper_open`) ≠ census (`I_B/U_B`) ; existence carrier (activation) ≠ énumération complète (production) ; q2 = seule lane où frontière du cœur et shell final coïncident, donc **meilleur banc d'essai end-to-end**.
8. **Les seuils** `h2=10, h3=9, h4=8` sous `smax=11`, paramétrés par lane, et la profondeur radix bornée `48+ceil(log2 n)` (typée au backend Morton 48 bits).

**À éviter absolument** :

1. **Aucun cap dans le critère terminal de la WSPD**, aucune scission pilotée par la population : c'est la cause prouvée du quadratique v3.
2. **Aucune matérialisation** : ni catalogue résident de toutes les paires (`Θ(n²/cap)` frames), ni matrice indexée par paire, ni descente systématique aux feuilles (CSR de points), ni `carrier×apex`. À des dizaines de millions de points (cible V4), toute structure `∝ n²` est morte-née.
3. **Aucune borne moyenne promue en cap** : carriers, longueurs d'ancres, profondeur « bornée par h » — dimensionner sur la queue, prévoir overflow/continuation/backend collectif dès la conception (l'anneau rend le backend collectif non optionnel).
4. **Aucune confusion d'unités** : handle ≠ point, masse logique ≠ spans physiques ≠ octets, span `ALL` ≠ candidat ; caps et télémétrie en unités physiques nommées, un seul ledger de continuation pour tous les chemins de cap.
5. **Aucun majorant saturé maintenu incrémentalement** ; transporter la masse exacte et dériver.
6. **Aucune élision aveugle** : shell q2 (`H=0`), spans relationnels, et le fait qu'un `NONE_OPEN` q2 n'est pas `OUTSIDE_CLOSED` — décider **avant** l'ABI si le census q2 est global indépendant ou par frontière shell transportée.
7. **Aucun jugement par cardinalité**, aucun identifiant de sortie en rang Morton, aucun oubli des paires `D=0` et des positions dupliquées (les autres multiplicités se conservent).
8. **Aucun claim de sortie sparse universelle** : q3 est output-sensitive ; le contrat V4 « <100 ms K=10 » devra être conditionné aux régimes mesurés (uniforme/terrain/clusters), pas énoncé au pire cas.
9. **Aucun probe « u16 » stockant en `short`** : `int32_t` minimum, garde de domaine avant Morton, mêmes coordonnées pour clé et prédicat.
10. **Aucune décision endpoint prise par un job témoin individuel** — uniquement le réducteur d'état ; aucune politique (buckets, tie-breaks, scores) dans l'ABI de preuve.

---

## Questions ouvertes / ambiguïtés

1. **ABI de la masse relationnelle** : l'option A (tag `RELATION` sur un span compté dans `MIXED`) vs option B (`upper = lower + mixed_mass + relation_mass_upper`) n'est **pas tranchée** dans cette tranche (1.2 § 2.4). Une V4 doit choisir avant d'implémenter le moindre replay.
2. **Architecture du census q2** : census global indépendant (permet d'élider `NONE_OPEN`) vs frontière `SHELL_POSSIBLE` transportée (exige `shell_handle_cap`, `shell_census_eval_cap` et sa continuation) — choix explicitement laissé ouvert (1.4 § 3.3, 1.5 commit C). La règle d'élision q2 en dépend.
3. **`U_closed` de `PROPOSITION.md`** : 1.10 § 2.3 cite `L_open>=10 → fermeture ; U_closed<=9 → petit packet exact` comme forme recommandée pour q2, alors que 1.9/1.11 suppriment `upper_closed` du cœur q3/q4. La cohabitation exacte (q2 garde-t-il un `U_closed` de cœur ?) n'est pas complètement unifiée dans cette tranche.
4. **Constantes de Campbell–Mecke** : `(c_d / v_{4,d})·(h_4+1)/2 ≈ 29,335` — `c_d` et `v_{4,d}` ne sont pas définis dans ces fichiers (renvoi implicite à `NOTE_AUDITEUR_POISSON_CHARGE_CARRIERS_AIGUS_20260815.md`, hors tranche).
5. **`Q4SeedAxisTopR4-LBVH` et `EdgeCenterShallowCut`** : nommés et positionnés (source de l'apex ; backend collectif), mais leur spécification (définition des racines axiales, top-k, arrangement des centres, `Lane3View : pieds des droites, ellipse D/12`) n'est pas dans cette tranche.
6. **Définition canonique du carrier** : le faible est spécifié (`Phi>0`, `|ax|²<=|ab|²`, `|bx|²<=|ab|²`) ; le canonique exige un tie-break « plus petite `EdgeKey` en vrais `PointId` » sur l'owner maximal, mais la règle complète d'owner (fenêtre d'arêtes, cas d'égalités multiples) n'est décrite que partiellement. Le comptage canonique n'a jamais été mesuré (le 96 % est faible).
7. **Aucune mesure de temps mur ni de matériel** dans ces 12 fichiers : tous les chiffres sont des compteurs de nœuds/fates à `n <= 800` (carriers) ou `n=120` (télémétrie) ; SLO GPU/G4 partout « ouvert ». Les contrats V4 (<100 ms K=10 sur G4, dizaines de millions de points) ne s'appuient sur **aucune** donnée de cette tranche.
8. **Cible d'échelle** : les audits raisonnent sous `n <= 50 000` (justifiant `uint32` pour les masses, `O(s³n)` rectangles « raisonnable »). La V4 vise des dizaines de millions de points : les choix de largeurs (`uint32` vs `uint64`), la borne radix (`48+ceil(log2 n)`) et le dimensionnement des continuations doivent être re-dérivés.
9. **Sémantique de `smax=11`** : la correspondance `h2=10, h3=9, h4=8` est donnée en liste (`h_2=smax-1` explicité une fois) ; la définition générale `h_q = smax-(q-1)` est inférée, non énoncée formellement dans cette tranche.
10. **État final de q2 dans v3** : à la clôture de cette tranche, le payload q2 (`BallKey`, `I_B/U_B`, niveau, vrais `PointId`, reprise sérialisée) restait **non produit** ; je ne sais pas, depuis ces seuls fichiers, si des commits ultérieurs l'ont fermé.
11. **Exécutions non reproduites** : plusieurs audits notent explicitement qu'aucun statut CI/workflow n'est attaché aux SHA relus (`2880328`, `a6171d1`) — les « 30 portes » et tableaux de mesures sont des reçus développeur, relus mais non rejoués indépendamment.
