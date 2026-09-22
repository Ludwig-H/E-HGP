# État courant des audits v9

22 septembre 2026, ouverture. Aucun audit indépendant de la v9 n'a encore été
rendu : il n'existe pas encore de code v9 à auditer.

Ce dossier appartient aux **auditeurs indépendants** de la v9. Le développeur
n'y écrit pas, sauf sous la forme de fichiers `REPONSE_CLAUDE_*`,
`NOTE_CLAUDE_*` ou `QUESTION_CLAUDE_*` datés. Chaque auditeur tient son propre
fichier de dialogue et consigne ses constats dans des fichiers datés
`_YYYYMMDD`, ancrés au hash court du code jugé, avec leurs sources et captures
**dans le dépôt** (jamais dans une archive jointe à une conversation). Ce
fichier `ETAT_COURANT.md` porte le verdict mutable unique, ancré au `HEAD`
audité. Le canal commun est
[`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md).

## Premiers objets d'audit

Demande du développeur sortant, en deux lots :
[QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md](QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md).

1. L'audit d'ouverture lui-même : [synthèse](../docs/AUDIT_V8_SYNTHESE.md),
   [plan](../docs/PLAN_V9.md), [héritage](../docs/HERITAGE_V7_V8.md). Il est
   écrit par le développeur sortant et ne vaut pas audit indépendant.
2. Les six changements moteur v8 des 21–22 septembre, qu'aucun auditeur
   indépendant n'a relus : `748ec082` (atlas en i64 à Q = 2^20), `02987f18`
   (fragments d'atlas), `0948d2d0` (rejet des graines q3 par l'atlas),
   `5224ff4e` (file de plages), `5fdda963` (chronos par worker), `a74e90f2`
   (moteur 18 bits, bornes réécrites avec M = 262 143). À contre-prouver avant
   tout port en v9.
3. Chaque port v9 : épinglage, requalification, fixtures d'égalité, mutants.
4. La première tour v9 de bout en bout : objet (contre le juge T2 et les
   fixtures E5, A–E, quatre points), sorties, chronomètre.

Verdict public : `not_claimed`. Aucun contrat acquis.
