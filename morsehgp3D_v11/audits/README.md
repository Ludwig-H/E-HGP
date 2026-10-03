# Audits courants de la v11

Suivi du 3 octobre 2026 : retour côté auditeur sur instruction utilisateur.
Priorité : passage FULL → hiérarchie de points, avec tests synthétiques et `Zoltan/`.
Ses deux notes sont mises à jour en place :

- [État du moteur, corrections et mesures G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Invariants mathématiques et FULL→points](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

Note du développeur : [écart v10/v11, correctifs et mesure G4](NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).
Note du développeur, 3 octobre au soir : [audit de la v11, ports v10 et tranche 3](NOTE_CLAUDE_AUDIT_V11_20261003.md)
(répond aussi aux deux points de relecture du pipeline : tri des blocs en place, banc à verdict).

Moteur jugé **c40f40798** : 4073/4073 portes, 326 mutants tués, 81/81 prises
appariées aux sorties identiques. FULL K5/CPU/u21/W48 médian **489 / 345 / 432 ms** ;
cible 200 ms ouverte. [Preuves et lectures](../receipts/qualification_performance_20261003/README.md).
[Nettoyage du Codespace](../receipts/developpement_20261003/codespace_cleanup/README.md) clos ; HGP-old préservé.

Les notes du développeur et de l'autre auditeur sont préservées. Les échanges
d'ouverture du 2 octobre conservent leurs décisions, pas l'autorité de leurs
anciens statuts ; l'état présent est dans les deux notes ci-dessus.

Conventions : notes motivées et ancrées à une source, réponse par le développeur,
personne ne réécrit le fichier d'un autre. Une contradiction devient une fixture
permanente. Preuves historiques dans receipts/Git, pas de nouveau journal redondant.
Une seule session G4 gardée à la fois ; arrêt ciblé certifié après chaque session.
