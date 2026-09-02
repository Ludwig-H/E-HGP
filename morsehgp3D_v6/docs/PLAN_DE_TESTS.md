# Plan de tests v6

Conventions héritées v5, toutes reconduites :

- Portes à code de sortie **exact** via `cmake/run_expect.cmake`
  (`mhgp6_gate`) : 0 conforme, 1 désaccord du juge, 2 refus avant calcul,
  3 plancher/invariant violé, 4 mutant tué. Crash par signal refusé partout.
- Labels : `gate` (défaut), `oracle` (petits n, vérité établie),
  `scale8000/16000/32000` (les seules tailles où une conclusion de coût
  s'énonce), `slow`.
- **Planchers `--min-*`** sur toute porte contre le vert-par-vacuité
  (violation = code 3).
- **Mutants causaux** : registre unique (`src/core/mutants.hpp::kMutants`),
  points d'injection `MHGP6_MUTANT("nom")` dans le code de production,
  compilés seulement sous `MHGP6_TESTING` (posé par `mhgp6_executable`,
  jamais par `mhgp6_product_executable`) ; nom inconnu refusé code 2. Cible :
  chaque nom = un point d'injection + une porte code 4 EXÉCUTÉE. État réel
  (recompté le 2 septembre après les contre-corrections des relectures, par
  `grep -rn MHGP6_MUTANT src/ cli/ oracle/`, hors `#define` et hors lignes de
  commentaire) : **107 noms au registre** et **116 lignes de point
  d'injection** pour **105 noms distincts** porteurs d'un site hôte (les sept
  `gpu-*` de census restent traduits en drapeaux device, sans site
  `MHGP6_MUTANT` hôte). Quatre noms ajoutés le 2 septembre :
  `hwm-instant-rss` (palier P2) puis, en contre-correction,
  `provisional-keep-sum-parents` (`invalidate_provisional` n'efface plus
  Σ|parents| : un **préfixe exact de payload** — le Σ|parents| des forêts déjà
  publiées — survivrait à un refus ; tué code 4 par
  `mhgp6_contrat_echec_fuite_sum_parents`), `drop-stage-milestone` (un jalon de
  résidence n'est plus relevé du tout ; tué par le plancher de jalons jugés) et
  `keep-ball-chunks` (les trois libérations par tranche du palier P3 retirées ;
  tué par le plafond de coexistence du census). Deux noms étaient **orphelins** — au registre, avec un
  point d'injection gardé par `kmax < 10`, mais qu'aucune porte n'exerçait
  puisque aucune porte v6 ne tournait à `smax < 11`. Ils sont tués depuis le
  palier P1 : `prefix-tamper-batch-levels` (par l'invariant de lot
  `batch_levels.size() == batches`, vérifié au callback) et
  `prefix-tamper-event-order` (par le témoin du multiensemble canonique
  d'événements ; **aucun digest ni aucune cardinalité ne peut le tuer** — voir
  la ligne « préfixe exact » ci-dessous).
  Décompte du palier KeyCSR, conservé : 100 points d'injection pour 91 noms distincts porteurs d'un site
  hôte (les sept `gpu-*` de census sont traduits en drapeaux device, sans
  site `MHGP6_MUTANT` hôte ; `wspd-cap-terminal` et `wspd-split-heaviest`
  ont un site sur la route FUSIONNÉE en plus de la route brute) ;
  **70 noms distincts** tués par une porte exécutée, dont les 15 `csr-*`
  (`mhgp6_fold_csr_mutant_*` sur le champ annoncé ; la boucle
  `mhgp6_mutant_csr_*` ne reprend que ceux qui changent l'objet — jamais un
  refus compté comme mise à mort) : mutants dédiés + boucle de divergence d'objet
  `mhgp6_mutant_*` + `family-scanline-overshoot`.
  `wspd-drop-rect` est désormais UNE omission par DESCENTE appliquée après la
  fusion ordonnée, masse omise soustraite du grand-livre reconstruit
  (`emis + tués + omis == attendu`, delta −1 littéral gravé par
  `mhgp6_fused_mutant_droprect` ; un mutant hors déclaration rend 3, jamais
  4). Le reste `[PRÉVU]` avec les portes v5 à porter
  (`fold-inject-b-exception-k3` exige le juge d'in-flight dédié : il termine
  par signal, jamais par la boucle de conformité). Un contrôle textuel
  registre ≡ grep est un complément, jamais un kill.
- Équivariance par permutation physique et par réétiquetage (`PointId` ≠
  index dense ≠ rang Morton, mutant `dense-pointid`).
- Sortie **bit-identique** quel que soit le nombre de fils (fils ∈ {1, 8, max})
  et `fold_inflight` ∈ {1, 2, 8} ; ouvriers mesurés, jamais déclarés.
- Jamais de vérification exhaustive : les théorèmes s'invoquent, on grave
  leurs fixtures d'égalité ; exception : oracles bornés n ≤ 12–14 qui
  **établissent** la vérité.
- Oracle à arithmétique volontairement autre (`oracle/obig.hpp`, limbes
  32 bits signe-magnitude, échec fermé par drapeau collant) ; le juge du juge
  (`mhgp6_obig_selftest`) contre `__int128` et une reconstruction
  indépendante.

## Portes par étage — état RÉEL au 31 août ; tout ce qui n'est pas dans
`CMakeLists.txt` est `[PRÉVU]`, jamais implicite

| Étage | Portes |
|---|---|
| cœur arithmétique | `mhgp6_arith_selftest` (bornes, U192/U320, DI128 vs __int128 échantillonné), `mhgp6_sha256_selftest` (FIPS + streaming). `[PRÉVU]` : `mhgp6_level_cmp` contre l'oracle 384 bits (mutant `level-trunc-hi`), `mhgp6_dint_gate` complet, porte d'égalité SHA-NI/portable |
| familles | `mhgp6_families_fixture` : déterminisme, profil, unicité, cardinalité (l'égalité bit à bit aux nuages v5 a été vérifiée hors porte à la livraison — 36 configurations). `[PRÉVU]` : digests gravés par famille et mutant `family-scanline-overshoot` raccordé |
| index | `mhgp6_tree_selftest` (structure, boîtes, équivariance) |
| WSPD | ledger exact `Σ émis + Σ tués = C(n,2) − Σ C(mult,2)` ; mutants `wspd-cap-terminal`, `wspd-split-heaviest`, `wspd-drop-rect` ; `--check-permutation` |
| descente fusionnée | `mhgp6_fused_descent_gate` : listes identiques à la triple descente test-only, avec `smax_effective` (cas `collinear_seven` à 9 points gravé) ; mutant `fused-mask-stuck` |
| fuseaux/facteurs | fixtures W2 ⊃ W3 ⊃ W4, boule-cœur ⊆ fuseau ; route M : porte différentielle contre le produit direct (`min(hist, need)` par lane), mutants `endpoint-credit-use-weight`, `factor-none-overclaim` |
| crédits/tape | mutants `credit-compose-sum`, `core-partial-exclude` ; fixture croisée de lanes (W3-pas-W4) ; fixtures rôles A∪B (membre de A complétion valide, seed valide) |
| tueurs d'ancre | fixtures F1–F11 portées ; secteurs : fixture croisée + mutant `sector-credit-global` ; grille : fixtures F9/F10 + mutants `cell-kill-h-minus-one`, `cell-kill-nonstrict` |
| sweep q4 | **oracle du sweep** (re-balayage exhaustif en μ, échange des quantificateurs, racines/frontières) ; fixtures : relais `F1=μ+1, F2=1−μ`, racines confondues, complétion incidente (compte zéro), clip d'égalité à μ*, les trois cas B=0, sortie dans cellule profonde avant portion shallow ; mutants `sweep-drop-exit-root`, `sweep-nonstrict-depth`, `sweep-skip-fragment`, `sweep-completion-from-witness-tape`, `chord-dead-skip-positive` (hérité) |
| RLE/census | mutants `rle-drop`, `depth-threshold-minus-one`, `range-add-max-le-zero`, `census-nonstrict`, `skip-full-census` ; fixtures plateau (carré cocyclique) |
| fold/rendu | mutants `drop-nonmerge`, `attach-prebatch`, `repr-ties`, `binary-ties`, `canonical-is-uf-root`, `fold-inject-a-failure-k2` tués (boucle de divergence d'objet) ; sondes d'ablation du reduce `ablation-mat-sans-copie`, `ablation-mat-sans-tris`, `ablation-post-cle-factice` (chacune change l'objet, tuée dans la même boucle ; binaire `mhgp6_profile_sonde` sous `MHGP6_TESTING` seul à accepter `--inject=`, le produit refuse : `mhgp6_profile_refuse_inject`, `mhgp6_profile_sonde_refuse_inconnu`, allowlist des trois ablations sans item vide `mhgp6_profile_sonde_refuse_inject_vide` / `_virgule` / `_mutant_production` ; reçu `bench/sonde_ablation_reduce.sh`, jamais un mur). **KeyCSR** (stockage `csr_facet_keys_v1`, route `--layout=csr`, même objet, aucune route de repli) : `mhgp6_fold_csr_{fixtures,offsets,overflow,copie,pipeline}` — 13 fixtures gravées du fold + refus amont sous csr (kind csr signé, payload vide, `--min-refus`) (bras classique contre texte, compteurs et pins de digest ; bras csr contre le classique par `first_divergence`, lecteur tiers de `tests/forest_witness.hpp` lisant les deux stockages À CRU, sans l'accesseur `delta(i)`), validateur d'offsets (cinq contrôles, message exact), gardes de capacité (plafond d'append ET majorant, deux crochets test-only distincts), copie autonome post-callback, matrice fils × inflight × join × layout ; 15 mutants `csr-*` tués sur le champ ANNONCÉ (`--expect-divergence=`, sinon code 1, jugé en `--fixtures` comme en `--overflow`) + refus pipeline `mhgp6_fold_csr_refus_csr-offset-*` (invariant, zéro callback, provisoires vides — SEULE preuve des `csr-offset-*`, exclus de la boucle de conformité où tout statut non complet vaudrait 4 par vacuité de refus) + `csr-inject-bad-alloc` (`bad_alloc` d'arène capturé dans le fold → `resource_exhausted`, payload vide, zéro callback, jamais une exception hors de `reduce_fold`) + boucle `mhgp6_mutant_csr_*` (mutants qui changent l'objet seulement) ; rejeu « catalogue + deltas → partition » (seconde autorité, deux layouts, fixtures et pipeline) ; dent de compilation : `delta(i)` et `for_each_delta` refusés sur un `ForestResult` temporaire ; pré-inscription de mesure `mhgp6_plan_keycsr_gate` (`bench/plan_keycsr.py` : graine dérivée `0xa2ffb4db2884ddc4`, SplitMix64, Fisher–Yates spécifié, fixture des orientations). `[PRÉVU]` : juge borné n ≤ 14 (miniboule + cliques + Kruskal à lots), K=1 ≡ MST indépendant, juge d'in-flight (`fold-inject-b-exception-k3`), `render-active-only`, planchers |
| conformité v5 | `mhgp6_conformity_*` : `digest_all` + `digest_forest_K*` (l'OBJET) ≡ `receipts/conformite_v5/` sur 5 familles × {8000, 16000, 32000} (labels scale*) et petites tailles en `gate` ; le digest candidats v5-compat est rapporté, jamais un critère (cover q4 coefficient 4) ; golden post-préfiltre v6 gravé (uniform 400) ; + `mhgp6_conformity_csr_*` : MÊMES reçus sous `--layout=csr` (8 petites en `gate`, 15 `scale*`) avec non-vacuité `csr_fallback=0` et `ordres_storage_conformes=kmax_eff`, refus 2 d'un layout inconnu (`mhgp6_conformity_refus_layout_inconnu`, `mhgp6_cli_refus_layout_{inconnu,vide}`), signatures CLI `mhgp6_cli_layout_{classic,csr}_signature` |
| préfixe exact (P1) | `mhgp6_prefix_<famille>_<n>` : **23 portes**, une par reçu de `receipts/conformite_v5/`, à `smax=6` — les `digest_forest_K1..K5` **et** les six cardinalités `K=1..5` doivent égaler celles de la référence gravée à `smax=11` (8 en `gate`, 15 en `scale8000/16000/32000`). La référence doit être **strictement plus longue** que le préfixe (`mhgp6_prefix_refus_reference_courte`, code 2, fixture `tests/refs/uniform_400_cinq_ordres.txt`), sans quoi la porte se validerait elle-même ; `digest_all` n'est **jamais** comparé (il chaîne un nombre différent d'ordres) et le digest candidats v5-compat diverge légitimement (l'élagage de génération dépend de `smax`). Planchers `--min-deltas/--min-facets/--min-events` gravés à ~50 % du mesuré et accumulés sur les cardinalités **du run** (`got`), jamais sur celles de la référence gravée : un plancher doit être un observable du calcul jugé et rester un témoin si la comparaison régressait. `--min-orders` n'est **pas** un plancher de couverture : `cmp_orders` vaut `kmax_eff` par construction (boucle `K = 1..kmax_eff` sans continuation), c'est le **témoin que le câblage des planchers est vivant**, et rien d'autre. Refus : `mhgp6_prefix_refus_smax11`, `mhgp6_prefix_refus_sans_reference` (code 2) ; plancher câblé `mhgp6_prefix_plancher_cable` (code 3). Mutants : `mhgp6_prefix_mutant_batch_levels` (4), `mhgp6_prefix_mutant_depth_smax6` (4, le seuil `h_q = smax + 1 − arité` exercé à `smax=6` en plus de `smax=11`), `mhgp6_prefix_mutant_event_order` (4, sous `--prefix-witness`) et sa **contre-fixture gravée** `mhgp6_prefix_contre_event_order_sans_temoin` (code **3 = survivant**) : sous `--prefix` seul, l'échange `interior[0]`/`interior[1]` ne change **ni** les facettes (ensembles triés par `facet_minus`), **ni** un digest, **ni** une cardinalité — la porte ne ment donc pas sur sa portée. Témoin `--prefix-witness` (`mhgp6_prefix_temoin_evenements`, code 0) : second run `smax=11` dans le **même processus**, multiensembles d'événements **triés** comparés ordre par ordre (l'ordre brut dépend du découpage en tranches, qui diffère entre les deux `smax`) ; le mutant étant gardé par `kmax < 10`, il frappe le préfixe et jamais le témoin — la comparaison est causale. **PORTÉE EXACTE du témoin** (rectification du 2 septembre) : c'est cette garde qui rend le mutant visible, donc le témoin voit les défauts d'ordre **différentiels en `smax`** et **pas** un défaut d'ordre **uniforme** — un défaut qui permuterait les intérieurs à tous les ordres et pour tout `smax` subirait la même permutation dans les deux bras, `canonical_events` les trierait à l'identique et le témoin rendrait 0. Fermer cette classe demanderait de **décider** l'ordre des intérieurs à la source (`ball_census` remplit `interior_ids` dans l'ordre de dépilement d'une pile DFS, **pas** trié) : c'est un coût sur le chemin produit, non payé ici, et donc `[PRÉVU]` |
| résidence des jalons et du census (P2/P3) | `mhgp6_residence_*` : **portes de RÉSIDENCE, jamais une preuve de correction** — elles jugent l'instrumentation mémoire et la résidence de l'étage de census, pas l'objet (`opt.digest=false` : un run aux digests faux les passerait). Label CTest **`gate;residence`** : elles tournent avec la suite mais ne comptent pas parmi les portes de correction (`ctest -L residence` les isole, `ctest -L gate -LE residence` compte les autres). Les six `rss_mb` sont des **instantanés** ; `residence_hwm_mb` relève `VmHWM` aux **mêmes** frontières et `residence_increment_mb` en publie les **incréments** — `hwm[j] − hwm[j−1]` est la **seule** quantité imputable à l'intervalle, `hwm[j] − rss[j]` n'étant qu'un majorant global (un étage qui n'alloue rien affiche un écart hérité). Jugé : **(0)** une **sonde auto-portée** `mmap`/touch/`munmap` de taille adaptative, exécutée **après** le pipeline — le noyau retire les pages de façon déterministe, l'allocateur n'a **aucune autorité** sur le verdict : c'est elle qui tue `hwm-instant-rss` (`mhgp6_residence_mutant_hwm_instantane`, code 4) ; **(1)** HWM non nul à chaque jalon relevé **et** `--min-jalons` (mutant `drop-stage-milestone`, `mhgp6_residence_mutant_jalon_manquant`, code 4 — sans ce compte la porte sautait le jalon manquant et restait verte) ; **(2)** `hwm ≥ rss` au même jalon à la tolérance ; **(3)** monotonie du pic **à la même tolérance et non stricte** — **contre-mesure du 2 septembre** : `/proc/pid/status` publie `VmHWM = max(mm->hiwater_rss, rss courant)` et `hiwater_rss` n'est rafraîchi qu'à certains points, d'où des **baisses mesurées** de `VmHWM` (0,758 Mo sur le pipeline à `smax=6`, 0,18 Mo en sonde dédiée) : exiger la monotonie stricte rendrait 3 sur du code sain ; **(4)** `hwm[fin] ≥ max_j rss_mb[j]` ; **(5)** cohérence avec `ru_maxrss`, **explicitement pas une seconde source** (`VmHWM` et `ru_maxrss` lisent le **même** champ `mm->hiwater_rss` : l'égalité est une tautologie, elle n'est comptée dans aucune propriété jugée) ; **(6)** identité `census_balls == expand.survivors` ; **(7)** **PLAFOND** `--max-coexistence-census-pct` sur la coexistence **mesurée et déterministe** de la fusion du census (`expand.census_merge_peak_bytes` : octets copiés + octets encore détenus par les tranches non consommées, maximisés sur les pas ; fonction de l'entrée, invariante par nombre de fils, **insensible à l'allocateur**) — c'est la garde du palier P3 et le tueur de `keep-ball-chunks` (`mhgp6_residence_mutant_tranches_gardees`, code 4 ; mesuré : 155,2 % sain contre 255,2 % sous mutant à `smax=11`, 193,8 % contre 293,8 % à `smax=6`). **Aucun plancher mémoire** : `--min-hwm-mb` et `--min-ecart-fin-mb` ont été **retirés** — mesuré, `MALLOC_TRIM_THRESHOLD_` suffit à faire tomber l'écart de fin à 0 sur du code **sain** (code 3 indiscernable d'une régression), et dans un chantier qui abaisse le mur un plancher mémoire rougirait sur un palier **réussi**. Un plafond sur le RSS ne séparait pas non plus (incrément de pic du census 201 Mo avec libération contre 213 sans, à `n=2000`) : glibc ne rend une tranche libérée à l'OS qu'au-delà de son seuil dynamique de `mmap`. Planchers conservés, **déterministes** : `--min-census-balls`, `--min-plateau-balls`, `--min-sum-parents`. Contre-fixtures câblées : `mhgp6_residence_plafond_cable` et `mhgp6_residence_plancher_cable` (code 3), refus `mhgp6_residence_refus_smax` (code 2) ; instrumentation indisponible = **refus code 2 nommé**, jamais un plancher violé. Deux compteurs neufs publiés par `print_run` : `boules_plateau` / `plateau_pct` et `somme_parents_total` + `somme_parents_par_K` (la valeur par K **existait déjà**, enfouie dans `stockage_foret … cles_parents=` ; le total n'existait nulle part) ; `boules_census` **fait double emploi** avec `survivantes=` — l'identité (6) le transforme en observable au lieu d'un doublon |
| cover q4 | `mhgp6_cover_coef4` (contre-fixture tétraèdre régulier + z, frontière de génération) + mutant `q4-cover-coef3` |
| barrière de génération/census | `mhgp6_linked_arcs_u16` + mutant d'oracle i64 (portée : génération→census ; l'extension aux facettes de forêt est `[PRÉVU]`) |
| frontières du sweep | `mhgp6_sweep_frontieres` (F1–F5 : racines égales, extrémité de Jung exacte, B=0, complétion dans le facteur, profondeur h4−1) + 2 mutants |
| parallélisme | mutants `par-drop-shard`, `par-drop-ball-chunk` tués (boucle) ; `mhgp6_fold_csr_pipeline` : matrice fils {1,8} × inflight {1,2} × join {0,1} × layout {classic,csr} sur les sept familles des petites conformités, témoin complet par K (`first_divergence`) contre la référence classique 1 fil, digests/cartes/totaux identiques, rejeu csr par K. `[PRÉVU]` : `parallel-sort-unstable`, `fold_inflight=8` |
| profil reduce (§ 5.10) | cibles EXPLICITES `mhgp6_profile` et `mhgp6_profile_liveness` (identité de build signée par la cible, jamais par des flags) ; `mhgp6_profil_identite` : la PROJECTION DÉTERMINISTE NOMMÉE (`digest_all` + `digest_forest_K*` + `cardinalites K=` — ni `batch_levels` ni le `ForestResult` complet) identique entre normal/profil/vivacité × join 0/1 × layout classic/csr (jeton `layout=` exigé sur `profil_kind=`, aucune colonne ajoutée à `profil_reduce`/`profil_intern`), builds DISCRIMINÉS (zéro ligne `profil_*` côté normal), structure valide (K cohérents entre forêt/cardinalités/reduce/intern/vivantes, temps finis non négatifs, fermeture somme/résiduel aux bornes INTERNES `mur_reduce_interne`, planchers strictement positifs), causalité de `fold_join` (chaîne A→reduce ordonnée par K ; join=1 ⟹ sérialisation inter-K et pics à 1), vivacité (pic intra-lot > 0, frontière ≤ pic) ; `mhgp6_profil_contrat_echec` + `mhgp6_profil_contrat_echec_k2` (COMPILÉS, inspectent le `RunResult` — le CLI ne print jamais après un refus ; scène K2 sous jonction, profil K1 non vide vérifié au callback, effacement au terminal) ; `mhgp6_profil_contre_fixture` (§ 5.13 : la scène « neuf composantes nulles, somme=0.008, residuel=0.012, mur=0.020 » tuée par les seuils serrés 0.0051/0.006 ; audit post-session : DENTS ISOLÉES — dérive de somme 0.008 à fermeture exacte tuée par la seule dent somme avec son message, écart de fermeture 0.007 à somme exacte tué par la seule dent fermeture, frontière honnête 0.005 acceptée — le juge exercé est le VRAI `check_profile_output` importé). Le binaire de profil n'est JAMAIS un mur de débit |
| caps/budget | `mhgp6_caps_refus` (fenêtres (u)/(a)/(a0)/(a2)/(b)/(c)/(w)/(w2)/(f)/(d)) + mutants `caps-drop-emission`, `caps-late-wave-check`, `caps-skip-prefusion-budget` (garde 2E AVANT la fusion globale, § 6.1 de la réponse auditeurs — sautée, le refus retombe sur le tri APRÈS la matérialisation du payload logique nommé) ; signature CLI `mhgp6_cli_budget_signature` (code + ligne exacte en une exécution) ; **étage nommé d'un `bad_alloc`** (alerte G4 du 2 septembre) : mutants `caps-throw-bad-alloc-provision` (provision du message, avant le corps), `caps-throw-bad-alloc-census` (fil principal) et `caps-throw-bad-alloc-fold` (worker de l'étage B, K=1) injectés dans `src/pipeline/run.hpp`, tués par `tests/bad_alloc_gate.cpp` — portes `mhgp6_bad_alloc_temoin` (code 0, anti-vacuité : la même scène complète sans mutant), `mhgp6_bad_alloc_etage_provision`, `mhgp6_bad_alloc_etage_census` et `mhgp6_bad_alloc_etage_fold` (code EXACT du refus 2 **et** `EXPECT_LINE` portant `allocation impossible a l'etage <nom>` sur la MÊME exécution ; pour `fold_inflight` ∈ {1, 2, 8} : curseur `stage_reached` égal à l'étage annoncé, zéro callback `on_forest`, aucun provisoire, jamais un abort 134). La capture n'est pas une garantie anti-OOM : l'OOM killer reste hors de portée et `RLIMIT_AS` ne borne pas le RSS | **Gardes dures du fold (P5)** : `fold_capacity_ok` porte les deux premiers refus typés d'une campagne d'échelle (événements ≥ (2^32−1)/11, incidences > 2^31−1), qui tirent au-delà du million de points — inexerçables jusqu'ici. Crochets abaissables `fold_events_cap_for_tests` / `fold_incidences_cap_for_tests` sous `MHGP6_TESTING` seulement (le binaire produit n'a que des constantes) ; portes `mhgp6_fold_cap_{evenements,incidences}` (code 0, témoin non abaissé en anti-vacuité, message nommant le plafond STRUCTUREL et l'ordre fautif, zéro callback, aucun provisoire, trois ordonnancements), mutant `caps-fold-guard-skip` tué code 4 sur les deux gardes (le refus attendu devient absent), plancher câblé et refus d'argument.
| GPU série C (hôte) | `mhgp6_executor_pool` + mutants `pool-serial`, `pool-drop-exception`, `pool-worker-resume-after-fatal` (fixture permanente scénario 10 : second travail en file, fatal déclenché, AUCUN travail post-fatal — la course § 5.6), `pool-activate-after-unlock` (scénario 13 : hook test-only, fermeture linéarisée après le pop — le ticket doit être VU actif) ; scénarios 11 (échecs de construction) et 12 (fenêtre file→actif N=2) ; témoin stub `mhgp6_device_witness_stub` : nominal 0, trois dents à 4 (carry, skip-arith, skip-native — tableaux séparés) + contre-fixture composée skip+carry gravée à code 1 (preuve C++ hôte — jamais un reçu device) |
| GPU série C — wire et kernels (hôte) | `mhgp6_wire` (aller-retour bit-exact du `GpuCloudIndexWire`, t1 contre balayage exhaustif, 3 refus hors-domaine, digest gravé) + mutants `gpu-index-drop-node`/`wire-t1-plus-one` ; `mhgp6_census_device_stub` (bit-identité boule à boule contre le scalaire sur candidats réels, 2 familles) + 4 mutants (`gpu-range-add-le`, `gpu-stack-shallow`, `gpu-swap-push-order` à multiset égal, `gpu-census-nonstrict`) ; `mhgp6_pilot_stub` (pipeline COMPLET : objet identique CPU vs route série C, refus du run entier sous mutants) ; `mhgp6_pilote_stub_*` (syntaxe/logique du pilote .cu, refus de parsing) ; JUGE DES RECORDS (§ 5.13-5.15, `tests/pilote_juge.py` — LE MÊME juge pour la porte stub, le runner G4 en mode fichier et le validateur) : `mhgp6_pilote_juge_contre_fixtures` (27 flux falsifiés intégrés tous tués : records/ABBA/signatures recalculées/formules d'octets 112-100-100/chronos fermés et enveloppants/stabilité inter-répétitions/identité d'en-tête famille-n-graine-fils-**arch**/lot_effectif=min/parité imprimée/grammaire hex64), `mhgp6_pilote_stub_juge` + `_ordre_inverse` (le pilote stub réel jugé sous les deux ordres de base) |
| GPU série C (device, `MHGP6_ENABLE_CUDA`, G4 seulement) | `mhgp6_device_witness` + 3 mutants (socle arithmétique PARTIEL) ; `mhgp6_census_device` + 4 mutants (jumelle device de la porte stub, arch compilée signée) ; `mhgp6_pilote_parite_400` + refus de parsing (pilote `mhgp6_cuda` : deux routes, parité de tous les digests, coûts wire/H2D/kernels/D2H). `[PRÉVU]` : contre-fixture composée du témoin portée sur device ; le reçu de GAIN = profil de campagne G4, jamais un gate |

## Fixtures permanentes aux coordonnées exactes

Corpus hérité : carré cocyclique (110/100/90...), `q2_one_interior_attachment`,
croissance unaire, cœur q4 discriminant, « dix témoins q2 qui ne ferment pas
q4 », fixtures q4 13/22 points, skinny 89°–89°–2°, témoin de forte
annulation, contre-familles `two_lines`/`collinear_seven`, F1–F11 des tests
d'ancre, sphère diamétrale à 37 sites. Corpus neuf : fixtures du sweep
(ci-dessus), `linked_arcs_u16`, fixture de masque de lane
`a=(1000,1000,1000), b=(2000,1000,1000), z=(1010,1016,1000)`,
calotte–lentille (V6-Q4), peigne de facteurs singletons. Fixtures du fold
(palier KeyCSR, `tests/fold_csr_gate.cpp`, dérivées à la main puis
re-dérivées machine, pins du bras classique) : F1 born-only, F2/F2b
parents-only (support inversé), F3 continuation (`batch_levels` de taille 2
pour un seul delta), F4/F4-min multi-parents S5 (ordre de `pre_list` ≠ ordre
des canons), F5 S2 (deux racines post d'un lot, ordre par racine UF ≠ ordre
par output), F6 forêt vide, S1/S1a/S1b inter-segments (output figé au lot,
niveau = représentation du premier événement du lot), R2/R2b encodage réel
K=2 ; refus amont (deux identifiants égaux) sous les deux routes : même
refus, kind csr signé, payload vide.

## Campagnes

Mesures d'échelle : compteurs déterministes (grand-livre), 5 familles
dilatées + 2 stationnaires × {8000, 16000, 32000} × graines {3,4,5} ; pentes
sécantes par terme ; reçus immuables dans `receipts/` (pin, hash de binaire,
sorties brutes). Temps : localement seulement en banc apparié contrebalancé ;
sinon G4 avec reçu.

Le validateur `bench/pentes.py` est prouvé fail-closed par
`tests/pentes_gate.py` (cinquième cycle : nominal + 20 falsifications à
code 3 et stdout vide — dont famille dupliquée du META, entier invalide sans
traceback, compteur dupliqué, digest dupliqué/non hexadécimal, fichier
d'extension inattendue, identités fermantes des octaves violées — + zéro
légitime sur un compteur réellement parsé avec `-` affiché). Le juge de
conformité refuse une référence à clefs de forêt HORS PROFIL (ensemble exact
`{1..kmax_eff}` exigé ; porte `mhgp6_juge_refus_k_en_trop`, K1 correct + K10
en trop à n=2) en plus du narrowing et de la référence tronquée. Le
protocole G4 v6 (`gcp-migration/session_campagne_v6_g4.sh` : conformité
v5≡v6 à 50 000, bench apparié ABBA sans digest, queue stationnaire) a son
selftest transactionnel à faux pilotes (`selftest_campagne_v6.sh`, à lancer
à la main avant toute session payante).

Reprise persistante (audit série C § 5.18.6, `selftest_cycle_vie_v6.sh`) :
le bootstrap matérialise `WORK` dans une base 0700 persistante sur le
volume du dépôt (`/workspaces/.ehgp-sessions`, jamais `/tmp`), le cycle de
vie s'exécute en session de processus propre (`setsid`) et publie avant
toute mutation `session.env`, `superviseur.pid` (pid + starttime +
boot_id) et `marques/` ; le garde `start_and_verify.sh` y publie deux
marques exclusives et distinctes — `guest_guard_pending` (génération
certifiée par la garde GCE) puis `double_guard_verified` (armement invité
relu). `recover_v6_session.sh` (épinglé, deux étages ré-authentifiés par
`git show`) ne démarre JAMAIS la VM : superviseur vivant → refus ;
registre `targeted_stopped` → rien ; sans seconde marque → arrêt immédiat
sur la génération exacte ; avec → scp bornée, un STOP, validateur épinglé,
classification FORCÉE `partiel_ou_invalide`, reçu `…_reprise_<epoch>`
jamais une décision ; génération inconnue → blocage 71 avec la commande à
lancer à la main. Scénarios : SIGKILL de toute la session après le
handshake puis reprise (R1), superviseur vivant refusé (R2), tué entre les
deux marques (R3), mutants génération discordante / copie épinglée altérée
/ cible discordante / pid recyclé (R4), scp en échec (R5), base 755 refusée.
Durcissements du contre-audit (`CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902`) :
exclusion par verrou noyau (`flock -n` tenu jusqu'à la sortie, pid/starttime
en diagnostic) ; vivacité par pid + starttime + boot_id ET par sid/pgid
gravés dans `superviseur.pid` (cinq champs) ; registre strict (un état qui
implique une cible démarrée porte une génération non vide) ; marques
parsées comme objets stricts (fichier régulier, keyset exact, `mark=<nom>`,
génération non vide) ; describe en tuple exact (RUNNING, génération) avant
la scp, scp vers un staging puis relecture du tuple avant promotion, toute
autre génération → 71 sans STOP ; `REMOTE_DIR` lié à (commit, époque de la
génération) ; entrée en `targeted_stop_failed`/`targeted_stopping` →
STOP-FIRST (ni describe, ni scp, ni validateur avant l'arrêt certifié),
arrêt non certifié → témoin MINIMAL ; purge des credentials VÉRIFIÉE avant
le témoin `recu_publie` (reprise et cycle nominal), échec → rc 67 et
re-purge locale sans appel GCP ; la reprise n'exécute QUE la garde d'arrêt
épinglée ré-authentifiée (le harnais exerce la VRAIE `stop_and_verify.sh`
sous un faux `gcloud`) ; politique des rejeux explicite (manuels, un STOP
chacun, jamais de boucle). Dents : deux reprises simultanées (D1),
`targeted_stopped` sans génération (D2), génération concurrente avant /
pendant la scp (D3), marque au champ `mark=` falsifié (D4), orphelin de
session sans `WORK` dans son argv (D5), purge en échec puis re-purge locale
(D6, D8 nominal), stop-first et troisième rejeu (D7). Coutures du § 5.21 :
tous les choix terminaux (session déjà conclue comprise) sont pris SOUS le
verrou ; le staging n'est promu en `out/` que si la scp a réussi et si le
tuple postérieur est lisible, exact et de la génération attendue, sinon le
partiel est conservé sous `out.partiel_<epoch>` et seul l'arrêt ciblé se
poursuit ; l'échec de la publication du témoin domine le succès (code 68,
marqueur `temoin_non_publie`, dans la reprise et le cycle nominal) ; le
témoin minimal après arrêt non certifié ne porte que des champs fixes et des
tails plafonnés à 64 Kio (ni `out/`, ni `marques/`, ni `sync`) ; les rejeux
sont bornés par tentative, jamais par un ledger ; la vivacité couvre
l'identité sid/pgid enregistrée, pas un descendant qui refait `setsid`. Dents :
témoin non publiable (D9 reprise, D11 nominal) puis publication sans appel
GCP, describe indisponible après la scp (D10 : partiel conservé, rien promu).
Le revalidateur recompare aussi l'inventaire des répertoires (mutant :
répertoire vide créé par le validateur). Dents du § 5.22 : funnel d'arrêt
inconditionnel (dès que la génération est connue, une erreur locale pré-STOP
déclenche exactement un arrêt ciblé, rc 74, témoin minimal — D12, faux `tee`
en échec après la scp) ; l'arrêt précède toute promotion ou sauvegarde
locale ; provenance de `out/` par marqueur atomique `out.promotion`
(génération, commit, `scp_rc`), seul un `out/` promu par CE rapatriement est
validé (D13 : `out/` résiduel + scp en échec ⇒ aucun validateur) ; purge
nominale incomplète ⇒ code 67 (priorité 67 > 68 > 0/65, D8 l'exige) ; reçu
minimal aux champs bornés (marques connues seulement, résidus non énumérés) ;
D11 causal (rendez-vous fatal, exactement un STOP, fast-path et
`issue=arret_certifie_par_le_garde`) ; les manifestes de reçus n'excluent que
le `SHA256SUMS` racine (un `out/SHA256SUMS` est inventorié : leurre du faux
scp assert en R1). Revalidateur : validateur canonique authentifié (sha256
gravé ; autre chemin seulement sous `EHGP_REVALIDATE_SELFTEST=1`), résumés
attendus exigés après l'appel (juge muet ⇒ rc 3), inventaire NUL injectif
(type, mode, nom ; nom à saut de ligne refusé), « intact » = noms, types,
modes et octets — `selftest_revalidate_v6.sh` (22 scènes : un résumé
re-produit différent d'un octet ⇒ rc 3 ; répertoire racine « out marques »
refusé par l'allowlist NUL en Python). Retour des auditeurs sur le WIP § 5.22 :
funnel armé dès la génération résolue (sous `errtrace`, hérité par les
fonctions), garde d'arrêt exécutée D'ABORD et tout en best effort autour
(registre, journal), 70 domine 74 ; marqueur `out.promotion` lié à un
identifiant de TENTATIVE (D13 précharge un marqueur valide d'une tentative
antérieure : aucun validateur) ; dents D14 (panne de journal dès la génération
connue ⇒ un STOP, rc 74) et D15 (panne de `publish_state` ⇒ garde exécutée
exactement une fois, code non nul, aucun témoin).
