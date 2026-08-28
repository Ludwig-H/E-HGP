# Audits de MorseHGP3D v5

Ce dossier est le canal de travail entre l'implémenteur et l'auditeur. Il reste
volontairement court : un état courant mutable, les questions encore utiles et
leurs réponses. Les incidents fermés et les mesures munies d'un reçu restent
consultables dans l'historique Git ou dans `../receipts/` ; ils ne sont pas
recopiés au tip.

## Entrées actives

- [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict courant et ordre de fermeture.
- [`AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md`](AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md) : chemin de résolution GPU/multi-CPU — pool persistant, géométrie résidente, compaction device et protocole cpuset.
- [`AUDIT_PASSAGE_ECHELLE_20260828.md`](AUDIT_PASSAGE_ECHELLE_20260828.md) : fold vivant small-to-large, lifetime exact, wire u64, amont externe et reprise par K.
- [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) : question V15 et six raccords encore utiles après réception bornée du noyau ; le fold renvoie désormais à l'état courant.
- [`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md) : arbitrages V17–V30 pour passer des G0/G1 déjà implémentés à une réception device, puis à G2/L7 selon l'ablation.
- [`QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`](QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md) : échange V36–V42 et réponse auditée ; sépare diagnostic de pente, budget 10 M, coupure aveugle, raffinement certifié et porte littérale bornée.
- [`QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`](QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md) : pont historique condensé, conservé tant que le document mathématique canonique le référence.
- [`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md) : reçu de décision sur la future tour.
- [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md) : reçu des arbitrages V1–V4 désormais intégrés aux documents canoniques.

La question V15 reste au tip tant que ses corrections documentaires ne sont pas
requalifiées. Les réponses générales de fermeture de Claude ont été
requalifiées jusqu'au pin `556c421e` et retirées du tip ; leur contenu reste
dans l'historique Git. La réponse V17–V30 a été consolidée dans la question L7
conservée après réception bornée de la session 13 dans
`../receipts/campagne_g4_v5_20260828_instrument_scale/`. La question V7–V14 a
été condensée de la même manière.

## Convention de fraîcheur

`ETAT_COURANT.md` nomme le commit fonctionnel effectivement jugé et distingue
les constats sur ce pin des observations sur un worktree sale. Le commit qui
publie l'état peut naturellement être postérieur au pin jugé ; tout commit
fonctionnel ultérieur rend le verdict périmé jusqu'à relecture. Un audit ne
change jamais `public_status=not_claimed` et ne remplace ni un oracle ni un
reçu reproductible.

Claude peut écrire ici une question ou une réponse. L'auditeur répond de façon
actionnable, requalifie les corrections sur un pin propre et retire les échanges
devenus inutiles après migration de leurs décisions vers `docs/` et de leurs
preuves vers `receipts/`.

Les réponses auxquelles les documents canoniques renvoient restent de courts
reçus de décision. Les longues notes d'incident, réponses de fermeture et
mesures déjà reçues sont retirées du tip dès que `ETAT_COURANT.md` les a
requalifiées ; leur historique Git demeure disponible.

Les Markdown suivent les règles KaTeX du dépôt. `python tools/check_docs.py`
contrôle les documents produit, reçus et écrits de Claude de la v5, mais exclut
délibérément les mots de l'auditeur — dont `README.md`, `ETAT_COURANT.md` et
`REPONSE_A_CLAUDE_*`. Ceux-ci doivent être passés explicitement à la fonction
`validate()` du vérificateur ; aucun vert documentaire ne prouve la fraîcheur
sémantique ni la conformité du code.
