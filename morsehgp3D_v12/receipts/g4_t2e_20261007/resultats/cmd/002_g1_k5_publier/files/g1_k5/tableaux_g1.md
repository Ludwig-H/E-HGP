# MES-G1, seconde moitie : quatrieme bras replique_v12_saut

Tables tirees de `rapport_g1.json` (aucune valeur recopiee a la main). Verdict : **rejete**.

Rejets : ng00_k5 : borne haute 1.036 >= 1 ; ng01_k5 : borne haute 1.043 >= 1 ; ng02_k5 : borne haute 1.036 >= 1

| Cas | Prises | Rapport saut / v12 (moyenne geometrique) | IC 95 % | Censuses satures evites | Foret |
| --- | ---: | ---: | --- | ---: | --- |
| ng00_k5 | 5 | 1.032 | [1.030 ; 1.036] | 81.1 % | identique |
| ng01_k5 | 5 | 1.041 | [1.039 ; 1.043] | 81.5 % | identique |
| ng02_k5 | 5 | 1.035 | [1.033 ; 1.036] | 82.5 % | identique |

## ng00_k5 (campagne c20261007T184544Z_524695, 5 prises valides, minimum de 3 passe(s) par processus)

| k | v11 (s) | replique v11 (s) | replique v12 (s) | replique v12 saut (s) | satures v12 | satures saut | complets v12 | complets saut | sauts certifies / tentatives | tests par tentative | pas v12 | pas saut | chaine max v12 / saut | graines differentes | foret |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- | ---: | --- |
| 2 | 0.140 | 0.080 | 0.077 | 0.076 | 2515 | 631 | 0 | 0 | 2000 / 2631 | 7.4 | 475662 | 475810 | 5 / 6 | 985 | identique |
| 3 | 0.322 | 0.199 | 0.202 | 0.199 | 14000 | 2731 | 0 | 0 | 11855 / 14586 | 9.3 | 861202 | 861860 | 8 / 7 | 4494 | identique |
| 4 | 0.583 | 0.385 | 0.380 | 0.376 | 57239 | 11402 | 435 | 440 | 47280 / 59122 | 11.7 | 1338895 | 1340837 | 11 / 9 | 11307 | identique |
| 5 | 1.080 | 0.792 | 0.757 | 0.808 | 150419 | 27550 | 66906 | 66954 | 125275 / 219779 | 14.9 | 1919207 | 1922221 | 11 / 11 | 19838 | identique |

Temps : mediane des prises du minimum de R passes par processus (informatif ; le juge porte sur les rapports par processus).

## ng01_k5 (campagne c20261007T184544Z_524695, 5 prises valides, minimum de 3 passe(s) par processus)

| k | v11 (s) | replique v11 (s) | replique v12 (s) | replique v12 saut (s) | satures v12 | satures saut | complets v12 | complets saut | sauts certifies / tentatives | tests par tentative | pas v12 | pas saut | chaine max v12 / saut | graines differentes | foret |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- | ---: | --- |
| 2 | 0.112 | 0.061 | 0.056 | 0.056 | 2080 | 550 | 0 | 0 | 1664 / 2214 | 7.5 | 403029 | 403168 | 6 / 6 | 833 | identique |
| 3 | 0.253 | 0.154 | 0.156 | 0.155 | 11078 | 2203 | 0 | 0 | 9418 / 11621 | 9.2 | 714492 | 715276 | 6 / 8 | 3659 | identique |
| 4 | 0.444 | 0.288 | 0.285 | 0.284 | 43008 | 8344 | 242 | 253 | 35976 / 44573 | 11.5 | 1086978 | 1088524 | 11 / 11 | 9017 | identique |
| 5 | 0.796 | 0.576 | 0.548 | 0.592 | 112877 | 20123 | 48119 | 48142 | 94988 / 163253 | 14.6 | 1541599 | 1544211 | 12 / 13 | 16028 | identique |

Temps : mediane des prises du minimum de R passes par processus (informatif ; le juge porte sur les rapports par processus).

## ng02_k5 (campagne c20261007T184544Z_524695, 5 prises valides, minimum de 3 passe(s) par processus)

| k | v11 (s) | replique v11 (s) | replique v12 (s) | replique v12 saut (s) | satures v12 | satures saut | complets v12 | complets saut | sauts certifies / tentatives | tests par tentative | pas v12 | pas saut | chaine max v12 / saut | graines differentes | foret |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | --- | ---: | --- |
| 2 | 0.142 | 0.083 | 0.084 | 0.084 | 1943 | 534 | 0 | 0 | 1555 / 2089 | 7.5 | 510995 | 511148 | 5 / 5 | 828 | identique |
| 3 | 0.316 | 0.192 | 0.195 | 0.194 | 10172 | 1923 | 0 | 0 | 8804 / 10727 | 9.1 | 902072 | 902735 | 8 / 9 | 3498 | identique |
| 4 | 0.539 | 0.345 | 0.341 | 0.340 | 39012 | 7315 | 251 | 261 | 32963 / 40539 | 11.5 | 1344953 | 1346378 | 10 / 10 | 8314 | identique |
| 5 | 0.939 | 0.666 | 0.630 | 0.677 | 105580 | 17654 | 51148 | 51293 | 90291 / 159238 | 14.7 | 1893266 | 1895676 | 10 / 11 | 14351 | identique |

Temps : mediane des prises du minimum de R passes par processus (informatif ; le juge porte sur les rapports par processus).

