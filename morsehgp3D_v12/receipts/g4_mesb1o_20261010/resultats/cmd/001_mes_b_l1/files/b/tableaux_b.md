# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n10_sans_sol : 3.232 s par million ; boreas_202011261358_f4500_n10 : 2.893 s par million ; ign_marseille_0891_6248_sans_sol : 2.097 s par million ; forinst_scion_plot61_sans_sol : 8.302 s par million ; eth3d_meadow_scan1 : 2.184 s par million ; forinst_scion_plot61 : 6.034 s par million ; forinst_tuwien_train : 5.791 s par million ; ign_marseille_0891_6248 : 2.017 s par million ; forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : non tenu — forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget)
- B3 : non evalue
- B4 : non tenu — boreas_202011261358_f4500_n10_sans_sol (appareil) : 23.953 s par million ; ign_marseille_0891_6248_sans_sol (appareil) : 14.741 s par million

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 5 | appareil | ok | 2 | 0.30 | 0.26 | 1.806 | 7.4 | 1.4 | 1.7 | 1.8 | 2.4 | 10.5 | `f2ec1106e274` |
| `boreas_202011261358_f4500_n1` | 215 665 | 5 | appareil | ok | 2 | 0.40 | 0.35 | 1.641 | 9.8 | 2.0 | 2.2 | 2.2 | 2.8 | 9.7 | `52c5cf71ee01` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 5.10 | 4.89 | 3.232 | 160.8 | 21.0 | 22.5 | 21.5 | 22.0 | 14.9 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10` | 2 153 342 | 5 | appareil | ok | 2 | 6.45 | 6.23 | 2.893 | 204.8 | 27.3 | 28.7 | 28.5 | 29.1 | 13.6 | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 5 | appareil | ok | 2 | 5.29 | 5.17 | 2.097 | 166.2 | 21.8 | 22.7 | 23.0 | 23.6 | 9.5 | — |
| `forinst_scion_plot61_sans_sol` | 3 439 371 | 5 | appareil | ok | 2 | 21.43 | 28.55 | 8.302 | 899.3 | 76.1 | 78.2 | 0.0 | 81.3 | 23.8 | — |
| `forinst_tuwien_train_sans_sol` | 5 199 758 | 5 | appareil | refus (resource_exhausted/memory_budget) | 1 | 24.82 | 24.82 (froide) | 4.773 | 859.8 | 78.6 | 81.1 | 80.0 | 87.6 | 16.2 | — |
| `eth3d_meadow_scan1` | 6 181 091 | 5 | appareil | ok | 2 | 13.92 | 13.50 | 2.184 | 478.3 | 45.1 | 48.2 | 43.6 | 48.1 | 7.8 | — |
| `forinst_nibio_plot12_sans_sol` | 7 793 680 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.3 | — | — |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 268 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.0 | — | — |
| `boreas_202011261358_f4500_n50` | 10 766 998 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.2 | — | — |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 10 | appareil | ok | 1 | 36.25 | 36.25 (froide) | 23.953 | 1299.0 | 102.7 | 105.1 | 0.0 | 45.8 | 72.8 | `04061b6d7557` |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 10 | appareil | ok | 1 | 36.34 | 36.34 (froide) | 14.741 | 1274.2 | 99.6 | 101.9 | 0.0 | 45.8 | 43.4 | — |
| `forinst_scion_plot61` | 3 589 247 | 5 | appareil | ok | 1 | 21.66 | 21.66 (froide) | 6.034 | 772.0 | 77.9 | 79.7 | 81.2 | 81.8 | 23.3 | — |
| `forinst_tuwien_train` | 6 236 167 | 5 | appareil | ok | 1 | 36.11 | 36.11 (froide) | 5.791 | 1142.0 | 90.8 | 92.0 | 0.1 | 45.5 | 15.6 | — |
| `ign_marseille_0891_6248` | 6 709 045 | 5 | appareil | ok | 1 | 13.54 | 13.54 (froide) | 2.017 | 436.7 | 53.2 | 54.1 | 55.0 | 55.6 | 8.5 | — |
| `forinst_nibio_plot12` | 7 825 857 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.3 | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.18 / 0.18 | 3.87 / 3.87 | 8.40 / 10.54 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.14 / 0.14 | 3.52 / 3.52 | 7.66 / 9.72 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 / 0.08 | 5.43 / 5.43 | 11.89 / 14.92 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.07 / 0.07 | 4.69 / 4.69 | 10.51 / 13.59 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.07 / 0.07 | 3.18 / 3.18 | 7.27 / 9.49 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.07 / 0.07 | 8.98 / 12.80 | 19.21 / 23.76 |
| `forinst_tuwien_train_sans_sol` | 5 | appareil | 0.06 / 0.06 | 5.96 / 5.96 | 13.07 / 16.24 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.07 / 0.07 | 2.04 / 2.04 | 5.88 / 7.84 |
| `boreas_202011261358_f4500_n10_sans_sol` | 10 | appareil | 0.06 / 0.06 | 25.98 / 38.27 | 55.70 / 72.83 |
| `ign_marseille_0891_6248_sans_sol` | 10 | appareil | 0.06 / 0.06 | 15.04 / 22.83 | 32.79 / 43.40 |
| `forinst_scion_plot61` | 5 | appareil | 0.06 / 0.06 | 8.88 / 8.88 | 18.83 / 23.30 |
| `forinst_tuwien_train` | 5 | appareil | 0.06 / 0.06 | 5.92 / 8.40 | 12.57 / 15.64 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.06 / 0.06 | 2.62 / 2.62 | 6.47 / 8.52 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.01 | 0.10 | 0.01 | 0.13 | 0.03 | 0.15 | 1.21 | 0.00 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.01 | 0.12 | 0.01 | 0.17 | 0.04 | 0.22 | 1.65 | 0.00 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.09 | 1.37 | 0.11 | 2.65 | 0.79 | 3.16 | 17.79 | 0.37 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.12 | 1.67 | 0.14 | 3.33 | 1.10 | 4.41 | 0.00 | 0.54 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.14 | 1.41 | 0.12 | 2.63 | 0.99 | 3.75 | 0.00 | 0.43 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.22 | 11.81 | 2.35 | 12.94 | 3.33 | 15.20 | 0.00 | 1.86 |
| `forinst_tuwien_train_sans_sol` | 5 | appareil | 0.35 | 5.54 | 0.44 | 14.88 | 3.75 | 15.85 | 0.00 | 1.88 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.36 | 2.81 | 0.24 | 8.24 | 1.93 | 5.19 | 0.00 | 1.09 |
| `boreas_202011261358_f4500_n10_sans_sol` | 10 | appareil | 0.09 | 13.13 | 3.01 | 21.54 | 0.99 | 16.93 | 107.50 | 2.24 |
| `ign_marseille_0891_6248_sans_sol` | 10 | appareil | 0.16 | 13.40 | 2.94 | 20.90 | 1.42 | 18.38 | 0.00 | 2.25 |
| `forinst_scion_plot61` | 5 | appareil | 0.26 | 4.59 | 0.46 | 13.30 | 3.22 | 15.93 | 0.00 | 1.99 |
| `forinst_tuwien_train` | 5 | appareil | 0.42 | 14.75 | 2.61 | 16.63 | 3.96 | 19.57 | 0.00 | 2.18 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.45 | 3.27 | 0.29 | 6.67 | 2.96 | 14.04 | 0.00 | 1.22 |
