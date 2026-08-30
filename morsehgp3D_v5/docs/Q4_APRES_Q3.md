# q4 après q3 : la canopée explique un facteur 3, et le résidu est de 47 ancres

Document durable, 30 août 2026, pin `76a0ad4a`. Toutes les mesures à $s=8$, avec
le vrai prédicat `is_acute_seed`, le vrai fuseau $W_4$ et le seuil $h_4=8$.

## Ce qui se transporte de q3

La lane q4 emploie **exactement le même prédicat de seed** que q3
(`is_acute_seed` après la lentille, `generate.hpp` dans `process_anchor_q4`).
Le diagnostic de canopée établi sur q3 s'y transporte donc directement, et la
mesure le confirme — `seeds` par ancre survivante sur `terrain` :

| $n$ | canopée du dépôt ($\mathrm{coord}/8$) | canopée bornée ($\leq 3$) |
|---:|---:|---:|
| 2 000 | 6,93 | 5,62 |
| 8 000 | 18,00 | 7,87 |
| 32 000 | **41,32** | **13,89** |

La canopée pèse un facteur **3,0** à $n=32\,000$. Même mécanisme que sur q3, et
même signature : le fuseau $W_4$ est encore **plus étroit** que $W_3$
($125{,}26$ degrés contre $120$), donc une ancre entre un point suspendu et le
sol y est *a fortiori* sans témoin.

## Ce qui NE se transporte pas : q4 garde un résidu

Sur q3, avec une canopée bornée, `seeds/ancre` devient **plat** :
$5{,}41 \to 5{,}45 \to 5{,}72$, soit $n^{0{,}013}$.

Sur q4, avec la **même** canopée bornée, il croît encore :
$5{,}62 \to 7{,}87 \to 13{,}89$, soit $n^{0{,}33}$.

La cause est localisée exactement. Survie et masse par octave de longueur
d'ancre (unité $1$-NN $=2{,}83$), `terrain` $n=32\,000$, canopée bornée,
2 500 rectangles tirés par hachage :

| octave $\lvert ab\rvert$ | ancres | survie | seeds par survivante | **part de la masse** |
|---|---:|---:|---:|---:|
| [1, 2) | 179 | 100,0 % | 0,4 | 0,2 % |
| [2, 4) | 534 | 100,0 % | 1,2 | 1,6 % |
| [4, 8) | 2 056 | 93,3 % | 5,2 | 24,2 % |
| [8, 16) | 2 072 | 39,9 % | 11,3 | 22,8 % |
| [16, 32) | 96 | 5,2 % | 101,8 | 1,2 % |
| **[32, 64)** | 852 | **3,8 %** | **414,9** | **32,4 %** |
| **[64, 128)** | 874 | **1,7 %** | **479,2** | **17,6 %** |

**Les $47$ ancres survivantes des deux octaves les plus longues — $1{,}3\,\%$ des
$3\,509$ survivantes — portent $50\,\%$ de la masse de seeds.**

Or q3, à canopée bornée et à la même taille, tue **$100\,\%$** des ancres de ces
octaves ($0{,}0\,\%$ de survie à $[32,64)$ et $[64,128)$). Le résidu est donc
**propre à q4**.

## La cause du résidu

$W_4 \subset W_3$ : le citron q4 est plus étroit, donc il contient moins de
témoins. Le seuil plus bas ($h_4=8$ contre $h_3=9$) ne compense pas. Une ancre
longue accumule $6{,}4$ à $7{,}0$ témoins $W_4$ en moyenne contre un seuil de
$8$ : **elle passe de peu**, et chaque survivante coûte ensuite $415$ à $479$
seeds, chacun suivi de sa boucle de complétion.

C'est cohérent avec les pentes des reçus : sur `terrain`, `seeds_corde_tues`
croît en $n^{2{,}01}$ et `tues_profondeur[2]` en $n^{2{,}29}$, alors que les
candidats émis restent en $n^{1{,}12}$.

## Effet de la canopée du dépôt, pour comparaison

Mêmes octaves, canopée $\mathrm{coord}/8$ :

| octave | survie | part de la masse |
|---|---:|---:|
| [16, 32) | **70,8 %** | 19,4 % |
| [32, 64) | **26,2 %** | **59,5 %** |
| [64, 128) | 2,2 % | 11,6 % |

La canopée fait passer la survie de $5{,}2$ à $70{,}8\,\%$ dans $[16,32)$ et de
$3{,}8$ à $26{,}2\,\%$ dans $[32,64)$, et porte la part de masse des octaves
longues de $51\,\%$ à $90\,\%$.

## Ce que cela désigne

Le mur de q4 n'est pas diffus : il tient dans **quelques dizaines d'ancres très
longues par échantillon**, que $W_4$ rate de peu. Deux directions en découlent,
et aucune n'est encore mesurée :

1. **Renforcer le certificat d'ancre pour les ancres longues seulement.** Elles
   sont rares ($5{,}5\,\%$ des ancres) et portent la moitié du travail : un
   certificat plus cher mais plus fort y serait amorti comme nulle part ailleurs
   dans ce projet — c'est l'inverse exact de la situation de q3, où aucun
   mécanisme n'avait de base d'amortissement.
2. **Ne pas chercher côté complétion en premier.** Le rapport
   complétions/seed est plat sur `uniform` ($4{,}74$) et `eight_clusters`
   ($6{,}25$), et ne croît que sur `terrain` ($2{,}84 \to 5{,}13$, $n^{0{,}21}$) —
   soit bien moins que la croissance des seeds eux-mêmes.

## Reproduction

Sonde `octq4.cpp`, dérivée de la sonde d'octaves q3 : `alive_rectangles` avec
`lane_idx = 2`, $h_4 = \mathrm{lane\_h}(\mathrm{kQ4}, 11) = 8$, fuseau
`in_spindle(Lane::kQ4, …)`, cover au coefficient $3$ (`q4_cover_coef` sans
mutant), lentille puis `is_acute_seed` — l'ordre exact de `process_anchor_q4`.
Échantillonnage par hachage sur toute la liste vivante, $s=8$, graine 3.
Le générateur `terrain_cloud` est recopié à l'identique, seul le plafond de
canopée étant paramétré.
