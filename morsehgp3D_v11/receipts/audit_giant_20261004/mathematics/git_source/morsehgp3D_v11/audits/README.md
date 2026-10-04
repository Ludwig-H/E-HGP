# Audits courants de la v11

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Produit ab1a739d1, outil G4 f1a53fe1c.
Priorité : **hiérarchie de points → clustering plat** ; preuves et contrôles
bornés dans receipts/, deux notes d'audit mises à jour en place.

- [Mathématiques : sélection, frontières et contrats](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](REPONSE_CLAUDE_POINTS_20261003.md).
- [Audit indépendant d'ouverture, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**Nouveaux résultats utiles.** Le raffinement de la sélection quand z augmente
est prouvé à condensation/cohortes fixes ; la factorisation conserve les blocs
mais exige de transporter les cohortes différées pour conserver les scores.
[12 917 gardes exactes](../receipts/flat_model_followup_20261004/README.md).
Une formule de réciproque certifie les égalités EOM, même irrationnelles :
[888 gardes et helper](../receipts/eom_exact_audit_20261004/README.md).

Deux corrections ciblées : le plafond B(H) atomique ne couvre pas les clusters
binaires officiels de HDBSCAN ; le préparateur omet un hash exigé par le nouvel
outil d'archives G4. [Métriques et F2](../receipts/flat_evidence_followup_20261004/README.md),
[interopération des manifestes](../receipts/unpack_manifest_review_20261004/README.md).
Complétion, exception de racine et code compact :
[contrats testés](../receipts/flat_contract_followup_20261004/README.md).

FULL K5/u21/W48 : dernières médianes publiées **412 /352 /381 ms** sur trois
trames sans sol de la séquence08. Le banc F qualifie export FULL natif +
projection Python exacte ; **points/selection natifs, 100 ms, GPU et massif
restent ouverts**. Aucun nouveau G4 dans cette publication.

**Nettoyage : cinq Markdown actifs.** Six dialogues dépassés sont
[archivés avec leurs octets, auteurs et empreintes](../receipts/audit_dialogues_20261004/README.md).
Les liens entrants sont réorientés ; les reçus déjà clos restent immuables.
Pas de nouveau journal ni de chronologie redondante dans audits/.
Une contradiction utile devient une fixture ; les résultats sont rattachés
à leur source et leur domaine. Une seule session G4 gardée à la fois,
avec arrêt ciblé certifié.
