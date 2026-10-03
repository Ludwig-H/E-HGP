# Audits courants de la v11

Suivi du 3 octobre 2026 : retour côté auditeur sur instruction utilisateur.
Priorité : passage FULL → hiérarchie de points, avec tests synthétiques et `Zoltan/`.
Ses deux notes sont mises à jour en place :

- [État du moteur, corrections et mesures G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Invariants mathématiques et FULL→points](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

Note du développeur : [écart v10/v11, correctifs et mesure G4](NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).
Note du développeur, 3 octobre au soir : [audit de la v11, ports v10 et tranche 3](NOTE_CLAUDE_AUDIT_V11_20261003.md)
(répond aussi aux deux points de relecture du pipeline : tri des blocs en place, banc à verdict).
Questions du développeur, 3 octobre 22 h 45 : [preuves manquantes pour la hiérarchie de points
$H^{r}_{k+1}$](QUESTION_CLAUDE_PREUVES_POINTS_20261003.md) (stabilité de bout en bout, optimalité et retard sous
qualification, chapitre 7, compatibilité verticale, cibles contre stabilité, port natif).
Réponse du développeur, 3 octobre 23 h 38 : [réponses Q1–Q8 adoptées, porte stricte, campagnes C/D
identiques](REPONSE_CLAUDE_POINTS_20261003.md) ; note [HIERARCHIE_POINTS](../docs/HIERARCHIE_POINTS.md).

Référence figée **c40f40798** : 4073/4073 portes, 326 mutants tués, 81/81 prises
appariées aux sorties identiques. FULL K5/CPU/u21/W48 médian **489 / 345 / 432 ms** ;
cible 200 ms ouverte. [Preuves et lectures](../receipts/qualification_performance_20261003/README.md).
[Nettoyage du Codespace](../receipts/developpement_20261003/codespace_cleanup/README.md) clos ; HGP-old préservé.

Publication **b87285378** relue : réserves du pipeline et du banc levées.
Reçu `claudeab7` :666portes/7TSan/11mutants et36prises appariées conformes ;
FULL K5/u21/W48 médian **412 /352 /381 ms**. Périmètre et limites dans la
première note.

[Campagne FULL→points close](../receipts/full_points_20261003/README.md) sur c40
figé :12synthétiques +5Zoltan entiers,68fits HDBSCAN,2553comparaisons, arrêts
certifiés. À K5, fermeture m3 / premières attaches-LCA / HDBSCAN :
**0,8176 /0,8458 /0,7746** en synthétique ; **0,6118 /0,6460 /0,6030** sur
Zoltan (meilleur IoU par objet, pas une partition automatique). La fermeture
est stable mais ne gagne pas partout. Raccord direct exact par forts/faibles
proposé au développeur ; règle statistique et synthèse multi-k encore ouvertes.

[Réponses Q1–Q8 au développeur](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md)
et [preuves reproductibles](../receipts/points_answers_20261003/README.md) :
entrelacement géométrique et stabilité 3ε, retard qualifié borné, optimalité
intrinsèque ; asymptotique conditionnelle, croisement multi-k exact.
L'impossibilité générale de Q6 est réfutée sous les axiomes écrits ; ER0h
n'a pas de borne uniforme pour ses dates en rayon. Contrat de dates natives
proposé avec budgets u18/u21/u24, sans port natif qualifié.
Les anciens défauts arithmétiques sont corrigés. Le mauvais propriétaire
au plateau exact de l'oracle ab200 est corrigé en 2f05 et vérifié
indépendamment en Python ; cette revue ne qualifie pas la campagne G4.

Les notes du développeur et de l'autre auditeur sont préservées. Les échanges
d'ouverture du 2 octobre conservent leurs décisions, pas l'autorité de leurs
anciens statuts ; l'état présent est dans les deux notes ci-dessus.

Conventions : notes motivées et ancrées à une source, réponse par le développeur,
personne ne réécrit le fichier d'un autre. Une contradiction devient une fixture
permanente. Preuves historiques dans receipts/Git, pas de nouveau journal redondant.
Une seule session G4 gardée à la fois ; arrêt ciblé certifié après chaque session.
