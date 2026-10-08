# T2-d-A : Session recouverte (D-F2), avant / apres (genere par pilote_t2d_a.py)

Verdict (REGLE_T2D_A) : **adopte** ; identite FUL1 : oui.

| trame | mur avant (ms) | mur apres (ms) | rapport (IC 95 %) |
| --- | ---: | ---: | --- |
| ng00 | 147.5 | 99.7 | 0.676 (0.670-0.684) |
| ng01 | 117.2 | 81.0 | 0.694 (0.688-0.699) |
| ng02 | 150.4 | 97.8 | 0.647 (0.636-0.659) |

Partition murale (mediane des passes chaudes, ms ; apres : G jusqu'a la fin du dernier calcul de G, TMVR = queue, foret non recouverte) :

| trame | bras | P | C | G | raccord | TMVR |
| --- | --- | --- | --- | --- | --- | --- |
| ng00 | avant | 1.9 | 28.4 | 56.7 | 0.0 | 60.6 |
| ng00 | apres | 2.2 | 28.4 | 62.0 | 0.0 | 5.5 |
| ng01 | avant | 1.7 | 24.6 | 42.4 | 0.0 | 48.4 |
| ng01 | apres | 1.9 | 24.7 | 48.2 | 0.0 | 5.2 |
| ng02 | avant | 1.9 | 28.5 | 51.4 | 0.0 | 68.5 |
| ng02 | apres | 1.9 | 28.5 | 52.0 | 0.0 | 13.7 |

T, M, V, R (ms) : avant, murs des etages sequentiels ; apres, SOMMES DE FENETRES MURALES des taches (temps-fils, ni murs ni temps CPU) :

| trame | avant : murs T / M / V / R | apres : fenetres T / M / V / R | apres : fenetres G / foret / foret apres G |
| --- | --- | --- | --- |
| ng00 | 28.9 / 11.2 / 2.2 / 14.2 | 140.7 / 72.8 / 72.7 / 203.5 | 1942.1 / 489.9 / 116.2 |
| ng01 | 22.8 / 8.3 / 2.0 / 11.9 | 109.3 / 65.3 / 58.6 / 165.1 | 1431.9 / 395.9 / 140.7 |
| ng02 | 34.1 / 12.1 / 2.3 / 15.4 | 164.1 / 93.8 / 77.7 / 216.4 | 1634.1 / 551.6 / 383.1 |

Ressources (publiees, non jugees) :

| trame | bras | CPU par passe (ms, mediane) | pic du budget (Mio, max) |
| --- | --- | ---: | ---: |
| ng00 | avant | 2895.0 | 1071.5 |
| ng00 | apres | 3015.5 | 1144.3 |
| ng01 | avant | 2189.4 | 893.4 |
| ng01 | apres | 2314.6 | 955.0 |
| ng02 | avant | 2675.2 | 1102.9 |
| ng02 | apres | 2786.8 | 1185.0 |

Recouvrement (apres, ms ; instants depuis le debut de build_tower) :

| trame | ouverture | fin du dernier calcul de G | queue | reprises du noyau | arrets du noyau | fins par ordre G/noyau/M/V/R |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| ng00 | 13.8 | 62.0 | 5.5 | 18 | 13 | 62.0/63.2/64.0/0.0/65.1 ; 61.8/62.7/64.2/65.4/66.3 ; 57.5/59.0/61.4/65.8/65.2 ; 50.0/52.6/56.5/63.7/62.5 ; 39.0/44.5/48.4/59.8/56.7 |
| ng01 | 12.5 | 48.2 | 5.2 | 18 | 13 | 48.1/49.2/50.0/0.0/51.2 ; 47.7/48.9/49.8/51.2/52.0 ; 43.6/46.2/48.4/51.6/51.5 ; 38.9/41.4/44.8/50.2/49.3 ; 30.9/35.8/39.2/47.3/45.2 |
| ng02 | 14.4 | 52.0 | 13.7 | 21 | 16 | 51.9/53.5/54.2/0.0/55.4 ; 51.9/54.3/56.1/57.0/60.3 ; 49.2/53.9/56.1/59.0/61.6 ; 44.1/49.2/52.2/59.0/59.0 ; 35.3/51.6/55.3/57.5/64.2 |

Session sur les 37 trames v12set (second tour, publie, non juge) :

- avant : mediane 230.8 ms, maximum 448.2 ms sur 37 trames.
- apres : mediane 160.6 ms, maximum 327.3 ms sur 37 trames.
