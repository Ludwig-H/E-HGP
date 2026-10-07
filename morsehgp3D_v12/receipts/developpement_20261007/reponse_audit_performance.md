# Réponse du développeur aux contre-lectures `audit_performance` et `audit_reprise` (7 octobre 2026)

Constats `CST-0233` à `CST-0238` de l'auditeur Codex ([performance](../audit_performance_20261007/README.md),
[reprise](../audit_reprise_20261007/README.md)). Les cellules du registre renvoient ici pour rester sous le plafond
du canal.

| Constat | Réponse |
| --- | --- |
| `CST-0233` | Transmis à la tranche T1-b (agent de la voie appareil), qui écrit une fin d'étage unique en source partagée (exécuteur Pool sur l'hôte, CUDA sur l'appareil) et la confronte à la preuve et aux quatre mutants du scan par blocs proposé ; mesure CPU à 48 fils au prochain passage G4. |
| `CST-0234` | Transmis à T1-b : correction en source unique proposée en commit séparé si les émissions restent identiques mot à mot, sinon suite déclarée. |
| `CST-0235` | Bilan T0 corrigé dans `PLAN.md` : le budget C n'est pas confirmé par M2 et M5 ; émission, fin d'étage et transferts restent à mesurer ; la mesure G4 de T1-b publiera séparément parcours, feuilles, émission, fin d'étage, transferts et total. |
| `CST-0236` | [Erratum du reçu de la session G](../g4_t0g_20261007/ERRATUM_20261007.md) : quasi-sphère arrondie, refus `wide_leaf` sur une liste candidate ; `MESURE.md` et le README de `MES-P` corrigés. |
| `CST-0237` | Objectif déclaré non couvert par T1 ; chantier **T1-c, voie large**, ouvert dans `PLAN.md` § 2 avec le protocole en quatre temps de l'auditeur. |
| `CST-0238` | `analyse_p.py` réécrit (groupes (K, fils, famille), fils dans chaque ligne, cohorte commune, échecs avec fils) ; pilote : sélection vide refusée avant toute construction, prise expirée ou en échec sans valeur chaude, groupe de processus tué au délai ; porte `microbancs/mes_p_petits/test_pilote_p.py` (quatre cas, aussi sous `-O`), sur laquelle l'ancien lecteur fait 5 écarts. Commit `5682d00f5`. |
