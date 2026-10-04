# Audits courants de la v11

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Revue transversale **0f5e8a207**,
actualisation **0af635a71/b72fe8771** : moteur inchangé, démos et sortie de membres.
**Audit depuis les fondations** : sept modules/101 fichiers natifs relus,
mathématiques, points/tête, bancs, contrats temps/mémoire et protocole G4.
[Matrice, preuves, contrôles et limites](../receipts/audit_giant_20261004/README.md).
[Dernier delta relu](../receipts/audit_giant_publication_20261004/README.md).

- [Mathématiques : sélection, frontières et contrats](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](REPONSE_CLAUDE_POINTS_20261003.md).
- [Audit indépendant d'ouverture, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**Deux corrections nouvelles.** Après réveil sur abandon, le pipeline peut
poursuivre la lecture de l'ordre incomplet ; ajouter la garde après l'attente.
L'export POINTS192 refuse un tétraèdre u24 valide de niveau 196/148 bits.
Les témoins et correctifs proposés figurent dans la note moteur. Aucun faux
succès FULL ni race native reproduite n'est revendiqué.

**Aide mathématique nouvelle.** B=rencontre(H,core), date stable 3ε sous H3 :
un LCA par point remplace son balayage des coupes. **12 968 gardes exactes**,
32 ordres/1 082 coupes/156 dates B ; preuve et limites dans la note mathématique.
Les corrections encore ouvertes sur manifestes, cohortes/EOM, racine et
métriques restent intégrées aux deux notes, sans journal supplémentaire.

FULL K5/u21/W48 : dernières médianes publiées **412 /352 /381 ms** sur trois
trames sans sol de la séquence08. Le banc F qualifie export FULL natif +
projection Python exacte ; **points/sélection natifs, 100 ms, GPU et massif
restent ouverts**. Le reçu G4 récent de **360 bouts/10 séquences** est
recoupé ; il mesure des meilleurs blocs sur extraits annotés. Les auditeurs
n'ont lancé aucun nouveau G4, build/test natif ou fit pour cette publication.

**Nettoyage : cinq Markdown actifs.** Six dialogues dépassés sont
[archivés avec leurs octets, auteurs et empreintes](../receipts/audit_dialogues_20261004/README.md).
Les liens entrants sont réorientés ; les reçus déjà clos restent immuables.
Pas de nouveau journal ni de chronologie redondante dans audits/.
Une contradiction utile devient une fixture ; les résultats sont rattachés
à leur source et leur domaine. Une seule session G4 gardée à la fois,
avec arrêt ciblé certifié.
