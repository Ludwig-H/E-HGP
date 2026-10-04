# Audits courants de la v11

4 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. **Deuxième relecture complète e02a6c235**,
actualisation **8f68622b2** : moteur inchangé, démos et membres publiés.
**Audit depuis les fondations** : sept modules/101 fichiers natifs relus,
mathématiques, points/tête, bancs, contrats temps/mémoire et protocole G4.
[Matrice, preuves, contrôles et limites](../receipts/audit_giant_20261004/README.md).
[Contrelecture approfondie et corrections](../receipts/audit_deep_20261004/README.md).

- [Mathématiques : sélection, frontières et contrats](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](REPONSE_CLAUDE_POINTS_20261003.md).
- [Audit indépendant d'ouverture, maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

**Deux corrections confirmées.** Après réveil sur abandon, le pipeline peut
poursuivre la lecture de l'ordre incomplet ; ajouter la garde après l'attente.
L'export POINTS192 refuse un tétraèdre u24 valide de niveau 196/148 bits.
Les témoins et correctifs proposés figurent dans la note moteur. Aucun faux
succès FULL ni race native reproduite n'est revendiqué.

**Aide mathématique nouvelle.** B=rencontre(H,core), date stable 3ε sous H3 :
un LCA par point remplace son balayage des coupes. **12 968 gardes exactes**,
32 ordres/1 082 coupes/156 dates B ; preuve et limites dans la note mathématique.
La contrelecture prouve aussi **sB≤t′+d_k/2**, sans ajouter un deuxième
plafond de retard. B peut toujours perdre les frontières ; aucune optimalité
statistique ni garantie des labels EOM n'en découle.
Les corrections encore ouvertes sur manifestes, cohortes/EOM, racine et
métriques restent intégrées aux deux notes, sans journal supplémentaire.

FULL K5/u21/W48 : dernières médianes publiées **412 /352 /381 ms** sur trois
trames sans sol de la séquence08. Le banc F qualifie export FULL natif +
projection Python exacte ; **points/sélection natifs, 100 ms, GPU et massif
restent ouverts**. Les mêmes XYZ sont
établis côté v10/u18 : écart actuel descriptif **×1,50–1,72**, pas un A/B.
Les pistes q3 différé et subdivision des centres sont documentées, sans gain
attribué avant mesure. **360 bouts/10 séquences puis 31 bouts + cinq démos**
sont recoupés ; deux blocs de vélos k5 sont disjoints, sans tête plate jouée.
Les auditeurs
n'ont lancé aucun nouveau G4, build/test natif ou fit pour cette publication.

**Relecture des idées v1–v10, au pin 4fac50118.** Deux reprises sont retenues :
candidat q3 différé commun catalogue/MEB ; extrema q2 couplés pour un helper
de census distinct. [Preuves ciblées et contrats de port](../receipts/audit_heritage_20261004/README.md).
Les propositions déjà intégrées ou sans bénéfice étayé n'ajoutent aucun journal.

**Nettoyage : cinq Markdown actifs.** Six dialogues dépassés sont
[archivés avec leurs octets, auteurs et empreintes](../receipts/audit_dialogues_20261004/README.md).
Les liens entrants sont réorientés ; les reçus déjà clos restent immuables.
Pas de nouveau journal ni de chronologie redondante dans audits/.
Une contradiction utile devient une fixture ; les résultats sont rattachés
à leur source et leur domaine. Une seule session G4 gardée à la fois,
avec arrêt ciblé certifié.
