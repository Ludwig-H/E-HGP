# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n10_sans_sol : 5.418 s par million
- B2 : tenu — 1 scenes jouees sous 10 millions de sites
- B3 : non evalue
- B4 : non evalue

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 8.47 | 8.20 | 5.418 | 196.8 | 19.9 | 24.9 | 0.0 | 5.0 | 14.1 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | cpu | ok | 1 | 19.48 | 19.48 (froide) | 12.873 | 777.5 | 24.2 | 26.8 | — | — | 17.1 | `49f90d23ad06` |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.11 / 0.11 | 4.75 / 6.86 | 11.21 / 14.09 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.06 / 0.06 | 4.71 / 17.14 | 11.17 / 14.05 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.09 | 2.57 | 0.55 | 2.77 | 2.75 | 3.24 | 17.97 | 0.35 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.09 | 13.69 | 0.00 | 2.79 | 2.81 | 3.17 | 18.00 | 0.42 |
