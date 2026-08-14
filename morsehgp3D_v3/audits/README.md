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

Règle de supersession globale pour les textes historiques : seule une paire
endpoint de distance `D=0` est filtrée. Une géométrie peut bucketiser une
position dupliquée, mais tous les vrais `PointId`, leur multiplicité et leurs
paires vers les autres positions restent dans les pools et produits. Toute
ancienne phrase imposant le rejet global des positions dupliquées est périmée.

## Autorités actives

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : unique verdict mutable,
  pin, worktree, tests et blocages.
- [`NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md) :
  autorité contractuelle mathématique, sans réception logicielle : réponse
  négative à Q14, théorème exact des fuseaux `W2/W3/W4` et contrat de
  trois producteurs autonomes. `Lane2`, `Lane3` et `Lane4` peuvent mutualiser
  un index Morton et une partition neutre, mais jamais leurs records, verdicts,
  caps, continuations ou preuves de complétude. Le `Q4Seed3` est un préfixe
  interne à `Lane4`, pas un support ou une sortie q3.
- [`AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md`](AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md) :
  contre-audit consolidé au `3507b5e` du noyau ponctuel interne à `Lane4` :
  `r4` paramétrique, IDs `I_B/U_B`, `MORT_GAP` et exact-once désormais exercés ;
  P0 restant sur une fixture de 97 IDs cosphériques, où le shell attendu de 100
  IDs est tronqué à 99 sans fate ; distinction obligatoire entre borne de
  groupes et masse d'IDs, dégénérescence fail-closed et portée encore
  quadratique par seed.
- [`AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md`](AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md) :
  contre-audit de l'ancienne recette G4 CPU et de son successeur au `d55bb9a` :
  fermeture générationnelle et zone IA réparées, mais parser du verdict
  incompatible avec les nouveaux records, exact-once rendant le cardinal de
  lignes impossible, réfutations copiées après verdict, absence de deadline
  global et P0 `U_B` préalable. GCP non utilisé par l'audit.
- [`NOTE_CLAUDE_Q4SEED_AXIS_TOPR4_ET_BLOCAGE_G4_20260814.md`](NOTE_CLAUDE_Q4SEED_AXIS_TOPR4_ET_BLOCAGE_G4_20260814.md) :
  journal de tranche de Claude, non autorité de réception : indépendance des
  lanes mesurée, 36 portes locales, correction du diagnostic de zone GCP et
  liste de blocages déclarés. Son claim de census complet est supersédé par la
  fixture 100-vers-99 du contre-audit, et le runner G4 reste interdit.
- [`AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md`](AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md) :
  contrôle algébrique Python corrélé et borné des identités q4 `Delta=O^2` et
  `Phi=O*J` sur 10 000 fixtures u16 non dégénérées ; il ne reçoit ni microkernel
  C++, ni enclosure de boîtes,
  ni positivité, ni source WST, ni census, ni ABI, ni coût.
- [`AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md) :
  théorème exact de la miniboule au support minimal positif complet, q3 ambiant,
  supersource exhaustive par Carathéodory, puis nécessité du census, du test
  `c in relint(conv(U_B))` et de la disposition exacts sur le shell complet,
  réception conditionnelle de Corner8, réfutation de l'exact-once physique des
  probes WST3/WST4 et réponses Q6--Q9. Le statut reçu est `CandidateCover`
  avant distinct-ID/owner/positivité ; la profondeur doit précéder le produit
  WST4 brut. Son post-scriptum conserve la réfutation du signe à `3703097`, sa
  réparation à `a73161c`, le diagnostic Corner8 de `069d903` et les réponses
  Q10--Q13 sur orientation, bisigne, raffinement par lane et diagonale `Sym2`.
  Le verdict live au pin logiciel `6e815d2` est maintenu dans
  `AUDIT_ETAT_COURANT.md` :
  le bisigne est commis, les couples non séparés ne sont plus jetés, mais
  `Sym2` reste un compteur, le caller bisigne perd le mauvais bit et Corner8
  demeure post-produit. La généralisation exacte d'ordre `q` par vecteurs de
  multiplicités `alpha` est consolidée dans ce contre-audit et la proposition.
  Le sampler `--masse` ajouté ensuite n'est ni uniforme dans les quadruplets, ni
  filtré par positivité ; sa CTest à regex reste un diagnostic réfuté.
  Au commit historique `f1b78c0`, `--supports-retenus` énumérait seulement
  `orientation!=0 && I<=7`. `34cf05d` lui ajoute la positivité q4 ponctuelle par
  Cramer et `08dec609` corrige sa porte : les `18/18` tests Corner8/WST ciblés
  passent, mais restent à regex. La population courante est
  `Positive4 intersect {I<=7}` des
  `SupportKey`, toujours sans shell, `BallKey/RLE`, positions dupliquées, ABI
  typée ni oracle C++ indépendant. Sa métrique est arête/espacement et son cap
  en points n'est pas un budget. Le détail, les hashes, les fixtures manquantes
  et les commentaires CMake périmés sont maintenus dans
  `AUDIT_ETAT_COURANT.md`.
- [`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md) :
  lemme exact de miniboule unique au support complet, famille normale au support
  partiel, cœur affine, sandwich `U<=D<=C`, q3 par pied, q4 par intersections,
  contre-fixtures u16 de non-hérédité, contre-audit des samplers et réception de
  `MidballBlockDepth` au pin `8fd6f59`, puis du raccord/juge WSPD au pin
  `a58d020` : `ALL` continu sûr, `NONE` limité au réseau u16, portes à regex
  fail-open et autorité à consolider avec `rect_h_interval`. Il contre-audite
  aussi la première révision `HCBlockDepth` : facteurs q3/q4 sûrs, enclosure
  conservatrice et autorité exacte `Corner512` déjà existante. Son état
  logiciel est supersédé par le pin `c1e2e3b` et l'audit courant.
- [`AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md`](AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md) :
  contre-audit de `--borne-sup` après `a58d020` ; la révision `90640885`
  oubliait les enfants `MIXED` et supprimait toutes les fermetures. La
  réparation désormais commise `ec5ec3d4` restaure la parité singleton sur l'ablation
  rejouée, mais perd encore des fermetures avec la vue combinée et BJD. La
  source `--climb` omet indépendamment une feuille malgré un statut final.
- [`AUDIT_LIVE_HC_BLOCK_DEPTH_A58D020_20260814.md`](AUDIT_LIVE_HC_BLOCK_DEPTH_A58D020_20260814.md) :
  preuve du certificat `(H,C)` q2/q3/q4 et audit historique du delta `--hc`
  après `a58d020`. Le pin `c1e2e3b` a réparé la CTest et ajouté cinq portes ;
  le triple calcul, le juge de promotions, les compositions et le coût restent
  ouverts dans l'audit courant.
- [`AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md`](AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md) :
  contre-audit historique du raccord `BlockJungDual64` au parent `783a789` ;
  réfutation du double crédit d'identité, packing réparé mais no-go dans
  l'ordonnance actuelle, fixture des sept témoins collinéaires, remplacement
  par `tau(F)>=8`, génération par coupes, préfiltre bilinéaire exact et état de
  la dernière session G4. Sa bannière renvoie au verdict courant.
- [`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md) :
  autorité courante sur le verrou M4 ; contre-fixture de l'intervalle nul du
  sampler v2, limites du brute-force/H4, `Depth=tau(E)`,
  `BlockJungDual64`, noyau d'axe top-k, `Corner8BallDepth`, count M4 factorisé
  par Möbius et fallback shallow edge-local.
- [`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md) :
  partition factorisée Callahan--Kosaraju de toutes les paires, source de tous
  les q2 propres après `D>0`, extensions exact-once `OwnedCK-WST3/WST4`, disque
  de Jung paire-level et fixture de non-cascade. Sa réalisation live est
  requalifiée en `CandidateCover` par le contre-audit `22d1cb0` ci-dessus ; un
  lift Jung vers tout un rectangle exige encore un théorème uniforme.
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
  `M4 cubique` et « `BlockJungDualTile` implémenté » sont requalifiés par les
  bannières de supersession et les contre-audits actifs.
- [`NOTE_CLAUDE_SOURCE_WST3_WST4_ET_SUPPORT_COMPLET_20260814.md`](NOTE_CLAUDE_SOURCE_WST3_WST4_ET_SUPPORT_COMPLET_20260814.md) :
  réponse de Claude et mesures WST3/WST4 jusqu'au pin `3703097`. La masse WST4
  brute rouge est un diagnostic utile ; ses claims exact-once, constante WST3,
  borne composée et filtre d'acuité au signe inversé sont rétractés par sa
  bannière et le contre-audit support-complet actif.
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
