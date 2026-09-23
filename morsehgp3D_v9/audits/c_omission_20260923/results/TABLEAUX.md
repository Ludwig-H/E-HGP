## Populations du catalogue

| cas | Kmax | boules | ordre haut ≤ Kmax | vues par Euler (régulières) | angle mort conjoint (régulières) | coquilles étendues |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| clusters_8000_k5 | 5 | 510 758 | 62,4 % | 68,9 % | 133 810 (26,2 %) | 83 (dont 39 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s00_8000_k5 | 5 | 342 181 | 64,2 % | 67,2 % | 91 930 (26,9 %) | 54 (dont 28 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s01_8000_k5 | 5 | 257 607 | 66,4 % | 66,7 % | 69 201 (26,9 %) | 66 (dont 23 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k5 | 5 | 278 809 | 66,2 % | 66,8 % | 75 000 (26,9 %) | 111 (dont 44 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k7 | 7 | 532 339 | 74,0 % | 79,1 % | 94 927 (17,8 %) | 163 (dont 52 d'ordre haut > Kmax) ; sortie 0 |
| terrain_8000_k5 | 5 | 160 174 | 70,2 % | 60,9 % | 47 146 (29,4 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |
| uniform_8000_k5 | 5 | 594 386 | 61,4 % | 68,7 % | 157 736 (26,5 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |
| uniform_8000_k7 | 7 | 1 293 712 | 69,6 % | 81,0 % | 217 989 (16,8 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |

## Retraits isolés, sommés sur les cas

| Kmax | ordre haut p+u ≤ Kmax | coquille | q | population | retraits | refusés | issues |
| ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| 5 | non | étendue | 2 | 132 | 80 | 30 | complete_relative, full_ball_static_missing_weak_terminal |
| 5 | non | étendue | 3 | 1 | 1 | 1 | full_ball_static_missing_weak_terminal |
| 5 | non | étendue | 4 | 1 | 1 | 0 | complete_relative |
| 5 | non | régulière | 2 | 123 608 | 120 | 0 | complete_relative |
| 5 | non | régulière | 3 | 451 215 | 120 | 2 | complete_relative, full_ball_final_component_count |
| 5 | non | régulière | 4 | 197 953 | 120 | 11 | complete_relative, full_ball_final_component_count |
| 5 | oui | étendue | 2 | 182 | 82 | 82 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 2 | 519 342 | 120 | 120 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 3 | 713 381 | 120 | 120 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 4 | 138 100 | 120 | 120 | full_ball_static_missing_weak_terminal |
| 7 | non | étendue | 2 | 51 | 10 | 3 | complete_relative, full_ball_static_missing_weak_terminal |
| 7 | non | étendue | 4 | 1 | 1 | 1 | full_ball_static_missing_weak_terminal |
| 7 | non | régulière | 2 | 45 032 | 20 | 0 | complete_relative |
| 7 | non | régulière | 3 | 267 884 | 20 | 0 | complete_relative |
| 7 | non | régulière | 4 | 218 443 | 20 | 1 | complete_relative, full_ball_final_component_count |
| 7 | oui | étendue | 2 | 110 | 11 | 11 | full_ball_static_missing_weak_terminal |
| 7 | oui | étendue | 3 | 1 | 1 | 1 | full_ball_static_missing_weak_terminal |
| 7 | oui | étendue | 4 | 1 | 1 | 1 | full_ball_static_missing_weak_terminal |
| 7 | oui | régulière | 2 | 282 885 | 20 | 20 | full_ball_static_missing_weak_terminal |
| 7 | oui | régulière | 3 | 709 128 | 20 | 20 | full_ball_static_missing_weak_terminal |
| 7 | oui | régulière | 4 | 302 515 | 20 | 20 | full_ball_static_missing_weak_terminal |
