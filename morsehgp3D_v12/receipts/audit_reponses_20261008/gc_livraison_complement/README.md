# Complément Gc : D6 et journal CTest

La [proposition D6 G/Gc](../gc_livraison/README.md), puis le [correctif du plan](../../audit_d6_20261007/pilotage/README.md), se composent sur le D6 livré `48f40fd6`. Les cinq témoins déjà joués via le harness publié donnent les codes0/2/2/2/2 : six prises nominales ; aucune sur les quatre plans refusés. Le lecteur ci-dessous revérifie l'application et le hash résultant, sans rejouer ces témoins.

Le journal primaire local contient **700 Passed + 1 Skipped sur 701 sélectionnés**, zéro Failed, parmi 731 portes enregistrées. La sentinelle saute faute de `MHGP12_DATA_DIR`. D2/MEMO et les 18 juges G normal/−O sont Passed. Build Release u21, CUDA OFF ; répertoires différentiels v11 vides. Tests du 8 octobre, 01:05–01:11 UTC. Les logs pointent vers le worktree main mutable ; aucun manifeste source avant/après dans les artefacts lus. Cela confirme le résultat local consigné, sans requalifier un snapshot, LiDAR, GPU ou G4.

```
python CHEMIN/check.py --repo DEPOT --logs DOSSIER_DES_LOGS
```
Logs externes non copiés. Relecture de métadonnées et composition temporaire seulement ; aucun moteur lancé. Le reçu initial reste inchangé.
