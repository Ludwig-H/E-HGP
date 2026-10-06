# Diagnostic de l'étage domain : lot de feuilles sur le Pool, sur le GPU, ou voie CPU

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261006.claudedom1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudedom1/receipt.json`). Source : `7f51c0f74`. Trames LiDAR réelles ng00, ng01, ng02, W48. **Session de
diagnostic, sans règle d'adoption** : elle dimensionne l'exécuteur partagé du lot de feuilles (`2045ec27c`).

Modes :
- `cpu` : 278523, sans lot de feuilles ;
- `host` : 311291 (278523 + 32768), lot sur le Pool de l'hôte ;
- `gpu` : 344059, lot sur le GPU, avec l'arithmétique étroite C.

Les trois bancs sont `conforme`, et toutes les prises rendent les vidages des empreintes des trames.

## Médianes à chaud (ms)

| K, feuilles | Trame | Mode | `domain` | Passe unique | Lot | dont comptage | Préfixe | Tri |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| K5, 24 | ng00 | cpu | 229,0 | 172,0 | — | — | 19,7 | 10,6 |
| | | host | 233,1 | 35,3 | 127,5 | 115,1 | 19,7 | 9,9 |
| | | gpu | 178,9 | 35,6 | 76,7 | 73,2 | 19,7 | 9,7 |
| K5, 24 | ng01 | cpu | 186,8 | 138,1 | — | — | 16,9 | 8,6 |
| | | host | 190,5 | 29,8 | 102,3 | 92,9 | 17,0 | 8,1 |
| | | gpu | 157,8 | 29,3 | 71,4 | 68,4 | 16,9 | 7,9 |
| K5, 24 | ng02 | cpu | 225,4 | 164,4 | — | — | 21,1 | 12,4 |
| | | host | 225,5 | 33,8 | 118,5 | 108,0 | 20,5 | 11,7 |
| | | gpu | 179,1 | 33,5 | 75,0 | 71,5 | 20,4 | 11,4 |
| K5, 16 | ng00 / ng01 / ng02 | gpu | 211,5 / 176,0 / 206,8 | 99,7 / 81,3 / 92,4 | 38,8 / 32,0 / 36,3 | 32,7 / 26,8 / 30,6 | ≈ 20 | ≈ 10 |
| K10, 24 | ng00 | host / gpu | 845,8 / 594,2 | 137,7 / 139,6 | 461,8 / 208,8 | 424,7 / 193,6 | 30 | 50 |
| K10, 24 | ng01 | host / gpu | 671,8 / 477,3 | 114,3 / 111,9 | 364,2 / 169,4 | 335,5 / 156,8 | 25 | 37 |
| K10, 24 | ng02 | host / gpu | 790,7 / 565,1 | 128,4 / 124,9 | 419,0 / 195,4 | 387,3 / 180,5 | 31 | 51 |

## Lecture

- **Le lot entier sur le Pool (W48) prend 1,4 à 1,7 fois le temps du GPU à K5, et 2,1 à 2,2 fois à K10** : 102 à
  128 ms contre 71 à 77 ms à K5 avec des feuilles de 24, et 364 à 462 ms contre 169 à 209 ms à K10. Pendant le lot
  GPU, le Pool attend sans rien faire. Un partage dosé du travail ramènerait le lot vers 1/(1/T_GPU + 1/T_Pool) :
  42 à 48 ms à K5 et 116 à 144 ms à K10. D'où l'exécuteur partagé.
- Avec des feuilles de 16, le lot GPU tombe à 32–39 ms, mais la passe unique monte à 81–100 ms (plus de nœuds
  internes sur le CPU) : l'étage y est plus long qu'avec des feuilles de 24.
- En voie GPU, il reste environ 60 ms de phases CPU hors du lot à K5 : préfixe 17 à 21, tri 8 à 12, niveaux 4 à 5, et
  environ 30 ms de reste (assemblage, compaction, balayage des niveaux).

## Pièces

`claudedom1/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_k5_16.json`, `gpu_ab_report_k5_24.json`,
`gpu_ab_report_k10_24.json` (sous-temps du domaine gardés par prise). Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
