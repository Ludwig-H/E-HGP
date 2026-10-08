# T2-d-C : transferts et publication du catalogue sur G4

Verdict de REGLE_T2D_C : **adopte**.


Mutant appareil flux_sans_attente_appareil : tue (empreinte).

## Leviers (moyenne geometrique des rapports par tour, IC 95 % ; decision sur les bornes non arrondies)

| levier | de | vers | ng00 | ng01 | ng02 | verdict |
| --- | --- | --- | ---: | ---: | ---: | --- |
| lot | avant | apres | 0.7858 (0.7811-0.7908) | 0.7976 (0.7933-0.8018) | 0.7369 (0.7344-0.7398) | adopte |
| flux | avant | flux_et_repli_selectif | 0.8525 (0.8480-0.8570) | 0.8642 (0.8589-0.8691) | 0.8210 (0.8175-0.8243) (publie) | adopte |
| double_tampon | sans_double_tampon | apres | 0.9462 (0.9423-0.9501) | 0.9432 (0.9401-0.9461) | 0.9387 (0.9358-0.9418) | adopte |
| sorties_anticipees | sans_anticipation | apres | 0.9916 (0.9874-0.9956) | 0.9872 (0.9835-0.9904) | 0.9823 (0.9792-0.9857) | adopte |
| fenetres_du_repli | repli_cles_entieres | apres | 1.0010 (0.9970-1.0052) (publie) | 0.9962 (0.9931-0.9991) (publie) | 0.9787 (0.9761-0.9813) | adopte |
| A/A | avant | avant_bis | 0.9922 (0.9871-0.9977) | 1.0037 (0.9990-1.0090) | 1.0029 (1.0013-1.0046) | valide |

## Etage C a chaud, K5 (mediane des passes 2..P de tous les processus, ms ; transferts : partition du mur, pas une mesure du DMA ; sorties : reservation et premier toucher)

| trame | bras | total | parcours | feuilles | emission | fin_etage | transferts | sorties | publication | epinglee (Mo) | pic (Mo) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 | avant | 34.292 | 4.186 | 11.040 | 3.385 | 2.458 | 9.059 | - | 4.036 | 52.1 | 994.1 |
| ng00 | avant_bis | 33.989 | 4.182 | 11.033 | 3.391 | 2.450 | 8.988 | - | 3.814 | 52.1 | 994.1 |
| ng00 | apres | 26.896 | 4.110 | 10.990 | 2.129 | 2.043 | 3.264 | 2.602 | 1.662 | 16.8 | 906.7 |
| ng00 | sans_anticipation | 27.059 | 4.128 | 10.993 | 3.346 | 2.404 | 3.646 | 0.000 | 2.498 | 16.8 | 906.7 |
| ng00 | sans_double_tampon | 28.419 | 4.184 | 11.027 | 2.119 | 2.070 | 5.127 | 2.584 | 1.310 | 8.4 | 898.3 |
| ng00 | repli_cles_entieres | 26.850 | 4.169 | 11.029 | 2.157 | 2.049 | 3.210 | 2.547 | 1.651 | 16.8 | 906.7 |
| ng00 | flux_et_repli_selectif | 29.182 | 4.153 | 11.020 | 3.367 | 2.424 | 5.823 | 0.000 | 2.343 | 8.4 | 898.3 |
| ng01 | avant | 29.863 | 3.804 | 8.933 | 3.655 | 2.304 | 7.897 | - | 3.194 | 45.2 | 827.7 |
| ng01 | avant_bis | 29.962 | 3.811 | 8.937 | 3.668 | 2.313 | 7.836 | - | 3.291 | 45.2 | 827.7 |
| ng01 | apres | 23.763 | 3.734 | 8.900 | 2.533 | 1.959 | 2.942 | 2.223 | 1.372 | 16.8 | 754.1 |
| ng01 | sans_anticipation | 24.092 | 3.785 | 8.917 | 3.557 | 2.275 | 3.385 | 0.000 | 2.164 | 16.8 | 754.1 |
| ng01 | sans_double_tampon | 25.214 | 3.792 | 8.928 | 2.529 | 1.971 | 4.639 | 2.198 | 1.141 | 8.4 | 745.7 |
| ng01 | repli_cles_entieres | 23.855 | 3.741 | 8.901 | 2.540 | 1.960 | 2.989 | 2.211 | 1.420 | 16.8 | 754.1 |
| ng01 | flux_et_repli_selectif | 25.777 | 3.798 | 8.924 | 3.593 | 2.279 | 5.139 | 0.000 | 2.033 | 8.4 | 745.7 |
| ng02 | avant | 36.521 | 4.260 | 10.235 | 3.693 | 3.993 | 10.291 | - | 3.986 | 52.8 | 1010.5 |
| ng02 | avant_bis | 36.655 | 4.262 | 10.245 | 3.699 | 3.998 | 10.315 | - | 3.967 | 52.8 | 1010.5 |
| ng02 | apres | 26.848 | 4.183 | 10.197 | 2.364 | 2.197 | 3.472 | 2.710 | 1.630 | 16.8 | 921.7 |
| ng02 | sans_anticipation | 27.454 | 4.195 | 10.190 | 3.676 | 2.578 | 4.159 | 0.000 | 2.503 | 16.8 | 921.7 |
| ng02 | sans_double_tampon | 28.642 | 4.187 | 10.197 | 2.368 | 2.198 | 5.561 | 2.705 | 1.335 | 8.4 | 913.3 |
| ng02 | repli_cles_entieres | 27.439 | 4.186 | 10.197 | 2.378 | 2.200 | 4.022 | 2.683 | 1.677 | 16.8 | 921.7 |
| ng02 | flux_et_repli_selectif | 29.977 | 4.197 | 10.197 | 3.679 | 2.584 | 6.785 | 0.000 | 2.358 | 11.3 | 916.2 |

## Informations (ne decident rien)

| mesure | trame | bras | mediane (ms) |
| --- | --- | --- | ---: |
| k10 | ng00 | avant | 135.867 |
| k10 | ng00 | apres | 102.740 |
| k10 | ng01 | avant | 111.984 |
| k10 | ng01 | apres | 84.036 |
| k10 | ng02 | avant | 145.316 |
| k10 | ng02 | apres | 98.207 |
| cache | ng00 | apres | 26.562 |
| cache | ng00 | apres_cache | 25.636 |
| cache | ng01 | apres | 23.805 |
| cache | ng01 | apres_cache | 22.875 |
| cache | ng02 | apres | 26.704 |
| cache | ng02 | apres_cache | 25.705 |
| mur FULL (dont C) | ng00 | avant | 160.162 (35.453) |
| mur FULL (dont C) | ng00 | apres | 153.861 (28.305) |
| mur FULL (dont C) | ng01 | avant | 128.006 (30.978) |
| mur FULL (dont C) | ng01 | apres | 122.664 (25.018) |
| mur FULL (dont C) | ng02 | avant | 164.872 (37.541) |
| mur FULL (dont C) | ng02 | apres | 155.638 (27.903) |
| v12set | kitti_ng_00_000648 | avant / apres | 52.553 / 42.138 |
| v12set | kitti_ng_00_000660 | avant / apres | 52.229 / 41.918 |
| v12set | kitti_ng_00_001464 | avant / apres | 53.969 / 42.854 |
| v12set | kitti_ng_00_001502 | avant / apres | 47.180 / 38.250 |
| v12set | kitti_ng_00_001880 | avant / apres | 67.851 / 52.783 |
| v12set | kitti_ng_00_001896 | avant / apres | 89.850 / 68.580 |
| v12set | kitti_ng_00_003612 | avant / apres | 50.627 / 39.994 |
| v12set | kitti_ng_00_003624 | avant / apres | 51.483 / 40.750 |
| v12set | kitti_ng_02_000620 | avant / apres | 47.714 / 38.587 |
| v12set | kitti_ng_02_001600 | avant / apres | 57.151 / 43.699 |
| v12set | kitti_ng_02_001604 | avant / apres | 56.942 / 44.478 |
| v12set | kitti_ng_02_001606 | avant / apres | 53.292 / 42.564 |
| v12set | kitti_ng_02_001610 | avant / apres | 54.561 / 42.503 |
| v12set | kitti_ng_05_002060 | avant / apres | 61.302 / 48.721 |
| v12set | kitti_ng_05_002064 | avant / apres | 58.845 / 45.785 |
| v12set | kitti_ng_05_002748 | avant / apres | 37.005 / 30.527 |
| v12set | kitti_ng_06_000770 | avant / apres | 30.887 / 25.030 |
| v12set | kitti_ng_06_000772 | avant / apres | 33.504 / 27.284 |
| v12set | kitti_ng_06_000774 | avant / apres | 38.054 / 31.346 |
| v12set | kitti_ng_06_000780 | avant / apres | 32.979 / 26.835 |
| v12set | kitti_ng_06_000798 | avant / apres | 24.540 / 20.287 |
| v12set | kitti_ng_06_000800 | avant / apres | 25.195 / 21.118 |
| v12set | kitti_ng_06_000804 | avant / apres | 31.876 / 25.573 |
| v12set | kitti_ng_06_000806 | avant / apres | 33.997 / 27.445 |
| v12set | kitti_ng_08_000000 | avant / apres | 34.087 / 26.780 |
| v12set | kitti_ng_08_000100 | avant / apres | 30.211 / 24.192 |
| v12set | kitti_ng_08_000200 | avant / apres | 37.053 / 27.252 |
| v12set | kitti_ng_08_000246 | avant / apres | 35.110 / 28.195 |
| v12set | kitti_ng_08_000691 | avant / apres | 56.665 / 43.984 |
| v12set | kitti_ng_08_001176 | avant / apres | 58.387 / 45.457 |
| v12set | kitti_ng_08_001193 | avant / apres | 72.410 / 55.421 |
| v12set | kitti_ng_08_001302 | avant / apres | 41.719 / 34.127 |
| v12set | kitti_ng_08_001847 | avant / apres | 24.927 / 20.948 |
| v12set | kitti_ng_08_002119 | avant / apres | 89.994 / 71.739 |
| v12set | kitti_ng_08_002554 | avant / apres | 53.400 / 42.383 |
| v12set | kitti_ng_10_000424 | avant / apres | 55.382 / 43.190 |
| v12set | kitti_ng_10_000430 | avant / apres | 63.135 / 48.246 |

Informations non jouees (echeance) : 0 processus.
