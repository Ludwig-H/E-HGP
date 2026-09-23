# Omissions de catalogue que la tour FULL refuse, et la zone que rien ne juge encore

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
tour**. C'était une conjecture ; l'argument proposé alors (une telle boule est
une naissance à l'ordre $p+u$ ; son nœud $I\cup U$ doit fusionner, et sa
première référence vient d'une descente qui atteint sa clé, laquelle lève
`full_ball_*missing_weak_terminal` quand la clé manque) a une lacune
relevée par [B](../CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md) :
FULL construit programmes et nœuds depuis le catalogue **amputé**, donc la
racine unique ne force pas à elle seule cette référence. Il manque un lemme
de première fusion (une boule conservée impose une facette dont la
résolution atteint la boule omise, ou le retrait laisse plusieurs racines).
Le vérificateur l'a confrontée à un oracle borné (nuages de 6 à 8 sites,
3 382 retraits sur 3 382 refusés).

**Lemme conditionnel prouvé par A** :
[première cofacette](../LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md).
Pour $S=I_B\cup U_B$ et $K=|S|<n$, soit $z\notin S$ qui minimise le rayon
de $C=\mathrm{MEB}(S\cup\lbrace z\rbrace)$ : $C$ n'a aucun intérieur strict hors
de $S$, sa fenêtre contient $K$, et $S$, isolée juste avant le niveau de
$C$, est émise comme représentant par le bloc $C$ ; sa résolution trouve la
clé de $B$ absente et aucun intrus : refus. Le lemme couvre aussi plusieurs
retraits tous de classe $p+u\leq K_{\max}$ (prendre le plus grand rayon).
Il reste à qualifier, avant registre, la couture « première cofacette →
représentant strict → résolveur » sur coquilles étendues, égalités de
niveau et voies statique et temporelle. Mon esquisse antérieure, par le
premier bloc touchant la composante, est retirée au profit de ce lemme.

Ce dossier teste l'énoncé **à la taille d'intérêt** et mesure ce qu'il
laisse, en croisant avec Euler.

## Classes

Pour une coquille régulière ($u=q$) :

- **vue par la tour** si $p+q\leq K_{\max}$ (lemme conditionnel de A ci-dessus, observé à 8k) ;
- **vue par Euler** si $p\leq K_{\max}-3$ : une omission isolée change
  alors $E_{K}$ à $K=p+1\leq K_{\max}-2$, car le coefficient de $t^{p}$
  dans $t^{p}(t-1)^{q-1}$ vaut $(-1)^{q-1}$ ;
- **zone potentiellement aveugle** sinon (ni Euler ni le statut de la tour
  n'y sont systématiquement sensibles), c'est-à-dire, avec l'admission
  $p+q\leq K_{\max}+1$ : les boules de **fusion seule à l'ordre
  $K_{\max}$** de type q2 à $p=K_{\max}-1$ et q3 à $p=K_{\max}-2$ (les q4
  à $p=K_{\max}-3$ sont vues par Euler).

## Pièces

- `omission_tower_probe.cpp` : chaîne saine avec catalogue, puis tour
  reconstruite sans une boule **déjà émise** (la sonde ne cherche aucune clé
  jamais émise), tirée de façon déterministe, à pas régulier dans l'ordre
  des clés, dans chaque strate
  (ordre haut ≤ Kmax ou non, q, coquille régulière ou étendue) ; mode
  `b13` pour le nuage de 13 sites de
  [B](../CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md).
- `run_campaign.sh` : la campagne exécutée ; `aggregate.py` : les tableaux.
- `q2_sample_judge.cpp`, `run_q2_judge.sh` : juge d'échantillon q2 indépendant du
  générateur (section finale).
- `run_digest_campaign.sh`, `results/digest/` : condensés des tours acceptées.
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
ne rattrape presque rien (ci-dessous). Sortie brute : `results/b13.out`. Depuis la contrelecture B, le code de
sortie du mode `b13` **exige** chacun de ces refus (`results/b13_v2.out`,
code 0).

## Résultats à 8k

### Populations du catalogue

| cas | Kmax | boules | ordre haut ≤ Kmax | vues par Euler (régulières) | angle mort conjoint (régulières) | coquilles étendues |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| clusters_8000_k5 | 5 | 510 758 | 62,4 % | 68,9 % | 133 810 (26,2 %) | 83 (dont 39 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s00_8000_k5 | 5 | 342 181 | 64,2 % | 67,2 % | 91 930 (26,9 %) | 54 (dont 28 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s01_8000_k5 | 5 | 257 607 | 66,4 % | 66,7 % | 69 201 (26,9 %) | 66 (dont 23 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k10 | 10 | 1 083 173 | 81,0 % | 87,4 % | 119 089 (11,0 %) | 230 (dont 41 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k5 | 5 | 278 809 | 66,2 % | 66,8 % | 75 000 (26,9 %) | 111 (dont 44 d'ordre haut > Kmax) ; sortie 0 |
| lidar_s02_8000_k7 | 7 | 532 339 | 74,0 % | 79,1 % | 94 927 (17,8 %) | 163 (dont 52 d'ordre haut > Kmax) ; sortie 0 |
| terrain_8000_k5 | 5 | 160 174 | 70,2 % | 60,9 % | 47 146 (29,4 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |
| uniform_8000_k10 | 10 | 3 088 676 | 77,0 % | 89,3 % | 303 965 (9,8 %) | 1 (dont 0 d'ordre haut > Kmax) ; sortie 0 |
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
| 10 | non | étendue | 2 | 41 | 8 | 6 | complete_relative, full_ball_static_missing_weak_terminal |
| 10 | non | régulière | 2 | 41 818 | 16 | 0 | complete_relative |
| 10 | non | régulière | 3 | 381 236 | 16 | 0 | complete_relative |
| 10 | non | régulière | 4 | 493 259 | 16 | 1 | complete_relative, full_ball_final_component_count |
| 10 | oui | étendue | 2 | 187 | 9 | 9 | full_ball_static_missing_weak_terminal |
| 10 | oui | étendue | 3 | 1 | 1 | 1 | full_ball_static_missing_weak_terminal |
| 10 | oui | étendue | 4 | 2 | 2 | 2 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 2 | 414 272 | 16 | 16 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 3 | 1 629 768 | 16 | 16 | full_ball_static_missing_weak_terminal |
| 10 | oui | régulière | 4 | 1 211 265 | 16 | 16 | full_ball_static_missing_weak_terminal |

**Lecture.**

- **575 retraits sur 575 refusés** dans les strates d'ordre haut
  $p+u\leq K_{\max}$ (six cas à K5, deux à K7, deux à K10, coquilles
  régulières et étendues), toujours par `full_ball_static_missing_weak_terminal` :
  aucun contre-exemple à l'énoncé, à la taille d'intérêt. Ce n'est pas une
  preuve.
- Dans la zone potentiellement aveugle (q2 à $p=K_{\max}-1$, q3 à
  $p=K_{\max}-2$), **2 retraits sur 312** seulement sont refusés, par la
  connexité finale. Les autres sont acceptés avec le statut
  `complete_relative` ; la section suivante mesure s'ils changent la tour.
  Cet angle mort pèse **26,2 à 29,4 %** du catalogue à K5, 16,8 à 17,8 % à
  K7 et **9,8 à 11,0 %** à K10 ; à K10, il se compose de 26 860 et 14 958 q2
  à $p=9$ et de 277 105 et 104 131 q3 à $p=8$ (uniforme, LiDAR s02).
- Les q4 d'ordre haut > Kmax ($p=K_{\max}-3$) passent la tour (13 refus
  sur 156, connexité finale) mais sont vus par Euler.
- À K7, les boules de l'angle mort de K5 ($p+q=6$) appartiennent aux
  strates d'ordre haut ≤ 7, où les 73 retraits tirés sont tous refusés
  (tirage non ciblé sur elles). D'où le contrôle proposé à K5 :
  exécution à Kmax+1 ou Kmax+2 **avec tour**, plus l'égalité clé par clé
  de la restriction $p+q_{\min}\leq K_{\max}+1$. À K10, ce contrôle
  demanderait K11, hors domaine (`kBallInteriorMax = 9`).

## Retraits acceptés : la tour change-t-elle ?

B a montré sur des nuages de 8 sites que 9 retraits acceptés sur 12 changent
le condensé FULL. Seconde campagne (`run_digest_campaign.sh`, sonde durcie
compilée contre les sources de `9b3491ec`) : pour chaque retrait accepté,
le condensé `tower_digest` de la tour mutée est comparé à celui de la tour
saine. Un condensé différent établit qu'un champ de la tour diffère ; un
condensé égal ne certifie pas l'égalité du payload.

### Retraits isolés, sommés sur les cas

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

**Lecture.** Dans la zone potentiellement aveugle, **10 des 68** retraits q2 acceptés et **26 des 67** retraits q3 acceptés changent le condensé de la tour (uniforme et deux coupes LiDAR à K5, une coupe LiDAR à K10) : ce sont des **tours fausses publiées avec le statut `complete_relative`**, qu'Euler ne voit pas non plus. Les autres gardent un condensé égal : la boule retirée n'y portait vraisemblablement qu'une fusion redondante (non démontré, un condensé égal ne certifie pas le payload). Les q4 d'ordre haut > Kmax changent le condensé 48 fois sur 62 acceptés, mais Euler les voit. Dans les strates d'ordre haut ≤ Kmax, tous les retraits sont refusés. Le risque résiduel concret, sans juge aujourd'hui, est donc la famille **q3 à $p=K_{\max}-2$**.

## Juge d'échantillon q2 indépendant du générateur

`q2_sample_judge.cpp` couvre la partie q2 de l'angle mort, pour tout $p$.
Pour un site $a$ tiré à pas régulier et **tout** autre site $b$, la boule
diamétrale de $\lbrace a,b\rbrace$ est recensée par balayage brut de tous
les sites en entiers exacts ($x$ intérieur strict si et seulement si
$(x-a)\cdot(x-b)<0$, sur la sphère si $=0$). Si elle a au plus
$K_{\max}-1$ intérieurs, elle est admise et le catalogue doit contenir une
boule de même $p$, portée par la même sphère, dont la coquille contient
$a$ et $b$ et a le même nombre de sites. Ni WSPD, ni témoins, ni Pool :
seul le catalogue de la chaîne est lu. Coût : $O(n)$ candidats par site
tiré, chacun un balayage arrêté dès que $p$ dépasse $K_{\max}-1$, donc
jamais $O(n^{3})$. Anti-vacuité : la première boule trouvée est retirée de
la table, et le juge doit alors la déclarer manquante (`mutant_killed=1`).
Campagne `run_q2_judge.sh`, sorties dans `results/q2_judge/` :

| cas | Kmax | sites tirés | boules q2 attendues | trouvées | dont $p=K_{\max}-1$ | coquilles étendues | anti-vacuité |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| LiDAR s00 8k | 10 | 1 000 | 50 739 | 50 739 | 4 880 | 24 | tué |
| LiDAR s01 8k | 10 | 1 000 | 41 755 | 41 755 | 3 664 | 43 | tué |
| LiDAR s02 8k | 10 | 1 000 | 44 301 | 44 301 | 3 731 | 62 | tué |
| LiDAR s02 8k | 5 | 1 000 | 23 883 | 23 883 | 4 479 | 34 | tué |
| uniforme 8k | 10 | 500 | 35 280 | 35 280 | 3 373 | 0 | tué |
| trame entière 08/000000 sans sol (39 885 sites) | 10 | 200 | 8 725 | 8 725 | 858 | 6 | tué |

**204 683 boules q2 attendues, 204 683 trouvées**, dont 16 506 à $p=9$ à
K10, la famille q2 de l'angle mort. Ce juge est un échantillon : il ne
certifie pas les sites non tirés. Il reste à couvrir la famille **q3 à
$p=K_{\max}-2$**, la plus nombreuse de l'angle mort (104 131 à 277 105 à
K10 sur 8k) : pour elle, je n'ai pas de balayage indépendant de coût
$O(n^{2})$ par site, car une boule circonscrite peu peuplée peut avoir un
très grand rayon (les ancres longues du LiDAR).

Bibliothèques : la sonde d'omission a été compilée contre les sources de
`67fce4e9` (v14), le juge q2 contre celles de `243373f6` (v15) ; le
générateur est le même dans les deux. Le mode `b13` de la sonde publiée
identifie D et T par leurs coordonnées ; la campagne à 8k a tourné avec la
version précédente de ce seul mode (le mode d'échelle est inchangé).
