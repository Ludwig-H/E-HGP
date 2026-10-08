# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n1_sans_sol : 2.088 s par million ; boreas_202011261358_f4500_n1 : 2.035 s par million ; boreas_202011261358_f4500_n10_sans_sol : 4.747 s par million ; boreas_202011261358_f4500_n10 : 4.099 s par million ; ign_marseille_0891_6248_sans_sol : 3.284 s par million ; forinst_scion_plot61_sans_sol : 10.531 s par million ; eth3d_meadow_scan1 : 4.792 s par million ; forinst_scion_plot61 : 8.461 s par million ; forinst_tuwien_train : 7.303 s par million ; ign_marseille_0891_6248 : 3.738 s par million ; forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : non tenu — forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget)
- B3 : non evalue
- B4 : non tenu — boreas_202011261358_f4500_n10_sans_sol (appareil) : 25.518 s par million ; ign_marseille_0891_6248_sans_sol (appareil) : 15.626 s par million

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 5 | appareil | ok | 2 | 0.35 | 0.31 | 2.088 | 7.5 | 1.4 | 1.5 | 1.8 | 2.4 | 10.0 | `f2ec1106e274` |
| `boreas_202011261358_f4500_n1` | 215 665 | 5 | appareil | ok | 2 | 0.50 | 0.44 | 2.035 | 10.2 | 1.8 | 2.1 | 2.2 | 2.8 | 9.2 | `52c5cf71ee01` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 7.12 | 7.18 | 4.747 | 170.0 | 19.8 | 21.1 | 21.5 | 22.0 | 14.1 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10` | 2 153 342 | 5 | appareil | ok | 2 | 9.29 | 8.83 | 4.099 | 213.6 | 25.2 | 26.5 | 28.5 | 29.1 | 12.6 | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 5 | appareil | ok | 2 | 8.16 | 8.09 | 3.284 | 172.2 | 20.2 | 21.2 | 23.0 | 23.6 | 8.8 | — |
| `forinst_scion_plot61_sans_sol` | 3 439 371 | 5 | appareil | ok | 2 | 29.32 | 36.22 | 10.531 | 937.7 | 71.7 | 74.4 | 0.0 | 81.3 | 22.4 | — |
| `forinst_tuwien_train_sans_sol` | 5 199 758 | 5 | appareil | refus (resource_exhausted/memory_budget) | 1 | 32.86 | 32.86 (froide) | 6.319 | 922.2 | 74.6 | 76.6 | 80.0 | 87.6 | 15.4 | — |
| `eth3d_meadow_scan1` | 6 181 091 | 5 | appareil | ok | 2 | 30.22 | 29.62 | 4.792 | 512.1 | 41.7 | 44.8 | 43.6 | 48.1 | 7.2 | — |
| `forinst_nibio_plot12_sans_sol` | 7 793 680 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.3 | — | — |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 268 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.0 | — | — |
| `boreas_202011261358_f4500_n50` | 10 766 998 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 49.3 | — | — |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 10 | appareil | ok | 1 | 38.62 | 38.62 (froide) | 25.518 | 1397.5 | 98.4 | 101.4 | 0.0 | 45.8 | 69.8 | `04061b6d7557` |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 10 | appareil | ok | 1 | 38.52 | 38.52 (froide) | 15.626 | 1365.7 | 94.9 | 97.8 | 0.0 | 45.8 | 41.3 | — |
| `forinst_scion_plot61` | 3 589 247 | 5 | appareil | ok | 1 | 30.37 | 30.37 (froide) | 8.461 | 832.4 | 73.4 | 75.8 | 81.2 | 81.8 | 22.0 | — |
| `forinst_tuwien_train` | 6 236 167 | 5 | appareil | ok | 1 | 45.54 | 45.54 (froide) | 7.303 | 1178.2 | 85.8 | 88.1 | 0.1 | 45.5 | 14.8 | — |
| `ign_marseille_0891_6248` | 6 709 045 | 5 | appareil | ok | 1 | 25.08 | 25.08 (froide) | 3.738 | 441.7 | 49.2 | 50.6 | 55.0 | 55.6 | 7.9 | — |
| `forinst_nibio_plot12` | 7 825 857 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.3 | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.18 / 0.18 | 3.41 / 3.41 | 7.94 / 10.04 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.14 / 0.14 | 3.08 / 3.08 | 7.22 / 9.17 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 / 0.08 | 4.72 / 4.72 | 11.18 / 14.06 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.07 / 0.07 | 4.09 / 4.09 | 9.92 / 12.57 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.07 / 0.07 | 2.74 / 2.74 | 6.83 / 8.79 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.07 / 0.07 | 7.84 / 11.65 | 18.06 / 22.37 |
| `forinst_tuwien_train_sans_sol` | 5 | appareil | 0.06 / 0.06 | 5.20 / 5.20 | 12.31 / 15.40 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.07 / 0.07 | 1.66 / 1.66 | 5.50 / 7.25 |
| `boreas_202011261358_f4500_n10_sans_sol` | 10 | appareil | 0.06 / 0.06 | 23.14 / 35.43 | 52.86 / 69.80 |
| `ign_marseille_0891_6248_sans_sol` | 10 | appareil | 0.06 / 0.06 | 13.30 / 21.09 | 31.05 / 41.35 |
| `forinst_scion_plot61` | 5 | appareil | 0.06 / 0.06 | 7.78 / 7.78 | 17.73 / 21.96 |
| `forinst_tuwien_train` | 5 | appareil | 0.06 / 0.06 | 5.16 / 7.65 | 11.82 / 14.78 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.06 / 0.06 | 2.21 / 2.21 | 6.06 / 7.87 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.01 | 0.09 | 0.01 | 0.13 | 0.08 | 0.14 | 1.34 | 0.00 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.01 | 0.12 | 0.01 | 0.18 | 0.13 | 0.22 | 1.64 | 0.00 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.09 | 1.31 | 0.08 | 2.76 | 3.03 | 3.18 | 18.08 | 0.34 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.12 | 1.65 | 0.13 | 3.42 | 3.64 | 4.34 | 0.00 | 0.50 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.14 | 1.39 | 0.10 | 2.59 | 3.98 | 3.70 | 0.00 | 0.36 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.22 | 11.17 | 2.36 | 14.20 | 10.38 | 15.40 | 0.00 | 1.72 |
| `forinst_tuwien_train_sans_sol` | 5 | appareil | 0.35 | 5.46 | 0.38 | 16.37 | 10.43 | 15.70 | 0.00 | 1.82 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.35 | 2.79 | 0.19 | 7.47 | 18.87 | 5.20 | 0.00 | 0.94 |
| `boreas_202011261358_f4500_n10_sans_sol` | 10 | appareil | 0.09 | 12.40 | 2.97 | 24.51 | 1.15 | 16.89 | 105.98 | 2.16 |
| `ign_marseille_0891_6248_sans_sol` | 10 | appareil | 0.15 | 12.54 | 2.83 | 23.70 | 1.67 | 18.04 | 0.00 | 2.08 |
| `forinst_scion_plot61` | 5 | appareil | 0.26 | 4.45 | 0.37 | 14.59 | 10.81 | 15.77 | 0.00 | 1.75 |
| `forinst_tuwien_train` | 5 | appareil | 0.42 | 13.88 | 2.56 | 17.96 | 12.94 | 19.50 | 0.00 | 2.05 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.43 | 3.20 | 0.24 | 6.10 | 15.21 | 14.09 | 0.00 | 1.10 |
