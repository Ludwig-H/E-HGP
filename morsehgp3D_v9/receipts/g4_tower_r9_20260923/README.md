# Reçu : session G4 R9, ablation appariée de l'ordonnancement q3/q4 (sonde v14)

23 septembre 2026, 10:19–10:25 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la cible
fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48), génération
`2026-09-23T03:19:20.797-07:00`, arrêt ciblé certifié et `TERMINATED` relu
(dernier arrêt `2026-09-23T03:25:51.064-07:00`). GPU non utilisé.

Paquet `fe1142b5` (sonde v14, plan v6, snapshot `cd1dd5a3…`, manifeste
`aa35b148…`, worker `6669a06d…`, contrôleur `cd6ff697…`, protocole au commit).
Reçus hôte et worker **`completed`**. Les 24 cas sont `complete_relative` ; les
18 comparaisons d'objet sont égales et Euler vaut « holds » partout. Seuls
leviers variants : `q34_jobs_by_mass` et `q34_fine_jobs`, ensemble ON (défaut
épinglé, premier cas du préflight) ou ensemble OFF. W48, tour statique 48,
s = 8, deux répétitions entrelacées ON/OFF.

## Résultats (répétitions 0 / 1)

Colonnes :
- **attente** : temps passé par les fils q3/q4 à attendre la file de
  tâches / (fils × mur q34) ;
- **plus long job** : mur du plus long job du front.

| trame | K | chaîne OFF (s) | chaîne ON (s) | q34 OFF → ON (s) | attente OFF → ON | plus long job OFF → ON (s) |
| --- | ---: | --- | --- | --- | --- | --- |
| 000100 | 5 | 3,69 / 3,80 | **2,80 / 2,82** | 2,52 → 1,66 | 35 % → 1 % | 2,51 → 0,23 |
| 000000 | 5 | 5,24 / 5,48 | **3,80 / 3,80** | 3,70 → 2,21 | 42 % → 1 % | 3,51 → 0,31 |
| 000200 | 5 | 6,30 / 6,29 | **3,99 / 4,07** | 4,59 → 2,36 | 49 % → 1 % | 3,70 → 0,26 |
| 000100 | 10 | 9,51 / 9,45 | **8,60 / 8,65** | 5,34 → 4,44 | 17 % → 0 % | 5,31 → 0,46 |
| 000000 | 10 | 13,60 / 13,57 | **11,75 / 11,75** | 8,07 → 6,24 | 29 % → 7 % | 6,80 → 0,57 |
| 000200 | 10 | 14,95 / 15,09 | **11,89 / 11,85** | 9,63 → 6,61 | 34 % → 3 % | 6,83 → 0,55 |

Condensés de tour identiques entre ON et OFF, et à R7b/R8.

## Lecture

- **Diagnostic confirmé** : sans les leviers, le plus long job du front
  (2,5 à 6,8 s) est presque égal au mur de q34. La préparation en largeur
  laissait un produit dense entier.
- Préparation par masse et grain 4× plus fin : l'attente tombe à 0–7 %, et
  q34 baisse de ×1,5 à ×2,0 à K5, de ×1,2 à ×1,5 à K10.
- **Meilleure chaîne K5 : 2,80 s** (000100), 3,80 s (000000), 3,99 s
  (000200). K10 : 8,60 / 11,75 / 11,89 s.
- Contrat (1 s puis 100 ms) non atteint : aucune qualification. Restent :
  - q34 à K5 : 1,7 à 2,4 s ;
  - la tour : 0,7 à 1,0 s à K5 et 3 à 4 s à K10 ;
  - q2 + recensement + fusion : environ 0,4 à 0,8 s.

## Contenu

Même structure que R8 : `vm/` (sorties du worker), `host/` (contrôleur et
journaux expurgés ; ni `oslogin_add`, ni clé, ni archive), `PACKAGE.json`,
`SUMMARY.json` (schéma `mhgp9_g4_tower_summary_v2`), `SHA256SUMS`.
