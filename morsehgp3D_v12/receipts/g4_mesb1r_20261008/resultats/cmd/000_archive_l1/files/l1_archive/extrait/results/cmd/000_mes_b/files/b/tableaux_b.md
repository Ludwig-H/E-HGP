# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n1_sans_sol : 3.629 s par million ; boreas_202011261358_f4500_n1 : 3.356 s par million ; boreas_202011261358_f4500_n10_sans_sol : 6.651 s par million ; boreas_202011261358_f4500_n10 : 6.075 s par million ; ign_marseille_0891_6248_sans_sol : 4.513 s par million ; forinst_scion_plot61_sans_sol : 12.218 s par million ; eth3d_meadow_scan1 : 5.719 s par million ; forinst_scion_plot61 : 12.006 s par million ; ign_marseille_0891_6248 : 4.753 s par million ; forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : non tenu — forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget)
- B3 : non evalue
- B4 : non tenu — boreas_202011261358_f4500_n10_sans_sol (appareil) : refus (resource_exhausted/memory_budget) ; ign_marseille_0891_6248_sans_sol (appareil) : refus (resource_exhausted/memory_budget)

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 5 | appareil | ok | 2 | 0.56 | 0.53 | 3.629 | 10.0 | 1.1 | 1.6 | 1.8 | 2.4 | 8.3 | `f2ec1106e274` |
| `boreas_202011261358_f4500_n1` | 215 665 | 5 | appareil | ok | 2 | 0.76 | 0.72 | 3.356 | 13.5 | 1.5 | 2.0 | 2.2 | 2.8 | 7.5 | `52c5cf71ee01` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 10.19 | 10.07 | 6.651 | 206.3 | 15.9 | 16.3 | 21.5 | 22.0 | 11.3 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | cpu | ok | 2 | 22.45 | 22.41 | 14.804 | 808.4 | 22.8 | 23.1 | — | — | 16.2 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10` | 2 153 342 | 5 | appareil | ok | 2 | 13.16 | 13.08 | 6.075 | 260.6 | 20.2 | 20.6 | 28.5 | 29.1 | 10.1 | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 5 | appareil | ok | 2 | 11.21 | 11.13 | 4.513 | 213.2 | 15.9 | 16.2 | 23.0 | 23.6 | 6.9 | — |
| `forinst_scion_plot61_sans_sol` | 3 439 371 | 5 | appareil | ok | 2 | 42.11 | 42.02 | 12.218 | 976.7 | 58.2 | 58.4 | 80.7 | 81.3 | 18.2 | — |
| `forinst_tuwien_train_sans_sol` | 5 199 758 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 81.6 | — | — |
| `eth3d_meadow_scan1` | 6 181 091 | 5 | appareil | ok | 2 | 35.09 | 35.35 | 5.719 | 529.9 | 32.6 | 33.0 | 47.5 | 48.1 | 5.7 | — |
| `forinst_nibio_plot12_sans_sol` | 7 793 680 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 34.7 | — | — |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 268 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 71.6 | — | — |
| `boreas_202011261358_f4500_n50` | 10 766 998 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 37.0 | — | — |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 86.3 | — | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 26.1 | — | — |
| `forinst_scion_plot61` | 3 589 247 | 5 | appareil | ok | 1 | 43.09 | 43.09 (froide) | 12.006 | 994.2 | 59.4 | 59.5 | 81.2 | 81.8 | 17.8 | — |
| `forinst_tuwien_train` | 6 236 167 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 24.4 | — | — |
| `ign_marseille_0891_6248` | 6 709 045 | 5 | appareil | ok | 1 | 31.89 | 31.89 (froide) | 4.753 | 522.2 | 38.2 | 38.4 | 55.0 | 55.6 | 6.1 | — |
| `forinst_nibio_plot12` | 7 825 857 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 34.5 | — | — |

Etages de la passe chaude (secondes) :

| Scene | K | voie | P | C | dont transferts | G | T | M | V | R | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.01 | 0.12 | 0.03 | 0.15 | 0.15 | 0.03 | 0.01 | 0.05 | 0.15 | 3.87 | 0.03 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.01 | 0.17 | 0.04 | 0.20 | 0.21 | 0.04 | 0.01 | 0.06 | 0.22 | 5.29 | 0.04 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 | 1.72 | 0.47 | 2.97 | 3.54 | 0.46 | 0.14 | 0.95 | 3.28 | 56.01 | 0.45 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.08 | 14.11 | 0.00 | 3.01 | 3.47 | 0.46 | 0.14 | 0.95 | 3.21 | 56.80 | 0.44 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.12 | 2.17 | 0.59 | 3.64 | 4.78 | 0.62 | 0.20 | 1.30 | 4.46 | 0.00 | 0.61 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.16 | 1.78 | 0.46 | 2.87 | 4.34 | 0.51 | 0.20 | 1.06 | 3.77 | 0.00 | 0.48 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.25 | 5.79 | 1.79 | 14.57 | 14.41 | 1.66 | 0.56 | 4.06 | 15.16 | 0.00 | 1.78 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.40 | 3.39 | 0.81 | 7.74 | 20.03 | 1.15 | 0.24 | 1.96 | 5.24 | 0.00 | 1.08 |
| `forinst_scion_plot61` | 5 | appareil | 0.27 | 6.00 | 1.83 | 14.72 | 14.83 | 1.74 | 0.59 | 4.20 | 16.05 | 0.00 | 1.84 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.44 | 4.05 | 1.04 | 6.23 | 15.61 | 1.36 | 0.61 | 2.97 | 14.21 | 0.00 | 1.18 |
