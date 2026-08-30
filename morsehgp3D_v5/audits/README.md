# Audits de MorseHGP3D v5

Ce dossier est le canal de travail compact entre Claude et les auditeurs.

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Décision active : le profil produit doit imposer `s>=8` sur toute la voie, q2
comprise. Les séparations plus petites restent au plus des diagnostics de
primitives ou des contre-fixtures de génération explicitement test-only ; elles
ne sont pas des échelles candidates et ne peuvent publier aucun payload.

## À lire maintenant

- [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict mutable, pin source jugé,
  blocages et ordre de fermeture. Il prime sur les réponses historiques.
- [`REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md`](REPONSE_A_CLAUDE_BLOCS_ABC_20260829.md) :
  échange consolidé. Sa fin contient la contre-relecture V151–V157 et le
  verdict sur la sémantique Gamma/Gabriel.
- [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) :
  raccords encore ouverts pour la grille de cellules.
- [`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md) :
  décisions encore ouvertes pour la lane device.

## Reçus de décision conservés

- [`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md)
- [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md)

Les questions V146–V157 déjà traitées sont consolidées puis supprimées ; Git
en conserve l'historique. Les sorties brutes restent dans `receipts/`, jamais
recopiées ici.

## Fraîcheur et validation

`ETAT_COURANT.md` nomme séparément le dernier commit Claude et le dernier pin
fonctionnel effectivement jugé. Un commit fonctionnel ultérieur périme le
verdict, mais un commit documentaire de question ne transforme pas le code.

Avant publication, passer chaque Markdown du dossier à
`tools.check_docs.validate`, puis exécuter `python tools/check_docs.py` et
`git diff --check`. Un vert documentaire ne prouve ni exactitude, ni
performance, ni fraîcheur sémantique.

Un audit doit fermer une couture ou répondre à une question. Il ne promeut
jamais le registre et ne remplace ni oracle indépendant, ni reçu reproductible.
