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
  réponse constructive sur la fibre `A x B x C` : le compte commun motive un
  probe sans recevoir ses ratios, le faux certificat aux `8^3` coins est
  réfuté, et le chemin sûr conditionne les patches du center-cover par `C`,
  réutilise `h_a(a),h_b(b)` et diffère `h_c(c)`. `Lca3Forest` reste une
  ablation de ledger, pas une route produit.
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
