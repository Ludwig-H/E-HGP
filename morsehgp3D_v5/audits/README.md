# morsehgp3D_v5/audits — le dossier de l'auditeur

Ce dossier est le **seul** endroit où l'auditeur intervient. Il reprend le
cycle documentaire des v3/v4 sans son inflation : moins de fichiers, datés,
ancrés au hash court du commit jugé, et un état courant unique.

## Rôles

- **L'auditeur** écrit ici : audits (`AUDIT_*`, `CONTRE_AUDIT_*`), arbitrages,
  réponses (`REPONSE_*`), corrections. Il motive des corrections ; il ne
  certifie rien — aucun audit ne promeut un statut public. Il pousse sur
  `main`.
- **Claude** (l'implémenteur) écrit ici : questions (`QUESTION_CLAUDE_*`),
  notes (`NOTE_CLAUDE_*`), réponses aux audits (`REPONSE_CLAUDE_*`),
  spécifications de solution (`NOTE_SOLUTION_*`). Chaque livraison a son reçu
  **immuable** dans `../receipts/<chantier>_<date>/`, ancré au commit.
- Tout fichier est daté (`_YYYYMMDD`) et, quand il juge du code, ancré au hash
  court.

## Conventions

- `ETAT_COURANT.md` : le verdict mutable unique, ancré au `HEAD` — pas un
  fichier par jour. Son pin doit être frais par rapport au `HEAD` pour qu'un
  claim, même borné, soit lisible ; un pin antérieur interdit tout claim.
- `../docs/PISTES_FERMEES.md` : mémo append-only des tentatives fermées
  (idée, cause d'abandon, ce qui survit). Une piste fermée ne se rouvre
  qu'avec un nouveau théorème + fixture, jamais sur un benchmark.
- Toute contradiction mathématique devient une **fixture minimale
  permanente** dans `../tests/` avant la poursuite du travail.
- Un audit se **lit et s'exécute** (commandes reproduites, codes de sortie
  comparés) avant toute réponse et avant toute dépense.
- Équations : une seule ligne physique, accolades explicites, pas de
  `\operatorname`, `\left\Vert` / `\left\lbrace` (`python tools/check_docs.py`).

## Questions ouvertes au démarrage

Les questions posées à l'auditeur vivent dans `QUESTION_CLAUDE_*` ; les
questions mathématiques héritées de la v4 (Q1–Q4, non tranchées) sont reprises
avec leur statut dans `../docs/MATHEMATIQUES.md`.
