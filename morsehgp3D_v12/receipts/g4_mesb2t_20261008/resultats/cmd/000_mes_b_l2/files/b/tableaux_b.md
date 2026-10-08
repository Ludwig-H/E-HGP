# MES-B : scenes LiDAR reelles entieres, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- B1 : non tenu — ign_paris_0651_6863_sans_sol : 5.098 s par million ; ign_paris_0651_6863 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget) ; ign_lyon_0842_6521_sans_sol : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget) ; ign_lyon_0842_6521 : refus tolere au-dela de 10 millions de sites (resource_exhausted/memory_budget)
- B2 : tenu — 1 scenes jouees sous 10 millions de sites
- B3 : non evalue
- B4 : non evalue

| Scene | sites | K | voie | etat | passes | mur froid (s) | mur chaud (s) | s par million | CPU·s chaud | budget hote (Gio) | RSS max (Gio) | appareil garde (Gio) | pic appareil (Gio) | Ko par site | FUL1 |
| --- | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `ign_paris_0651_6863_sans_sol` | 9 111 422 | 5 | appareil | ok | 1 | 46.45 | 46.45 (froide) | 5.098 | 1036.2 | 82.1 | 83.8 | 0.1 | 45.6 | 9.7 | — |
| `ign_paris_0651_6863` | 14 551 520 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 46.9 | — | — |
| `ign_lyon_0842_6521_sans_sol` | 24 016 862 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 65.8 | — | — |
| `ign_lyon_0842_6521` | 32 412 887 | 5 | appareil | refus (resource_exhausted/memory_budget) | 0 | — | — | — | — | — | — | — | 71.1 | — | — |

Memoire du budget de l'hote par etage, passe chaude (Ko par site : en usage a la fin de l'etage / pic pendant l'etage) :

| Scene | K | voie | P | C | tour |
| --- | ---: | --- | ---: | ---: | ---: |
| `ign_paris_0651_6863_sans_sol` | 5 | appareil | 0.06 / 0.06 | 3.06 / 4.70 | 7.60 / 9.67 |

Etages de la passe chaude (secondes ; G jusqu'au dernier calcul de G, queue = foret non recouverte) :

| Scene | K | voie | P | C | dont transferts | G | queue | validation | empreinte | liberation |
| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `ign_paris_0651_6863_sans_sol` | 5 | appareil | 0.66 | 13.85 | 2.43 | 14.26 | 17.37 | 20.86 | 0.00 | 1.95 |
