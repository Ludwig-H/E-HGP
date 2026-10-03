# Canal des audits de la v11

Mêmes conventions que pour la v10.

Suivi actif au 3 octobre 2026 : l’auditeur indépendant passe côté développeur
sur instruction de l’utilisateur. Ses deux notes sont mises à jour **en place** :

- [Reprise, état du moteur et écart avec la v10](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Invariants mathématiques et chantiers précis](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

Note développeur du 3 octobre : [écart v10/v11 et premiers correctifs](NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).

Les questions/réponses du 2 octobre conservent les décisions d’ouverture ;
leurs anciens statuts ne remplacent pas le suivi actuel. Le fichier de
l’autre auditeur et les reçus historiques sont préservés. Les nouvelles
preuves bornées sont regroupées dans une [capture compacte](../receipts/developpement_20261003/reprise_performance/README.md).

- Les auditeurs déposent ici `AUDIT_*`, `CONTRE_AUDIT_*`, `ADDENDUM_*`, `QUESTION_AUDITEUR_*`, datés `_YYYYMMDD` et
  ancrés au hash court du code jugé. Ils poussent sur `main`.
- Le développeur répond par `REPONSE_CLAUDE_*`, `NOTE_CLAUDE_*`, `QUESTION_CLAUDE_*`.
- Personne ne modifie le fichier d'un autre.
- Un audit motive une correction ; il ne certifie rien. Une contradiction mathématique devient une fixture minimale
  permanente avant toute autre dépense.
- Heures : celle de `date -u` au moment d'écrire, jamais une estimation.
- Une seule VM G4 : le développeur lance les sessions ; un auditeur demande une fenêtre par `QUESTION_AUDITEUR_*`.
