# morsehgp3D_v5/audits — le dossier de l'auditeur

Ce dossier est le **seul** endroit où l'auditeur intervient. Il reprend le
cycle documentaire des v3/v4 sans son inflation : moins de fichiers, datés,
ancrés au hash court du commit jugé, et un état courant unique.

## État audité

- Verdict mutable : [`ETAT_COURANT.md`](ETAT_COURANT.md).
- Audit bloquant du pin fonctionnel `87e915bd` :
  [`AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md`](AUDIT_BLOQUANT_87E915BD_SECURITE_CONFORMITE_PREUVES_20260827.md).
- Réponse aux verrous V1–V4 :
  [`REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`](REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md).

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

- `ETAT_COURANT.md` : le verdict mutable unique — pas un fichier par jour. Il
  nomme le pin fonctionnel complet effectivement jugé. Après livraison du
  verdict, le dernier commit qui touche `ETAT_COURANT.md` doit être `HEAD` pour
  qu'un claim, même borné, soit lisible ; tout commit fonctionnel ultérieur le
  rend automatiquement périmé.
- Un probe sur worktree sale est provisoire. Il conserve le SHA-256 du patch
  suivi (`git diff --binary`) et un manifeste SHA-256 de chaque fichier non
  suivi consommé. Il sépare toujours résultats du pin et résultats du
  worktree. Un worktree sale ne soutient aucun claim.
- `../docs/PISTES_FERMEES.md` est le chemin réservé au futur mémo append-only
  des tentatives fermées ; il est absent du pin courant. Une piste fermée ne
  se rouvre qu'avec un nouveau théorème + fixture, jamais sur un benchmark.
- Toute contradiction mathématique devient une **fixture minimale
  permanente** dans `../tests/` avant la poursuite du travail.
- Un audit se **lit et s'exécute** (commandes reproduites, codes de sortie
  comparés) avant toute réponse et avant toute dépense.
- Équations : une seule ligne physique, accolades explicites, pas de
  `\operatorname`, `\left\Vert` / `\left\lbrace`. Tant que
  `tools/check_docs.py` n'inclut pas la v5, appeler directement sa fonction
  `validate()` sur chaque Markdown de ce dossier.

## Questions ouvertes au démarrage

Les questions posées à l'auditeur vivent dans `QUESTION_CLAUDE_*`. Le chemin
réservé aux questions mathématiques héritées de la v4 est
`../docs/MATHEMATIQUES.md`, mais ce document est absent du pin courant et
n'existe que comme proposition non suivie dans le worktree capturé.
