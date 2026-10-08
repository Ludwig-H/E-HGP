# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n1_sans_sol : 3.547 s par million ; boreas_202011261358_f4500_n1 : 3.244 s par million ; boreas_202011261358_f4500_n10_sans_sol : 6.624 s par million ; boreas_202011261358_f4500_n10 : 6.038 s par million ; ign_marseille_0891_6248_sans_sol : 4.444 s par million ; forinst_scion_plot61_sans_sol : 12.197 s par million ; eth3d_meadow_scan1 : 5.715 s par million ; forinst_scion_plot61 : 11.933 s par million ; ign_marseille_0891_6248 : 4.702 s par million ; forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : non tenu — forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget)
- B3 : non evalue
- B4 : non tenu — boreas_202011261358_f4500_n10_sans_sol (appareil) : refus (resource_exhausted/memory_budget) ; ign_marseille_0891_6248_sans_sol (appareil) : refus (resource_exhausted/memory_budget)

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 5 | appareil | ok | 2 | 0.56 | 0.52 | 3.547 | 10.0 | 1.1 | 1.6 | 1.8 | 2.4 | 8.3 | `f2ec1106e274` |
| `boreas_202011261358_f4500_n1` | 215 665 | 5 | appareil | ok | 2 | 0.74 | 0.70 | 3.244 | 13.5 | 1.5 | 2.0 | 2.2 | 2.8 | 7.5 | `52c5cf71ee01` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 9.98 | 10.02 | 6.624 | 205.2 | 15.9 | 16.3 | 21.5 | 22.0 | 11.3 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | cpu | ok | 2 | 22.23 | 22.34 | 14.764 | 806.5 | 22.8 | 23.1 | — | — | 16.2 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10` | 2 153 342 | 5 | appareil | ok | 2 | 13.09 | 13.00 | 6.038 | 260.1 | 20.2 | 20.6 | 28.5 | 29.1 | 10.1 | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 5 | appareil | ok | 2 | 11.09 | 10.96 | 4.444 | 211.4 | 15.9 | 16.2 | 23.0 | 23.6 | 6.9 | — |
| `forinst_scion_plot61_sans_sol` | 3 439 371 | 5 | appareil | ok | 2 | 41.83 | 41.95 | 12.197 | 978.7 | 58.2 | 58.4 | 80.7 | 81.3 | 18.2 | — |
| `forinst_tuwien_train_sans_sol` | 5 199 758 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 81.6 | — | — |
| `eth3d_meadow_scan1` | 6 181 091 | 5 | appareil | ok | 2 | 35.24 | 35.32 | 5.715 | 529.2 | 32.6 | 32.9 | 47.5 | 48.1 | 5.7 | — |
| `forinst_nibio_plot12_sans_sol` | 7 793 680 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 34.7 | — | — |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 268 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 59.6 | — | — |
| `boreas_202011261358_f4500_n50` | 10 766 998 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 37.0 | — | — |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 86.3 | — | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 26.1 | — | — |
| `forinst_scion_plot61` | 3 589 247 | 5 | appareil | ok | 1 | 42.83 | 42.83 (froide) | 11.933 | 990.5 | 59.4 | 59.5 | 81.2 | 81.8 | 17.8 | — |
| `forinst_tuwien_train` | 6 236 167 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 24.4 | — | — |
| `ign_marseille_0891_6248` | 6 709 045 | 5 | appareil | ok | 1 | 31.55 | 31.55 (froide) | 4.702 | 517.0 | 38.2 | 38.4 | 55.0 | 55.6 | 6.1 | — |
| `forinst_nibio_plot12` | 7 825 857 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 34.5 | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | G | raccord | TMVR |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.52 / 0.52 | 3.61 / 4.59 | 5.63 / 6.53 | 5.63 / 5.63 | 7.93 / 8.35 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.37 / 0.37 | 3.15 / 4.01 | 4.99 / 5.81 | 4.99 / 4.99 | 7.12 / 7.51 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.10 / 0.10 | 4.49 / 5.82 | 7.47 / 8.69 | 7.47 / 7.47 | 10.67 / 11.26 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.06 / 0.06 | 4.44 / 16.16 | 7.43 / 8.64 | 7.43 / 7.43 | 10.63 / 11.22 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.09 / 0.09 | 3.97 / 5.14 | 6.63 / 7.76 | 6.63 / 6.63 | 9.54 / 10.07 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.09 / 0.09 | 2.60 / 3.27 | 4.42 / 5.24 | 4.42 / 4.42 | 6.54 / 6.93 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.08 / 0.08 | 7.52 / 9.92 | 12.38 / 14.28 | 12.38 / 12.38 | 17.24 / 18.16 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.07 / 0.07 | 1.64 / 1.71 | 3.54 / 4.27 | 3.54 / 3.54 | 5.33 / 5.67 |
| `forinst_scion_plot61` | 5 | appareil | 0.06 / 0.06 | 7.35 / 9.69 | 12.11 / 13.96 | 12.11 / 12.11 | 16.87 / 17.77 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.06 / 0.06 | 2.08 / 2.46 | 3.76 / 4.54 | 3.76 / 3.76 | 5.74 / 6.11 |

Etages de la passe chaude (secondes) :

| Scene | K | voie | P | C | dont transferts | G | T | M | V | R | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.01 | 0.12 | 0.03 | 0.15 | 0.14 | 0.03 | 0.01 | 0.05 | 0.15 | 3.86 | 0.03 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.01 | 0.17 | 0.04 | 0.20 | 0.19 | 0.04 | 0.01 | 0.06 | 0.22 | 5.25 | 0.04 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 | 1.74 | 0.47 | 2.95 | 3.51 | 0.46 | 0.14 | 0.95 | 3.25 | 55.57 | 0.44 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.08 | 14.09 | 0.00 | 2.98 | 3.45 | 0.46 | 0.14 | 0.95 | 3.23 | 55.67 | 0.44 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.11 | 2.17 | 0.60 | 3.64 | 4.73 | 0.60 | 0.20 | 1.29 | 4.45 | 0.00 | 0.60 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.16 | 1.78 | 0.46 | 2.84 | 4.19 | 0.50 | 0.20 | 1.07 | 3.73 | 0.00 | 0.48 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.25 | 5.80 | 1.79 | 14.63 | 14.25 | 1.67 | 0.56 | 4.07 | 15.29 | 0.00 | 1.80 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.40 | 3.40 | 0.81 | 7.73 | 20.00 | 1.14 | 0.24 | 1.97 | 5.25 | 0.00 | 1.08 |
| `forinst_scion_plot61` | 5 | appareil | 0.26 | 5.96 | 1.83 | 14.73 | 14.68 | 1.70 | 0.58 | 4.17 | 15.66 | 0.00 | 1.80 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.44 | 4.03 | 1.04 | 6.15 | 15.39 | 1.34 | 0.61 | 2.98 | 14.02 | 0.00 | 1.17 |
