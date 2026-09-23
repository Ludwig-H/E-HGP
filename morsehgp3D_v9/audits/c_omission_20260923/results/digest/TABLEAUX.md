## Populations du catalogue

| cas | Kmax | boules | ordre haut ≤ Kmax | vues par Euler (régulières) | angle mort conjoint (régulières) | coquilles étendues |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| lidar_s00_8000_k5 | 5 | 342 181 | 64,2 % | 67,2 % | 91 930 (26,9 %) | 54 (dont 28 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k10 | 10 | 1 083 173 | 81,0 % | 87,4 % | 119 089 (11,0 %) | 230 (dont 41 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k5 | 5 | 278 809 | 66,2 % | 66,8 % | 75 000 (26,9 %) | 111 (dont 44 d'ordre haut > Kmax) ; sortie 0 |
| uniform_8000_k5 | 5 | 594 386 | 61,4 % | 68,7 % | 157 736 (26,5 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |

## Retraits isolés, sommés sur les cas

| Kmax | ordre haut p+u ≤ Kmax | coquille | q | population | retraits | refusés | acceptés, condensé changé | acceptés, condensé égal | issues |
| ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 5 | non | étendue | 2 | 70 | 40 | 14 | 1 | 25 | complete_relative, full_ball_static_missing_weak_terminal |
| 5 | non | étendue | 3 | 1 | 1 | 1 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 5 | non | étendue | 4 | 1 | 1 | 0 | 0 | 1 | complete_relative |
| 5 | non | régulière | 2 | 66 341 | 60 | 0 | 8 | 52 | complete_relative |
| 5 | non | régulière | 3 | 258 325 | 60 | 1 | 23 | 36 | complete_relative, full_ball_final_component_count |
| 5 | non | régulière | 4 | 121 668 | 60 | 5 | 43 | 12 | complete_relative, full_ball_final_component_count |
| 5 | oui | étendue | 2 | 94 | 41 | 41 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 2 | 277 678 | 60 | 60 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 3 | 407 012 | 60 | 60 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 5 | oui | régulière | 4 | 84 186 | 60 | 60 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | non | étendue | 2 | 41 | 8 | 6 | 0 | 2 | complete_relative, full_ball_static_missing_weak_terminal |
| 10 | non | régulière | 2 | 14 958 | 8 | 0 | 2 | 6 | complete_relative |
| 10 | non | régulière | 3 | 104 131 | 8 | 0 | 3 | 5 | complete_relative |
| 10 | non | régulière | 4 | 86 825 | 8 | 1 | 5 | 2 | complete_relative, full_ball_final_component_count |
| 10 | oui | étendue | 2 | 186 | 8 | 8 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | oui | étendue | 3 | 1 | 1 | 1 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | oui | étendue | 4 | 2 | 2 | 2 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 2 | 161 589 | 8 | 8 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 3 | 480 522 | 8 | 8 | 0 | 0 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 4 | 234 918 | 8 | 8 | 0 | 0 | full_ball_static_missing_weak_terminal |
