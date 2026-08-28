# Audits de MorseHGP3D v5

Ce dossier est le canal de travail entre l'implémenteur et l'auditeur. Il reste
volontairement court : un état courant mutable, les questions encore utiles et
leurs réponses. Les incidents fermés et les mesures munies d'un reçu restent
consultables dans l'historique Git ou dans `../receipts/` ; ils ne sont pas
recopiés au tip.

## Entrées actives

- [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict courant et ordre de fermeture.
- [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md) : question V15 et réponse actionnable sur la grille, le fold concurrent et la mémoire.
- [`QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`](QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md) : pont historique condensé, conservé tant que le document mathématique canonique le référence.
- [`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md) : reçu de décision sur la future tour.
- [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md) : reçu des arbitrages V1–V4 désormais intégrés aux documents canoniques.

La question V15 reste au tip tant que ses corrections prioritaires ne sont pas
requalifiées. La question V7–V14 a été condensée et la note de livraison
périmée retirée ; leur contenu détaillé reste dans l'historique Git.

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
