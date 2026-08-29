# Audits de MorseHGP3D v5

Ce dossier est le canal de travail entre Claude et les auditeurs. Il reste
court par construction : un verdict courant mutable, les seules questions
encore actionnables et deux reçus de décision auxquels les documents
canoniques renvoient.

Cadre de toute entrée active : `phase=exploration_v5_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Entrées actives

- [`ETAT_COURANT.md`](ETAT_COURANT.md) : seul verdict mutable, pin jugé,
  réserves et ordre de fermeture.
- [`QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`](QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md) :
  contre-audit de l'exposant q3/q4 ; il rejette la double WSPD globale et
  place le center-cover entier de blocs avant les ancres, avec terminal par
  lignes ; il répond aussi au reçu de rescans sans confondre le routeur de
  handles avec `m_e`. La WSPD locale d'arête opposée reste une ablation q4
  conditionnelle.
- [`REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`](REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md) :
  réponse constructive sur la fibre `A x B x C` : le prédicat idéal est reçu,
  mais la v3 reste diagnostique tant que ledger, caps et compteurs causaux ne
  sont pas indépendants ; le chemin sûr calcule `g_AB[j]` une fois par
  rectangle, laisse `C` masquer les patches par les médiatrices, réutilise
  `h_a(a),h_b(b)` et diffère `h_c(c)`. Pour q4, les deux
  porteurs restants restent une paire non ordonnée dont le ledger ferme
  `6*C(n_u,4)`. La note donne aussi la relève concrète de
  `corner_histograms` par requêtes d'arbre saturées, puis bitsets des seuls
  couples survivants. Les fates de boîtes restent en shadow sur les seuls
  porteurs de supports ; l'acuité emploie la forme couplée `hmin_boxes>=0`, et
  la lentille requalifie d'abord les extrema corrélés `OwnerD2Exact` de la v4,
  sans en importer le code ; le terme `|w.d|`, dominé après le cover, reste une
  identité d'oracle/mutant. Aucun fate ne retire les mêmes handles du census.
  Le pavage P1 à 64 patches existait déjà en v4 avec des pentes rouges ; le
  delta à requalifier est sa factorisation par rectangle WSPD, `t_C/P[t_C]`,
  le ledger q3 pondéré par seuil et le passage q4 seuil--axial, jamais le
  center-cover pris isolément.
  Le seuil `t_C` condense les patches et réutilise les bitsets `B_lt[t]` sans
  `A x B x C`; les histogrammes d'intersection ferment exactement la masse q3
  même pour `P[t]>0`, en temps linéaire en handles après les facteurs. En q4,
  neuf classes `s_H` retirent le produit `C x D`, puis les faces ternaires
  résiduelles passent au terminal axial ; `t_CD` reste un oracle borné. Cover
  brut, partition de complétions, capacité de seed, source de certificats et
  census exact restent séparés : une sous-source sonore suffit à un minorant,
  tandis que le ranking/census q4 exige l'arbre entier ou une source complète
  prouvée. Les `h_a/h_b` historiques sont des minorants facteurs après split,
  pas les cardinalités exactes de la fibre enfant. Témoins de position, masque
  de patches calculés et prédicat seed q4 symétrique sont explicites. La
  réponse V73--V81 reçoit le retrait de `EMPTY` comme priorité
  produit, réfute le faux plafond 95--99 % censuré au premier shallow et la
  conclusion « patches nécessaires ». Un fast path global commun reste un
  premier shadow légitime, dont l'échec vaut `UNKNOWN`. La note fixe aussi le
  contrefactuel multicomptes du center-cover et sépare
  existence, profondeur et action en trois axes avec non-vacuité explicite.
  L'universel autorise un prune sans promouvoir `ALL_DEEP` tant que l'existence
  reste inconnue. Le shadow visite tous les blocs ; une sélection préalable des
  non vides serait circulaire.
  `Lca3Forest` reste une ablation de ledger, pas une route produit.
- [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) : six raccords encore ouverts pour la grille de cellules.
- [`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md) : décisions et dents restantes de la lane device.
- [`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md) : reçu de décision sur la future tour.
- [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md) : reçu des arbitrages V1–V4.

Une question reste au tip seulement tant que sa décision ou sa preuve n'a pas
été migrée vers `docs/`, `tests/` ou `receipts/`. Elle est ensuite supprimée ;
l'historique Git conserve l'échange. Les sorties brutes et les mesures reçues
ne sont jamais recopiées ici.

## Fraîcheur et validation

`ETAT_COURANT.md` nomme le commit fonctionnel effectivement jugé et distingue
ce pin d'un éventuel worktree concurrent. Tout commit fonctionnel ultérieur
rend le verdict périmé jusqu'à relecture. Le commit qui publie l'audit est
naturellement postérieur au pin qu'il juge.

`python tools/check_docs.py` exclut volontairement une partie des écrits
d'audit. Avant publication, passer chaque Markdown de ce dossier à
`tools.check_docs.validate`, puis exécuter `git diff --check`. Un vert
documentaire ne prouve ni la fraîcheur sémantique, ni l'exactitude du code.

Un audit aide à fermer une couture ; il ne promeut jamais le registre et ne
remplace ni un oracle indépendant, ni un reçu reproductible.
