# morsehgp3D_v4/audits — le dossier de l'auditeur mathématique

Ce dossier est le **seul** endroit où l'auditeur mathématique intervient. Il
reprend le cycle documentaire qui a fait ses preuves en v3, sans son
inflation : moins de fichiers, ancrés, datés, et un état courant unique.

## Rôles

- **L'auditeur** écrit ici : audits (`AUDIT_*`), arbitrages, réponses
  (`REPONSE_*`), corrections. Il motive des corrections ; il ne certifie
  rien — aucun audit ne promeut un statut public.
- **Claude** (l'implémenteur) écrit ici : notes (`NOTE_CLAUDE_*`), questions
  (`QUESTION_CLAUDE_*`), spécifications de solution (`NOTE_SOLUTION_*`).
- Tout fichier est daté (`_YYYYMMDD`) et, quand il juge du code, ancré au
  hash court du commit jugé.

## Conventions minimales

- `ETAT_COURANT.md` (à créer au premier audit) : le verdict mutable unique,
  ancré au HEAD — pas un fichier par jour.
- `PISTES_FERMEES.md` : mémo append-only des tentatives fermées (idée, cause
  d'abandon, ce qui survit). Une piste fermée ne se rouvre qu'avec un nouveau
  théorème + fixture, jamais sur un benchmark.
- Toute contradiction mathématique devient une **fixture minimale
  permanente** dans `tests/` avant la poursuite du travail.
- Équations : une seule ligne physique, accolades explicites, pas de
  `\operatorname`, `\left\Vert`/`\left\lbrace` (règles du dépôt,
  `python tools/check_docs.py`).

## Questions ouvertes au démarrage

Les cinq premières questions posées à l'auditeur sont au bas de
[`../docs/MATHEMATIQUES.md`](../docs/MATHEMATIQUES.md) (Q1–Q5) : bijection
événements-boules, qualité du minorant de témoins, complétude de la source
WSPD fail-open de bout en bout, convention `F_K` du rendu § 9.1, politique
des ex æquo.
