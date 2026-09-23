# Reçu : session G4 R10, ablation appariée du recouvrement de la tour (sonde v15)

23 septembre 2026, 10:48–10:54 UTC (VM). Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`. **GCP utilisé** : session SPOT gardée sur la cible
fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48), génération
`2026-09-23T03:48:08.059-07:00`, arrêt ciblé certifié et `TERMINATED` relu
(dernier arrêt `2026-09-23T03:54:20.305-07:00`). GPU non utilisé.

Paquet `33d51efd` (sonde v15, plan v6, snapshot `8851fa76…`, manifeste
`b305a444…`, worker `0e2f3f53…`, contrôleur `cd6ff697…`, protocole au commit).
Reçus hôte et worker **`completed`**. Les 24 cas sont `complete_relative` ; les
18 comparaisons d'objet sont égales et Euler vaut « holds » partout. Seul
levier variant : `tower_overlap_static` (ON, défaut épinglé, ou OFF) ; tous les
autres leviers sont ON, dont l'ordonnancement q34 de R9. W48, tour statique
48, s = 8, deux répétitions entrelacées.

## Résultats (répétitions 0 / 1)

| trame | K | chaîne OFF (s) | chaîne ON (s) | tour OFF → ON (s) | lots OFF → ON (s) | phase 0 ON (s) |
| --- | ---: | --- | --- | --- | --- | --- |
| 000100 | 5 | 2,87 / 2,83 | **2,75 / 2,74** | 0,74 → 0,62 | 0,26 → 0,15 | 0,21 |
| 000000 | 5 | 3,73 / 3,77 | **3,65 / 3,64** | 0,89 → 0,78 | 0,33 → 0,22 | 0,24 |
| 000200 | 5 | 4,11 / 4,08 | **3,96 / 3,94** | 0,91 → 0,86 | 0,34 → 0,26 | 0,25 |
| 000100 | 10 | 8,65 / 8,63 | **8,19 / 8,07** | 2,95 → 2,49 | 0,80 → 0,27 | 1,19 |
| 000000 | 10 | 11,89 / 12,00 | **11,13 / 11,30** | 3,90 → 3,14 | 1,16 → 0,39 | 1,48 |
| 000200 | 10 | 12,04 / 12,15 | **11,36 / 11,50** | 3,71 → 2,98 | 1,04 → 0,36 | 1,38 |

`lots` ON est le reste de la phase A après la phase 0 (sonde v15). Condensés de
tour identiques entre ON et OFF, et à R7b–R9.

## Lecture

- La phase A de l'ordre le plus lent se recouvre presque entièrement avec la
  phase 0 des ordres inférieurs : tour K10 −0,46 à −0,76 s, K5 −0,05 à
  −0,12 s ; la phase 0 ralentit d'au plus 8 %.
- **Meilleures chaînes : K5 2,74 / 3,64 / 3,94 s, K10 8,07 / 11,13 /
  11,36 s**. Postes restants à K5 (000100) :
  - q34 : 1,65–1,69 s ;
  - tour : 0,62–0,66 s, dont validation 0,11 s et phase 0 0,21 s ;
  - q2 : 0,23–0,25 s.
- Contrat (1 s puis 100 ms) non atteint : aucune qualification.

## Contenu

Même structure que R8/R9 : `vm/`, `host/` (expurgé ; ni `oslogin_add`, ni clé,
ni archive), `PACKAGE.json`, `SUMMARY.json`, `SHA256SUMS`.
