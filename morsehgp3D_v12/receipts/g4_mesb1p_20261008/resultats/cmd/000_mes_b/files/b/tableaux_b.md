# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n1_sans_sol : 2.186 s par million ; boreas_202011261358_f4500_n1 : 2.087 s par million ; boreas_202011261358_f4500_n10_sans_sol : 4.822 s par million ; boreas_202011261358_f4500_n10 : 4.362 s par million ; ign_marseille_0891_6248_sans_sol : 3.387 s par million ; forinst_scion_plot61_sans_sol : 9.008 s par million ; eth3d_meadow_scan1 : 4.945 s par million ; forinst_scion_plot61 : 8.865 s par million ; ign_marseille_0891_6248 : 3.965 s par million ; forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : non tenu — forinst_tuwien_train_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12_sans_sol : refus (resource_exhausted/memory_budget) ; boreas_202011261358_f4500_n50_sans_sol : refus (resource_exhausted/memory_budget) ; forinst_tuwien_train : refus (resource_exhausted/memory_budget) ; forinst_nibio_plot12 : refus (resource_exhausted/memory_budget)
- B3 : non evalue
- B4 : non tenu — boreas_202011261358_f4500_n10_sans_sol (appareil) : refus (resource_exhausted/memory_budget) ; ign_marseille_0891_6248_sans_sol (appareil) : refus (resource_exhausted/memory_budget)

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 5 | appareil | ok | 2 | 0.37 | 0.32 | 2.186 | 8.4 | 1.4 | 1.5 | 1.8 | 2.4 | 10.1 | `f2ec1106e274` |
| `boreas_202011261358_f4500_n1` | 215 665 | 5 | appareil | ok | 2 | 0.51 | 0.45 | 2.087 | 11.5 | 1.9 | 2.1 | 2.2 | 2.8 | 9.2 | `52c5cf71ee01` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 7.47 | 7.30 | 4.822 | 196.3 | 19.9 | 21.1 | 21.5 | 22.0 | 14.1 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | cpu | ok | 2 | 20.11 | 20.06 | 13.252 | 809.9 | 24.2 | 28.0 | — | — | 17.1 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10` | 2 153 342 | 5 | appareil | ok | 2 | 9.65 | 9.39 | 4.362 | 248.2 | 25.4 | 26.9 | 28.5 | 29.1 | 12.6 | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 5 | appareil | ok | 2 | 8.53 | 8.35 | 3.387 | 201.0 | 20.2 | 21.2 | 23.0 | 23.6 | 8.8 | — |
| `forinst_scion_plot61_sans_sol` | 3 439 371 | 5 | appareil | ok | 2 | 31.11 | 30.98 | 9.008 | 946.1 | 72.1 | 74.9 | 80.7 | 81.3 | 22.5 | — |
| `forinst_tuwien_train_sans_sol` | 5 199 758 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 81.6 | — | — |
| `eth3d_meadow_scan1` | 6 181 091 | 5 | appareil | ok | 2 | 30.72 | 30.56 | 4.945 | 539.4 | 41.9 | 44.7 | 47.5 | 48.1 | 7.3 | — |
| `forinst_nibio_plot12_sans_sol` | 7 793 680 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 85.8 | — | — |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 268 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 73.3 | — | — |
| `boreas_202011261358_f4500_n50` | 10 766 998 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 87.5 | — | — |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 86.3 | — | — |
| `ign_marseille_0891_6248_sans_sol` | 2 465 285 | 10 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 82.0 | — | — |
| `forinst_scion_plot61` | 3 589 247 | 5 | appareil | ok | 1 | 31.82 | 31.82 (froide) | 8.865 | 967.8 | 73.8 | 75.9 | 81.2 | 81.8 | 22.1 | — |
| `forinst_tuwien_train` | 6 236 167 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 85.7 | — | — |
| `ign_marseille_0891_6248` | 6 709 045 | 5 | appareil | ok | 1 | 26.60 | 26.60 (froide) | 3.965 | 525.4 | 49.5 | 50.6 | 55.0 | 55.6 | 7.9 | — |
| `forinst_nibio_plot12` | 7 825 857 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 85.9 | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.18 / 0.18 | 3.41 / 3.41 | 7.94 / 10.09 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.14 / 0.14 | 3.08 / 3.08 | 7.22 / 9.22 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 / 0.08 | 4.72 / 4.72 | 11.18 / 14.14 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.06 / 0.06 | 4.71 / 17.14 | 11.17 / 14.13 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.07 / 0.07 | 4.09 / 4.09 | 9.92 / 12.64 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.07 / 0.07 | 2.74 / 2.74 | 6.83 / 8.80 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.07 / 0.07 | 7.82 / 7.82 | 18.05 / 22.50 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.07 / 0.07 | 1.66 / 1.66 | 5.50 / 7.28 |
| `forinst_scion_plot61` | 5 | appareil | 0.06 / 0.06 | 7.78 / 7.78 | 17.73 / 22.07 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.06 / 0.06 | 2.21 / 2.21 | 6.06 / 7.92 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n1_sans_sol` | 5 | appareil | 0.01 | 0.09 | 0.01 | 0.14 | 0.08 | 0.15 | 1.23 | 0.00 |
| `boreas_202011261358_f4500_n1` | 5 | appareil | 0.01 | 0.12 | 0.01 | 0.19 | 0.13 | 0.22 | 1.65 | 0.00 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.09 | 1.33 | 0.09 | 2.93 | 2.95 | 3.23 | 18.03 | 0.36 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.08 | 13.96 | 0.00 | 2.94 | 3.02 | 3.24 | 18.03 | 0.45 |
| `boreas_202011261358_f4500_n10` | 5 | appareil | 0.12 | 1.65 | 0.11 | 3.58 | 4.04 | 4.38 | 0.00 | 0.53 |
| `ign_marseille_0891_6248_sans_sol` | 5 | appareil | 0.15 | 1.40 | 0.10 | 2.72 | 4.08 | 3.67 | 0.00 | 0.36 |
| `forinst_scion_plot61_sans_sol` | 5 | appareil | 0.22 | 4.20 | 0.32 | 14.89 | 11.43 | 15.20 | 0.00 | 1.69 |
| `eth3d_meadow_scan1` | 5 | appareil | 0.36 | 2.72 | 0.19 | 7.57 | 19.78 | 5.36 | 0.00 | 0.96 |
| `forinst_scion_plot61` | 5 | appareil | 0.26 | 4.34 | 0.35 | 15.19 | 11.77 | 15.53 | 0.00 | 1.73 |
| `ign_marseille_0891_6248` | 5 | appareil | 0.43 | 3.15 | 0.25 | 6.25 | 16.62 | 14.20 | 0.00 | 1.10 |
