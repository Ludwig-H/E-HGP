# Index court des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier n'est plus un journal chronologique. Les propositions abandonnées,
questions résolues et audits absorbés ont été supprimés. Les preuves durables
sont consolidées dans `../PROPOSITION.md`; seules restent les autorités live et
les dépendances historiques encore citées par le logiciel ou un reçu.

## Autorités actives

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : unique verdict mutable,
  pin, worktree, tests et blocages.
- [`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md) :
  contre-audit de la tranche `0A`; overflows u16, indépendance du juge, ABI,
  `RelevantGP`, caps et fixtures.
- [`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md) :
  réponse mathématique consolidée, contre-audit de l'autre auditeur,
  `SOC64/CORNER512`, LP projectif et cages quatre--six sites.
- [`AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md`](AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md) :
  autre audit ayant découvert l'owner `GenerationRank`; conservé pour la chaîne
  de preuve, avec ses corrections dans l'audit précédent.
- [`NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md`](NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md) :
  note active de Claude et questions de route. Ses claims live sont toujours
  subordonnés à `AUDIT_ETAT_COURANT.md`.

## Dépendances historiques gardées

Ces fichiers ne sont pas des verdicts live. Ils restent parce que CMake, un
prototype ou un reçu les cite encore. Le code ne doit pas prendre leur titre ou
leur ancien statut pour une réception actuelle.

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
- `check_centre_cell_thread_invariance.py`.

Ils sont appelés par CTest et ne sont pas des documents historiques.

## Règle de fraîcheur

Un fichier daté ne devient jamais live. Avant toute conclusion :

1. lire `AUDIT_ETAT_COURANT.md` ;
2. comparer son `HEAD` et son worktree au dépôt ;
3. distinguer sujet, oracle, mutant, provenance et payload ;
4. conserver toute contradiction mathématique comme fixture ;
5. mettre à jour la proposition consolidée, puis supprimer la note absorbée.
