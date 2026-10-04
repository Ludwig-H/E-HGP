# Audits courants de la v11

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. **Deuxième relecture complète e02a6c235**,
actualisation **8f68622b2** : moteur inchangé, démos et membres publiés.
Suivi ciblé **c22be4e41 → 66372e621** : correctifs, auto-audit et sept questions
de vitesse traitées dans la note moteur (R1–R7).
Actualisation ciblée **77db5738e** : Euler/J1, feuilles GPU, budget commun
et banc froid/chaud relus ; qualification CUDA G4 encore attendue.
**d5b1d0179 / 61da03749** : réserves sur lecteurs/ordre/checkpoints corrigées
en source, nouvelles captures CPU relues séparément. **00800dd88** :
plafond du lot, ordre GPU et préchauffage relus ; banc revérifié au pinb74.
**Audit depuis les fondations** : sept modules/101 fichiers natifs relus,
mathématiques, points/tête, bancs, contrats temps/mémoire et protocole G4.
[Matrice, preuves, contrôles et limites](../receipts/audit_giant_20261004/README.md).
[Contrelecture approfondie et corrections](../receipts/audit_deep_20261004/README.md).

- [Mathématiques : sélection, frontières et contrats](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](QUESTION_CLAUDE_VITESSE_100MS_20261004.md) : P1 et P2 corrigés, sept verrous avant les leviers vers 100 ms (précédente : [points](REPONSE_CLAUDE_POINTS_20261003.md)).
- [Audit indépendant d'ouverture, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**P1/P2 corrigés dans les sources 3bd4d734e.** Garde d'abandon après réveil
et export u24 à quatre mots/version 2 relus ; portes causales ajoutées.
La matrice G4 **claudequal2 / eb036dbe2** passe : GCC Release, ASan/UBSan,
TSan et profils u21/u24 ; Clang absent. Les deux nouveaux mutants sont tués.
[Qualification publiée et recoupée](../receipts/developpement_20261004/qualification_p1p2/README.md).
[Contrelecture ciblée](../receipts/audit_selfreview_20261004/README.md).

**Ports relus : q3 différé, census par masques R et compteurs locaux.**
Géométrie exacte, niveau brut, contacts, support et compteurs logiques relus ;
q3+R est qualifié G4 Release/u21 dans claudeab8 (673 portes/sept mutants).
R1 et M3/E4 ont des essais G4 publiés avec le refus de style de diag1
conservé ; pas de gain qualifié. Réponse au contrat
J3 dans la note moteur, sans reproduire les hits du cache.
**Ordre A/B, lecteur pipeline, contexte parent et checkpoints corrigés en
source au pin61da.** Williams remplace l'ordre fixé à N pair ; les temps des
voies se rapportent au mur forêt et les fins de tâches sont exportées.
Le dérivé garde refus/identité/plan ; les timeouts conservent les prises
terminées. Rejeux bornés dans la note moteur. Le préenregistrement synthétique
conserve encore une attribution catégorique sous T−A non significatif.
H_L2 et z2 sont corrigés dans les sources ; le rejeu G4 z2 reste attendu.
[Ports](../receipts/audit_ports_20261004/README.md),
[réponses aux lecteurs et preuves actuelles](../receipts/audit_gpu_euler_20261004/README.md).
M3/E4 restent favorables en lecture : 7 552 gardes exactes. Les formules
statistiques sont justes ; cinq paires restent descriptives à 5 %.

**Suivi GPU/Euler : pins77/008, banc b74.** Euler à K+2 et restriction J1
favorables (3 623 gardes exactes), feuilles device et repli entier favorables
(145 391 gardes Fraction/Gram). Réservation CUDA dans le compte commun
confirmée sur ces sources. **Garde feuilles≤sites corrigée dans008** :
le témoin de 80 feuilles pour neuf sites est conservé ; les nouveaux plafonds,
l'ordre GPU et le préchauffage sont relus favorablement. La suppression CPU
de M3/E4 proposée par le développeur ne soulève pas d'objection mathématique.
Le banc accepte encore « conforme » sans prise froide ou sans passe chaude
mesurée (**75 gardes AST au pinb74**). Le ledger logique reste distinct du
travail physique. **Les transports scratch/pool b74 et warp/copie16 restent à auditer.**
[Sources épinglées et portée](../receipts/audit_gpu_euler_20261004/README.md),
[contrelecture du correctif et défaut du banc](../receipts/audit_gpu_update_20261004/README.md).

**Aide mathématique nouvelle.** B=rencontre(H,core), date stable 3ε sous H3 :
un LCA par point remplace son balayage des coupes. **12 968 gardes exactes**,
32 ordres/1 082 coupes/156 dates B ; preuve et limites dans la note mathématique.
La contrelecture prouve aussi **sB≤t′+d_k/2**, sans ajouter un deuxième
plafond de retard. B peut toujours perdre les frontières ; aucune optimalité
statistique ni garantie des labels EOM n'en découle.
Le producteur de manifestes est corrigé ; sa porte d'interopération reste
à terminer. Cohortes/EOM, racine et métriques restent dans les deux notes,
sans journal supplémentaire.

FULL K5/u21/W48 : référence qualifiée **412 /352 /381 ms** sur trois
trames sans sol de la séquence08. Le banc F qualifie export FULL natif +
projection Python exacte ; **points/sélection natifs, 100 ms, GPU et massif
restent ouverts**. Les diagnostics e49 donnent **408 /296 /360 ms** à K5/W48 libre et
**2 506 /1 822 /2 064 ms** à K10/leaf24 : identité non enregistrée pour ces
48 prises, refus de style du second lot conservé. Les 126 prises A/B ont
leurs dumps égaux ; W24 épinglé/W48 libre confond workers et affinité.
[Recoupe des nouvelles mesures CPU](../receipts/audit_gpu_euler_20261004/mesures_bindings/README.md).
Les mêmes XYZ sont
établis côté v10/u18 : écart b872/v10 descriptif **×1,50–1,72**, pas un A/B.
La subdivision des centres reste une piste documentée ; les ablations q3/R
restent descriptives. **360 bouts/10 séquences puis 31 bouts + cinq démos**
sont recoupés ; les deux meilleurs blocs de vélos k5 sont disjoints,
mais la tête plate publiée ne les sélectionne pas ensemble (note mathématique).
Les auditeurs
n'ont lancé aucun nouveau G4, build/test natif ou fit pour cette publication.

**Relecture des idées v1–v10, au pin 4fac50118.** Deux reprises sont retenues :
candidat q3 différé commun catalogue/MEB ; extrema q2 couplés pour un helper
de census distinct. [Preuves ciblées et contrats de port](../receipts/audit_heritage_20261004/README.md).
Les propositions déjà intégrées ou sans bénéfice étayé n'ajoutent aucun journal.

**Nettoyage : six Markdown actifs**, dont la nouvelle question courante
du développeur ; aucune nouvelle note d'auditeur. Six dialogues dépassés sont
[archivés avec leurs octets, auteurs et empreintes](../receipts/audit_dialogues_20261004/README.md).
Les liens entrants sont réorientés ; les reçus déjà clos restent immuables.
Pas de nouveau journal ni de chronologie redondante dans audits/.
Une contradiction utile devient une fixture ; les résultats sont rattachés
à leur source et leur domaine. Une seule session G4 gardée à la fois,
avec arrêt ciblé certifié.
