# Omissions de catalogue que la tour FULL refuse, et la zone jugée par échantillon

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
- `q2_sample_judge.cpp`, `q3_sample_judge.cpp` : juges d'échantillon q2 et q3
  indépendants du générateur ; `run_q2_judge.sh` (première campagne q2),
  `run_judges_v5.sh` (campagne v5), `run_judges_v6_gates.sh` (portes v6),
  `run_judges_v7_gates.sh` (portes v7, `--selftest`), `regen_inputs.py`
  (entrées), `tables_judges.py` (tableaux, `--gates`) ; `results/judges_v5/`,
  `results/gates_v6/`, `results/gates_v7/` (sorties, `PROVENANCE.txt`,
  `STATUS`) ;
  `verification_juge_q3.json` (vérification adverse du juge q3).
- `run_digest_campaign.sh`, `results/digest/` : condensés des tours acceptées.
- `batch_diff.cpp`, `run_batch_diff.sh`, `results/batch_diff_v1/` : différentiel
  moteur / lots du chemin q3/q4 S2 (section finale).
- `results/` : sorties brutes et `TABLEAUX.md`. Les coupes LiDAR 8k
  (`s00`, `s01`, `s02`, disques emboîtés du runner v12) et la trame entière
  08/000000 ne sont pas copiées ici : `regen_inputs.py` les régénère bit à
  bit depuis les trames sans sol versionnées du reçu v8, dans un dossier neuf
  hors dépôt, et contrôle leurs empreintes (`--check` pour un dossier
  existant).

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

**Lecture.** Dans la zone potentiellement aveugle, **10 des 68** retraits q2 acceptés et **26 des 67** retraits q3 acceptés changent le condensé de la tour (uniforme et deux coupes LiDAR à K5, une coupe LiDAR à K10) : ce sont des **tours fausses publiées avec le statut `complete_relative`**, qu'Euler ne voit pas non plus. Les autres gardent un condensé égal : la boule retirée n'y portait vraisemblablement qu'une fusion redondante (non démontré, un condensé égal ne certifie pas le payload). Les q4 d'ordre haut > Kmax changent le condensé 48 fois sur 62 acceptés, mais Euler les voit. Dans les strates d'ordre haut ≤ Kmax, tous les retraits sont refusés. Le risque résiduel concret est donc la famille **q3 à $p=K_{\max}-2$**, que seul le juge q3 ci-dessous juge, par échantillon.

## Juge d'échantillon q2 indépendant du générateur

`q2_sample_judge.cpp` couvre la partie q2 de la zone potentiellement
aveugle, pour tout $p$. Pour un site $a$ tiré et **tout** autre site $b$,
la boule diamétrale de $\lbrace a,b\rbrace$ est recensée par balayage brut
de tous les sites en entiers exacts ($x$ intérieur strict si et seulement si
$(x-a)\cdot(x-b)<0$, sur la sphère si $=0$). Si elle a au plus
$K_{\max}-1$ intérieurs, elle est admise : le catalogue doit contenir une
boule de même clé canonique ($\vert x\vert^{2}-(a+b)\cdot x+a\cdot b$,
comparée à `ball.key`), de même niveau exact, de même liste triée de sites
de coquille, de mêmes intérieurs, d'arité 2. En sens inverse, toute boule
régulière à deux sites, d'arité 2 et $p\leq K_{\max}-1$, passant par un site
tiré, doit être retrouvée. Ni WSPD, ni témoins, ni Pool. Coût : $O(n)$
candidats par site tiré, chacun un balayage arrêté dès que $p$ dépasse
$K_{\max}-1$, donc jamais $O(n^{3})$.

Première campagne (v1, `b05fbf36`, recoupement par coquille et $p$
seulement) : 204 683 **incidences** (site tiré, partenaire) admissibles,
toutes présentes, dont 16 506 à $p=9$ à K10 ; ses sorties restent dans
`results/q2_judge/`. La version v5 ci-dessous ajoute clé, niveau, coquille
exacte, intérieurs, arité, sens inverse, tirage à graine et mutants
(contrelectures de B et vérification adverse).

## Juge d'échantillon q3 indépendant du générateur

`q3_sample_judge.cpp` vise la famille que ni Euler ni la tour ne jugent,
**q3 à $p=K_{\max}-2$**, et plus largement toute boule q3 admissible
($p\leq K_{\max}-2$) dont un triangle aigu passe par un site tiré, supports
longs compris. Pour un site $a$ tiré et un triangle **strictement aigu**
$(a,b,c)$, la boule circonscrite (centre dans l'intérieur relatif du
triangle, donc support positif et $q_{\min}\leq3$) est admise dès qu'elle a
au plus $K_{\max}-2$ intérieurs stricts. Le catalogue doit alors contenir
une boule de **même clé canonique** (forme puissance reconstruite
indépendamment, réduite par le pgcd, comparée champ par champ à
`ball.key`), de **même niveau exact**, dont la liste triée des sites de
coquille est exactement celle du recensement, avec les **mêmes intérieurs**
et une arité cohérente (2 si la coquille a une paire antipodale, 3 sinon). En sens
inverse, toute boule régulière à trois sites, d'arité 3 et $p\leq K_{\max}-2$,
dont la coquille contient un site tiré doit être retrouvée (sinon `EXTRA` :
boule non critique ou mal recensée). Une coquille de plus de 12 sites trouvée
sur une chaîne complète est une omission, puisque la chaîne refuse ce
domaine.

**Arithmétique exacte.** Avec $u=b-a$, $v=c-a$, $w=u\times v$, le centre
est $O=a+P/D$, où $P=\vert u\vert^{2}(v\times w)+\vert v\vert^{2}(w\times u)$
et $D=2\vert w\vert^{2}$ ; un site $x$ est strictement intérieur si et
seulement si $s(x)=D\vert x-a\vert^{2}-2(x-a)\cdot P<0$, sur la sphère si
$s(x)=0$. Sur la grille u18, $\vert s\vert<2^{116}$ : entiers de 128 bits.
Le niveau $R^{2}=\vert P\vert^{2}/D^{2}$ est comparé à celui du catalogue en
entiers multiprécision (Boost `cpp_int`, produits d'au plus $2^{346}$).
Le recensement passe par un arbre k-d propre au juge : boîtes exactes pour
les boules diamétrales, filtre flottant conservateur (marge d'au moins 4
unités de grille au carré, environ 15 bits au-dessus des erreurs mesurées)
pour les boules circonscrites, tests exacts aux feuilles.

**Lemme de la demi-boule diamétrale (élagage exact).** Soit $B$ une sphère
passant par $a$ et $b$, de centre $O$ et rayon $R$, et $m$ le milieu de
$ab$. Tout site $y$ strictement intérieur à la boule diamétrale $D_{ab}$ et
tel que $(y-m)\cdot(O-m)\geq0$ est strictement intérieur à $B$.

*Preuve.* $O-m$ est orthogonal à $ab$, donc
$R^{2}=\vert a-m\vert^{2}+\vert O-m\vert^{2}$. Alors
$\vert y-O\vert^{2}=\vert y-m\vert^{2}-2(y-m)\cdot(O-m)+\vert O-m\vert^{2}<\vert a-m\vert^{2}+\vert O-m\vert^{2}=R^{2}$. $\square$

Le lemme ne demande pas l'acuité, et le cas $O=m$ ne pose pas de problème.
La combinaison « intérieur **strict** de $D_{ab}$ et demi-plan **fermé** »
est exactement la bonne : avec $D_{ab}$ fermé, le lemme devient faux
($a=(0,0,0)$, $b=(8,0,0)$, $c=(4,8,0)$ et $y=(4,0,4)$, qui est sur la
coquille de $B$).

*Conséquence.* Si $B$ a au plus $K_{\max}-2$ intérieurs stricts, le
demi-plan fermé de direction $O-m$, dans le plan orthogonal à $ab$, contient
au plus $K_{\max}-2$ projections des intérieurs stricts de $D_{ab}$ : la
profondeur de Tukey de l'origine parmi ces projections est au plus
$K_{\max}-2$. Le juge ne garde que les partenaires $b$ qui vérifient cette
condition, et il énumère les triangles dont les deux partenaires la
vérifient ($ac$ est aussi une arête de $B$). La profondeur d'un
sous-ensemble minore la vraie profondeur : l'élagage reste sûr.

**Portes du juge** (`run_judges_v5.sh`, codes attendus) :
- `--compare` rejuge chaque site sans élagage et exige le même ensemble de
  triangles attendus ;
- une **fixture d'égalité** gravée (trois sites sur le segment $ab$, un site
  opposé dans $D_{ab}$ hors de $B$ : profondeur $=p=K_{\max}-2$) passe sans
  mutant et tue `--inject=overprune`, l'élagage dès la profondeur
  $K_{\max}-2$, que les familles aléatoires ne tuent pas ;
- `--inject=level`, `--inject=key` et `--inject=shell-dup` faussent
  respectivement tous les niveaux, toutes les clés seules, ou remplacent un
  site de coquille par un doublon : chaque recoupement doit échouer
  (code 1), pour les deux juges ;
- `--compare` est aussi exécuté sur les quatre sites **les plus isolés**
  (plus grande distance au $K_{\max}$-ième voisin), choisis depuis les
  seules coordonnées d'entrée : ce sont eux qui portent les ancres longues,
  qu'aucun tirage aléatoire n'atteignait ; le mutant de sur-élagage y est
  observé ;
- mutant ciblé : une clé régulière trouvée de rang $p=K_{\max}-2$ et
  d'arité 3 est retirée de la table et son site rejugé ; le manquant doit
  porter deux autres sites de sa coquille ; plancher `--min-top` de clés
  distinctes de cette famille.

Le lanceur `run_judges_v5.sh` reconstruit les bibliothèques v9 depuis ce
dépôt (dossier de build et `src/` propre vérifiés), compile les deux juges
avec une recette fixe et hache, de façon bloquante, son script, la recette,
les sources, les binaires, les bibliothèques et les entrées
(`PROVENANCE.txt`). Le filtre flottant des boîtes est certifié sûr par
[B](../CERTIFICAT_B_MARGE_JUGE_Q3_U18_20260923.md) sous IEEE binary64
conforme, sans fast-math.

**Résultats**

Campagne **v5** (`run_judges_v5.sh`, juges épinglés à `c6042af2`, produit reconstruit depuis `0d5ad2e8`, `STATUS=0`, sorties dans `results/judges_v5/`). **Juge q3 : 286 706 incidences admissibles, toutes présentes**, 0 recoupement faux (clé, niveau, coquille, intérieurs, arité), 0 `EXTRA` ; 278 314 clés distinctes, dont **55 297 clés régulières de la famille $p=K_{\max}-2$ d'arité 3**, soit environ 11 % de cette famille à 8k avec 300 sites tirés ; 2 624 incidences LiDAR à 1 600 unités de grille ou plus (sur la famille uniforme u16, l'étiquette de longueur n'a pas de sens). **Juge q2 : 205 182 incidences, toutes présentes**, 193 951 clés distinctes, dont 19 961 de la famille $p=K_{\max}-1$. Sur les quatre sites les plus isolés de s00 et s02, `--compare` retrouve exactement les mêmes triangles avec et sans élagage (1 513 et 430 incidences, dont 308 et 59 longues). Les limites du lanceur v5 relevées par B (substitutions `git` non contrôlées, redirection prise pour un mutant tué) ne changent pas ces sorties, dont les codes et marqueurs sont lisibles ; elles sont fermées pour les portes par v6.

#### Portes v5 (`run_judges_v5.sh`)

| cas | code | attendu |
| --- | ---: | ---: |
| `q3_fixture_eq_compare` (désaccords d'élagage : 0, incidences : 369, dont ≥ 1 600 unités : 0) | 0 | 0 |
| `q3_fixture_eq_overprune` (désaccords d'élagage : 2, incidences : 369, dont ≥ 1 600 unités : 0) | 1 | 1 |
| `q3_fixture_eq_level` | 1 | 1 |
| `q3_fixture_eq_shell_dup` | 1 | 1 |
| `q3_fixture_eq_key` | 1 | 1 |
| `q2_lidar_s02_8000_k5_level` | 1 | 1 |
| `q2_lidar_s02_8000_k5_shell_dup` | 1 | 1 |
| `q2_lidar_s02_8000_k5_key` | 1 | 1 |
| `q3_lidar_s00_8000_k10_compare_isolated` (désaccords d'élagage : 0, incidences : 1513, dont ≥ 1 600 unités : 308) | 0 | 0 |
| `q3_lidar_s00_8000_k10_overprune_isolated` (désaccords d'élagage : 0, incidences : 1513, dont ≥ 1 600 unités : 308) | 0 | obs |
| `q3_lidar_s02_8000_k10_compare_isolated` (désaccords d'élagage : 0, incidences : 430, dont ≥ 1 600 unités : 59) | 0 | 0 |
| `q3_lidar_s02_8000_k10_overprune_isolated` (désaccords d'élagage : 0, incidences : 430, dont ≥ 1 600 unités : 59) | 0 | obs |

#### Juge q3, campagne v5

| cas | Kmax | sites | incidences | trouvées | manquantes | recoupements faux | EXTRA | clés distinctes | clés q2 | clés p=Kmax−2 (arité 3) / population | ≥ 1 600 unités (dont p=Kmax−2) | code |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| q3_lidar_s00_8000_k10 | 10 | 300 | 89 577 | 89 577 | 0 | 0 | 0 | 86 425 | 0 | 16 733 / 152 067 | 131 (34) | 0 |
| q3_lidar_s01_8000_k10 | 10 | 300 | 60 568 | 60 568 | 0 | 0 | 0 | 58 321 | 0 | 10 533 / 95 032 | 1 690 (474) | 0 |
| q3_lidar_s02_8000_k10 | 10 | 300 | 62 873 | 62 873 | 0 | 0 | 0 | 60 932 | 0 | 10 911 / 104 131 | 338 (78) | 0 |
| q3_lidar_s02_8000_k5 | 5 | 300 | 16 010 | 16 010 | 0 | 0 | 0 | 15 499 | 0 | 5 898 / 56 747 | 46 (19) | 0 |
| q3_lidar_scene00_full_k10 | 10 | 30 | 6 374 | 6 374 | 0 | 0 | 0 | 6 374 | 0 | 1 265 / 555 223 | 419 (144) | 0 |
| q3_uniform_8000_k10 | 10 | 100 | 51 304 | 51 304 | 0 | 0 | 0 | 50 763 | 0 | 9 957 / 277 105 | 51 300 (10 077) | 0 |

#### Juge q2, campagne v5

| cas | Kmax | sites | incidences | trouvées | manquantes | recoupements faux | EXTRA | clés distinctes | clés p=Kmax−1 (arité 2) / population | code |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| q2_lidar_s00_8000_k10 | 10 | 1000 | 50 774 | 50 774 | 0 | 0 | 0 | 47 652 | 4 615 / 19 695 | 0 |
| q2_lidar_s01_8000_k10 | 10 | 1000 | 42 019 | 42 019 | 0 | 0 | 0 | 39 380 | 3 461 / 14 816 | 0 |
| q2_lidar_s02_8000_k10 | 10 | 1000 | 44 378 | 44 378 | 0 | 0 | 0 | 41 580 | 3 526 / 14 958 | 0 |
| q2_lidar_s02_8000_k5 | 5 | 1000 | 24 126 | 24 126 | 0 | 0 | 0 | 22 609 | 4 335 / 18 253 | 0 |
| q2_lidar_scene00_full_k10 | 10 | 200 | 8 587 | 8 587 | 0 | 0 | 0 | 8 565 | 810 / 83 241 | 0 |
| q2_uniform_8000_k10 | 10 | 500 | 35 298 | 35 298 | 0 | 0 | 0 | 34 165 | 3 214 / 26 860 | 0 |

Portes **v6** (`run_judges_v6_gates.sh`, épinglé à `abf3c382`, `STATUS=0`, sorties dans `results/gates_v6/`) : provenance contrôlée, dossier neuf, chaque mutant tué **avec son marqueur causal** ; `drop-long` tué sur les sites isolés de s00 et s02, au moins 50 incidences longues exigées.

#### Portes v6 (`run_judges_v6_gates.sh`)

| cas | code | attendu |
| --- | ---: | ---: |
| `q3_fixture_eq_compare` (désaccords d'élagage : 0, incidences : 369, dont ≥ 1 600 unités : 0) | 0 | 0 |
| `q3_fixture_eq_overprune` (désaccords d'élagage : 2, incidences : 369, dont ≥ 1 600 unités : 0) | 1 | 1 |
| `q3_fixture_eq_level` | 1 | 1 |
| `q3_fixture_eq_shell_dup` | 1 | 1 |
| `q3_fixture_eq_key` | 1 | 1 |
| `q2_lidar_s02_8000_k5_level` | 1 | 1 |
| `q2_lidar_s02_8000_k5_shell_dup` | 1 | 1 |
| `q2_lidar_s02_8000_k5_key` | 1 | 1 |
| `q3_lidar_s00_8000_k10_compare_isolated` (désaccords d'élagage : 0, incidences : 1513, dont ≥ 1 600 unités : 308) | 0 | 0 |
| `q3_lidar_s00_8000_k10_overprune_isolated` (désaccords d'élagage : 0, incidences : 1513, dont ≥ 1 600 unités : 308) | 0 | obs |
| `q3_lidar_s02_8000_k10_compare_isolated` (désaccords d'élagage : 0, incidences : 430, dont ≥ 1 600 unités : 59) | 0 | 0 |
| `q3_lidar_s02_8000_k10_overprune_isolated` (désaccords d'élagage : 0, incidences : 430, dont ≥ 1 600 unités : 59) | 0 | obs |

#### Portes v7 (`run_judges_v7_gates.sh`, après l'audit A du juge v6)

L'[audit A](../AUDIT_A_JUGE_Q3_V6_LONGUES_INCIDENCES_20260923.md) a montré
que le plancher « long » v6 comptait des incidences de tous rangs (s02 :
59 longues, dont 13 seulement au rang critique), que `drop-long` retirait
des *partenaires* longs alors que l'étiquette porte sur l'arête maximale du
triangle, et qu'une observation `obs` pouvait absorber un code 2. La v7
(`9ffb871e`) répond ainsi, après une seconde vérification adverse à deux
lentilles dont tous les défauts ont été corrigés avant exécution :

- **strate CRL** : $p=K_{\max}-2$, coquille de trois sites, $q_{\min}=3$
  (sans paire antipodale, ce que l'acuité stricte garantit déjà), arête
  maximale d'au moins 1 600 unités de grille. C'est exactement la famille
  que ni Euler ni la tour ne voient, prise dans sa partie longue ;
- `--min-crl` porte sur les **triangles CRL distincts du parcours brut**, et
  exige qu'une **clé CRL retirée du catalogue** soit déclarée manquante
  (chemin `MISSING`, pas seulement la comparaison d'énumération) ;
- `--compare` publie les désaccords restreints à la strate
  (`PRUNE_DISAGREES_CRL`) ; `--inject=drop-crl` retire la strate du parcours
  élagué et doit être tué avec ce marqueur ; `drop-long` porte désormais sur
  l'arête maximale, et les deux mutants sont refusés hors `--compare` et
  rendent 3 sur une strate vide ;
- **fixture CRL gravée** (le triangle de l'audit A, $a$, $a+(1500,0,0)$,
  $a+(100,1500,0)$, trois intérieurs stricts, $K=5$) : depuis la seule ancre
  $a$, l'ancien `drop-long` par partenaires survivait, le nouveau est tué
  dans la strate ;
- lanceur : tout code ≥ 2 échoue, même pour une observation ; la ligne de
  synthèse ` n=… kmax=` est exigée ; le `--selftest` donne à chaque cas une
  vraie ligne de synthèse, si bien que la suppression de chacun des quatre
  contrôles de `run()` est détectée par son propre cas (mutation du lanceur
  vérifiée) ; les entrées sont contrôlées contre `regen_inputs.EXPECTED` ;
  la provenance couvre `src/`, `tests/gen/` et `CMakeLists.txt`, exige un
  build Release et est relue après la construction.

| cas | code | attendu | désaccords (dont CRL) | incidences CRL / triangles CRL distincts | clé CRL retirée déclarée manquante | marqueur |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `q2_lidar_s02_8000_k5_key` | 1 | 1 | — (—) | — / — | — | MISSING |
| `q2_lidar_s02_8000_k5_level` | 1 | 1 | — (—) | — / — | — | MISSING |
| `q2_lidar_s02_8000_k5_shell_dup` | 1 | 1 | — (—) | — / — | — | MISSING |
| `q3_fixture_crl_compare` | 0 | 0 | 0 (0) | 1 / 1 | oui | — |
| `q3_fixture_crl_drop_crl` | 1 | 1 | 1 (1) | 1 / 1 | oui | PRUNE_DISAGREES_CRL |
| `q3_fixture_crl_drop_long` | 1 | 1 | 1 (1) | 1 / 1 | oui | PRUNE_DISAGREES_CRL |
| `q3_fixture_eq_compare` | 0 | 0 | 0 (0) | 0 / 0 | non | — |
| `q3_fixture_eq_key` | 1 | 1 | 0 (0) | 0 / 0 | non | MISSING |
| `q3_fixture_eq_level` | 1 | 1 | 0 (0) | 0 / 0 | non | MISSING |
| `q3_fixture_eq_overprune` | 1 | 1 | 2 (0) | 0 / 0 | non | PRUNE_DISAGREES |
| `q3_fixture_eq_shell_dup` | 1 | 1 | 0 (0) | 0 / 0 | non | MISSING |
| `q3_lidar_s00_8000_k10_compare_isolated` | 0 | 0 | 0 (0) | 92 / 92 | oui | — |
| `q3_lidar_s00_8000_k10_drop_crl_isolated` | 1 | 1 | 5 (5) | 92 / 92 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s00_8000_k10_drop_long_isolated` | 1 | 1 | 5 (5) | 92 / 92 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s00_8000_k10_overprune_isolated` | 0 | obs | 0 (0) | 92 / 92 | oui | — |
| `q3_lidar_s01_8000_k10_compare_isolated` | 0 | 0 | 0 (0) | 188 / 179 | oui | — |
| `q3_lidar_s01_8000_k10_drop_crl_isolated` | 1 | 1 | 8 (8) | 188 / 179 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s01_8000_k10_drop_long_isolated` | 1 | 1 | 8 (8) | 188 / 179 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s01_8000_k10_overprune_isolated` | 0 | obs | 0 (0) | 188 / 179 | oui | — |
| `q3_lidar_s02_8000_k10_compare_isolated` | 0 | 0 | 0 (0) | 18 / 17 | oui | — |
| `q3_lidar_s02_8000_k10_drop_crl_isolated` | 1 | 1 | 3 (3) | 18 / 17 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s02_8000_k10_drop_long_isolated` | 1 | 1 | 3 (3) | 18 / 17 | oui | PRUNE_DISAGREES_CRL |
| `q3_lidar_s02_8000_k10_overprune_isolated` | 0 | obs | 0 (0) | 18 / 17 | oui | — |

**Lecture.** `STATUS=0` (sorties, `PROVENANCE.txt`, contrôle des entrées dans `results/gates_v7/`). Sur les huit sites les plus isolés, la strate CRL compte **92, 179 et 17 triangles distincts** à s00, s01, s02 (planchers 80, 150, 15) ; dans chaque cas, la clé CRL retirée du catalogue est déclarée manquante ; `drop-crl` et `drop-long` sont tués **dans la strate** (désaccords CRL sur 5, 8 et 3 sites) ; l'élagage réel ne diffère jamais de la force brute. Le mutant réaliste d'élagage (`overprune`) reste une observation sans désaccord sur ces sites, faute de cas d'égalité : sa porte est la fixture d'égalité. Échantillon toujours, `run_tower=false`.

**Portée et réserves** (vérification adverse archivée dans
`verification_juge_q3.json`, sur une source antérieure : trois
vérificateurs, lemme et arithmétique confirmés ; défauts corrigés, puis
contrelectures statiques de B sur les versions v3 et v4, également
corrigées) :
- C'est un juge d'échantillon : il ne certifie ni les sites non tirés, ni
  la tour (`run_tower=false`), et ne change aucun statut public.
- Les comptes sont des **incidences** (site tiré, triangle aigu) ; les clés
  distinctes et la fraction de la famille $p=K_{\max}-2$ couverte sont
  publiées à part (`top_keys` sur `top_population`).
- Une boule à coquille étendue n'est visible que depuis les sommets de ses
  triangles aigus de grand cercle : contre-exemple exact de centre
  $(1000,1000,1000)$, $R^{2}=25$, coquille $a=(1000,1003,1004)$,
  $b=(1005,1000,1000)$, $c=(997,1004,1000)$, $d=(997,996,1000)$, invisible
  depuis $a$. De même, une boule diamétrale de $(b,c)$ passant par $a$
  (angle droit en $a$) n'est jugée depuis $a$ ni par le juge q2 ni par le
  juge q3.
- Le juge partage avec le produit l'index des positions uniques (vérifié
  contre les points d'entrée), la structure `BallData`, le catalogue et
  `run_tower_chain` ; en mode `family`, le générateur d'entrées. Il ne
  partage ni WSPD, ni témoins, ni cœur, ni cover, ni atlas.
- Le catalogue jugé est produit à deux fils, pas à la configuration des
  reçus G4 (W48).
- Les strates de longueur sont des étiquettes en unités de grille ; elles
  ne valent des millimètres qu'en mode `file`.
- Les sites sont tirés par une permutation à graine publiée, distincte de
  celle du juge q2.

## Différentiel moteur / lots (S2, référence CPU)

Le chemin q3/q4 par lots du développeur (`a6d81f9c`, levier
`q34_batch_filter`) décide tous les rectangles puis toutes les paires en un
appel, que le GPU peut exécuter. A et B ont montré que cet appel est une
frontière de confiance (un doublon ou des masques nuls passent les identités
de masse). `batch_diff.cpp` exécute deux chaînes sur les mêmes points
(moteur, puis lots en référence CPU) et compare les deux catalogues
**complets**, champ par champ après tri canonique (clé, niveau, arité,
intérieurs, coquille), puis les ordres, le condensé FULL et les sommes
d'Euler. `--inject=drop-one` retire une boule du catalogue par lots et doit
être vu. Lanceur `run_batch_diff.sh`, sources épinglées à `111f871d`
avant exécution, produit à `a6d81f9c`, `STATUS=0`, sorties dans
`results/batch_diff_v1/` :

| cas | Kmax | boules moteur | boules lots | première différence | même condensé FULL | mêmes ordres | Euler | survivants du lot | code / attendu |
| --- | ---: | ---: | ---: | --- | --- | --- | --- | ---: | --- |
| `lidar_s00_8000_k10` | 10 | 1 567 942 | 1 567 942 | none | oui | oui | holds | 948 457 | 0 / 0 |
| `lidar_s00_8000_k5` | 5 | 342 181 | 342 181 | none | oui | oui | holds | 414 260 | 0 / 0 |
| `lidar_s01_8000_k10` | 10 | 987 076 | 987 076 | none | oui | oui | holds | 1 042 440 | 0 / 0 |
| `lidar_s01_8000_k5` | 5 | 257 607 | 257 607 | none | oui | oui | holds | 486 984 | 0 / 0 |
| `lidar_s02_8000_k10` | 10 | 1 083 173 | 1 083 173 | none | oui | oui | holds | 619 237 | 0 / 0 |
| `lidar_s02_8000_k5` | 5 | 278 809 | 278 809 | none | oui | oui | holds | 275 467 | 0 / 0 |
| `lidar_scene00_full_k10` | 10 | 5 512 670 | 5 512 670 | none | oui | oui | holds | 4 507 278 | 0 / 0 |
| `lidar_scene00_full_k5` | 5 | 1 306 696 | 1 306 696 | none | oui | oui | holds | 2 043 612 | 0 / 0 |
| `mutant_drop_one_s02_k5` | 5 | 278 809 | 278 808 | 139404 | oui | oui | holds | 275 467 | 1 / 1 |
| `uniform_8000_k10` | 10 | 3 088 676 | 3 088 676 | none | oui | oui | holds | 891 809 | 0 / 0 |

**Lecture.** Sur les trois coupes LiDAR 8k à K5 et K10, la trame entière
08/000000 sans sol à K5 et K10 (5 512 670 boules, 4 507 278 survivants du
lot à K10) et l'uniforme 8k à K10, les catalogues sont **identiques clé par
clé**, de même que les tours FULL et les sommes d'Euler ; le mutant est
détecté. Les verdicts des juges d'échantillon rendus sur le chemin moteur
valent donc pour le chemin par lots sur ces entrées. Ce n'est ni une
qualification du GPU (le lot CPU de référence partage `filter_impl` avec le
moteur ; une erreur commune aux deux ne serait vue que par les juges
indépendants et Euler), ni une mesure de temps, ni une preuve pour d'autres
nuages.

Bibliothèques : la sonde d'omission a été compilée contre les sources de
`67fce4e9` (v14) ; la première campagne du juge q2 contre celles de
`243373f6` (v15) ; la campagne v5 des deux juges et les portes v6 contre les
sources du produit de `0d5ad2e8`, reconstruites par leurs lanceurs
(`PROVENANCE.txt` de chaque dossier). Le générateur est le même dans toutes.
Le mode `b13` de la sonde publiée identifie D et T par leurs coordonnées ; la
campagne d'omission à 8k a tourné avec la version précédente de ce seul mode
(le mode d'échelle est inchangé).
