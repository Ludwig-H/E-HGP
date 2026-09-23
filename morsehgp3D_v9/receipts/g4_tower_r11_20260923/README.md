# Reçu : session G4 R11, ablation appariée de l'ordonnancement q2 (sonde v16)

23 septembre 2026, 11:26–11:32 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la cible
fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48), génération
`2026-09-23T04:26:33.722-07:00`, arrêt ciblé certifié et `TERMINATED` relu
(dernier arrêt `2026-09-23T04:32:36.658-07:00`). GPU non utilisé.

Paquet `f685461a` (sonde v16, plan v6, snapshot `8c3f1182…`, manifeste
`6d68b6b7…`, worker `d22c3b73…`, contrôleur `cd6ff697…`, protocole au commit).
Reçus hôte et worker **`completed`**. Les 24 cas sont `complete_relative` ; les
18 comparaisons d'objet sont égales et Euler vaut « holds » partout. Seul
levier variant : `q2_jobs_by_mass` (ON, défaut épinglé, ou OFF) ; tous les
autres leviers sont ON. W48, tour statique 48, s = 8, deux répétitions
entrelacées.

## Résultats (répétitions 0 / 1)

| trame | K | chaîne OFF (s) | chaîne ON (s) | q2 OFF → ON (s) | q34 (s) | tour (s) |
| --- | ---: | --- | --- | --- | --- | --- |
| 000100 | 5 | 2,72 / 2,65 | **2,56 / 2,54** | 0,23 → 0,07 | 1,70 | 0,60 |
| 000000 | 5 | 3,58 / 3,64 | **3,21 / 3,25** | 0,45 → 0,10 | 2,15 | 0,73 |
| 000200 | 5 | 3,89 / 3,92 | **3,52 / 3,53** | 0,50 → 0,11 | 2,41 | 0,75 |
| 000100 | 10 | 7,76 / 7,97 | **7,68 / 7,79** | 0,41 → 0,15 | 4,45 | 2,39 |
| 000000 | 10 | 10,78 / 11,07 | **10,36 / 10,37** | 0,74 → 0,20 | 6,25 | 3,00 |
| 000200 | 10 | 11,06 / 11,20 | **10,58 / 10,53** | 0,82 → 0,22 | 6,58 | 2,75 |

Condensés de tour identiques entre ON et OFF, et à R7b–R10.

## Lecture

- Même pathologie que q3/q4 dans R9 : le plus long job du front q2 faisait
  tout q2. La préparation par masse et le grain 4× plus fin divisent q2 par
  3,3 à 4,5.
- **Meilleures chaînes : K5 2,54 / 3,21 / 3,52 s ; K10 7,68 / 10,36 /
  10,53 s** (contre 3,67 / 5,31 / 6,40 et 9,58 / 13,91 / 15,28 à R7b ce
  matin).
- À K5, q3/q4 pèse désormais 66 à 69 % de la chaîne, la tour 21 à 23 %. Il ne
  reste presque plus d'attente : réduire le travail q3/q4 ou changer de
  backend.
- Contrat (1 s puis 100 ms) non atteint : aucune qualification.

## Contenu

Même structure que R8–R10 : `vm/`, `host/` (expurgé ; ni `oslogin_add`, ni clé,
ni archive), `PACKAGE.json`, `SUMMARY.json`, `SHA256SUMS`.
