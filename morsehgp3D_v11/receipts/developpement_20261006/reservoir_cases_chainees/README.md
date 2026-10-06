# Réservoir chaîné des cases du lot : l'écriture disparaît, mais le comptage ralentit (session reservoir2)

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

## Incident de démarrage (première tentative)

La session `v11.20261006.claudereservoir` (13 h 35 UTC) n'a jamais démarré. L'opération `start` a échoué en
`ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS` : plus de capacité SPOT dans `us-central1-c`. Le contrôleur a rendu
`shutdown_uncertified` (génération inconnue).

Procédure suivie :
- `--recover` : aucun processus ne référençait la session ;
- lectures seules à 13 h 37 puis 13 h 40 UTC : `TERMINATED`, `lastStartTimestamp` égal à la génération de la session
  précédente (05:46:32.748 -07:00), aucune opération en cours.

Aucun démarrage n'a donc eu lieu, et aucune commande mutante n'a été lancée. Pièces : `receipt_tentative_spot.json`,
`recovery_tentative_spot.json`.

## Session reservoir2

Session `v11.20261006.claudereservoir2`, 13 h 42 UTC, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt
`TERMINATED` certifié. Source `59509bbc8`.

Les trois commandes sont conformes. Le code 1 du worker vient du dépassement du plafond des résultats : le banc
sanitizer laissait un dump de 272 Mo dans son dossier, défaut corrigé en `79fa5e9f7`.

**Exactitude.** Les deux bancs sont `conforme` : dumps et registres identiques en CPU, en GPU avec réservoir (81915)
et en GPU sans réservoir (212987, débordements rejoués). Compute Sanitizer (memcheck et racecheck sur A et B, synccheck
sur A, K10) : zéro erreur, dumps égaux au CPU.

**Médianes à chaud (ms)**

| K, feuilles | Trame | Exécuteur avec / sans réservoir | Écriture avec / sans | Comptage avec / sans | Comptage j2memo |
| --- | --- | --- | --- | --- | --- |
| K5, 16 | ng00 | 49 / 62 | 0,3 / 12,9 | 43 / 43 | 30 |
| K5, 16 | ng01 | 40 / 55 | 0,3 / 14,8 | 35 / 35 | 25 |
| K5, 16 | ng02 | 46 / 56 | 0,4 / 9,3 | 41 / 41 | 28 |
| K10, 24 | ng00 | 240 / 312 | 1,7 / 75,5 | 224 / 222 | 172 |
| K10, 24 | ng01 | 193 / 268 | 1,5 / 78,0 | 181 / 180 | 139 |
| K10, 24 | ng02 | 223 / 290 | 1,8 / 70,9 | 209 / 207 | 159 |

**Critère écrit d'avance** (exécuteur avec réservoir ≤ 0,95 × sans à K5, ≤ 0,85 × à K10) : atteint, à 0,73 à 0,83.

**Mais le comptage a ralenti dans les deux bras**, de +43 % à K5 et +30 % à K10 par rapport à la session j2memo. Le
gain net sur j2memo est nul à K5 et de −8 % à K10.

Cause : le nouveau puits du comptage. Il vérifiait le bloc à chaque octet de population et passait par l'arène à
chaque écriture. Ce code est intégré dans les trois instanciations du recensement.

Correction (`79fa5e9f7`) : le chemin chaud redevient l'écriture directe de la case du 5 octobre, et le changement de
bloc passe dans une fonction hors ligne. Elle est mesurée par la session reservoir3, avec un critère écrit d'avance
relatif à j2memo.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec le critère écrit d'avance |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `gpu_ab_report_k5.json`, `gpu_ab_report_k10.json` | Bancs |
| `gpu_sanitizer.json` | Compute Sanitizer |
| `receipt_tentative_spot.json`, `recovery_tentative_spot.json` | Tentative sans démarrage |

`SHA256SUMS` couvre les autres fichiers.
