# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non evalue
- B2 : non evalue
- B3 : non evalue
- B4 : non evalue

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `ign_paris_0651_6863_sans_sol` | 9 111 422 | 5 | cpu | ok | 1 | 112.87 | 112.87 (froide) | 12.387 | 4020.5 | 93.1 | 93.2 | — | — | 11.0 | — |
| `ign_paris_0651_6863` | 14 551 520 | 5 | cpu | ok | 1 | 165.79 | 165.79 (froide) | 11.393 | 5674.7 | 137.0 | 138.1 | — | — | 10.1 | — |
| `eth3d_courtyard_scan1` | 16 828 368 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | 0 | — | — | — | — | — | — | — | — | — | — |
| `ign_lyon_0842_6521_sans_sol` | 24 016 862 | 5 | cpu | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | — | — | — |
| `ign_lyon_0842_6521` | 32 412 887 | 5 | cpu | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | — | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | G | raccord | TMVR |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `ign_paris_0651_6863_sans_sol` | 5 | cpu | 0.06 / 0.06 | 2.94 / 10.97 | 4.97 / 5.89 | 4.97 / 4.97 | 7.26 / 7.69 |
| `ign_paris_0651_6863` | 5 | cpu | 0.06 / 0.06 | 2.63 / 10.11 | 4.54 / 5.40 | 4.54 / 4.54 | 6.73 / 7.13 |

Etages de la passe chaude (secondes) :

| Scene | K | voie | P | C | dont transferts | G | T | M | V | R | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `ign_paris_0651_6863_sans_sol` | 5 | cpu | 0.65 | 67.68 | 0.00 | 14.90 | 20.30 | 2.12 | 1.21 | 5.13 | 20.88 | 0.00 | 2.00 |
| `ign_paris_0651_6863` | 5 | cpu | 1.03 | 94.21 | 0.00 | 20.97 | 35.08 | 3.35 | 1.88 | 7.89 | 38.97 | 0.00 | 3.05 |
