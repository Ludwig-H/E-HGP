# Omissions de catalogue que la tour FULL refuse, et celles que personne ne voit

Auditeur C, 23 septembre 2026. Cadre : `exploration_v9_hors_registre`,
`reference_cpu`, `quantized_u18_input_only`, `public_status=not_claimed`.
GCP non utilisé. Sonde d'audit hors produit, compilée contre les
bibliothèques v9 construites depuis `12a5f28f` (sources de `67fce4e9`,
sonde v14). Exécutions locales `nice -n 19`, deux fils, hôte partagé ;
aucun temps n'est revendiqué.

## Question

La porte T2 et l'invariant d'Euler ne disent pas ce que fait la tour FULL
quand une `BallKey` manque **entièrement** au catalogue. La vérification
adverse de l'audit C (constat L4-02) a réfuté la phrase de la révision 1
« à $K=1$, une arête d'arbre couvrant minimal omise fausse les niveaux sans
refus », avec cet énoncé : sous le contrat ($K_{\max}\geq2$), **une boule
omise dont l'ordre haut $p+u$ est au plus $K_{\max}$ est refusée par la
tour**. Argument : une telle boule est une naissance à l'ordre $p+u$ ; son
nœud $I\cup U$ doit fusionner avant la fin de l'ordre (racine unique), et
sa première référence ne peut venir que d'une descente qui atteint sa clé ;
absente, la descente trouve une boule sans intrus hors facette et lève
`full_ball_missing_weak_terminal` (`full_ball_tower.hpp:1471`, voie
statique `:1114`). Le vérificateur l'a confronté à un oracle borné (nuages
de 6 à 8 sites, 3 382 retraits sur 3 382 refusés).

Ce dossier teste l'énoncé **à la taille d'intérêt** et mesure ce qu'il
laisse, en croisant avec Euler.

## Classes

Pour une coquille régulière ($u=q$) :

- **vue par la tour** si $p+q\leq K_{\max}$ (énoncé ci-dessus) ;
- **vue par Euler** si $p\leq K_{\max}-3$ : une omission isolée change
  alors $E_{K}$ à $K=p+1\leq K_{\max}-2$, car le coefficient de $t^{p}$
  dans $t^{p}(t-1)^{q-1}$ vaut $(-1)^{q-1}$ ;
- **angle mort conjoint** sinon, c'est-à-dire, avec l'admission
  $p+q\leq K_{\max}+1$ : les boules de **fusion seule à l'ordre
  $K_{\max}$** de type q2 à $p=K_{\max}-1$ et q3 à $p=K_{\max}-2$ (les q4
  à $p=K_{\max}-3$ sont vues par Euler).

## Pièces

- `omission_tower_probe.cpp` : chaîne saine avec catalogue, puis tour
  reconstruite sans une boule, tirée à pas régulier dans chaque strate
  (ordre haut ≤ Kmax ou non, q, coquille régulière ou étendue) ; mode
  `b13` pour le nuage de 13 sites de
  [B](../CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md).
- `run_campaign.sh` : la campagne exécutée ; `aggregate.py` : les tableaux.
- `results/` : sorties brutes et `TABLEAUX.md`. Les coupes LiDAR 8k
  (`s00`, `s01`, `s02`, disques emboîtés du runner v12) ne sont pas
  versionnées : seuls les comptes le sont.

Compilation (depuis ce dossier, `B` = build v9 Release avec Boost) :

```bash
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -I../../ -I../../src/gen -isystem "$BOOST_ROOT/include" omission_tower_probe.cpp \
  "$B/libmhgp9_chain.a" "$B/libmhgp9_gen.a" -lpthread -o omission_tower_probe
```

## Nuage de 13 sites de B

Chaîne saine à K5 (57 boules) et à K7 (70 boules) ; D (q2, $p=4$) et T
(q3, $p=4$) identifiés par les coordonnées exactes de leur coquille, sans
ambiguïté. Le couple Euler + restriction clé par clé ne voit pas l'omission
de D et T (note de B), mais **la tour la refuse** dans les cinq cas, sur la
voie séquentielle comme sur la voie statique à deux fils : sans D à K5
(connexité finale, `full_ball_final_component_count`), sans D, sans T et
sans les deux à K7 (connexité finale ou `full_ball_static_missing_weak_terminal`).
Sur un nuage de 13 sites, la connexité finale suffit souvent ; à 8k, elle
ne rattrape presque rien (ci-dessous). Sortie brute : `results/b13.out`.

## Résultats à 8k

### Populations du catalogue

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

### Retraits isolés, sommés sur les cas

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

**Lecture.**

- **515 retraits sur 515 refusés** dans les strates d'ordre haut
  $p+u\leq K_{\max}$ (six cas à K5, deux à K7, coquilles régulières et
  étendues), toujours par `full_ball_static_missing_weak_terminal` : aucun
  contre-exemple à l'énoncé, à la taille d'intérêt. Ce n'est pas une preuve.
- Dans l'angle mort conjoint (q2 à $p=K_{\max}-1$, q3 à $p=K_{\max}-2$),
  **2 retraits sur 280** seulement sont refusés, par la connexité finale.
  Cet angle mort pèse **26,2 à 29,4 %** du catalogue à K5 et 16,8 à
  17,8 % à K7, sur les familles synthétiques comme sur les coupes LiDAR.
- Les q4 d'ordre haut > Kmax ($p=K_{\max}-3$) passent la tour (12 refus
  sur 140, connexité finale) mais sont vus par Euler.
- À K7, les boules de l'angle mort de K5 ($p+q=6$) appartiennent aux
  strates d'ordre haut ≤ 7, où les 73 retraits tirés sont tous refusés
  (tirage non ciblé sur elles). D'où le contrôle proposé à K5 :
  exécution à Kmax+1 ou Kmax+2 **avec tour**, plus l'égalité clé par clé
  de la restriction $p+q_{\min}\leq K_{\max}+1$.
- K10 : campagne en cours au moment de ce commit (`run_campaign.sh`,
  deux cas, huit retraits par strate), ajoutée ensuite.
