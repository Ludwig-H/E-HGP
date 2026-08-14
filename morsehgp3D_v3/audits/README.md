# Index court des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier n'est plus un journal chronologique. Les propositions abandonnées,
questions résolues et audits absorbés ont été supprimés. Les preuves durables
sont consolidées dans `../PROPOSITION.md`; seules restent les autorités live et
les dépendances historiques encore citées directement ou transitivement par le
logiciel, un reçu ou une autorité conservée.

## Autorités actives

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : unique verdict mutable,
  pin, worktree, tests et blocages.
- [`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md) :
  autorité courante sur le verrou M4 ; contre-fixture de l'intervalle nul du
  sampler v2, limites du brute-force/H4, `Depth=tau(E)`,
  `BlockJungDual64`, noyau d'axe top-k, `Corner8BallDepth`, count M4 factorisé
  par Möbius et fallback shallow edge-local.
- [`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md) :
  partition factorisée Callahan--Kosaraju de toutes les paires, source de tous
  les q2 propres après `D>0`, extensions exact-once `OwnedCK-WST3/WST4`, disque
  de Jung paire-level et fixture de non-cascade. Un lift Jung vers tout un
  rectangle exige encore un théorème uniforme.
- [`AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md) :
  première réponse aux six questions de Claude ; séparation
  `E4/F3/C4_carrier/F4/M4_apex/W4/H4/T4_site`, preuve sharp de `2B_R` et ordre
  blockwise. Son état des samplers et ses premières fixtures Jung/BlockBall
  sont supersédés par le contre-audit v2 ci-dessus.
- [`AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md) :
  contre-audit du shadow SOC64, obstruction u16 aux routes universelles, lemme
  du porteur aigu, sweep q4 1D, pelages inversés et portes physiques.
- [`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md) :
  contre-audit de la tranche `0A`; overflows u16, indépendance du juge, ABI,
  `RelevantGP`, caps et fixtures.
- [`AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md) :
  le probe Kruskal/Floyd reçoit une fermeture de points bornée, pas les dix
  folds HGP ; filtre unsupported, comparateur de niveau et fixtures bloquantes.
- [`AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md) :
  gain `E4`, coût du raffinement, ledgers tentative/terminal, héritage de
  preuves et défauts fail-closed de la recette G4.
- [`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md) :
  réponses aux trois questions de Claude, portée du NO-GO amas et cascade
  `SOC64 -> LP -> cages` avant davantage de raffinement.
- [`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md) :
  finalité des fenêtres, pentes terrain sur surensemble, coût non comparable,
  portée du SLO et réparations de la recette réutilisable.
- [`AUDIT_CONTRE_RECEPTION_BALL_EVENT_2B89EA1_20260813.md`](AUDIT_CONTRE_RECEPTION_BALL_EVENT_2B89EA1_20260813.md) :
  rejeu indépendant de 0A ; RLE avant census, Gram au-delà d'i128, vrais
  `PointId`, statut transactionnel et générateur pouvant ne pas terminer.
- [`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md) :
  preuves historiques `SOC64/CORNER512`, LP projectif et cages quatre--six
  sites. Son ordre d'exécution est supersédé par `AUDIT_ETAT_COURANT.md` et
  `PROPOSITION.md`.
- [`AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md`](AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md) :
  autre audit ayant découvert l'owner `GenerationRank`; conservé pour la chaîne
  de preuve, avec ses corrections dans l'audit précédent.
- [`NOTE_CLAUDE_SOC64_LEDGER_COMBINE_ET_PARADOXES_20260814.md`](NOTE_CLAUDE_SOC64_LEDGER_COMBINE_ET_PARADOXES_20260814.md) :
  réponse de Claude aux deux contre-audits du 14 août. Acceptation du
  double-comptage, replay virtuel combiné, chiffres corrigés, mesure de `f(n)`
  sur trois tailles, puis sampler de porteurs et cinq questions. Ses claims
  `M4 cubique` sont requalifiés par l'audit précédent.
- [`NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md`](NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md) :
  questions de route de Claude, conservées avec bannière de rétractation. Ses
  claims live sont subordonnés à `AUDIT_ETAT_COURANT.md`.
- [`NOTE_CLAUDE_CRITERE_DE_MORT_ETAPE_1_20260813.md`](NOTE_CLAUDE_CRITERE_DE_MORT_ETAPE_1_20260813.md) :
  mesure et questions de Claude, avec bannière vers les contre-audits
  `35fcea8`; ses généralisations initiales sont rétractées.

## Dépendances historiques gardées

Ces fichiers ne sont pas des verdicts live. Ils restent parce que CMake, un
prototype, un reçu ou une autorité les cite directement ou transitivement. Le
code ne doit pas prendre leur titre ou leur ancien statut pour une réception
actuelle.

- géométrie/cellules :
  `NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`,
  `AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`,
  `AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md`,
  `AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md`,
  `AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`,
  `AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md` ;
- source et enveloppes :
  `NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md`,
  `AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`,
  `AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`,
  `AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md` ;
- route ancienne encore citée :
  `NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`,
  `AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`,
  `AUDIT_CONE_CIBLE_LIVE_ROUTE_50K_20260813.md` ;
- composants historiques :
  `AUDIT_SOURCE_DIRECTE_24AD3D37.md`,
  `AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`,
  `AUDIT_ORDER_K_FLATS_9C587E6.md`,
  `AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`,
  `NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md`,
  `NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md`,
  `NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md`,
  `NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`,
  `NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`,
  `NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`,
  `NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md`.

## Scripts gardés

- `check_gate_d_fold_f0.py` ;
- `check_centre_cell_independent_judge.py` ;
- `check_centre_cell_thread_invariance.py` ;
- `check_rampe_pentes.py`.

Ils sont appelés par CTest et ne sont pas des documents historiques.

## Règle de fraîcheur

Un fichier daté ne devient jamais live. Avant toute conclusion :

1. lire `AUDIT_ETAT_COURANT.md` ;
2. comparer son `HEAD` et son worktree au dépôt ;
3. distinguer sujet, oracle, mutant, provenance et payload ;
4. conserver toute contradiction mathématique comme fixture ;
5. mettre à jour la proposition consolidée, puis supprimer la note absorbée.
