# Contrôle G4 après MES-B1o — 10 octobre 2026

Contrôle réel **20:30:17 UTC**, après fermeture de `v12.20261010.mesb1o` :
même cible et génération, **TERMINATED avant et après**, garde ciblée de code0,
**zéro VM E-HGP active** dans l’inventaire. Aucun nouvel ordre d’arrêt nécessaire.
Verrou commun acquis ; aucune mesure ni session lancée par l’auditeur.

Le script d’arrêt épinglé a été comparé au pin avant son exécution. Réponses
brutes hors dépôt, statuts et empreintes anonymisées dans `capture.json`.
Ce constat porte sur cet instant, pas sur un éventuel redémarrage ultérieur.

Le lecteur reprend le [contrôle B3b](../g4_controle_b3b/README.md), seul le texte
d’usage change. Il recalcule les empreintes et recoupe cible, génération, label,
statuts et inventaire, sans appel cloud :

```sh
python3 -B check.py /chemin/snapshot /chemin/session_mesb1o/receipt.json
```
