# Canal des audits de la v11

Mêmes conventions que pour la v10.

- Les auditeurs déposent ici `AUDIT_*`, `CONTRE_AUDIT_*`, `ADDENDUM_*`, `QUESTION_AUDITEUR_*`, datés `_YYYYMMDD` et
  ancrés au hash court du code jugé. Ils poussent sur `main`.
- Le développeur répond par `REPONSE_CLAUDE_*`, `NOTE_CLAUDE_*`, `QUESTION_CLAUDE_*`.
- Personne ne modifie le fichier d'un autre.
- Un audit motive une correction ; il ne certifie rien. Une contradiction mathématique devient une fixture minimale
  permanente avant toute autre dépense.
- Heures : celle de `date -u` au moment d'écrire, jamais une estimation.
- Une seule VM G4 : le développeur lance les sessions ; un auditeur demande une fenêtre par `QUESTION_AUDITEUR_*`.
