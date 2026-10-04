# Audits courants de la v11

Suivi du 4 octobre 2026, source **ab1a739d1** : côté auditeur sur instruction utilisateur.
Priorité active : **hiérarchie de points → clustering plat**, avec tests synthétiques et `Zoltan/`.
Ses deux notes sont mises à jour en place :

- [État du moteur, corrections et mesures G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Invariants mathématiques et FULL→points](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

Le chantier privé de sélection est suivi : condensation N-aire, EOM/feuilles,
critère d'existence et complétion. Conseil courant : tête de référence à masses
entières/λ=1/r, variantes déclarées ; antichaîne globale et bruit explicite.
[Témoin HGP exact à neuf points](../receipts/flat_selection_math_20261004/README.md),
[complément sur les départs condensés](../receipts/flat_selection_math_r2_20261004/README.md) :
λ=1/r et λ=1/r³ choisissent des partitions différentes ; H3 ne garantit pas
la stabilité du choix à une égalité de scores.
[Contrat numérique et compact](../receipts/flat_selection_contract_20261004/README.md) :
scores EOM certifiés ou refus, arbre de points sans matrice n², diagonales
ignorables seulement à mcs≥2. [Équité du banc](../receipts/flat_selection_evidence_20261004/README.md) :
HDBSCAN officiel et sélection commune atomique doivent rester deux bras distincts.

Note du développeur : [écart v10/v11, correctifs et mesure G4](NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).
Note du développeur, 3 octobre au soir : [audit de la v11, ports v10 et tranche 3](NOTE_CLAUDE_AUDIT_V11_20261003.md)
(répond aussi aux deux points de relecture du pipeline : tri des blocs en place, banc à verdict).
Questions du développeur, 3 octobre 22 h 45 : [preuves manquantes pour la hiérarchie de points
$H^{r}_{k+1}$](QUESTION_CLAUDE_PREUVES_POINTS_20261003.md) (stabilité de bout en bout, optimalité et retard sous
qualification, chapitre 7, compatibilité verticale, cibles contre stabilité, port natif).
Réponse du développeur, complétée le 4 octobre : [Q1–Q8 adoptées, porte stricte G4 conforme,
relance sur la distance de mesures](REPONSE_CLAUDE_POINTS_20261003.md) ; note
[HIERARCHIE_POINTS](../docs/HIERARCHIE_POINTS.md).

Référence figée **c40f40798** : 4073/4073 portes, 326 mutants tués, 81/81 prises
appariées aux sorties identiques. FULL K5/CPU/u21/W48 médian **489 / 345 / 432 ms** ;
cible 200 ms ouverte. [Preuves et lectures](../receipts/qualification_performance_20261003/README.md).
[Nettoyage du Codespace](../receipts/developpement_20261003/codespace_cleanup/README.md) clos ; HGP-old préservé.

Publication **b87285378** relue : réserves du pipeline et du banc levées.
Reçu `claudeab7` :666portes/7TSan/11mutants et36prises appariées conformes ;
FULL K5/u21/W48 médian **412 /352 /381 ms**. Périmètre et limites dans la
première note.

[Porte stricte et campagnes E/F relues](../receipts/points_gate_qualification_20261004/README.md) :
F joue le commit poussé f02f91c7e, conforme sur 2 854 nuages, 12 fixtures,
4 mutants **Python** et 205 cas synthétiques/LiDAR. Export FULL natif CPU/u21,
règle de points et oracle exacts Python ; port natif des dates encore ouvert.
Les 201 JSON communs D/F sont identiques hors temps, pas toutes les décisions
internes. E garde son échec de dossier manquant ; arrêts E/F certifiés.
F termine les voisines : 71 scènes distinctes/859 observations d'instances,
corrélées ; sol conservé en démo 04, tailles jusqu'à 126 267 sites.
Les scores mesurent le meilleur bloc ; condensation et sélection restent ouvertes.

[Réponses Q1–Q8 au développeur](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md)
et [preuves reproductibles](../receipts/points_answers_20261003/README.md) :
entrelacement géométrique et stabilité 3ε, retard qualifié borné, optimalité
intrinsèque ; asymptotique conditionnelle, croisement multi-k exact.
L'impossibilité générale de Q6 est réfutée sous les axiomes écrits ; ER0h
n'a pas de borne uniforme pour ses dates en rayon. Contrat de dates natives
proposé avec budgets u18/u21/u24, sans port natif qualifié.
Les anciens défauts arithmétiques sont corrigés, y compris le propriétaire au
plateau exact de l'oracle, désormais exercé par F. [Limite d'API](../receipts/points_code_review_20261004/README.md) :
m>n refuse explicitement, domaine sauté par la porte ; choisir domaine m≤n
ou points inactifs avant une API générale.
[Complément de Palm conditionnel](../receipts/palm_obstruction_20261003/README.md) :
à k2/m3 et intensité/rayon fixes, un événement ouvert retarde un site core du
géant ; sous définition mesurable/fidèle de H∞, ΘH<Θpoly. Aucun transfert
automatique au théorème limite de la thèse.

Compléments transmis au développeur :

- [Mesures normalisées et insertions](../receipts/measure_metric_20261004/README.md) :
  W_p fini ne contrôle pas uniformément les dates en norme SUP si les seuils
  restent des comptes unitaires. W∞ et l'erreur pondérée ne sont pas tranchés.
- [Propriétaires infinis et expérience κ=1/2](../receipts/points_math_followup_20261004/README.md) :
  localement fini ne garantit pas une rencontre atteinte ; preuve presque sûre
  requise pour Poisson. Augmenter κ augmente le rappel dans une même composante
  FULL, sans garantie sur l'IoU ; diagnostic exact R1 et horizon en rayon fournis.

Les notes du développeur et de l'autre auditeur sont préservées. Les échanges
d'ouverture du 2 octobre conservent leurs décisions, pas l'autorité de leurs
anciens statuts ; l'état présent est dans les deux notes ci-dessus.

Conventions : notes motivées et ancrées à une source, réponse par le développeur,
personne ne réécrit le fichier d'un autre. Une contradiction devient une fixture
permanente. Preuves historiques dans receipts/Git, pas de nouveau journal redondant.
Une seule session G4 gardée à la fois ; arrêt ciblé certifié après chaque session.
