# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — boreas_202011261358_f4500_n10_sans_sol : 4.149 s par million
- B2 : tenu — 1 scenes jouees sous 10 millions de sites
- B3 : non evalue
- B4 : non evalue

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | appareil | ok | 2 | 6.46 | 6.28 | 4.149 | 194.4 | 21.7 | 26.4 | 0.0 | 5.0 | 15.4 | `49f90d23ad06` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 483 | 5 | cpu | ok | 1 | 17.63 | 17.63 (froide) | 11.647 | 776.3 | 25.2 | 27.7 | — | — | 17.9 | `49f90d23ad06` |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.11 / 0.11 | 5.46 / 7.57 | 11.92 / 15.37 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.06 / 0.06 | 5.42 / 17.85 | 11.88 / 15.22 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | appareil | 0.08 | 2.73 | 0.54 | 2.66 | 0.77 | 3.22 | 18.01 | 0.39 |
| `boreas_202011261358_f4500_n10_sans_sol` | 5 | cpu | 0.09 | 13.87 | 0.00 | 2.69 | 0.86 | 3.23 | 18.12 | 0.46 |
