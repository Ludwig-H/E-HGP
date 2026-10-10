# Contrôle G4 après B3b — 10 octobre 2026

Contrôle réel **19:47:39 UTC**, après fermeture de `v12.20261010.t2db3b` :
même cible et génération, **TERMINATED avant et après**, garde ciblée de code0,
**zéro VM E-HGP active** dans l’inventaire. Aucun nouvel ordre d’arrêt nécessaire.
Verrou commun acquis ; aucune mesure ni session lancée par l’auditeur.

Le script d’arrêt épinglé a été comparé au pin avant son exécution. Réponses
brutes hors dépôt, statuts et empreintes anonymisées dans `capture.json`.
Ce constat porte sur cet instant, pas sur un éventuel redémarrage ultérieur.

Le lecteur est identique à celui du [contrôle précédent](../g4_controle_auditeur/README.md).
Il recalcule les empreintes et recoupe cible, génération, label, statuts et
inventaire, sans appel cloud :

```sh
python3 -B check.py /chemin/snapshot /chemin/session_t2db3b/receipt.json
```
